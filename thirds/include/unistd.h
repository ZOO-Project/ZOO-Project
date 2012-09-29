#ifndef __STRICT_AINSI__

#include <io.h>
#include <process.h>
#define mode_t int
#define strtok_r strtok_s
#define S_IRWXU _S_IREAD|_S_IWRITE
#define S_IRGRP _S_IREAD|_S_IWRITE
#define S_IROTH _S_IREAD|_S_IWRITE
#define S_IXGRP _S_IREAD|_S_IWRITE
#define S_IXOTH _S_IREAD|_S_IWRITE
#define S_IWOTH _S_IREAD|_S_IWRITE
#define S_IRUSR _S_IREAD
#define S_IXGRP _S_IREAD|_S_IWRITE
#define S_IWGRP _S_IREAD|_S_IWRITE
#define S_IWUSR _S_IREAD|_S_IWRITE

#endif
