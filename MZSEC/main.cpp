// main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <Windows.h>
#include <mzhFile.h>
#include "mzhCrypt.h"

using namespace std;
using namespace MZH;

#ifdef _UNICODE
#define tcout wcout
#endif

//#ifdef _DEBUG
//#include <assert.h>
//#define ASSERT(f) assert(f)
//#define VERIFY(f) assert(f)
//#else
//#define ASSERT(f)
//#define VERIFY(f) f
//#endif


const UINT MAXBUFLEN = (1 << 20);

typedef struct CDATA {
	ULARGE_INTEGER uli[2];
	CDATA& operator = (const CDATA& dat)
	{ uli[0].QuadPart = dat.uli[0].QuadPart, uli[1].QuadPart = dat.uli[1].QuadPart; return *this; }
	CDATA& operator ^= (const CDATA& dat)
	{ uli[0].QuadPart ^= dat.uli[0].QuadPart, uli[1].QuadPart ^= dat.uli[1].QuadPart; return *this; }
} *PCDATA;

enum EACTION
{
	NONE,
	ENCRYPT,
	DECRYPT,
	SETSIZE,
	PROMPT
};
bool HitTest(LPCTSTR& pStr, LPCTSTR pKey)
{
	int n = (int)_tcslen(pKey);
	if (!_tcsnicmp(pStr, pKey, n))
	{
		switch (pStr[n])
		{
		case _T(':'):
		case _T('='):
			pStr += n + 1;
			return true;
		case _T('\0'):
			pStr = NULL;
			return true;
		}
	}
	return false;
}
template<typename T>
bool ParseSize(T& val, LPCTSTR pstr)
{
	if (!pstr || !*pstr)
		return false;

	val = 0;
	for (; *pstr; pstr++)
	{
		if (_T('0') <= *pstr && *pstr <= _T('9'))
		{
			val = val * 10 + (*pstr - _T('0'));
			continue;
		}
		switch (*pstr)
		{
		case _T('K'):
			val <<= 10;
			pstr++;
			break;
		case _T('M'):
			val <<= 20;
			pstr++;
			break;
		case _T('G'):
			val <<= 30;
			pstr++;
			break;
		case _T('T'):
			val <<= 40;
			pstr++;
			break;
		case _T('B'):
		case _T('\0'):
			return true;
		default:
			return false;
		}
		if (!*pstr || !_tcscmp(pstr, _T("B")))
			return true;
	}
	return true;
}

template<typename T>
BOOL HashKey(LPCTSTR pszKey, T& hashData)
{
	if (pszKey)
	{
		int n = WideCharToMultiByte(CP_UTF8, 0, pszKey, -1, NULL, 0, NULL, NULL);
		if (n > 0)
		{
			n += 2;
			LPSTR pUtf8 = new char[n];
			int x = WideCharToMultiByte(CP_UTF8, 0, pszKey, -1, pUtf8, n, NULL, NULL);
			CCrypt crypt;
			if (crypt.AcquireContext(NULL, NULL, PROV_RSA_AES))
			{
				if (crypt.CreateHash(CALG_SHA))
				{
					if (crypt.HashData((PBYTE)pUtf8, x))
					{
						BYTE btSHA[20];
						DWORD len(sizeof(btSHA));
						if (crypt.GetHashParam(HP_HASHVAL, btSHA, len))
						{
							crypt.DestroyHash();

							if (crypt.CreateHash(CALG_MD5))
							{
								if (crypt.HashData(btSHA, len))
								{
									len = sizeof(hashData);
									if (crypt.GetHashParam(HP_HASHVAL, (PBYTE)&hashData, len))
									{
										delete[] pUtf8;
										return TRUE;
									}
								}
								crypt.DestroyHash();
							}
						}
					}
				}
			}
			delete[] pUtf8;
		}
	}

	return FALSE;
}

