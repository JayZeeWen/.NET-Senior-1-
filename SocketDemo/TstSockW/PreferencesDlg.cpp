
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

// thesa are the functions for the dialog setting user preferences


void SetChildDlgPos(HWND hWnd)
{		
	RECT rect;
	POINT point;
	HWND hWndParent;
	HWND hWndTab;


	hWndParent = GetParent(hWnd);
	hWndTab = GetDlgItem(hWndParent,IDC_TAB1);

	GetWindowRect(hWndTab,&rect);
	point.x = rect.left;
	point.y = rect.top;
	ScreenToClient(hWndParent,&point);

	GetClientRect(hWndTab,&rect);
	TabCtrl_AdjustRect(hWndTab, FALSE, &rect);

	SetWindowPos(hWnd,HWND_TOP,
					rect.left + point.x,
					rect.top + point.y,
					rect.right - rect.left,
					rect.bottom - rect.top,
					SWP_SHOWWINDOW);

	return;
}

BOOL CALLBACK optionpage1dlgproc(HWND hWnd,UINT message,WPARAM wparam,LPARAM lparam)
{
	BOOL rc = FALSE;

	switch (message) {
	case WM_INITDIALOG:
		CheckDlgButton(hWnd, IDC_CLOSEAFTERFILESENT, g_bCloseAfterFileSent);
		CheckDlgButton(hWnd, IDC_BEEPATSOCKETCLOSE, g_bBeepAtSocketClose);
		CheckDlgButton(hWnd, IDC_DELETEAFTERCLOSE, g_bDeleteAfterClose);
		SetChildDlgPos(hWnd);
		rc = TRUE;
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_COMMAND:
		break;
	}
	return rc;
}

// the options dialog displays a dialog box with a tab control (1 tabs)

BOOL CALLBACK optionsproc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lparam)
{
	BOOL rc = FALSE;
	HWND hWndTab;
	HWND hWndCtrlDlg;
	char szStr[80];
	TC_ITEM tci;

	switch (message) {
	case WM_INITDIALOG:
		hWndTab = GetDlgItem(hWnd,IDC_TAB1);

		tci.mask = TCIF_TEXT;
		tci.pszText = szStr;
		tci.cchTextMax = sizeof(szStr);
		strcpy(szStr,"General");
		TabCtrl_InsertItem(hWndTab,0,&tci);
		tci.lParam = (LPARAM)CreateDialogParam( g_hInstance, MAKEINTRESOURCE(IDD_OPTIONPAGE1), hWnd, optionpage1dlgproc,0);
		tci.mask = TCIF_PARAM;
		TabCtrl_SetItem(hWndTab,0,&tci);

#if 0				
		tci.pszText = szStr;
		tci.lParam = IDD_PROPPAGE2;
		tci.iImage = 1;
		tci.cchTextMax = sizeof(szStr);
		strcpy(szStr,"Page 2");
		TabCtrl_InsertItem(hWndTab,1,&tci);
#endif

		break;
	case WM_CLOSE:
		EndDialog(hWnd,0);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			hWndTab = GetDlgItem(hWnd,IDC_TAB1);
			tci.mask = TCIF_PARAM;
			TabCtrl_GetItem(hWndTab,0,&tci);
			hWndCtrlDlg = (HWND)tci.lParam;

			g_bCloseAfterFileSent = IsDlgButtonChecked(hWndCtrlDlg,IDC_CLOSEAFTERFILESENT);
			g_bBeepAtSocketClose = IsDlgButtonChecked(hWndCtrlDlg,IDC_BEEPATSOCKETCLOSE);
			g_bDeleteAfterClose = IsDlgButtonChecked(hWndCtrlDlg,IDC_DELETEAFTERCLOSE);
		}
		SendMessage(hWnd,WM_CLOSE,0,0);
		break;
	}
	return rc;
}
