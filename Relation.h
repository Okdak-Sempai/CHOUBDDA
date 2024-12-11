
#ifndef SHINBDDA_RELATION_H
#define SHINBDDA_RELATION_H

#include "HeapFile.h"
#include "Structures.h"
#include "Record.h"


#ifdef __cplusplus
extern "C" {
#endif

struct FieldMetadata{
    const char *name;
    FieldType type;
    size_t size; // Used for single element size (int, double, char)
    size_t len; // Used for Fixed length String
};

struct Relation {
    uint64_t oid;
    const char *name;
    uint8_t is_dynamic;
    int nb_fields;
    FieldMetadata *fieldsMetadata;

    PageId *headHdrPageId;
    PageId *tailHdrPageId;
};

Relation *new_relation(const char *name, int nb_fields, FieldMetadata *fields);
size_t relation_alloc_size(const Relation *relation);
void addDataPage(Relation *rel);
PageId *getFreeDataPage(const Relation *rel, size_t record_size);
RecordId writeRecordToDataPage(const Record *record, PageId *pageId);
RecordList *getRecordsInDataPage(Relation *rel, PageId *pageId);
HeapFilePageIdList *getDataPages(const Relation *rel);

RecordId InsertRecord(const Record *record);
RecordList *GetAllRecords(Relation *rel);

void free_relation(Relation *relation);

#ifdef __cplusplus
}
#endif

#endif //SHINBDDA_RELATION_H
