// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2007,2008,2009 [sin]
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
//
// DESCRIPTION:
//
//  Invisible detection system by [sin]
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"

//#include "m_cluiframes.h"

typedef struct s_checkstatus {
	DWORD dwUin;
	DWORD dwCookie;
	DWORD dwContacted;
	BOOL bManual;
	BOOL bWentInvis;
} checkstatus;

static checkstatus StatusList[500];//are 500 enough?

static int user = 0;
static int list;

//static WORD checkspeed;

static BOOL bMakeOff = FALSE;
static BOOL bPop = FALSE;

//static 	BYTE IDS = 0;

static HANDLE hQueueEventS = NULL;
static HANDLE hMakeOffS = NULL;

static int hTimeoutOff = 0;

static int queue[6] = {EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY};


void CIcqProto::setInvisibility(DWORD dwUin, HANDLE hContact)
{
	if(!gbIDSEnabled)
		return;

    time_t TimeStamp = time(NULL);

    if (hContact && hContact != INVALID_HANDLE_VALUE)
    	dwUin = getSettingDword(hContact, UNIQUEIDSETTING, 0);
	else
		hContact = HContactFromUIN(dwUin,0);

	//determine user
	for(user=0;StatusList[user].dwUin != dwUin; user++)
	{}

    StatusList[user].dwCookie = SUCCESS;

	if(CheckContactCapabilities(hContact, WAS_FOUND))
	{
		bMakeOff = FALSE;
		if(bPop)
		{
            ShowPopUpMsg(hContact, NickFromHandleUtf(hContact), "is still invisible", POPTYPE_SCAN);
			bPop = FALSE;
		}
	}
	else if(!CheckContactCapabilities(hContact, WAS_FOUND) &&
		     (getSettingWord(hContact, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE))
	{
	setSettingWord(hContact, "Status", ID_STATUS_INVISIBLE);
	setSettingDword(hContact, "OldLogonTS", (DWORD)TimeStamp);

    SetContactCapabilities(hContact, WAS_FOUND);
	ClearContactCapabilities(hContact, CAPF_SRV_RELAY); // for compability

	// dim icon
	if (!getSettingDword(hContact, "IdleTS", 0) && gbIDSDimEnabled)
	    setSettingDword(hContact, "IdleTS", (DWORD)TimeStamp);

	if(StatusList[user].bWentInvis == TRUE)
	{
		ShowPopUpMsg(hContact, NickFromHandleUtf(hContact), "went into invisible mode", POPTYPE_SCAN);
		StatusList[user].bWentInvis = FALSE;
	}
	else
	    ShowPopUpMsg(hContact, NickFromHandleUtf(hContact), "was found invisible", POPTYPE_SCAN);
	}

	setInvisibleIcon(hContact, TRUE);
}


/*
void CIcqProto::checkSpeed(WORD wStatus)
{
// 2 - slower! 3 - very slower! 4 - speed up!

	switch (wStatus)
	{
    case 2: if (bFLC) checkspeed += 1000; break;
    case 3: if (bFLC) checkspeed += 5000; break;
    case 4: if (bFLC) checkspeed = DEFSPEED; break;
	case 9: if (bFLC) if(checkspeed > DEFSPEED) checkspeed -= 250; break;
    default: break;
    }
    Netlib_Logf(m_hServerNetlibUser, "Rate status recieved: %u. Delay: %u ms", wStatus, checkspeed);
	if(wStatus == 3)
	{
		char szFormat[MAX_PATH];
        char szMsg[MAX_PATH];
        char *nick = NickFromHandleUtf(HContactFromUIN(StatusList[user].dwUin,FALSE));

		null_snprintf(szMsg, MAX_PATH, ICQTranslateUtfStatic("Scanning \"%s\" failed, too much traffic, SLOW DOWN!", szFormat, MAX_PATH), nick);
		ShowPopUpMsg(NULL, ICQTranslateUtfStatic("WARNING!", szFormat, MAX_PATH), szMsg, LOG_WARNING);
		SAFE_FREE((void**)&nick);
	}
}
*/

//TODO: needs more checks & modes
void CIcqProto::checkInvisibility(DWORD dwUin, HANDLE hContact, BYTE mode)
{
/* mode meanings:
	0 - "user went/is offline"-message received (basically)
	1 - Full contactlist check
	2 - manual specific user check
	3 - "user went/is offline"-message received, contact went into inv mode
	4 - "user went/is offline"-message received, set inv since it must be invisible (qip)
*/
	if(!gbIDSEnabled)
		return;

    if (hContact && hContact != INVALID_HANDLE_VALUE)
    	dwUin = getSettingDword(hContact, UNIQUEIDSETTING, 0);
	else
		hContact = HContactFromUIN(dwUin,0);

/*
	if((ICQGetContactSettingWord(hContact, "Status", 0) != ID_STATUS_OFFLINE &&
		!CheckContactCapabilities(hContact, WAS_FOUND)))
		return;
*/

	//determine user
	for(user=0;StatusList[user].dwUin != dwUin; user++)
	{}
/*
	if(CheckContactCapabilities(hContact, WAS_FOUND))
	{
	}
*/
	if(mode == 2)
	    StatusList[user].bManual = TRUE;
	else if(mode == 3)
	{
		StatusList[user].bWentInvis = TRUE;
	}
	else if(mode == 4)
	{
		if(CheckContactCapabilities(hContact, WAS_FOUND))
		{
            makeUserOffline(hContact);
			return;
		}
		else
		{
		    setInvisibility(dwUin, hContact);
		    return;
		}
	}




	if((m_iStatus==ID_STATUS_INVISIBLE) && !getSettingByte(NULL, "InvisIDS", 0))
		return;

	if(getSettingByte(hContact, "DontScan", 0))
		return;

	if (hQueueEventS) SetEvent(hQueueEventS);
    
}

void CIcqProto::setInvisibleIcon(HANDLE hContact, BOOL set)
{

	if(!gbIDSIconEnabled)
		return;

	if(set)
	{
      IconExtraColumn iec;

      iec.cbSize = sizeof(iec);
      iec.hImage = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON,(WPARAM)hStaticIcons[ISI_SET_VIS]->GetIcon(),0);
      iec.ColumnType = EXTRA_ICON_EMAIL;//TODO: look for best value, or make option
      CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
	}
	else
	{
      IconExtraColumn iec;

      iec.cbSize = sizeof(iec);
      iec.hImage = (HANDLE)-1;
      iec.ColumnType = EXTRA_ICON_EMAIL;//TODO: look for best value, or make option
      CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
	}
}


