#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#define FAILED -1

#define ISDIGIT(a)		('0' <= (unsigned char)(a) && (unsigned char)(a) <= '9')

static char _game_TitleID[16]; //#define _game_TitleID  _game_info+0x04
static char _game_Title  [64]; //#define _game_Title    _game_info+0x14

#endif /* __COMMON_H__ */

