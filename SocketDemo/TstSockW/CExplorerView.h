
/////////////////////////////////////////////////////

class CExplorerView {
protected:
	HWND m_hWndTreeView;
	HWND m_hWndListView;
	HWND m_hWndToolbar;
	HWND m_hWndStatusBar;
	HWND m_hWndEdit;
	HWND m_hWndPrompt;
private:
	int iToolbarHeight;
	int iOldResizeLine;
	HBRUSH hHalftoneBrush;
	HCURSOR hCursor;

public:
	void SetStatusText(int iPart, char * pText);

private:
	int ResizeClients(HWND hWnd);
	int AdjustTreeAndList(HWND hWnd, int);
	void DrawResizeLine(HWND, int);
	void DoPaint(HWND hWnd);
protected:
	CExplorerView(HWND hWnd);
	~CExplorerView();
	LRESULT OnNotify(HWND hWnd, WPARAM wparam, LPNMHDR);
	LRESULT OnMessage(HWND hWnd,UINT message,WPARAM wparam,LPARAM lparam);
//	LRESULT OnCommand(HWND hWnd,WPARAM wparam,LPARAM lparam);
};

