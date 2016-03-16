
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

#define VIRTLIST 0	/* 1 means virtual listview, but doesnt work yet */

// functions of class CTestSocketView

// Constructor of the main view class
// all child windows are created here

CTestSocketView::CTestSocketView(HWND hWnd) : CExplorerView(hWnd)
{
	int i;	
	TBBUTTON button[2];
	HFONT hFont;
	char szStr[80];
	SIZE sSize;
	HDC hdc;
	
	m_hWndTreeView = CreateWindowEx( WS_EX_CLIENTEDGE,
					WC_TREEVIEW,
					NULL,
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
					0,0,0,0,
					hWnd,
					(HMENU)IDC_TREEVIEW,
					g_hInstance,
					NULL);

	m_hWndListView = CreateWindowEx( WS_EX_CLIENTEDGE,
					WC_LISTVIEW,
					NULL,
#if VIRTLIST
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA,
#else
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
#endif
					0,0,0,0,
					hWnd,
					(HMENU)IDC_LISTVIEW,
					g_hInstance,
					NULL);
	ListView_SetExtendedListViewStyle( m_hWndListView, LVS_EX_INFOTIP | LVS_EX_FULLROWSELECT);
		
	button[0].iBitmap = 0;
	button[0].idCommand = IDM_NEW;
	button[0].fsState = TBSTATE_ENABLED;
	button[0].fsStyle = TBSTYLE_BUTTON;
	button[0].dwData = 0;
	button[0].iString = 0;

	button[1].iBitmap = 1;
	button[1].idCommand = IDM_KILL;
	button[1].fsState = 0;
	button[1].fsStyle = TBSTYLE_BUTTON;
	button[1].dwData = 0;
	button[1].iString = 0;

	for (i = 0;i < LVMODE_MAX;i++) {
		iSortColumn[i] = -1;
	}

	m_hWndToolbar = CreateToolbarEx ( hWnd,
					WS_CHILD | WS_VISIBLE | CCS_TOP | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT,
					IDC_TOOLBAR,
					2,	// number of button images
					g_hInstance,
					IDB_BITMAP1,
					button,
					2,	// number of buttons in toolbar
					16,16,	// x und y fuer	buttons
					16,15,	// x und y fuer bitmaps
					sizeof(TBBUTTON));

	m_hWndStatusBar = CreateWindowEx ( 0,
					STATUSCLASSNAME,
					NULL,
					WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | CCS_BOTTOM,
					0,0,0,0,
					hWnd,
					(HMENU)IDC_STATUSBAR,
					g_hInstance,
					NULL);

	strcpy(szStr,"Text to send:");
	if ( m_hWndStatusBar) {
		hdc = GetDC( m_hWndStatusBar);
		GetTextExtentPoint32(hdc,szStr,strlen(szStr),&sSize);
		ReleaseDC( m_hWndStatusBar,hdc);
	}
	m_hWndPrompt = CreateWindowEx ( WS_EX_CLIENTEDGE,
					"static",
					szStr,
					WS_CHILD | WS_VISIBLE | SS_LEFT,
					0,0,sSize.cx + 4,sSize.cy + 4,
					hWnd,
					(HMENU)IDC_PROMPT,
					g_hInstance,
					NULL);
	if ( m_hWndPrompt) {
		hFont = GetWindowFont( m_hWndStatusBar);
		SetWindowFont( m_hWndPrompt,hFont,0);
	}
	m_hWndEdit = CreateWindowEx ( WS_EX_CLIENTEDGE,
					"edit",
					"",
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT,
					0,0,0,0,
					hWnd,
					(HMENU)IDC_EDIT,
					g_hInstance,
					NULL);
	if ( m_hWndEdit) {
		hFont = GetWindowFont( m_hWndStatusBar);
		SetWindowFont( m_hWndEdit,hFont,0);
	}
	iLVMode = -1;
	hThreadList = 0;
	hTrackMenus = LoadMenu(GetWindowInstance(hWnd),MAKEINTRESOURCE(IDR_MENU2));
	m_hActTreeViewItem = 0;

	if ( m_hWndTreeView && m_hWndListView && m_hWndStatusBar) {
		TreeView_SetImageList( m_hWndTreeView, g_himlSmall, TVSIL_STATE);
		SetFocus( m_hWndTreeView);
		fActive = TRUE;
		SetWindowLong(hWnd,0,(LONG)this);
		UpdateTreeView();
		SetListViewCols(LVMODE_OVERVIEW);
	} else
		fActive = FALSE;
}

// destructor of main view

CTestSocketView::~CTestSocketView()
{
	CSockThread * pST;
	int iIndex;

	fActive = FALSE;
	if (hTrackMenus)
		DestroyMenu(hTrackMenus);

	if (hThreadList) {
		iIndex = -1;
		while (GetNextListItem(hThreadList, iIndex, &pST, sizeof(pST))) {
			pST->TryShutDown();
			while (pST->hThreadSend != -1)
				Sleep(0);
			delete pST;
			iIndex++;
		}
		DestroyList(hThreadList);
	}
}


char * GetProtocol(int iSockType)
{
	if (iSockType == SOCK_STREAM)
		return "TCP";
	else
		return "UDP";
}

// create a new server/client

BOOL CTestSocketView::AddThread(int iType, int iSockType, int iPort, char * szHost)
{
	CSockThread * pST;

	if (!hThreadList)
		hThreadList = CreateList();
	if (hThreadList) {
		pST = new CSockThread(iType, iSockType, iPort, szHost);
		AddListItem(hThreadList,&pST,sizeof(pST));
		pST->Init();
		return TRUE;
	}
	return FALSE;
}

// set text of a treeview item

void CTestSocketView::SetTVChildItem(CSockThread * pST,char * pStr)
{
	if (pST->iType == STT_CLIENT)
		if (pST->pParent)
			sprintf(pStr,"%s",pST->pszRemoteName);
		else
			sprintf(pStr,"%s:%u,%s",pST->pszRemoteName,pST->iPort,
				GetProtocol(pST->iSockType));
	else
		sprintf(pStr,"%u,%s",pST->iPort,GetProtocol(pST->iSockType));

	return;
}

// refresh the left panel (the treeview). In fact called just 1 time

