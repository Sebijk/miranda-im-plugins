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
// File name      : $URL: http://miranda.googlecode.com/svn/trunk/miranda/protocols/IcqOscarJ/icq_server.cpp $
// Revision       : $Revision: 9376 $
// Last change on : $Date: 2009-04-09 21:55:33 +0200 (Do, 09 Apr 2009) $
// Last change by : $Author: jokusoftware $
//
// DESCRIPTION:
//
//  Manages main server connection, low-level communication
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"

void icq_newConnectionReceived(HANDLE hNewConnection, DWORD dwRemoteIP, void *pExtra);

/////////////////////////////////////////////////////////////////////////////////////////
// ICQ Server thread

void __cdecl CIcqProto::ServerThread(serverthread_start_info* infoParam)
{
	serverthread_info info = {0};

	info.isLoginServer = 1;
	info.wAuthKeyLen = infoParam->wPassLen;
	strncpy((char*)info.szAuthKey, infoParam->szPass, info.wAuthKeyLen);
	// store server port
	info.wServerPort = infoParam->nloc.wPort;

	srand(time(NULL));

	ResetSettingsOnConnect();

	// Connect to the login server
	NetLog_Server("Authenticating to server");
	{
		NETLIBOPENCONNECTION nloc = infoParam->nloc;

		hServerConn = NetLib_OpenConnection(m_hServerNetlibUser, NULL, &nloc);

    if (hServerConn && m_bSecureConnection)
    {
#ifdef _DEBUG
      NetLog_Server("(%d) Starting SSL negotiation", CallService(MS_NETLIB_GETSOCKET, (WPARAM)hServerConn, 0));
#endif
	    CallService(MS_NETLIB_STARTSSL, (WPARAM)hServerConn, 0);
    }

		SAFE_FREE((void**)&nloc.szHost);
	}
  SAFE_FREE((void**)&infoParam);

	// Login error
	if (hServerConn == NULL)
	{
		DWORD dwError = GetLastError();

		hServerConn = NULL;

		SetCurrentStatus(ID_STATUS_OFFLINE);

		icq_LogUsingErrorCode(LOG_ERROR, dwError, LPGEN("Unable to connect to ICQ login server"));
		return;
	}

	// Initialize direct connection ports
	{
		DWORD dwInternalIP;
		BYTE bConstInternalIP = getSettingByte(NULL, "ConstRealIP", 0);

		info.hDirectBoundPort = NetLib_BindPort(icq_newConnectionReceived, this, &wListenPort, &dwInternalIP);
		if (!info.hDirectBoundPort)
		{
			icq_LogUsingErrorCode(LOG_WARNING, GetLastError(), LPGEN("Miranda was unable to allocate a port to listen for direct peer-to-peer connections between clients. You will be able to use most of the ICQ network without problems but you may be unable to send or receive files.\n\nIf you have a firewall this may be blocking Miranda, in which case you should configure your firewall to leave some ports open and tell Miranda which ports to use in M->Options->ICQ->Network."));
			wListenPort = 0;
			if (!bConstInternalIP) deleteSetting(NULL, "RealIP");
		}
		else if (!bConstInternalIP)
			setSettingDword(NULL, "RealIP", dwInternalIP);
	}

	// This is the "infinite" loop that receives the packets from the ICQ server
	{
		int recvResult;
		NETLIBPACKETRECVER packetRecv = {0};

		info.hPacketRecver = (HANDLE)CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hServerConn, 0x2400);
		packetRecv.cbSize = sizeof(packetRecv);
		packetRecv.dwTimeout = INFINITE;
		while(hServerConn)
		{
			if (info.bReinitRecver)
			{ // we reconnected, reinit struct
				info.bReinitRecver = 0;
				ZeroMemory(&packetRecv, sizeof(packetRecv));
				packetRecv.cbSize = sizeof(packetRecv);
				packetRecv.dwTimeout = INFINITE;
			}

			recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM)info.hPacketRecver, (LPARAM)&packetRecv);

			if (recvResult == 0)
			{
				NetLog_Server("Clean closure of server socket");
				break;
			}

			if (recvResult == SOCKET_ERROR)
			{
				NetLog_Server("Abortive closure of server socket, error: %d", GetLastError());
				break;
			}

      if (m_iDesiredStatus == ID_STATUS_OFFLINE)
      { // Disconnect requested, send disconnect packet
			  icq_sendCloseConnection();

        // disconnected upon request
        m_bConnectionLost = FALSE;

	  		NetLog_Server("Logged off.");
        break;
      }

			// Deal with the packet
			packetRecv.bytesUsed = handleServerPackets(packetRecv.buffer, packetRecv.bytesAvailable, &info);
		}

		// Close the packet receiver (connection may still be open)
		NetLib_SafeCloseHandle(&info.hPacketRecver);

		// Close DC port
		NetLib_SafeCloseHandle(&info.hDirectBoundPort);
	}

	// signal keep-alive thread to stop
	StopKeepAlive(&info);

	// disable auto info-update thread
	icq_EnableUserLookup(FALSE);

	// Time to shutdown
	icq_serverDisconnect(FALSE);
	if (m_iStatus != ID_STATUS_OFFLINE && m_iDesiredStatus != ID_STATUS_OFFLINE)
	{
		if (!info.bLoggedIn)
			icq_LogMessage(LOG_FATAL, LPGEN("Connection failed.\nLogin sequence failed for unknown reason.\nTry again later."));

    // set flag indicating we were kicked out
    m_bConnectionLost = TRUE;

		SetCurrentStatus(ID_STATUS_OFFLINE);
	}

	// Close all open DC connections
	CloseContactDirectConns(NULL);

	// Close avatar connection if any
	StopAvatarThread();

	// Offline all contacts
	HANDLE hContact = FindFirstContact();
	while (hContact)
	{
		DWORD dwUIN;
		uid_str szUID;

		if (!getContactUid(hContact, &dwUIN, &szUID))
		{
			if (getContactStatus(hContact) != ID_STATUS_OFFLINE)
			{
				setSettingWord(hContact, "Status", ID_STATUS_OFFLINE);

				handleXStatusCaps(hContact, NULL, 0, NULL, 0);
			}
		}

		hContact = FindNextContact(hContact);
	}

	setSettingDword(NULL, "LogonTS", 0); // clear logon time

	servlistPendingFlushOperations(); // clear pending operations list
	
  EnterCriticalSection(&ratesMutex);
  delete m_rates;
  m_rates = NULL;
	LeaveCriticalSection(&ratesMutex);

	FlushServerIDs();         // clear server IDs list

	NetLog_Server("%s thread ended.", "Server");
}


