#include <stdbool.h>
#include <sys/types.h>

#ifndef __FILE_H__
#define __FILE_H__

#define CP_LINK 1
#define CP_RECURSIVE 2
#define CP_NO_TARGET_DIR 4

int copy(const char * path, const char * dest, u_int32_t flags);
int delete(const char * path, bool recursive);
int move(const char * source, const char * destination);
void casefold(const char * folder);

#endif
