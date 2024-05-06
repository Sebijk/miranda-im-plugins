/*
MSN Account Setup


PLEASE CONTACT ME FIRST IF YOU WANT TO USE PART OF MY CODE IN YOUR OWN WORK :-)

Felipe Brahm - souFrag
ICQ#50566818
http://www.soufrag.cl

You cand send me an e-mail from my webpage :-)

Some ideas taken from ICQ.dll source.

*/

#include "../../AggressiveOptimize.h"

#include <windows.h>
#include <stdio.h>
#include "resrc1.h"

/*
#include "../miranda_src/SDK/Headers_c/newpluginapi.h"
#include "../miranda_src/SDK/Headers_c/m_skin.h"
#include "../miranda_src/SDK/Headers_c/m_langpack.h"
#include "../miranda_src/SDK/Headers_c/m_database.h"
#include "../miranda_src/SDK/Headers_c/m_options.h"
#include "../miranda_src/SDK/Headers_c/m_utils.h"
*/

#include "../../headers_c/newpluginapi.h"
#include "../../headers_c/m_langpack.h"
#include "../../headers_c/m_database.h"
//#include "../../headers_c/m_protosvc.h"
#include "../../headers_c/m_utils.h"
//#include "../../headers_c/m_options.h"
//#include "../../headers_c/m_utils.h"
//#include "../../headers_c/m_clist.h"
//#include "../../headers_c/m_skin.h"

//#define	dbERROR	-999999
#define	MSN_MAX_EMAIL_LEN	128

HINSTANCE hInst;
PLUGINLINK *pluginLink;

PLUGININFO pluginInfo={
	sizeof(PLUGININFO),
	"MSN Account Setup",
	PLUGIN_MAKE_VERSION(0,0,1,2),
	"Shows a dialog to enter your e-mail and password on first run or if there's no e-mail/password stored. Shows a dialog to enter your e-mail and password on first run or if there's no e-mail/password stored. Also, it gives you the possibility not to store e-mail/password (they are deleted on exit if you select the option not remember password/e-mail).",
	"Felipe Brahm - souFrag",
	"soufrag <at> sourceforge.net",
	"© 2005 Felipe Brahm - souFrag",
	"http://www.soufrag.cl/",
	0,		//not transient
	0		//doesn't replace anything built-in
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst=hinstDLL;
	return TRUE;
}

__declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	return &pluginInfo;
}