void CIcqProto::icq_serverDisconnect(BOOL bBlock)
{
	EnterCriticalSection(&connectionHandleMutex);

	if (hServerConn)
	{
		NetLib_CloseConnection(&hServerConn, TRUE);
		LeaveCriticalSection(&connectionHandleMutex);

		// Not called from network thread?
		if (bBlock && GetCurrentThreadId() != serverThreadId)
		{
			while (ICQWaitForSingleObject(serverThreadHandle, INFINITE) != WAIT_OBJECT_0);
			CloseHandle(serverThreadHandle);
		}
		else
			CloseHandle(serverThreadHandle);
	}
	else
		LeaveCriticalSection(&connectionHandleMutex);
}

int CIcqProto::handleServerPackets(unsigned char* buf, int len, serverthread_info* info)
{
	BYTE channel;
	WORD sequence;
	WORD datalen;
///
    unsigned char *tempbuf;
    WORD tempdatalen;
    int totallen = len;
///
	int bytesUsed = 0;

	while (len > 0)
	{
///
	  snac_header snacHeader = {0};
///

		if (info->bReinitRecver)
			break;

		// All FLAPS begin with 0x2a
		if (*buf++ != FLAP_MARKER)
			break;

		if (len < 6)
			break;

		unpackByte(&buf, &channel);
		unpackWord(&buf, &sequence);
		unpackWord(&buf, &datalen);

		if (len < 6 + datalen)
			break;
			
/////////////////////// exclude unneeded "went off" messages, for now ^^
	tempbuf = buf;
	tempdatalen = datalen;

	if(channel == ICQ_DATA_CHAN)
	{
      if (!unpackSnacHeader(&snacHeader, &tempbuf, &tempdatalen) || !snacHeader.bValid)
      {
        NetLog_Server("Error: Failed to parse SNAC header");
      }
	  else
      {
	      if(snacHeader.wSubtype == ICQ_USER_OFFLINE)
		  {
			  if(totallen != (6+datalen))
			  {
			      NetLog_Server("dropping fake paket: Len: %u",datalen);
                  // Increase pointers so we can check for more FLAPs
                  buf += datalen;
                  len -= (datalen + 6);
                  bytesUsed += (datalen + 6);
			      continue;
			  }
			  else
			  {}//do nothing, go on
		  }
      }
	}
////////////////////////////////////////////////////////////


#ifdef _DEBUG
		NetLog_Server("Server FLAP: Channel %u, Seq %u, Length %u bytes", channel, sequence, datalen);
#endif

		switch (channel) {
		case ICQ_LOGIN_CHAN:
			handleLoginChannel(buf, datalen, info);
			break;

		case ICQ_DATA_CHAN:
			handleDataChannel(buf, datalen, info);
			break;

		case ICQ_ERROR_CHAN:
			handleErrorChannel(buf, datalen);
			break;

		case ICQ_CLOSE_CHAN:
			handleCloseChannel(buf, datalen, info);
			break; // we need this for walking thru proxy

		case ICQ_PING_CHAN:
			handlePingChannel(buf, datalen);
			break;

		default:
			NetLog_Server("Warning: Unhandled %s FLAP Channel: Channel %u, Seq %u, Length %u bytes", "Server", channel, sequence, datalen);
			break;
		}

		/* Increase pointers so we can check for more FLAPs */
		buf += datalen;
		len -= (datalen + 6);
		bytesUsed += (datalen + 6);
	}

	return bytesUsed;
}

