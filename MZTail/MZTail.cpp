// MZTail.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <list>
using namespace std;

#ifdef _UNICODE
#define tcout wcout
#define tstring wstring
#else
#define tcout cout
#endif

using namespace System;
using namespace System::IO;
using namespace System::Text;

const UINT BUFLEN = 8 * (1 << 10);
LPVOID gpbuf(NULL);
HANDLE ghDir(INVALID_HANDLE_VALUE);
HANDLE ghEvent(NULL);


// MZTail [/T:<1>ms] [/EC:<UTF-8|UTF-16LE|...>] [/M:<...>] [/E] <file name>
int _tmain(int argc, _TCHAR* argv[])
{
	bool bEnd(false);
	DWORD dwInterval(INFINITE);
	LPCTSTR lpszCCS(NULL);
	LPCTSTR lpszMark(NULL);
	LPCTSTR lpszPath(NULL);
	for (int i(1); i < argc; i++)
	{
		LPCTSTR ptr(argv[i]);
		switch (*ptr)
		{
		case _T('/'):
		case _T('-'):
			ptr++;
			if (!_tcsnicmp(ptr, _T("t:"), 2))
			{
				if (INFINITE != dwInterval) goto USAGE;
				for (LPCTSTR px(ptr + 2); *px; px++)
				{
					if (*px < _T('0') || *px > _T('9')) goto USAGE;
					dwInterval = dwInterval * 10 + *px - _T('0');
				}
				continue;
			}
			if (!_tcsnicmp(ptr, _T("ec:"), 3))
			{
				if (lpszCCS) goto USAGE;
				lpszCCS = ptr + 3;
				continue;
			}
			if (!_tcsnicmp(ptr, _T("m:"), 2))
			{
				if (lpszMark) goto USAGE;
				lpszMark = ptr + 2;
				continue;
			}
			if (!_tcsicmp(ptr, _T("e")))
			{
				if (bEnd) goto USAGE;
				bEnd = true;
				continue;
			}
			goto USAGE;
		}
		if (lpszPath) goto USAGE;
		lpszPath = ptr;
	}

	if (!lpszPath) goto USAGE;

	TextReader^ tr = nullptr;
	try
	{
		Encoding^ ec = nullptr;
		if (lpszCCS)
			ec = Encoding::GetEncoding(gcnew String(lpszCCS));
		FileStream^ fs = File::Open(gcnew String(lpszPath), FileMode::Open, FileAccess::Read, FileShare::ReadWrite | FileShare::Delete);
		tr = ec ? gcnew StreamReader(fs, ec) : gcnew StreamReader(fs, true);
		if (bEnd)
			tr->ReadToEnd();
	}
	catch (FileNotFoundException^)
	{
		tcout << _T("Cannot find the file: ") << lpszPath << endl;
		return -2;
	}
	catch (Exception^ ex)
	{
		Console::WriteLine(ex->ToString());
		return -2;
	}
	try
	{
		ghEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
		if (!ghEvent) goto USAGE;
		LPCTSTR lpszFileName = _tcsrchr(lpszPath, _T('\\'));
		{
			TCHAR szDir[MAX_PATH];
			if (lpszFileName)
			{
				int len = lpszFileName - lpszPath;
				if (len == 2 && lpszPath[1] == _T(':')) len = 3;
				_tcsncpy_s(szDir, lpszPath, len);
				lpszFileName++;
			}
			else
			{
				_tcscpy_s(szDir, _T("."));
				lpszFileName = lpszFileName;
			}
			ghDir = CreateFile(szDir, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_BACKUP_SEMANTICS, NULL);
			if (INVALID_HANDLE_VALUE == ghDir)
			{
				DWORD dwErr = GetLastError();
				tcout << _T("Failed to open the parent directory: ") << dwErr << endl;
				return -4;
			}
		}
		
		gpbuf = malloc(BUFLEN);
		OVERLAPPED ovl = {};
		ovl.hEvent = ghEvent;
		String^ strMark = lpszMark ? gcnew String(lpszMark) : nullptr;
		for (;;)
		{
			while (tr)
			{
				String^ text = tr->ReadLine();
				if (text == nullptr) break;
				if (strMark != nullptr && text->IndexOf(strMark) >= 0)
				{
					Console::ForegroundColor = ConsoleColor::Red;
					Console::WriteLine(text);
					Console::ResetColor();
					//SetConsoleTextAttribute(hStdOut, FOREGROUND_RED);
					//tcout << szText << endl;
					//SetConsoleTextAttribute(hStdOut, tc);
				}
				else
				{
					Console::WriteLine(text);
					//tcout << szText << endl;
				}
			}
			for (DWORD dwTime = GetTickCount();;)
			{
				if (!ReadDirectoryChangesW(ghDir, gpbuf, BUFLEN, FALSE, FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_CREATION, NULL, &ovl, NULL))
				{
					DWORD dwErr = GetLastError();
					tcout << _T("Failed to read changes: ") << dwErr << endl;
					return -5;
				}
				DWORD dwWait = dwInterval;
				if (dwWait != INFINITE)
				{
					DWORD dwNext = dwTime + dwInterval;
					DWORD dwNow = GetTickCount();
					dwWait = dwNow > dwNext ? 0 : dwNext - dwNow;
				}
				DWORD dwRes = WaitForSingleObject(ghEvent, dwInterval);
				if (WAIT_OBJECT_0 == dwRes)
				{
					DWORD nBytes(0);
					if (!GetOverlappedResult(ghDir, &ovl, &nBytes, TRUE))
					{
						DWORD dwErr = GetLastError();
						tcout << _T("Failed to get result: ") << dwErr << endl;
						return -6;
					}
					for (PFILE_NOTIFY_INFORMATION pfni = (PFILE_NOTIFY_INFORMATION)gpbuf;; pfni = (PFILE_NOTIFY_INFORMATION)(((PBYTE)pfni) + pfni->NextEntryOffset))
					{
						tstring filename(pfni->FileName, pfni->FileNameLength / sizeof(TCHAR));
						if (!_tcsicmp(filename.c_str(), lpszFileName))
						{
							switch (pfni->Action)
							{
							case FILE_ACTION_MODIFIED:
								{
									if (tr)
									{
										for (;;)
										{
											String^ text = tr->ReadLine();
											if (text == nullptr) break;
											if (strMark != nullptr && text->IndexOf(strMark) >= 0)
											{
												Console::ForegroundColor = ConsoleColor::Red;
												Console::BackgroundColor = ConsoleColor::Black;
												Console::WriteLine(text);
												Console::ResetColor();
											}
											else
											{
												Console::WriteLine(text);
											}
										}
										dwRes = NOERROR;
										dwTime = GetTickCount();
										break;
									}
								}
							case FILE_ACTION_ADDED:
							case FILE_ACTION_RENAMED_NEW_NAME:
								{
									if (tr)
									{
										tr->Close();
										tr = nullptr;
									}
									try
									{
										Encoding^ ec = nullptr;
										if (lpszCCS)
											ec = Encoding::GetEncoding(gcnew String(lpszCCS));
										FileStream^ fs = File::Open(gcnew String(lpszPath), FileMode::Open, FileAccess::Read, FileShare::ReadWrite | FileShare::Delete);
										tr = ec ? gcnew StreamReader(fs, ec) : gcnew StreamReader(fs, true);
										dwRes = WAIT_TIMEOUT;
										Console::ForegroundColor = ConsoleColor::Yellow;
										Console::BackgroundColor = ConsoleColor::Black;
										Console::WriteLine("========================NEW=========================");
										Console::ResetColor();
									}
									catch (FileNotFoundException^) {}
									catch (Exception^ ex)
									{
										Console::WriteLine(ex->ToString());
									}
									break;
								}
							case FILE_ACTION_REMOVED:
							case FILE_ACTION_RENAMED_OLD_NAME:
								{
									if (tr)
									{
										tr->Close();
										tr = nullptr;
										Console::ForegroundColor = ConsoleColor::Yellow;
										Console::BackgroundColor = ConsoleColor::Black;
										Console::WriteLine("========================END=========================");
										Console::ResetColor();
									}
									break;
								}
							}
						}
						if (!pfni->NextEntryOffset)
							break;
					}
				}
				if (WAIT_TIMEOUT == dwRes || ((GetTickCount() - dwTime) >= dwInterval)) break;
			}
		}
	}
	catch (Exception^ ex)
	{
		Console::WriteLine(ex->ToString());
		return -11;
	}
	finally
	{
		if (INVALID_HANDLE_VALUE != ghDir)
		{
			CloseHandle(ghDir);
			ghDir = INVALID_HANDLE_VALUE;
		}
		if (ghEvent)
		{
			CloseHandle(ghEvent);
			ghEvent = NULL;
		}
		if (gpbuf)
		{
			free(gpbuf);
			gpbuf = NULL;
		}
	}
	return 0;
USAGE:
	tcout << _T("MZTail [/EC:<UTF-8|...>] [/M:<...>] [/T:<N>ms] [/E] <file name>") << endl;
	return -1;
}

