static CellPadData pad_data;

// This only works in
static CellPadData pad_read(void)
{
	pad_data.len = 0;

	CellPadInfo2 padinfo;
	if(cellPadGetInfo2(&padinfo) == CELL_OK)
		for(u8 n = 0; n < 20; n++)
		{
			for(u8 p = 0; p < 8; p++)
				if((padinfo.port_status[p] == CELL_PAD_STATUS_CONNECTED) && (cellPadGetData(p, &pad_data) == CELL_PAD_OK) && (pad_data.len > 0)) return pad_data;

			sys_ppu_thread_usleep(10000);
		}

	pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] = pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] = 0;
	return pad_data;
}
