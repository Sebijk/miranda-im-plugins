#include <windows.h>
#include <stdio.h>

#include <newpluginapi.h>
#include <m_clist.h>
#include <m_langpack.h>
#include <m_protocols.h>
#include <m_database.h>
#include <m_options.h>
#include <m_netlib.h>

#include <m_options.h>
#include <m_system.h>
#include <m_clui.h>

#include <m_popup.h>

#include <commctrl.h>

#define MODULE "ICQStatusChecker"

#define NAME "ICQ Status Checker"
#define WNAME L"ICQ Status Checker"

#define URL "http://apps.forrest79.net/icq/check_status.php?uin=%d&method=%d"

#define STATUS_ERROR 0
#define STATUS_OFFLINE 1
#define STATUS_ONLINE 2
#define STATUS_AWAY 3
#define STATUS_DND 4
#define STATUS_NA 5
#define STATUS_OCCUPIED 6
#define STATUS_FREEFORCHAT 7
#define STATUS_INVISIBLE 8

#ifndef MIID_ICQSTATUSCHECKER
#define MIID_ICQSTATUSCHECKER	{0x535b1644, 0x1485, 0x13a6, { 0x56, 0xde, 0xd4, 0x09, 0x0e, 0xf4, 0x34, 0x28}}
#endif

typedef struct {
	char url[100];
	int showMessages;
	int method;
} Options;

typedef struct {
	int uin;
    HANDLE hContact;
} StatusParam;

void LoadOptions();

void SaveOptions();

static BOOL CALLBACK DlgProcOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

int OptionsInit(WPARAM wParam, LPARAM lParam);

void showError(char* text);

void showStatus(int uin, int status, HANDLE hContact);

int HttpGet(int uin, int method, HANDLE hContact);

unsigned long __stdcall GetStatusThread(void *param);

static int ICQCheckStatusMenuCommand(WPARAM wParam, LPARAM lParam);

static int BuildMenu(WPARAM wParam, LPARAM lParam);

static int PluginInit(WPARAM wParam, LPARAM lParam);

