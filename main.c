#include <stdio.h>
#include <assert.h>
#include <time.h>

#include "DBConfig.h"
#include "Tools_L.h"
#include "DiskManager.h"
#include "BufferManager.h"
#include "Relation.h"
#include "Record.h"

void assert_head(PageId *pageId) {
    assert(bufferManager->bufferHead->bufferPageId == pageId);
}

void random_string(char *out, size_t len)
{
    for (size_t i = 0; i < len; i++)
        out[i] = (char)(32 + rand() % (127 - 32));
}

static Record *random_record(const Relation *rel)
{
    Record *rec = newRecord(rel);

    for (int i = 0; i < rel->nb_fields; i++)
    {
        const FieldMetadata meta = rel->fieldsMetadata[i];

        switch (meta.type)
        {
        case INT:
            write_field_i32(rec, i, (rand() % 2 ? -1 : 1) * rand() % 10000);
            break;
        case REAL:
            write_field_f32(rec, i, (rand() % 2 ? -1 : 1) * rand() / 10000);
            break;
        case FIXED_LENGTH_STRING:
        {
            char *str = calloc(meta.len, meta.size);
            random_string(str, meta.len);
            write_field_string(rec, i, str, meta.len);
            free(str);
            break;
        }
        case VARCHAR:
        {
            size_t len = 2 + rand() % 32;
            char *str = calloc(len, meta.size);
            random_string(str, len);
            write_field_string(rec, i, str, len);
            free(str);
            break;
        }
        default:
            assert(0);
        }
    }

    return rec;
}

void assert_rec_eq(const Record *rec1, const Record *rec2)
{
    assert(rec1->rel == rec2->rel);
    assert(rec1->io.length == rec2->io.length && memcmp(rec1->io.start, rec2->io.start, rec1->io.length) == 0);


    for (int i = 0; i < rec1->rel->nb_fields; i++)
    {
        FieldMetadata meta = rec1->rel->fieldsMetadata[i];

        switch (meta.type)
        {
        case INT:
            assert(read_field_i32(rec1, i) == read_field_i32(rec2, i));
            break;
        case REAL:
            assert(read_field_f32(rec1, i) == read_field_f32(rec2, i));
            break;
        case FIXED_LENGTH_STRING: case VARCHAR:
            {
                char *s1, *s2;
                const size_t len1 = read_field_string(rec1, i, &s1, NULL);
                const size_t len2 = read_field_string(rec2, i, &s2, NULL);

                assert(len1 == len2);
                assert(strncmp(s1, s2, len1) == 0);
                free(s1);
                free(s2);
                break;
            }
        default:
            assert(0);
        }
    }
}

