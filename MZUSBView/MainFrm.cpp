// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MZUSBViewView.h"
#include "MainFrm.h"

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;

	return m_view.PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle()
{
	UIUpdateToolBar();
	return FALSE;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// create command bar window
	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
	// attach menu
	m_CmdBar.AttachMenu(GetMenu());
	// load command bar images
	m_CmdBar.LoadImages(IDR_MAINFRAME);
	// remove old menu
	SetMenu(NULL);

	HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(hWndToolBar, NULL, TRUE);

	CreateSimpleStatusBar();

	m_ImageList.Create(16, 16, ILC_COLOR32 | ILC_MASK, 1, 1);
	HINSTANCE hInst = GetModuleHandle(NULL);
	m_ImageList.AddIcon(LoadIcon(hInst, MAKEINTRESOURCE(IDI_CPT)));
	m_ImageList.AddIcon(LoadIcon(hInst, MAKEINTRESOURCE(IDI_UBUS)));
	m_ImageList.AddIcon(LoadIcon(hInst, MAKEINTRESOURCE(IDI_UDEV)));

	m_hWndClient = m_splitter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

	m_pane.SetPaneContainerExtendedStyle(PANECNT_NOBORDER);
	m_pane.Create(m_splitter, _T("USB Device"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_treeview.Create(m_pane, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE);
	m_pane.SetClient(m_treeview);
	m_treeview.SetImageList(m_ImageList);

	m_view.Create(m_splitter, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE);

	m_splitter.SetSplitterPanes(m_pane, m_view);
	UpdateLayout();
	m_splitter.SetSplitterPosPct(25);

	UIAddToolBar(hWndToolBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);
	UISetCheck(ID_VIEW_TREEPANE, 1);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	InitTree();
	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;
	return 1;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnViewTreePane(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	bool bShow = (m_splitter.GetSinglePaneMode() != SPLIT_PANE_NONE);
	m_splitter.SetSinglePaneMode(bShow ? SPLIT_PANE_NONE : SPLIT_PANE_RIGHT);
	UISetCheck(ID_VIEW_TREEPANE, bShow);

	return 0;
}

LRESULT CMainFrame::OnTreePaneClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/)
{
	if(hWndCtl == m_pane.m_hWnd)
	{
		m_splitter.SetSinglePaneMode(SPLIT_PANE_RIGHT);
		UISetCheck(ID_VIEW_TREEPANE, 0);
	}

	return 0;
}

CString GuidToString(const GUID& guid)
{
	CString strText;
	strText.Format(_T("{%08X-%04X-%04X-%08X}"), guid.Data1, guid.Data2, guid.Data3, *(PDWORD)guid.Data4);
	return strText;
}

#include <initguid.h>
#include <usbiodef.h>
#include <usbioctl.h>
#include <SetupAPI.h>
#pragma comment(lib, "setupapi.lib")
void CMainFrame::InitTree()
{
	m_treeview.DeleteAllItems();
	HDEVINFO hDI = SetupDiGetClassDevs(&GUID_CLASS_USB_DEVICE, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (INVALID_HANDLE_VALUE != hDI)
	{
		HTREEITEM hRoot = m_treeview.InsertItem(_T("Computer"), 0, 0, TVI_ROOT, TVI_LAST);
		m_treeview.Expand(hRoot);
		for (DWORD i(0); ;i++)
		{
			SP_INTERFACE_DEVICE_DATA sidd = { sizeof(sidd) };
			if (SetupDiEnumDeviceInterfaces(hDI, NULL, &GUID_CLASS_USB_DEVICE, i, &sidd))
			{
				SP_DEVINFO_DATA sdd = { sizeof(sdd) };
				if (SetupDiEnumDeviceInfo(hDI, i, &sdd))
				{
					const DWORD CPIDS[] = {
						SPDRP_FRIENDLYNAME,
						SPDRP_DEVICEDESC,
					};
					HTREEITEM hIItem(NULL);
					for (DWORD propid : CPIDS)
					{
						TCHAR szText[512];
						if (SetupDiGetDeviceRegistryProperty(hDI, &sdd, propid, 0, (LPBYTE)szText, sizeof(szText), NULL))
						{
							hIItem = m_treeview.InsertItem(szText, 1, 1, hRoot, TVI_LAST);
							break;
						}
					}
					if (hIItem)
					{
						BYTE buffer[1024];
						PSP_DEVICE_INTERFACE_DETAIL_DATA psdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)buffer;
						psdidd->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
						if (SetupDiGetDeviceInterfaceDetail(hDI, &sidd, psdidd, sizeof(buffer), NULL, NULL))
						{
							HTREEITEM hItem = m_treeview.InsertItem(psdidd->DevicePath, 1, 1, hIItem, TVI_LAST);
							//HANDLE hHC = CreateFile(psdidd->DevicePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
							//if (INVALID_HANDLE_VALUE != hHC)
							//{
							//	DWORD nBR(0);
							//	PUSB_ROOT_HUB_NAME purhn = (PUSB_ROOT_HUB_NAME)buffer;
							//	if (DeviceIoControl(hHC, IOCTL_USB_GET_ROOT_HUB_NAME, 0, 0, purhn, sizeof(buffer), &nBR, NULL))
							//	{
							//		HTREEITEM hItem = m_treeview.InsertItem(purhn->RootHubName, 1, 1, hIItem, TVI_LAST);
							//		//GetHub(purhn->RootHubName, hItem);
							//	}

							//	CloseHandle(hHC);
							//}
						}
					}
				}
			}
			else if (GetLastError() == ERROR_NO_MORE_ITEMS)
			{
				break;
			}
		}
		SetupDiDestroyDeviceInfoList(hDI);
	}
}

void CMainFrame::GetHub(LPCTSTR pszHubName, HTREEITEM hRItem)
{
	HANDLE hHub = CreateFile(pszHubName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_DEVICE, NULL);
	if (INVALID_HANDLE_VALUE == hHub)
	{
		DWORD dwErr = GetLastError();
		AtlTrace(_T("Failed to open hub: %d => %s\n"), dwErr, pszHubName);
	}
	else
	{
		USB_NODE_INFORMATION uni = {};
		DWORD dwVal(0);
		if (DeviceIoControl(hHub, IOCTL_USB_GET_NODE_INFORMATION, NULL, 0, &uni, sizeof(uni), &dwVal, NULL))
		{
			for (UCHAR uPort(1), upMax(uni.u.HubInformation.HubDescriptor.bNumberOfPorts); uPort <= upMax; uPort++)
			{
				BYTE buffer[512] = {};
				PUSB_NODE_CONNECTION_INFORMATION_EX punci = (PUSB_NODE_CONNECTION_INFORMATION_EX)buffer;
				punci->ConnectionIndex = uPort;
				dwVal = 0;
				if (DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX, punci, sizeof(buffer), punci, sizeof(buffer), &dwVal, NULL))
				{
					PUSB_NODE_CONNECTION_NAME pncn = (PUSB_NODE_CONNECTION_NAME)malloc(512);
					pncn->ConnectionIndex = uPort;
					if (DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_NAME, pncn, 512, pncn, 512, &dwVal, NULL))
					{
						m_treeview.InsertItem(pncn->NodeName, hRItem, TVI_LAST);
					}
					free(pncn);
					if (punci->ConnectionStatus == NoDeviceConnected)
					{
						AtlTrace(_T("[%d] No Connection\n"), uPort);
					}
					else
					{
						for (ULONG i(0); i < punci->NumberOfOpenPipes; i++)
						{
							const USB_ENDPOINT_DESCRIPTOR& ued = punci->PipeList[i].EndpointDescriptor;
						}
					}
				}
			}
		}

		CloseHandle(hHub);
	}
}