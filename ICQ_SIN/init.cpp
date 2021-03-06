// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright � 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001-2002 Jon Keating, Richard Hughes
// Copyright � 2002-2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004-2009 Joe Kucera
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// -----------------------------------------------------------------------------
//
// File name      : $URL: http://miranda.googlecode.com/svn/trunk/miranda/protocols/IcqOscarJ/init.cpp $
// Revision       : $Revision: 10156 $
// Last change on : $Date: 2009-06-20 20:26:28 +0200 (Sa, 20 Jun 2009) $
// Last change by : $Author: jokusoftware $
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"
#include "m_extraicons.h"

HINSTANCE hInst;
PLUGINLINK* pluginLink;
MM_INTERFACE mmi;
UTF8_INTERFACE utfi;
MD5_INTERFACE md5i;
LIST_INTERFACE li;

BYTE gbUnicodeCore;
DWORD MIRANDA_VERSION;

HANDLE hStaticServices[1];
IcqIconHandle hStaticIcons[7];
HANDLE hStaticHooks[1];;
HANDLE hExtraXStatus = NULL;


PLUGININFOEX pluginInfo = {
    sizeof(PLUGININFOEX),
    "IcqOscarJ S!N Protocol",
    PLUGIN_MAKE_VERSION(0,5,0,93),
    "Support for ICQ network, modded.",
    "Joe Kucera, [sin], Bio, Martin �berg, Richard Hughes, Jon Keating, etc",
    "sinbugreport@gmx.de",
    "(C) 2000-2009 [sin], M.�berg, R.Hughes, J.Keating, Bio, Angeli-Ka, G.Hazan, J.Kucera",
    "http://addons.miranda-im.org/details.php?action=viewfile&id=1683",
    UNICODE_AWARE,
    0,   //doesn't replace anything built-in
    #if defined( _UNICODE )
    {0x2df966ee, 0x52ec, 0x11de, {0x91, 0xaa, 0xa6, 0x8a, 0x56, 0xd8, 0x95, 0x93}}
    #else
    {0x2df9ee66, 0x52ec, 0x11de, {0x91, 0xa6, 0xaa, 0x8a, 0x56, 0xd8, 0x93, 0x95}}
    #endif
};

extern "C" PLUGININFOEX __declspec(dllexport) *MirandaPluginInfoEx(DWORD mirandaVersion)
{
	// Only load for 0.8.0.29 or greater
	// We need the core stubs for PS_GETNAME and PS_GETSTATUS
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 8, 0, 29))
	{
		MessageBoxA( NULL, "ICQ plugin cannot be loaded. It requires Miranda IM 0.8.0.29 or later.", "ICQ Plugin",
			MB_OK|MB_ICONWARNING|MB_SETFOREGROUND|MB_TOPMOST );
		return NULL;
	}

	return &pluginInfo;
}

static const MUUID interfaces[] = {MIID_PROTOCOL, MIID_LAST};
extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst = hinstDLL;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////

static PROTO_INTERFACE* icqProtoInit( const char* pszProtoName, const TCHAR* tszUserName )
{
	return new CIcqProto( pszProtoName, tszUserName );
}

static int icqProtoUninit( PROTO_INTERFACE* ppro )
{
	delete ( CIcqProto* )ppro;
	return 0;
}

static int OnModulesLoaded( WPARAM, LPARAM )
{
	hExtraXStatus = ExtraIcon_Register("xstatus", "ICQ XStatus");
	return 0;
}

