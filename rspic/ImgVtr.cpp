#include "stdafx.h"
#include "ImgVtr.h"

HRESULT CImgVtr::CreateNativeDataByIJL(void)
{
	TRACE_FUNC();
	if(m_hBmp)
	{
		_DSM_Free(m_hBmp);
	}

	HRESULT hr(E_FAIL);
	m_nBmpSize = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * GetColorUsed() + GetImageSize();
	m_hBmp = _DSM_Alloc(m_nBmpSize);
	if(!m_hBmp)
	{
		LOG_ERR(_T("Low Memory:") << m_nBmpSize);
		hr = E_OUTOFMEMORY;
	}
	else
	{
		LPBITMAPINFOHEADER lpBih = (LPBITMAPINFOHEADER)_DSM_LockMemory(m_hBmp);
		if(!lpBih)
		{
			LOG_ERR(_T("Failed to lock memory!!"));
			hr = E_OUTOFMEMORY;
		}
		else
		{
			//Use the IJL to load up the jpeg
			JPEG_CORE_PROPERTIES jcp;
			ZeroMemory(&jcp, sizeof(jcp));

			//Init the IJL
			IJLERR ijlErr = ijlInit(&jcp);
			if(IJL_OK != ijlErr)
			{
				LOG_ERR(_T("Cannot initialize Intel JPEG library: ") << ijlErr);
			}
			else
			{
				//Read in the Jpeg file parameters
				jcp.JPGFile      = NULL;
				jcp.JPGSizeBytes = (int)GlobalSize(m_hGlbJpeg);
				jcp.JPGBytes     = (LPBYTE)GlobalLock(m_hGlbJpeg);
				if(!jcp.JPGBytes)
				{
					LOG_ERR(_T("GlobalLock error: ") << GetLastError());
				}
				else
				{
					ijlErr = ijlRead(&jcp, IJL_JBUFF_READPARAMS);
					if(IJL_OK != ijlErr)
					{
						LOG_ERR(_T("Cannot Read JPEG header: ") << ijlErr);
					}
					else
					{
						// Set up the info on the desired DIB properties.
						jcp.DIBWidth	= jcp.JPGWidth;
						jcp.DIBHeight	= -jcp.JPGHeight; // Implies a bottom-up DIB

						lpBih->biWidth	= GetWidth();
						lpBih->biHeight	= GetHeight();

						lpBih->biSize			= sizeof(BITMAPINFOHEADER);
						lpBih->biXPelsPerMeter	= RES_I2M(m_biXdpi);
						lpBih->biYPelsPerMeter	= RES_I2M(m_biYdpi);
						lpBih->biPlanes			= 1;
						lpBih->biCompression	= BI_RGB;
						lpBih->biClrImportant	= 0;
						lpBih->biSizeImage		= GetImageSize();

						if(IJL_G == jcp.JPGColor)
						{ // Crayscale
							ASSERT(8 == GetBitCount());
							jcp.DIBColor		= IJL_G;
							jcp.DIBChannels		= 1;
							jcp.DIBPadBytes		= IJL_DIB_PAD_BYTES(jcp.DIBWidth, jcp.DIBChannels);

							lpBih->biBitCount	= 8;
							lpBih->biClrUsed	= 256;
							for(int i(0); i < 256; i++)
							{
								LPRGBQUAD lpQuad = ((LPRGBQUAD)(lpBih + 1)) + i;
								lpQuad->rgbBlue = lpQuad->rgbGreen = lpQuad->rgbRed = (BYTE)i;
								lpQuad->rgbReserved = 0;
							}
							jcp.DIBBytes = (LPBYTE)(lpBih + 1) + 256 * sizeof(RGBQUAD);
						}
						else
						{ // Color
							ASSERT(24 == GetBitCount());
							jcp.DIBColor		= IJL_BGR;
							jcp.DIBChannels		= 3;
							jcp.DIBPadBytes		= IJL_DIB_PAD_BYTES(jcp.JPGWidth, jcp.DIBChannels);
							lpBih->biBitCount	= 24;
							lpBih->biClrUsed	= 0;
							jcp.DIBBytes		= (LPBYTE)(lpBih + 1);
						}

						ijlErr = ijlRead(&jcp, IJL_JBUFF_READWHOLEIMAGE);
						if(IJL_OK != ijlErr)
						{
							LOG_ERR(_T("Cannot Read image JPEG data: ") << ijlErr);
						}
						else
						{
							ijlFree(&jcp);
							_DSM_UnlockMemory(m_hBmp);
							GlobalUnlock(m_hGlbJpeg);
							return S_OK;
						}
					}
					GlobalUnlock(m_hGlbJpeg);
				}
				ijlFree(&jcp);
			}
			_DSM_UnlockMemory(m_hBmp);
		}
		_DSM_Free(m_hBmp);
		m_hBmp = NULL;
	}

	return hr;
}

