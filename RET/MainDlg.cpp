// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"


BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	if( WM_CHAR == pMsg->message && ::GetDlgCtrlID(pMsg->hwnd) == IDC_EDT_RET )
	{
		switch(pMsg->wParam)
		{
		case _T('-'):
			{
				LRESULT lSel = ::SendMessage(pMsg->hwnd, EM_GETSEL, 0, 0);
				if(LOWORD(lSel) != 0)
					return TRUE;
				if(HIWORD(lSel) == 0)
				{
					TCHAR szText[10] = {};
					GetDlgItemText(IDC_EDT_RET, szText, 10);
					if(_T('-') == szText[0])
						return TRUE;
				}
			}
			break;
		case VK_BACK:
		case VK_DELETE:
			break;
		default:
			{
				if(pMsg->wParam < _T('0') || pMsg->wParam > _T('9'))
				{
					return TRUE;
				}
			}
			break;
		}
	}

	return CWindow::IsDialogMessage(pMsg);
}

BOOL CMainDlg::OnIdle()
{
	UIUpdateChildWindows();
	return FALSE;
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);
	{
		TCHAR szPath[MAX_PATH] = {};
		::GetModuleFileName(NULL, szPath, MAX_PATH);
		LPCTSTR pstr = _tcsrchr(szPath, _T('\\'));
		SetWindowText(pstr ? ++pstr : szPath);
	}
	SetDlgItemText(IDC_EDT_PARAM, GetCommandLine());

	m_waitTime = INFINITE;
	DWORD dwRet(0);
	CRegKey regKey;
	LSTATUS lStatus = regKey.Open(HKEY_CURRENT_USER, _T("Software\\MZTool\\RET"), KEY_READ);
	if (ERROR_SUCCESS == lStatus)
	{
		regKey.QueryDWORDValue(_T("WaitTime"), m_waitTime);
		regKey.QueryDWORDValue(_T("DefRetVal"), dwRet);
		regKey.Close();
	}
	SetDlgItemInt(IDC_EDT_RET, dwRet);
	SendDlgItemMessage(IDC_EDT_RET, EM_LIMITTEXT, 11);

	if (m_waitTime != INFINITE)
	{
		SetTimer(2001, 1, NULL);
	}

	return TRUE;
}
DWORD gdwOrg = GetTickCount();
LRESULT CMainDlg::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// TODO: Add your message handler code here and/or call default
	KillTimer(wParam);
	DWORD dwInterval = GetTickCount() - gdwOrg;
	if (dwInterval >= m_waitTime)
	{
		PostMessage(WM_COMMAND, MAKEWPARAM(IDOK, 0));
		SetDlgItemText(IDOK, _T("OK"));
	}
	else
	{
		m_waitTime -= dwInterval;
		gdwOrg += dwInterval;
		TCHAR szText[20];
		_stprintf_s(szText, _T("OK (%d s)"), m_waitTime / 1000);
		SetDlgItemText(IDOK, szText);
		SetTimer(2001, m_waitTime % 1000);
	}
	return 0;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	return 0;
}

LRESULT CMainDlg::OnExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add validation code 
	BOOL bTrans(FALSE);
	int nVal = GetDlgItemInt(IDC_EDT_RET, &bTrans);
	if (bTrans)
	{
		DestroyWindow();
		::PostQuitMessage(nVal);
	}
	else
	{
		TCHAR szText[100] = {};
		_stprintf_s(szText, _T("%d -> %d."), INT_MIN, INT_MAX);
		EDITBALLOONTIP ebt = { sizeof(ebt) };
		ebt.pszText = szText;
		Edit_ShowBalloonTip(GetDlgItem(IDC_EDT_RET), &ebt);
	}
	return 0;
}
