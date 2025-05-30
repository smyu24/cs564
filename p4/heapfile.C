#include "error.h"
#include "heapfile.h"

// routine to create a heapfile
const Status createHeapFile(const string fileName) {
  File *file;
  Status status;
  FileHdrPage *hdrPage;
  int hdrPageNo;
  int newPageNo;
  Page *newPage;

  // try to open the file. This should return an error
  status = db.openFile(fileName, file);
  if (status == OK) {
    return FILEEXISTS;
  }
  // create and open the file
  status = db.createFile(fileName);
  if (status != OK) {
    return status;
  }
  status = db.openFile(fileName, file);
  if (status != OK) {
    return status;
  }

  // make header page
  status = bufMgr->allocPage(file, hdrPageNo, newPage);
  if (status != OK) {
    return status;
  }
  hdrPage = (FileHdrPage *)newPage;
  strncpy(hdrPage->fileName, fileName.data(),
          std::min((unsigned)fileName.size(), MAXNAMESIZE));
  // make first data page
  status = bufMgr->allocPage(file, newPageNo, newPage);
  if (status != OK) {
    bufMgr->unPinPage(file, hdrPageNo, true);
    return status;
  }
  newPage->init(newPageNo);
  hdrPage->firstPage = newPageNo;
  hdrPage->lastPage = newPageNo;
  hdrPage->pageCnt = 1;
  hdrPage->recCnt = 0;

  // unpin, flush, close
  status = bufMgr->unPinPage(file, hdrPageNo, true);
  if (status != OK) {
    return status;
  }
  status = bufMgr->unPinPage(file, newPageNo, true);
  if (status != OK) {
    return status;
  }
  status = bufMgr->flushFile(file);
  if (status != OK) {
    return status;
  }
  status = db.closeFile(file);
  if (status != OK) {
    return status;
  }
  return status;
}

// routine to destroy a heapfile
const Status destroyHeapFile(const string fileName) {
  return (db.destroyFile(fileName));
}

// constructor opens the underlying file
HeapFile::HeapFile(const string &fileName, Status &returnStatus) {
  Status status;
  Page *pagePtr;

  cout << "opening file " << fileName << endl;

  // open the file and read in the header page and the first data page
  if ((status = db.openFile(fileName, filePtr)) == OK) {
    status = filePtr->getFirstPage(headerPageNo);
    if (status != OK) {
      returnStatus = status;
      return;
    }

    // read in the header page and first data page
    bufMgr->readPage(filePtr, headerPageNo, pagePtr);
    if (status != OK) {
      returnStatus = status;
      return;
    }
    hdrDirtyFlag = false;
    headerPage = (FileHdrPage *)pagePtr;
    curPageNo = headerPage->firstPage;
    status = bufMgr->readPage(filePtr, curPageNo, curPage);
    if (status != OK) {
      returnStatus = status;
      return;
    }
    curDirtyFlag = false;
    curRec = NULLRID;
    returnStatus = OK;
  } else {
    cerr << "open of heap file failed\n";
    returnStatus = status;
  }
}

// the destructor closes the file
HeapFile::~HeapFile() {
  Status status;
  cout << "invoking heapfile destructor on file " << headerPage->fileName
       << endl;

  // see if there is a pinned data page. If so, unpin it
  if (curPage != NULL) {
    status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
    curPage = NULL;
    curPageNo = 0;
    curDirtyFlag = false;
    if (status != OK)
      cerr << "error in unpin of date page\n";
  }

  // unpin the header page
  status = bufMgr->unPinPage(filePtr, headerPageNo, hdrDirtyFlag);
  if (status != OK)
    cerr << "error in unpin of header page\n";

  // status = bufMgr->flushFile(filePtr);  // make sure all pages of the file
  // are flushed to disk if (status != OK) cerr << "error in flushFile call\n";
  // before close the file
  status = db.closeFile(filePtr);
  if (status != OK) {
    cerr << "error in closefile call\n";
    Error e;
    e.print(status);
  }
}

// Return number of records in heap file

const int HeapFile::getRecCnt() const { return headerPage->recCnt; }

// retrieve an arbitrary record from a file.
// if record is not on the currently pinned page, the current page
// is unpinned and the required page is read into the buffer pool
// and pinned.  returns a pointer to the record via the rec parameter

const Status HeapFile::getRecord(const RID &rid, Record &rec) {
  // cout<< "getRecord. record (" << rid.pageNo << "." << rid.slotNo << ")" <<
  // endl;
  Status status;
  // if there is a curr page and it doesn't match, unpin it
  if (curPage && curPageNo != rid.pageNo) {
    status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
    if (status != OK) {
      return status;
    }
    curPage = nullptr;
  } else if (curPage) {
    // page number matches so get the record
    status = curPage->getRecord(rid, rec);
    return status;
  }

  // read in the page, then get the record
  if (curPage == nullptr) {
    status = bufMgr->readPage(filePtr, rid.pageNo, curPage);
    if (status != OK) {
      return status;
    }
    curPageNo = rid.pageNo;
    curDirtyFlag = false;
  }
  status = curPage->getRecord(rid, rec);
  return status;
}