INT_PTR CIcqProto::checkUserStatus(WPARAM wParam,LPARAM lParam)
{
	if (icqOnline())
	{
		bPop = TRUE;//TODO: change, kinda weird way ;)
		checkInvisibility(0,(HANDLE)wParam, 2);
	}
	return 0;
}

void CIcqProto::initList(void)
{
	uid_str szUID;
	DWORD dwUin;
	HANDLE hContact;

	user = 0;

	NetLog_Server("refreshing list");

	//Init List
    hContact = FindFirstContact();
    while (hContact)
    {
      dwUin = getContactUin(hContact);

	  if(dwUin)
	  {

	  StatusList[user].dwUin = dwUin;
	  StatusList[user].dwCookie = 0;
	  StatusList[user].dwContacted = 0;
	  StatusList[user].bManual = FALSE;
	  StatusList[user].bWentInvis = FALSE;

	  //ClearAllContactCapabilities(hContact);//clear all

	  NetLog_Server("%s added", strUID(StatusList[user].dwUin, szUID));

	  user++;
	  }

      hContact = FindNextContact(hContact);
	}
}


void CIcqProto::initInvisibleDetection(void)
{
	uid_str szUID;
	DWORD dwUin;
	HANDLE hContact;

	char  pszServiceName[MAX_PATH+30];

   	CLISTMENUITEM mi={0};
 	mi.cbSize = sizeof(mi);


    hQueueEventS = CreateEvent(NULL, FALSE, FALSE, NULL);
	hMakeOffS = CreateEvent(NULL, FALSE, FALSE, NULL);

	//Init List
    hContact = FindFirstContact();
    while (hContact)
    {
      dwUin = getContactUin(hContact);

	  if(dwUin)
	  {

	  StatusList[user].dwUin = dwUin;
	  StatusList[user].dwCookie = 0;
	  StatusList[user].dwContacted = 0;
	  StatusList[user].bManual = FALSE;
	  StatusList[user].bWentInvis = FALSE;

	  ClearAllContactCapabilities(hContact);//clear all

	  NetLog_Server("%s added", strUID(StatusList[user].dwUin, szUID));

	  user++;
	  }

      hContact = FindNextContact(hContact);
	}
	list = user-1;
	NetLog_Server("%u users added", list+1);

	//checkspeed = DEFSPEED;

	//start invisible detection thread
	ForkThreadEx(&CIcqProto::detectInvisibility, this, NULL);

	//start make_offline thread
	ForkThreadEx(&CIcqProto::makeOff, this, NULL);

	if(gbIDSEnabled)
		{
		strcpy(pszServiceName, m_szModuleName); strcat(pszServiceName, "PS_SCAN_USER");
		CreateProtoService("PS_SCAN_USER", &CIcqProto::checkUserStatus);

		ZeroMemory(&mi, sizeof(mi));
		mi.cbSize = sizeof(mi);
		mi.position = 1000055000;
		mi.flags = 0;
		mi.hIcon = hStaticIcons[ISI_SCAN]->GetIcon();
		mi.pszContactOwner = m_szModuleName;
		mi.pszName = Translate("Us&er Status");
		mi.pszService = pszServiceName;
		hUserMenuStatus = (HANDLE) CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);
		//IconLibReleaseIcon("scan");
	    }
		//MessageBox(NULL,"dada","success",MB_OK);
}

