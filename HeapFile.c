#include "HeapFile.h"

#include <assert.h>

#include "BufferManager.h"

#include <stdlib.h>

HeapFileDataPage *getDataPage(PageId *pageId)
{
	HeapFileDataPage *dataPage = calloc(1, sizeof(HeapFileDataPage));
	if (dataPage == NULL)
		return NULL;

	dataPage->head = GetPage(pageId);
	dataPage->page_id = pageId;

	dataPage->directory = (SlotDirectory *)(dataPage->head + config->pagesize - sizeof(SlotDirectory));
	dataPage->entriesTail = (SlotDirectoryEntry *)(dataPage->head + (config->pagesize - sizeof(SlotDirectory)));

	return dataPage;
}

void freeDataPage(HeapFileDataPage *page, uint8_t dirty)
{
	if (!page)
		return;

	FreePage(page->page_id, dirty);
	free(page);
}

HeapFilePageIdList *newPageIdList()
{
	HeapFilePageIdList *list = malloc(sizeof(HeapFilePageIdList));
	if (list == NULL)
		return NULL;

	list->capacity = 0;
	list->length = 0;
	list->page_ids = NULL;

	return list;
}

void freePageIdList(HeapFilePageIdList *list)
{
	if (list == NULL)
		return;

	free(list->page_ids);
	free(list);
}

void appendPageIdList(HeapFilePageIdList *list, PageId *pageId)
{
	assert(list);

	if (list->length + 1 >= list->capacity)
	{
		list->capacity += 64;
		void *tmp = realloc(list->page_ids, list->capacity * sizeof *list->page_ids);
		if (!tmp)
			return;
		list->page_ids = tmp;
	}

	list->page_ids[list->length++] = pageId;
}
