#include "songinfo.h"

static struct
{
	Uint songno;
	Uint maxsongno;
	Uint startsongno;
	Uint extdevice;
	Uint initaddress;
	Uint playaddress;
	Uint channel;
	Uint initlimit;
} local = {
	0,0,0,0,0,0,1,0
};

Uint SONGINFO_GetSongNo(void)
{
	return local.songno;
}
void SONGINFO_SetSongNo(Uint v)
{
	if (local.maxsongno && v > local.maxsongno) v = local.maxsongno;
	if (v == 0) v++;
	local.songno = v;
}
Uint SONGINFO_GetStartSongNo(void)
{
	return local.startsongno;
}
void SONGINFO_SetStartSongNo(Uint v)
{
	local.startsongno = v;
}
Uint SONGINFO_GetMaxSongNo(void)
{
	return local.maxsongno;
}
void SONGINFO_SetMaxSongNo(Uint v)
{
	local.maxsongno = v;
}
Uint SONGINFO_GetExtendDevice(void)
{
	return local.extdevice;
}
void SONGINFO_SetExtendDevice(Uint v)
{
	local.extdevice = v;
}
Uint SONGINFO_GetInitAddress(void)
{
	return local.initaddress;
}
void SONGINFO_SetInitAddress(Uint v)
{
	local.initaddress = v;
}
Uint SONGINFO_GetPlayAddress(void)
{
	return local.playaddress;
}
void SONGINFO_SetPlayAddress(Uint v)
{
	local.playaddress = v;
}
Uint SONGINFO_GetChannel(void)
{
	return local.channel;
}
void SONGINFO_SetChannel(Uint v)
{
	local.channel = v;
}

Uint SONGINFO_GetInitializeLimiter(void)
{
	return local.initlimit;
}
void SONGINFO_SetInitializeLimiter(Uint v)
{
	local.initlimit = v;
}