int CTestSocketView::UpdateTreeView()
{
	TV_INSERTSTRUCT tvs;
	HTREEITEM hTree;
	HTREEITEM hTreeFirst = 0;
	char szStr[260];
	CSockThread * pST;
	int i;
	int iIndex;

	SendMessage( m_hWndTreeView,WM_SETREDRAW,0,0);
	SendMessage( m_hWndListView,WM_SETREDRAW,0,0);

	EnableWindow( m_hWndEdit,FALSE);

	TreeView_DeleteAllItems( m_hWndTreeView);
	ListView_DeleteAllItems( m_hWndListView);

	tvs.hInsertAfter = TVI_LAST;
	tvs.item.mask = TVIF_TEXT | TVIF_PARAM;		// nur Text und Textmax in TV_ITEM ist gültig
	tvs.item.cchTextMax = 0;


	for (i = 0; i < 2; i++) {
		tvs.item.pszText = szStr;
		tvs.item.lParam = i;
		tvs.hParent = 0;
		if (i == 0)
			strcpy(szStr,"Server");
		else
			strcpy(szStr,"Clients");
		if (hTree = TreeView_InsertItem( m_hWndTreeView, &tvs)) {
			tvs.hParent = hTree;
			if (hThreadList) {
				iIndex = -1;
				while (GetNextListItem(hThreadList, iIndex, &pST, sizeof(pST))) {
					if (pST->iType == i) {
						tvs.item.lParam = (LPARAM)pST;
						SetTVChildItem(pST,szStr);
						TreeView_InsertItem( m_hWndTreeView, &tvs);
					}
					iIndex++;
				}
			}
		}
	}

	SendMessage( m_hWndListView,WM_SETREDRAW,1,0);
	SendMessage( m_hWndTreeView,WM_SETREDRAW,1,0);

	SetStatusText( 0, "ready"); 

	return 1;
}

// insert a new item in the treeview
// this is always a new connection
// this proc is called from SockThread
// thread isnt the main message loop thread!!!

HTREEITEM CTestSocketView::InsertTreeViewItem(CSockThread * pST, BOOL bSelect)
{
	HTREEITEM hTree;
	char szStr[260];
	TV_INSERTSTRUCT tvs;
	TV_ITEM tvi;

	hTree = TreeView_GetRoot( m_hWndTreeView);
	if ((pST->iType == STT_CLIENT) && (pST->pParent == 0))
		hTree = TreeView_GetNextSibling( m_hWndTreeView,hTree);
	if (pST->pParent) {
		tvi.mask = TVIF_PARAM;
		hTree = TreeView_GetChild( m_hWndTreeView,hTree);
		while (hTree) {
			tvi.hItem = hTree;
			TreeView_GetItem( m_hWndTreeView,&tvi);
			if (tvi.lParam == (LPARAM)pST->pParent)
				break;
			hTree = TreeView_GetNextSibling( m_hWndTreeView,hTree);
		}				
	}

	tvs.hParent = hTree;
	tvs.hInsertAfter = TVI_LAST;
	tvs.item.mask = TVIF_TEXT | TVIF_PARAM;
	tvs.item.cchTextMax = 0;
	tvs.item.lParam = (LPARAM)pST;
	tvs.item.pszText = szStr;

	SetTVChildItem(pST, szStr);
	if (hTree = TreeView_InsertItem( m_hWndTreeView, &tvs)) {
		if (bSelect) {
			TreeView_EnsureVisible( m_hWndTreeView, hTree);
			TreeView_SelectItem( m_hWndTreeView, hTree);
		} else {
			TreeView_Expand( m_hWndTreeView, TreeView_GetParent( m_hWndTreeView, hTree),\
				TVE_EXPAND);
		}
	}
	return hTree;
}

// get the current pST

CSockThread * CTestSocketView::GetActSockThread(void)
{
	TV_ITEM tvi;
	LV_ITEM lvi;
	CSockThread * pST = 0;

#if 0
//------------ this method doesnt work perfectly with right mouse clicks
//------------ because iLVMode is determined by "selected" treeview item
//------------ but the "current" treeview item on right clicks may differ
	switch (iLVMode) {
	case LVMODE_OVERVIEW:
		if ((lvi.iItem = ListView_GetNextItem( m_hWndListView,-1,LVNI_SELECTED)) != -1) {
			lvi.mask = LVIF_PARAM;
			ListView_GetItem( m_hWndListView,&lvi);
			pST = (CSockThread *)lvi.lParam;
		}
		break;
	case LVMODE_DETAIL:
		tvi.mask = TVIF_PARAM;
		tvi.hItem = m_hActTreeViewItem;
	   	if (TreeView_GetItem( m_hWndTreeView,&tvi))
			pST = (CSockThread *)tvi.lParam;
		break;
	}
#else
	tvi.mask = TVIF_PARAM;
	tvi.hItem = m_hActTreeViewItem;
   	if (TreeView_GetItem( m_hWndTreeView,&tvi))
		pST = (CSockThread *)tvi.lParam;
	if (!pST) {
		if ((lvi.iItem = ListView_GetNextItem( m_hWndListView,-1,LVNI_SELECTED)) != -1) {
			lvi.mask = LVIF_PARAM;
			ListView_GetItem( m_hWndListView,&lvi);
			pST = (CSockThread *)lvi.lParam;
		}
	}
#endif
	return pST;
}

// delete a thread object
// 1. delete entry in thread list (hThreadList)
// 2. delete TreeView item
// 3. delete ListView Item, if in "OverView" mode

