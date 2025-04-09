#include "catalog.h"
#include "query.h"
#include "heapfile.h"

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

  // open file, initialize scan
  HeapFile heapFile(relation, status);
  if (status != OK) return status;

  HeapFileScan scan(relation, status);
  if (status != OK) return status;
 
  // get attribute description
  status = attrCat->getInfo(relation, attrName, attrDesc);
  if (status != OK) return status;

  int offset = attrDesc.attrOffset;
  int length = attrDesc.attrLen;

  int intNum;
	float floatNum;

  // search HeapFileScan search conditions using our given parameters
  switch(type) {
		case STRING:
			status = scan.startScan(offset, length, type, attrValue, op);
      cout << "data is string" << endl;
			break;
		
		case INTEGER:
    intNum = atoi(attrValue);
			status = scan.startScan(offset, length, type, (char *)&intNum, op);
      cout << "data is int" << endl;
			break;
		
		case FLOAT:
    floatNum = atof(attrValue);
			status = scan.startScan(offset, length, type, (char *)&floatNum, op);
      cout << "data is float" << endl;
			break;
	}

  if (status != OK) return status;
 
  // iterate through the heap file to find matching records to delete
  RID rid;
  while (scan.scanNext(rid) == OK) {
    cout << "Found matching record. Deleting." << endl;
    status = scan.deleteRecord();
    if (status != OK) return status;
  }

  status = scan.endScan();
  if (status != OK) return status;

  return status;
}
