#include "catalog.h"
#include "error.h"
#include "heapfile.h"
#include "query.h"
#include "stdlib.h"

// forward declaration
const Status ScanSelect(const string &result, const int projCnt,
                        const AttrDesc projNames[], const AttrDesc *attrDesc,
                        const Operator op, const char *filter,
                        const int reclen);

/*
 * Selects records from the specified relation.
 *
 * Returns:
 * 	OK on success
 * 	an error code otherwise
 */

const Status QU_Select(const string &result, const int projCnt,
                       const attrInfo projNames[], const attrInfo *attr,
                       const Operator op, const char *attrValue) {
  // Qu_Select sets up things and then calls ScanSelect to do the actual work
  cout << "Doing QU_Select " << endl;
  Status status;

  // get attr descriptions
  AttrDesc attrDescArray[projCnt];
  for (int i = 0; i < projCnt; i++) {
    status = attrCat->getInfo(projNames[i].relName, projNames[i].attrName,
                              attrDescArray[i]);
    if (status != OK) {
      return status;
    }
  }

  // get attr info if not nullptr
  AttrDesc attrDesc;
  if (attr) {
    if ((status = attrCat->getInfo(attr->relName, attr->attrName, attrDesc)) !=
        OK) {
      return status;
    }
  }

  // get length of record
  int reclen = 0;
  for (int i = 0; i < projCnt; i++) {
    reclen += attrDescArray[i].attrLen;
  }

  return ScanSelect(result, projCnt, attrDescArray,
                    attr == nullptr ? nullptr : &attrDesc, op, attrValue,
                    reclen);
}

const Status ScanSelect(const string &result, const int projCnt,
                        const AttrDesc projNames[], const AttrDesc *attrDesc,
                        const Operator op, const char *filter,
                        const int reclen) {
  cout << "Doing HeapFileScan Selection using ScanSelect()" << endl;
  Status status;
  InsertFileScan resultRel(result, status);
  if (status != OK) {
    return status;
  }

  Record outputRec;
  char outputData[reclen];
  outputRec.data = (void *)outputData;
  outputRec.length = reclen;

  HeapFileScan scan(projNames[0].relName, status);
  if (status != OK) {
    return status;
  }

  if (attrDesc == nullptr) {
    status = scan.startScan(0, 0, STRING, nullptr, EQ);
  } else if (attrDesc->attrType == STRING) {
    status = scan.startScan(attrDesc->attrOffset, attrDesc->attrLen, STRING,
                            filter, op);
  } else if (attrDesc->attrType == FLOAT) {
    float f = atof(filter);
    status = scan.startScan(attrDesc->attrOffset, attrDesc->attrLen, FLOAT,
                            (char *)&f, op);
  } else if (attrDesc->attrType == INTEGER) {
    int i = atoi(filter);
    status = scan.startScan(attrDesc->attrOffset, attrDesc->attrLen, INTEGER,
                            (char *)&i, op);
  }
  if (status != OK) {
    return status;
  }

  RID rid;
  while (scan.scanNext(rid) == OK) {
    Record scanRec;
    status = scan.getRecord(scanRec);
    if (status != OK) {
      return status;
    }
    int outputOffset = 0;
    for (int i = 0; i < projCnt; i++) {
      memcpy((char *)outputRec.data + outputOffset,
             (char *)scanRec.data + projNames[i].attrOffset,
             projNames[i].attrLen);
      outputOffset += projNames[i].attrLen;
    }

    // add new record to output relation
    RID outRID;
    status = resultRel.insertRecord(outputRec, outRID);
    if (status != OK) {
      return status;
    }
  }
  return scan.endScan();
}
