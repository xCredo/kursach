#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

int main() {
    struct stat file_info;

    // get information about the file 'example.txt'
    if (stat("example.txt", &file_info) != 0) {
        perror("Error getting file info");
        return 1;
    }

    printf("File size: %lld bytes\n", file_info.st_size);
    printf("Date created: %s", ctime(&file_info.st_mtime));
    printf("Permissions: %o\n", file_info.st_mode & 0777);

    return 0;
}