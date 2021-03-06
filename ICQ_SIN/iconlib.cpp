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
// File name      : $URL: http://miranda.googlecode.com/svn/trunk/miranda/protocols/IcqOscarJ/iconlib.cpp $
// Revision       : $Revision: 8868 $
// Last change on : $Date: 2009-01-17 16:02:10 +0100 (Sa, 17 Jan 2009) $
// Last change by : $Author: jokusoftware $
//
// DESCRIPTION:
//
//  Support for IcoLib plug-in
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"
#include "m_icolib.h"


IcqIconHandle IconLibDefine(const char *desc, const char *section, const char *module, const char *ident, const TCHAR *def_file, int def_idx)
{
  SKINICONDESC sid = {0};
  char szTemp[MAX_PATH];
  char szName[MAX_PATH + 128];

  sid.cbSize = SKINICONDESC_SIZE;
  sid.pwszSection = make_unicode_string(section);
  sid.pwszDescription = make_unicode_string(ICQTranslateUtfStatic(desc, szTemp, MAX_PATH));
  sid.flags = SIDF_UNICODE | SIDF_PATH_TCHAR;

  null_snprintf(szName, sizeof(szName), "%s_%s", module ? module : ICQ_PROTOCOL_NAME, ident);
  sid.pszName = szName;
  sid.ptszDefaultFile = (TCHAR*)def_file;
  sid.iDefaultIndex = def_idx;
  sid.cx = sid.cy = 16;

  IcqIconHandle hIcon = (IcqIconHandle)SAFE_MALLOC(sizeof(IcqIconHandle_s));
  hIcon->szName = null_strdup(sid.pszName);
  hIcon->hIcoLib = (HANDLE)CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);

  SAFE_FREE((void**)&sid.pwszSection);
  SAFE_FREE((void**)&sid.pwszDescription);

  return hIcon;
}


void IconLibRemove(IcqIconHandle *phIcon)
{
  if (phIcon && *phIcon)
  {
    IcqIconHandle hIcon = *phIcon;

    CallService(MS_SKIN2_REMOVEICON, 0, (LPARAM)hIcon->szName);
    SAFE_FREE(&hIcon->szName);
    SAFE_FREE((void**)phIcon);
  }
}


HANDLE IcqIconHandle_s::Handle()
{
  if (this)
    return hIcoLib;

  return NULL;
}


HICON IcqIconHandle_s::GetIcon()
{
  if (this)
    return (HICON)CallService(MS_SKIN2_GETICONBYHANDLE, 0, (LPARAM)hIcoLib);

  return NULL;
}

