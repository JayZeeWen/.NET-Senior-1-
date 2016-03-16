
#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <stdio.h>
#include <io.h>
#include <share.h>
#include <fcntl.h>
#include <sys\stat.h>

#include "lists.h"

#include "tstsockw.h"
#include "rsrc.h"

static int g_iTypeId = IDC_SERVER;
static int g_iProtocolId = IDC_TCPIP;
static int g_iPort = 0;
static char g_szHost[80] = {0};

BOOL NewThread(HWND hWnd,UINT message,WPARAM wparam,LPARAM lparam)
{
	int iPort;
	int iType;
	int iSockType;
	int i;
	int iTypeId;
	int iProtocolId;
	char szHost[80];
	BOOL fError = FALSE;
	BOOL bTranslated;
	BOOL rc = FALSE;

	switch (message) {
	case WM_INITDIALOG:
		g_pCW->SetStatusText( 1, ""); 
		SendDlgItemMessage(hWnd, g_iTypeId, BM_SETCHECK, BST_CHECKED,0);
		SendDlgItemMessage(hWnd, g_iProtocolId, BM_SETCHECK, BST_CHECKED,0);
		SendDlgItemMessage(hWnd,IDC_HOST,EM_SETLIMITTEXT,sizeof(szHost),0);
		if (g_iPort)
			SetDlgItemInt(hWnd,IDC_PORT,g_iPort, FALSE);
		SetDlgItemText(hWnd, IDC_HOST, g_szHost);
		if (g_iTypeId == IDC_SERVER)
			EnableWindow(GetDlgItem(hWnd, IDC_HOST), FALSE);
		rc = TRUE;
		break;
	case WM_CLOSE:
		g_pCW->SetStatusText( 1, ""); 
		EndDialog(hWnd,0);
		break;
	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case IDC_SERVER:
		case IDC_CLIENT:
			if (HIWORD(wparam) == BN_CLICKED)
				EnableWindow(GetDlgItem(hWnd,IDC_HOST),\
					IsDlgButtonChecked(hWnd,IDC_CLIENT));
			break;
		case IDC_TCPIP:
//			EnableWindow(GetDlgItem(hWnd,IDC_SERVER),1);
//			EnableWindow(GetDlgItem(hWnd,IDC_CLIENT),1);
			break;
		case IDC_UDP:
			if (HIWORD(wparam) == BN_CLICKED) {
//				SendDlgItemMessage(hWnd,IDC_CLIENT,BM_CLICK,0,0);
//				EnableWindow(GetDlgItem(hWnd,IDC_SERVER),0);
//				EnableWindow(GetDlgItem(hWnd,IDC_CLIENT),0);
			}
			break;
		case IDOK:
			iPort = GetDlgItemInt(hWnd,IDC_PORT,&bTranslated, FALSE);
			i = GetDlgItemText(hWnd,IDC_HOST,szHost,sizeof(szHost));
			if (SendDlgItemMessage(hWnd,IDC_SERVER,BM_GETCHECK,0,0) == BST_CHECKED) {
				iType = STT_SERVER;
				iTypeId = IDC_SERVER;	
			} else {
				iType = STT_CLIENT;
				iTypeId = IDC_CLIENT;	
			}
			if (SendDlgItemMessage(hWnd,IDC_TCPIP,BM_GETCHECK,0,0) == BST_CHECKED) {
				iSockType = SOCK_STREAM;
				iProtocolId = IDC_TCPIP;	
			} else {
				iSockType = SOCK_DGRAM;
				iProtocolId = IDC_UDP;	
			}
			
			if ((iPort < 1) || (iPort > 65535) || (bTranslated == FALSE)) {
				g_pCW->SetStatusText( 1, "Enter port please (1 - 65535)"); 
				SetFocus(GetDlgItem(hWnd,IDC_PORT));
				fError = TRUE;
			}
			if ((fError == FALSE) && (iType == 1) && (!i)) {// Host zwingend bei client
				g_pCW->SetStatusText( 1, "Enter host please"); 
				SetFocus(GetDlgItem(hWnd,IDC_HOST));
				fError = TRUE;
			}

			if (fError == FALSE) {
				g_pCW->AddThread(iType, iSockType, iPort, szHost);
				SendMessage(hWnd,WM_CLOSE,0,0);
				g_iTypeId = iTypeId;
				g_iProtocolId = iProtocolId;
				g_iPort = iPort;
				strcpy(g_szHost, szHost);
			} else
				MessageBeep(MB_OK);

			break;
		case IDCANCEL:
			SendMessage(hWnd,WM_CLOSE,0,0);
			break;
		}
		break;
	}
	return rc;
}

// newproc creates a new thread

BOOL CALLBACK newproc(HWND hWnd,UINT message,WPARAM wparam,LPARAM lparam)
{
	return NewThread(hWnd, message, wparam, lparam);
}

