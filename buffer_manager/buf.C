#include "buf.h"
#include "error.h"
#include "page.h"
#include <cassert>
#include <errno.h>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ASSERT(c)                                                              \
  {                                                                            \
    if (!(c)) {                                                                \
      cerr << "At line " << __LINE__ << ":" << endl << "  ";                   \
      cerr << "This condition should hold: " #c << endl;                       \
      exit(1);                                                                 \
    }                                                                          \
  }

//----------------------------------------
// Constructor of the class BufMgr
//----------------------------------------

BufMgr::BufMgr(const int bufs) {
  numBufs = bufs;

  bufTable = new BufDesc[bufs];
  memset(bufTable, 0, bufs * sizeof(BufDesc));
  for (int i = 0; i < bufs; i++) {
    bufTable[i].frameNo = i;
    bufTable[i].valid = false;
  }

  bufPool = new Page[bufs];
  memset(bufPool, 0, bufs * sizeof(Page));

  int htsize = ((((int)(bufs * 1.2)) * 2) / 2) + 1;
  hashTable = new BufHashTbl(htsize); // allocate the buffer hash table

  clockHand = bufs - 1;
}

BufMgr::~BufMgr() {

  // flush out all unwritten pages
  for (int i = 0; i < numBufs; i++) {
    BufDesc *tmpbuf = &bufTable[i];
    if (tmpbuf->valid == true && tmpbuf->dirty == true) {

#ifdef DEBUGBUF
      cout << "flushing page " << tmpbuf->pageNo << " from frame " << i << endl;
#endif

      tmpbuf->file->writePage(tmpbuf->pageNo, &(bufPool[i]));
    }
  }

  delete[] bufTable;
  delete[] bufPool;
}

// allocates a free frame
// return BUFFEREXCEEDED if no space, UNIXERR if unix error, else OK
const Status BufMgr::allocBuf(int &frame) {
  int selectedFrameIdx = -1;
  int i = 0;
  // at most, iterate over the pool twice, since one buffer can be marked 0 ref
  // bit and need to come back to it
  while (i < this->numBufs * 2) {
    clockHand = (clockHand + 1) % numBufs;
    BufDesc &curr = bufTable[clockHand];

    // if valid must check, otherwise use right away
    if (curr.valid) {
      // if ref bit: set 0 and continue;
      if (curr.refbit) {
        curr.refbit = false;
        // if not pinned: use, else continue
      } else if (curr.pinCnt == 0) {
        selectedFrameIdx = clockHand;
        break;
      }
    } else {
      // invalid so use it
      selectedFrameIdx = clockHand;
      break;
    }
    i++;
  }
  // buffer pool full
  if (selectedFrameIdx == -1) {
    return Status::BUFFEREXCEEDED;
  }
  frame = selectedFrameIdx;

  BufDesc &selected = bufTable[selectedFrameIdx];
  if (selected.valid) {
    // free from hash table
    auto res = hashTable->remove(selected.file, selected.pageNo);
    assert(res == OK && "failed to remove from hashtable");
    // shouldn't return this since hashtable remove shouldn't fail
    if (res) {
      Error().print(res);
      return res;
    }
    // flush to disk if dirty
    if (selected.dirty) {
      Status res =
          selected.file->writePage(selected.pageNo, &bufPool[selectedFrameIdx]);
      if (res) {
        return UNIXERR;
      }
    }
    bufTable[selectedFrameIdx].Clear();
  }
  return OK;
}

