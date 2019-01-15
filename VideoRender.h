#pragma once

#if defined(_WIN32) || defined(_WIN64)
//#define EABLE_FONT

//#include "log.h"
#include <cstdint>
#include <sstream>
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")

#ifdef EABLE_FONT
#include <d3dx9.h>
#pragma comment(lib, "d3dx9.lib")
#endif

class VideoRender
{
public:
	VideoRender(HWND hWnd, LONG nWidth, LONG nHeight, std::uint32_t dwFourCC)
	{
		HMONITOR hWndMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

		m_hWnd = hWnd;
		m_nWidth = nWidth;
		m_nHeight = nHeight;
		m_dwFourCC = dwFourCC;

		m_pDirect3D9 = NULL;
		HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pDirect3D9);
		if (FAILED(hr))
		{
			return;
		}
		m_AdapterCount = m_pDirect3D9->GetAdapterCount();
		LONG nAdapter = D3DADAPTER_DEFAULT;
		for (int i = 0; i < m_AdapterCount; ++i)
		{
			HMONITOR hD3DMonitor = m_pDirect3D9->GetAdapterMonitor(i);
			if (hWndMonitor == hD3DMonitor)
			{
				nAdapter = i;
				std::stringstream ss;
				ss << "[CHEN] Wnd=" << hWnd << " -> Adapter=" << nAdapter;
				OutputDebugStringA(ss.str().c_str());
				printf("%s\n", ss.str().c_str());
				break;
			}
			else
			{
				nAdapter = i;
				std::stringstream ss;
				ss << "[CHEN] Wnd=" << hWnd << " xxx Adapter=" << nAdapter;
				OutputDebugStringA(ss.str().c_str());
				printf("%s\n", ss.str().c_str());
			}
		}

		RECT rc = { 0 };
		GetClientRect(hWnd, &rc);
		int w = rc.right - rc.left;
		int h = rc.bottom - rc.top;

		for (int x = 0; x < 1; ++x)
		{
			D3DDISPLAYMODE mode = { 0 };
			m_pDirect3D9->GetAdapterDisplayMode(nAdapter, &mode);
			D3DCAPS9 caps;
			m_pDirect3D9->GetDeviceCaps(nAdapter, D3DDEVTYPE_HAL, &caps);
			FillD3DPP(hWnd, m_D3DPP);
			m_D3DPP.BackBufferFormat = mode.Format;
			//m_D3DPP.Windowed = !(mode.Width == w && mode.Height == h) ? TRUE : FALSE;
			std::stringstream ss;
			ss << "[CHEN] Wnd=" << hWnd << " Windowed=" << m_D3DPP.Windowed;
			OutputDebugStringA(ss.str().c_str());
			printf("%s\n", ss.str().c_str());

			DWORD BehaviorFlags = 0;
			if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
			{ 
				BehaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
			}
			else
			{
				BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
			}

			hr = m_pDirect3D9->CreateDeviceEx(nAdapter/*D3DADAPTER_DEFAULT*/, D3DDEVTYPE_HAL, hWnd,
				BehaviorFlags,
				&m_D3DPP, NULL, &m_pDirect3DDevice);
			if (FAILED(hr))
			{
				continue;
			}

			// 目前发现在个别独显下不支持I420/J420，所以统一使用YV12方式，当要绘制时，作相应转换
			if (dwFourCC == MAKEFOURCC('I', '4', '2', '0') || dwFourCC == D3DFORMAT(MAKEFOURCC('J', '4', '2', '0')))
			{
				dwFourCC = MAKEFOURCC('Y', 'V', '1', '2');
			}

			hr = m_pDirect3DDevice->CreateOffscreenPlainSurface(nWidth, nHeight,
				(D3DFORMAT)dwFourCC, //(D3DFORMAT)MAKEFOURCC('N', 'V', '1', '2'),
				D3DPOOL_DEFAULT,
				&m_pDirect3DSurfaceRender,
				NULL);
			if (FAILED(hr))
			{
				if (hr == D3DERR_INVALIDCALL)
				{
					printf("D3DERR_INVALIDCALL\n");
				}
				m_pDirect3DDevice->Release();
				m_pDirect3DDevice = NULL;
				continue;
			}

			if (SUCCEEDED(hr))
			{
				D3DLOCKED_RECT d3d_rect;
				hr = m_pDirect3DSurfaceRender->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
				if (SUCCEEDED(hr))
				{
					BYTE * yd = (BYTE *)d3d_rect.pBits;
					BYTE * xd = yd + d3d_rect.Pitch * m_nHeight;
					memset(yd, 0, m_nHeight * d3d_rect.Pitch);
					memset(xd, 128, m_nHeight / 2 * d3d_rect.Pitch);
					hr = m_pDirect3DSurfaceRender->UnlockRect();
				}
				m_nAdapterUsed = nAdapter;
				printf("[VR] Adapter=%d\n", m_nAdapterUsed);
				break;
			}
		}

