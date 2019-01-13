// rpcmd.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;
#define tcout wcout
#define tstring wstring

// rpcmd command [param...] [-rp <key> <value>] [-rpi <key> <value>] [-rpf:filename]


class CRP
{
public:
	CRP(LPCTSTR pszKey, LPCTSTR pszValue, bool noCase = false)
		: noCase(noCase)
		, key(pszKey)
		, value(pszValue)
	{ }
	~CRP(){}

private:
	tstring	key;
	tstring	value;
	bool	noCase;

public:
	tstring Handle(LPCTSTR pszText)
	{
		tstring shText(pszText);
		tstring shKey(key);
		if (noCase)
		{
			transform(shText.begin(), shText.end(), shText.begin(), _totupper);
			transform(shKey.begin(), shKey.end(), shKey.begin(), _totupper);
		}
		tstring strRet;
		for (PCTSTR pstr1 = pszText, pstr2 = shText.c_str();;)
		{
			PCTSTR ptmp = _tcsstr(pstr2, shKey.c_str());
			if (ptmp)
			{
				INT_PTR nVal = ptmp - pstr2;
				strRet.append(pstr1, nVal);
				strRet.append(value);
				nVal += shKey.length();
				pstr1 += nVal;
				pstr2 += nVal;
			}
			else
			{
				strRet.append(pstr1);
				break;
			}
		}
		return strRet;
	}
};

int GetRPFromFile(vector<CRP>& rp, LPCTSTR lpszFileName)
{
	tcout << _T("Not Implement!") << endl;
	return -1;
}

int _tmain(int argc, _TCHAR* argv[])
{
	int nRet = 0;
	vector<PCTSTR> vtrCmd;
	vector<CRP> vtrRP;
	for (int i(1); i < argc; i++)
	{
		PCTSTR pstr = argv[i];
		if (!_tcsicmp(pstr, _T("-rp")))
		{
			i += 2;
			if (i >= argc)
			{
				nRet = -1;
				break;
			}
			vtrRP.push_back(CRP(argv[i - 1], argv[i]));
			continue;
		}
		if (!_tcsicmp(pstr, _T("-rpi")))
		{
			i += 2;
			if (i >= argc)
			{
				nRet = -1;
				break;
			}
			vtrRP.push_back(CRP(argv[i-1], argv[i], true));
			continue;
		}
		if (!_tcsnicmp(pstr, _T("-rpf:"), 5))
		{
			pstr += 5;
			nRet = GetRPFromFile(vtrRP, pstr);
			if (nRet)
			{
				tcout << _T("Failed to get option from file(") << nRet << _T("): ") << pstr << endl;
				break;
			}
			continue;
		}
		vtrCmd.push_back(pstr);
	}

	if (vtrCmd.empty() || vtrRP.empty())
	{
		nRet = -1;
	}
	else if (nRet == 0)
	{
		tstring strCmd;
		for (UINT i(0); i < vtrCmd.size(); i++)
		{
			tstring strTmp(vtrCmd[i]);
			for (UINT i(0); i < vtrRP.size(); i++)
			{
				strTmp = vtrRP[i].Handle(strTmp.c_str());
			}
			//strTmp.erase(strTmp.find_last_not_of(_T(" ")) + 1);
			//strTmp.erase(0, strTmp.find_first_not_of(_T(" ")));
			if (!strCmd.empty())
				strCmd.append(_T(" "));
			if (strTmp.empty() || strTmp.find(_T(' ')) != tstring::npos)
			{
				strCmd.append(_T("\""));
				strCmd.append(strTmp);
				strCmd.append(_T("\""));
			}
			else
			{
				strCmd.append(strTmp);
			}
		}
		tcout << _T("MZ > ") << strCmd << endl;
		nRet = _tsystem(strCmd.c_str());
		return nRet;
	}

	if (-1 == nRet)
	{
		tcout << _T("rpcmd command [param...] [-rp <key> <value>] [-rpi <key> <value>]") << endl;
		//tcout << _T("rpcmd command [param...] [-rp <key> <value>] [-rpi <key> <value>] [-rpf:filename]") << endl;
	}
	return nRet;
}

