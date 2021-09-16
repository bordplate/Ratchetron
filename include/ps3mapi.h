#ifndef PS3MAPI_H
#define PS3MAPI_H

#include "syscall8.h"

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

#endif