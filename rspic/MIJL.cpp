#include "stdafx.h"
#include <ijl.h>
#include "MIJL.h"

#pragma comment(lib, "ijl15.lib")


MIJL::MIJL()
: m_pDib(NULL)
, m_nDibLen(0)
, m_nBitCount(0)
, m_nWidth(0)
, m_nHeight(0)
, m_biXdpi(0)
, m_biYdpi(0)
, m_jVersion(0)
, m_jUnits(0)
{
}


MIJL::~MIJL()
{
}

void MIJL::Release()
{
	if (m_pDib)
	{
		free(m_pDib);
		m_pDib = NULL;
		m_nDibLen = 0;
	}
}

DWORD MIJL::Load(LPCTSTR lpszFile)
{
	DWORD dwErr(0);
	HANDLE hFile = CreateFile(lpszFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		dwErr = GetLastError();
	}
	else
	{
		DWORD dwSize = ::GetFileSize(hFile, NULL);
		if (INVALID_FILE_SIZE == dwSize)
		{
			dwErr = GetLastError();
		}
		else
		{
			LPVOID pData = malloc(dwSize);
			if (NULL == pData)
			{
				dwErr = ERROR_NOT_ENOUGH_MEMORY;
			}
			else
			{
				DWORD dwRead(0);
				if (!ReadFile(hFile, pData, dwSize, &dwRead, NULL))
				{
					dwErr = GetLastError();
				}
				else
				{
					dwErr = Load(pData, dwRead);
				}

				free(pData);
			}

		}

		CloseHandle(hFile);
	}

	return dwErr;
}

int MIJL::Load(LPVOID pData, int nLen)
{
	Release();

	//Use the IJL to load up the jpeg
	JPEG_CORE_PROPERTIES jcp = {};
	//Init the IJL
	IJLERR ijlErr = ijlInit(&jcp);
	if (IJL_OK != ijlErr)
	{
		return ijlErr;
	}

	//Read in the Jpeg file parameters
	jcp.JPGFile = NULL;
	jcp.JPGSizeBytes = nLen;
	jcp.JPGBytes = (LPBYTE)pData;
	if (jcp.JPGBytes)
	{
		ijlErr = ijlRead(&jcp, IJL_JBUFF_READPARAMS);
		if (IJL_OK == ijlErr)
		{
			// Set up the info on the desired DIB properties.
			m_nWidth = jcp.DIBWidth = jcp.JPGWidth;
			m_nHeight = jcp.DIBHeight = jcp.JPGHeight; // Implies a bottom-up DIB

			m_jVersion = jcp.jprops.jfif_app0_version;
			m_jUnits = jcp.jprops.jfif_app0_units;
			m_biXdpi = jcp.jprops.jfif_app0_Xdensity;
			m_biYdpi = jcp.jprops.jfif_app0_Ydensity;

			if (IJL_G == jcp.JPGColor)
			{ // Crayscale
				m_nBitCount = 8;
				jcp.DIBColor = IJL_G;
				jcp.DIBChannels = 1;
			}
			else
			{ // Color
				m_nBitCount = 24;
				jcp.DIBColor = IJL_BGR;
				jcp.DIBChannels = 3;
			}

			jcp.DIBPadBytes = IJL_DIB_PAD_BYTES(jcp.DIBWidth, jcp.DIBChannels);
			m_nDibLen = IJL_DIB_AWIDTH(m_nWidth, jcp.DIBChannels) * m_nHeight;
			m_pDib = jcp.DIBBytes = (LPBYTE)malloc(m_nDibLen);
			if (NULL == jcp.DIBBytes)
			{
				ijlErr = IJL_BUFFER_TOO_SMALL;
			}
			else
			{
				ijlErr = ijlRead(&jcp, IJL_JBUFF_READWHOLEIMAGE);
				if (IJL_OK != ijlErr)
				{
					free(m_pDib);
					m_pDib = NULL;
					m_nDibLen = 0;
				}
			}
		}
	}
	ijlFree(&jcp);

	return ijlErr;
}