HeapFileScan::HeapFileScan(const string &name, Status &status)
    : HeapFile(name, status) {
  filter = NULL;
}

const Status HeapFileScan::startScan(const int offset_, const int length_,
                                     const Datatype type_, const char *filter_,
                                     const Operator op_) {
  if (!filter_) { // no filtering requested
    filter = NULL;
    return OK;
  }

  if ((offset_ < 0 || length_ < 1) ||
      (type_ != STRING && type_ != INTEGER && type_ != FLOAT) ||
      (type_ == INTEGER && length_ != sizeof(int) ||
       type_ == FLOAT && length_ != sizeof(float)) ||
      (op_ != LT && op_ != LTE && op_ != EQ && op_ != GTE && op_ != GT &&
       op_ != NE)) {
    return BADSCANPARM;
  }

  offset = offset_;
  length = length_;
  type = type_;
  filter = filter_;
  op = op_;

  return OK;
}

const Status HeapFileScan::endScan() {
  Status status;
  // generally must unpin last page of the scan
  if (curPage != NULL) {
    status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
    curPage = NULL;
    curPageNo = 0;
    curDirtyFlag = false;
    return status;
  }
  return OK;
}

HeapFileScan::~HeapFileScan() { endScan(); }

const Status HeapFileScan::markScan() {
  // make a snapshot of the state of the scan
  markedPageNo = curPageNo;
  markedRec = curRec;
  return OK;
}

const Status HeapFileScan::resetScan() {
  Status status;
  if (markedPageNo != curPageNo) {
    if (curPage != NULL) {
      status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
      if (status != OK)
        return status;
    }
    // restore curPageNo and curRec values
    curPageNo = markedPageNo;
    curRec = markedRec;
    // then read the page
    status = bufMgr->readPage(filePtr, curPageNo, curPage);
    if (status != OK)
      return status;
    curDirtyFlag = false; // it will be clean
  } else
    curRec = markedRec;
  return OK;
}

const Status HeapFileScan::scanNext(RID &outRid) {
  Status status = OK;
  RID tmpRid;
  int nextPageNo;
  Record rec;

  if (curPageNo == -1) {
    return FILEEOF;
  }

  // if no page, read in the first page and record
  if (curPage == nullptr) {
    if (headerPage->firstPage == -1) {
      return FILEEOF;
    }
    curPageNo = headerPage->firstPage;
    status = bufMgr->readPage(filePtr, curPageNo, curPage);
    if (status != OK) {
      return status;
    }
    curDirtyFlag = false;
    status = curPage->firstRecord(curRec);

    // if no records, unpin the page and return
    if (status == NORECORDS) {
      status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
      if (status != OK) {
        return status;
      }
      curPage = nullptr;
      curPageNo = -1;
      curDirtyFlag = false;
      return FILEEOF;
    }
    // otherwise, read first record and match it
    status = curPage->getRecord(curRec, rec);
    if (status != OK) {
      return status;
    }
    if (matchRec(rec)) {
      outRid = curRec;
      return OK;
    }
  }

  // iterate through pages until match or end of file
  while (true) {
    // next record
    status = curPage->nextRecord(curRec, tmpRid);
    if (status == OK) {
      curRec = tmpRid;
    } else if (status != ENDOFPAGE) {
      // error exists
      return status;
    } else {
      // get next page
      while (status != OK) {
        // get next page, if none then EOF
        status = curPage->getNextPage(nextPageNo);
        if (nextPageNo == -1) {
          return FILEEOF;
        }
        if (status != OK) {
          return status;
        }
        // curr page is done so unpin
        status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
        if (status != OK) {
          return status;
        }
        // read next page and first record
        curPage = nullptr;
        curPageNo = nextPageNo;
        curDirtyFlag = false;
        status = bufMgr->readPage(filePtr, curPageNo, curPage);
        if (status != OK) {
          return status;
        }
        // read in the first record of the page
        status = curPage->firstRecord(curRec);
      }
    }

    // get the current record and match against it
    status = getRecord(rec);
    if (status != OK) {
      return status;
    }
    if (matchRec(rec)) {
      outRid = curRec;
      return OK;
    }
  }
}

// returns pointer to the current record.  page is left pinned
// and the scan logic is required to unpin the page

const Status HeapFileScan::getRecord(Record &rec) {
  return curPage->getRecord(curRec, rec);
}

