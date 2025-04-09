#include "catalog.h"
#include "error.h"
#include "heapfile.h"
#include "query.h"

/*
 * Inserts a record into the specified relation.
 *
 * Returns:
 * 	OK on success
 * 	an error code otherwise
 */

const Status QU_Insert(const string &relation, const int attrCnt,
                       const attrInfo attrList[]) {
  Status status;
  InsertFileScan insertScan(relation, status);
  if (status != OK) {
    return status;
  }

  // reject nulls
  for (int i = 0; i < attrCnt; i++) {
    if (attrList[i].attrValue == nullptr) {
      return ATTRNOTFOUND;
    }
  }

  int relAttrCnt = 0;
  AttrDesc *relAttrInfos = nullptr;
  status = attrCat->getRelInfo(relation, relAttrCnt, relAttrInfos);
  if (status != OK) {
    return status;
  }
  if (relAttrCnt != attrCnt) {
    return ATTRTYPEMISMATCH;
  }
  int reclen = 0;
  for (int i = 0; i < attrCnt; i++) {
    reclen += relAttrInfos[i].attrLen;
  }

  Record newRec;
  newRec.data = malloc(reclen);
  memset(newRec.data, 0, reclen);
  newRec.length = reclen;
  if (!newRec.data) {
    return UNIXERR;
  }
  bool seen[attrCnt];
  memset(seen, 0, sizeof(int) * attrCnt);

  // submitted attrs may not be in order, so iterate over the relation's
  // attributes first, then find the submitted attribute
  for (int i = 0; i < attrCnt; i++) {
    for (int j = 0; j < attrCnt; j++) {
      auto &insertAttr = attrList[j];
      if (strcmp(insertAttr.attrName, relAttrInfos[i].attrName) == 0) {
        seen[i] = true;
        if (relAttrInfos[i].attrType != insertAttr.attrType) {
          status = ATTRTYPEMISMATCH;
          goto end;
        }
        void *data = nullptr;
        int asInt;
        float asFloat;
        switch (insertAttr.attrType) {
        case INTEGER:
          asInt = atoi((char *)insertAttr.attrValue);
          data = &asInt;
          break;
        case FLOAT:
          asFloat = atof((char *)insertAttr.attrValue);
          data = &asFloat;
          break;
        default:
          data = insertAttr.attrValue;
          break;
        }
        if (data) {
          memcpy((char *)newRec.data + relAttrInfos[i].attrOffset, data,
                 relAttrInfos[i].attrLen);
        } else {
          status = ATTRTYPEMISMATCH;
          goto end;
        }
        break;
      }
    }
  }

  for (int i = 0; i < attrCnt; i++) {
    if (!seen[i]) {
      status = ATTRNOTFOUND;
      goto end;
    }
  }

  RID rid;
  status = insertScan.insertRecord(newRec, rid);
end:
  if (newRec.data) {
    free(newRec.data);
  }
  return status;
}
