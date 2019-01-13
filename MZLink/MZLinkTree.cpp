
#include "stdafx.h"
#include "resource.h"

#include "MZLinkTree.h"


BOOL CMZLinkTree::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}


LRESULT CMZLinkTree::OnDropFiles(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// TODO: Add your message handler code here and/or call default

	return 0;
}