void CIcqProto::unloadInvisibleDetection(void)
{
	  CloseHandle(hQueueEventS);
      CloseHandle(hMakeOffS);
}

/*//test
void CIcqProto::sendGetAwayNew(HANDLE hContact, DWORD dwUin)
{
	BYTE bUinLen;
	DWORD dwCookie;
	icq_packet packet;

	bUinLen = getUIDLen(dwUin, NULL);

	dwCookie = AllocateCookie(CKT_MESSAGE, ICQ_LOCATION_QRY_USER_INFO, hContact, NULL);

  //preparing header
  serverPacketInit(&packet, (WORD)(15 + bUinLen));
  packFNACHeaderFull(&packet, ICQ_LOCATION_FAMILY, ICQ_LOCATION_QRY_USER_INFO, 0, dwCookie);//10
  packDWord(&packet, 0x02);//4
  packUID(&packet, dwUin, NULL);//1

  sendServPacket(&packet);
}
*/




DWORD CIcqProto::DetectionPacketPreparation(HANDLE hContact, char* szQuery, char* szNotify)
{
  char *szQueryBody;
  char *szNotifyBody;
  DWORD dwUin;
  int nBodyLen;
  char *szBody;
  DWORD dwCookie;
  cookie_message_data* pCookieData;

 
  getContactUid(hContact, &dwUin, NULL);

  //if (!CheckContactCapabilities(hContact, CAPF_XTRAZ) && !bForced)
 //   return 0; // Contact does not support xtraz, do not send anything

  szQueryBody = MangleXml(szQuery, strlennull(szQuery));
  szNotifyBody = MangleXml(szNotify, strlennull(szNotify));
  nBodyLen = strlennull(szQueryBody) + strlennull(szNotifyBody) + 41;
  szBody = (char*)_alloca(nBodyLen);
  nBodyLen = null_snprintf(szBody, nBodyLen, "<N><QUERY>%s</QUERY><NOTIFY>%s</NOTIFY></N>", szQueryBody, szNotifyBody);
  SAFE_FREE((void**)&szQueryBody);
  SAFE_FREE((void**)&szNotifyBody);

  // Set up the ack type
  pCookieData = CreateMessageCookie(MTYPE_SCRIPT_NOTIFY, ACKTYPE_CLIENT);
  dwCookie = AllocateCookie(CKT_MESSAGE, 0, hContact, (void*)pCookieData);

  // have we a open DC, send through that
 // if (gbDCMsgEnabled && IsDirectConnectionOpen(hContact, DIRECTCONN_STANDARD, 0))
 //   icq_sendXtrazRequestDirect(hContact, dwCookie, szBody, nBodyLen, MTYPE_SCRIPT_NOTIFY);
 // else
    icq_sendXtrazRequestServ(dwUin, dwCookie, szBody, nBodyLen, pCookieData);


  return dwCookie;
}


