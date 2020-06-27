#include "ICQStatusChecker.h"
#include "resource.h"

HINSTANCE hInst;

HANDLE hMenuCheckStatus;

HANDLE hNetlibUser;

PLUGINLINK *pluginLink;

Options options;

HANDLE hContact2;

PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
	"ICQ Status Checker",
	PLUGIN_MAKE_VERSION(1,2,0,0),
	"Check status ICQ users and detect invisible user for all IM clients. Used http://kanicq.ru/invisible/en.",
	"Forrest79",
	"info@forrest79",
	"© 2008 Forrest79.net",
	"http://forrest79.net",
	0,		//not transient
	0,		//doesn't replace anything built-in
    {0x44d0ebb2, 0x1485, 0x13a6, { 0x56, 0x7e, 0x1d, 0xd7, 0xfd, 0xe0, 0x67, 0x15 }} // {44D0EBB2-1485-13A6-567E1DD7FDE06715}
};

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst=hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	return &pluginInfo;
}

static const MUUID interfaces[] = {MIID_ICQSTATUSCHECKER, MIID_LAST};
extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}

void LoadOptions() {
	DBVARIANT dbv = {0};
	if(!DBGetContactSetting(0, MODULE, "Url", &dbv)) {
        strcpy(options.url, dbv.pszVal);
	} else {
        strcpy(options.url, URL);
	}
	DBFreeVariant(&dbv);

    if(DBGetContactSettingByte(0, MODULE, "Method", 0) == 2) {
        options.method = 2;
    } else {
        options.method = 1;
    }

    if(DBGetContactSettingByte(0, MODULE, "ShowMessages", 1) == 1) {
        options.showMessages = TRUE;
    } else {
        options.showMessages = FALSE;
    }
}

void SaveOptions() {
    DBWriteContactSettingByte(0, MODULE, "Method", options.method);
    DBWriteContactSettingByte(0, MODULE, "ShowMessages", options.showMessages);
}

static BOOL CALLBACK DlgProcOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_INITDIALOG: {
            TranslateDialogDefault(hwndDlg);
            {
                if(options.method == 1) {
                    CheckDlgButton(hwndDlg, IDC_RADIO_METHOD1, TRUE);
                } else {
                    CheckDlgButton(hwndDlg, IDC_RADIO_METHOD2, TRUE);
                }

                if(options.showMessages == 1) {
                    CheckDlgButton(hwndDlg, IDC_CHECK_SHOWMESSAGES, TRUE);
                }
            }

            return FALSE;
        }

        case WM_COMMAND:
            if(HIWORD(wParam) == BN_CLICKED) {
                switch(LOWORD(wParam)) {
                    case IDC_RADIO_METHOD1:
                    case IDC_RADIO_METHOD2:
                    case IDC_CHECK_SHOWMESSAGES:
                        SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
                        return TRUE;
                }
            }

            break;

        case WM_NOTIFY:
            if(((LPNMHDR)lParam)->code == (DWORD)PSN_APPLY) {
                if(IsDlgButtonChecked(hwndDlg, IDC_RADIO_METHOD1)) {
                    options.method = 1;
                } else {
                    options.method = 2;
                }

                if(IsDlgButtonChecked(hwndDlg, IDC_CHECK_SHOWMESSAGES)) {
                    options.showMessages = 1;
                } else {
                    options.showMessages = 0;
                }

                SaveOptions();

                return TRUE;
            }

            break;

        case WM_DESTROY:
            break;
	}

	return FALSE;
}

int OptionsInit(WPARAM wParam, LPARAM lParam) {
	OPTIONSDIALOGPAGE odp = {0};
	odp.cbSize = 60;
	odp.position = -790000000;
	odp.hInstance = hInst;
	odp.pszTemplate	= MAKEINTRESOURCE(IDD_OPT);
	odp.pszTitle = NAME;
	odp.pszGroup = Translate("Status");
	odp.flags = ODPF_BOLDGROUPS;
	odp.pfnDlgProc = DlgProcOpts;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM)&odp);

 	return 0;
}

void showError(char* text) {
    if(ServiceExists(MS_POPUP_SHOWMESSAGE)) {
		char popup[255];
		sprintf(popup, "%s: %s", NAME, text);
		PUShowMessage(popup, SM_WARNING);
    } else {
        char titleError[100];
        char messageError[255];

        sprintf(titleError, "%s [%s]", NAME, Translate("Error"));
        sprintf(messageError, "[%s] %s", Translate("ERROR"), text);

        MessageBox(NULL, messageError, titleError, MB_ICONERROR | MB_OK);
    }
}