// returns UNIXERR if unix error, BUFFEREXCEEDED if all buffer frames pinned,
// HASHTBLERROR if hash table error
const Status BufMgr::readPage(File *file, const int PageNo, Page *&page) {
  // get page, if not page error
  int frameNo;
  Status err = hashTable->lookup(file, PageNo, frameNo);
  // page not found: make a buf, read the page into the frame, add it into the
  // hash table set() on frame to set it up
  if (err == HASHNOTFOUND) {
    // alloc buf to read in the page
    err = allocBuf(frameNo);
    if (err) {
      Error().print(err);
      return err;
    }
    // read page from disk
    err = file->readPage(PageNo, &bufPool[frameNo]);
    if (err) {
      Error().print(err);
      return err;
    }

    // add page to table
    err = hashTable->insert(file, PageNo, frameNo);
    if (err) {
      return err;
    }
    // set up the frame
    bufTable[frameNo].Set(file, PageNo);
  } else {
    // page is in the pool, set the refBit, inc pinCnt and return it
    BufDesc &desc = bufTable[frameNo];
    desc.refbit = true;
    desc.pinCnt++;
  }
  // return the page
  page = &bufPool[frameNo];
  return OK;
}

const Status BufMgr::unPinPage(File *file, const int PageNo, const bool dirty) {
  // get the frame
  int frameNo;
  Status err = hashTable->lookup(file, PageNo, frameNo);
  if (err == HASHNOTFOUND) {
    Error().print(err);
    return err;
  }
  assert(!err);

  // if already 0 return PAGENOTPINNED, else dec
  BufDesc &buf = bufTable[frameNo];
  if (buf.pinCnt == 0) {
    return PAGENOTPINNED;
  }
  if (dirty) {
    buf.dirty = true;
  }
  buf.refbit = true;
  buf.pinCnt--;

  return OK;
}

const Status BufMgr::allocPage(File *file, int &pageNo, Page *&page) {
  // allocate empty page
  Status err = file->allocatePage(pageNo);
  if (err) {
    Error().print(err);
    return UNIXERR;
  }
  // get a buffer pool frame
  int frameNo;
  err = allocBuf(frameNo);
  if (err) {
    Error().print(err);
    return err;
  }

  // add frame to hash table
  err = hashTable->insert(file, pageNo, frameNo);
  if (err) {
    return err;
  }
  // set up the frame
  bufTable[frameNo].Set(file, pageNo);

  // set the page
  page = &bufPool[frameNo];
  return OK;
}

const Status BufMgr::disposePage(File *file, const int pageNo) {
  // see if it is in the buffer pool
  Status status = OK;
  int frameNo = 0;
  status = hashTable->lookup(file, pageNo, frameNo);
  if (status == OK) {
    // clear the page
    bufTable[frameNo].Clear();
  }
  status = hashTable->remove(file, pageNo);

  // deallocate it in the file
  return file->disposePage(pageNo);
}

const Status BufMgr::flushFile(const File *file) {
  Status status;

  for (int i = 0; i < numBufs; i++) {
    BufDesc *tmpbuf = &(bufTable[i]);
    if (tmpbuf->valid == true && tmpbuf->file == file) {

      if (tmpbuf->pinCnt > 0)
        return PAGEPINNED;

      if (tmpbuf->dirty == true) {
#ifdef DEBUGBUF
        cout << "flushing page " << tmpbuf->pageNo << " from frame " << i
             << endl;
#endif
        if ((status = tmpbuf->file->writePage(tmpbuf->pageNo, &(bufPool[i]))) !=
            OK)
          return status;

        tmpbuf->dirty = false;
      }

      hashTable->remove(file, tmpbuf->pageNo);

      tmpbuf->file = NULL;
      tmpbuf->pageNo = -1;
      tmpbuf->valid = false;
    }

    else if (tmpbuf->valid == false && tmpbuf->file == file)
      return BADBUFFER;
  }

  return OK;
}

void BufMgr::printSelf(void) {
  BufDesc *tmpbuf;

  cout << endl << "Print buffer...\n";
  for (int i = 0; i < numBufs; i++) {
    tmpbuf = &(bufTable[i]);
    cout << i << "\t" << (char *)(&bufPool[i])
         << "\tpinCnt: " << tmpbuf->pinCnt;

    if (tmpbuf->valid == true)
      cout << "\tvalid\n";
    cout << endl;
  };
}
