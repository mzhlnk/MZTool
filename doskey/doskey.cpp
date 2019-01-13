// doskey.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <vector>
using namespace std;
#define tcout std::wcout
typedef std::wstring tstring;
typedef std::vector<tstring> vtrstring;

const LPCTSTR CTSTR_REGPATH = _T("Software\\MZTool\\doskey");
const int NAME_MAXLEN = MAX_PATH;
const int VALUE_MAXLEN = 1024;

void GetModuleName(tstring& name, LPCTSTR pszText)
{
	name.erase();
	PCTSTR pstr1 = _tcsrchr(pszText, _T('\\'));
	if (pstr1) pstr1++;
	else pstr1 = pszText;
	PCTSTR pstr2 = _tcsrchr(pstr1, _T('.'));
	if (pstr2)
	{
		name.append(pstr1, pstr2 - pstr1);
	}
	else
	{
		name.append(pstr1);
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	tstring strMod;
	GetModuleName(strMod, argv[0]);

	if (_tcsicmp(strMod.c_str(), _T("doskey")))
	{
		// exec
		TCHAR szValue[VALUE_MAXLEN] = {};
		ATL::CRegKey regKey;
		LSTATUS lstatus = regKey.Open(HKEY_CURRENT_USER, CTSTR_REGPATH, KEY_READ);
		if (ERROR_SUCCESS == lstatus)
		{
			ULONG nChars(ARRAYSIZE(szValue) - 1);
			lstatus = regKey.QueryStringValue(strMod.c_str(), szValue, &nChars);
			if (ERROR_SUCCESS != lstatus)
			{
				//if (ERROR_MORE_DATA == lstatus)
				szValue[0] = _T('\0');
			}
			regKey.Close();
		}
		if (!szValue[0])
		{
			// no set
			return 1;
		}

		tstring strCmd;
		int nNum(0);
		TCHAR** ppArgv = ::CommandLineToArgvW(szValue, &nNum);
		for (int i(0); i < nNum; i++)
		{
			PCTSTR pszArgv(ppArgv[i]);
			if (!_tcslen(pszArgv))
			{
				if (strCmd.empty())
				{
					strCmd.append(_T("\"\""));
				}
				else
				{
					strCmd.append(_T(" \"\""));
				}
				continue;
			}
			if (!_tcscmp(pszArgv, _T("$*")))
			{
				if (!strCmd.empty())
				{
					strCmd.append(_T(" "));
				}
				for (int i(1); i < argc; i++)
				{
					PCTSTR pstr(argv[i]);
					if (!_tcschr(pstr, _T(' ')))
					{
						strCmd.append(pstr);
					}
					else
					{
						strCmd.append(_T("\""));
						strCmd.append(pstr);
						strCmd.append(_T("\""));
					}
				}
				continue;
			}

			tstring strArg;
			for (; pszArgv && *pszArgv;)
			{
				for (PCTSTR pstr = _tcschr(pszArgv, _T('$')); ;)
				{
					if (!pstr)
					{
						strArg.append(pszArgv);
						pszArgv = NULL;
						break;
					}

					TCHAR ch = pstr[1];
					if (_T('*') == ch)
					{
						for (int i(1); i < argc; i++)
						{
							if (i > 1) strArg.append(_T(" "));
							strArg.append(argv[i]);
						}

						strArg.append(pszArgv, pstr - pszArgv);
						pszArgv = pstr += 2;
						break;
					}
					if (_T('0') <= ch && ch <= _T('9'))
					{
						int index = ch - _T('0');
						if (index < argc)
						{
							strArg.append(argv[index]);
						}

						strArg.append(pszArgv, pstr - pszArgv);
						pszArgv = pstr += 2;
						break;
					}

					pstr = _tcschr(pstr + 1, _T('$'));
				}
			}

			if (!strCmd.empty())
			{
				strCmd.append(_T(" "));
			}
			if (strArg.find(_T(' ')) == tstring::npos)
			{
				strCmd.append(strArg);
			}
			else
			{
				strCmd.append(_T("\""));
				strCmd.append(strArg);
				strCmd.append(_T("\""));
			}
		}

		return _tsystem(strCmd.c_str());
	}
	else
	{
		if (argc < 2)
		{
			// show
			ATL::CRegKey regKey;
			LSTATUS lstatus = regKey.Open(HKEY_CURRENT_USER, CTSTR_REGPATH, KEY_READ);
			if (ERROR_SUCCESS == lstatus)
			{
				TCHAR szName[NAME_MAXLEN] = {};
				for (DWORD iIndex(0);; iIndex++)
				{
					DWORD dwLen(ARRAYSIZE(szName));
					lstatus = RegEnumValue(regKey, iIndex, szName, &dwLen, NULL, NULL, NULL, NULL);
					if (ERROR_SUCCESS == lstatus)
					{
						TCHAR szValue[VALUE_MAXLEN] = {};
						ULONG nChars(ARRAYSIZE(szValue));
						lstatus = regKey.QueryStringValue(szName, szValue, &nChars);
						if (ERROR_SUCCESS == lstatus)
						{
							tcout << szName << _T('=') << szValue << endl;
						}
					}
					else if (ERROR_NO_MORE_ITEMS == lstatus)
					{
						break;
					}
				}
				regKey.Close();
			}
		}
		else
		{
			// set
			tstring name, value;
			{
				PCTSTR pstr = _tcschr(argv[1], _T('='));
				if (pstr)
				{
					name.append(argv[1], pstr - argv[1]);
					pstr++;
					if (_tcschr(pstr, _T(' ')))
					{
						value.append(_T("\""));
						value.append(pstr);
						value.append(_T("\""));
					}
					else
					{
						value.append(pstr);
					}
					for (int i(2); i < argc; i++)
					{
						if (!value.empty())
							value.append(_T(" "));
						pstr = argv[i];
						if (!_tcslen(pstr))
						{
							value.append(_T("\"\""));
						}
						else if (_tcschr(pstr, _T(' ')))
						{
							value.append(_T("\""));
							value.append(pstr);
							value.append(_T("\""));
						}
						else
						{
							value.append(pstr);
						}
					}
				}
			}
			if (name.empty())
			{
				tcout << _T("doskey cmd=<Value>") << endl;
				return -1;
			}
			ATL::CRegKey regKey;
			LSTATUS lstatus = regKey.Create(HKEY_CURRENT_USER, CTSTR_REGPATH);
			if (ERROR_SUCCESS != lstatus)
			{
				tcout << _T("Failed to create registry: ") << lstatus << endl;
			}
			else
			{
				if (value.empty())
				{
					lstatus = regKey.DeleteValue(name.c_str());
				}
				else
				{
					lstatus = regKey.SetStringValue(name.c_str(), value.c_str());
					if (ERROR_SUCCESS != lstatus)
					{
						tcout << _T("Failed to set registry: ") << lstatus << endl;
					}
				}
				regKey.Close();
			}
			return lstatus;
		}
	}

	return 0;
}