		if (NULL == m_pDirect3DDevice)
		{
			FillD3DPP(hWnd, m_D3DPP);
			hr = m_pDirect3D9->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
				D3DCREATE_SOFTWARE_VERTEXPROCESSING,
				&m_D3DPP, NULL, &m_pDirect3DDevice);
			if (FAILED(hr))
			{
				return;
			}
		}

#ifdef EABLE_FONT
		//填充D3DXFONT_DESC结构
		int font_width  = double(m_nWidth) / 1920 * 20;// 20;// m_nWidth / 60;
		int font_height = double(m_nHeight) / 1080 * 50;// font_width + 30;// m_nHeight / 60;

		D3DXFONT_DESC df;
		ZeroMemory(&df, sizeof(D3DXFONT_DESC));
		df.Height = font_height;
		df.Width = font_width;
		df.MipLevels = D3DX_DEFAULT;
		df.Italic = false;
		df.CharSet = DEFAULT_CHARSET;
		df.OutputPrecision = 0;
		df.Quality = 0;
		df.PitchAndFamily = 0;
		strcpy_s(df.FaceName, "Times New Roman");
		//创建ID3DXFont对象  
		hr = D3DXCreateFontIndirectA(m_pDirect3DDevice, &df, &m_pDirect3DFont);
		if (FAILED(hr))
		{
			printf("D3DXCreateFontIndirect FAILED!\n");
		}
#endif
	}

	~VideoRender()
	{
#ifdef EABLE_FONT
		if (m_pDirect3DFont)
		{
			m_pDirect3DFont->Release();
		}
#endif
		if (m_pDirect3DSurfaceRender)
		{
			m_pDirect3DSurfaceRender->Release();
		}
		if (m_pDirect3DDevice)
		{
			m_pDirect3DDevice->Release();
		}
		if (m_pDirect3D9)
		{
			m_pDirect3D9->Release();
		}
	}

	int SnapShot(std::string pic)
	{
		if (!m_pDirect3DDevice)
		{
			return -1;
		}

#ifdef EABLE_FONT
		m_pDirect3DDevice->BeginScene();
		IDirect3DSurface9 * pBackBuffer = NULL;

		m_pDirect3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
		m_pDirect3DDevice->StretchRect(m_pDirect3DSurfaceRender, NULL, pBackBuffer, NULL, D3DTEXF_LINEAR);

		m_pDirect3DDevice->EndScene();
		HRESULT hr = m_pDirect3DDevice->Present(NULL, NULL, NULL, NULL);
		if (pBackBuffer)
		{
			hr = D3DXSaveSurfaceToFileA(pic.c_str(), _D3DXIMAGE_FILEFORMAT::D3DXIFF_JPG, pBackBuffer, NULL, NULL);
			pBackBuffer->Release();
			if (SUCCEEDED(hr))
			{
				return 0;
			}
		}
#endif

		return -2;
	}

