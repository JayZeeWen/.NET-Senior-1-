
#define STRICT

#define _WIN32_WINNT    0x0400  /* Get new material from the headers */
#define _WIN32_WINDOWS  0x040a

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <stdio.h>
#include <io.h>
#include <share.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <process.h>

#include "lists.h"

#include "tstsockw.h"
#include "rsrc.h"

//#define sprintf wsprintf		// max buffer size of wsprintf is 1024, which is insufficient

// functions of class CSockThread

#define SD_BOTH 0x02 /* now in winsock2.h */

#define MAX_PENDING_CONNECTS 2  // The backlog allowed for listen()
#define NO_FLAGS_SET         0  // Used with recv()/send()         
#define MULTICLIENTS         1
#define SENDBUFFSIZE         2048

#ifdef _DEBUG
char CDebug::szStr[4096];
void CDebug::Print(LPSTR pStr,int pVar)
{
	sprintf(szStr,pStr,pVar);
	OutputDebugString(szStr);
	OutputDebugString("\r\n");
}
#endif

int CSockThread::iCount = 0;

CSockThread::CSockThread(int iReqType, int iReqSockType, int iReqPort, char * pHost)
{
	char szStr[128];
	int status;
	WSADATA WSAData;

	DEBUGOUT("CSockThread::CSockThread");
	memset(this,0,sizeof(CSockThread));
	iType = iReqType;
	iSockType = iReqSockType;
	iClients = 0;
	iPort = iReqPort;
	hThreadSend = (UINT)-1;
	hThreadRecv = (UINT)-1;
	hTextList = CreateList();
	fProtokoll = TRUE;
	fConnected = FALSE;
	sock = 0;
	pSendText = 0;
	pszFileRecv = 0;
	pszFileSend = 0;
	hFileRecv = -1;
	hFileSend = -1;
	pChild = 0;
	pParent = 0;
	hTreeItem = NULL;

	if ((iType == STT_CLIENT) || (iSockType == SOCK_DGRAM)) {
		hSemaphor = CreateSemaphore(0,0,1,NULL);	// count = 0 blockiert Thread bei WaitForSingle...
	} else
		hSemaphor = 0;

	if (iType == STT_CLIENT) {
		pszRemoteName = (char *)malloc(strlen(pHost)+1);
		strcpy(pszRemoteName,pHost);
	} else
		pszRemoteName = 0;
			
	if (!iCount) {
		if ((status = WSAStartup(MAKEWORD(1,1), &WSAData)) == 0) {
			sprintf(szStr,"%s %s",WSAData.szDescription, WSAData.szSystemStatus);
			PrintMessage(szStr);
		} else {
			PrintMessage("WSAStartup(1.1) failed");
		}
	}
	iCount++;
	
}

// request accepted by server

int CSockThread::SetChild(CSockThread * pST)
{

	DEBUGOUT("CSockThread::SetChild");

	sock = pST->RemoteSocket;
	memcpy(&local_sin,&pST->local_sin,sizeof(SOCKADDR_IN));
	RemoteSocket = pST->RemoteSocket;
	memcpy(&remote_sin,&pST->remote_sin,sizeof(SOCKADDR_IN));
	fConnected = TRUE;
	pszLocalName = (char *)malloc(strlen(pST->pszLocalName)+1);
	strcpy(pszLocalName,pST->pszLocalName);

	pST->RemoteSocket = 0;
	memset(&pST->remote_sin,0,sizeof(SOCKADDR_IN));

// currently server cannot have file parms
// for "receiving data" it would be nice to have a file
// in which data of all clients would then be written.

	pszFileRecv = pST->pszFileRecv;
	pST->pszFileRecv = 0;
	hFileRecv = pST->hFileRecv;
	pST->hFileRecv = -1;

	pszFileSend = pST->pszFileSend;
	pST->pszFileSend = 0;
	hFileSend = pST->hFileSend;
	pST->hFileSend = -1;
	
	pParent = pST;
	pChild = pST->pChild;
	pST->pChild = this;
	pST->iClients++;
	return 1;
}

HTREEITEM CSockThread::GetTreeItem()
{
	return hTreeItem;
}

