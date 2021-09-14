///////////// PS3MAPI BEGIN //////////////

#define SYSCALL8_OPCODE_PS3MAPI						0x7777

#define PS3MAPI_SERVER_VERSION						0x0120
#define PS3MAPI_SERVER_MINVERSION					0x0120

#define PS3MAPI_CORE_MINVERSION						0x0111

#define PS3MAPI_OPCODE_GET_CORE_VERSION				0x0011
#define PS3MAPI_OPCODE_GET_CORE_MINVERSION			0x0012
#define PS3MAPI_OPCODE_GET_FW_TYPE					0x0013
#define PS3MAPI_OPCODE_GET_FW_VERSION				0x0014
#define PS3MAPI_OPCODE_GET_ALL_PROC_PID				0x0021
#define PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID			0x0022
#define PS3MAPI_OPCODE_GET_PROC_BY_PID				0x0023
#define PS3MAPI_OPCODE_GET_CURRENT_PROC				0x0024
#define PS3MAPI_OPCODE_GET_CURRENT_PROC_CRIT		0x0025
#define PS3MAPI_OPCODE_GET_PROC_MEM					0x0031
#define PS3MAPI_OPCODE_SET_PROC_MEM					0x0032
#define PS3MAPI_OPCODE_GET_ALL_PROC_MODULE_PID		0x0041
#define PS3MAPI_OPCODE_GET_PROC_MODULE_NAME			0x0042
#define PS3MAPI_OPCODE_GET_PROC_MODULE_FILENAME		0x0043
#define PS3MAPI_OPCODE_LOAD_PROC_MODULE				0x0044
#define PS3MAPI_OPCODE_UNLOAD_PROC_MODULE			0x0045
#define PS3MAPI_OPCODE_UNLOAD_VSH_PLUGIN			0x0046
#define PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO			0x0047
#define PS3MAPI_OPCODE_GET_PROC_MODULE_SEGMENTS		0x0048 // TheRouletteBoi
#define PS3MAPI_OPCODE_GET_VSH_PLUGIN_BY_NAME		0x004F

#define PS3MAPI_OPCODE_GET_IDPS 					0x0081
#define PS3MAPI_OPCODE_SET_IDPS 					0x0082
#define PS3MAPI_OPCODE_GET_PSID 					0x0083
#define PS3MAPI_OPCODE_SET_PSID						0x0084
#define PS3MAPI_OPCODE_CHECK_SYSCALL				0x0091
#define PS3MAPI_OPCODE_DISABLE_SYSCALL				0x0092
#define PS3MAPI_OPCODE_PDISABLE_SYSCALL8 			0x0093
#define PS3MAPI_OPCODE_PCHECK_SYSCALL8 				0x0094
#define PS3MAPI_OPCODE_RENABLE_SYSCALLS				0x0095
#define PS3MAPI_OPCODE_REMOVE_HOOK					0x0101

#define PS3MAPI_OPCODE_SUPPORT_SC8_PEEK_POKE		0x1000

static void ps3mapi_get_process_name_by_id(u32 pid, char *name, u16 size)
{
	memset(name, 0, size);
	system_call_4(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID, (u64)pid, (u64)(u32)name);
}

/*static u32 get_current_pid(void)
{
	if(IS_INGAME)
		return GetGameProcessID();

	u32 pid_list[16];
	{system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_PID, (u64)(u32)pid_list); }

	for(int i = 0; i < 16; i++)
	{
		if(pid_list[i] > 2)
		{
			return pid_list[i];
		}
	}
	return 0;
}*/

static int ps3mapi_get_memory(u32 pid, u32 address, char *mem, u32 size)
{
    system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MEM, (u64)pid, (u64)address, (u64)(u32)mem, size);
    return (int)p1;
}

static int ps3mapi_patch_process(u32 pid, u32 address, const char *new_value, int size)
{
	system_call_6(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PROC_MEM, (u64)pid, (u64)address, (u64)(u32)new_value, (u64)size);
	return (int)p1;
}

///////////// PS3MAPI BEGIN //////////////


#define THREAD_NAME_PS3MAPI						"ps3m_api_server"
#define THREAD02_NAME_PS3MAPI					"ps3m_api_client"

#define PS3MAPI_RECV_SIZE  2048

#define PS3MAPI_MAX_LEN	383

#define API_REVISION 1