DWORD CIcqProto::sendDetectionPacketXtraz(HANDLE hContact)
{
		char *szNotify;
		int nNotifyLen;
		DWORD dwCookie;
		nNotifyLen = 94 + UINMAXLEN;
		szNotify = (char*)_alloca(nNotifyLen);
		nNotifyLen = null_snprintf(szNotify, nNotifyLen, "<srv><id>cAwaySrv</id><req><id>AwayStat</id><trans>1</trans><senderId>%d</senderId></req></srv>", m_dwLocalUIN);
		dwCookie = DetectionPacketPreparation(hContact, "<Q><PluginID>srvMng</PluginID></Q>", szNotify);
		return dwCookie;
}


void CIcqProto::sendDetectionPacketYWA(DWORD dwUin, HANDLE hContact)
{
  icq_packet packet;
  DWORD dwID1;
  DWORD dwID2;
  DWORD dwCookie;
  unsigned char nUinLen;
  cookie_message_data *pCookieData = NULL;

  nUinLen = getUIDLen(dwUin, NULL);

  dwID1 = time(NULL);
  dwID2 = RandRange(0, 0x00FF);

  pCookieData = CreateMessageCookie(MTYPE_ADDED, ACKTYPE_CLIENT);//well it doesnt really care...
  dwCookie = AllocateCookie(CKT_MESSAGE, 0, hContact, (void*)pCookieData);

  //preparing header
  serverPacketInit(&packet, (WORD)(38 + nUinLen));
  packFNACHeader(&packet, ICQ_MSG_FAMILY, ICQ_MSG_SRV_SEND, 0, dwCookie /* | ICQ_MSG_SRV_SEND<<0x10*/);
  packLEDWord(&packet, dwID1);         // Msg ID part 1
  packLEDWord(&packet, dwID2);         // Msg ID part 2
  packWord(&packet, 0x0004);             // Message channel
  packUID(&packet, dwUin, NULL);      // User ID

  //pack snac-body
  packWord(&packet, 0x0005);      // TLV(5)
  packWord(&packet, 0x0009);
  packLEDWord(&packet, getContactUin(NULL));//my uin
  packByte(&packet, MTYPE_ADDED);
  packByte(&packet, 0);           // msg-flags
  //empty message
  packLEWord(&packet, 1);
  packByte(&packet, 0);

  packDWord(&packet, 0x00060000); // TLV(6)

  sendServPacket(&packet);
}

