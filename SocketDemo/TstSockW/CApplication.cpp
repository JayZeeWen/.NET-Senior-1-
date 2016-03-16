
#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <stdio.h>

#include "tstsockw.h"
#include "rsrc.h"

// global vars

CTestSocketView * g_pCW = NULL;
HINSTANCE g_hInstance = 0;
HWND g_hWnd = 0;
BOOL g_bCloseAfterFileSent = FALSE;
BOOL g_bBeepAtSocketClose = FALSE;
BOOL g_bDeleteAfterClose = FALSE;
HIMAGELIST g_himlSmall = NULL;


// the about dialog displays a static dialog box


BOOL CALLBACK aboutproc(HWND hWnd,UINT message,WPARAM wparam,LPARAM lparam)
{
	BOOL rc = FALSE;
	switch (message) {
	case WM_INITDIALOG:
		rc = TRUE;
		break;
	case WM_CLOSE:
		EndDialog(hWnd,0);
		break;
	case WM_COMMAND:
		SendMessage(hWnd,WM_CLOSE,0,0);
		break;
	}
	return rc;
}


// main window proc


LRESULT CALLBACK wndproc(HWND hWnd,UINT message,WPARAM wparam,LPARAM lparam)
{
	LRESULT rc = 0;

	switch (message) {
	case WM_CREATE: // create 3 client windows (treeview, listview, statusbar)

		g_pCW = new CTestSocketView(hWnd);
		if ((!g_pCW) || (!g_pCW->fActive))
			rc = -1;
		break;
	case WM_DESTROY:
		if (g_pCW) {
			delete g_pCW;
			g_pCW = 0;
		}
		PostQuitMessage(0);
		break;
	default:
		if (g_pCW)
			rc = g_pCW->OnMessage(hWnd,message,wparam,lparam);
		else
			rc = DefWindowProc(hWnd,message,wparam,lparam);
		break;
	}
	return rc;
}


HWND CApplication::InitApp(int nShowCmd)
{
	WNDCLASS wc;

	memset(&wc,0,sizeof(wc));
	wc.style = 0;
//	wc.cbWndExtra = sizeof(CTestSocketView *);
	wc.lpfnWndProc = wndproc;
	wc.hInstance = g_hInstance;
	wc.hIcon = LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_ICON1));
//	wc.hCursor = LoadCursor(NULL,IDC_SIZEWE);
//	wc.hCursor = LoadCursor(g_hInstance,MAKEINTRESOURCE(IDC_CURSOR2));
	wc.hCursor = LoadCursor(NULL,IDC_ARROW);
//	wc.hbrBackground = (HBRUSH)(COLOR_ACTIVEBORDER + 1);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wc.lpszClassName = CLASSNAME;

	if (!RegisterClass(&wc))
		return 0;

	g_hWnd = CreateWindowEx(0,
					CLASSNAME,
					APPNAME,
					WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					(HWND)NULL,
					(HMENU)NULL,
					(HINSTANCE)g_hInstance,
					0);
	if (g_hWnd)
		ShowWindow(g_hWnd, SW_SHOWNORMAL);

	return g_hWnd;
}

int CApplication::MessageLoop()
{
	MSG msg;
	HACCEL hAccel;

	hAccel = LoadAccelerators(g_hInstance,MAKEINTRESOURCE(IDR_ACCELERATOR1));

	__try {
	
	while (GetMessage(&msg, 0, 0, 0)) {
		if (IsDialogMessage(g_hWnd, &msg))
			continue;
		if (!TranslateAccelerator(g_hWnd, hAccel, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	} __except (1) {
		MessageBox(g_hWnd,"A fatal error has occurred in this application.\nIt will be terminated now",0,MB_OK);
	}
	
	return 1;
}

CApplication::CApplication(HINSTANCE hInst)
{
	INITCOMMONCONTROLSEX icce;

	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_BAR_CLASSES;
	InitCommonControlsEx(&icce);

	g_himlSmall = ImageList_LoadImage(g_hInstance, MAKEINTRESOURCE(IDB_BITMAP3),\
		13, 3, 0xC0C0C0, IMAGE_BITMAP, LR_DEFAULTCOLOR);

	return;
}

CApplication::~CApplication()
{
	if (g_himlSmall) {
		ImageList_Destroy(g_himlSmall);
	}
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpszCmdLine, int nShowCmd)
{					 
	CApplication * pApp;

	g_hInstance = hInst;

	if (!(pApp = new CApplication(hInst)))
		return 1;

	if (!(pApp->InitApp(nShowCmd)))
		return 1;

	pApp->MessageLoop();	

	delete pApp;
	
	return 0;
}