CSockThread::~CSockThread()
{
	CSockThread * pST;
	HANDLE tTextList;

	DEBUGOUT("CSockThread::~CSockThread");

	if ((RemoteSocket != sock) && (RemoteSocket != 0)) {
	 	shutdown(RemoteSocket,SD_BOTH);
		closesocket(RemoteSocket);
		RemoteSocket = 0;
	}
	if (pParent) {			// are we a dependant thread?
		pST = pParent;		// then remove node!
		while (pST->pChild != this)
			pST = pST->pChild;
		pST->pChild = pChild;
		pChild = 0;			// so we aren't regarded as "mother process"
	}

							// are we a "mother process"`?
	while (pST = pChild) {
		delete pST;			// destructor will do the rest
	}

	if (sock > 0) {
		if (pParent != 0) {
			pParent->iClients--;
			sock = 0;			
		} else {
			shutdown(sock,SD_BOTH);
			closesocket(sock);
			sock = 0;
		}			
	}

	if (hSemaphor)
		CloseHandle(hSemaphor);
	if (pszLocalName)
		free(pszLocalName);
	if (pszRemoteName)
		free(pszRemoteName);
	if (pSendText)
		free(pSendText);
	if (pszFileRecv)
		free(pszFileRecv);
	if (pszFileSend)
		free(pszFileSend);
	if (hFileRecv != -1)
		_close(hFileRecv);
	if (hFileSend != -1)
		_close(hFileSend);
	iCount--;
	if (!iCount)
		WSACleanup();

	tTextList = hTextList;
	hTextList = 0;
	DestroyList(tTextList);
}

// we aren't in the "UI" thread, so do post (NOT send) messages

int CSockThread::PrintMessage(char * pMsg, int iSize)
{
	SYSTEMTIME lt;
	char szStr[2048];

	DEBUGOUT2("CSockThread::PrintMessage(%.80s)", (int)pMsg);

	if (hTextList) {
		GetLocalTime(&lt);
		sprintf(szStr,"%02u:%02u:%02u\t%08X %.2000s",lt.wHour,lt.wMinute,lt.wSecond,GetCurrentThreadId(),pMsg);
		AddListString(hTextList,szStr);
//		if (memcmp(pMsg,"error",5) == 0)
//			g_pCW->SetStatusText( 1, pMsg); 
		g_pCW->CheckIfListViewUpdate(this);
	}
	return 1;
}

// prepare "Save As" dialog and show it

int CSockThread::OnFileNameRecv(void)
{
	OPENFILENAME ofn;
	char szStr[288];
	char szStr1[260];
	char szStr2[128];
	char szStr3[128];
	int i;
	HWND hWnd = g_hWnd;

	if (pszFileRecv) {
		sprintf(szStr1,"Should the current file\n%s\nbe closed?",pszFileRecv);
		if ((i = MessageBox(hWnd,szStr1,"Warning",MB_OKCANCEL)) == IDOK) {
			_close(hFileRecv);
			hFileRecv = -1;
			free(pszFileRecv);
			pszFileRecv = 0;
		} else
			return 0;
	}

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
		hFileRecv = _sopen(szStr1,
							_O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY,
							_SH_DENYWR,
							_S_IREAD | _S_IWRITE);
		if (hFileRecv == -1) {
			sprintf(szStr,"Error %u occured when trying to open\n%s",errno,szStr1);
			MessageBox(hWnd,szStr,0,MB_OK); 
		} else {
			pszFileRecv = (char *)malloc(strlen(szStr1)+1);
			strcpy(pszFileRecv,szStr1);
		}

	}
	return 1;
}

// server + client: send a file, prepare "Open" dialog and show it

int CSockThread::OnFileNameSend(void)
{
	OPENFILENAME ofn;
	char szStr[288];
	char szStr1[260];
	char szStr2[128];
	char szStr3[128];
	int i;
	HWND hWnd = g_hWnd;

	DEBUGOUT("CSockThread::OnFileNameSend");

	if (pszFileSend) {
		sprintf(szStr1,"Should the currently selected file\n%s\nbe dropped?",pszFileSend);
		if ((i = MessageBox(hWnd,szStr1,"Warning",MB_OKCANCEL)) == IDOK) {
			_close(hFileSend);
			hFileSend = -1;
			free(pszFileSend);
			pszFileSend = 0;
		} else
			return 0;
	}			

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
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
//	ofn.Flags = 0;
	if (GetOpenFileName(&ofn)) {
		DEBUGOUT2("CSockThread::OnFileNameSend(%u)", (int)pszFileSend);
		hFileSend = _sopen(szStr1,
							_O_BINARY | _O_RDONLY,
							_SH_DENYNO);
		if (hFileSend == -1) {
			sprintf(szStr,"Error %u occured when trying to open\n%s",errno,szStr1);
			MessageBox(hWnd,szStr,0,MB_OK); 
		} else {
			pszFileSend = (char *)malloc(strlen(szStr1)+1);
			strcpy(pszFileSend,szStr1);
			ReleaseSemaphore(hSemaphor,1,NULL);
		}
	}
	return 1;
}

