// MACTool.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <iostream>
using namespace std;

#ifdef _UNICODE
#define tcout std::wcout
#define	tstring std::wstring
#else
#define tcout std::cout
#define tstring std::string
#endif

inline bool GetOption(PCTSTR& pstr)
{
	switch (*pstr)
	{
	case _T('-'):
	case _T('/'):
		pstr++;
		return true;
	}
	return false;
}

union MAC
{
	BYTE		byMac[6];
	LONGLONG	lMac;
};

bool Parse(MAC& mac, const TCHAR* npstr, const TCHAR** endptr)
{
	if (!npstr)
		return false;
	mac.lMac = 0L;
	int hi(0), lo(0);
	for (int i(5); i >= 0; i--)
	{
		if (!npstr[0] || !npstr[1])
			return false;

		if (_T('0') <= npstr[0] && npstr[0] <= _T('9'))
			hi = npstr[0] - _T('0');
		else if (_T('A') <= npstr[0] && npstr[0] <= _T('F'))
			hi = npstr[0] - _T('A');
		else if (_T('a') <= npstr[0] && npstr[0] <= _T('f'))
			hi = npstr[0] - _T('a');
		else
			return false;

		if (_T('0') <= npstr[1] && npstr[1] <= _T('9'))
			lo = npstr[1] - _T('0');
		else if (_T('A') <= npstr[1] && npstr[1] <= _T('F'))
			lo = npstr[1] - _T('A');
		else if (_T('a') <= npstr[1] && npstr[1] <= _T('f'))
			lo = npstr[1] - _T('a');
		else
			return false;

		mac.byMac[i] = (BYTE)((hi << 4) | lo);

		switch (npstr[2])
		{
		case _T(':'):
		case _T('-'):
			npstr += 3;
			continue;
		default:
			if (endptr) *endptr = &npstr[2];
			return true;
		}
		break;
	}

	return false;
}
bool Parse(LONGLONG& lVal, const TCHAR* npstr, const TCHAR** endptr)
{
	if (!npstr)
		return false;

	const TCHAR* pch = npstr;
	if (*pch == _T('-'))
		pch++;

	if (*pch < _T('0') || *pch > _T('9'))
		return false;

	lVal = *pch - _T('0');
	pch++;
	for (; *pch; pch++)
	{
		if (*pch < _T('0') || *pch > _T('9'))
			break;
		lVal = lVal * 10 + (*pch - _T('0'));
	}
	if (*npstr == _T('-'))
		lVal = -lVal;
	if (endptr) *endptr = pch;
	return true;
}

// MACTool [/v] <start>[,<step>,<count>]
int _tmain(int argc, _TCHAR* argv[])
{
	bool bov(false);
	MAC mac; mac.lMac = -1;
	LONGLONG step = 1;
	LONGLONG count = 1;
	for (int i = 1; i < argc; i++)
	{
		PCTSTR pstr(argv[i]);
		if (GetOption(pstr))
		{
			if (!_tcsicmp(pstr, _T("v")))
			{
				bov = true;
			}
			else
			{
				goto ERR;
			}
		}
		else if (mac.lMac <= 0)
		{
			PCTSTR ptmp(NULL);
			if(!Parse(mac, pstr, &ptmp))
				goto ERR;
			if (!*ptmp) continue;
			if (*ptmp != _T(',')) goto ERR;
			pstr = ptmp + 1;

			if (!Parse(count, pstr, &ptmp))
				goto ERR;
			if (!*ptmp) continue;
			if (*ptmp != _T(',')) goto ERR;
			pstr = ptmp + 1;
			
			if (!Parse(step, pstr, &ptmp) || *ptmp == _T(':') || *ptmp == _T('-'))
			{
				MAC tMac = {};
				if (!Parse(tMac, pstr, &ptmp))
					goto ERR;
				step = tMac.lMac;
			}
			if (*ptmp) goto ERR;
		}
		else
			goto ERR;
	}
	if (mac.lMac < 0) goto ERR;

	if (bov)
	{
		for (; count > 0; count--, mac.lMac += step)
		{
			tcout << mac.lMac << endl;
		}
	}
	else
	{
		for (; count > 0; count--, mac.lMac += step)
		{
			_tprintf_s(_T("%02X:%02X:%02X:%02X:%02X:%02X"),
				mac.byMac[5], mac.byMac[4], mac.byMac[3], mac.byMac[2], mac.byMac[1], mac.byMac[0]);
			tcout << endl;
		}
	}
	return 0;

ERR:
	tcout << _T("MACTool [/v] <start>[,<count>[,<step>]]") << endl;
	return -1;
}

