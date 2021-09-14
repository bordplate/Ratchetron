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

static u32 BUFFER_SIZE_FTP;

static u32 BUFFER_SIZE;
static u32 BUFFER_SIZE_PSX;
static u32 BUFFER_SIZE_PSP;
static u32 BUFFER_SIZE_PS2;
static u32 BUFFER_SIZE_DVD;
static u32 BUFFER_SIZE_ALL;

#define MAX_PAGES   ((BUFFER_SIZE_ALL / (_64KB_ * MAX_WWW_THREADS)) + 1)

typedef struct {
	u32 total;
	u32 avail;
} _meminfo;

static u32 get_buffer_size(u8 footprint)
{
	if(footprint == USE_MC) //vsh_mc
	{
		return _3MB_;
	}

	if(footprint == 1) //MIN
	{
		return ( 320*KB);
	}
	else
	if(footprint == 2 || footprint >= 4) //MAX
	{
		return ( 1280*KB);
	}
	else
	if(footprint == 3) //MIN+
	{
		return (_512KB_);
	}
	else	//STANDARD
	{
		return ( 896*KB);
	}
}

static void set_buffer_sizes(u8 footprint)
{
	BUFFER_SIZE_ALL = get_buffer_size(footprint);
	BUFFER_SIZE_FTP = ( _128KB_);

	BUFFER_SIZE_PSP = ( _32KB_);
	BUFFER_SIZE_PS2 = ( _64KB_);
	BUFFER_SIZE_DVD = ( _64KB_);

	if(footprint == USE_MC) //vsh_mc
	{
		//BUFFER_SIZE_FTP = ( _256KB_);
	}
	else
	if(footprint == 1) //MIN
	{
		//BUFFER_SIZE	= ( _128KB_);
		BUFFER_SIZE_PSX = (  _32KB_);
	}
	else
	if(footprint == 2) //MAX
	{
		BUFFER_SIZE_FTP	= ( _256KB_);

		//BUFFER_SIZE	= ( _512KB_);
		BUFFER_SIZE_PSX = ( _256KB_);
		BUFFER_SIZE_PSP = (  _64KB_);
		BUFFER_SIZE_PS2 = ( _128KB_);
		BUFFER_SIZE_DVD = ( _192KB_);
	}
	else
	if(footprint == 3) //MIN+
	{
		//BUFFER_SIZE	= ( 320*KB);
		BUFFER_SIZE_PSX = (  _32KB_);
	}
	else
	if(footprint == 4) //MAX PS3+
	{
		//BUFFER_SIZE	= ( 1088*KB);
		BUFFER_SIZE_PSX = (  _32KB_);
	}
	else
	if(footprint == 5) //MAX PSX+
	{
		//BUFFER_SIZE	= (  368*KB);
		BUFFER_SIZE_PSX = ( _768KB_);
		BUFFER_SIZE_PSP = (  _64KB_);
	}
	else
	if(footprint == 6) //MAX BLU+
	{
		//BUFFER_SIZE	= (  368*KB);
		BUFFER_SIZE_PSX = (  _64KB_);
		BUFFER_SIZE_PSP = (  _64KB_);
		BUFFER_SIZE_DVD = ( _768KB_);
	}
	else
	if(footprint == 7) //MAX PSP+
	{
		//BUFFER_SIZE	= (  368*KB);
		BUFFER_SIZE_PSX = (  _64KB_);
		BUFFER_SIZE_PSP = ( _768KB_);
		BUFFER_SIZE_DVD = (  _64KB_);
	}
	else
	if(footprint == 8) //MAX PS2+
	{
		//BUFFER_SIZE	= (  368*KB);
		BUFFER_SIZE_PSX = (  _64KB_);
		BUFFER_SIZE_PS2 = ( _768KB_);
		BUFFER_SIZE_DVD = (  _64KB_);
	}
	else	// if(footprint == 0) STANDARD
	{
		//BUFFER_SIZE	= ( 448*KB);
		BUFFER_SIZE_PSX = ( 160*KB);
		BUFFER_SIZE_DVD = ( _192KB_);
	}


#ifdef MOUNT_ROMS
	BUFFER_SIZE = BUFFER_SIZE_ALL - (BUFFER_SIZE_PSX + BUFFER_SIZE_PSP + BUFFER_SIZE_PS2 + BUFFER_SIZE_DVD + BUFFER_SIZE_IGN);
#else
	BUFFER_SIZE = BUFFER_SIZE_ALL - (BUFFER_SIZE_PSX + BUFFER_SIZE_PSP + BUFFER_SIZE_PS2 + BUFFER_SIZE_DVD);
#endif
}