int main(int argc, char **argv)
{
    //Init process
    char* mainpath = currentDirectory();
    char* settingpath = "fichier_config.txt";
    LoadDBConfig(argv[1]);
    int diskinitreturn = diskInit(config); // Mandatory to call
    constructBufferManager();

    //Tests BufferManager
    /*
    *           // 1. Remplir les buffers
    *           // 2. Recup des buffers existants dans le truc
    *           // 3. Ajouter un nouveau buffer (verifier la disparition du tail)
    *           // 4. Changer le mode de recup
    *           // 5. Ajouter un buffer (verifier le depop du premier buffer)
    *
    *           PageId *pages[6];
    *           for (int i = 0; i < 6; i++)
    *               pages[i] = AllocPage();
    *
    *           GetPage(pages[0]);
    *           assert_head(pages[0]);
    *           GetPage(pages[1]);
    *           assert_head(pages[1]);
    *           GetPage(pages[2]);
    *           assert_head(pages[2]);
    *           assert(bufferManager->bufferHead->pin_count == 1);
    *           GetPage(pages[3]);
    *           assert_head(pages[3]);
    *
    *           GetPage(pages[2]);
    *           assert_head(pages[2]);
    *           assert(bufferManager->bufferHead->pin_count == 2);
    *           assert(bufferManager->bufferHead->next->bufferPageId == pages[3]);
    *           assert(bufferManager->bufferHead->next->next->bufferPageId == pages[1]);
    *           assert(bufferManager->bufferHead->next->next->next->bufferPageId == pages[0]);
    *           GetPage(pages[2]);
    *           assert_head(pages[2]);
    *           assert(bufferManager->bufferHead->next->bufferPageId == pages[3]);
    *           assert(bufferManager->bufferHead->next->next->bufferPageId == pages[1]);
    *           assert(bufferManager->bufferHead->next->next->next->bufferPageId == pages[0]);
    *           assert(bufferManager->bufferHead->pin_count == 3);
    *           FreePage(pages[2], 0);
    *           assert(bufferManager->bufferHead->pin_count == 2);
    *
    *           GetPage(pages[4]);
    *           assert_head(pages[4]);
    *           assert(bufferManager->bufferTail->bufferPageId != pages[0]);
    *
    *
    *           SetCurrentReplacementPolicy(POLICY_MRU);
    *
    *           uint8_t *content = GetPage(pages[5]);
    *           assert_head(pages[5]);
    *           assert(bufferManager->bufferTail->bufferPageId == pages[1]);
    *           assert(bufferManager->bufferHead->next->bufferPageId != pages[4]);
    *
    *           snprintf((char *) content, config->pagesize, "abcdefghijklmnopqrstuvwxyz");
    *           FreePage(pages[5], 1);
    *       GetPage(AllocPage());
    *
    *           FlushBuffers();
    */


    //DBConfig* config2 = ConfigInit();
    //char* testfile = createFileInDirectory(mainpath,"ZTEST",".pdf");
    //int numberoftables=10;
    //PageIdList* pageidlist = InitiatiateTables(config->dbpath,config->dm_maxfilesize,config->pagesize,numberoftables);



    //int addTablereturn = addTable(config->dbpath,config->dm_maxfilesize,config->pagesize,&numberoftables,5,pageidlist);  //Add 5 tables
    //int diskinitreturn = diskInit(config); // Mandatory to call
    //int addTablereturn = addTable(config->dbpath,config->dm_maxfilesize,config->pagesize,&numberoftables,5,pageidlist);

    //    for (int i = 0; i < pageidlist->size; i++) {
    //        printf("Élément %d : FileIdx = %d, PageIdx = %d\n",
    //               i,
    //               pageidlist->list[i].FileIdx,
    //               pageidlist->list[i].PageIdx);
    //    }


    //printf("[%s]", getPageIdFile(config,&pageidlist->list[25])); //If uncommented there's a memory leak

    //AllocPage tests
    //    PageId* test1 = AllocPage();
    //    PageId* test2 = AllocPage();
    //    PageId* test3 = AllocPage();
    //
    //
    //    DeallocPage(test3);
    //
    //
    //    PageId* test4 = AllocPage();
    //    PageId* test5 = AllocPage();
    //    PageId* test6 = AllocPage();
    //
    //    unsigned char* hello = (unsigned char*) "Hello my world";
    //    unsigned char* testprint=NULL;
    //    testprint = (unsigned char*)malloc(config->pagesize * sizeof(unsigned char));
    //    if (testprint == NULL) {perror("Erreur d'allocation mémoire");}
    //    strcpy((char*)testprint, (char*)hello);
    //
    //    unsigned char* testRead = NULL;
    //    testRead = (unsigned char*)malloc(config->pagesize*sizeof(unsigned char));
    //    if (testRead == NULL) {perror("Erreur d'allocation mémoire");}
    //    //printf("[%s]\n",testRead);
    //
    //    WritePage(test1,hello);
    //
    //    PageId* test7 = AllocPage();
    //    PageId* test8 = AllocPage();
    //    PageId* test9 = AllocPage();
    //
    //
    //    //char* get = getPageIdFile(config,test9);
    //    //printf("[%s]",get);
    //    //free(get);
    //
    //
    //    ReadPage(test1,testRead);
    //
    //    printf("[%s]",testRead);
    //
    //    free(testprint);
    //    free(testRead);
    //
    //    PageId* test10 = AllocPage();
    //    PageId* test11 = AllocPage();
    //
    //    PageId* test12 = AllocPage();
    //    DeallocPage(test1);
    //    PageId* test14 = AllocPage();
    //    PageId* test16 = AllocPage();
    //    PageId* test17 = AllocPage();
    //    DeallocPage(test8);
    //    DeallocPage(test4);
    //    DeallocPage(test2);
    //    DeallocPage(test5);
    //    PageId* test15 = AllocPage();
    //    DeallocPage(test9);
    //    DeallocPage(test10);
    //    DeallocPage(test11);
    //    DeallocPage(test6);
    //    DeallocPage(test7);
    //



    //free(config2);
    //free(testfile);


    //    printf("Line 0:[%s]\n",readData(settingpath,0));
    //    printf("Line 1:[%s]\n",readData(settingpath,1));
    //    printf("Line 2:[%s]\n",readData(settingpath,2));
    //    printf("Line 3:[%s]\n",readData(settingpath,3));
    //    printf("Line 4:[%s]\n",readData(settingpath,4));
    //    printf("\n##-GG-##\n");


    //Test Record

    // FieldMetadata fields1[20];
    // fields1[0] = FIELD_METADATA_INT("ROGER");
    // fields1[1] = FIELD_METADATA_DOUBLE("albert");
    //
    // fields1[2] = FIELD_METADATA_INT("fred");
    // fields1[3] = FIELD_METADATA_FSTRING("george", 12);
    // fields1[4] = FIELD_METADATA_DOUBLE("Patrice");
    //
    // fields1[5] = FIELD_METADATA_DOUBLE("COUISDHIOFGUHDOSIJ");
    // fields1[6] = FIELD_METADATA_VARCHAR("oui");
    // fields1[7] = FIELD_METADATA_VARCHAR("non");
    //
    // fields1[8] = FIELD_METADATA_FSTRING(";fdslfksdpoi2142o3knmok;321/;''/", 5);
    // fields1[9] = FIELD_METADATA_VARCHAR("stylo");
    // fields1[10] = FIELD_METADATA_VARCHAR("pen");
    // fields1[11] = FIELD_METADATA_INT("marteau");
    // fields1[12] = FIELD_METADATA_FSTRING("vroum", 7);
    // fields1[13] = FIELD_METADATA_INT("bleu");
    //
    // Relation *r1 = new_relation("Food",2,fields1);
    // Relation *r2 = new_relation("Cars",3,fields1 + 2);
    // Relation *r3 = new_relation("Teachers",3,fields1 + 5);
    // Relation *r4 = new_relation("Games",6,fields1 + 8);

    // Record *rec1 = newRecord(r1);
    // Record *rec2  = newRecord(r2);
    // Record *rec3  = newRecord(r3);
    //
    // write_field_i32(rec1, 0, 42);
    // assert(read_field_i32(rec1, 0) == 42);
    // write_field_f32(rec1, 1, 3.141592f);
    // assert(read_field_f32(rec1, 1) == 3.141592f);
    //
    // write_field_i32(rec2, 0, 42);
    // assert(read_field_i32(rec2, 0) == 42);
    // write_field_string(rec2, 1, "0123456789ab", 12);
    // char *str;
    // size_t max;
    // size_t len = read_field_string(rec2, 1, &str, &max);
    // assert(strncmp(str, "0123456789ab", len) == 0 && len == max && len == 12);
    // write_field_f32(rec2, 2, 2.71828f);
    // assert(read_field_f32(rec2, 2) == 2.71828f);
    //
    // write_field_f32(rec3, 0, 1.618f);
    // assert(read_field_f32(rec3, 0) == 1.618f);
    //
    // size_t base_size = rec3->io.length;
    // write_field_string(rec3, 1, "1234", 4);
    // len = read_field_string(rec3, 1, &str, &max);
    // assert(strncmp(str, "1234", len) == 0 && len == 4 && max >= config->pagesize && rec3->io.length == base_size + 4);
    //
    // base_size = rec3->io.length;
    // write_field_string(rec3, 1, "bonjour", 7);
    // len = read_field_string(rec3, 1, &str, &max);
    // assert(strncmp(str, "bonjour", len) == 0 && len == 7 && rec3->io.length == base_size + 3);
    //
    // base_size = rec3->io.length;
    // write_field_string(rec3, 1, "b", 1);
    // len = read_field_string(rec3, 1, &str, &max);
    // assert(strncmp(str, "b", len) == 0 && len == 1 && rec3->io.length == base_size - 6);
    //
    //
    // write_field_string(rec3, 2, "654", 3);
    // len = read_field_string(rec3, 2, &str, &max);
    // assert(strncmp(str, "654", len) == 0 && len == 3);


    srand(time(NULL));

    // uint8_t *buf = calloc(config->pagesize, sizeof(uint8_t));
    // Record *rec = calloc(1, sizeof(Record));
    // size_t pos = 0;
    //
    // for (size_t i = 0; i < 10; i++)
    // {
    //     char string[32];
    //
    //     Record *rec = newRecord(r4);
    //     /*
    //     *    [8] = FSTRING (5)
    //     *    [9] = VARCHAR
    //     *    [10] = VARCHAR
    //     *    [11] = INT
    //     *    [12] = FSTRING (7)
    //     *    [13] = INT
    //     */
    //
    //
    //     size_t true_total = rec->io.length;
    //     random_string(string, 5);
    //     write_field_string(rec, 0, str, 5);
    //
    //     size_t string_len = 1 + rand() % sizeof(string);
    //     true_total += string_len;
    //     random_string(string, string_len);
    //     write_field_string(rec, 1, string, string_len);
    //
    //     string_len = 1 + rand() % sizeof(string);
    //     true_total += string_len;
    //     random_string(string, string_len);
    //     write_field_string(rec, 2, string, string_len);
    //
    //     write_field_i32(rec, 3, rand());
    //
    //     random_string(string, 7);
    //     write_field_string(rec, 4, string, 7);
    //
    //     write_field_i32(rec, 5, rand());
    //
    //     Record *res = newRecord(r4);
    //
    //     const size_t shift = writeRecordToBuffer(rec, buf, pos);
    //     const size_t read_shift = readFromBuffer(res, buf, pos);
    //     assert(shift == true_total);
    //     assert(read_shift == true_total);
    //
    //     assert_rec_eq(rec, res);
    //
    //     pos += shift;
    //
    //     freeRecord(rec);
    //     freeRecord(res);
    // }

    // free(rec);
    // free(buf);
    // freeRecord(rec1);
    // freeRecord(rec2);
    // freeRecord(rec3);

    // free_relation(r1);
    // free_relation(r2);
    // free_relation(r3);
    // free_relation(r4);


    // TEST Relation insertion
    FieldMetadata fields[] = {
        FIELD_METADATA_VARCHAR("f1"),
        FIELD_METADATA_INT("f2"),
        FIELD_METADATA_DOUBLE("f3"),
        FIELD_METADATA_VARCHAR("f4"),
        FIELD_METADATA_VARCHAR("f5"),
        FIELD_METADATA_FSTRING("f6", 5),
        FIELD_METADATA_DOUBLE("f7"),
        FIELD_METADATA_VARCHAR("f8"),
    };
    Relation *rel = new_relation("RELATION", sizeof fields / sizeof fields[0], fields);
    Record *records[10];

    for (size_t i = 0; i < sizeof records / sizeof records[0]; i++)
    {
        records[i] = newRecord(rel);
        InsertRecord(records[i]);
    }

    RecordList *list = GetAllRecords(rel);
    assert(list->length == sizeof records / sizeof records[0]);
    for (size_t i = 0; i < list->length; i++)
        assert_rec_eq(records[i], list->records[i]);

    freeRecordList(list);
    free_relation(rel);

    //End process
    SaveState();
    free(mainpath);
    clearBufferManager();
    DBFree();
    diskFREE();

    return 0;
}
