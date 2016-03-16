
#include "CExplorerView.h"

#define IDC_TREEVIEW	0x800
#define IDC_LISTVIEW	0x801
#define IDC_STATUSBAR	0x802
#define IDC_TOOLBAR		0x803
#define IDC_PROMPT		0x804
#define IDC_EDIT		0x805

#define CLASSNAME "TstSockWClass"
#define APPNAME   "Test Socket"

BOOL CALLBACK newproc(HWND hWnd,UINT message,WPARAM wparam,LPARAM lparam);
BOOL CALLBACK viewlineproc(HWND hWnd,UINT message,WPARAM wparam,LPARAM lparam);
BOOL CALLBACK aboutproc(HWND hWnd,UINT message,WPARAM wparam,LPARAM lparam);
BOOL CALLBACK optionsproc(HWND hWnd,UINT message,WPARAM wparam,LPARAM lparam);

extern HINSTANCE g_hInstance;
extern HWND g_hWnd;
extern HIMAGELIST g_himlSmall;
extern BOOL g_bCloseAfterFileSent;
extern BOOL g_bBeepAtSocketClose;
extern BOOL g_bDeleteAfterClose;

int CALLBACK compareproc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

#define LVMODE_OVERVIEW	0
#define LVMODE_DETAIL	1
#define LVMODE_MAX		1

// application class

class CApplication {
public:
	CApplication(HINSTANCE);
	~CApplication(void);
	HWND InitApp(int);
	int MessageLoop(void);
};

// listview columns class

class CColHdr {
public:
	int iSize;
	int bNumeric;
	char * pszText;
};

// iType enum

#define STT_SERVER	0
#define STT_CLIENT	1

// CSockThread does all the winsock work

class CSockThread {
	static int iCount;
	HTREEITEM hTreeItem;
	int		iClients;
	unsigned long	hThreadRecv;
	char *	pszLocalName;
	char *	pszFileSend;
	int		hFileRecv;
	int		hFileSend;

public:
	int		iType;			// STT_SERVER, STT_CLIENT
	int		iSockType;		// SOCK_STREAM, SOCK_DGRAM
	int		iPort;
	unsigned long	hThreadSend;
	HANDLE	hTextList;
	HANDLE	hSemaphor;
	char *	pszRemoteName;
	BOOL	fProtokoll;
	BOOL	fConnected;
	BOOL	fActive;
	SOCKET	sock;
	char *	pszFileRecv;
	unsigned long ulBytesRecv;
	unsigned long ulBytesSend;

	SOCKET	RemoteSocket;	// fuer Server beim recv (return von accept())
	char *  pSendText;
	SOCKADDR_IN remote_sin;	// Client/Server Remote socket addr bei Connect/Accept - internet style
	SOCKADDR_IN local_sin;	// Server Local socket addr bei Bind - internet style
	CSockThread * pParent;	// if client: ptr to server 
	CSockThread * pChild;	// if server: ptr to accepted connections (clients)

private:
	int		SetChild(CSockThread *);
	SOCKET	GetSocket(void);
	BOOL	DoConnect(void);
	BOOL	DoBind(void);
	int		DoListen();
	int		DoSend(char * pMsg, int iSize);
	int		DoAccept(void);
	BOOL	FillAddr(PSOCKADDR_IN psin,BOOL bRemote);
	void	InvokeSendLoop(void);
public:
	CSockThread(int iType, int iSockType, int iPort, char * pHost);
	~CSockThread();
	int		DoRecv(void);
	int		InitServer(void);
	int		InitClient(void);
	int		PrintMessage(char * pMsg,  int iSize = 0);
	void	TryShutDown(void);
	HTREEITEM GetTreeItem(void);
	int		Init(void);
	int		OnFileNameRecv(void);
	int		OnFileNameSend(void);
	char *	GetConnStatus(void);
};

/////////////////////////////////////////////////////

class CTestSocketView : public CExplorerView {

	HMENU hTrackMenus;
	int iSortColumn[LVMODE_MAX+1];
	int iSortOrder[LVMODE_MAX+1];
	HANDLE hThreadList;
	int iLVMode;
	HTREEITEM m_hActTreeViewItem;	// currently active treeview item
// this is the thread of currently selected item
	CSockThread * m_pST;

public:
	BOOL fActive;

private:
	int UpdateTreeView(void);
	int UpdateListView(int fMode);
	BOOL SetListViewCols(int fMode);
	LRESULT OnNotify(HWND hWnd, WPARAM wparam, LPNMHDR);
	LRESULT OnCommand(HWND hWnd, WPARAM wparam, LPARAM lparam);
	int ShowProperties(HWND hWnd);
	int OnSaveAs(HWND hWnd);
	int SetListViewlParam(void);
	int TranslateProtectFlag(UINT flags,char * szStr,int iMax);
	int SaveListViewContent(char *);
	int InsertListViewItem(CSockThread *, char *, int);
	int InsertListViewItemEx(CSockThread * pST,int j);
	void DeleteSockThread(CSockThread * pST);
	void CloseSocket(void);
	void Reconnect(int);
	void SetTVChildItem(CSockThread *,char *);
	void SmartUpdateListView(void);
	int SetLocalMenu(CSockThread * pST);
	void GetFileNameRecv(void);
	void GetFileNameSend(void);
	void DisplayContextMenu(HWND hWnd, LPNMHDR);
	int FillItem( int iItem, int iSubItem, char * pszText, int cchTextMax);

public:
	CTestSocketView(HWND);
	~CTestSocketView();
	BOOL AddThread(int iType, int iSockType, int iPort, char * szHost);
	LRESULT OnMessage(HWND hWnd,UINT message, WPARAM wparam,LPARAM lparam);
	int CompareListViewItems(LPARAM lParam1, LPARAM lParam2);
	void CheckIfListViewUpdate(CSockThread *);
	void CheckIfTreeViewUpdate(CSockThread *);
	HTREEITEM InsertTreeViewItem(CSockThread *, BOOL bSelect);
	CSockThread * GetActSockThread(void);
	BOOL ViewProtocolLine(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam);

};

extern CTestSocketView * g_pCW;

#ifdef _DEBUG
#define DEBUGOUT(x) CDebug::Print(x,0)
#define DEBUGOUT2(x,y) CDebug::Print(x,y)
class CDebug {
public:
	static void Print(LPSTR,int);
	static char szStr[4096];
};
#else
#define DEBUGOUT(x) 
#define DEBUGOUT2(x,y)
#endif
