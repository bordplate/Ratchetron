//------------
/* Based on PHTNC's code to write VSH Notify notifications with icons */

enum rco_icons
{
	ICON_CAUTION	 = 3,
	ICON_NETWORK	 = 5,
	ICON_PSN		 = 18,
	ICON_CHECK		 = 22,
	ICON_EXCLAMATION = 23,
	ICON_NOTIFY		 = 29,
	ICON_ERROR		 = 35,
	ICON_MUSIC		 = 30,
	ICON_VIDEO		 = 32,
	ICON_GAME		 = 33,
	ICON_PS2_DISC	 = 43,
	ICON_PSP_UMD	 = 48,
	ICON_WAIT		 = 49,
	ICON_MOUNT		 = 50,
};

#define MAX_RCO_IMAGES	51

static int32_t vshNotify_WithIcon(u8 icon_id, const char *msg)
{
	const char *rco_images[MAX_RCO_IMAGES] = {
								// system_plugin icons
								"tex_notification_info",			//0
								"tex_notification_friend",			//1
								"tex_notification_headset",			//2
								"tex_notification_caution",			//3
								"tex_notification_keypad",			//4
								"tex_notification_mediasever",		//5
								"tex_temporary_icon",				//6
								"tex_notification_psbutton_insensitive",	//7
								"tex_notification_settings",		//8
								"tex_notification_trophy_bronze",	//9
								"tex_notification_trophy_silver",	//10
								"tex_notification_trophy_gold",		//11
								"tex_notification_trophy_platinum",	//12
								"tex_pointer_hand",					//13
								"tex_pointer_pen",					//14
								"tex_pointer_arrow",				//15
								"tex_pointer_grab",					//16
								"tex_arrow_right",					//17
								// explore_plugin icons
								"tex_psn_big",						//18
								"tex_psplus_icon",					//19
								"tex_Signing_In",					//20
								"tex_new_ws",						//21
								"tex_check_ws",						//22
								"tex_urgent_ws",					//23
								"item_tex_cam_icon",				//24
								"item_tex_Profile_LevelIcon",		//25
								"item_tex_ps_store",				//26
								"tex_album_icon",					//27
								"item_tex_Players",					//28
								"tex_indi_NewRoom",					//29
								"tex_music",						//30
								"tex_photo",						//31
								"tex_video",						//32
								"tex_game",							//33
								"tex_lock_icon",					//34
								"tex_indi_Sign_out",				//35-Error icon
								"tex_indi_Message",					//36
								"tex_Message_Sent",					//37
								"item_tex_CardBallon",				//38
								"tex_loading_icon",					//39
								"tex_Avatar_Default",				//40
								"item_tex_disc_bd",					//41-PS3
								"item_tex_disc_icon",				//42-CD
								"item_tex_disc_cd_ps2",				//43-PS2
								"item_tex_disc_ps1",				//44-PSX
								"item_tex_disc_bd_contents",		//45-BD
								"item_tex_disc_dvd",				//46-DVD
								"game_tex_disc_unknown",			//47
								"item_tex_psp_icon",				//48
								"tex_indi_AFK",						//49
								"tex_go_game",						//50
							};

	if(icon_id >= MAX_RCO_IMAGES) icon_id = 0;
	char *plugin = (char*)"explore_plugin";
	char *tex = (char*)rco_images[icon_id];

	if(icon_id < 18) plugin = (char*)"system_plugin";

	if(IS_INGAME)
		return vshtask_notify(msg);

	uint32_t _plugin = View_Find(plugin);
	if (_plugin <= 0)
		return FAILED;

	int len = strlen(msg); if(len >= 200) len = 198;

	wchar_t message[len + 2];

	mbstowcs((wchar_t *)message, (const char *)msg, len + 1);  //size_t stdc_FCAC2E8E(wchar_t *dest, const char *src, size_t max)

	int teximg, dummy = 0;
	LoadRCOTexture(&teximg, _plugin, tex);
	return NotifyWithTexture(0, tex, 0, &teximg, &dummy, "", "", 0, message, 0, 0, 0);
}
//------------

static void show_msg(const char *text)
{
	//if(!vshtask_notify)
	//	vshtask_notify = getNIDfunc("vshtask", 0xA02D46E7, 0);
	//if(!vshtask_notify) return;

	char msg[240];
	snprintf(msg, 240, "%s", text);

	if(strstr(msg, "&icon="))
		vshNotify_WithIcon(0, msg);
	else
		vshtask_notify(msg);
}