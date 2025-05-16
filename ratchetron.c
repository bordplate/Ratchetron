//
// Created by Vetle Hjelle on 9/14/2021.
//

#include "include/ratchetron.h"
#include "include/syscall8.h"

#include <cell/pad.h>

#define PS3MAPI_RECV_SIZE  2048
#define PS3MAPI_MAX_LEN	383
#define API_REVISION 4

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
#define SRV_CMD_CONNECTED       (u8)0x01    // <CMD|B:1>
#define SRV_CMD_VERSION         (u8)0x02    // <CMD|B:1><REVISION|I:4>
#define SRV_CMD_MSG             (u8)0x03    // <CMD|B:1><SIZE|I:4><MSG|S:SIZE>
#define SRV_CMD_ERROR           (u8)0x04    // <CMD|B:1><ERROR|H:1>
#define SRV_CMD_MEMSET          (u8)0x05    // <CMD|B:1><PID|I:4><ADDR|I:4><SIZE|I:4><MEMORY:SIZE>
#define SRV_CMD_MEM_CHANGED     (u8)0x06    // <CMD|B:1><MSID|I:4><SIZE|I:4><NEWVALUE:SIZE>
#define SRV_CMD_CURRENT_GAME    (u8)0x07    // <CMD|B:1><SIZE|I:4><TITLEID:SIZE>

/// Client commands
#define CLT_CMD_NOTIFY          (u8)0x02    // <CMD|B:1><SIZE|I:4><MSG|S:SIZE>
#define CLT_CMD_LIST_PROCESSES  (u8)0x03    // <CMD|B:1>
#define CLT_CMD_MEMREAD         (u8)0x04    // <CMD|B:1><PID|I:4><ADDR|I:4><BYTES|I:4>
#define CLT_CMD_MEMSET          (u8)0x05    // <CMD|B:1><PID|I:4><ADDR|I:4><SIZE|I:4><MEMORY:SIZE>
#define CLT_CMD_TITLEID         (u8)0x06    // <CMD|B:1>
#define CLT_CMD_TITLE           (u8)0x07    // <CMD|B:1>
#define CLT_CMD_PAD_DATA        (u8)0x08    // <CMD|B:1>
#define CLT_CMD_OPEN_ASYNC_DATA (u8)0x09    // <CMD|B:1><PORT|UINT:4>
#define CLT_CMD_SUB_MEM         (u8)0x0a    // <CMD|B:1><PID|I:4><ADDR|I:4><SIZE|I:4><COND|B:1><CONDMEM:SIZE>
#define CLT_CMD_FREEZE_MEM      (u8)0x0b    // <CMD|B:1><PID|I:4><ADDR|I:4><SIZE|I:4><COND|B:1><CONDMEM:SIZE>
#define CLT_CMD_FREE_SUB        (u8)0x0c    // <CMD|B:1><MSID|I:4>
#define CLT_CMD_ENABLE_DEBUG    (u8)0x0d    // <CMD|B:1>
#define CLT_CMD_ALLOC_PAGE      (u8)0x0e    // <CMD|B:1><PID|I:4><SIZE|I:4><FLAGS|I:4><ISEXECUTABLE|I:4>
#define CLT_CMD_LV2_POKE        (u8)0x0f    // <CMD|B:1><ADDR|I:8><VALUE|I:8>

#define MEM_SUB_TYPE_FREEZE     (u8)0x01
#define MEM_SUB_TYPE_NOTIFY     (u8)0x02

#define MEM_COND_ANY            (u8)0x01
#define MEM_COND_CHANGED        (u8)0x02
#define MEM_COND_ABOVE          (u8)0x03
#define MEM_COND_BELOW          (u8)0x04
#define MEM_COND_EQUAL          (u8)0x05
#define MEM_COND_NOT_EQUAL      (u8)0x06

#define RATCHETRON_CONNECT_NOTIF 	    "Ratchetron: Client connected [%s]\r\n"
#define RATCHETRON_DISCONNECT_NOTIF     "Ratchetron: Client disconnected [%s]\r\n"

struct MemorySub {
    u32 pid;
    u32 addr;
    u32 size;
    u8 cond;
    u64 value;  // if size is larger than u64, value is a ptr
    u8 type;  // memory subscription or freeze
};

struct MemorySubContainer {
    struct MemorySub memory_sub;
    u32 id;
    u8 free;
    int sock_fd;
    u64 last_value;
    u32 tick_updated;
    u32 sent_last_tick;
    struct MemorySubContainer* fwd;
};

struct MemorySubContainer *memory_sub_area = 0;
u32 memory_sub_size = 0;

struct AsyncDataDesc {
    struct sockaddr_in sockaddr;
    int open;
    int tcp_sock_fd;
};

int reallocating = 0;
int debug_enabled = 0;