extern "C" int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink = link;
	mir_getLI( &li );
	mir_getMMI( &mmi );
	mir_getUTFI( &utfi );
	mir_getMD5I( &md5i );

  // Get Miranda version
  MIRANDA_VERSION = (DWORD)CallService(MS_SYSTEM_GETVERSION, 0, 0);

	{ // Are we running under unicode Miranda core ?
		char szVer[MAX_PATH];

		CallService(MS_SYSTEM_GETVERSIONTEXT, MAX_PATH, (LPARAM)szVer);
		_strlwr(szVer); // make sure it is lowercase
		gbUnicodeCore = (strstrnull(szVer, "unicode") != NULL);

		if (strstrnull(szVer, "alpha") != NULL)
		{ // Are we running under Alpha Core
			MIRANDA_VERSION |= 0x80000000;
		}
		else if (MIRANDA_VERSION >= 0x00050000 && strstrnull(szVer, "preview") == NULL)
		{ // for Final Releases of Miranda 0.5+ clear build number
			MIRANDA_VERSION &= 0xFFFFFF00;
		}
	}

	srand(time(NULL));
	_tzset();

	// Register the module
	PROTOCOLDESCRIPTOR pd = {0};
	pd.cbSize   = sizeof(pd);
	pd.szName   = ICQ_PROTOCOL_NAME;
	pd.type     = PROTOTYPE_PROTOCOL;
	pd.fnInit   = icqProtoInit;
	pd.fnUninit = icqProtoUninit;
	CallService(MS_PROTO_REGISTERMODULE, 0, (LPARAM)&pd);

	// Initialize charset conversion routines
	InitI18N();

  // Register static services
  hStaticServices[0] = CreateServiceFunction(ICQ_DB_GETEVENTTEXT_MISSEDMESSAGE, icq_getEventTextMissedMessage);

  { // Define global icons
    TCHAR lib[MAX_PATH];
    char szSectionName[MAX_PATH];
    char szProtocolsBuf[100], szNameBuf[100];

    null_snprintf(szSectionName, sizeof(szSectionName), "%s/%s", 
      ICQTranslateUtfStatic(LPGEN("Protocols"), szProtocolsBuf, sizeof(szProtocolsBuf)), 
      ICQTranslateUtfStatic(ICQ_PROTOCOL_NAME, szNameBuf, sizeof(szNameBuf)));

    GetModuleFileName(hInst, lib, MAX_PATH);
    hStaticIcons[ISI_AUTH_REQUEST] = IconLibDefine(LPGEN("Request authorization"), szSectionName, NULL, "req_auth", lib, -IDI_AUTH_ASK);
    hStaticIcons[ISI_AUTH_GRANT] = IconLibDefine(LPGEN("Grant authorization"), szSectionName, NULL, "grant_auth", lib, -IDI_AUTH_GRANT);
    hStaticIcons[ISI_AUTH_REVOKE] = IconLibDefine(LPGEN("Revoke authorization"), szSectionName, NULL, "revoke_auth", lib, -IDI_AUTH_REVOKE);
    hStaticIcons[ISI_ADD_TO_SERVLIST] = IconLibDefine(LPGEN("Add to server list"), szSectionName, NULL, "add_to_server", lib, -IDI_SERVLIST_ADD);
	hStaticIcons[ISI_SET_VIS] = IconLibDefine(LPGEN("Set Visible"), szSectionName, NULL, "set_vis", lib, -IDI_INVIS);
	hStaticIcons[ISI_SET_INVIS] = IconLibDefine(LPGEN("Set Invisible"), szSectionName, NULL, "set_invis", lib, -IDI_INVISOFF);
	hStaticIcons[ISI_SCAN] = IconLibDefine(LPGEN("Scan"), szSectionName, NULL, "scan", lib, -IDI_SCAN);
  }

  hStaticHooks[0] = HookEvent(ME_SYSTEM_MODULESLOADED, OnModulesLoaded);

	return 0;
}

extern "C" int __declspec(dllexport) Unload(void)
{
  int i;

  // Release static icon handles
  for (i = 0; i < SIZEOF(hStaticIcons); i++)
    IconLibRemove(&hStaticIcons[i]);

  // Release static event hooks
  for (i = 0; i < SIZEOF(hStaticHooks); i++)
    if (hStaticHooks[i])
      UnhookEvent(hStaticHooks[i]);

  // Destroy static service functions
  for (i = 0; i < SIZEOF(hStaticServices); i++)
    if (hStaticServices[i])
      DestroyServiceFunction(hStaticServices[i]);

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnPrebuildContactMenu event

void CListShowMenuItem(HANDLE hMenuItem, BYTE bShow)
{
	CLISTMENUITEM mi = {0};

	mi.cbSize = sizeof(mi);
	if (bShow)
		mi.flags = CMIM_FLAGS;
	else
		mi.flags = CMIM_FLAGS | CMIF_HIDDEN;

	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hMenuItem, (LPARAM)&mi);
}

static void CListSetMenuItemIcon(HANDLE hMenuItem, HICON hIcon)
{
	CLISTMENUITEM mi = {0};

	mi.cbSize = sizeof(mi);
	mi.flags = CMIM_FLAGS | CMIM_ICON;

	mi.hIcon = hIcon;
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hMenuItem, (LPARAM)&mi);
}


int CIcqProto::OnPreBuildContactMenu(WPARAM wParam, LPARAM lParam)
{
	CLISTMENUITEM cli={0};
	
	CListShowMenuItem(m_hContactMenuItems[ICMI_AUTH_REQUEST], getSettingByte((HANDLE)wParam, "Auth", 0));
	CListShowMenuItem(m_hContactMenuItems[ICMI_AUTH_GRANT], getSettingByte((HANDLE)wParam, "Grant", 0));
	CListShowMenuItem(m_hContactMenuItems[ICMI_AUTH_REVOKE], getSettingByte(NULL, "PrivacyItems", 0) && !getSettingByte((HANDLE)wParam, "Grant", 0));
	CListShowMenuItem(m_hContactMenuItems[ICMI_SET_VIS], icqOnline()); //loook
    CListShowMenuItem(m_hContactMenuItems[ICMI_SET_INVIS], icqOnline());
    
    cli.cbSize=sizeof(CLISTMENUITEM);
    cli.flags=CMIM_FLAGS;
    cli.hIcon=NULL;
    cli.pszContactOwner=NULL;

    switch(getSettingWord((HANDLE)wParam, "ApparentMode",0))
    {
	  case ID_STATUS_ONLINE:
		  cli.flags|=CMIF_CHECKED;
	      CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)hUserMenuSetVis,(LPARAM)&cli);
		  break;

	  case ID_STATUS_OFFLINE:
		  cli.flags|=CMIF_CHECKED;
		  CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)hUserMenuSetInvis,(LPARAM)&cli);
		  break;

      default:
		  break;
    } 
    
	if (m_bSsiEnabled && !getSettingWord((HANDLE)wParam, DBSETTING_SERVLIST_ID, 0) && !getSettingWord((HANDLE)wParam, DBSETTING_SERVLIST_IGNORE, 0))
		CListShowMenuItem(m_hContactMenuItems[ICMI_ADD_TO_SERVLIST], 1);
	else
		CListShowMenuItem(m_hContactMenuItems[ICMI_ADD_TO_SERVLIST], 0);

	BYTE bXStatus = getContactXStatus((HANDLE)wParam);

  CListShowMenuItem(m_hContactMenuItems[ICMI_XSTATUS_DETAILS], (BYTE)(m_bHideXStatusUI ? 0 : bXStatus));
	if (bXStatus && !m_bHideXStatusUI)
		CListSetMenuItemIcon(m_hContactMenuItems[ICMI_XSTATUS_DETAILS], getXStatusIcon(bXStatus, LR_SHARED));

	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////