// MZSEC /[E|D] /KEY:<sec key> [/FILE:<source file>] [/OUT:<out file>] [/START:<0>] [/SIZE:<1MB>]
LPCTSTR CTSTR_PROMPT = TEXT("MZSEC\t/E|D /KEY:<pwd> [/FILE:<source>] [/START:<0>] [/SIZE:<-1>] [/OUT:<dest>] [/OUTSTART:<-1>]\n\t/S /FILE:<file> /SIZE:<*>");
#define TESTPROMPT(f)	if(f) { tcout << CTSTR_PROMPT << endl; return -1; }

int _tmain(int argc, _TCHAR* argv[])
{
	EACTION eAction(NONE);
	LPCTSTR lpszKey(NULL);
	LPCTSTR lpszFile(NULL);
	LPCTSTR lpszOutput(NULL);
	LONGLONG lStartPos(-1LL);
	LONGLONG lOutStart(-1LL);
	LONGLONG lSize(-1LL);
	{
#define CHECKVAL(var, val) if(var != val) { eAction = PROMPT; break; }
#define ADJUST(var)	\
	if(var) { }		\
	else if(i + 1 < argc) var = argv[++i];	\
	else { eAction = PROMPT; break; }

		for (int i(1); i < argc; i++)
		{
			LPCTSTR pstr(argv[i]);
			switch (*pstr)
			{
			case _T('/'):
			case _T('-'):
				pstr++;
				break;
			}

			if (!_tcsicmp(pstr, _T("e")))
			{
				CHECKVAL(eAction, NONE);
				eAction = ENCRYPT;
				continue;
			}
			if (!_tcsicmp(pstr, _T("d")))
			{
				CHECKVAL(eAction, NONE);
				eAction = DECRYPT;
				continue;
			}
			if (!_tcsicmp(pstr, _T("s")))
			{
				CHECKVAL(eAction, NONE);
				eAction = SETSIZE;
				continue;
			}
			if (HitTest(pstr, _T("key")))
			{
				ADJUST(pstr);
				CHECKVAL(lpszKey, NULL);
				lpszKey = pstr;
				continue;
			}
			if (HitTest(pstr, _T("file")))
			{
				ADJUST(pstr);
				CHECKVAL(lpszFile, NULL);
				lpszFile = pstr;
				continue;
			}
			if (HitTest(pstr, _T("start")))
			{
				ADJUST(pstr);
				if (lStartPos != -1LL || !ParseSize(lStartPos, pstr))
				{
					eAction = PROMPT;
					break;
				}
				continue;
			}
			if (HitTest(pstr, _T("size")))
			{
				ADJUST(pstr);
				if (lSize != -1L || !ParseSize(lSize, pstr))
				{
					eAction = PROMPT;
					break;
				}
				continue;
			}
			if (HitTest(pstr, _T("out")))
			{
				ADJUST(pstr);
				CHECKVAL(lpszOutput, NULL);
				lpszOutput = pstr;
				continue;
			}
			if (HitTest(pstr, _T("outstart")))
			{
				ADJUST(pstr);
				if (lOutStart != -1LL || !ParseSize(lOutStart, pstr))
				{
					eAction = PROMPT;
					break;
				}
				continue;
			}
			eAction = PROMPT;
			break;
		}

		if (eAction == SETSIZE)
		{
			if (!lpszFile || lpszOutput || lpszKey || lStartPos >= 0LL || lOutStart >= 0LL || lSize < 0LL)
				eAction = PROMPT;
			CFile tmpFile;
			if (!tmpFile.Open(lpszFile, GENERIC_READ | GENERIC_WRITE))
			{
				DWORD dwVal = GetLastError();
				tcout << _T("failed to open file (") << dwVal << _T("): ") << lpszFile << endl;
				return -3;
			}
			FILETIME ftMod = {};
			VERIFY(tmpFile.GetFileTime(&ftMod));
			VERIFY(tmpFile.SetLength(lSize));
			VERIFY(tmpFile.SetFileTime(&ftMod));
			tmpFile.Close();
			return 0L;
		}
	}
	TESTPROMPT(eAction == PROMPT || !lpszKey);

	CDATA hashData = {};
	if (!HashKey(lpszKey, hashData))
	{
		DWORD dwErr = GetLastError();
		_tprintf_s(_T("failed to get the hash data: 0x%x\n"), dwErr);
		return -2;
	}

	if (lpszFile)
	{
		TESTPROMPT(eAction == NONE)
		if (-1LL == lStartPos) lStartPos = 0;

		if (lpszOutput)
		{
			CFile srcFile;
			if (!srcFile.Open(lpszFile, GENERIC_READ, OPEN_EXISTING, FILE_SHARE_READ))
			{
				DWORD dwVal = GetLastError();
				tcout << _T("failed to open file (") << dwVal << _T("): ") << lpszFile << endl;
				return -3;
			}
			CFile dstFile;
			if (!dstFile.Open(lpszOutput, GENERIC_WRITE, OPEN_ALWAYS))
			{
				DWORD dwVal = GetLastError();
				tcout << _T("failed to create file (") << dwVal << _T("): ") << lpszOutput << endl;
				return -4;
			}

			LONGLONG size = srcFile.GetLength();
			if (lStartPos > size)
			{
				tcout << _T("start position error!") << endl;
				return -5;
			}
			size -= lStartPos;
			if (lSize < 0LL || lSize > size) lSize = size;

			size = (lSize + sizeof(CDATA) - 1) / sizeof(CDATA);
			if (size > MAXBUFLEN) size = MAXBUFLEN;

			srcFile.SeekTo(lStartPos);
			if (lOutStart < 0LL)
				dstFile.SeekToEnd();
			else
				dstFile.SeekTo(lOutStart);

			const UINT BUFLEN = (UINT)(size);
			const UINT BUFSIZE = BUFLEN * sizeof(CDATA);
			const UINT MAXLEN = BUFLEN + 1;
			PCDATA pData = new CDATA[MAXLEN];
			PCDATA pTmp(pData + 1);

			int ret = 0;
			//EncryptFile(hSrcFile, hDestFile, uStartPos, uSize);
			if (eAction == ENCRYPT)
			{
				while (lSize)
				{
					UINT uVal = (UINT)min(lSize, BUFSIZE);
					if (!srcFile.Read(pTmp, uVal))
					{
						DWORD dwVal = GetLastError();
						tcout << _T("failed to read data: ") << dwVal << endl;
						ret = -6;
						break;
					}
					int iMaxIndex = (uVal - 1) / sizeof(CDATA);

					pData[0] = hashData;
					hashData = pTmp[iMaxIndex];

					for (int i(iMaxIndex); i >= 0; i--)
					{
						pTmp[i] ^= pData[i];
					}

					if (dstFile.Write(pTmp, uVal))
					{
						lSize -= uVal;
					}
					else
					{
						DWORD dwVal = GetLastError();
						tcout << _T("failed to write data: ") << dwVal << endl;
						ret = -7;
						break;
					}
				}
			}
			else// if (eAction == DECRYPT)
			{
				pData[0] = hashData;

				while (lSize)
				{
					UINT uVal = (UINT)min(lSize, BUFSIZE);
					if (!srcFile.Read(pTmp, uVal))
					{
						DWORD dwVal = GetLastError();
						tcout << _T("failed to read data: ") << dwVal << endl;
						ret = -6;
						break;
					}
					int iMaxIndex = (uVal - 1) / sizeof(CDATA);
					for (int i(0); i <= iMaxIndex; i++)
					{
						pTmp[i] ^= pData[i];
					}

					if (dstFile.Write(pTmp, uVal))
					{
						lSize -= uVal;
						pData[0] = pTmp[iMaxIndex];
					}
					else
					{
						DWORD dwVal = GetLastError();
						tcout << _T("failed to write data: ") << dwVal << endl;
						ret = -7;
						break;
					}
				}
			}
			delete[] pData;
			return ret;
		}
		else
		{
			CFile mFile;
			if (!mFile.Open(lpszFile, GENERIC_READ | GENERIC_WRITE, OPEN_EXISTING))
			{
				DWORD dwVal = GetLastError();
				tcout << _T("failed to open file (") << dwVal << _T("): ") << lpszFile << endl;
				return -3;
			}
			FILETIME ftMod = {};
			VERIFY(mFile.GetFileTime(&ftMod));
			LONGLONG size = mFile.GetLength();
			if (lStartPos > size)
			{
				tcout << _T("start position error!") << endl;
				return -5;
			}
			size -= lStartPos;
			if (lSize > size) lSize = size;

			size = (lSize + sizeof(CDATA) - 1) / sizeof(CDATA);
			if(size > MAXBUFLEN) size = MAXBUFLEN;

			const UINT BUFLEN = (UINT)(size);
			const UINT BUFSIZE = BUFLEN * sizeof(CDATA);
			const UINT MAXLEN = BUFLEN + 1;
			PCDATA pData = new CDATA[MAXLEN];
			PCDATA pTmp(pData + 1);
			int ret = 0;
			//EncryptFile(hFile, uStartPos, uSize);
			if(eAction == ENCRYPT)
			{
				mFile.SeekTo(lStartPos);
				while(lSize)
				{
					UINT uVal = (UINT)min(lSize, BUFSIZE);
					if (!mFile.Read(pTmp, uVal))
					{
						DWORD dwVal = GetLastError();
						tcout << _T("failed to read data: ") << dwVal << endl;
						ret = -6;
						break;
					}
					int iMaxIndex = (uVal - 1) / sizeof(CDATA);

					pData[0] = hashData;
					hashData = pTmp[iMaxIndex];

					for (int i(iMaxIndex); i >= 0; i--)
					{
						pTmp[i] ^= pData[i];
					}
					mFile.SeekTo(lStartPos);
					if (mFile.Write(pTmp, uVal))
					{
						lStartPos += uVal;
						lSize -= uVal;
					}
					else
					{
						DWORD dwVal = GetLastError();
						tcout << _T("failed to write data: ") << dwVal << endl;
						ret = -7;
						break;
					}
				}
			}
			else// if (eAction == DECRYPT)
			{
				pData[0] = hashData;

				mFile.SeekTo(lStartPos);
				while(lSize)
				{
					UINT uVal = (UINT)min(lSize, BUFSIZE);
					if (!mFile.Read(pTmp, uVal))
					{
						DWORD dwVal = GetLastError();
						tcout << _T("failed to read data: ") << dwVal << endl;
						ret = -6;
						break;
					}
					int iMaxIndex = (uVal - 1) / sizeof(CDATA);
					for (int i(0); i <= iMaxIndex; i++)
					{
						pTmp[i] ^= pData[i];
					}
					mFile.SeekTo(lStartPos);
					if (mFile.Write(pTmp, uVal))
					{
						lStartPos += uVal;
						lSize -= uVal;

						pData[0] = pTmp[iMaxIndex];
					}
					else
					{
						DWORD dwVal = GetLastError();
						tcout << _T("failed to write data: ") << dwVal << endl;
						ret = -7;
						break;
					}
				}
			}
			delete[] pData;
			VERIFY(mFile.SetFileTime(&ftMod));
			return ret;
		}
	}
	else
	{
		TESTPROMPT(eAction != NONE)

		PBYTE pData = (PBYTE)&hashData;
		for (int i(0); i < sizeof(hashData); i++) _tprintf(_T("%02X"), pData[i]);
		tcout << endl;

		if (lpszOutput)
		{
			CFile mFile;
			if (!mFile.Open(lpszOutput, GENERIC_WRITE, CREATE_ALWAYS))
			{
				DWORD dwVal = GetLastError();
				tcout << _T("failed to open file (") << dwVal << _T("): ") << lpszOutput << endl;
				return -3;
			}
			UINT uVal(sizeof(hashData));
			if (mFile.Write(&hashData, uVal) || uVal != sizeof(hashData))
			{
				DWORD dwVal = GetLastError();
				mFile.Close();
				DeleteFile(lpszOutput);
				tcout << _T("failed to write file (") << dwVal << _T(")") << endl;
				return -4;
			}
		}
	}

	return 0;
}

