/*
 * Miranda-IM Vypress Chat/quickChat plugins
 * Copyright (C) Saulius Menkevicius
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: main.c,v 1.11 2005/03/09 14:44:22 bobas Exp $
 */

#include "miranda.h"
#include "main.h"
#include "service.h"
#include "user.h"
#include "userlist.h"
#include "chatroom.h"
#include "msghandler.h"
#include "options.h"
#include "skin.h"

/* forward references
 */

/* global data
 */
HINSTANCE	g_hDllInstance;	/* plugin dll instance */
HANDLE		g_hMainThread;

PLUGINLINK	* pluginLink;

struct MM_INTERFACE memoryManagerInterface;
struct UTF8_INTERFACE utfi;
/* exported routines
 */
BOOL WINAPI
DllMain(HINSTANCE hInstanceDLL, DWORD fwReason, LPVOID lpvReserved)
{
	g_hDllInstance = hInstanceDLL;
	return TRUE;
}

PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
	VQCHAT_PROTO_NAME " Protocol",
	VERSION,
	"Adds support for " VQCHAT_PROTO_NAME " networks.",
	"Saulius Menkevicius",
	"bobas@sourceforge.net",
	"© 2005 Saulius Menkevicius, © 2008 amp, © 2009 Home of the Sebijk.com",
	"http://sourceforge.net/projects/miranda-vqchat/",
	0,
	0,
	{0x2d41f191, 0xd2, 0x4172, { 0xca, 0xa1, 0xf1, 0x4, 0xab, 0xc8, 0x21, 0xfe }}
};

/////////////////////////////////////////////////////////////////////////////////////////
// MirandaPluginInfoEx - returns an information about a plugin

__declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 7, 0, 0))
		return NULL;

	return &pluginInfo;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MirandaPluginInterfaces - returns the protocol interface to the core

static const MUUID interfaces[] = {MIID_IMPORT, MIID_LAST};

__declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}

int __declspec(dllexport) Load(PLUGINLINK * link)
{
	PROTOCOLDESCRIPTOR pd;
	INITCOMMONCONTROLSEX iccs;

	/* setup static data */
	pluginLink = link;

	// set the memory & utf8 managers
	mir_getMMI( &memoryManagerInterface );
	if ( mir_getUTFI( &utfi ) == CALLSERVICE_NOTFOUND )
		return 1;

	/* get main thread handle */
	DuplicateHandle(
		GetCurrentProcess(), GetCurrentThread(),
		GetCurrentProcess(), &g_hMainThread,
		THREAD_SET_CONTEXT, FALSE, 0);
	
	/* init common controls library (for the IP adress entries to work)
	 */
	memset(&iccs, 0, sizeof(iccs));
	iccs.dwSize = sizeof(iccs);
	iccs.dwICC = ICC_INTERNET_CLASSES;
	InitCommonControlsEx(&iccs);

	/* init vqp_link module */
	vqp_init(NULL, NULL);

	/* register this module with miranda */
	memset(&pd, 0, sizeof(pd));
	pd.cbSize = sizeof(pd);
	pd.szName = VQCHAT_PROTO;
	pd.type = PROTOTYPE_PROTOCOL;
	CallService(MS_PROTO_REGISTERMODULE, 0, (LPARAM)&pd);

	/* init our modules */
	options_init();
	msghandler_init();
	skin_init();
	user_init();
	userlist_init();
	chatroom_init();

	/* register protocol services */
	service_register_services();
	service_hook_all();

	return 0;
}

int __declspec(dllexport) Unload(void)
{
	service_uninit();
	user_uninit();
	userlist_uninit();
	chatroom_uninit();
	msghandler_uninit();
	options_uninit();
	skin_uninit();

	vqp_uninit();

	return 0;
}

