#include "Record.h"
#include "Relation.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "BufferManager.h"

Record *newRecord(Relation *rel)
{
	assert(rel);

	Record *newRecord = calloc(1, sizeof(Record));
	if (!newRecord)
		return NULL;

	newRecord->rel = rel;

	const size_t offsets_size = (rel->nb_fields + 1) * sizeof(*newRecord->offsets);
	const size_t data_size = relation_alloc_size(rel);

	newRecord->buffer = calloc(offsets_size + data_size, sizeof *newRecord->buffer);
	if (!newRecord->buffer)
	{
		free(newRecord);
		return NULL;
	}

	newRecord->offsets = (uint32_t *)newRecord->buffer;
	for (int i = 1; i <= rel->nb_fields; i++)
		newRecord->offsets[i] = newRecord->offsets[i - 1] + FIELD_SIZEOF(rel, i - 1);

	newRecord->data = newRecord->buffer + offsets_size;

	if (rel->is_dynamic)
	{
		newRecord->io.start = newRecord->buffer;
		newRecord->io.length = data_size + offsets_size;
	}
	else
	{
		newRecord->io.start = newRecord->buffer + offsets_size;
		newRecord->io.length = data_size;
	}


	return newRecord;
}

void freeRecord(Record *rec)
{
	if (!rec)
		return;

	free(rec->buffer);
	free(rec);
}

#define FIELD_GET_SET_IMPL(data_type, name_mod, type_enum) \
	data_type read_field_##name_mod(const Record *record, int col) \
	{ \
		assert(record->rel->fieldsMetadata[col].type == type_enum);          \
		return *(data_type *)(record->data + record->offsets[col]);           \
	} \
	\
	void write_field_##name_mod(Record *record, int col, data_type data) \
	{ \
		assert(record->rel->fieldsMetadata[col].type == type_enum);          \
		*(data_type *)(record->data + record->offsets[col]) = data;           \
	}


FIELD_GET_SET_IMPL(float, f32, REAL)
FIELD_GET_SET_IMPL(int, i32, INT)

void write_field_string(Record *record, int col, const char *data, size_t len)
{
	const FieldMetadata *meta = record->rel->fieldsMetadata + col;
	assert(meta->type == VARCHAR || meta->type == FIXED_LENGTH_STRING);

	const size_t available = record->offsets[col + 1] - record->offsets[col];
	if (meta->type == FIXED_LENGTH_STRING)
		assert(available == meta->len);

	const ssize_t diff = (ssize_t)len - (ssize_t)available;

	if (meta->type == FIXED_LENGTH_STRING || diff == 0) // basic memcpy for type == VARCHAR, but only if no byte is added/removed
	{
		len = available < len ? available : len;

		memset(record->data + record->offsets[col], 0, available);
		memcpy(record->data + record->offsets[col], data, len);
	}
	else if (meta->type == VARCHAR)
	{
		size_t new_length = record->io.length + diff;

		void *tmp = realloc(record->buffer, new_length * sizeof *record->buffer);
		if (!tmp)
		{
			perror("realloc");
			return;
		}

		size_t offsets_size = (record->rel->nb_fields + 1) * sizeof(*record->offsets);

		record->buffer = tmp;
		record->io.start = record->buffer;
		record->offsets = (uint32_t *)record->buffer;
		record->data = record->buffer + offsets_size;

		//					  diff
		//                  <- ? ->
		// | f1 | ... |  f_col | ... | fn

		if (col != record->rel->nb_fields - 1)
		{
			void *dst = record->data + record->offsets[col + 1] + diff;
			const void *src = record->data + record->offsets[col + 1];
			const size_t mv_len = record->io.length - offsets_size - record->offsets[col + 1];
			memmove(dst, src, mv_len);
		}

		memcpy(record->data + record->offsets[col], data, len);
		record->io.length = new_length;
		for (size_t c = col + 1; c <= record->rel->nb_fields; c++)
			record->offsets[c] += diff;
	}
}

size_t read_field_string(const Record *record, int col, char **data, size_t *max_len)
{
	const FieldMetadata *meta = record->rel->fieldsMetadata + col;
	assert(meta->type == VARCHAR || meta->type == FIXED_LENGTH_STRING);

	const size_t data_len = record->offsets[col + 1] - record->offsets[col];
	if (meta->type == FIXED_LENGTH_STRING)
		assert(data_len == meta->len);

	*data = strndup((char *)record->data + record->offsets[col], data_len);
	if (max_len)
		*max_len = meta->type == VARCHAR ? config->pagesize : data_len;
	return data_len;
}

size_t writeRecordToBuffer(const Record *record, uint8_t *buf, size_t pos)
{
	memcpy(buf + pos, record->io.start, record->io.length);
	return record->io.length;
}

size_t readFromBuffer(Record *record, const uint8_t *buf, size_t pos)
{
	if (!record->rel->is_dynamic)
	{
		memcpy(record->io.start, buf + pos, record->io.length);
		return record->io.length;
	}

	const size_t nb_offsets = record->rel->nb_fields + 1;
	const size_t sz_offsets = nb_offsets * sizeof *record->offsets;
	memcpy(record->offsets, buf + pos, sz_offsets);
	pos += sz_offsets;

	size_t total = record->offsets[nb_offsets - 1];
	void *tmp = realloc(record->buffer, total + sz_offsets);
	if (!tmp)
		return 0;

	record->buffer = tmp;
	record->io.start = record->buffer;
	record->offsets = (uint32_t *)record->buffer;
	record->io.length = total + sz_offsets;
	record->data = record->buffer + sz_offsets;

	memcpy(record->data, buf + pos, total);
	return record->io.length;
}

RecordList *newRecordList()
{
	RecordList *list = malloc(sizeof *list);
	if (!list)
		return NULL;

	list->records = NULL;
	list->capacity = 0;
	list->length = 0;
	return list;
}

void freeRecordList(RecordList *list)
{
	if (!list)
		return;

	free(list->records);
	free(list);
}

void appendRecordList(RecordList *list, Record *record)
{
	assert(list);

	if (list->length + 1 >= list->capacity)
	{
		list->capacity += 64;
		void *tmp = realloc(list->records, list->capacity * sizeof *list->records);
		if (!tmp)
			return;
		list->records = tmp;
	}

	list->records[list->length++] = record;
}

void concatRecordList(RecordList *list1, const RecordList *list2)
{
	if (!list2)
		return;

	for (size_t i = 0; i < list2->length; i++)
		appendRecordList(list1, list2->records[i]);
}
