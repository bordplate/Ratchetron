#define NONE -1
#define SYS_PPU_THREAD_NONE        (sys_ppu_thread_t)NONE
#define SYS_EVENT_QUEUE_NONE       (sys_event_queue_t)NONE
#define SYS_DEVICE_HANDLE_NONE     (sys_device_handle_t)NONE
#define SYS_MEMORY_CONTAINER_NONE  (sys_memory_container_t)NONE



#define STOP_THREAD_NAME 		"wwwds"

#define THREAD_PRIO_HIGH		0x000
#define THREAD_PRIO				0x400
#define THREAD_PRIO_NET			0x480
#define THREAD_PRIO_FTP			0x500
#define THREAD_PRIO_POLL		0xA00
#define THREAD_PRIO_STOP		0xB00

#define THREAD_STACK_SIZE_6KB		0x01800UL
#define THREAD_STACK_SIZE_8KB		0x02000UL
#define THREAD_STACK_SIZE_16KB		0x04000UL
#define THREAD_STACK_SIZE_24KB		0x06000UL
#define THREAD_STACK_SIZE_32KB		0x08000UL
#define THREAD_STACK_SIZE_48KB		0x0C000UL
#define THREAD_STACK_SIZE_64KB		0x10000UL
#define THREAD_STACK_SIZE_96KB		0x18000UL
#define THREAD_STACK_SIZE_128KB		0x20000UL
#define THREAD_STACK_SIZE_192KB		0x30000UL
#define THREAD_STACK_SIZE_256KB		0x40000UL

#define THREAD_STACK_SIZE_PS3MAPI_SVR	THREAD_STACK_SIZE_6KB
#define THREAD_STACK_SIZE_PS3MAPI_CLI	THREAD_STACK_SIZE_48KB

#define THREAD_STACK_SIZE_NET_ISO		THREAD_STACK_SIZE_8KB
#define THREAD_STACK_SIZE_NTFS_ISO		THREAD_STACK_SIZE_8KB

#define THREAD_STACK_SIZE_STOP_THREAD	THREAD_STACK_SIZE_6KB
#define THREAD_STACK_SIZE_INSTALL_PKG	THREAD_STACK_SIZE_6KB
#define THREAD_STACK_SIZE_POLL_THREAD	THREAD_STACK_SIZE_48KB
#define THREAD_STACK_SIZE_UPDATE_XML	THREAD_STACK_SIZE_128KB
#define THREAD_STACK_SIZE_MOUNT_GAME	THREAD_STACK_SIZE_128KB

#define SYS_PPU_THREAD_CREATE_NORMAL	0x000

#define MAX_WWW_THREADS		(8)
#define MAX_FTP_THREADS		(10)