/**
 * Custom binary protocol for managing PS3MAPI. This can't end well, should probably had gone for an established
 *  protocol, but I'm not that smart.
 *
 * Protocol is very strict, doesn't have much recovery handling if the client mess things up.
 *
 * All commands are one byte, followed by whatever extra data is expected for the command.
 * Defined commands have a comment following them explaining how the commands are expected to look.
 * It's kind of defined like this:
 * <LABEL|TYPE:size>
 *
 * Size is amount of bytes, dynamic sizes like for strings and shit are specified as the label name. So like a MSG is
 *  like this:
 *  <CMD|B:1><SIZE|I:4><MSG|S:SIZE>
 *
 * Hope that's kinda clear.
 *
 * Types are defined as followed:
 *
 *  B: Byte
 *  H: Halfword (short/16-bit num)
 *  I: 32-bit integer
 *  S: String
 *
 *  Errors:
 *      0x0001: Message too long
 */

typedef unsigned char BYTE;

/// Server commands
#define SRV_CMD_CONNECTED       (BYTE)0x01    // <CMD|B:1>
#define SRV_CMD_VERSION         (BYTE)0x02    // <CMD|B:1><REVISION|I:4>
#define SRV_CMD_MSG             (BYTE)0x03    // <CMD|B:1><SIZE|I:4><MSG|S:SIZE>
#define SRV_CMD_ERROR           (BYTE)0x04    // <CMD|B:1><ERROR|H:1>

/// Client commands
#define CLT_CMD_NOTIFY          (BYTE)0x02    // <CMD|B:1><SIZE|I:4><MSG|S:SIZE>
#define CLT_CMD_LIST_PROCESSES  (BYTE)0x03    // <CMD|B:1>
#define CLT_CMD_MEMREAD         (BYTE)0x04    // <CMD|B:1><PID|I:4><ADDR|I:4><BYTES|I:4>
#define CLT_CMD_MEMSET          (BYTE)0x05    // <CMD|B:1><PID|I:4><ADDR|I:4><SIZE|I:4><MEMORY:SIZE>
#define CLT_CMD_TITLEID         (BYTE)0x06    // <CMD|B:1>
#define CLT_CMD_TITLE           (BYTE)0x07    // <CMD|B:1>
#define CLT_CMD_PAD_DATA        (BYTE)0x08    // <CMD|B:1>


#define RATCHETRON_CONNECT_NOTIF 	    "Ratchetron: Client connected [%s]\r\n"
#define RATCHETRON_DISCONNECT_NOTIF     "Ratchetron: Client disconnected [%s]\r\n"

static u32 BUFFER_SIZE_PS3MAPI = (_64KB_);

static sys_ppu_thread_t thread_id_ps3mapi = SYS_PPU_THREAD_NONE;