#if 0
	static int Render(IDirect3DSurface9 * surface, HWND hWnd, LONG img_w, LONG img_h)
	{
		RECT rc = { 0 };
		GetClientRect(hWnd, &rc);
		int w = rc.right - rc.left;
		int h = rc.bottom - rc.top;
		if (w <= 0 || h <= 0)
		{
			//OutputDebugStringA("[CHEN] w <= 0 || h <= 0");
			return 0;
		}

		if (!surface)
		{
			return -1112;
		}

		IDirect3DDevice9 * device = NULL;
		surface->GetDevice(&device);
		if (!device)
		{
			return -2223;
		}

		/*D3DVIEWPORT9 view = { 0 };
		device->GetViewport(&view);
		if (view.Width != img_w || view.Height != img_h)
		{
			D3DPRESENT_PARAMETERS D3DPP = { 0 };
			D3DPP.Windowed = TRUE;
			D3DPP.hDeviceWindow = hWnd;

			D3DPP.Flags = D3DPRESENTFLAG_VIDEO;
			D3DPP.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
			D3DPP.BackBufferCount = 1;
			D3DPP.BackBufferFormat = D3DFMT_UNKNOWN;

			RECT r = { 0, 0, img_w, img_h };
			D3DPP.BackBufferWidth = r.right - r.left;
			D3DPP.BackBufferHeight = r.bottom - r.top;

			D3DPP.SwapEffect = D3DSWAPEFFECT_DISCARD;
			device->Reset(&D3DPP);
		}*/
		
		HRESULT hr = device->BeginScene();
		if (FAILED(hr))
		{
			return -34324;
		}

		IDirect3DSurface9 * pBackBuffer = NULL;

		hr = device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
		if (SUCCEEDED(hr))
		{
			RECT rc = { 0, 0, img_w, img_h };
			device->StretchRect(surface, &rc, pBackBuffer, NULL, D3DTEXF_LINEAR);
			pBackBuffer->Release();
		}
		else
		{

		}

		device->EndScene();
		if (SUCCEEDED(hr))
		{
			hr = device->Present(NULL, NULL, hWnd, NULL);
		}
		device->Release();
		if (FAILED(hr))
		{
			return -1;
		}

		return 0;
	}
