// runtime.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>
#include <vector>

#ifdef _UNICODE
#define tcout std::wcout
#define tstring std::wstring
#define tendl std::endl
#endif

template<size_t N>
void PrintTime(TCHAR (&szText)[N], LONGLONG llt)
{
	if (llt == 0L)
	{
		_stprintf_s(szText, _T("%4ld"), 0L);
	}
	else
	{
		LONGLONG llv;
		if ((llv = llt / 10000000) > 0)
		{
			int x = llt % 10000000;
			_stprintf_s(szText, _T("%4ld.%07d s"), llv, x);
		}
		else if ((llv = llt / 10000) > 0)
		{
			int x = llt % 10000;
			_stprintf_s(szText, _T("%4ld.%04d ms"), llv, x);
		}
		else
		{
			llv = llt / 10;
			int x = llt % 10;
			_stprintf_s(szText, _T("%4ld.%d us"), llv, x);
		}
	}
}

void PrintTime(LONGLONG llKernal, LONGLONG llUser)
{
	TCHAR szText[50] = {};
	PrintTime(szText, llKernal);
	tcout << _T(" Kernal\t") << szText << tendl;
	PrintTime(szText, llUser);
	tcout << _T(" User  \t") << szText << tendl;
	PrintTime(szText, llKernal + llUser);
	tcout << _T(" Total \t") << szText << tendl;
}

template<size_t N>
void PrintTime(const FILETIME (&ftTime)[N])
{
	if (N == 4)
	{
		SYSTEMTIME st[2];
		{
			FILETIME ft;
			FileTimeToLocalFileTime(ftTime, &ft);
			FileTimeToSystemTime(&ft, &st[0]);
			FileTimeToLocalFileTime(ftTime + 1, &ft);
			FileTimeToSystemTime(&ft, &st[1]);
		}
		if (st[0].wYear == st[1].wYear && st[0].wMonth == st[1].wMonth && st[0].wDay == st[1].wDay)
		{
			DWORD dwSec = (st[1].wHour * 3600 + st[1].wMinute * 60 + st[1].wSecond) - (st[0].wHour * 3600 + st[0].wMinute * 60 + st[0].wSecond);
			DWORD dwMSec = 0;
			if (st[1].wMilliseconds >= st[0].wMilliseconds)
			{
				dwMSec = st[1].wMilliseconds - st[0].wMilliseconds;
			}
			else
			{
				dwMSec = 1000 + st[1].wMilliseconds - st[0].wMilliseconds;
				dwSec -= 1;
			}
			_tprintf_s(_T(" [%02d:%02d:%02d.%03d -> %02d:%02d:%02d.%03d] => %d.%d\n"),
				st[0].wHour, st[0].wMinute, st[0].wSecond, st[0].wMilliseconds,
				st[1].wHour, st[1].wMinute, st[1].wSecond, st[1].wMilliseconds,
				dwSec, dwMSec);
		}
		else
		{
			_tprintf_s(_T(" [%d-%02d-%02d %02d:%02d:%02d.%03d -> %d-%02d-%02d %02d:%02d:%02d.%03d] => %d.%d\n"),
				st[0].wYear, st[0].wMonth, st[0].wDay, st[0].wHour, st[0].wMinute, st[0].wSecond, st[0].wMilliseconds,
				st[1].wYear, st[1].wMonth, st[1].wDay, st[1].wHour, st[1].wMinute, st[1].wSecond, st[1].wMilliseconds);
		}
		PLARGE_INTEGER pli((PLARGE_INTEGER)(ftTime + 2));
		PrintTime(pli[0].QuadPart, pli[1].QuadPart);
	}
}

