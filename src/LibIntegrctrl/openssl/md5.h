#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>

#define BUFFER_SIZE 1024

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: md5sum <filename>\n");
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    unsigned char buffer[BUFFER_SIZE];
    MD5_CTX md5_context;
    MD5_Init(&md5_context);

    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        MD5_Update(&md5_context, buffer, bytes_read);
    }

    fclose(file);

    unsigned char md5_sum[MD5_DIGEST_LENGTH];
    MD5_Final(md5_sum, &md5_context);

    printf("MD5 sum of file %s: ", argv[1]);
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        printf("%02x", md5_sum[i]);
    }
    printf("\n");

    return 0;
}

