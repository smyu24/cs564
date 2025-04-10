#include "catalog.h"
#include "heapfile.h"
#include "query.h"

/*
 * Deletes records from a specified relation.
 *
 * Returns:
 * 	OK on success
 * 	an error code otherwise
 */

const Status QU_Delete(const string &relation, const string &attrName,
                       const Operator op, const Datatype type,
                       const char *attrValue) {
  Status status;
  AttrDesc attrDesc;
  HeapFileScan scan(relation, status);
  if (status != OK)
    return status;

  if (attrName.empty()) {
    // unconditional delete
    status = scan.startScan(0, 0, STRING, nullptr, EQ);
    if (status != OK)
      return status;
  } else {
    // get attribute description
    status = attrCat->getInfo(relation, attrName, attrDesc);
    if (status != OK)
      return status;

    int offset = attrDesc.attrOffset;
    int length = attrDesc.attrLen;

    int intNum;
    float floatNum;

    // search HeapFileScan search conditions using our given parameters
    switch (type) {
    case STRING:
      status = scan.startScan(offset, length, type, attrValue, op);
      break;

    case INTEGER:
      intNum = atoi(attrValue);
      status = scan.startScan(offset, length, type, (char *)&intNum, op);
      break;

    case FLOAT:
      floatNum = atof(attrValue);
      status = scan.startScan(offset, length, type, (char *)&floatNum, op);
      break;
    }
    if (status != OK)
      return status;
  }

  // iterate through the heap file to find matching records to delete
  RID rid;
  while ((status = scan.scanNext(rid)) == OK) {
    status = scan.deleteRecord();
    if (status != OK) {
      return status;
    }
  }
  if (status != FILEEOF) {
    return status;
  }

  return scan.endScan();
}
