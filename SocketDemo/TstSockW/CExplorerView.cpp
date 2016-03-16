
#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <stdio.h>
#include <io.h>

#include "CExplorerView.h"
#include "rsrc.h"

extern HINSTANCE g_hInstance;

CExplorerView::CExplorerView(HWND hWnd)
{
	hCursor = LoadCursor(g_hInstance,MAKEINTRESOURCE(IDC_CURSOR2));

	hHalftoneBrush = 0;
}

CExplorerView::~CExplorerView()
{
	if (hHalftoneBrush)
		DeleteObject(hHalftoneBrush);
}

// set text in status bar

void CExplorerView::SetStatusText(int iPart, char * pText)
{
	SendMessage( m_hWndStatusBar, SB_SETTEXT, iPart, (LPARAM)pText);
	return;
}

// user resizes TreeView/ListView

void CExplorerView::DrawResizeLine(HWND hwnd, int xPos)
{
        
	HBRUSH hBrushOld;
	HDC hdc; 
	RECT rc;
	
	if (!hHalftoneBrush) {
		int i;
		HBITMAP hBitmap;
		WORD pattern[8];

		for (i=0;i<8;i++) 
			pattern[i] = 0x5555 << (i & 1);
										// Create a pattern halftone brush
		hBitmap = CreateBitmap(8,8,1,1,pattern);
		hHalftoneBrush = CreatePatternBrush(hBitmap);
		DeleteObject(hBitmap);				// bitmap no longer needed
	}
	GetClientRect(m_hWndTreeView, &rc);

	hdc = GetDC(hwnd);
	hBrushOld = (HBRUSH)SelectObject(hdc, hHalftoneBrush); 
	PatBlt(hdc,xPos,iToolbarHeight + 2, 4, rc.bottom, PATINVERT);

	SelectObject(hdc, hBrushOld);		// clean up 
	ReleaseDC(hwnd, hdc);

	iOldResizeLine = xPos;
}

// adjust child windows if size of main window has changed (WM_SIZE)