#endif

	int Render(BYTE * pData[], int nLineSize[], std::string text = "", int text_x = 10, int text_y = 10, std::uint32_t text_color = D3DCOLOR_ARGB(140, 255, 255, 255), std::uint32_t format = DT_CENTER | DT_VCENTER/*DT_TOP | DT_LEFT*/)
	{
		RECT rc = { 0 };
		GetClientRect(m_hWnd, &rc);
		int w = rc.right - rc.left;
		int h = rc.bottom - rc.top;
		if (w <= 0 || h <= 0)
		{
			//OutputDebugStringA("[CHEN] w <= 0 || h <= 0");
			return 0;
		}

		std::int64_t bt = GetTickCount();

#ifdef EABLE_FONT
		extern bool g_show_text;
		if (!g_show_text)
		{
			text.clear();
		}
#endif

		if (m_pDirect3DSurfaceRender == NULL)
		{
			ShowText("NO SURFACE!");
			return -1;
		}

		if (m_pDirect3DDevice == NULL)
		{
			ShowText("NO D3D DEVICE!");
			return -2;
		}

		FillFrame(pData, nLineSize);

		//m_pDirect3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
		if (w > 0 && h > 0)
		{
			m_pDirect3DDevice->BeginScene();
			IDirect3DSurface9 * pBackBuffer = NULL;

			m_pDirect3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
			m_pDirect3DDevice->StretchRect(m_pDirect3DSurfaceRender, NULL, pBackBuffer, NULL, D3DTEXF_LINEAR);
			pBackBuffer->Release();

#ifdef EABLE_FONT
			if (!text.empty())
			{
				RECT rect = { 0, 0, m_nWidth, m_nHeight };
				//GetClientRect(m_hWnd, &rect);
				rect.left = text_x;
				rect.top = text_y;

				int e = m_pDirect3DFont->DrawTextA(NULL, text.c_str(), -1, &rect, format, text_color);
			}
#endif

			m_pDirect3DDevice->EndScene();
			HRESULT hr = m_pDirect3DDevice->Present(NULL, NULL, NULL, NULL);
			if (FAILED(hr))
			{
				ShowText("Present FAILED!");
			}
		}

		std::int64_t et = GetTickCount();


		//printf("Win=%dx%d. Render=%lld\n", w, h, et-bt);



		return 0;
	}

	int ReRender(std::string text = "", int text_x = 10, int text_y = 10, std::uint32_t text_color = D3DCOLOR_ARGB(140, 255, 255, 255), std::uint32_t format = DT_CENTER | DT_VCENTER/*DT_TOP | DT_LEFT*/)
	{
		RECT rc = { 0 };
		GetClientRect(m_hWnd, &rc);
		int w = rc.right - rc.left;
		int h = rc.bottom - rc.top;
		if (w <= 0 || h <= 0)
		{
			return 0;
		}

		extern bool g_show_text;
		if (!g_show_text)
		{
			text.clear();
		}

		if (m_pDirect3DSurfaceRender == NULL)
		{
			return -1;
		}

		//if (!UpdateDevice())
		//{
		//	return -2;
		//}

		if (m_pDirect3DDevice == NULL)
		{
			return -4;
		}

		//m_pDirect3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

		m_pDirect3DDevice->BeginScene();
		IDirect3DSurface9 * pBackBuffer = NULL;

		m_pDirect3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
		m_pDirect3DDevice->StretchRect(m_pDirect3DSurfaceRender, NULL, pBackBuffer, NULL, D3DTEXF_LINEAR);
		pBackBuffer->Release();

#ifdef EABLE_FONT
		if (!text.empty())
		{
			RECT rect = { 0, 0, m_nWidth, m_nHeight };
			//GetClientRect(m_hWnd, &rect);
			rect.left = text_x;
			rect.top = text_y;

			int e = m_pDirect3DFont->DrawTextA(NULL, text.c_str(), -1, &rect, format, text_color);
		}
#endif

		m_pDirect3DDevice->EndScene();
		HRESULT hr = m_pDirect3DDevice->Present(NULL, NULL, NULL, NULL);
		if (hr == D3DERR_DEVICELOST)
		{
			return -333;
		}
		return 0;
	}

	int Clear()
	{
		if (!m_pDirect3DDevice)
		{
			return -1;
		}

		HRESULT hr = m_pDirect3DDevice->Clear(0, 0, D3DCLEAR_TARGET, 0xff000000, 1.0f, 0);
		if (FAILED(hr))
		{
			return -2;
		}

		m_pDirect3DDevice->Present(NULL, NULL, NULL, NULL);

		return 0;
	}

	int ShowText(std::string text, int x = 0, int y = 0, std::uint32_t color = D3DCOLOR_ARGB(140, 255, 255, 255), std::uint32_t format = DT_CENTER | DT_VCENTER/*DT_TOP | DT_LEFT*/)
	{
#ifdef EABLE_FONT
		RECT rc = { 0 };
		GetClientRect(m_hWnd, &rc);
		int w = rc.right - rc.left;
		int h = rc.bottom - rc.top;
		if (w <= 0 || h <= 0)
		{
			return 0;
		}

		extern bool g_show_text;
		if (!g_show_text)
		{
			text.clear();
			return 0;
		}

		if (!m_pDirect3DDevice)
		{
			return -1;
		}

		HRESULT hr = m_pDirect3DDevice->BeginScene();
		if (FAILED(hr))
		{
			return -2;
		}

		IDirect3DSurface9 * pBackBuffer = NULL;

		hr = m_pDirect3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
		if (SUCCEEDED(hr))
		{
			hr = m_pDirect3DDevice->StretchRect(m_pDirect3DSurfaceRender, NULL, pBackBuffer, NULL, D3DTEXF_LINEAR);
			pBackBuffer->Release();
		}	

		int e = 0;
		if (!text.empty())
		{
			RECT rect = { 0, 0, m_nWidth, m_nHeight };
			//GetClientRect(m_hWnd, &rect);
			rect.left = x;
			rect.top = y;

			e = m_pDirect3DFont->DrawTextA(NULL, text.c_str(), -1, &rect, format, color);
		}

		m_pDirect3DDevice->EndScene();
		m_pDirect3DDevice->Present(0, 0, 0, 0);
		////m_pDirect3DDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
		//m_pDirect3DDevice->BeginScene();

		//RECT rect = { 0, 0, m_nWidth, m_nHeight };
		////GetClientRect(m_hWnd, &rect);
		//rect.left = x;
		//rect.top  = y;
		//
		//int e = m_pDirect3DFont->DrawTextA(NULL, text.c_str(), -1, &rect, format, color);
		//m_pDirect3DDevice->EndScene();
		//m_pDirect3DDevice->Present(0, 0, 0, 0);

		return e;
#endif
		return 0;
	}

	int Width()
	{
		return m_nWidth;
	}

	int Height()
	{
		return m_nHeight;
	}

	std::uint32_t FourCC()
	{
		return m_dwFourCC;
	}

