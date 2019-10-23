#ifndef OUFS_LIB_H
#define OUFS_LIB_H
#include "oufs.h"

#define MAX_PATH_LENGTH 200

// PROVIDED
void oufs_get_environment(char *cwd, char *disk_name, char *pipe_name_base);

// PROJECT 3: to implement
int oufs_format_disk(char  *virtual_disk_name, char *pipe_name_base);
int oufs_mkdir(char *cwd, char *path);
int oufs_list(char *cwd, char *path);
int oufs_rmdir(char *cwd, char *path);

#endif

