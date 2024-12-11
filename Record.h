
#ifndef SHINBDDA_RECORD_H
#define SHINBDDA_RECORD_H

#include "Structures.h"
#include <stdint.h>
#include <stdlib.h>

#define FIELD_METADATA_INT(field_name) (FieldMetadata){.size = sizeof(int), .len = 1, .type = INT, .name = field_name}
#define FIELD_METADATA_DOUBLE(field_name) (FieldMetadata){.size = sizeof(float), .len = 1, .type = REAL, .name = field_name}
#define FIELD_METADATA_FSTRING(field_name, field_len) (FieldMetadata){.size = sizeof(char), .len = field_len, .type = FIXED_LENGTH_STRING, .name = field_name}
#define FIELD_METADATA_VARCHAR(field_name) (FieldMetadata){.size = sizeof(char), .len = 0, .type = VARCHAR, .name = field_name}
#define FIELD_SIZEOF(rel, i) ((rel)->fieldsMetadata[(i)].size * (rel)->fieldsMetadata[(i)].len)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum FieldType {
    INT,
    REAL,
    FIXED_LENGTH_STRING,
    VARCHAR,
} FieldType;

typedef struct Record {
    Relation *rel;

    uint8_t *buffer;
    struct
    {
        uint8_t *start;
        size_t length;
    } io;

    uint32_t *offsets;
    uint8_t *data;
} Record;

typedef struct RecordList
{
    Record **records;
    size_t length;
    size_t capacity;
} RecordList;

RecordList *newRecordList();
void freeRecordList(RecordList *list);
void appendRecordList(RecordList *list, Record *record);
void concatRecordList(RecordList *list1, const RecordList *list2);

Record *newRecord(Relation *rel);
void freeRecord(Record *rec);
size_t writeRecordToBuffer(const Record *record, uint8_t *buf, size_t pos);
size_t readFromBuffer(Record *record, const uint8_t *buf, size_t pos);

#define FIELD_GET_SET_PROTO(type, name_mod) \
    void write_field_##name_mod(Record *record, int col, type data); \
    type read_field_##name_mod(const Record *record, int col);

FIELD_GET_SET_PROTO(float, f32)
FIELD_GET_SET_PROTO(int, i32)

void write_field_string(Record *record, int col, const char *data, size_t len);
size_t read_field_string(const Record *record, int col, char **data, size_t *max_len);

#ifdef __cplusplus
}
#endif

#endif //SHINBDDA_RECORD_H