void CTestSocketView::DeleteSockThread(CSockThread * pST)
{
	HTREEITEM hTI;
	LV_ITEM lvi;
	int i;
	int iIndex;
//	SockThread * pST = 0;
	CSockThread * pST2;
	void * pLI;

	DEBUGOUT2("DeleteSockThread(%X)",(int)pST);

	SetStatusText( 1, ""); 

	if (!pST)
		pST = GetActSockThread();

	if (pST == 0) {
		MessageBeep(MB_OK);
		return;
	}

	if (pST->sock > 0) {
		MessageBeep(MB_OK);
		SetStatusText( 1, "To delete entry socket must be closed first"); 
		return;
	}

// 1. delete from list of threads

	if (pST->pParent == 0) {
		iIndex = -1;
		while (pLI = GetNextListItem(hThreadList, iIndex, &pST2,sizeof(pST2))) {
			if (pST == pST2) {
				DeleteListItem(hThreadList,pLI);
				break;
			}
			iIndex++;
		}
	}

// 2. delete from TreeView/ListView

	switch (iLVMode) {
	case LVMODE_OVERVIEW:		
		hTI = pST->GetTreeItem();
		TreeView_DeleteItem( m_hWndTreeView,hTI);

		i = -1;
		while ((i = ListView_GetNextItem( m_hWndListView,i,LVNI_ALL)) != -1) {
			lvi.mask = LVIF_PARAM;
			lvi.iItem = i;
			lvi.iSubItem = 0;
			ListView_GetItem( m_hWndListView, &lvi);
			if (lvi.lParam == (LPARAM)pST) {
				ListView_DeleteItem( m_hWndListView, i);
				break;
			}
		}

		break;
	case LVMODE_DETAIL:	// just TreeView is interesting
		hTI = pST->GetTreeItem();
		TreeView_DeleteItem( m_hWndTreeView,hTI);
		break;
	}

// 3. delete CSockThread
	
	delete pST;

	return;
}



void CTestSocketView::CloseSocket(void)
{
	CSockThread * pST;

	SetStatusText( 1, ""); 

	pST = GetActSockThread();

	if (pST)
		pST->TryShutDown();

	return;
}


void CTestSocketView::Reconnect(int iDummy)
{
	CSockThread * pST;

	SetStatusText( 1, ""); 

	pST = GetActSockThread();

	if (pST)
		pST->Init();

	return;
}

void CTestSocketView::GetFileNameRecv(void)
{
	CSockThread * pST;

	SetStatusText( 1, ""); 

	pST = GetActSockThread();

	if (pST) {
		pST->OnFileNameRecv();
		if (pST->pszFileRecv)
			SetStatusText( 2, pST->pszFileRecv);
		else
			SetStatusText( 2, "");
	}

	return;
}

// File zum senden auswählen

void CTestSocketView::GetFileNameSend(void)
{
	CSockThread * pST;

	SetStatusText( 1, ""); 

	pST = GetActSockThread();

	if (pST)
		pST->OnFileNameSend();

	return;
}

static CColHdr chOverview[] = {
		12, 0, "Local Address",
		 8, 1, "Local Port",
		 8, 0, "Protocol",
		12, 0, "Remote Address",
		 8, 1, "Remote Port",
		12, 0, "Status",
		0};
static CColHdr chDetail[] = {
		 12, 0, "Time",
		 12, 0, "Thread",
		128, 0, "Text",
		0};


// set listview column headers

BOOL CTestSocketView::SetListViewCols(int fMode)
{
	LV_COLUMN lvc;
	int i;
	CColHdr * pCH;

	if (fMode == iLVMode)
		return 1;

	while (ListView_DeleteColumn( m_hWndListView,0));

	switch (fMode) {							
	case LVMODE_OVERVIEW:
		pCH = chOverview; 
		break;
	case LVMODE_DETAIL:
		pCH = chDetail; 
		break;
	}
	for (i = 0; pCH->iSize; pCH++,i++) {
		if (pCH->bNumeric) {
			lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
			lvc.fmt = LVCFMT_RIGHT;
		} else
			lvc.mask = LVCF_TEXT | LVCF_WIDTH;

		lvc.cx = pCH->iSize * 8;
		lvc.pszText = pCH->pszText;
		ListView_InsertColumn( m_hWndListView,i,&lvc);
	}

	iLVMode = fMode;

	return TRUE;
}

#if !VIRTLIST 
// add a listview item in LVMODE_DETAIL mode (protocol line)

int CTestSocketView::InsertListViewItem(CSockThread * pST, char * pStr, int j)
{
	LV_ITEM lvi;
	char szStr[80];
	lvi.mask = LVIF_TEXT;
	memcpy(szStr,pStr,8);
	szStr[8] = 0;
	lvi.pszText = szStr;
	lvi.lParam = 0;
	lvi.iItem = j;
	lvi.iSubItem = 0;
	ListView_InsertItem( m_hWndListView, &lvi);
		
	lvi.mask = LVIF_TEXT;
	lvi.iSubItem++;
	memcpy(szStr,pStr+9,8);
	szStr[8] = 0;
	lvi.pszText = szStr;
	ListView_SetItem( m_hWndListView, &lvi);

	lvi.mask = LVIF_TEXT;
	lvi.iSubItem++;
	lvi.pszText = pStr+9+9;
	ListView_SetItem( m_hWndListView, &lvi);

	return 1;
}

// add a listview item in LVMODE_OVERVIEW

int CTestSocketView::InsertListViewItemEx(CSockThread * pST,int j)
{
	char szStr[80];
	LV_ITEM lvi;
	int tPort;

	sprintf(szStr,"%u.%u.%u.%u",
			pST->local_sin.sin_addr.S_un.S_un_b.s_b1,
			pST->local_sin.sin_addr.S_un.S_un_b.s_b2,
			pST->local_sin.sin_addr.S_un.S_un_b.s_b3,
			pST->local_sin.sin_addr.S_un.S_un_b.s_b4);
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.pszText = szStr;
	lvi.lParam = (LPARAM)pST;
	lvi.iItem = j;
	lvi.iSubItem = 0;
	ListView_InsertItem( m_hWndListView, &lvi);

	lvi.iSubItem++;
	lvi.mask = LVIF_TEXT;
	tPort = ntohs((short)pST->local_sin.sin_port);
	sprintf(szStr,"%u",tPort);
	ListView_SetItem( m_hWndListView, &lvi);

	lvi.iSubItem++;
	lvi.mask = LVIF_TEXT;
	strcpy(szStr,GetProtocol(pST->iSockType));
	ListView_SetItem( m_hWndListView, &lvi);

	lvi.iSubItem++;
	if ((pST->iType == STT_CLIENT) || (pST->iSockType == SOCK_DGRAM)) {
		lvi.mask = LVIF_TEXT;
		sprintf(szStr,"%u.%u.%u.%u",
				pST->remote_sin.sin_addr.S_un.S_un_b.s_b1,
				pST->remote_sin.sin_addr.S_un.S_un_b.s_b2,
				pST->remote_sin.sin_addr.S_un.S_un_b.s_b3,
				pST->remote_sin.sin_addr.S_un.S_un_b.s_b4);
		ListView_SetItem( m_hWndListView, &lvi);
	}

	lvi.iSubItem++;
	if ((pST->iType == STT_CLIENT) || (pST->iSockType == SOCK_DGRAM)) {
		lvi.mask = LVIF_TEXT;
		tPort = ntohs((short)pST->remote_sin.sin_port);
		sprintf(szStr,"%u",tPort);
		ListView_SetItem( m_hWndListView, &lvi);
	}


	lvi.iSubItem++;
	if (pST->iSockType == SOCK_STREAM) {
		lvi.mask = LVIF_TEXT;
		sprintf(szStr,"%s",pST->GetConnStatus());
		ListView_SetItem( m_hWndListView, &lvi);
	}
/*
	lvi.mask = LVIF_TEXT;
	lvi.iSubItem++;
	sprintf(szStr,"%u",pST->remote_sin.sin_port);
	ListView_SetItem( m_hWndListView, &lvi);
*/
	return 1;
}
#endif