void CIcqProto::sendServPacket(icq_packet* pPacket)
{
  // make sure to have the connection handle
  EnterCriticalSection(&connectionHandleMutex);

	if (hServerConn)
	{
		int nSendResult;

    // This critsec makes sure that the sequence order doesn't get screwed up
    EnterCriticalSection(&localSeqMutex);

		// :IMPORTANT:
		// The FLAP sequence must be a WORD. When it reaches 0xFFFF it should wrap to
		// 0x0000, otherwise we'll get kicked by server.
		wLocalSequence++;

		// Pack sequence number
		pPacket->pData[2] = ((wLocalSequence & 0xff00) >> 8);
		pPacket->pData[3] = (wLocalSequence & 0x00ff);

		for (int nRetries = 3; nRetries >= 0; nRetries--)
		{
			nSendResult = Netlib_Send(hServerConn, (const char *)pPacket->pData, pPacket->wLen, 0);

			if (nSendResult != SOCKET_ERROR)
				break;

			Sleep(1000);
		}

    LeaveCriticalSection(&localSeqMutex);
    LeaveCriticalSection(&connectionHandleMutex);

		// Rates management
		EnterCriticalSection(&ratesMutex);
		m_rates->packetSent(pPacket);
		LeaveCriticalSection(&ratesMutex);

		// Send error
		if (nSendResult == SOCKET_ERROR)
		{
			icq_LogUsingErrorCode(LOG_ERROR, GetLastError(), LPGEN("Your connection with the ICQ server was abortively closed"));
			icq_serverDisconnect(FALSE);

			if (m_iStatus != ID_STATUS_OFFLINE)
			{
				SetCurrentStatus(ID_STATUS_OFFLINE);
			}
		}
	}
	else
	{
    LeaveCriticalSection(&connectionHandleMutex);

		NetLog_Server("Error: Failed to send packet (no connection)");
	}

	SAFE_FREE((void**)&pPacket->pData);
}

void __cdecl CIcqProto::SendPacketAsyncThread(icq_packet* pkt)
{
	sendServPacket( pkt );
	SAFE_FREE((void**)&pkt);
}

void CIcqProto::sendServPacketAsync(icq_packet *packet)
{
	icq_packet *pPacket;

	pPacket = (icq_packet*)SAFE_MALLOC(sizeof(icq_packet)); // This will be freed in the new thread
	memcpy(pPacket, packet, sizeof(icq_packet));

	ForkThread(( IcqThreadFunc )&CIcqProto::SendPacketAsyncThread, pPacket);
}

int CIcqProto::IsServerOverRate(WORD wFamily, WORD wCommand, int nLevel)
{
	int result = FALSE;

	EnterCriticalSection(&ratesMutex);
	WORD wGroup = m_rates->getGroupFromSNAC(wFamily, wCommand);

	// check if the rate is not over specified level
	if (m_rates->getNextRateLevel(wGroup) < m_rates->getLimitLevel(wGroup, nLevel))
		result = TRUE;

	LeaveCriticalSection(&ratesMutex);

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
// ICQ Server thread

void CIcqProto::icq_login(const char* szPassword)
{
	DWORD dwUin = getContactUin(NULL);
	serverthread_start_info* stsi = (serverthread_start_info*)SAFE_MALLOC(sizeof(serverthread_start_info));

	// Server host name
	char szServer[MAX_PATH];
	if (getSettingStringStatic(NULL, "OscarServer", szServer, MAX_PATH))
		stsi->nloc.szHost = null_strdup(DEFAULT_SERVER_HOST);
	else if(!strcmp(szServer, DEFAULT_SERVER_HOST_SSL))//temporarly
	{
		MessageBox(NULL, _T("SSL connection will be temporarily disabled\n due to complications with the mod\n\n [sin]"), _T("IcqJ S!N Plugin"), IDOK);
		stsi->nloc.szHost = null_strdup(DEFAULT_SERVER_HOST);
		setSettingString(NULL, "OscarServer", DEFAULT_SERVER_HOST);
	}
	else
	{
		stsi->nloc.szHost = null_strdup(szServer);
	}

	// Server port
	stsi->nloc.wPort = getSettingWord(NULL, "OscarPort", DEFAULT_SERVER_PORT);
	if (stsi->nloc.wPort == 0)
		stsi->nloc.wPort = RandRange(1024, 65535);

	// User password
	stsi->wPassLen = strlennull(szPassword);
	if (stsi->wPassLen > 9) stsi->wPassLen = 9;
	strncpy(stsi->szPass, szPassword, stsi->wPassLen);
	stsi->szPass[stsi->wPassLen] = '\0';

	// Randomize sequence
	wLocalSequence = generate_flap_sequence();

	m_dwLocalUIN = dwUin;

	serverThreadHandle = ForkThreadEx(( IcqThreadFunc )&CIcqProto::ServerThread, stsi, &serverThreadId);
}