// OnPrebuildContactMenu event

int CIcqProto::OnPreBuildStatusMenu(WPARAM wParam, LPARAM lParam)
{
	InitXStatusItems(TRUE);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnReloadIcons event

int CIcqProto::OnReloadIcons(WPARAM wParam, LPARAM lParam)
{
	memset(bXStatusCListIconsValid, 0, sizeof(bXStatusCListIconsValid));
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// UpdateGlobalSettings event

void CIcqProto::UpdateGlobalSettings()
{
	if (m_hServerNetlibUser)
	{
		NETLIBUSERSETTINGS nlus = {0};

		nlus.cbSize = sizeof(NETLIBUSERSETTINGS);
		if (CallService(MS_NETLIB_GETUSERSETTINGS, (WPARAM)m_hServerNetlibUser, (LPARAM)&nlus))
		{
			if (nlus.useProxy && nlus.proxyType == PROXYTYPE_HTTP)
				m_bGatewayMode = 1;
			else
				m_bGatewayMode = 0;
		}
		else
			m_bGatewayMode = 0;
	}

	m_bSecureLogin = getSettingByte(NULL, "SecureLogin", DEFAULT_SECURE_LOGIN);
  m_bSecureConnection = FALSE; //getSettingByte(NULL, "SecureConnection", DEFAULT_SECURE_CONNECTION); //temporarily
	m_bAimEnabled = getSettingByte(NULL, "AimEnabled", DEFAULT_AIM_ENABLED);
	m_bUtfEnabled = getSettingByte(NULL, "UtfEnabled", DEFAULT_UTF_ENABLED);
	m_wAnsiCodepage = getSettingWord(NULL, "AnsiCodePage", DEFAULT_ANSI_CODEPAGE);
	m_bDCMsgEnabled = getSettingByte(NULL, "DirectMessaging", DEFAULT_DCMSG_ENABLED);
	m_bTempVisListEnabled = getSettingByte(NULL, "TempVisListEnabled", DEFAULT_TEMPVIS_ENABLED);
	m_bSsiEnabled = getSettingByte(NULL, "UseServerCList", DEFAULT_SS_ENABLED);
	m_bSsiSimpleGroups = FALSE; /// TODO: enable, after server-list revolution is over
	m_bAvatarsEnabled = getSettingByte(NULL, "AvatarsEnabled", DEFAULT_AVATARS_ENABLED);
	m_bXStatusEnabled = getSettingByte(NULL, "XStatusEnabled", DEFAULT_XSTATUS_ENABLED);
 
  //mod globals
  gbIDSEnabled = getSettingByte(NULL, "IDS", 0);
  gbLCNEnabled = getSettingByte(NULL, "LeaveChatNotify", DEFAULT_LCN);
  gbForceXEnabled = getSettingByte(NULL, "ForceXstatus", DEFAULT_FORCEXSTATUS);
  gbInvalidSMREnabled = getSettingByte(NULL, "InvalidSMR", DEFAULT_INVALIDSMR);
  gbXtrazMethodEnabled = getSettingByte(NULL, "XtrazMethod", DEFAULT_XTRAZMETHOD);
  gbMalUrlEnabled = getSettingByte(NULL, "MalUrl", DEFAULT_MALURL);
  gbAddedMethodEnabled = getSettingByte(NULL, "AddedMethod", DEFAULT_ADDEDMETHOD);
  gbMalConEnabled = getSettingByte(NULL, "MalCon", DEFAULT_MALCON);
  gbIDSInInvisibleEnabled = getSettingByte(NULL, "InvisIDS", DEFAULT_INVISIDS);
  gbIDSIconEnabled = getSettingByte(NULL, "IDSIcon", DEFAULT_IDSICON);
  gbIDSDimEnabled = getSettingByte(NULL, "IDSDim", DEFAULT_IDSDIM);
  gbNewSMSEnabled = getSettingByte(NULL, "NewSMS", DEFAULT_NEWSMS);
  gbNoSMSProfileEnabled = getSettingByte(NULL, "NoSMSProfile", DEFAULT_NOSMSPROFILE);
}