private:

	int FillFrame(BYTE * pData[], int nLineSize[])
	{
		if (!pData)
		{
			return -1;
		}

		if (!m_pDirect3DSurfaceRender)
		{
			return -2;
		}

		D3DSURFACE_DESC desc;
		HRESULT hr = m_pDirect3DSurfaceRender->GetDesc(&desc);
		if (FAILED(hr))
		{
			return -3;
		}

		if (desc.Height <= 0)
		{
			return -4;
		}

		D3DLOCKED_RECT d3d_rect;
		hr = m_pDirect3DSurfaceRender->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
		if (FAILED(hr))
		{
			//ShowText("LockRect FAILED!");
			return -5;
		}

		const LONG MAX_BUF_NUM = 4;
		BYTE * dst_data[MAX_BUF_NUM] = { 0 };
		LONG   dst_line_size[MAX_BUF_NUM] = { 0 };
		LONG   line_num[MAX_BUF_NUM] = { 0 };
		memset(dst_data, 0, sizeof(dst_data));
		memset(dst_line_size, 0, sizeof(dst_line_size));
		memset(line_num, 0, sizeof(line_num));

		LONG height = (LONG(desc.Height) < m_nHeight) ? LONG(desc.Height) : m_nHeight;

		if (m_dwFourCC == D3DFORMAT(MAKEFOURCC('N', 'V', '1', '2')))
		{
			dst_data[0] = (BYTE *)d3d_rect.pBits;
			dst_line_size[0] = d3d_rect.Pitch;
			line_num[0] = height;

			dst_data[1] = ((BYTE *)d3d_rect.pBits) + d3d_rect.Pitch * height;
			dst_line_size[1] = d3d_rect.Pitch;

			line_num[1] = height >> 1;
		}
		else if (m_dwFourCC == D3DFORMAT(MAKEFOURCC('I', '4', '2', '0')) || m_dwFourCC == D3DFORMAT(MAKEFOURCC('J', '4', '2', '0')) || m_dwFourCC == D3DFORMAT(MAKEFOURCC('Y', 'V', '1', '2')))
		{
			dst_data[0] = (BYTE *)d3d_rect.pBits;
			dst_line_size[0] = d3d_rect.Pitch;
			line_num[0] = height;

			dst_data[1] = ((BYTE *)d3d_rect.pBits) + d3d_rect.Pitch * height;
			dst_line_size[1] = d3d_rect.Pitch >> 1;
			line_num[1] = height >> 1;

			dst_data[2] = ((BYTE *)d3d_rect.pBits) + d3d_rect.Pitch * height + (d3d_rect.Pitch >> 1) * (height >> 1);
			dst_line_size[2] = d3d_rect.Pitch >> 1;
			line_num[2] = height >> 1;

			if (m_dwFourCC == MAKEFOURCC('I', '4', '2', '0') || m_dwFourCC == D3DFORMAT(MAKEFOURCC('J', '4', '2', '0')))
			{
				std::swap(dst_data[1], dst_data[2]);
				std::swap(dst_line_size[1], dst_line_size[2]);
				std::swap(line_num[1], line_num[2]);
			}
		}

		for (int i = 0; i < MAX_BUF_NUM; ++i)
		{
			BYTE * src = pData[i];
			BYTE * dst = dst_data[i];

			for (int h = 0; h < line_num[i]; ++h)
			{
				LONG n = (dst_line_size[i] < nLineSize[i]) ? dst_line_size[i] : nLineSize[i];
				memcpy(dst, src, n);
				src += nLineSize[i];
				dst += dst_line_size[i];
			}
		}

		//int DHH = m_nHeight >> 1;
		//int DHW = d3d_rect.Pitch >> 1;

		//BYTE * ys = pData[0];
		//BYTE * yd = (BYTE *)d3d_rect.pBits;
		//BYTE * xs = pData[1];
		//BYTE * xd = yd + d3d_rect.Pitch * m_nHeight;

		//BYTE * us = NULL;
		//BYTE * ud = NULL;
		//BYTE * vs = NULL;
		//BYTE * vd = NULL;
		//
		//for (int h = 0; h < m_nHeight; ++h)
		//{
		//	LONG n = min(d3d_rect.Pitch, nLineSize[0]);
		//	memcpy(yd, ys, n);
		//	ys += nLineSize[0];
		//	yd += d3d_rect.Pitch;

		//	
		//	if (m_dwFourCC == D3DFORMAT(MAKEFOURCC('N', 'V', '1', '2')))
		//	{
		//		if (h < DHH)
		//		{
		//			n = min(d3d_rect.Pitch, nLineSize[1]);
		//			memcpy(xd, xs, n);
		//			xs += nLineSize[1];
		//			xd += d3d_rect.Pitch;
		//		}
		//	}
		//	else if (m_dwFourCC == D3DFORMAT(MAKEFOURCC('I', '4', '2', '0')) || m_dwFourCC == D3DFORMAT(MAKEFOURCC('Y', 'V', '1', '2')))
		//	{
		//		if (!us || !vs)
		//		{
		//			us = pData[1];
		//			ud = yd + d3d_rect.Pitch * m_nHeight;
		//			vs = pData[2];
		//			vd = ud + DHW * DHH;
		//		}

		//		if (us)
		//		{
		//			int x = min(DHW, nLineSize[1]);
		//			memcpy(ud, us, x);
		//			us += nLineSize[1];
		//			ud += DHW;
		//		}

		//		if (vs)
		//		{
		//			int x = min(DHW, nLineSize[2]);
		//			memcpy(vd, vs, x);
		//			vs += nLineSize[2];
		//			vd += DHW;
		//		}
		//	}
		//}

		hr = m_pDirect3DSurfaceRender->UnlockRect();

		return 0;
	}

	void FillD3DPP(HWND hWnd, D3DPRESENT_PARAMETERS& D3DPP)
	{
		//D3DPP.EnableAutoDepthStencil = false;
		//D3DPP.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

		D3DPP.Windowed = TRUE;
		D3DPP.hDeviceWindow = (HWND)hWnd;

		D3DPP.Flags = D3DPRESENTFLAG_VIDEO;
		D3DPP.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
		//D3DPP.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // note that this setting leads to an implicit timeBeginPeriod call
		D3DPP.BackBufferCount = 1;
		D3DPP.BackBufferFormat = D3DFMT_UNKNOWN;
		//D3DPP.BackBufferFormat = D3DFMT_X8R8G8B8;

		/*RECT rc = { 0 };
		GetClientRect(hWnd, &rc);
		LONG w = rc.right - rc.left;
		LONG h = rc.bottom - rc.top;
		printf("[VR] Init\n");

		if (w <= 0 || h <= 0)
		{
			w = 600;
			h = 400;
		}*/
		LONG w = m_nWidth;
		LONG h = m_nHeight;

		RECT r = { 0, 0, w, h };
		//GetClientRect((HWND)hWnd, &r);
		D3DPP.BackBufferWidth = r.right - r.left;
		D3DPP.BackBufferHeight = r.bottom - r.top;

		//
		// Mark the back buffer lockable if software DXVA2 could be used.
		// This is because software DXVA2 device requires a lockable render target
		// for the optimal performance.
		//

		//D3DPP.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

		D3DPP.SwapEffect = D3DSWAPEFFECT_DISCARD;
	}

private:
	enum { MAX_ADAPTER_COUNT = 64 };

	HWND					m_hWnd = NULL;
	LONG					m_nWidth = 0;
	LONG					m_nHeight = 0;
	std::uint32_t			m_dwFourCC = 0;
	LONG					m_AdapterCount = 0;
	D3DPRESENT_PARAMETERS	m_D3DPP = { 0 };
	IDirect3D9Ex *			m_pDirect3D9 = NULL;
	IDirect3DDevice9Ex *	m_pDirect3DDevice = NULL;
	IDirect3DSurface9 *		m_pDirect3DSurfaceRender = NULL;
#ifdef EABLE_FONT
	ID3DXFont *				m_pDirect3DFont = NULL;
#endif
	RECT					m_tWinRect = { 0 };
	LONG					m_nAdapterUsed = 0;
};

#else
struct VideoRender
{

};

#endif