// delete record from file.
const Status HeapFileScan::deleteRecord() {
  Status status;

  // delete the "current" record from the page
  status = curPage->deleteRecord(curRec);
  curDirtyFlag = true;

  // reduce count of number of records in the file
  headerPage->recCnt--;
  hdrDirtyFlag = true;
  return status;
}

// mark current page of scan dirty
const Status HeapFileScan::markDirty() {
  curDirtyFlag = true;
  return OK;
}

const bool HeapFileScan::matchRec(const Record &rec) const {
  // no filtering requested
  if (!filter)
    return true;

  // see if offset + length is beyond end of record
  // maybe this should be an error???
  if ((offset + length - 1) >= rec.length)
    return false;

  float diff = 0; // < 0 if attr < fltr
  switch (type) {

  case INTEGER:
    int iattr, ifltr; // word-alignment problem possible
    memcpy(&iattr, (char *)rec.data + offset, length);
    memcpy(&ifltr, filter, length);
    diff = iattr - ifltr;
    break;

  case FLOAT:
    float fattr, ffltr; // word-alignment problem possible
    memcpy(&fattr, (char *)rec.data + offset, length);
    memcpy(&ffltr, filter, length);
    diff = fattr - ffltr;
    break;

  case STRING:
    diff = strncmp((char *)rec.data + offset, filter, length);
    break;
  }

  switch (op) {
  case LT:
    if (diff < 0.0)
      return true;
    break;
  case LTE:
    if (diff <= 0.0)
      return true;
    break;
  case EQ:
    if (diff == 0.0)
      return true;
    break;
  case GTE:
    if (diff >= 0.0)
      return true;
    break;
  case GT:
    if (diff > 0.0)
      return true;
    break;
  case NE:
    if (diff != 0.0)
      return true;
    break;
  }

  return false;
}

InsertFileScan::InsertFileScan(const string &name, Status &status)
    : HeapFile(name, status) {
  // Do nothing. Heapfile constructor will bread the header page and the first
  //  data page of the file into the buffer pool

  // Heapfile constructor will read the header page and the first
  // data page of the file into the buffer pool
  // if the first data page of the file is not the last data page of the file
  // unpin the current page and read the last page
  if ((curPage != NULL) && (curPageNo != headerPage->lastPage)) {
    status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
    if (status != OK)
      cerr << "error in unpin of data page\n";
    curPageNo = headerPage->lastPage;
    status = bufMgr->readPage(filePtr, curPageNo, curPage);
    if (status != OK)
      cerr << "error in readPage \n";
    curDirtyFlag = false;
  }
}

InsertFileScan::~InsertFileScan() {
  Status status;
  // unpin last page of the scan
  if (curPage != NULL) {
    status = bufMgr->unPinPage(filePtr, curPageNo, true);
    curPage = NULL;
    curPageNo = 0;
    if (status != OK)
      cerr << "error in unpin of data page\n";
  }
}

// Insert a record into the file
const Status InsertFileScan::insertRecord(const Record &rec, RID &outRid) {
  Page *newPage;
  int newPageNo;
  Status status;
  RID rid;

  // check for very large records
  if ((unsigned int)rec.length > PAGESIZE - DPFIXED) {
    // will never fit on a page, so don't even bother looking
    return INVALIDRECLEN;
  }

  // read last page in if no current page
  if (curPage == nullptr) {
    status = bufMgr->readPage(filePtr, headerPage->lastPage, curPage);
    if (status != OK) {
      return status;
    }
    curPageNo = headerPage->lastPage;
  }

  status = curPage->insertRecord(rec, rid);
  if (status != OK && status != NOSPACE) {
    return status;
  }
  if (status == NOSPACE) {
    // allocate and insert a new page, unpin current page, insert new
    status = bufMgr->allocPage(filePtr, newPageNo, newPage);
    if (status != OK) {
      return status;
    }
    // link new page
    newPage->init(newPageNo);
    newPage->setNextPage(-1);
    int tmpPageNo;
    curPage->getNextPage(tmpPageNo);
    curPage->setNextPage(newPageNo);
    newPage->setNextPage(tmpPageNo);
    if (tmpPageNo == -1) {
      headerPage->lastPage = newPageNo;
    }
    // unpin current page and move to new page
    headerPage->pageCnt++;
    status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
    if (status != OK) {
      return status;
    }
    curPage = newPage;
    curPageNo = newPageNo;
    // insert record into the new page
    status = curPage->insertRecord(rec, rid);
    if (status != OK) {
      return status;
    }
  }

  headerPage->recCnt++;
  outRid = rid;
  curDirtyFlag = true;
  hdrDirtyFlag = true;
  return OK;
}