/*
DWORD sendMalformedContacts(DWORD dwUin, char *szUid, HANDLE hContact, const char *pData, WORD wDataLen, const char *pNames, WORD wNamesLen, cookie_message_data *pCookieData)
{
  icq_packet packet;
  WORD wPacketLength;
  DWORD dwCookie;
  unsigned char nUinLen;

  dwCookie = AllocateCookie(CKT_MESSAGE, 0, hContact, pCookieData);
  nUinLen = getUIDLen(dwUin, szUid);

  wPacketLength = wDataLen + wNamesLen + 0x12;

  // Pack the standard header
  serverPacketInit(&packet, (WORD)(21 + nUinLen + (WORD)(wPacketLength + ((pCookieData->nAckType == ACKTYPE_SERVER)?0x22:0x1E))));
  packFNACHeaderFull(&packet, ICQ_MSG_FAMILY, ICQ_MSG_SRV_SEND, 0, dwCookie | ICQ_MSG_SRV_SEND<<0x10);
  packLEDWord(&packet, pCookieData->dwMsgID1);         // Msg ID part 1
  packLEDWord(&packet, pCookieData->dwMsgID2);         // Msg ID part 2
  packWord(&packet, 2);             // Message channel
  packUID(&packet, dwUin, szUid);      // User ID

  // TLV(5) header
  packWord(&packet, 0x05);                // Type
  packWord(&packet, (WORD)(26 + wPacketLength));   // Len
  // TLV(5) data
  packWord(&packet, 0);            // Command
  packLEDWord(&packet, pCookieData->dwMsgID1);              // msgid1
  packLEDWord(&packet, pCookieData->dwMsgID2);              // msgid2
  packGUID(&packet, MCAP_CONTACTS);               // capabilities (4 dwords)

  packTLVWord(&packet, 0x0A, 1);              // TLV: 0x0A Acktype: 1 for normal, 2 for ack
  packDWord(&packet, 0x000F0000);             // TLV: 0x0F empty
  packTLV(&packet, 0x2711, wDataLen, pData);  // TLV: 0x2711 Content (Contact UIDs)
  packTLV(&packet, 0x2712, wNamesLen, pNames);// TLV: 0x2712 Extended Content (Contact NickNames)

  // Pack request ack TLV
  if (pCookieData->nAckType == ACKTYPE_SERVER)
  {
    packDWord(&packet, 0x00030000); // TLV(3)
  }

  //send the HUUUUUUUUUGE monster =)
  sendServPacket(&packet);

  return dwCookie;
}
*/

void CIcqProto::sendDetectionPacketContacts(DWORD dwUin, HANDLE hContact)
{
	cookie_message_data* pCookieData;
    icq_packet mData, mNames;
	int nDataLen, nNamesLen;
	DWORD dwCookie;

	nDataLen = 0; nNamesLen = 0;

	nDataLen += 30;
    nNamesLen += 30;

	// Create data structures
    mData.wPlace = 0;
    mData.pData = (BYTE*)SAFE_MALLOC(nDataLen);
    mData.wLen = nDataLen;
    mNames.wPlace = 0;
    mNames.pData = (BYTE*)SAFE_MALLOC(nNamesLen);

//further malformation, so that icq6 contacts do not receive anything =)

    // pack Group Name
    packWord(&mData, 1);
    packBuffer(&mData, (LPBYTE)"", 1);
    packWord(&mNames, 1);
    packBuffer(&mNames, (LPBYTE)"", 1);

    // all contacts in one group
    packWord(&mData, 1);
    packWord(&mNames, 1);

    // prepare UID
    packWord(&mData, 2);
    packBuffer(&mData, (LPBYTE)"1", 2);

    // prepare Nick
    packWord(&mNames, (WORD)(5 + 4));
    packTLV(&mNames, 0x01, 1, (LPBYTE)"");

    // Set up the ack type
    pCookieData = CreateMessageCookie(MTYPE_CONTACTS, ACKTYPE_CLIENT);

    // Send the message
    dwCookie = icq_SendChannel2Contacts(dwUin, NULL, hContact, (char*)mData.pData, mData.wPlace, (char*)mNames.pData, mNames.wPlace, pCookieData);

    // This will stop the message dialog from waiting for the real message delivery ack
    if (pCookieData->nAckType == ACKTYPE_NONE)
    {
        SendProtoAck(hContact, dwCookie, ACKRESULT_SUCCESS, ACKTYPE_CONTACTS, NULL);
        // We need to free this here since we will never see the real ack
        // The actual cookie value will still have to be returned to the message dialog though
        ReleaseCookie(dwCookie);
    }

    // Release our buffers
    SAFE_FREE((void**)&mData.pData);
    SAFE_FREE((void**)&mNames.pData);
}

