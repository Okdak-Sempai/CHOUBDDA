
#include "Relation.h"

#include <assert.h>

#include "DBConfig.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "BufferManager.h"
#include "DiskManager.h"
#include "HeapFile.h"

static uint64_t oid = 0;

size_t relation_alloc_size(const Relation *relation) {
    size_t size = 0;

    for (int i = 0; i < relation->nb_fields; i++) {
        const FieldMetadata type = relation->fieldsMetadata[i];
        size += type.size * type.len;
    }

    return size;
}

Relation *new_relation(const char *name, int nb_fields, FieldMetadata *fields) {
    Relation  *rel = calloc(1, sizeof(Relation));
    if (!rel)
        return NULL;

    rel->headHdrPageId = AllocPage();
    if (!rel->headHdrPageId)
    {
        free(rel);
        return NULL;
    }

    HeapFileHdr *hdr = (HeapFileHdr *)GetPage(rel->headHdrPageId);
    hdr->has_next = 0;
    hdr->has_prev = 0;
    hdr->nb_data_pages = 0;

    rel->nb_fields = nb_fields;
    rel->name = strdup(name);
    rel->fieldsMetadata = calloc(nb_fields, sizeof(FieldMetadata));
    rel->oid = oid++;

    FreePage(rel->headHdrPageId, 1);
    rel->tailHdrPageId = rel->headHdrPageId;

    for (int i = 0; i < nb_fields; i++)
    {
        rel->fieldsMetadata[i] = fields[i];
        rel->fieldsMetadata[i].name = strdup(rel->fieldsMetadata[i].name);

        rel->is_dynamic = rel->is_dynamic || fields[i].type == VARCHAR;
    }

    return rel;
}

void free_relation(Relation *relation) {
    if (!relation)
        return;
    free((char *)relation->name);

    if (relation->fieldsMetadata)
    {
        for (int i = 0; i < relation->nb_fields; i++) {
            free((char *)relation->fieldsMetadata[i].name);
        }

        free(relation->fieldsMetadata);
    }

    free(relation);
}

void addDataPage(Relation *rel)
{
    HeapFileHdr *hdr = (HeapFileHdr *)GetPage(rel->tailHdrPageId);
    size_t cur_hdr_sz = sizeof(*hdr) + hdr->nb_data_pages * sizeof(HeapFileDataDesc); // can sizeof hdr because flexible array at the HeapFileHdr structure's end

    if (cur_hdr_sz + sizeof(HeapFileDataDesc) > config->pagesize)
    {
        assert(!hdr->has_next);

        PageId *next = AllocPage();
        if (!next)
            return;
        hdr->has_next = 1;
        hdr->next = *next;

        HeapFileHdr *new_hdr = (HeapFileHdr *)GetPage(next);
        new_hdr->has_next = 0;
        new_hdr->has_prev = 1;
        new_hdr->prev = *rel->tailHdrPageId;
        new_hdr->nb_data_pages = 0;

        FreePage(rel->tailHdrPageId, 1);
        rel->tailHdrPageId = next;

        hdr = new_hdr;
    }

    PageId *data = AllocPage();
    if (!data)
        return;

    HeapFileDataPage *sd = getDataPage(data);
    sd->directory->first_free = 0;
    sd->directory->nb_slots = 0;
    freeDataPage(sd, 1);

    hdr->pages[hdr->nb_data_pages].free = config->pagesize;
    hdr->pages[hdr->nb_data_pages].pageId = *data;

    hdr->nb_data_pages++;
    FreePage(rel->tailHdrPageId, 1);
}

