// Beep.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <iostream>
#include <vector>
#include <regex>
using namespace std;

#define tcout	wcout
#define tstring wstring
#define tregex	wregex
#define tmatch	wcmatch

struct STBEEP
{
	DWORD	dwFreq;
	DWORD	dwDuration;
};

typedef vector<STBEEP> VTR_BEEP;

// Beep [500] [500,500]
int _tmain(int argc, _TCHAR* argv[])
{
	tregex rx(_T("^[0-9]+$"));
	VTR_BEEP vtrBeep(argc - 1);
	for (int i(1); i < argc; i++)
	{
		tstring str(argv[i]);
		STBEEP& sBeep = vtrBeep[i-1];
		size_t idx = str.find(_T(','));
		if (tstring::npos == idx)
		{
			if (regex_match(str, rx))
			{
				sBeep.dwFreq = 0;
				sBeep.dwDuration = stoul(str);
				continue;
			}
		}
		else if (str.find(_T(','), idx + 1) == tstring::npos)
		{
			const tstring& fre = str.substr(0, idx);
			const tstring& dur = str.substr(idx + 1);
			if (regex_match(dur, rx) && regex_match(fre, rx))
			{
				sBeep.dwDuration = stoul(dur);
				sBeep.dwFreq = stoul(fre);
				continue;
			}
		}
		vtrBeep.clear();
		break;
	}
	if (vtrBeep.empty())
	{
		tcout << _T("Beep [500]|[500,500] ...") << endl;
		return -1;
	}

	for each (const STBEEP& var in vtrBeep)
	{
		if (var.dwFreq)
		{
			tcout << var.dwFreq << _T(",") << var.dwDuration;
			if (!Beep(var.dwFreq, var.dwDuration))
			{
				DWORD dwVal = GetLastError();
				tcout << _T(" => EC:") << dwVal;
			}
		}
		else
		{
			tcout << var.dwDuration;
			Sleep(var.dwDuration);
		}
		tcout << endl;
	}

	return 0;
}