char * CSockThread::GetConnStatus(void)
{
	if (!sock)
		return "inactive";

	if (iType == STT_SERVER)
		return "waiting";

	if (fConnected)
		return "connected";
	else
		return "not connected";
}


void CSockThread::TryShutDown(void)
{
	fActive = FALSE;
	CSockThread * pST;

	if ((RemoteSocket != 0) && (RemoteSocket != sock))
		shutdown(RemoteSocket,2);
	if (sock > 0)
		if (iType == STT_SERVER) {
			closesocket(sock);	// no other possibility to terminate server
			sock = 0;
			pST = this;
			while (pST = pST->pChild) { // we're closing all open connections
				pST->TryShutDown();
			}
		} else
			shutdown(sock,2);
	if (hSemaphor)
		ReleaseSemaphore(hSemaphor,1,NULL);

	return;
}

// set Address (+ Port)

BOOL CSockThread::FillAddr(PSOCKADDR_IN psin,BOOL bRemote)
{
	PHOSTENT phe;
	int pse;
	char szStr[260];
	int lerror;
	unsigned long inaddr;
	char * pHost;
	char * pStr;

	psin->sin_family = AF_INET;

//			Retrieve local address.

	if (!bRemote) {
		pse = gethostname(szStr,sizeof(szStr));
		if (!pszLocalName)
			pszLocalName = (char *)malloc(strlen(szStr)+1);
		if (pse == SOCKET_ERROR) {
			sprintf(szStr,"error: gethostname() failed with %d", WSAGetLastError() );
			PrintMessage(szStr);
			return FALSE;
		} else {
			strcpy(pszLocalName,szStr);
			pHost = pszLocalName;
			psin->sin_addr.s_addr = INADDR_ANY;	// jeder port erlaubt bei bind
			sprintf(szStr,"gethostname() ok, returns %s",pszLocalName);
			PrintMessage(szStr);
		}
	} else
		pHost = pszRemoteName;

//			If we are setting up for a listen() call (bConnect = FALSE),
//			fill servent with our address.

	if (INADDR_NONE != (inaddr = inet_addr(pHost))) {
		psin->sin_addr.S_un.S_addr = inaddr;
		phe = gethostbyaddr((char *)&(psin->sin_addr),sizeof(in_addr),PF_INET);
		pStr = "gethostbyaddr";
	} else {
		phe = gethostbyname(pHost);
		pStr = "gethostbyname";
	}
	if (phe == NULL) {
		lerror = WSAGetLastError();
		sprintf(szStr,"error: %s(%s) returns %d",pStr,pHost,lerror);
		PrintMessage(szStr);
		switch (lerror) {
		case WSAHOST_NOT_FOUND:
			PrintMessage("host not found");
			break;
		case WSANO_ADDRESS:
			PrintMessage("host is unknown");
			break;
		}
		return FALSE;
	} else {
		memcpy((char FAR *)&(psin->sin_addr), phe->h_addr,phe->h_length);

		psin->sin_port = htons((short)iPort);

		sprintf(szStr,"%s(%s) ok, returns %u.%u.%u.%u %s",pStr,pHost,
				psin->sin_addr.S_un.S_un_b.s_b1,
				psin->sin_addr.S_un.S_un_b.s_b2,
				psin->sin_addr.S_un.S_un_b.s_b3,
				psin->sin_addr.S_un.S_un_b.s_b4,
				phe->h_name
			);
		PrintMessage(szStr);
	}

	return TRUE;
}



