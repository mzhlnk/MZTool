// rspic.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "MIJL.h"
#include <iostream>

using namespace std;

#define tcout std::wcout

DWORD SaveFile(LPCTSTR lpszFile, LPVOID pData, int nLen);

// rspic
//			SIZE	/Max:<max size> SrcPic DestPic
//			JQ		/JQ:<1-100> SrcPic DestPic

int _tmain(int argc, _TCHAR* argv[])
{
	int nRet = -1;

	if (argc == 5)
	{
		if (!_tcsicmp(argv[1], _T("SIZE")))
		{
			int jq = 0;
			int size = 0;

			LPCTSTR pstr = argv[2];
			switch (*pstr)
			{
			case _T('/'):
			case _T('-'):
				pstr++;
				break;
			}
			if (!_tcsnicmp(pstr, _T("MAX:"), 4))
			{
				pstr += 4;
				for (size = 0; *pstr; pstr++)
				{
					if (_T('0') <= *pstr && *pstr <= _T('9'))
					{
						size = size * 10 + (*pstr - _T('0'));
						continue;
					}
					if (!_tcscmp(pstr, _T("KB")))
					{
						size *= KB;
					}
					else if (!_tcscmp(pstr, _T("MB")))
					{
						size *= MB;
					}
					else if (_tcscmp(pstr, _T("B")))
					{
						size = 0;
					}
					break;
				}
				if (size > 0)
				{
					MIJL mijl;
					nRet = mijl.Load(argv[3]);
					if (nRet)
					{
						tcout << _T("Failed to load image(") << nRet << _T("): ") << argv[3] << endl;
					}
					else
					{
						LPVOID pData = malloc(size);
						if (!pData)
						{
							nRet = ERROR_NOT_ENOUGH_MEMORY;
						}
						else
						{
							nRet = mijl.GetJPEGData(pData, size, jq);
							if (nRet)
							{
								tcout << _T("Faield to get jpeg data: ") << nRet << endl;
							}
							else
							{
								nRet = SaveFile(argv[4], pData, size);
								if (!nRet)
								{
									tcout << _T("Success SIZE=") << size << _T(" JQ=") << jq << endl;
								}
							}

							free(pData);
						}
					}
				}
			}
		}
		else if (!_tcsicmp(argv[1], _T("JQ")))
		{
			LPCTSTR pstr = argv[2];
			switch (*pstr)
			{
			case _T('/'):
			case _T('-'):
				pstr++;
				break;
			}
			if (!_tcsnicmp(pstr, _T("JQ:"), 3))
			{
				pstr += 3;
			}

			int jq = _ttoi(pstr);
			if (jq > 0 && jq <= 100)
			{
				MIJL mijl;
				nRet = mijl.Load(argv[3]);
				if (nRet)
				{
					tcout << _T("Failed to load image(") << nRet << _T("): ") << argv[3] << endl;
				}
				else
				{
					LPVOID pData(NULL);
					int size(0);
					nRet = mijl.GetJPEGData(pData, size, jq);
					if (nRet)
					{
						tcout << _T("Faield to get jpeg data: ") << nRet << endl;
					}
					else
					{
						nRet = SaveFile(argv[4], pData, size);
						if (!nRet)
						{
							tcout << _T("Success JQ=") << jq << _T(" SIZE=") << size << endl;
						}
					}

					free(pData);
				}
			}
		}
	}
	if (nRet == -1)
	{
		tcout << _T("RSPIC") << endl
			<< _T("\t SIZE /MAX:<max file size> SrcPic DestPic") << endl
			<< _T("\t JQ /JQ:<0-100> SrcPic DestPic") << endl;
	}
	return nRet;
}

DWORD SaveFile(LPCTSTR lpszFile, LPVOID pData, int nLen)
{
	DWORD dwVal = 0;
	HANDLE hFile = CreateFile(lpszFile, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		dwVal = GetLastError();
		tcout << _T("Failed to create file(") << dwVal << _T("): ") << lpszFile << endl;
	}
	else
	{
		DWORD nWrite(0);
		if (!WriteFile(hFile, pData, nLen, &nWrite, NULL) || nWrite != nLen)
		{
			dwVal = GetLastError();
			tcout << _T("Failed to write data:") << dwVal << endl;
		}
		SetEndOfFile(hFile);
		CloseHandle(hFile);
	}
	return dwVal;
}