int CTestSocketView::UpdateListView(int fMode)
{
	TV_ITEM tvi;
	HTREEITEM hti;
//	LV_ITEM lvi;
	int j;
	HCURSOR hCursorOld;
	LPARAM lParam;
	int iIndex;
	HANDLE hList;
	CSockThread * pST;
	CSockThread * pST2;
	char * pStr;

	SendMessage( m_hWndListView,WM_SETREDRAW,0,0);

	SetStatusText( 0, "busy"); 
	SetStatusText( 2, ""); 

	hCursorOld = SetCursor(LoadCursor(0,IDC_WAIT));

	ListView_DeleteAllItems( m_hWndListView);

	if (fMode == -1)
		fMode = iLVMode;

	SetListViewCols(fMode);

   	hti = TreeView_GetSelection( m_hWndTreeView);
	tvi.hItem = hti;
	tvi.mask = TVIF_PARAM;
   	if (TreeView_GetItem( m_hWndTreeView,&tvi))
		lParam = tvi.lParam;
	else {
		SetCursor(hCursorOld);
		return 0;	
	}


	switch (iLVMode) {
	case LVMODE_OVERVIEW:
		hList = hThreadList;
		if (hList) {
			iIndex = -1;
			j = 0;
			while (GetNextListItem(hList, iIndex, &pST, sizeof(pST))) {
				if (pST->iType == (int)lParam) {
#if !VIRTLIST
					InsertListViewItemEx(pST,j);
#endif
					j++;
					pST2 = pST->pChild;
					while (pST2) {
#if !VIRTLIST
						InsertListViewItemEx(pST2,j);
#endif
						j++;
						pST2 = pST2->pChild;
					}
				}
				iIndex++;
			}
#if VIRTLIST
			ListView_SetItemCount(m_hWndListView, j);
#endif
		}
		break;
	case LVMODE_DETAIL:
		pST = (CSockThread *)lParam;
		j = GetListItemCount(pST->hTextList);
		ListView_SetItemCount(m_hWndListView, j);
#if !VIRTLIST
		iIndex = -1;
		j = 0;
		while (pStr = GetNextListString(pST->hTextList, iIndex)) {
			InsertListViewItem(pST, pStr, j);
			j++;
			iIndex++;
		}
#endif
		if (pST->pszFileRecv)
			SetStatusText( 2, pST->pszFileRecv); 
		break;
	}
	ListView_EnsureVisible( m_hWndListView,j-1,TRUE);
	
	SendMessage( m_hWndListView,WM_SETREDRAW,1,0);
	SetStatusText( 0, "ready");


	SetCursor(hCursorOld);

	return 1;
}



void CTestSocketView::SmartUpdateListView()
{
	TV_ITEM tvi;
	int i,j,iIndex;
	char * pStr;
	CSockThread * pST;
	
	if (iLVMode != LVMODE_DETAIL)
		return;
	j = ListView_GetItemCount( m_hWndListView);
   	tvi.hItem = TreeView_GetSelection( m_hWndTreeView);
	tvi.mask = TVIF_PARAM;
	if (TreeView_GetItem(m_hWndTreeView,&tvi))
		if (pST = (CSockThread *)tvi.lParam) {
			i = GetListItemCount(pST->hTextList);
			if (i > j) {
#if VIRTLIST
				ListView_SetItemCount(m_hWndListView, i);
#else
				iIndex = j - 1;
				while (pStr = GetNextListString(pST->hTextList, iIndex)) {
					InsertListViewItem(pST, pStr, j);
					j++;
					iIndex++;
				}
#endif
				ListView_EnsureVisible( m_hWndListView,j-1,TRUE);
			}
		}
	return;
}



BOOL CTestSocketView::ViewProtocolLine(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	LV_ITEM lvi;
	BOOL rc = FALSE;
	char szStr[2048];

	switch (message) {
	case WM_INITDIALOG:
		lvi.iItem = ListView_GetNextItem( m_hWndListView,-1,LVNI_SELECTED);
		lvi.iSubItem = 2;
		lvi.mask = LVIF_TEXT;
		lvi.cchTextMax = sizeof(szStr);
		lvi.pszText = szStr;
		ListView_GetItem( m_hWndListView,&lvi);	
		SetDlgItemText(hWnd,IDC_EDIT1,szStr);
		SendDlgItemMessage( hWnd, IDC_EDIT1, EM_FMTLINES, TRUE, 0);
		rc = TRUE;
		break;
	case WM_CLOSE:
		EndDialog(hWnd,0);
		break;
	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case IDOK:
		case IDCANCEL:
			PostMessage(hWnd,WM_CLOSE,0,0);
			break;
		}
		break;
	}
	return rc;
}

// viewlineproc

BOOL CALLBACK viewlineproc(HWND hWnd,UINT message,WPARAM wparam,LPARAM lparam)
{
	return g_pCW->ViewProtocolLine(hWnd, message, wparam, lparam);
}


// this function is called by other threads