HRESULT CImgVtr::CreateNativeDataByGDI(void)
{
	TRACE_FUNC();

	CComPtr<IStream> ptrSrcStream, ptrBmpStream;
	HRESULT hr = CreateStreamOnHGlobal(m_hGlbJpeg, FALSE, &ptrSrcStream);
	if(FAILED(hr))
	{
		LOG_ERR(_T("CreateStreamOnHGlobal error: ") << HEX8(hr));
		return hr;
	}

	Status st(Ok);
	ULONG_PTR gdiplusToken(0);
	GdiplusStartupInput gdiplusStartupInput;
	st = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	if(Ok != st)
	{
		LOG_ERR(_T("GdiplusStartup error: ") << HEX8(hr));
		return GDIS2HR(st);
	}

	CLSID clsidEncoder = {0};
	hr = GetEncodecClsid(clsidEncoder, ImageFormatBMP);
	if(FAILED(hr))
	{
		LOG_ERR(_T("GetEncodecClsid error: ") << hr);
	}
	else
	{
		Image* pImage = new Image(ptrSrcStream);
		if(!pImage)
		{
			LOG_ERR(_T("Failed to create Image object!"));
			hr = E_OUTOFMEMORY;
		}
		else
		{
			hr = CreateStreamOnHGlobal(NULL, TRUE, &ptrBmpStream);
			if(FAILED(hr))
			{
				LOG_ERR(_T("CreateStreamOnHGlobal error: ") << HEX8(hr));
			}
			else
			{
				st = pImage->Save(ptrBmpStream, &clsidEncoder);
				if(Ok != st)
				{
					LOG_ERR(_T("Image::Save error") << st);
					ptrBmpStream.Release();
					hr = GDIS2HR(st);
				}
			}

			delete pImage;
		}
	}
	GdiplusShutdown(gdiplusToken);
	ptrSrcStream.Release();

	if(FAILED(hr))
	{
		return hr;
	}
	ASSERT(ptrBmpStream);
	if(m_hBmp)
	{
		_DSM_Free(m_hBmp);
	}

	m_nBmpSize = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * GetColorUsed() + GetImageSize();
	m_hBmp = _DSM_Alloc(m_nBmpSize);
	if(!m_hBmp)
	{
		LOG_ERR(_T("Low Memory:") << m_nBmpSize);
		hr = E_OUTOFMEMORY;
	}
	else
	{
		LPBITMAPINFOHEADER lpBih = (LPBITMAPINFOHEADER)_DSM_LockMemory(m_hBmp);
		if(!lpBih)
		{
			LOG_ERR(_T("LockMemory error!"));
		}
		else
		{
			// get bitmap data
			LARGE_INTEGER liOffset = { sizeof(BITMAPFILEHEADER) };
			hr = ptrBmpStream->Seek(liOffset, STREAM_SEEK_SET, NULL);
			if(FAILED(hr))
			{
				LOG_ERR(_T("IStream::Seek error: ") << HEX8(hr));
			}
			else
			{
				ULONG ulVal(0);
				hr = ptrBmpStream->Read(lpBih, m_nBmpSize, &ulVal);
				if(FAILED(hr))
				{
					LOG_ERR(_T("IStream::Read error: ") << HEX8(hr));
				}
				else if(ulVal != m_nBmpSize)
				{
					hr = E_FAIL;
					LOG_ERR(_T("Read bitmap data error!"));
				}
				else
				{
					lpBih->biSizeImage = GetImageSize();
					_DSM_UnlockMemory(m_hBmp);
					return S_OK;
				}
			}
			_DSM_UnlockMemory(m_hBmp);
		}
		_DSM_Free(m_hBmp);
		m_hBmp = NULL;
	}
	return hr;
}

