//
// Created by Vetle Hjelle on 9/14/2021.
//

#ifndef RATCHETRON_H
#define RATCHETRON_H

#include <sys/prx.h>
#include <sys/ppu_thread.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/memory.h>
#include <sys/timer.h>
#include <sys/process.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netex/net.h>
#include <netex/errno.h>
#include <netex/libnetctl.h>
#include <netex/sockinfo.h>
#include <netinet/tcp.h>
#include <sys/syscall.h>

#include <semaphore.h>

#include "types.h"

#include "ps3mapi.h"
#include "buffer_size.h"
#include "socket.h"
#include "thread.h"
#include "vsh_notify.h"

#define APIPORT		(9671)  // Last 4 digits of Clank's serial number

#define THREAD_NAME_PS3MAPI						"api_server"
#define THREAD02_NAME_PS3MAPI					"api_client"
#define THREAD_NAME_ASYNC_DATA                  "api_async_data"
#define THREAD_NAME_ASYNC_DATA_WAIT             "api_async_data_wait"

#define	MSG_OOB		0x1
#define	MSG_PEEK	0x2
#define MSG_DONTROUTE	0x4
#define MSG_EOR		0x8
#define MSG_TRUNC	0x10
#define MSG_CTRUNC	0x20
#define	MSG_WAITALL	0x40
#define	MSG_DONTWAIT	0x80
#define MSG_BCAST	0x100
#define MSG_MCAST	0x200
#define MSG_USECRYPTO	0x400
#define MSG_USESIGNATURE	0x800

sys_semaphore_t data_channel_wait_mutex;

static int active_socket[4] = {NONE, NONE, NONE, NONE}; // 0=FTP, 1=WWW, 2=PS3MAPI, 3=PS3NETSRV
static volatile u8 working = 1;

static u32 BUFFER_SIZE_PS3MAPI = (_64KB_);

static sys_ppu_thread_t thread_id_ps3mapi = SYS_PPU_THREAD_NONE;
static sys_ppu_thread_t thread_id_data_channel_wait = SYS_PPU_THREAD_NONE;
void ps3mapi_thread(__attribute__((unused)) u64 arg);
void data_channel_wait_thread(__attribute__((unused)) u64 arg);

#endif //RATCHETRON_H