void CTestSocketView::CheckIfListViewUpdate(CSockThread * pST)
{
	TV_ITEM tvi;

	if (!fActive)
		return;
	if (iLVMode == LVMODE_DETAIL) {
	   	tvi.hItem = TreeView_GetSelection( m_hWndTreeView);
		tvi.mask = TVIF_PARAM;
   		if (TreeView_GetItem( m_hWndTreeView,&tvi))
			if (pST == (CSockThread *)tvi.lParam) {
				PostMessage(g_hWnd, WM_COMMAND, IDM_SMARTUPDATE, 0);
			}
	}
	return;
}

// a thread is started/closed.
// check if it is selected in treeview.

void CTestSocketView::CheckIfTreeViewUpdate(CSockThread * pST)
{
	TV_ITEM tvi;

//------------------ no updates if we are shutting down
	if (!fActive)	
		return;

	if (iLVMode == LVMODE_OVERVIEW)
		PostMessage(g_hWnd, WM_COMMAND, IDM_UPDATE, 0);
	else {
	   	tvi.hItem = TreeView_GetSelection( m_hWndTreeView);
		tvi.mask = TVIF_PARAM;
		if (TreeView_GetItem( m_hWndTreeView,&tvi))
			if (pST == (CSockThread *)tvi.lParam) {
//				EnableWindow( m_hWndEdit,(pST->fActive) && (pST->RemoteSocket > 0));
				SetLocalMenu(pST);
			}
	}
	tvi.stateMask = TVIS_STATEIMAGEMASK;
	tvi.mask = TVIF_STATE;
	tvi.hItem = pST->GetTreeItem();
	if (pST->fActive) {
		tvi.state = INDEXTOSTATEIMAGEMASK(2);
	} else {
		tvi.state = INDEXTOSTATEIMAGEMASK(1);
	}
	TreeView_SetItem( m_hWndTreeView, &tvi );
	return;
}

// property sheet 1 of properties dlg

int CALLBACK PropDialogProc1(HWND hWnd,UINT message, WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pNMHdr = (LPNMHDR)lParam;
	CSockThread * pST;
	int tPort;
	char szStr[80];
	int rc = 0;

	switch (message) {
	case WM_INITDIALOG:
		pST = g_pCW->GetActSockThread();
		sprintf(szStr,"%u.%u.%u.%u",
			pST->local_sin.sin_addr.S_un.S_un_b.s_b1,
			pST->local_sin.sin_addr.S_un.S_un_b.s_b2,
			pST->local_sin.sin_addr.S_un.S_un_b.s_b3,
			pST->local_sin.sin_addr.S_un.S_un_b.s_b4);
		SetDlgItemText(hWnd,IDC_LOCALADDR,szStr);

		tPort = ntohs((short)pST->local_sin.sin_port);
		SetDlgItemInt(hWnd, IDC_LOCALPORT, tPort, FALSE);

		sprintf(szStr,"%s",GetProtocol(pST->iSockType));
		SetDlgItemText(hWnd,IDC_PROTOCOL,szStr);

		SetDlgItemInt(hWnd, IDC_BYTESRECV, pST->ulBytesRecv, FALSE);
		SetDlgItemInt(hWnd, IDC_BYTESSEND, pST->ulBytesSend, FALSE);

		rc = 1;
		break;
	case WM_NOTIFY:
		break;			
	}
	return rc;
}


// show properties

int CTestSocketView::ShowProperties(HWND hWnd)
{
    PROPSHEETPAGE psp[1];
    PROPSHEETHEADER psh;

    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USEICONID | PSP_USETITLE;
    psp[0].hInstance = g_hInstance;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE1);
    psp[0].pszIcon = MAKEINTRESOURCE(IDI_ICON2);
    psp[0].pfnDlgProc = PropDialogProc1;
    psp[0].pszTitle = MAKEINTRESOURCE(IDS_PROPSTR1);
    psp[0].lParam = 0;
    psp[0].pfnCallback = NULL;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_USEICONID | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hWnd;
    psh.hInstance = g_hInstance;
    psh.pszIcon = MAKEINTRESOURCE(IDI_PSICON);
    psh.pszCaption = (LPSTR) "Properties";
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;
    psh.pfnCallback = NULL;

    PropertySheet(&psh);

	return 1;
}

// write content of Listview in a file

int CTestSocketView::SaveListViewContent(PSTR pszFileName)
{
    int i,iNextItem, iColumns, hFile;
    LV_ITEM lvi;
    LV_COLUMN lvc;
    char szStr[260];
    
    hFile = _sopen(pszFileName,_O_WRONLY | _O_CREAT | _O_BINARY | _O_TRUNC, _SH_DENYWR, _S_IREAD | _S_IWRITE);
	if (hFile == -1)
		return 0;

	for (i = 0; i < 32;i++) {
		lvc.mask = LVCF_TEXT;
		lvc.pszText = szStr;
		lvc.cchTextMax = sizeof(szStr);
		if (!ListView_GetColumn( m_hWndListView,i,&lvc))
			break;
		if (i)
			_write(hFile,"\t",1);
		_write(hFile,szStr,strlen(szStr));	
	}		      
	_write(hFile,"\r\n",2);	
	iColumns = i;
	iNextItem = ListView_GetNextItem( m_hWndListView,-1,LVNI_ALL);
	while (iNextItem != -1) {
		lvi.iItem = iNextItem;
		for (i = 0; i < iColumns;i++) {
			lvi.iSubItem = i;
			lvi.mask = LVIF_TEXT;
			lvi.pszText = szStr;
			lvi.cchTextMax = sizeof(szStr);
			if (!ListView_GetItem( m_hWndListView,&lvi))
				break;
			if (i)
				_write(hFile,"\t",1);
			_write(hFile,szStr,strlen(szStr));	
		}
		_write(hFile,"\r\n",2);	
		iNextItem = ListView_GetNextItem( m_hWndListView,iNextItem,LVNI_ALL);
	}
		
	_close(hFile);	
	return 1;
}

// prepare "Save As" dialog and show it