PageId *getFreeDataPage(const Relation *rel, size_t record_size)
{
    PageId *next = rel->headHdrPageId;

    do
    {
        HeapFileHdr *hdr = (HeapFileHdr *)GetPage(next);
        PageId *res = NULL;

        uint8_t found = 0;
        for (size_t i = 0; i < hdr->nb_data_pages; i++)
        {
            if (record_size <= hdr->pages[i].free)
            {
                found = 1;
                res = FindPageId(hdr->pages[i].pageId);
                break;
            }
        }

        FreePage(next, 0);

        if (found)
            return res;

        next = hdr->has_next ? &hdr->next : NULL;
    } while (next);

    return NULL;
}

RecordId writeRecordToDataPage(const Record *record, PageId *pageId)
{
    HeapFileDataPage *data_page = getDataPage(pageId);
    SlotDirectory *dir = data_page->directory;
    RecordId rid;
    rid.page_id = pageId;
    rid.slot_idx = 0;

    // Search free slot (in case of record deletion)
    SlotDirectoryEntry *free_entry = NULL;
    SlotDirectoryEntry *tail = data_page->entriesTail;
    for (size_t i = 0; i < dir->nb_slots; i++, tail--)
    {
        if (tail->size_record == 0)
        {
            // May lead to fragmentation... in case of new record too small to fill completely the record
            size_t start_next = i + 1 < dir->nb_slots ? (tail - 1)->start_record : 0;
            size_t cur_max_size = start_next - tail->start_record;

            if (cur_max_size >= record->io.length)
            {
                free_entry = tail;
                rid.slot_idx = i;
                break;
            }
        }
    }

    uint8_t is_new_slot = 0;
    if (!free_entry)
    {
        // Create a new slot...
        free_entry = data_page->entriesTail - data_page->directory->nb_slots - 1;
        free_entry->start_record = data_page->directory->first_free;
        rid.slot_idx = data_page->directory->nb_slots;
        is_new_slot = 1;
    }

    free_entry->size_record = writeRecordToBuffer(record, data_page->head, free_entry->start_record);

    if (is_new_slot)
        data_page->directory->first_free += free_entry->size_record;

    data_page->directory->nb_slots++;
    freeDataPage(data_page, 1);
    return rid;
}

RecordList *getRecordsInDataPage(Relation *rel, PageId *pageId)
{
    HeapFileDataPage *data_page = getDataPage(pageId);
    RecordList *records = newRecordList();
    if (!records)
        return NULL;

    SlotDirectoryEntry *tail = data_page->entriesTail - 1;
    for (size_t i = 0; i < data_page->directory->nb_slots; i++, tail--)
    {
        if (tail->size_record == 0)
            continue;

        Record *record = newRecord(rel);
        readFromBuffer(record, data_page->head, tail->start_record);

        appendRecordList(records, record);
    }

    freeDataPage(data_page, 0);
    return records;
}

HeapFilePageIdList *getDataPages(const Relation *rel)
{
    HeapFilePageIdList *list = newPageIdList();

    PageId *curPageId = rel->headHdrPageId;
    do
    {
        HeapFileHdr *hdr = (HeapFileHdr *)GetPage(curPageId);

        for (size_t i = 0; i < hdr->nb_data_pages; i++)
            appendPageIdList(list, FindPageId(hdr->pages[i].pageId));

        FreePage(curPageId, 0);

        curPageId = hdr->has_next ? &hdr->next : NULL;
    } while (curPageId);

    return list;
}

RecordId InsertRecord(const Record *record)
{
    PageId *pageId = getFreeDataPage(record->rel, record->io.length);

    if (!pageId)
    {
        addDataPage(record->rel);
        pageId = getFreeDataPage(record->rel, record->io.length);
    }

    return writeRecordToDataPage(record, pageId);
}

RecordList *GetAllRecords(Relation *rel)
{
    HeapFilePageIdList *list = getDataPages(rel);
    RecordList *records = newRecordList();

    for (size_t i = 0; i < list->length; i++)
    {
        RecordList *intList = getRecordsInDataPage(rel, list->page_ids[i]);
        concatRecordList(records, intList);
        freeRecordList(intList);
    }

    freePageIdList(list);
    return records;
}