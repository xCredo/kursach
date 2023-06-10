#include <stdio.h>
#include "../thirdparty/ctest.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <openssl/md5.h>
#include "../src/LibIntegrctrl/libintegrctrl.h"

FileInfo file_inf[MAX_NUM_FILES];
int num_file = 0;

CTEST(md5_test, md5_hash) {
    char* filename = "test_file.txt";
    FILE* fp = fopen(filename, "w");
    fprintf(fp, "This is a test file for md5 hashing");
    fclose(fp);

    char* expected_failhash = "6d99af58f59b4df8acf82a4bb2f212ec";
    char* expected_realhash = "aa23a901ff6a79cb594cca22f1f44ab9";
    char* actual_hash = md5_hash(filename);

    ASSERT_STR(expected_realhash, actual_hash);
    // ASSERT_STR(expected_failhash, actual_hash);

    remove(filename);
    free(actual_hash);
}

CTEST(integrity_test, write_and_read_integrity_record) {
    IntegrityRecord record;
    record.id = get_new_id();
    record.name = "testfile.txt";
    record.type = "file";
    record.parent_id = 0;
    record.md5 = "6d99af58f59b4df8acf82a4bb2f212ec";

    FILE* fp = fopen(DB_FILENAME, "wb");
    write_integrity_record(fp, &record);
    fclose(fp);

    fp = fopen(DB_FILENAME, "rb");
    IntegrityRecord* read_record = read_integrity_record(fp);

    ASSERT_EQUAL(record.id, read_record->id);
    ASSERT_STR(record.name, read_record->name);
    ASSERT_STR(record.type, read_record->type);
    ASSERT_EQUAL(record.parent_id, read_record->parent_id);
    ASSERT_STR(record.md5, read_record->md5);

    fclose(fp);
    free(read_record->name);
    free(read_record->type);
    free(read_record->md5);
    free(read_record);
}