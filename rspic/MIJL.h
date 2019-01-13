#pragma once


const int KB = 1 << 10;
const int MB = KB << 10;

class MIJL
{
public:
	MIJL();
	~MIJL();

private:
	LPVOID	m_pDib;
	int		m_nDibLen;
	WORD	m_nBitCount;
	WORD	m_nWidth;
	WORD	m_nHeight;
	WORD	m_biXdpi;
	WORD	m_biYdpi;
	WORD	m_jVersion;
	BYTE	m_jUnits;
	
public:
	DWORD Load(LPCTSTR lpszFile);
	int Load(LPVOID pData, int nLen);
	void Release();

public:
	int GetJPEGData(LPVOID& pData, int& nLen, int& jquality);
};

