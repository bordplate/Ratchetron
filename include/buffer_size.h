#define SC_GET_FREE_MEM 	(352)

#define KB			     1024UL
#define   _2KB_		     2048UL
#define   _4KB_		     4096UL
#define   _6KB_		     6144UL
#define   _8KB_		     8192UL
#define  _12KB_		    12288UL
#define  _16KB_		    16384UL
#define  _32KB_		    32768UL
#define  _48KB_		    49152UL
#define  _64KB_		    65536UL
#define _128KB_		   131072UL
#define _192KB_		   196608UL
#define _256KB_		   262144UL
#define _384KB_		   393216UL
#define _512KB_		   524288UL
#define _640KB_		   655360UL
#define _768KB_		   786432UL
#define  _1MB_		0x0100000UL
#define  _2MB_		0x0200000UL
#define  _3MB_		0x0300000UL
#define _32MB_		0x2000000UL

#define USE_MC		99
#define MIN_MEM		_192KB_

#define PS3 (1<<0)
#define PS2 (1<<1)
#define PS1 (1<<2)
#define PSP (1<<3)
#define BLU (1<<4)
#define DVD (1<<5)
#define ROM (1<<6)

#define MAX_PAGES   ((BUFFER_SIZE_ALL / (_64KB_ * MAX_WWW_THREADS)) + 1)

typedef struct {
	u32 total;
	u32 avail;
} _meminfo;