void showStatus(int uin, int status, HANDLE hContact) {
    char statusText[20];

    HICON statusIcon = NULL;

    char *nick = (char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, (LPARAM)0);

	DBVARIANT dbv = {0};
	DBGetContactSettingString(hContact, "Protocol", "p", &dbv);

    if(status == STATUS_OFFLINE) {
        DBWriteContactSettingDword(hContact, dbv.pszVal, "Status", (DWORD)40071);
        strcpy(statusText, "Offline");
        statusIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_OFFLINE), IMAGE_ICON, 0, 0, 0);
    } else if(status == STATUS_ONLINE) {
        DBWriteContactSettingDword(hContact, dbv.pszVal, "Status", (DWORD)40072);
        strcpy(statusText, "Online");
        statusIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_ONLINE), IMAGE_ICON, 0, 0, 0);
    } else if(status == STATUS_AWAY) {
        DBWriteContactSettingDword(hContact, dbv.pszVal, "Status", (DWORD)40073);
        strcpy(statusText, "Away");
        statusIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_AWAY), IMAGE_ICON, 0, 0, 0);
    } else if(status == STATUS_DND) {
        DBWriteContactSettingDword(hContact, dbv.pszVal, "Status", (DWORD)40074);
        strcpy(statusText, "DND");
        statusIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_DND), IMAGE_ICON, 0, 0, 0);
    } else if(status == STATUS_NA) {
        DBWriteContactSettingDword(hContact, dbv.pszVal, "Status", (DWORD)40075);
        strcpy(statusText, "N/A");
        statusIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_NA), IMAGE_ICON, 0, 0, 0);
    } else if(status == STATUS_OCCUPIED) {
        DBWriteContactSettingDword(hContact, dbv.pszVal, "Status", (DWORD)40076);
        strcpy(statusText, "Occupied");
        statusIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_OCCUPIED), IMAGE_ICON, 0, 0, 0);
    } else if(status == STATUS_FREEFORCHAT) {
        DBWriteContactSettingDword(hContact, dbv.pszVal, "Status", (DWORD)40077);
        strcpy(statusText, "Free for chat");
        statusIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_FREEFORCHAT), IMAGE_ICON, 0, 0, 0);
    } else if(status == STATUS_INVISIBLE) {
        DBWriteContactSettingDword(hContact, dbv.pszVal, "Status", (DWORD)40078);
        strcpy(statusText, "Invisible");
        statusIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_INVISIBLE), IMAGE_ICON, 0, 0, 0);
    }

	DBFreeVariant(&dbv);

    if(options.showMessages && ServiceExists(MS_POPUP_ADDPOPUP)) {
        POPUPDATAEX* ppd = (POPUPDATAEX*)malloc(sizeof(POPUPDATAEX));
        memset((void *)ppd, 0, sizeof(POPUPDATAEX));

        ppd->lchContact = hContact;

        ppd->lchIcon = statusIcon;

        strncpy(ppd->lpzContactName, nick, MAX_CONTACTNAME);
        strncpy(ppd->lpzText, statusText, MAX_SECONDLINE);

        ppd->PluginWindowProc = NULL;
        ppd->PluginData = NULL;

        ppd->PluginData = (void *)hContact;

        if(ServiceExists(MS_POPUP_ADDPOPUPEX)) {
            CallService(MS_POPUP_ADDPOPUPEX, (WPARAM)ppd, 0);
        } else {
            if(ServiceExists(MS_POPUP_ADDPOPUP)) {
                CallService(MS_POPUP_ADDPOPUP, (WPARAM)ppd, 0);
            }
        }

        free( ppd );
    } else if(options.showMessages) {
        char messageStatus[255];
        char titleStatus[100];

        sprintf(titleStatus, "%s [%s]", NAME, Translate("Status"));
        sprintf(messageStatus, "[%s] %s", nick, statusText);

        MessageBox(NULL, messageStatus, titleStatus, MB_ICONINFORMATION | MB_OK);
    }
}

