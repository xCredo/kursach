#ifndef LIBINTEGRCTRL_H
#define LIBINTEGRCTRL_H
#include <string.h>

void calculate_md5sum(char *filename, unsigned char *md5sum);
int is_directory(const char *path);
void save_integrity_info(char *dir_path);
void check_integrity(char *dir_path);

#endif 