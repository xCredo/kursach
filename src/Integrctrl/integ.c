#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <openssl/md5.h>
#include <unistd.h>
#include "../LibIntegrctrl/libintegrctrl.h"

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: integrctrl [-s|-c] <directory_path>\n");
        exit(EXIT_FAILURE);
    }
    if (strcmp(argv[1], "-s") == 0)
    {
    	FILE *db_file = fopen(DB_FILENAME, "wb");
	    if (db_file == NULL) {
	        fprintf(stderr, "Error creating database file\n");
	        exit(EXIT_FAILURE);
	    }
        save_integrity_info(db_file, argv[2], -1);
        fclose(db_file);
    }
    else if (strcmp(argv[1], "-c") == 0)
    {
        check_integrity(argv[2]);
    } 
    else
    {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}