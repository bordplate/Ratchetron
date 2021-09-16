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

#include "types.h"

#include "ps3mapi.h"
#include "buffer_size.h"
#include "socket.h"
#include "thread.h"
#include "vsh_notify.h"

#define APIPORT		(9671)  // Last 4 digits of Clank's serial number

#define THREAD_NAME_PS3MAPI						"ps3m_api_server"
#define THREAD02_NAME_PS3MAPI					"ps3m_api_client"

static int active_socket[4] = {NONE, NONE, NONE, NONE}; // 0=FTP, 1=WWW, 2=PS3MAPI, 3=PS3NETSRV
static volatile u8 working = 1;

static u32 BUFFER_SIZE_PS3MAPI = (_64KB_);

static sys_ppu_thread_t thread_id_ps3mapi = SYS_PPU_THREAD_NONE;
void ps3mapi_thread(__attribute__((unused)) u64 arg);

#endif //RATCHETRON_H
