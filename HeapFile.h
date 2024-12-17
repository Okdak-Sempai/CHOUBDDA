#ifndef SHINBDDA_HEAPFILE_H
#define SHINBDDA_HEAPFILE_H

#include <stddef.h>
#include <stdint.h>
#include "PageId.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HeapFileDataDesc
{
	PageId pageId;
	uint32_t free;
} HeapFileDataDesc;

typedef struct HeapFileHdr
{
	uint8_t has_prev;
	uint8_t has_next;

	int nb_data_pages;

	PageId prev;
	PageId next;

	HeapFileDataDesc pages[];
} HeapFileHdr;

typedef struct SlotDirectory
{
	uint32_t nb_slots;
	uint32_t first_free;
} SlotDirectory;

typedef struct SlotDirectoryEntry
{
 	uint32_t start_record;
 	uint32_t size_record;
} SlotDirectoryEntry;

typedef struct RecordId
{
	uint32_t slot_idx;
	PageId *page_id;
} RecordId;

typedef struct HeapFileDataPage
{
	PageId *page_id;
	uint8_t *head;

	SlotDirectoryEntry *entriesTail;
	SlotDirectory *directory;
} HeapFileDataPage;

typedef struct HeapFilePageIdList
{
	PageId **page_ids;
	size_t length;
	size_t capacity;
} HeapFilePageIdList;

HeapFileDataPage *__getDataPage(PageId *pageId, const char *function, const char *filename, size_t line);
void __freeDataPage(HeapFileDataPage *page, uint8_t dirty, const char *function, const char *filename, size_t line);

#define getDataPage(pageId) __getDataPage(pageId, __PRETTY_FUNCTION__, __FILE__, __LINE__);
#define freeDataPage(pageId, dirty) __freeDataPage(pageId, dirty, __PRETTY_FUNCTION__, __FILE__, __LINE__);

HeapFilePageIdList *newPageIdList();
void freePageIdList(HeapFilePageIdList *list);
void appendPageIdList(HeapFilePageIdList *list, PageId *pageId);

#ifdef __cplusplus
}
#endif

#endif