int HttpGet(int uin, int method, HANDLE hContact) {
    char url[100];
    sprintf(url, options.url, uin, method);

	NETLIBHTTPREQUEST *resp = 0;
	NETLIBHTTPREQUEST req = {0};

	req.cbSize = sizeof(req);
	req.requestType = REQUEST_GET;
	req.szUrl = url;
	req.flags = NLHRF_DUMPASTEXT;

	resp = (NETLIBHTTPREQUEST *)CallService(MS_NETLIB_HTTPTRANSACTION, (WPARAM)hNetlibUser, (LPARAM)&req);

	if(!resp || (resp->resultCode != 200) || (resp->dataLength != 1)) {
        CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT, 0, (LPARAM)resp);

        showError(Translate("An error occured while checking status (commucation with server), please try it again later..."));

        return FALSE;
	} else {
        CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT, 0, (LPARAM)resp);

        int status = atoi(resp->pData);

        if(status == STATUS_ERROR) {
            showError(Translate("An error occured while checking status (service error), please try it again later..."));

            return FALSE;
        } else {
            showStatus((int)uin, status, hContact);

            return TRUE;
        }
	}
}

unsigned long __stdcall GetStatusThread(void *param) {
    StatusParam status = *(StatusParam*)param;

	HttpGet(status.uin, options.method, status.hContact);

	return 0;
}

static int ICQCheckStatusMenuCommand(WPARAM wParam, LPARAM lParam) {
	HANDLE hContact = (HANDLE)wParam;

	DBVARIANT dbv = {0};
	DWORD uin = 0;
	DBGetContactSettingString(hContact, "Protocol", "p", &dbv);
	uin = DBGetContactSettingDword(hContact, dbv.pszVal, "UIN", 0);
	DBFreeVariant(&dbv);
	if(uin > 0) {
	    DWORD tid;

	    hContact2 = hContact;

        StatusParam param;
        param.uin = (int)uin;
        param.hContact = hContact;

        CloseHandle(CreateThread(0, 0, GetStatusThread, &param, 0, &tid));
//        CloseHandle(CreateThread(0, 0, GetStatusThread, &uin, 0, &tid));
	} else {
		MessageBox(NULL, Translate("Sorry, this is not ICQ user. I can check status only ICQ users :("), NAME, MB_OK);
	}

	return 0;
}

static int BuildMenu(WPARAM wParam, LPARAM lParam) {
	CLISTMENUITEM mi;

	HANDLE hContact = (HANDLE)wParam;

	DBVARIANT dbv = {0};
	DWORD uin = 0;
	DBGetContactSettingString(hContact, "Protocol", "p", &dbv);
	uin = DBGetContactSettingDword(hContact, dbv.pszVal, "UIN", 0);
	DBFreeVariant(&dbv);

	mi.cbSize = sizeof(CLISTMENUITEM);
	mi.flags = CMIM_FLAGS;
	mi.pszContactOwner = NULL;

	if(uin > 0) {
		mi.flags = CMIM_FLAGS;
	} else {
		mi.flags |= CMIF_HIDDEN;
	}

	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hMenuCheckStatus, (LPARAM)&mi);

	return 0;
}

static int PluginInit(WPARAM wParam, LPARAM lParam) {
	CLISTMENUITEM mi;

	CreateServiceFunction("ICQStatusChecker/ICQCheckStatusMenuCommand", ICQCheckStatusMenuCommand);

	mi.cbSize = sizeof(CLISTMENUITEM);
	mi.flags = 0;
	mi.hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_MENU), IMAGE_ICON, 0, 0, 0);
	mi.pszContactOwner = NULL;
	mi.position = 1000000000;
	mi.pszName = Translate("ICQ [Check status]");
	mi.pszService = "ICQStatusChecker/ICQCheckStatusMenuCommand";

	hMenuCheckStatus = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);

	HookEvent(ME_CLIST_PREBUILDCONTACTMENU, BuildMenu);

    NETLIBUSER nl_user = {0};
    nl_user.cbSize = sizeof(nl_user);
    nl_user.szSettingsModule = MODULE;
    nl_user.szDescriptiveName = NAME;
    nl_user.flags = NUF_OUTGOING | NUF_HTTPCONNS;

    hNetlibUser = (HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nl_user);

	return FALSE;
}

extern "C" int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink = link;

	HookEvent(ME_SYSTEM_MODULESLOADED, PluginInit);

	HookEvent(ME_OPT_INITIALISE, OptionsInit);

    LoadOptions();

	return FALSE;
}

extern "C" int __declspec(dllexport) Unload(void)
{
	CallService(MS_NETLIB_CLOSEHANDLE, (WPARAM)hNetlibUser, 0);

	return 0;
}