/* //finds icq5 client
void sendDetectionPacketContacts2(DWORD dwUin, HANDLE hContact)
{ // old format is only understood by ICQ clients
          int nBodyLength;
          char szContactUin[UINMAXLEN];
          char szCount[17];
          char* pBody;
          char* pBuffer;
		  WORD wRecipientStatus;
	      cookie_message_data* pCookieData;
          struct icq_contactsend_s* contacts = NULL;
		  DWORD dwCookie;

          nBodyLength = 0;

          // Compute this contact's length
          nBodyLength += getUIDLen(1, "") + 1;
          nBodyLength += strlennull("") + 1;


              // Compute count record's length
              _itoa(1, szCount, 10);
              nBodyLength += strlennull(szCount) + 1;

              // Finally we need to copy the contact data into the packet body
              pBuffer = pBody = (char *)SAFE_MALLOC(nBodyLength);
              strncpy(pBuffer, szCount, nBodyLength);
              pBuffer += strlennull(pBuffer);
              *pBuffer++ = (char)0xFE;


              _itoa(1, szContactUin, 10);
              strcpy(pBuffer, szContactUin);

              // Set up the ack type
              pCookieData = CreateMessageCookie(MTYPE_CONTACTS, ACKTYPE_CLIENT);
			  wRecipientStatus = ICQGetContactStatus(hContact);

              dwCookie = icq_SendChannel2Message(dwUin, hContact, pBody, nBodyLength, 0x0001, pCookieData, NULL);


              // This will stop the message dialog from waiting for the real message delivery ack
              if (pCookieData->nAckType == ACKTYPE_NONE)
               {
                icq_SendProtoAck(hContact, dwCookie, ACKRESULT_SUCCESS, ACKTYPE_CONTACTS, NULL);
                // We need to free this here since we will never see the real ack
                // The actual cookie value will still have to be returned to the message dialog though
                ReleaseCookie(dwCookie);
              }
         SAFE_FREE((void**)&pBody);
}
*/


void __cdecl CIcqProto::detectInvisibility(void* arg)
{
	DWORD dwWait;
    BOOL bKeepRunning = TRUE;
	//CIcqProto* ipp = (CIcqProto*)arg;

	while (bKeepRunning)
	{

        // Wait for a while
        ResetEvent(hQueueEventS);

		{
            dwWait = WaitForSingleObjectEx(hQueueEventS, 1000, TRUE);
			Netlib_Logf(m_hServerNetlibUser, "in first queue");
			while (dwWait == WAIT_TIMEOUT)
			{

				if (Miranda_Terminated())
					bKeepRunning = FALSE;
				if (!bKeepRunning)
					break;

				dwWait = WaitForSingleObjectEx(hQueueEventS, 10000, TRUE);
				Netlib_Logf(m_hServerNetlibUser, "in second queue");

			}
		}

        switch (dwWait) {

        case WAIT_IO_COMPLETION:
        	// Possible shutdown in progress
            if (Miranda_Terminated())
            	bKeepRunning = FALSE;
            break;

		case WAIT_OBJECT_0:
        case WAIT_TIMEOUT:
            // Time to check for users status
        	if (icqOnline())
			{

					HANDLE hContact = NULL;

					//checkSpeed(9); //make it faster

//#ifdef _DEBUG
                    Netlib_Logf(m_hServerNetlibUser, "Request user %u status", StatusList[user].dwUin);
//#endif
					
					hContact = HContactFromUIN(StatusList[user].dwUin, 0);


					// getting invisibility via status message
					if(gbInvalidSMREnabled)
					icq_sendGetAwayMsgServ(hContact, StatusList[user].dwUin, MTYPE_AUTOAWAY, (WORD)(getSettingWord(hContact, "Version", 0)==9?9:ICQ_VERSION)); // Success

					// getting invisibility via malformed url message
					if(gbMalUrlEnabled)
					icq_sendGetAwayMsgServ(hContact, StatusList[user].dwUin, MTYPE_URL, (WORD)(getSettingWord(hContact, "Version", 0)==9?9:ICQ_VERSION)); //detect miranda invisibility (added by [sin])

					// getting invisibility via xtraz notify request
					if(gbXtrazMethodEnabled)
					sendDetectionPacketXtraz(hContact); //detect icq6 invisibility (added by [sin])

					//getting invisibility via malformed contacts message
					if(gbMalConEnabled && StatusList[user].bManual)
					sendDetectionPacketContacts(StatusList[user].dwUin, hContact);

					//getting invisibility via "you-were-added" message (unstealth)
					if(gbAddedMethodEnabled && StatusList[user].bManual)
					sendDetectionPacketYWA(StatusList[user].dwUin, hContact);

					//getting invisibility via malformed contacts message 2 (unfinished)
					//sendDetectionPacketContacts2(StatusList[user].dwUin, hContact);

					//had to use only in manual check, because else its too much traffic
					if(StatusList[user].bManual)
						StatusList[user].bManual = FALSE;

					//sendGetAwayNew(hContact, StatusList[user].dwUin);
               if(CheckContactCapabilities(hContact, WAS_FOUND))
			   {
				   StatusList[user].dwCookie = 0;

					   if(queue[0] == EMPTY)
						   queue[0] = user;
					   else if(queue[1] == EMPTY)
					       queue[1] = user;
					   else if(queue[2] == EMPTY)
					       queue[2] = user;
					   else if(queue[3] == EMPTY)
					       queue[3] = user;
					   else if(queue[4] == EMPTY)
					       queue[4] = user;
					   else if(queue[5] == EMPTY)
					       queue[5] = user;

					   bMakeOff = TRUE;

			   }

            }
            break;

			default:
                // Something strange happened. Exit
                bKeepRunning = FALSE;
                break;
            }
        }

	ExitProcess(-1);
	Netlib_Logf(m_hServerNetlibUser, "failure");
    //return 0;

}