BOOL CImgVtr::GetImageInfo(TW_IMAGEINFO& twImgInfo)
{
	if(IsValid())
	{
		ZeroMemory(&twImgInfo, sizeof(twImgInfo));

		twImgInfo.ImageWidth	= GetWidth();
		twImgInfo.ImageLength	= GetHeight();

		twImgInfo.XResolution	= FloatToFIX32(LONG2FLOAT(m_biXdpi));
		twImgInfo.YResolution	= FloatToFIX32(LONG2FLOAT(m_biYdpi));

		twImgInfo.Planar		= FALSE;
		twImgInfo.Compression	= TWCP_NONE;

		switch(GetBitCount())
		{
		case 24:
			{
				twImgInfo.PixelType			= TWPT_RGB;
				twImgInfo.BitsPerPixel		= 24;
				twImgInfo.SamplesPerPixel	= 3;
				twImgInfo.BitsPerSample[0]	= 8;
				twImgInfo.BitsPerSample[1]	= 8;
				twImgInfo.BitsPerSample[2]	= 8;
				return TRUE;
			}
		case 8:
			{
				twImgInfo.PixelType			= TWPT_GRAY;
				twImgInfo.BitsPerPixel		= 8;
				twImgInfo.SamplesPerPixel	= 1;
				twImgInfo.BitsPerSample[0]	= 8;
				return TRUE;
			}
		case 1:
			{
				twImgInfo.PixelType			= TWPT_BW;
				twImgInfo.BitsPerPixel		= 1;
				twImgInfo.SamplesPerPixel	= 1;
				twImgInfo.BitsPerSample[0]	= 1;
				return TRUE;
			}
		default: break;
		}
	}

	return FALSE;
}

HRESULT CImgVtr::GetNativeData(TW_HANDLE& hImageData)
{
	if(!m_hBmp)
	{
		HRESULT hr = CreateNativeData();
		if(S_OK != hr)
		{
			return hr;
		}
	}

	hImageData = m_hBmp;
	m_hBmp = NULL;
	if(!hImageData)
	{
		return E_FAIL;
	}
	return S_OK;
}

DWORD CImgVtr::GetMemoryData(LPBYTE lpBuffer, DWORD nBuffLen, DWORD nBeginLine, BOOL bLsb)
{
	LPBYTE lpDib = (LPBYTE)_DSM_LockMemory(m_hBmp);
	if(!lpDib)
	{
		LOG_ERR(_T("Failed to lock memory!"));
		return 0;
	}
	LPBITMAPINFOHEADER lpBih = (LPBITMAPINFOHEADER)lpDib;
	lpDib += sizeof(BITMAPINFOHEADER);
	lpDib += sizeof(RGBQUAD) * lpBih->biClrUsed;

	DWORD nBytesPerLine	= GetBytesPerLine();
	DWORD nReadLine	= nBuffLen / nBytesPerLine;
	DWORD nEndLine	= nBeginLine + nReadLine;
	if(nEndLine >= (DWORD)GetHeight())
	{
		nEndLine	= GetHeight() - 1;
		nReadLine	= GetHeight() - nBeginLine;
	}

	LPBYTE lpSrcData = lpDib + nBytesPerLine * (GetHeight() - nBeginLine - 1);
	if(bLsb && 24 == GetBitCount())
	{
		for(; nBeginLine <= nEndLine; nBeginLine++)
		{
			// BGR to RGB
			for(DWORD nOffset(0); nOffset < nBytesPerLine; nOffset += 3)
			{
				lpBuffer[nOffset]		= lpSrcData[nOffset + 2];
				lpBuffer[nOffset + 1]	= lpSrcData[nOffset + 1];
				lpBuffer[nOffset + 2]	= lpSrcData[nOffset];
			}
			lpBuffer	+= nBytesPerLine;
			lpSrcData	-= nBytesPerLine;
		}
	}
	else
	{
		for(; nBeginLine <= nEndLine; nBeginLine++)
		{
			CopyMemory(lpBuffer, lpSrcData, nBytesPerLine);
			lpBuffer	+= nBytesPerLine;
			lpSrcData	-= nBytesPerLine;
		}
	}
	_DSM_UnlockMemory(m_hBmp);
	return nReadLine;
}

