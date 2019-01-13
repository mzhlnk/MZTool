// WAIT.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <conio.h>
#include <iostream>
using namespace std;

#define tcout wcout

int Parse(LPCTSTR pstr);

// wait [1s]|until <time>
int _tmain(int argc, _TCHAR* argv[])
{
	DWORD dwNow = GetTickCount();
	if (argc == 2)
	{
		int wait = Parse(argv[1]);
		if (wait > 0)
		{
			//Sleep(wait);
			PCTSTR CSTR_BS = _T("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			PCTSTR CSTR_SP = _T("                    ");
			PCTSTR PCSTR_BS = CSTR_BS + _tcslen(CSTR_BS);
			PCTSTR PCSTR_SP = CSTR_SP + _tcslen(CSTR_SP);
			TCHAR szText[20] = {};

			DWORD dwEnd = dwNow + wait;
			int oldSec = -1;
			PCTSTR lpClr[3] = {};
			for (dwNow = GetTickCount(); dwNow < dwEnd; dwNow = GetTickCount())
			{
				DWORD dwVal = dwEnd - dwNow;
				int sec = dwVal / 1000;
				int min = sec / 60;
				int hour = min / 60;
				sec %= 60;
				min %= 60;
				if (oldSec != sec)
				{
					oldSec = sec;

					int len = 0;
					if (hour)
						len = _stprintf_s(szText, _T("%2d:%02d:%02d"), hour, min, sec);
					else if (min)
						len = _stprintf_s(szText, _T("%2d:%02d"), min, sec);
					else
						len = _stprintf_s(szText, _T("%2d"), sec);

					if (lpClr[0])
						for each (PCTSTR var in lpClr)
						{
							tcout << var;
						}
					tcout << szText;

					lpClr[0] = PCSTR_BS - len;
					lpClr[1] = PCSTR_SP - len;
					lpClr[2] = PCSTR_BS - len;
				}
				if (dwVal > 1000) dwVal %= 1000;
				Sleep(min(dwVal, 500));

				if (_kbhit() && _getch() == VK_ESCAPE)
				{
					if (lpClr[0])
						for each (PCTSTR var in lpClr)
						{
							tcout << var;
						}
					return -1;
				}
			}

			if (lpClr[0])
				for each (PCTSTR var in lpClr)
				{
					tcout << var;
				}

			return 0;
		}
	}

	if (argc == 3)
	{
		PCTSTR pstr = argv[1];
		switch (*pstr)
		{
		case _T('-'):
		case _T('/'):
			pstr++;
		default:
			break;
		}
		if (!_tcsicmp(argv[1], _T("until")) || !_tcsicmp(argv[1], _T("-u")))
		{

		}
		tcout << _T("Not Implement!") << endl;
	}
	tcout << _T("wait <N>[s|m|H|ms]") << endl;
	return -1;
}

int Parse(LPCTSTR pstr)
{
	int val = 0;
	PCTSTR pEnd(pstr);
	for (; *pEnd; pEnd++)
	{
		int x = *pEnd - _T('0');
		if (x < 0 || x > 9)
		{
			break;
		}
		val = val * 10 + x;
	}

	if (!*pEnd || !_tcsicmp(pEnd, _T("s")))
		val *= 1000;
	else if (!_tcsicmp(pEnd, _T("m")))
		val *= 60 * 1000;
	else if (!_tcsicmp(pEnd, _T("H")))
		val *= 60 * 60 * 1000;
	else if (!_tcsicmp(pEnd, _T("ms")))
		val;
	else
		val = 0;

	return val;
}