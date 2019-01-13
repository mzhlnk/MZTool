// ln.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <Shlwapi.h>
#include <iostream>

using namespace std;

#define tcout wcout
#define tcerr wcerr

#pragma comment(lib, "shlwapi.lib")

struct ST_OPT
{
	LPCTSTR lpszSrcFile;
	LPCTSTR	lpszDestFile;
	bool	bHard;
	bool	bSymbol;
	bool	bCopy;
	bool	bOverwrite;
	bool	bForce;
};

bool Parse(ST_OPT& opt, TCHAR* argv[], int argc)
{
	for (int i(0); i < argc; i++)
	{
		PCTSTR pstr = argv[i];
		if (_T('/') == *pstr || _T('-') == *pstr)
		{
			int count = 0; 
			for (; *pstr; count++)
			{
				switch (*++pstr)
				{
				case _T('h'):
				case _T('H'):
					opt.bHard = true;
					break;
				case _T('s'):
				case _T('S'):
					opt.bSymbol = true;
					break;
				case _T('c'):
				case _T('C'):
					opt.bCopy = true;
					break;
				case _T('o'):
				case _T('O'):
					opt.bOverwrite = true;
					break;
				case _T('f'):
				case _T('F'):
					opt.bForce = true;
					break;
				case _T('\0'):
					break;
				default:
					return false;
				}
			}

			continue;
		}
		
		if (!opt.lpszSrcFile)
		{
			opt.lpszSrcFile = pstr;
			continue;
		}

		if (!opt.lpszDestFile)
		{
			opt.lpszDestFile = pstr;
			continue;
		}

		return false;
	}

	return  true;
}

// ln [/h] [/s] [/c] [/o] [/f] <src> [dest]
int _tmain(int argc, _TCHAR* argv[])
{
	TCHAR szSrcFile[MAX_PATH] = {};
	TCHAR szDestFile[MAX_PATH] = {};
	ST_OPT opt = {};
	if (Parse(opt, argv + 1, argc - 1) && opt.lpszSrcFile)
	{
		PTSTR pszFileName(NULL);
		::GetFullPathName(opt.lpszSrcFile, MAX_PATH, szSrcFile, &pszFileName);

		if (!opt.lpszDestFile)
		{
			if (pszFileName)
			{
				_tcscpy_s(szDestFile, pszFileName);
			}
		}
		else
		{
			_tcscpy_s(szDestFile, opt.lpszDestFile);
			//::GetFullPathName(opt.lpszDestFile, MAX_PATH, szDestFile, NULL);
			DWORD dwVal = ::GetFileAttributes(szDestFile);
			if (INVALID_FILE_ATTRIBUTES != dwVal && (FILE_ATTRIBUTE_DIRECTORY & dwVal))
			{
				int len = _tcslen(szDestFile);
				if (len > 0) len--;
				if (szDestFile[len] != _T('\\')) _tcscat_s(szDestFile, _T("\\"));
				_tcscat_s(szDestFile, pszFileName);
			}
		}
	}
	if (!szSrcFile[0] || !szDestFile[0])
	{
		tcout << _T("ln [/h] [/s] [/c] [/o] [/f] <src> [dest]") << endl;
		return -1;
	}

	DWORD dwAttr = ::GetFileAttributes(szSrcFile);
	if (INVALID_FILE_ATTRIBUTES == dwAttr)
	{
		tcout << _T("cannot find: ") << szSrcFile << endl;
		return -2;
	}

	if (!opt.bCopy && !opt.bSymbol)
		opt.bHard = true;

	// 
	{
		DWORD dwVal = ::GetFileAttributes(szDestFile);
		if (INVALID_FILE_ATTRIBUTES != dwVal)
		{
			if ((dwVal & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				BY_HANDLE_FILE_INFORMATION bhfi1 = {}, bhfi2 = {};
				{
					HANDLE hFile = CreateFile(szDestFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if (INVALID_HANDLE_VALUE != hFile)
					{
						if (!GetFileInformationByHandle(hFile, &bhfi1))
						{
							DWORD dwErr = GetLastError();
							tcerr << _T("failed to get file information by handle: (") << dwErr << _T(") -> ") << szDestFile << endl;
						}
						CloseHandle(hFile);
					}
				}
				HANDLE hFile = CreateFile(szSrcFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if (INVALID_HANDLE_VALUE != hFile && bhfi1.nNumberOfLinks >= 1)
				{
					if (!GetFileInformationByHandle(hFile, &bhfi2))
					{
						DWORD dwErr = GetLastError();
						tcerr << _T("failed to get file information by handle: (") << dwErr << _T(") -> ") << szSrcFile << endl;
					}
					else if (bhfi1.dwVolumeSerialNumber == bhfi2.dwVolumeSerialNumber && bhfi1.nFileIndexLow == bhfi2.nFileIndexLow && bhfi1.nFileIndexHigh == bhfi2.nFileIndexHigh)
					{
						CloseHandle(hFile);
						hFile = INVALID_HANDLE_VALUE;
						if (opt.bHard)
						{
							tcout << _T("there is the same file.") << endl;
							return 0;
						}
					}
				}
				if (opt.bForce && FILE_ATTRIBUTE_READONLY & dwVal)
					::SetFileAttributes(szDestFile, dwVal & ~FILE_ATTRIBUTE_READONLY);

				if (opt.bOverwrite)
					DeleteFile(szDestFile);

				if (INVALID_HANDLE_VALUE != hFile)
					CloseHandle(hFile);
			}
		}
	}

	if (opt.bHard)
	{
		if (CreateHardLink(szDestFile, szSrcFile, NULL))
		{
			tcout << _T("success to create hard link: [") << szDestFile << _T("] -->> [") << szSrcFile << _T("]") << endl;
			return 0;
		}
		DWORD dwVal = GetLastError();
		tcout << _T("failed to create hard link: ") << dwVal << endl;
	}
	if (opt.bSymbol)
	{
		if (CreateSymbolicLink(szDestFile, szSrcFile, (dwAttr & FILE_ATTRIBUTE_DIRECTORY) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0))
		{
			tcout << _T("success to create symbolic link: [") << szDestFile << _T("] -->> [") << szSrcFile << _T("]") << endl;
			return 0;
		}
		DWORD dwVal = GetLastError();
		tcout << _T("failed to create symbolic link: ") << dwVal << endl;
	}
	if (opt.bCopy)
	{
		if (CopyFile(szSrcFile, szDestFile, !opt.bOverwrite))
		{
			tcout << _T("success to copy file") << endl;
			return 0;
		}
		DWORD dwVal = GetLastError();
		tcout << _T("failed to copy file: ") << dwVal << endl;
	}

	return -3;
}