// runtime [/PID <PID>]|[/TID <TID>]|[/CMD "<Command>"]
int _tmain(int argc, _TCHAR* argv[])
{
	for (; argc == 3; )
	{
		PTSTR pstr = argv[1];
		PTSTR parg = argv[2];
		switch (*pstr)
		{
		case _T('/'):
		case _T('-'):
			pstr++;
			break;
		}
		if (!_tcsicmp(pstr, _T("CMD")))
		{
			STARTUPINFO si = { sizeof(si) };
			si.hStdError	= GetStdHandle(STD_ERROR_HANDLE);
			si.hStdInput	= GetStdHandle(STD_INPUT_HANDLE);
			si.hStdOutput	= GetStdHandle(STD_OUTPUT_HANDLE);
			PROCESS_INFORMATION pi = {};
			if (!CreateProcess(NULL, parg, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
			{
				DWORD dwErr = GetLastError();
				tcout << _T("Failed to execute the command (") << dwErr << _T("): ") << parg << tendl;
				return -2;
			}
			__try
			{
				WaitForSingleObject(pi.hProcess, INFINITE);

				{ // Process
					FILETIME ftTime[4] = {};
					if (!::GetProcessTimes(pi.hProcess, &ftTime[0], &ftTime[1], &ftTime[2], &ftTime[3]))
					{
						DWORD dwErr = GetLastError();
						tcout << _T("Failed to get the process time: ") << dwErr << tendl;
						return -3;
					}
					_tprintf_s(_T("PID: %-6d"), pi.dwProcessId);
					PrintTime(ftTime);
				}
				{ // Thread
					FILETIME ftTime[4] = {};
					if (!::GetThreadTimes(pi.hThread, &ftTime[0], &ftTime[1], &ftTime[2], &ftTime[3]))
					{
						DWORD dwErr = GetLastError();
						tcout << _T("Failed to get the thread time: ") << dwErr << tendl;
						return -4;
					}
					_tprintf_s(_T("> TID: %-4d"), pi.dwThreadId);
					PrintTime(ftTime);
				}
				DWORD dwCode(0);
				if (!GetExitCodeProcess(pi.hProcess, &dwCode))
				{
					dwCode = GetLastError();
					tcout << _T("Failed to get process exit code: ") << dwCode << tendl;
				}
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				return dwCode;
			}
			__finally
			{
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
			break;
		}
		int val = _ttoi(parg);
		if (errno == ERANGE || val <= 0)
		{
			break;
		}
		bool bWait = false;
		if (!_tcsicmp(pstr, _T("PID")) || (bWait = !_tcsicmp(pstr, _T("WAITPID"))))
		{
			HANDLE hProcess = OpenProcess((bWait ? SYNCHRONIZE : 0) | PROCESS_QUERY_INFORMATION, FALSE, val);
			if (!hProcess)
			{
				DWORD dwErr = GetLastError();
				tcout << _T("Failed to open the process: ") << dwErr << tendl;
				return -5;
			}
			if (bWait) WaitForSingleObject(hProcess, INFINITE);
			{ // Process
				FILETIME ftTime[4] = {};
				if (!::GetProcessTimes(hProcess, &ftTime[0], &ftTime[1], &ftTime[2], &ftTime[3]))
				{
					DWORD dwErr = GetLastError();
					tcout << _T("Failed to get the process time: ") << dwErr << tendl;
					return -3;
				}
				if (!ftTime[1].dwLowDateTime && !ftTime[1].dwHighDateTime) GetSystemTimeAsFileTime(&ftTime[1]);
				_tprintf_s(_T("PID: %-4d"), val);
				PrintTime(ftTime);
			}
			DWORD dwCode(0);
			if (bWait && !GetExitCodeProcess(hProcess, &dwCode))
			{
				dwCode = GetLastError();
				tcout << _T("Failed to get process exit code: ") << dwCode << tendl;
			}
			CloseHandle(hProcess);
			return dwCode;
		}
		if (!_tcsicmp(pstr, _T("TID")) || (bWait = !_tcsicmp(pstr, _T("WAITTID"))))
		{
			HANDLE hThread = OpenThread((bWait ? SYNCHRONIZE : 0) | THREAD_QUERY_INFORMATION, FALSE, val);
			if (!hThread)
			{
				DWORD dwErr = GetLastError();
				tcout << _T("Failed to open the thread: ") << dwErr << tendl;
				return -5;
			}
			if (bWait) WaitForSingleObject(hThread, INFINITE);
			{ // thread
				FILETIME ftTime[4] = {};
				if (!::GetThreadTimes(hThread, &ftTime[0], &ftTime[1], &ftTime[2], &ftTime[3]))
				{
					DWORD dwErr = GetLastError();
					tcout << _T("Failed to get the thread time: ") << dwErr << tendl;
					return -3;
				}
				if (!ftTime[1].dwLowDateTime && !ftTime[1].dwHighDateTime) GetSystemTimeAsFileTime(&ftTime[1]);

				_tprintf_s(_T("TID: %-4d"), val);
				PrintTime(ftTime);
			}
			DWORD dwCode(0);
			if (bWait && !GetExitCodeThread(hThread, &dwCode))
			{
				dwCode = GetLastError();
				tcout << _T("Failed to get process exit code: ") << dwCode << tendl;
			}
			CloseHandle(hThread);
			return 0;
		}
		break;
	}

	tcout << _T("runtime [/[WAIT]PID <PID>]|[/[WAIT]TID <TID>]|[/CMD \"<Command>\"]") << tendl;
	return -1;
}

