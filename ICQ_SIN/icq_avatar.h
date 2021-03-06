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
// File name      : $URL: http://miranda.googlecode.com/svn/trunk/miranda/protocols/IcqOscarJ/icq_avatar.h $
// Revision       : $Revision: 8847 $
// Last change on : $Date: 2009-01-15 00:04:12 +0100 (Do, 15 Jan 2009) $
// Last change by : $Author: jokusoftware $
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#ifndef __ICQ_AVATAR_H
#define __ICQ_AVATAR_H

extern BOOL AvatarsReady;

extern BYTE hashEmptyAvatar[9];

#define AVATAR_HASH_STATIC  0x01
#define AVATAR_HASH_FLASH   0x08
#define AVATAR_HASH_PHOTO   0x0C

int  DetectAvatarFormat(const char *szFile);
void AddAvatarExt(int dwFormat, char* pszDest);

BYTE* calcMD5HashOfFile(const char *szFile);

#endif /* __ICQ_AVATAR_H */