int CTestSocketView::OnSaveAs(HWND hWnd)
{
	OPENFILENAME ofn;
	char szStr1[260];
	char szStr2[128];
	char szStr3[128];

	strcpy(szStr1,"");

	memset(szStr2,0,sizeof(szStr2));
	strcpy(szStr2,"Text files(*.txt)");
	strcpy(szStr2 + strlen(szStr2) + 1,"*.txt");
	memset(szStr3,0,sizeof(szStr3));
	strcpy(szStr3,"All files(*.*)");
	strcpy(szStr3 + strlen(szStr3) + 1,"*.*");

	memset(&ofn,0,sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = szStr3;
	ofn.lpstrCustomFilter = szStr2;
	ofn.nMaxCustFilter = sizeof(szStr2);
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = szStr1;
	ofn.nMaxFile = sizeof(szStr1);
	ofn.Flags = OFN_EXPLORER;
//	ofn.Flags = 0;
	if (GetSaveFileName(&ofn)) {
		SaveListViewContent(szStr1);
	}
	return 1;
}

// sort Listview

int CTestSocketView::CompareListViewItems(LPARAM lParam1, LPARAM lParam2)
{
	char szStr1[128];
	char szStr2[128];       
	LV_ITEM lvi1;
	LV_ITEM lvi2;

	lvi1.iItem = (int)lParam1;
	lvi1.iSubItem = iSortColumn[iLVMode];
	lvi1.mask = LVIF_TEXT;
	lvi1.pszText = szStr1;
	lvi1.cchTextMax = sizeof(szStr1);
	ListView_GetItem( m_hWndListView,&lvi1);

	lvi2.iItem = (int)lParam2;
	lvi2.iSubItem = iSortColumn[iLVMode];
	lvi2.mask = LVIF_TEXT;
	lvi2.pszText = szStr2;
	lvi2.cchTextMax = sizeof(szStr2);
	ListView_GetItem( m_hWndListView,&lvi2);

	if (iSortOrder[iLVMode])
		return strcmp(szStr1,szStr2);
	else
		return 0 - strcmp(szStr1,szStr2);
}

// compareprozedur

int CALLBACK compareproc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	
//	CTestSocketView * pCW = (CTestSocketView *)lParamSort;
	return g_pCW->CompareListViewItems(lParam1,lParam2);	
}

// to prepare sort, add iItem to lParam

int CTestSocketView::SetListViewlParam()
{
	int i,j;
	LV_ITEM lvi;
	
	j = ListView_GetItemCount( m_hWndListView);
	lvi.iSubItem = 0;
	lvi.mask = LVIF_PARAM;
	for (i = 0;i < j;i++) {
		lvi.iItem = i;
		ListView_GetItem( m_hWndListView,&lvi);
		lvi.lParam = i;
		ListView_SetItem( m_hWndListView,&lvi);
	}
	return j;
}

// handle WM_COMMAND

LRESULT CTestSocketView::OnCommand(HWND hWnd,WPARAM wparam,LPARAM lparam)
{
	int i;
	TV_ITEM tvi;
	CSockThread * pST;
	HWND hWndFocus;
	LRESULT rc = 0;

	switch (LOWORD(wparam)) {
	case IDM_NEW:
		DialogBox(GetWindowInstance(hWnd),
				MAKEINTRESOURCE(IDD_NEWTHREAD),
				hWnd,
				newproc);
		break;
	case IDM_UPDATE:
		UpdateListView(-1);
		break;
	case IDM_SMARTUPDATE:
		SmartUpdateListView();
		break;
	case IDM_VIEWLINE:
		DialogBox(GetWindowInstance(hWnd),
				MAKEINTRESOURCE(IDD_VIEWLINE),
				hWnd,
				viewlineproc);
		break;
	case IDM_DELETELINE:
	   	tvi.hItem = TreeView_GetSelection( m_hWndTreeView);
		tvi.mask = TVIF_PARAM;
		if (TreeView_GetItem( m_hWndTreeView,&tvi))
			if (pST = (CSockThread *)tvi.lParam) {
				while (-1 != (i = ListView_GetNextItem( m_hWndListView,-1,LVNI_SELECTED))) {
					ListView_DeleteItem( m_hWndListView, i);
					DeleteListItemByIndex(pST->hTextList, i);
				}
			}
		break;
//	case IDM_CHECKUPDATE:
//		CheckIfListViewUpdate((CSockThread *)lparam);
//		break;
	case IDM_KILL:
		DeleteSockThread(0);
		break;
	case IDM_RECONNECT:
		Reconnect(0);
		break;
	case IDM_KILLINTERNAL:
		DeleteSockThread((CSockThread *)lparam);
		break;
	case IDM_CLOSESOCKET:
		CloseSocket();
		break;
	case IDM_SAVEAS:
		OnSaveAs(hWnd);
		break;
	case IDM_PROPERTIES:
		ShowProperties(hWnd);
		break;
	case IDM_RECVSAVE:
		GetFileNameRecv();
		break;
	case IDM_SENDLOAD:
		GetFileNameSend();
		break;
	case IDM_OPTIONS:
#if 1
		DialogBox(GetWindowInstance(hWnd),
			MAKEINTRESOURCE(IDD_OPTIONS),
			hWnd,
			optionsproc);
#endif
		break;
	case IDM_ABOUT:
		DialogBox(GetWindowInstance(hWnd),
			MAKEINTRESOURCE(IDD_ABOUT),
			hWnd,
			aboutproc);
		break;
	case IDM_HELP:
		char szStr[260];
		GetModuleFileName(0,szStr,sizeof(szStr));
		strcpy(szStr+strlen(szStr)-3,"txt");
		if ((HINSTANCE)32 >= ShellExecute(hWnd,"open",szStr,NULL,".",SW_SHOWNORMAL)) {
			MessageBox(hWnd,"Couldn't start application to show file 'tstsockw.txt'",0,MB_OK);
		}
		break;
	case IDM_EXIT:
		DestroyWindow(hWnd);
		break;
	case IDC_EDIT:		// edit field notifications, currently nothing
		break;
	case IDCANCEL:		// user pressed escape, do nothing
		rc = -1;
		break;
	case IDOK:			// user pressed enter
		hWndFocus = GetFocus();
		if (hWndFocus == m_hWndListView) {
			if (iLVMode == LVMODE_DETAIL)
				PostMessage(hWnd, WM_COMMAND, IDM_VIEWLINE, 0);
			break;
		}
		if (hWndFocus == m_hWndEdit) {
			i = GetWindowText( m_hWndEdit, szStr, sizeof(szStr));
		
			if (!i)	{						// if blank line entered
				strcpy(szStr,"\r\n");		// transmit a CRLF
				i = 2;
			}

			tvi.hItem = TreeView_GetSelection( m_hWndTreeView);
			if (tvi.hItem != 0) {
				tvi.mask = TVIF_PARAM;
				TreeView_GetItem( m_hWndTreeView, &tvi);
				pST = (CSockThread *)tvi.lParam;
				if (!(pST->pSendText)) {
					pST->pSendText = (char *)malloc(i+1);
					strcpy(pST->pSendText,szStr);
					ReleaseSemaphore(pST->hSemaphor,1,NULL);
					SetWindowText( m_hWndEdit, "");
				}	// end if !(pST->pSendText
			}		// end if tvi.hItem != 0
			break;
		}			// end-if (hWndFocus == m_hWndEdit)
		break;		// end-case IDOK
	}				// end switch (LOWORD(wparam))
	return rc;
}