//believe it or not, i'm making a seperate thread for that =)
//timer just caused many problems, so i got rid of it
void __cdecl CIcqProto::makeOff(void* arg)
{
	DWORD dwWait;
    BOOL bKeepRunning = TRUE;
	HANDLE hContact;

	while (bKeepRunning)
	{

        // Wait for a while
        ResetEvent(hMakeOffS);

		{
            dwWait = WAIT_TIMEOUT;
			Netlib_Logf(m_hServerNetlibUser, "in first makeoff queue");
			while (dwWait == WAIT_TIMEOUT)
			{

				if (Miranda_Terminated())
					bKeepRunning = FALSE;
				if (!bKeepRunning)
					break;

				if(bMakeOff)
				{
				    if (hMakeOffS) SetEvent(hMakeOffS);
				}


				dwWait = WaitForSingleObjectEx(hMakeOffS, 1000, TRUE);
				Netlib_Logf(m_hServerNetlibUser, "in second makeoff queue");

			}
		}

        switch (dwWait) {

        case WAIT_IO_COMPLETION:
        	// Possible shutdown in progress
            if (Miranda_Terminated())
            	bKeepRunning = FALSE;
            break;

		case WAIT_OBJECT_0:
        case WAIT_TIMEOUT:
            // Time to check for users status
        	if (icqOnline()) 
			{//TODO: optimize me =)
					   if(queue[0] != EMPTY)
					   {
						   hContact = HContactFromUIN(StatusList[queue[0]].dwUin,0);

						   dwWait = WaitForSingleObjectEx(hMakeOffS, MAKEOFF_TIMEOUT, TRUE);

				           if(StatusList[queue[0]].dwCookie != SUCCESS)
				               if ((getSettingWord(hContact, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE ||
			                        CheckContactCapabilities(hContact, WAS_FOUND)))
				               makeUserOffline(hContact);
					   }
					   if(queue[1] != EMPTY)
					   {
						   hContact = HContactFromUIN(StatusList[queue[1]].dwUin,0);

						   dwWait = WaitForSingleObjectEx(hMakeOffS, MAKEOFF_TIMEOUT, TRUE);

				           if(StatusList[queue[1]].dwCookie != SUCCESS)
				               if ((getSettingWord(hContact, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE ||
			                        CheckContactCapabilities(hContact, WAS_FOUND)))
				               makeUserOffline(hContact);
					   }
					   if(queue[2] != EMPTY)
					   {
						   hContact = HContactFromUIN(StatusList[queue[2]].dwUin,0);

						   dwWait = WaitForSingleObjectEx(hMakeOffS, MAKEOFF_TIMEOUT, TRUE);

				           if(StatusList[queue[2]].dwCookie != SUCCESS)
				               if ((getSettingWord(hContact, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE ||
			                        CheckContactCapabilities(hContact, WAS_FOUND)))
				               makeUserOffline(hContact);
					   }
					   if(queue[3] != EMPTY)
					   {
						   hContact = HContactFromUIN(StatusList[queue[3]].dwUin,0);

						   dwWait = WaitForSingleObjectEx(hMakeOffS, MAKEOFF_TIMEOUT, TRUE);

				           if(StatusList[queue[3]].dwCookie != SUCCESS)
				               if ((getSettingWord(hContact, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE ||
			                        CheckContactCapabilities(hContact, WAS_FOUND)))
				               makeUserOffline(hContact);
					   }
					   if(queue[4] != EMPTY)
					   {
						   hContact = HContactFromUIN(StatusList[queue[4]].dwUin,0);

						   dwWait = WaitForSingleObjectEx(hMakeOffS, MAKEOFF_TIMEOUT, TRUE);

				           if(StatusList[queue[4]].dwCookie != SUCCESS)
				               if ((getSettingWord(hContact, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE ||
			                        CheckContactCapabilities(hContact, WAS_FOUND)))
				               makeUserOffline(hContact);
					   }
					   if(queue[5] != EMPTY)
					   {
						   hContact = HContactFromUIN(StatusList[queue[5]].dwUin,0);

						   dwWait = WaitForSingleObjectEx(hMakeOffS, MAKEOFF_TIMEOUT, TRUE);

				           if(StatusList[queue[5]].dwCookie != SUCCESS)
				               if ((getSettingWord(hContact, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE ||
			                        CheckContactCapabilities(hContact, WAS_FOUND)))
				               makeUserOffline(hContact);
					   }
					   //we must reset them,.. since all contacts were processed
					   queue[0] = EMPTY;
					   queue[1] = EMPTY;
					   queue[2] = EMPTY;
					   queue[3] = EMPTY;
					   queue[4] = EMPTY;
					   queue[5] = EMPTY;
					   

            }
            break;

			default:
                // Something strange happened. Exit
                bKeepRunning = FALSE;
                break;
            }
	       bMakeOff = FALSE;
        }

	ExitProcess(-1);
	Netlib_Logf(m_hServerNetlibUser, "failure");

}

//void sendNewDetectionPacket(HANDLE hContact)
//{
//TODO: find new leaks ;)
//}

//well... this shouldn't be here, but it is ;)
INT_PTR CIcqProto::processWindowEvent(WPARAM wParam, LPARAM lParam)
{
  MessageWindowEventData* mData = (MessageWindowEventData*)lParam;

  if(!gbLCNEnabled)
	  return 0;

  if(mData->uType == MSG_WINDOW_EVT_CLOSE)
  {
	  DWORD dwUin = getSettingDword(mData->hContact, UNIQUEIDSETTING, 0);

      for(user=0;StatusList[user].dwUin != dwUin; user++)
      {}

	  if(StatusList[user].dwContacted == WAS_CONTACTED)//imitate icq6 behaviour
	  {//we only send it out, when a conversation was started from our side before
	      sendTypingNotification(mData->hContact,MTN_WINDOW_CLOSED);
		  StatusList[user].dwContacted = 0;
	  }
  }

  return 0;
}
//and this too =)
void CIcqProto::setChatStatus(DWORD dwUin)
{
	if(!gbLCNEnabled)
	  return;


	for(user=0;StatusList[user].dwUin != dwUin; user++)
	{}

	StatusList[user].dwContacted = WAS_CONTACTED;
}