HRESULT CImgVtr::SaveAsBMP(LPCTSTR lpszFileName)
{
	HRESULT hr(S_OK);

	if(!m_hBmp)
	{
		hr = CreateNativeData();
		if(S_OK != hr)
		{
			return hr;
		}
	}

	LPBITMAPINFOHEADER lpbih = (LPBITMAPINFOHEADER)_DSM_LockMemory(m_hBmp);
	if(!lpbih)
	{
		return E_FAIL;
	}

	HANDLE hFile = CreateFile(lpszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(INVALID_HANDLE_VALUE == hFile)
	{
		DWORD dwErr = GetLastError();
		if(dwErr)
		{
			LOG_ERR(_T("CreateFile error: ") << dwErr);
			hr = HRESULT_FROM_WIN32(dwErr);
		}
		else
		{
			LOG_ERR(_T("Failed to create bitmap file!"));
			hr = E_FAIL;
		}
	}
	else
	{
		BITMAPFILEHEADER bfh = { MAKEWORD('B', 'M') };
		bfh.bfSize		= sizeof(bfh) + m_nBmpSize;
		bfh.bfOffBits	= sizeof(bfh) + lpbih->biSize + sizeof(RGBQUAD) * lpbih->biClrUsed;

		DWORD nBytes(0);
		if(!WriteFile(hFile, &bfh, sizeof(bfh), &nBytes, NULL) || sizeof(bfh) != nBytes)
		{
			DWORD dwErr = GetLastError();
			if(dwErr)
			{
				LOG_ERR(_T("WriteFile error: ") << dwErr);
				hr = HRESULT_FROM_WIN32(dwErr);
			}
			else
			{
				LOG_ERR(_T("Failed to write bitmap file header!"));
				hr = E_FAIL;
			}
		}
		else if(!WriteFile(hFile, lpbih, m_nBmpSize, &nBytes, NULL) || nBytes != m_nBmpSize)
		{
			DWORD dwErr = GetLastError();
			if(dwErr)
			{
				LOG_ERR(_T("WriteFile error: ") << dwErr);
				hr = HRESULT_FROM_WIN32(dwErr);
			}
			else
			{
				LOG_ERR(_T("Failed to write bitmap data!"));
				hr = E_FAIL;
			}
		}

		CloseHandle(hFile);
	}
	_DSM_UnlockMemory(m_hBmp);

	return hr;
}

HRESULT CImgVtr::SaveAsJPEG(LPCTSTR lpszFileName)
{
	if(!m_hGlbJpeg)
	{
		return E_FAIL;
	}

	LPVOID lpData = GlobalLock(m_hGlbJpeg);
	if(!lpData)
	{
		return E_FAIL;
	}

	HRESULT hr(S_OK);
	HANDLE hFile = CreateFile(lpszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(INVALID_HANDLE_VALUE == hFile)
	{
		DWORD dwErr = GetLastError();
		if(dwErr)
		{
			LOG_ERR(_T("CreateFile error: ") << dwErr);
			hr = HRESULT_FROM_WIN32(dwErr);
		}
		else
		{
			LOG_ERR(_T("Failed to create jpeg file!"));
			hr = E_FAIL;
		}
	}
	else
	{
		DWORD nBytes(0);
		if(!WriteFile(hFile, lpData, m_nJpgSize, &nBytes, NULL) || nBytes != m_nJpgSize)
		{
			DWORD dwErr = GetLastError();
			if(dwErr)
			{
				LOG_ERR(_T("WriteFile error: ") << dwErr);
				hr = HRESULT_FROM_WIN32(dwErr);
			}
			else
			{
				LOG_ERR(_T("Failed to write jpeg data!"));
				hr = E_FAIL;
			}
		}
		CloseHandle(hFile);
	}
	GlobalUnlock(m_hGlbJpeg);
	return hr;
}