void debug_msg(const char *message) {
    if (debug_enabled == 1) {
        show_msg(message);
    }
}

// this doesn't work
void reallocate_memory_sub_area() {
    reallocating = 1;
    memory_sub_area = (struct MemorySubContainer *)realloc(memory_sub_area, memory_sub_size);

    // Fix fwd memory pointers
    struct MemorySubContainer *current_btrfly = memory_sub_area;
    while (1) {
        if (current_btrfly->fwd == NULL) {
            break;
        }

        current_btrfly->fwd = current_btrfly + 1;
        current_btrfly = current_btrfly->fwd;
    }

    reallocating = 0;
}

u32 add_memory_sub(struct MemorySub *mem, int sock_fd) {
    struct MemorySubContainer btrfly;
    btrfly.id = 0;
    btrfly.free = 0;
    btrfly.fwd = 0;
    btrfly.sock_fd = sock_fd;

    memcpy(&btrfly, mem, sizeof(struct MemorySub));

    struct MemorySubContainer *current_btrfly = memory_sub_area;
    while (1) {
        // Allocate more memory
        if ((u64)current_btrfly + sizeof(struct MemorySubContainer) > (u64)memory_sub_area + memory_sub_size) {
            debug_msg("DEBUG: Allocating more memory for MemorySubContainers");
            memory_sub_size = memory_sub_size + 1024;
            reallocate_memory_sub_area();
        }

        btrfly.fwd = (current_btrfly + 1);
        if (current_btrfly->fwd == NULL) {
            // when fwd butterfly is null it means we haven't allocated this one yet
            memcpy(current_btrfly, &btrfly, sizeof(struct MemorySubContainer));
            break;
        }

        if (current_btrfly->free == 1) {
            memcpy(current_btrfly, &btrfly, sizeof(struct MemorySubContainer));
            break;
        }

        btrfly.id++;
        current_btrfly = current_btrfly->fwd;
    }

    return btrfly.id;
}

void remove_memory_sub(u32 id) {
    struct MemorySubContainer memorySubContainer = memory_sub_area[id];
    memorySubContainer.free = 1;
    memorySubContainer.sent_last_tick = 0;
    memorySubContainer.tick_updated = 0;

    memory_sub_area[id] = memorySubContainer;
}

void remove_memory_sub_by_sock_fd(int sock_fd) {
    struct MemorySubContainer *memorySubContainer = memory_sub_area;
    do {
        if (memorySubContainer->free == 1) {  // we don't do anything with free'd stuff
            memorySubContainer = memorySubContainer->fwd;
            continue;
        }

        if (memorySubContainer->sock_fd == sock_fd) {
            remove_memory_sub(memorySubContainer->id);
        }

        if (memorySubContainer->fwd == NULL) { // this means we reached the end
            break;
        }

        memorySubContainer = memorySubContainer->fwd;
    } while (1);
}

