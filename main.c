#include <sdk_version.h>
#include <cellstatus.h>
#include <cell/cell_fs.h>
#include <cell/rtc.h>
#include <cell/gcm.h>
#include <cell/pad.h>
#include <cell/pad/libpad_dbg.h>
#include <sys/vm.h>
#include <sysutil/sysutil_common.h>

#include <sys/prx.h>
#include <sys/ppu_thread.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <sys/socket.h>
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "types.h"
#include "include/timer.h"
#include "common.h"

#include "cobra/syscall8.h"
#include "vsh/game_plugin.h"
#include "vsh/netctl_main.h"
#include "vsh/vsh.h"
#include "vsh/vshnet.h"
#include "vsh/vshmain.h"
#include "vsh/vshcommon.h"
#include "vsh/vshtask.h"
#include "vsh/explore_plugin.h"
#include "vsh/paf.h"

#include "include/thread.h"

static char _game_TitleID[16]; //#define _game_TitleID  _game_info+0x04
static char _game_Title  [64]; //#define _game_Title    _game_info+0x14

SYS_MODULE_INFO(Ratchetron, 0, 1, 1);
SYS_MODULE_START(ratchetron_start);
SYS_MODULE_STOP(ratchetron_stop);
SYS_MODULE_EXIT(ratchetron_stop);


static int active_socket[4] = {NONE, NONE, NONE, NONE}; // 0=FTP, 1=WWW, 2=PS3MAPI, 3=PS3NETSRV

static volatile u8 working = 1;

static void show_msg(const char *text);
static void show_status(const char *label, const char *status);

#include "include/vpad.h"
#include "include/socket.h"
#include "include/buffer_size.h"
#include "include/vsh_notify.h"
#include "include/vsh.h"
#include "include/process.h"

#define APIPORT		(9671)  // Last 4 digits of Clank's serial number
#include "include/ps3mapi.h"

int ratchetron_start(size_t args, void *argp)
{
    sys_ppu_thread_create(&thread_id_ps3mapi, ps3mapi_thread, NULL, THREAD_PRIO, THREAD_STACK_SIZE_PS3MAPI_SVR, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_PS3MAPI);

	_sys_ppu_thread_exit(0); // remove for ccapi compatibility  // left this comment from original webman source, don't know what they're trying to say because this works fine with ccapi installed

	return SYS_PRX_RESIDENT;
}

static void ratchetron_stop_thread(u64 arg) {
 	sys_ppu_thread_sleep(2);  // Prevent unload too fast (give time to other threads to finish)

	u64 exit_code;
	if(thread_id_ps3mapi != SYS_PPU_THREAD_NONE)
	{
		sys_ppu_thread_join(thread_id_ps3mapi, &exit_code);
    }

	sys_ppu_thread_exit(0);
}

int ratchetron_stop(void)
{
	sys_ppu_thread_t t_id;
	int ret = sys_ppu_thread_create(&t_id, STOP_THREAD_NAME, NULL, THREAD_PRIO_STOP, THREAD_STACK_SIZE_STOP_THREAD, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);

	u64 exit_code;
	if (ret == 0) sys_ppu_thread_join(t_id, &exit_code);

	sys_ppu_thread_usleep(500000);

	unload_prx_module();

	_sys_ppu_thread_exit(0);

    show_msg("Ratchetron unloaded!");

	return SYS_PRX_STOP_OK;
}