int MIJL::GetJPEGData(LPVOID& pData, int& nLen, int& jquality)
{
	// JPEG•Û‘¶
	JPEG_CORE_PROPERTIES jcp = {};
	IJLERR ijlErr = ijlInit(&jcp);
	if (IJL_OK != ijlErr)
	{
		return ijlErr;
	}

	switch (m_nBitCount)
	{
	case 24:
	{
		jcp.DIBColor = IJL_BGR;
		jcp.JPGColor = IJL_YCBCR;
		jcp.JPGChannels = jcp.DIBChannels = 3;
		jcp.JPGSubsampling = IJL_411;
		break;
	}
	case 8:
	{
		jcp.JPGColor = jcp.DIBColor = IJL_G;
		jcp.JPGChannels = jcp.DIBChannels = 1;
		jcp.JPGSubsampling = IJL_NONE;
		break;
	}
	default:
		ijlFree(&jcp);
		return IJL_EXCEPTION_DETECTED;
	}

	jcp.DIBWidth = m_nWidth;
	jcp.DIBHeight = m_nHeight;
	jcp.DIBBytes = (LPBYTE)m_pDib;
	jcp.DIBPadBytes = IJL_DIB_PAD_BYTES(jcp.DIBWidth, jcp.DIBChannels);

	jcp.JPGWidth = m_nWidth;
	jcp.JPGHeight = m_nHeight;

	jcp.jprops.jfif_app0_version = m_jVersion;//256
	jcp.jprops.jfif_app0_units = m_jUnits;//1
	jcp.jprops.jfif_app0_Xdensity = m_biXdpi;
	jcp.jprops.jfif_app0_Ydensity = m_biYdpi;

	if (pData)
	{
		jcp.JPGBytes = (LPBYTE)pData;

		jcp.jquality = 75;
		for (int nMin(0), nMax(100); ; )
		{
			jcp.JPGSizeBytes = nLen;
			ijlErr = ijlWrite(&jcp, IJL_JBUFF_WRITEWHOLEIMAGE);
			if (IJL_OK == ijlErr)
			{
				nMin = jcp.jquality;
				if (nMax <= nMin)
				{
					break;
				}
			}
			else if (IJL_BUFFER_TOO_SMALL == ijlErr)
			{
				nMax = jcp.jquality - 1;
			}
			else
			{
				break;
			}
			jquality = (nMax + nMin + 1) / 2;
			jcp.jquality = jquality - (jcp.jquality == jquality ? 1 : 0);
		}
		jquality = jcp.jquality;
		nLen = jcp.JPGSizeBytes;
	}
	else
	{
		jcp.jquality = jquality;

		const int MAXBUF = 10 * MB;
		jcp.JPGSizeBytes = (jcp.JPGWidth * jcp.JPGHeight / 2) & (-1 << 19);
		if (jcp.JPGSizeBytes < MB)
			jcp.JPGSizeBytes = MB;
		else if (jcp.JPGSizeBytes > MAXBUF)
			jcp.JPGSizeBytes = MAXBUF;

		for (const int STPBUF(jcp.JPGSizeBytes);;)
		{
			jcp.JPGBytes = (PBYTE)malloc(jcp.JPGSizeBytes);
			if (NULL == jcp.JPGBytes)
			{
				ijlErr = IJL_BUFFER_TOO_SMALL;
				break;
			}

			ijlErr = ijlWrite(&jcp, IJL_JBUFF_WRITEWHOLEIMAGE);
			if (IJL_OK == ijlErr)
			{
				break;
			}
			if (IJL_BUFFER_TOO_SMALL == ijlErr)
			{
				free(jcp.JPGBytes);
				jcp.JPGSizeBytes += STPBUF;
				continue;
			}
			break;
		}

		pData = jcp.JPGBytes;
		nLen = jcp.JPGSizeBytes;
	}
	ijlFree(&jcp);

	return ijlErr;
}