static void async_data_handle(u64 sa) {
    char buffer[PS3MAPI_RECV_SIZE + 1];

    struct AsyncDataDesc *desc = (struct AsyncDataDesc *)sa;

    struct sockaddr_in sockaddr = desc->sockaddr;
    int owning_sock = desc->tcp_sock_fd;

    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(sockaddr.sin_addr.s_addr), str, INET_ADDRSTRLEN);

    sprintf(buffer, "DEBUG: Data channel opening with: %s:%d", str, htons(sockaddr.sin_port));
    debug_msg(buffer);

    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(s < 0) {
        show_msg("Failed to open data channel");
        sys_ppu_thread_exit(0);
    }

    debug_msg("DEBUG: Opened async data channel");

    const char *msg = "I was gonna tell you a joke about UDP\n";
    sendto(s, msg, strlen(msg)+1, 0, &sockaddr, sizeof(struct sockaddr_in));

    u32 tick = 0;  // at 120 ticks per second it would take 414 days for this to overflow
    while (desc->open == 1) {
        if (reallocating == 1) {  // wait for reallocation to finish
            continue;
        }

        struct MemorySubContainer *memorySubContainer = memory_sub_area;

        if (!IS_INGAME) {
            memorySubContainer->fwd = NULL;
            continue;
        }

        while (memorySubContainer->fwd != NULL && IS_INGAME) {
            if (memorySubContainer->free == 1 || memorySubContainer->sock_fd != owning_sock) {  // we don't do anything with free'd stuff or stuff that doesn't belong to this connection
                memorySubContainer = memorySubContainer->fwd;
                continue;
            }

            struct MemorySub memorySub = memorySubContainer->memory_sub;
            u64 curr_value;

            ps3mapi_get_memory(memorySub.pid, memorySub.addr, (char*)&curr_value, memorySub.size);

            int hit_conditional = 0;
            switch(memorySub.cond) {
                case MEM_COND_ANY: {
                    hit_conditional = 1;
                    break;
                }
                case MEM_COND_CHANGED: {
                    if (curr_value != memorySubContainer->last_value) {
                        hit_conditional = 1;
                    }
                    break;
                }
                case MEM_COND_ABOVE: {
                    if (curr_value > memorySub.value) {
                        hit_conditional = 1;
                    }
                    break;
                }
                case MEM_COND_BELOW: {
                    if (curr_value < memorySub.value) {
                        hit_conditional = 1;
                    }
                    break;
                }
                case MEM_COND_EQUAL: {
                    if (curr_value == memorySub.value) {
                        hit_conditional = 1;
                    }
                    break;
                }
                case MEM_COND_NOT_EQUAL: {
                    if (curr_value != memorySub.value) {
                        hit_conditional = 1;
                    }
                    break;
                }
                default: {
                    break;
                }
            }

            if (hit_conditional == 1) {
                if (memorySub.type == MEM_SUB_TYPE_FREEZE) {
                    ps3mapi_patch_process(memorySub.pid, memorySub.addr, (const char*)&memorySub.value, memorySub.size);
                } else {
                    memorySubContainer->tick_updated = tick;
                    memorySubContainer->sent_last_tick = tick;

                    u8 resp[22] = {SRV_CMD_MEM_CHANGED};
                    memcpy(&resp[1], &memorySubContainer->id, sizeof(memorySubContainer->id));
                    memcpy(&resp[5], &memorySub.size, sizeof(memorySub.size));
                    memcpy(&resp[9], &memorySubContainer->tick_updated, sizeof(memorySubContainer->tick_updated));
                    memcpy(&resp[13], &curr_value, memorySub.size);

                    sendto(s, &resp, sizeof(resp), 0, &sockaddr, sizeof(struct sockaddr_in));
                }
            }

            // since we're sending over UDP we just remind the client what the value was, if we lost any packets
            // this will make sure the client will know
            // this should remind the client every half a second (current_tick - last_sent_tick > 60)
            if (memorySubContainer->memory_sub.type == MEM_SUB_TYPE_NOTIFY && tick - memorySubContainer->sent_last_tick > 60) {
                memorySubContainer->sent_last_tick = tick;

                u8 resp[22] = {SRV_CMD_MEM_CHANGED};
                memcpy(&resp[1], &memorySubContainer->id, sizeof(memorySubContainer->id));
                memcpy(&resp[5], &memorySub.size, sizeof(memorySub.size));
                memcpy(&resp[9], &memorySubContainer->tick_updated, sizeof(memorySubContainer->tick_updated));
                memcpy(&resp[13], &curr_value, memorySub.size);

                sendto(s, &resp, sizeof(resp), 0, &sockaddr, sizeof(struct sockaddr_in));
            }

            memorySubContainer->last_value = curr_value;

            memorySubContainer = memorySubContainer->fwd;
        }

        sys_ppu_thread_yield();
        sys_timer_usleep(8333);  // loop should be running 120 times a second

        tick++;
    }

    free(desc);
    sys_ppu_thread_exit(0);
}

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

    struct AsyncDataDesc *async_desc = malloc(sizeof(struct AsyncDataDesc));
    memset(async_desc, 0, sizeof(struct AsyncDataDesc));

    sys_ppu_thread_t async_t_id = SYS_PPU_THREAD_NONE;

    while(connactive == 1) {
        memset(buffer, 0, sizeof(buffer));

        long received = 0;

        // Read command, 1 byte
        // Flag 0x40 is MSG_DONTWAIT
        while ((received = recv(conn_s, buffer, 1, 0x40)) > 0) {
            if (received > 0x80000000) {
                if (received != 0x80010223) {  // Just means there was no data available.
                    connactive = 0;
                    break;
                }
                continue;
            }

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

                    if (IS_INGAME) {
                        ps3mapi_get_memory(pid, address, mem, size);
                        send(conn_s, mem, size, 0);
                    }

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

                    if (size >= 1024 * 64) {
                        show_msg("Tried to set too much memory at a time. Pls don't");
                        break;
                    }

                    u32 n_bytes = 0;
                    while (n_bytes < size) {
                        n_bytes += (u32)recv(conn_s, memory, size, 0);
                    }

                    if (IS_INGAME) {
                        ps3mapi_patch_process(pid, address, memory, size);
                    }

                    break;
                }
                case CLT_CMD_LIST_PROCESSES: {
                    u32 pid_list[16];
                    { system_call_3(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_PID, (u64)pid_list); }
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

//                    struct CellPadData paddata = pad_read();
//
//                    send(conn_s, &paddata, sizeof(CellPadData), 0);

                    break;
                }
                case CLT_CMD_OPEN_ASYNC_DATA: {
                    u32 port;
                    recv(conn_s, &port, 4, 0);

                    if (async_t_id != SYS_PPU_THREAD_NONE) {
                        send(conn_s, '\x02', 1, 0);
                        break;
                    }

                    struct sockaddr_in *sa = &async_desc->sockaddr;
                    socklen_t sin_len = sizeof(struct sockaddr_in);
                    memset(sa, 0, sin_len);

                    sa->sin_family = AF_INET;
                    sa->sin_port = htons(port);
                    sa->sin_addr.s_addr = inet_addr(inet_ntoa(conn_info.remote_adr));  // dumb

                    char str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(sa->sin_addr.s_addr), str, INET_ADDRSTRLEN);

                    async_desc->open = 1;
                    async_desc->tcp_sock_fd = conn_s;

                    if (sys_ppu_thread_create(&async_t_id, async_data_handle, (u64)async_desc, THREAD_PRIO_POLL, THREAD_STACK_SIZE_24KB, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_ASYNC_DATA) == CELL_OK) {
                        send(conn_s, '\x01', 1, 0);
                    } else {
                        send(conn_s, '\x00', 1, 0);
                    }
                    break;
                }
                case CLT_CMD_SUB_MEM: {
                    u32 pid;
                    u32 addr;
                    u32 size;
                    u8 cond;
                    recv(conn_s, &pid, sizeof(pid), 0);
                    recv(conn_s, &addr, sizeof(addr), 0);
                    recv(conn_s, &size, sizeof(size), 0);
                    recv(conn_s, &cond, sizeof(cond), 0);

                    struct MemorySub memorySub;
                    memset(&memorySub, 0, sizeof(memorySub));

                    memorySub.pid = pid;
                    memorySub.addr = addr;
                    memorySub.size = size;
                    memorySub.cond = cond;
                    memorySub.type = MEM_SUB_TYPE_NOTIFY;
                    memorySub.value = 0;

                    if (size <= 8 && size != 0) {
                        recv(conn_s, &memorySub.value, size, 0);
                    } else if (size > 8) {
                        // malloc
                        send(conn_s, '\xff\xff\xff\xff', 4, 0); // can't freeze or sub to values with size above 8 at this time
                        break;
                    }

                    u32 sub_id = add_memory_sub(&memorySub, conn_s);
                    send(conn_s, &sub_id, sizeof(sub_id), 0);

                    break;
                }
                case CLT_CMD_FREEZE_MEM: { // <CMD|B:1><PID|I:4><ADDR|I:4><SIZE|I:4><COND|B:1><CONDMEM:SIZE>
                    u32 pid;
                    u32 addr;
                    u32 size;
                    u8 cond;
                    recv(conn_s, &pid, sizeof(pid), 0);
                    recv(conn_s, &addr, sizeof(addr), 0);
                    recv(conn_s, &size, sizeof(size), 0);
                    recv(conn_s, &cond, sizeof(cond), 0);

                    struct MemorySub memorySub;
                    memset(&memorySub, 0, sizeof(memorySub));

                    memorySub.pid = pid;
                    memorySub.addr = addr;
                    memorySub.size = size;
                    memorySub.cond = cond;
                    memorySub.type = MEM_SUB_TYPE_FREEZE;

                    if (size <= 8) {
                        recv(conn_s, &memorySub.value, size, 0);
                    } else {
                        // malloc
                        send(conn_s, '\xff\xff\xff\xff', 4, 0); // can't freeze or sub to values with size above 8 at this time
                        break;
                    }

                    u32 sub_id = add_memory_sub(&memorySub, conn_s);
                    send(conn_s, &sub_id, sizeof(sub_id), 0);

                    break;
                }
                case CLT_CMD_FREE_SUB: {
                    u32 mem_sub_id;
                    recv(conn_s, &mem_sub_id, sizeof(mem_sub_id), 0);

                    remove_memory_sub(mem_sub_id);

                    send(conn_s, '\x01', 1, 0);

                    break;
                }
                case CLT_CMD_ENABLE_DEBUG: {
                    debug_enabled = 1;
                    break;
                }
                default: {
                    // error of some kind, recover?
                }
            }
        }
    }

    sys_memory_free(sysmem);

    sprintf(buffer, RATCHETRON_DISCONNECT_NOTIF, inet_ntoa(conn_info.remote_adr));
    vshNotify_WithIcon(ICON_NETWORK, buffer);

    remove_memory_sub_by_sock_fd(conn_s);
    sclose(&conn_s);

    async_desc->open = 0;

    sys_ppu_thread_exit(0);
}

void ps3mapi_thread(__attribute__((unused)) u64 arg)
{
    if (memory_sub_area == NULL) {
        memory_sub_area = malloc(1024 * 32);
        memset(memory_sub_area, 0, 1024 * 32);
        memory_sub_size = 1024 * 32;
    }

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

    free(memory_sub_area);

    sys_ppu_thread_exit(0);
}