#define LOCALMENUITEMS 6

int CTestSocketView::SetLocalMenu(CSockThread * pST)
{
	HMENU hMenu = GetMenu(g_hWnd);
	HMENU hPopupMenu = GetSubMenu(hMenu,1);
	const int iMenuItems[LOCALMENUITEMS] = 
		{IDM_CLOSESOCKET, IDM_KILL, IDM_RECONNECT, IDM_RECVSAVE,IDM_SENDLOAD,IDM_PROPERTIES};
	int iMenuStates[LOCALMENUITEMS];
	BOOL iEditState = FALSE;
	int i;

	for (i = 0; i < LOCALMENUITEMS; i++)
		iMenuStates[i] = MF_BYCOMMAND | MF_GRAYED;

	if (pST) {
		if (pST->fActive && (pST->RemoteSocket > 0))
			iEditState = TRUE;

//------------------- "receive" file name can be edited even if not active
//------------------- but only with true clients
		if (pST->iType == STT_CLIENT && ((pST->pParent == NULL) || pST->fActive))
			iMenuStates[3] = MF_BYCOMMAND | MF_ENABLED;

//------------------- "close socket" & "send file" require a connected socket
		if (pST->sock > 0) {
			iMenuStates[0] = MF_BYCOMMAND | MF_ENABLED;
			if (pST->iType == STT_CLIENT) {
				iMenuStates[4] = MF_BYCOMMAND | MF_ENABLED;
			}
		} else {
//------------------- "delete" requires a closed socket
			iMenuStates[1] = MF_BYCOMMAND | MF_ENABLED;
//------------------- "reconnect" requires a closed true client connection
			if ((pST->iType == STT_CLIENT) && (pST->pParent == NULL))
				iMenuStates[2] = MF_BYCOMMAND | MF_ENABLED;
		}

		iMenuStates[5] = MF_BYCOMMAND | MF_ENABLED;
	}
	
	EnableWindow( m_hWndEdit,iEditState);
	
	SendMessage( m_hWndToolbar,TB_ENABLEBUTTON,iMenuItems[1],MAKELONG(iMenuStates[1] == MF_BYCOMMAND | MF_ENABLED,0));

	for (i = 0; i < LOCALMENUITEMS; i++)
		EnableMenuItem(hPopupMenu,iMenuItems[i],iMenuStates[i]);

	return 1;
}

// display context menu on right clicks or app key

void CTestSocketView::DisplayContextMenu(HWND hWnd, LPNMHDR pNMHdr)
{
	HTREEITEM hTreeItem;
	HMENU hPopupMenu;
	HMENU hMenu = GetMenu(hWnd);
	CSockThread * pST;
	LV_ITEM lvi;
	TV_ITEM tvi;
	POINT   pt;
	TV_HITTESTINFO ht;
	RECT rect;

	if (pNMHdr->code == NM_RCLICK)
		GetCursorPos(&pt);

	switch (pNMHdr->idFrom) {
	case IDC_TREEVIEW:

		if (pNMHdr->code != NM_RCLICK) {
			GetWindowRect( m_hWndTreeView, &rect);
			hTreeItem = TreeView_GetSelection(m_hWndTreeView);
			pt.x = rect.left;
			pt.y = rect.top;
			TreeView_GetItemRect(m_hWndTreeView, hTreeItem, &rect, TRUE);
			pt.x = pt.x + rect.left + (rect.bottom - rect.top)/2;
			pt.y = pt.y + rect.top  + (rect.bottom - rect.top)/2;
		}

		GetWindowRect( m_hWndTreeView, &rect);
		ht.pt.x = pt.x - rect.left;
		ht.pt.y = pt.y - rect.top;
		if (!(tvi.hItem = TreeView_HitTest( m_hWndTreeView,&ht)))
			break;
		m_hActTreeViewItem = tvi.hItem;
		hTreeItem = TreeView_GetParent( m_hWndTreeView,tvi.hItem);

		if (!hTreeItem)
			hPopupMenu = GetSubMenu(hTrackMenus,0);
		else {
			hPopupMenu = GetSubMenu(hMenu,1);
			tvi.mask = TVIF_PARAM;
			if (TreeView_GetItem( m_hWndTreeView,&tvi)) {
				pST = (CSockThread *)tvi.lParam;
				SetLocalMenu(pST);
			} else
				SetLocalMenu(0);
		}

		TrackPopupMenu(hPopupMenu, 
						TPM_LEFTALIGN | TPM_LEFTBUTTON,
						pt.x,
						pt.y,
						0,
						hWnd,
						NULL);
		break;
	case IDC_LISTVIEW:
		if ((lvi.iItem = ListView_GetNextItem( m_hWndListView,-1,LVNI_SELECTED)) == -1)
			break;
		if (pNMHdr->code != NM_RCLICK) {
			GetWindowRect(m_hWndListView,&rect);
			pt.x = rect.left;
			pt.y = rect.top;
			ListView_GetItemRect(m_hWndListView, lvi.iItem, &rect,LVIR_BOUNDS);
			pt.x = pt.x + rect.left + (rect.bottom - rect.top);
			pt.y = pt.y + rect.bottom;
		}
		switch (iLVMode) { 
		case LVMODE_OVERVIEW:
			lvi.mask = LVIF_PARAM;
			ListView_GetItem( m_hWndListView,&lvi);
			pST = (CSockThread *)lvi.lParam;
			SetLocalMenu(pST);
			if (hPopupMenu = GetSubMenu(hMenu,1)) {
				TrackPopupMenu(hPopupMenu, 
						TPM_LEFTALIGN | TPM_LEFTBUTTON,
						pt.x,
						pt.y,
						0,
						hWnd,
						NULL);
			}
			break;
		case LVMODE_DETAIL:
			if (hPopupMenu = GetSubMenu(hTrackMenus,1)) {
				TrackPopupMenu(hPopupMenu, 
						TPM_LEFTALIGN | TPM_LEFTBUTTON,
						pt.x,
						pt.y,
						0,
						hWnd,
						NULL);
			}
			break;
		}				// end-switch iLVMode
		break;			// end-case IDC_LISTVIEW
	}					// end-switch dwID
}