SOCKET CSockThread::GetSocket()
{
	SOCKET sock;
	char szStr[80];
	int lerror;

	sock = socket( AF_INET, iSockType, 0);

	if (sock == INVALID_SOCKET) {
		lerror = WSAGetLastError();
		sprintf(szStr,"error: socket() failed with rc %u", lerror);
		PrintMessage(szStr);
		return INVALID_SOCKET;
	} else {
		sprintf(szStr,"socket() ok");
		PrintMessage(szStr);
	}
	return sock;
}

// usually: client wants a connection
// before send/recv a socket needs bind() or connect()!

BOOL CSockThread::DoConnect(void)
{
	int lerror; 
	char szStr[80];
	int tPort;
	int i;

//			Retrieve the IP address and TCP Port number

	if (!FillAddr( &remote_sin, TRUE))  {
		return FALSE;
	}

//	remote_sin.sin_port = htons((short)iPort);        // Convert to network ordering

	if (connect( sock, (PSOCKADDR) &remote_sin, sizeof( remote_sin)) == SOCKET_ERROR) {
		lerror = WSAGetLastError();
		sprintf(szStr,"error: connect(%s:%u) failed with rc %d",pszRemoteName,iPort,lerror);
		PrintMessage(szStr);
		switch (lerror) {
		case WSAEISCONN:
			PrintMessage("socket is already connected");
			break;
		case WSAECONNREFUSED:
			PrintMessage("connection refused");
			break;
		case WSAETIMEDOUT:
			PrintMessage("time out");
			break;
		}
		return FALSE;
	} else {
		i = sizeof(local_sin);
		getsockname(sock,(sockaddr *)&local_sin,&i);
		RemoteSocket = sock;	// fuer client
		tPort = ntohs((short)local_sin.sin_port);
		sprintf(szStr,"connect(%s:%u) ok, local port is %u",pszRemoteName,iPort,tPort);
		PrintMessage(szStr);
	}
	return TRUE;
}

// DoBind() : to receive data (with recv/recvfrom) the socket needs
// connect() (client, streams) or bind() (server+streams or datagrams)
// to get a connection with Address:Port. The
// Address is the "local Address".
// The combination of "local Address:Port:Protokoll" is "used" then.