BOOL CALLBACK msn_FirstRunDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndOwner;
	RECT rc, rcDlg, rcOwner;
	DBVARIANT dbv;

    switch (msg)
	{
		
	case WM_INITDIALOG:
        {
            TranslateDialogDefault(hwndDlg);

			CheckDlgButton(hwndDlg, IDC_CHECKEMAIL, DBGetContactSettingByte(NULL,"MSNAccountSetup","rememberEmail",1) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECKPASS, DBGetContactSettingByte(NULL,"MSNAccountSetup","rememberPass",1) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECKALWAYS, DBGetContactSettingByte(NULL,"MSNAccountSetup","showAlways",0) ? BST_CHECKED : BST_UNCHECKED);

			if(!DBGetContactSetting(NULL, "MSN", "e-mail", &dbv))
				SetDlgItemText(hwndDlg,IDC_EMAIL,dbv.pszVal);
			
			DBFreeVariant(&dbv);

			if(!DBGetContactSetting( NULL, "MSN", "Password", &dbv )) {
				CallService( MS_DB_CRYPT_DECODESTRING, strlen( dbv.pszVal )+1, ( LPARAM )dbv.pszVal );
				SetDlgItemText( hwndDlg, IDC_PASS, dbv.pszVal );
			}
			
			DBFreeVariant(&dbv);

			if(!DBGetContactSetting(NULL, "MSN", "Nick", &dbv))
				SetDlgItemText(hwndDlg,IDC_NICK,dbv.pszVal);

			DBFreeVariant(&dbv);

			/* asteriscos en el password */

			SendDlgItemMessage(hwndDlg,
                               IDC_PASS, 
                               EM_SETPASSWORDCHAR, 
                               (WPARAM) '*', 
                               (LPARAM) 0); 

			/* end asteriscos */


			/*  centrar dialogbox  */
			// Get the owner window and dialog box rectangles. 

			if ((hwndOwner = GetParent(hwndDlg)) == NULL)
				hwndOwner = GetDesktopWindow(); 

			GetWindowRect(hwndOwner, &rcOwner); 
			GetWindowRect(hwndDlg, &rcDlg); 
			CopyRect(&rc, &rcOwner); 

			// Offset the owner and dialog box rectangles so that 
			// right and bottom values represent the width and 
			// height, and then offset the owner again to discard 
			// space taken up by the dialog box. 

			OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
			OffsetRect(&rc, -rc.left, -rc.top); 
			OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 

			// The new position is the sum of half the remaining 
			// space and the owner's original position. 

			SetWindowPos(hwndDlg, 
			HWND_TOP, 
			rcOwner.left + (rc.right / 2), 
			rcOwner.top + (rc.bottom / 2), 
			0, 0,          // ignores size arguments 
			SWP_NOSIZE); 

			if(GetDlgCtrlID((HWND) wParam) != IDD_MSNACCOUNTSETUP) {
				SetFocus(GetDlgItem(hwndDlg, IDD_MSNACCOUNTSETUP));
				return FALSE;
			}
			/*  centrar dialogbox end */
			
			return TRUE;

        }
		break;

	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		break;
		
	case WM_COMMAND:
		{

			switch (LOWORD(wParam))
			{

			case IDC_REGISTER:
				{
					CallService( MS_UTILS_OPENURL, 1, ( LPARAM )"http://lc2.law13.hotmail.passport.com/cgi-bin/register" );
					break;
				}

			case IDOK:
				{

					char screenStr[ 300 ];
					char tEmail[ MSN_MAX_EMAIL_LEN ];

					GetDlgItemText( hwndDlg, IDC_EMAIL, tEmail, sizeof( tEmail ));
					DBWriteContactSettingString(NULL,"MSN","e-mail",tEmail);

					GetDlgItemText( hwndDlg, IDC_PASS, screenStr, sizeof( screenStr ));
					CallService( MS_DB_CRYPT_ENCODESTRING, sizeof( screenStr ),( LPARAM )screenStr );
					DBWriteContactSettingString(NULL,"MSN","Password",screenStr);

					GetDlgItemText( hwndDlg, IDC_NICK, screenStr, sizeof( screenStr ));
					DBWriteContactSettingString(NULL,"MSN","Nick",screenStr);

					if(!IsDlgButtonChecked(hwndDlg,IDC_CHECKEMAIL))
						DBWriteContactSettingByte(NULL,"MSNAccountSetup","rememberEmail",0);
					else
						DBDeleteContactSetting(NULL,"MSNAccountSetup","rememberEmail");
					
					if(!IsDlgButtonChecked(hwndDlg,IDC_CHECKPASS))
						DBWriteContactSettingByte(NULL,"MSNAccountSetup","rememberPass",0);
					else
						DBDeleteContactSetting(NULL,"MSNAccountSetup","rememberPass");

					if(IsDlgButtonChecked(hwndDlg,IDC_CHECKALWAYS))
						DBWriteContactSettingByte(NULL,"MSNAccountSetup","showAlways",1);
					else
						DBWriteContactSettingByte(NULL,"MSNAccountSetup","showAlways",0);
					
					EndDialog(hwndDlg, IDOK);
				}
				break;
			
			case IDDISABLE:
				{
					MessageBox(NULL,Translate("MSN Protocol and MSN Account Setup have been disabled.\nYou must restart Miranda IM.\n\nTo enable MSN plugin again, you should go to:\n\n> Miranda IM Options\n>> Plugins\n>>> Check to enable MSN Protocol\n>>> Check to enable MSN Account Setup"),"MSN Account Setup",MB_OK);
					DBWriteContactSettingByte(NULL,"PluginDisable","msn.dll",1);
					DBWriteContactSettingByte(NULL,"PluginDisable","msn_accountsetup.dll",1);
					EndDialog(hwndDlg, IDDISABLE);
				}
				break;

			case IDCANCEL:
                {
					EndDialog(hwndDlg, IDCANCEL);
                }
				break;

            }
        }
		break;

    }

    return FALSE;

}

void dialog() {
	DialogBox(hInst, MAKEINTRESOURCE(IDD_MSNACCOUNTSETUP), NULL, msn_FirstRunDlgProc);
}

void msn_FirstRunCheck()
{
	DBVARIANT dbvE, dbvP;

	//MessageBox(NULL,"Debug","MSN Account Setup",MB_OK);

	//si no está disable y tiene ingresado los datos...
	if(!DBGetContactSettingByte(NULL,"PluginDisable","msn.dll",0)) {
		
		if(DBGetContactSettingByte(NULL,"MSNAccountSetup","showAlways",0))
			dialog();
		else if(DBGetContactSetting(NULL, "MSN", "e-mail", &dbvE) || DBGetContactSetting(NULL, "MSN", "Password", &dbvP))
			dialog();
		else if(!strcmp("", dbvE.pszVal) || !strcmp("", dbvP.pszVal))
			dialog();

	}

	DBFreeVariant(&dbvE); // let Miranda free the memory! 
	DBFreeVariant(&dbvP);
    
	return;
}

int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink=link;

	msn_FirstRunCheck();

	return 0;
}

int __declspec(dllexport) Unload(void)
{
	if(!DBGetContactSettingByte(NULL,"MSNAccountSetup","rememberEmail",1))
		DBWriteContactSettingString(NULL,"MSN","e-mail","");
	if(!DBGetContactSettingByte(NULL,"MSNAccountSetup","rememberPass",1))
		DBWriteContactSettingString(NULL,"MSN","Password","");
	return 0;
}