int CExplorerView::ResizeClients(HWND hWnd)
{
	RECT rect;
	RECT rectTV;
	RECT rectSB;
	RECT rectTB;
	RECT rectEdit;
	RECT rectPrompt;
	HDWP hdwp;

	GetClientRect(hWnd,&rect);
	
	GetWindowRect( m_hWndToolbar,&rectTB);
	rectTB.bottom = rectTB.bottom - rectTB.top;
	
	GetWindowRect( m_hWndStatusBar,&rectSB);
	rectSB.bottom = rectSB.bottom - rectSB.top;

	GetWindowRect( m_hWndPrompt,&rectPrompt);
	rectPrompt.bottom = rectPrompt.bottom - rectPrompt.top;
	rectPrompt.right = rectPrompt.right - rectPrompt.left;

	GetWindowRect( m_hWndEdit,&rectEdit);
	rectEdit.bottom = rectPrompt.bottom;
	rectEdit.right = rect.right - rectPrompt.right;

	GetWindowRect( m_hWndTreeView,&rectTV);
	rectTV.right = rectTV.right - rectTV.left;

	if (rectTV.right == 0) { 
		int aWidths[4];
		rectTV.right = MulDiv(rect.right,1,4) - 2;
		aWidths[0] = rect.right / 5;
		aWidths[1] = rect.right / 5 * 3;
		aWidths[2] = -1;
		SendMessage ( m_hWndStatusBar, SB_SETPARTS, 3, (LPARAM)aWidths);
	}

	iToolbarHeight = rectTB.bottom;

	hdwp = BeginDeferWindowPos(6);

// set the status bar
	
	DeferWindowPos(hdwp, m_hWndStatusBar, NULL,
				0,rect.bottom - rectSB.bottom,
				rect.right,rectSB.bottom,
				SWP_NOZORDER | SWP_NOACTIVATE);

// set the static & Edit control
	
	DeferWindowPos(hdwp, m_hWndPrompt, NULL,
				0,
				rect.bottom - rectSB.bottom - rectPrompt.bottom,
				rectPrompt.right,
				rectPrompt.bottom,
				SWP_NOZORDER | SWP_NOACTIVATE);
	DeferWindowPos(hdwp, m_hWndEdit, NULL,
				rectPrompt.right,
				rect.bottom - rectSB.bottom - rectEdit.bottom,
				rectEdit.right,
				rectEdit.bottom,
				SWP_NOZORDER | SWP_NOACTIVATE);


	// set the toolbar control	
	
	DeferWindowPos(hdwp, m_hWndToolbar, NULL,
				0,0,rect.right,rectTB.bottom,
				SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

// set the tree view control	
	
	DeferWindowPos(hdwp, m_hWndTreeView, NULL,
				0, rectTB.bottom,	rectTV.right,
				rect.bottom - rectSB.bottom - rectPrompt.bottom - rectTB.bottom,
				SWP_NOZORDER | SWP_NOACTIVATE);

// set the list view control	

	DeferWindowPos(hdwp, m_hWndListView, NULL, 
				rectTV.right + 4, rectTB.bottom,
				rect.right - rectTV.right - 4,
				rect.bottom - rectSB.bottom - rectPrompt.bottom - rectTB.bottom,
				SWP_NOZORDER | SWP_NOACTIVATE);

	return EndDeferWindowPos(hdwp);
}

// adjust treeview & listview only

int CExplorerView::AdjustTreeAndList(HWND hWnd,int iWidth)
{
	RECT rect;
	RECT rectTV;
	HDWP hdwp;

	GetClientRect(hWnd,&rect);
	if (iWidth > 0 && iWidth < rect.right) {
		GetWindowRect(m_hWndTreeView,&rectTV);
		rectTV.right = iWidth - rect.left;
		hdwp = BeginDeferWindowPos(2);
		DeferWindowPos(hdwp, m_hWndTreeView, NULL,
				0,0,
				rectTV.right,
				rectTV.bottom - rectTV.top,
				SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
		DeferWindowPos(hdwp,  m_hWndListView, NULL,
				rectTV.right + 4, iToolbarHeight,
				rect.right - rectTV.right - 4,
				rectTV.bottom - rectTV.top,
				SWP_NOZORDER | SWP_NOACTIVATE);
		EndDeferWindowPos(hdwp);
	}
	return 1;
}


// WM_PAINT main window
// the only painting required from the main window is the splitter bar

void CExplorerView::DoPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hDC;
	RECT rect;
	HBRUSH hBrush;

	hDC = BeginPaint(hWnd,&ps);
	hBrush = CreateSolidBrush(GetSysColor(COLOR_ACTIVEBORDER));
	GetWindowRect(m_hWndTreeView,&rect);
	rect.left = rect.right - rect.left;
	rect.bottom = rect.bottom - rect.top + iToolbarHeight; 
	rect.right = rect.left + 4;
	rect.top = iToolbarHeight;
	FillRect(hDC,&rect,hBrush);
	DeleteObject(hBrush);
	EndPaint(hWnd,&ps);

	return;
}

LRESULT CExplorerView::OnNotify(HWND hWnd,WPARAM wParam,LPNMHDR pNMHdr)
{
	LPTOOLTIPTEXT pTTT;
	LRESULT rc = 0;

	switch (pNMHdr->code) {
	case TTN_NEEDTEXT:	// we set hInst and resource id (in lpszText)
						// help string ID is command ID!
		pTTT = (LPTOOLTIPTEXT)pNMHdr;
		pTTT->hinst = GetWindowInstance(hWnd);
		pTTT->lpszText = (LPSTR)pTTT->hdr.idFrom;
		break;
	}
	return rc;
}


LRESULT CExplorerView::OnMessage(HWND hWnd,UINT message,WPARAM wparam,LPARAM lparam)
{
	static HWND hWndActive = NULL;
	LRESULT rc = 0;
	RECT rect;
	char szStr[80];

	switch (message) {
	case WM_SIZE:
		if (wparam != SIZE_MINIMIZED)
			ResizeClients(hWnd);
		break;
	case WM_LBUTTONDOWN:
		GetWindowRect(m_hWndTreeView,&rect);
		if (HIWORD(lparam) < (rect.bottom - rect.top + iToolbarHeight)) {
			RECT rect;
			RECT rect2;
			GetWindowRect(m_hWndTreeView, &rect);
			GetWindowRect(m_hWndListView, &rect2);
			UnionRect(&rect, &rect, &rect2);
			ClipCursor(&rect);
			SetCapture(hWnd);
			iOldResizeLine = -1;
			DrawResizeLine(hWnd,LOWORD(lparam));
		}
		break;
	case WM_LBUTTONUP:
		if (GetCapture() == hWnd) {
			ReleaseCapture();
			ClipCursor(NULL);
			DrawResizeLine(hWnd,iOldResizeLine);
			AdjustTreeAndList(hWnd,(int)LOWORD(lparam));
		}
		break;
	case WM_MOUSEMOVE:
		if (GetCursor() != hCursor) {
			GetWindowRect( m_hWndTreeView, &rect);
			if (HIWORD(lparam) < (rect.bottom - rect.top + iToolbarHeight))
				SetCursor(hCursor);
		}

		if (GetCapture() == hWnd) { 	// Draw only if cursor is captured
			DrawResizeLine(hWnd,iOldResizeLine);
			DrawResizeLine(hWnd,LOWORD(lparam));
		}
		break;
	case WM_ENTERMENULOOP:
		SendMessage( m_hWndStatusBar,SB_SIMPLE,1,0);	// simple mode on
		break;
	case WM_EXITMENULOOP:
		SendMessage( m_hWndStatusBar,SB_SIMPLE,0,0);	// simple mode off
		break;
	case WM_MENUSELECT:
		szStr[0] = 0;
		LoadString(GetWindowInstance(hWnd),LOWORD(wparam),szStr,sizeof(szStr));
		SetStatusText( 255, szStr);		
		break;
	case WM_PAINT:
		DoPaint(hWnd);
		break;
	case WM_ACTIVATE:							// save currently active
		if (LOWORD(wparam) == WA_INACTIVE) {	// child
			hWndActive = GetFocus();
		} else
			if (hWndActive)						// and restore it if we
				SetFocus(hWndActive);			// become active again
		break;
	default:
		rc = DefWindowProc(hWnd,message,wparam,lparam);
	}
	return rc;
}
