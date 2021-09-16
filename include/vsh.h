#ifndef __VSH_H
#define __VSH_H

#include "common.h"

#include "../vsh/game_plugin.h"
#include "../vsh/netctl_main.h"
#include "../vsh/vsh.h"
#include "../vsh/vshnet.h"
#include "../vsh/vshmain.h"
#include "../vsh/vshcommon.h"
#include "../vsh/vshtask.h"
#include "../vsh/explore_plugin.h"
#include "../vsh/paf.h"

#define EXPLORE_CLOSE_ALL   3

static int get_game_info(void)
{
	if(IS_ON_XMB) return 0; // prevents game_plugin detection during PKG installation

	int is_ingame = View_Find("game_plugin");

	if(is_ingame)
	{
		char _game_info[0x120];
		game_interface = (game_plugin_interface *)plugin_GetInterface(is_ingame, 1);
		game_interface->gameInfo(_game_info);

		snprintf(_game_TitleID, 10, "%s", _game_info+0x04);
		snprintf(_game_Title,   63, "%s", _game_info+0x14);
	}

	return is_ingame;
}

#endif