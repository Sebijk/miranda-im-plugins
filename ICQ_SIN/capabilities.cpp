// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001-2002 Jon Keating, Richard Hughes
// Copyright � 2002-2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004-2008 Joe Kucera
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
// File name      : $URL: http://miranda.googlecode.com/svn/trunk/miranda/protocols/IcqOscarJ/capabilities.cpp $
// Revision       : $Revision: 7500 $
// Last change on : $Date: 2008-03-24 19:07:37 +0100 (Mo, 24 Mrz 2008) $
// Last change by : $Author: jokusoftware $
//
// DESCRIPTION:
//
//  Contains helper functions to handle oscar user capabilities. Scanning and
//  adding capabilities are assumed to be more timecritical than looking up
//  capabilites. During the login sequence there could possibly be many hundred
//  scans but only a few lookups. So when you add or change something in this
//  code you must have this in mind, dont do anything that will slow down the
//  adding process too much.
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


typedef struct icq_capability_s
{
	DWORD fdwMirandaID;              // A bitmask, we use it in order to save database space
	BYTE  CapCLSID[BINARY_CAP_SIZE]; // A binary representation of a oscar capability
} icq_capability;

static icq_capability CapabilityRecord[] =
{
	{CAPF_SRV_RELAY, {CAP_SRV_RELAY }},
	{CAPF_UTF,       {CAP_UTF       }},
	{CAPF_RTF,       {CAP_RTF       }},
	{CAPF_HTML,      {CAP_HTML      }},
	{CAPF_CONTACTS,  {CAP_CONTACTS  }},
	{CAPF_TYPING,    {CAP_TYPING    }},
	{CAPF_ICQDIRECT, {CAP_ICQDIRECT }},
	{CAPF_XTRAZ,     {CAP_XTRAZ     }},
	{CAPF_OSCAR_FILE,{CAP_OSCAR_FILE}}
};

// Deletes all oscar capabilities for a given contact
void CIcqProto::ClearAllContactCapabilities(HANDLE hContact)
{
	setSettingDword(hContact, DBSETTING_CAPABILITIES, 0);
}

// Deletes one or many oscar capabilities for a given contact
void CIcqProto::ClearContactCapabilities(HANDLE hContact, DWORD fdwCapabilities)
{
	// Get current capability flags
	DWORD fdwContactCaps =  getSettingDword(hContact, DBSETTING_CAPABILITIES, 0);

	// Clear unwanted capabilities
	fdwContactCaps &= ~fdwCapabilities;

	// And write it back to disk
	setSettingDword(hContact, DBSETTING_CAPABILITIES, fdwContactCaps);
}

// Sets one or many oscar capabilities for a given contact
void CIcqProto::SetContactCapabilities(HANDLE hContact, DWORD fdwCapabilities)
{
	// Get current capability flags
	DWORD fdwContactCaps =  getSettingDword(hContact, DBSETTING_CAPABILITIES, 0);

	// Update them
	fdwContactCaps |= fdwCapabilities;

	// And write it back to disk
	setSettingDword(hContact, DBSETTING_CAPABILITIES, fdwContactCaps);
}

// Returns true if the given contact supports the requested capabilites
BOOL CIcqProto::CheckContactCapabilities(HANDLE hContact, DWORD fdwCapabilities)
{
	// Get current capability flags
	DWORD fdwContactCaps =  getSettingDword(hContact, DBSETTING_CAPABILITIES, 0);

	// Check if all requested capabilities are supported
	if ((fdwContactCaps & fdwCapabilities) == fdwCapabilities)
		return TRUE;

	return FALSE;
}

// Scans a binary buffer for oscar capabilities and adds them to the contact.
// You probably want to call ClearAllContactCapabilities() first.
void CIcqProto::AddCapabilitiesFromBuffer(HANDLE hContact, BYTE* pbyBuffer, int nLength)
{
	DWORD fdwContactCaps;
	int iCapability;
	int nIndex;
	int nRecordSize;

	// Calculate the number of records
	nRecordSize = SIZEOF(CapabilityRecord);

	// Get current capability flags
	fdwContactCaps =  getSettingDword(hContact, DBSETTING_CAPABILITIES, 0);

	// Loop over all capabilities in the buffer and
	// compare them to our own record of capabilities
	for (iCapability = 0; (iCapability + BINARY_CAP_SIZE) <= nLength; iCapability += BINARY_CAP_SIZE)
	{
		for (nIndex = 0; nIndex < nRecordSize; nIndex++)
		{
			if (!memcmp(pbyBuffer + iCapability, CapabilityRecord[nIndex].CapCLSID, BINARY_CAP_SIZE))
			{
				// Match
				fdwContactCaps |= CapabilityRecord[nIndex].fdwMirandaID;
				break;
			}
		}
	}

	// And write it back to disk
	setSettingDword(hContact, DBSETTING_CAPABILITIES, fdwContactCaps);
}