BOOL CSockThread::DoBind(void)
{
	int lerror; 
	char szStr[80];

	if (!FillAddr(&local_sin, FALSE)) {	// sets local net address:port in local_sin
		return FALSE;
	}	

// bind() binds a socket to a local address:port

	if (bind( sock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR) {
		lerror = WSAGetLastError();
		sprintf(szStr,"error: bind(%u.%u.%u.%u:%u) failed with rc %d",
			local_sin.sin_addr.S_un.S_un_b.s_b1,
			local_sin.sin_addr.S_un.S_un_b.s_b2,
			local_sin.sin_addr.S_un.S_un_b.s_b3,
			local_sin.sin_addr.S_un.S_un_b.s_b4,
			iPort,
			lerror);
		PrintMessage(szStr);
		switch (lerror) {
		case WSAEADDRINUSE:
			PrintMessage("another app is using this port already (WSAEADDRINUSE)");
			break;
		}
		return FALSE;
	} else {
		sprintf(szStr,"bind(%u.%u.%u.%u:%u) ok",
			local_sin.sin_addr.S_un.S_un_b.s_b1,
			local_sin.sin_addr.S_un.S_un_b.s_b2,
			local_sin.sin_addr.S_un.S_un_b.s_b3,
			local_sin.sin_addr.S_un.S_un_b.s_b4,
			iPort
			);
		PrintMessage(szStr);
	}

	return TRUE;
}

// server: DoListen

int CSockThread::DoListen(void)
{
	char szStr[80];
	int lerror;

	DEBUGOUT("CSockThread::DoListen");

	if (listen( sock, MAX_PENDING_CONNECTS ) < 0) {
		lerror = WSAGetLastError();
		sprintf(szStr,"error: listen(%u) failed with rc %d", MAX_PENDING_CONNECTS,lerror);
		PrintMessage(szStr);
		return 0;
	} else {
		sprintf(szStr,"listen(%u) ok", MAX_PENDING_CONNECTS);
		PrintMessage(szStr);
	}	
	return 1;
}

// server: DoAccept waits for connection requests
// for every successful accept() a client thread is created

int CSockThread::DoAccept(void)
{
	int remote_sin_len;        /* Accept socket address length */
	int status = 0;
	int rc = 1;
	char szRemoteName[256];
	char szStr[256];
	int tPort;
	char * pStr;
	PHOSTENT phe;
	CSockThread * pST;

	DEBUGOUT("CSockThread::DoAccept");

	while (status != SOCKET_ERROR) {

		remote_sin_len = sizeof(remote_sin);
		sprintf(szStr,"waiting for connection requests [accept()]...");
		PrintMessage(szStr);
		RemoteSocket = accept( sock,(struct sockaddr *) &remote_sin,(int *) &remote_sin_len );
		if (RemoteSocket == INVALID_SOCKET) {
			sprintf(szStr,"error: accept() failed with %d", WSAGetLastError());
			PrintMessage(szStr);
			status = SOCKET_ERROR;
			RemoteSocket = 0;
		} else {
			if (phe = gethostbyaddr((char *)&remote_sin.sin_addr,sizeof(in_addr),PF_INET))
				pStr = phe->h_name;
			else
				pStr = "???";
			sprintf(szRemoteName,"%u.%u.%u.%u (%s)",
				remote_sin.sin_addr.S_un.S_un_b.s_b1,
				remote_sin.sin_addr.S_un.S_un_b.s_b2,
				remote_sin.sin_addr.S_un.S_un_b.s_b3,
				remote_sin.sin_addr.S_un.S_un_b.s_b4,
				pStr);

			tPort = ntohs((short)remote_sin.sin_port);
			sprintf(szStr,"accept() ok, client %s, port %u", szRemoteName,tPort);
			PrintMessage(szStr);
		}

		if (status != SOCKET_ERROR) {
			pST = new CSockThread(STT_CLIENT,iSockType,iPort,szRemoteName);
			if (pST) {
				pST->SetChild(this);
				pST->Init();
			}
		}
	} // end while 

	return rc;
}

// DoSend : send some bytes

int CSockThread::DoSend(char * pMsg, int iSize)
{
	int lerror;
	int status = 0;
	char szStr[80];

	DEBUGOUT("CSockThread::DoSend");

	if (iSockType == SOCK_STREAM)
		status = send(RemoteSocket, pMsg, iSize, NO_FLAGS_SET );
	else
		status = sendto(RemoteSocket, pMsg, iSize, NO_FLAGS_SET,
				(SOCKADDR *)&remote_sin,sizeof(remote_sin));
	
	if (status < 0) {
		lerror = WSAGetLastError();
		if (iSockType == SOCK_STREAM)
			sprintf(szStr,"error: send() failed with rc %u",
				lerror);
		else
			sprintf(szStr,"error: sendto(%u.%u.%u.%u) failed with rc %u",
				remote_sin.sin_addr.S_un.S_un_b.s_b1,
				remote_sin.sin_addr.S_un.S_un_b.s_b2,
				remote_sin.sin_addr.S_un.S_un_b.s_b3,
				remote_sin.sin_addr.S_un.S_un_b.s_b4,
				lerror);
		PrintMessage(szStr);
	} else {
		ulBytesSend = ulBytesSend + status;
		if (fProtokoll) {
			if (iSockType == SOCK_STREAM)
				sprintf(szStr,"send(%.40s) ok, status %d",pMsg,status);
			else
				sprintf(szStr,"sendto(%u.%u.%u.%u,%.40s) ok, status %d",
					remote_sin.sin_addr.S_un.S_un_b.s_b1,
					remote_sin.sin_addr.S_un.S_un_b.s_b2,
					remote_sin.sin_addr.S_un.S_un_b.s_b3,
					remote_sin.sin_addr.S_un.S_un_b.s_b4,
					pMsg,
					status);
			PrintMessage(szStr);
		}
	}
	return status;
}

// DoRecv : receive some bytes

int CSockThread::DoRecv(void)
{
	int status = 0;
	int rc = 1;
	int lerror;
	unsigned long ulBytes;
	FD_SET fs;
	char * pBuffer = NULL;
	char * pOutBuf = NULL;
	unsigned long ulBufferSize = 0;
	char szStr[2048];
	char * pText = szStr;
	int i;

	DEBUGOUT("CSockThread::DoRecv");

	if (iSockType == SOCK_STREAM)
		sprintf(szStr,"waiting for messages [recv()] ...");
	else
		sprintf(szStr,"waiting for messages [recvfrom(%u.%u.%u.%u)] ...",
					remote_sin.sin_addr.S_un.S_un_b.s_b1,
					remote_sin.sin_addr.S_un.S_un_b.s_b2,
					remote_sin.sin_addr.S_un.S_un_b.s_b3,
					remote_sin.sin_addr.S_un.S_un_b.s_b4);
	PrintMessage(szStr);
	while (status != SOCKET_ERROR) {
//				we dont want a blocking recv()
//				so we use select() to block until data has arrived
//				and ioctlsocket() to get the number of bytes to receive
		fs.fd_count = 1;
		fs.fd_array[0] = sock;
		select(NULL, &fs, NULL, NULL, NULL);
		if (SOCKET_ERROR == (status = ioctlsocket(sock, FIONREAD, &ulBytes)))
			break;
		if (ulBytes > ulBufferSize) {
			if (pBuffer)
				free(pBuffer);
			if (!(pBuffer = (char *)malloc(ulBytes + 1)))
				break;
			ulBufferSize = ulBytes;
		}
		if (iSockType == SOCK_STREAM)
			status = recv( sock, pBuffer, ulBufferSize, NO_FLAGS_SET );
		else {
			i = sizeof(remote_sin);
			status = recvfrom( sock, pBuffer, ulBufferSize, NO_FLAGS_SET,
					(SOCKADDR *)&remote_sin,&i);
		}
		if (status == SOCKET_ERROR) {
			fConnected = FALSE;
			lerror = WSAGetLastError();
			if (iSockType == SOCK_STREAM)
				sprintf(szStr,"error: recv() failed with %d", lerror );
			else
				sprintf(szStr,"error: recvfrom(%u.%u.%u.%u) failed with %d",
					remote_sin.sin_addr.S_un.S_un_b.s_b1,
					remote_sin.sin_addr.S_un.S_un_b.s_b2,
					remote_sin.sin_addr.S_un.S_un_b.s_b3,
					remote_sin.sin_addr.S_un.S_un_b.s_b4,
					lerror );
			PrintMessage(szStr);
			switch (lerror) {
			case WSAEINTR:
				sprintf(szStr,"means operation canceled (WSAEINTR)");
				PrintMessage(szStr);
				break;					
			case WSAECONNRESET:
				sprintf(szStr,"means connection reset (WSAECONNRESET)");
				PrintMessage(szStr);
				break;					
			}
		} else {
			if (status) {
				ulBytesRecv = ulBytesRecv + status;
//---------------------------------- if we have large messages (> 2000 bytes)
//---------------------------------- we allocate a 64 kB buffer
//---------------------------------- if this is too small as well, we break!!
				if (status > 2000) {
					if (!pOutBuf)
						pOutBuf = (char *)malloc(0x10000);
					if ((!pOutBuf) || (status > 0xFFE0)) {
						PrintMessage("error: message too large or memory error");
						break;
					}
					pText = pOutBuf;
				}
				if (hFileRecv != -1)
					_write(hFileRecv, pBuffer, status);
				if (fProtokoll) {
					*(pBuffer + status) = '\0';  // NULL-terminate the string
					if (hFileRecv == -1)
						i = sprintf(pText,"recv() ok, status %u [%s]", status, pBuffer);
					else
						i = sprintf(pText,"recv() ok, status %u", status);
					PrintMessage(pText, i);
				}
				if ((iSockType == SOCK_DGRAM) && (!RemoteSocket)) {
					RemoteSocket = sock;	// evtl Edit freischalten
					g_pCW->CheckIfTreeViewUpdate(this);
				}
			} else  {
				PrintMessage("*** connection broken ***");
				status = SOCKET_ERROR;
				fConnected = FALSE;
			}
		}
	}
	if (pBuffer)
		free(pBuffer);
	if (pOutBuf)
		free(pOutBuf);

	PrintMessage("*** Thread ended ***");

	hThreadRecv = (UINT)-1;
	ReleaseSemaphore(hSemaphor,1,NULL);

	return rc;
}


void dorecv(void * pVoid)
{
	CSockThread * pST = (CSockThread *)pVoid;
	pST->DoRecv();
	return;
}

// send text in loop until exit

void CSockThread::InvokeSendLoop(void)
{
	int i;
	BOOL fProtokollOld;
	char * pStr;

	DEBUGOUT("CSockThread::InvokeSendLoop");

	while (fActive) {
		if (hThreadRecv == -1)
			hThreadRecv = _beginthread(dorecv,0,(void *)this);
		while (WaitForSingleObject(hSemaphor,INFINITE) != WAIT_FAILED) {
			if ((iType == STT_CLIENT) && (!fConnected))
				return;
			if (!fActive || (hThreadRecv == -1))
				break;
			if (pSendText) {
				i = strlen(pSendText);
				DoSend(pSendText,i);
				free(pSendText);
				pSendText = 0;
			}
			if (hFileSend != -1) {
				if (pStr = (char *)malloc(SENDBUFFSIZE)) {
					PrintMessage("no protocol during file send");
					fProtokollOld = fProtokoll;
					fProtokoll = FALSE;
					while ((i = _read(hFileSend,pStr,SENDBUFFSIZE)) > 0)
						DoSend(pStr,i);
					_close(hFileSend);
					hFileSend = -1;
					free(pszFileSend);
					pszFileSend = 0;
					free(pStr);
					fProtokoll = fProtokollOld;
					PrintMessage("file has been sent");
					if (g_bCloseAfterFileSent)
						TryShutDown();
				}
			}
		}
	}
	return;
}

// handles new server threads

int CSockThread::InitServer()
{
	int fOk;

	DEBUGOUT("CSockThread::InitServer");

	fActive = TRUE;

	hTreeItem = g_pCW->InsertTreeViewItem(this, TRUE);

	sock = GetSocket();
	
	g_pCW->CheckIfTreeViewUpdate(this);

	if (fOk = DoBind())
		if (iSockType == SOCK_STREAM) {
			if (fOk = DoListen())
				fOk = DoAccept();
		} else {
			InvokeSendLoop();
		}


	if (sock > 0) {
		shutdown(sock,SD_BOTH);
		PrintMessage("socket will be closed now");
		closesocket(sock);
		sock = 0;
	}

	fActive = FALSE;

	g_pCW->CheckIfTreeViewUpdate(this);
	
	hThreadSend = (UINT)-1;
	hThreadRecv = (UINT)-1;

	return 1;
}



int CSockThread::InitClient()
{
//	char szStr[80];
//	int i;
//	char * pStr;

	DEBUGOUT("CSockThread::InitClient");

	fActive = TRUE;

	if (!hTreeItem)
		if (pParent)
			hTreeItem = g_pCW->InsertTreeViewItem(this, FALSE);
		else
			hTreeItem = g_pCW->InsertTreeViewItem(this, TRUE);

	if (sock == 0)
		sock = GetSocket();

	if (iSockType == SOCK_STREAM) {
		if (!fConnected)
			fConnected = DoConnect();
	} else {
		fConnected = DoBind();
		RemoteSocket = sock;
		FillAddr(&remote_sin,TRUE);
	}
	
	g_pCW->CheckIfTreeViewUpdate(this);

	if (fConnected)
		InvokeSendLoop();

	RemoteSocket = 0;

	if (sock > 0) {
		shutdown(sock,SD_BOTH);
		PrintMessage("socket will be closed now");
		closesocket(sock);
		sock = 0;
	}
	fActive = FALSE;
	fConnected = FALSE;

	g_pCW->CheckIfTreeViewUpdate(this);

	hThreadSend = (UINT)-1;
	return 0;
}



void init(void * pVoid)
{
	CSockThread * pST = (CSockThread *)pVoid;
	
	switch (pST->iType) {
	case STT_SERVER:
		pST->InitServer();
		break;
	case STT_CLIENT:
		pST->InitClient();
		break;
	}

	pST->PrintMessage("*** Thread ended ***");

	if (g_bBeepAtSocketClose)
		MessageBeep(MB_OK);

	if (g_bDeleteAfterClose) {
//		if (pST->pParent)
			PostMessage(g_hWnd, WM_COMMAND, IDM_KILLINTERNAL, (LPARAM)pST);
	}

	return;
}



int CSockThread::Init(void)
{

	hThreadSend = _beginthread(init,0,(void *)this);

	return 1;
}