#if VIRTLIST
// get item info if listview is virtual

int CTestSocketView::FillItem( int iItem, int iSubItem, char * pszText, int cchTextMax)
{
	char * pStr;

	switch (iLVMode) {
	case LVMODE_OVERVIEW:
		break;
	case LVMODE_DETAIL:
		if (pStr = GetNextListString(m_pST->hTextList, iItem - 1)) {
			switch (iSubItem) {
			case 0:
				memcpy(pszText,pStr,8);
				*(pszText+8) = 0;
				break;
			case 1:
				memcpy(pszText,pStr+9,8);
				*(pszText+8) = 0;
				break;
			case 2:
				strncpy(pszText,pStr+9+9,cchTextMax);
				break;
			}
		}
		break;
	}

	return 1;
}
#endif

// handle WM_NOTIFY

LRESULT CTestSocketView::OnNotify(HWND hWnd,WPARAM wParam,LPNMHDR pNMHdr)
{
	CSockThread * pST;
	HTREEITEM hTreeItem;
	LPNM_TREEVIEW pNMTV;
	LPNM_LISTVIEW pNMLV;
	LRESULT rc = 0;

/*
	if (pNMHdr->idFrom == IDC_TREEVIEW) {
		char szStr[80];
		sprintf(szStr,"WM_NOTIFY, %X, %d, %d\r\n",wParam, pNMHdr->code, 0 - (pNMHdr->code - TVN_FIRST));
		OutputDebugString(szStr);
	}
*/
	switch (pNMHdr->code) {
	case TVN_SELCHANGED: // selection changed, update ListView
		pNMTV = (LPNM_TREEVIEW)pNMHdr;
		m_hActTreeViewItem = pNMTV->itemNew.hItem;
		hTreeItem = TreeView_GetParent(pNMTV->hdr.hwndFrom,pNMTV->itemNew.hItem);
		if (!hTreeItem) {
			m_pST = NULL;
			UpdateListView(LVMODE_OVERVIEW);
			EnableWindow( m_hWndEdit,FALSE);
		} else {
			m_pST = (CSockThread *)pNMTV->itemNew.lParam;
			UpdateListView(LVMODE_DETAIL);
		}
		SetLocalMenu(m_pST);
		break;
	case TVN_KEYDOWN:
		switch (((TV_KEYDOWN *)pNMHdr)->wVKey) {
		case VK_DELETE:
			pST = GetActSockThread();
			if (pST && (pST->sock == NULL))
				DeleteSockThread(pST);
			break;
		case VK_APPS:
			DisplayContextMenu(hWnd, pNMHdr);
			break;
		}
		break;

	case LVN_ITEMCHANGING:
		pNMLV = (LPNM_LISTVIEW)pNMHdr;
		if (iLVMode == LVMODE_OVERVIEW) {
			pST = (CSockThread *)pNMLV->lParam;
			if (pNMLV->uNewState & LVIS_SELECTED)
				SetLocalMenu(pST);
			else
				SetLocalMenu(0);
			}
		break;
	case LVN_COLUMNCLICK: // ListView Column Header clicked (# in iSubItem)
		pNMLV = (LPNM_LISTVIEW)pNMHdr;
		if (iSortColumn[iLVMode] == pNMLV->iSubItem)
			iSortOrder[iLVMode] = 1 - iSortOrder[iLVMode];
		else {
			iSortColumn[iLVMode] = pNMLV->iSubItem;
			iSortOrder[iLVMode] = 1;
		}
		SetListViewlParam();
		ListView_SortItems( m_hWndListView,compareproc,(LONG)(LPVOID)this);
		break;
	case LVN_KEYDOWN:
		switch (((LV_KEYDOWN *)pNMHdr)->wVKey) {
		case VK_DELETE:
			if (iLVMode == LVMODE_DETAIL) {
				PostMessage(hWnd, WM_COMMAND, IDM_DELETELINE, 0);
			}
			break;
		case VK_APPS:
			DisplayContextMenu(hWnd, pNMHdr);
			break;
		}
		break;
#if VIRTLIST
	case LVN_GETDISPINFO:
		LV_DISPINFO * pDI = (LV_DISPINFO *)pNMHdr;
		if (pDI->item.mask & LVIF_TEXT)
			FillItem(pDI->item.iItem,\
					pDI->item.iSubItem,\
					pDI->item.pszText,\
					pDI->item.cchTextMax);
		break;
#endif

	case NM_DBLCLK:
		switch (pNMHdr->idFrom) {
		case IDC_LISTVIEW:
			if (iLVMode == LVMODE_DETAIL)
				PostMessage(hWnd, WM_COMMAND, IDM_VIEWLINE, 0);
			break;
		}
		break;
	case NM_RCLICK:			// right mouse button -> context menu
		DisplayContextMenu(hWnd, pNMHdr);
		break;

	default:
		rc = CExplorerView::OnNotify( hWnd, wParam, pNMHdr);
		break;
	}						// end-switch pNMHdr->code
	return rc;
}


// handle WM_xxx from main window


LRESULT CTestSocketView::OnMessage(HWND hWnd,UINT message,WPARAM wparam,LPARAM lparam)
{
	LRESULT rc = 0;

	switch (message) {
	case WM_COMMAND:
		rc = OnCommand(hWnd,wparam,lparam);
		break;
	case WM_NOTIFY:
		rc = OnNotify(hWnd,wparam,(LPNMHDR)lparam);
		break;
	default:
		rc = CExplorerView::OnMessage(hWnd,message,wparam,lparam);
	}
	return rc;
}



