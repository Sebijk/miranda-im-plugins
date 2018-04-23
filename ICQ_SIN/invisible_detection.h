// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006 Joe Kucera
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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/init.h,v $
// Revision       : $Revision: 3663 $
// Last change on : $Date: 2006-08-30 22:18:59 +0200 (Mi, 30 Aug 2006) $
// Last change by : $Author: jokusoftware $
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

static unsigned __stdcall detectInvisibilityStub(void* arg);
static unsigned __stdcall makeOffStub(void* arg);
static unsigned __stdcall fullListCheckStub(void* arg);

static void CALLBACK EntireClistScan(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);

#define WAS_CONTACTED 720
#define DEFSPEED 6000
#define MAKEOFF_TIMEOUT 5000
#define SUCCESS 1511
#define EMPTY -1