static void handleclient_ps3mapi(u64 conn_s_ps3mapi_p)
{
	int conn_s = (int)conn_s_ps3mapi_p;         // main communications socket

	int connactive = 1;							// whether the ps3mapi connection is active or not

	char buffer[PS3MAPI_RECV_SIZE + 1];

	sys_net_sockinfo_t conn_info;
	sys_net_get_sockinfo(conn_s, &conn_info, 1);

	char ip_address[16];

    int sock_flag = 1;
    setsockopt(conn_s, IPPROTO_TCP, TCP_NODELAY, (char *) &sock_flag, sizeof(int));

    // Start by sending connected message immediately followed by API version
    BYTE conn_cmd[6] = {SRV_CMD_CONNECTED, SRV_CMD_VERSION};
    uint32_t api_v = (uint32_t)API_REVISION;
    memcpy(&conn_cmd[2], (char*)&api_v, 4);

	send(conn_s, conn_cmd, sizeof(conn_cmd), 0);

	u8 ip_len = sprintf(ip_address, "%s", inet_ntoa(conn_info.local_adr));
	for(u8 n = 0; n < ip_len; n++) {
        if(ip_address[n] == '.') {
            ip_address[n] = ',';
        }
    }

	sprintf(buffer, RATCHETRON_CONNECT_NOTIF, inet_ntoa(conn_info.remote_adr));
    vshNotify_WithIcon(ICON_NETWORK, buffer);

    // Allocate some system memory to use as buffer when setting and getting memory from a game
    sys_addr_t sysmem = NULL;
    if (sys_memory_allocate(BUFFER_SIZE_PS3MAPI, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) != CELL_OK) {
        show_msg("Ratchetron can't allocate system memory.");
        // We should unload ourselves at this point, but unloading doesn't work for some reason.
        return;
    }

    while(connactive == 1) {
        memset(buffer, 0, sizeof(buffer));

        // Read command, 1 byte
        if (recv(conn_s, buffer, 1, 0) > 0) {
            switch(buffer[0]) {
                case CLT_CMD_NOTIFY: {
                    int msg_size = 0;
                    char msg[2048];
                    recv(conn_s, &msg_size, 4, 0);  // Read size of string

                    if (msg_size >= 2048) {
                        // Send error 0x0001
                        // Should the read-buffer be emptied at this point?
                        show_msg("Client tried to send too large message.");
                        break;
                    } else if (msg_size <= 0) {
                        show_msg("Client tried to send message with size 0.");
                    }

                    recv(conn_s, msg, msg_size, 0);
                    show_msg(msg);

                    break;
                }
                case CLT_CMD_MEMREAD: {
                    u32 pid = 0;
                    u32 address = 0;
                    u32 size = 0;
                    recv(conn_s, &pid, 4, 0);
                    recv(conn_s, &address, 4, 0);
                    recv(conn_s, &size, 4, 0);

                    char *mem = (char *)sysmem;

                    ps3mapi_get_memory(pid, address, mem, size);
                    send(conn_s, mem, size, 0);

                    break;
                }
                case CLT_CMD_MEMSET: {
                    u32 pid = 0;
                    u32 address = 0;
                    u32 size = 0;
                    char *memory = (char*)sysmem;
                    recv(conn_s, &pid, 4, 0);
                    recv(conn_s, &address, 4, 0);
                    recv(conn_s, &size, 4, 0);

                    if (size >= 2048) {
                        show_msg("Tried to set too much memory at a time. Pls don't");
                        break;
                    }

                    u32 n_bytes = 0;
                    while (n_bytes < size) {
                        n_bytes += (u32)recv(conn_s, memory, size, 0);
                    }

                    ps3mapi_patch_process(pid, address, memory, size);

                    break;
                }
                case CLT_CMD_LIST_PROCESSES: {
                    u32 pid_list[16];
                    { system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_PID, (u64)(u32)pid_list); }
                    send(conn_s, &pid_list, sizeof(pid_list), 0);
                    break;
                }
                case CLT_CMD_TITLEID: {
                    get_game_info();

                    send(conn_s, &_game_TitleID, sizeof(_game_TitleID), 0);
                    break;
                }
                case CLT_CMD_TITLE: {
                    get_game_info();

                    send(conn_s, &_game_Title, sizeof(_game_Title), 0);
                    break;
                }
                case CLT_CMD_PAD_DATA: {
                    // Don't know how to make this work globally throughout all games.
                    // Might need to load a separate SPRX directly into the game and do IPC with it.
                    // For some reason there's no easy way to get controller data in processes that aren't on-screen.

                    //CellPadData paddata = pad_read();

                    //send(conn_s, &paddata, sizeof(CellPadData), 0);

                    break;
                }
                default: {
                    // error of some kind, recover?
                }
            }

            sys_ppu_thread_yield();
        } else {
            connactive = 0;
            break;
        }
    }

    sys_memory_free(sysmem);

    sprintf(buffer, RATCHETRON_DISCONNECT_NOTIF, inet_ntoa(conn_info.remote_adr));
	vshNotify_WithIcon(ICON_NETWORK, buffer);

	sclose(&conn_s);

	sys_ppu_thread_exit(0);
}

static void ps3mapi_thread(__attribute__((unused)) u64 arg)
{
    int core_minversion = 0;
	{
        system_call_2(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_MINVERSION);
        core_minversion = (int)(p1);
    }

    //Check if ps3mapi core has a compatible min_version.
	if((core_minversion != 0) && (PS3MAPI_CORE_MINVERSION == core_minversion)) {
		int list_s = NONE;

	relisten:
		if(working) {
            list_s = slisten(APIPORT, 128);
            show_msg("Ratchetron loaded!");
        } else {
            goto end;
        }

		if(list_s < 0) {
			//sys_ppu_thread_sleep(1);
			if(working) goto relisten;
			else goto end;
		}

		active_socket[2] = list_s;

        while(working) {
            //sys_ppu_thread_usleep(100000);
            int conn_s_ps3mapi;

            if (!working) {
                goto end;
            }

            if((conn_s_ps3mapi = accept(list_s, NULL, NULL)) >= 0) {
                sys_ppu_thread_t t_id;

                if(working) {
                    sys_ppu_thread_create(&t_id, handleclient_ps3mapi, (u64)conn_s_ps3mapi, THREAD_PRIO, THREAD_STACK_SIZE_PS3MAPI_CLI, SYS_PPU_THREAD_CREATE_NORMAL, THREAD02_NAME_PS3MAPI);
                } else {
                    sclose(&conn_s_ps3mapi);
                    break;
                }
            } else if((sys_net_errno == SYS_NET_EBADF) || (sys_net_errno == SYS_NET_ENETDOWN)) {
                sclose(&list_s);

                if(working) {
                    goto relisten;
                } else {
                    break;
                }
            }
        }
end:
		sclose(&list_s);
	} else {
        vshNotify_WithIcon(ICON_ERROR, (char *)"Ratchetron: Internal PS3MAPI version not supported!");
    }

	sys_ppu_thread_exit(0);
}