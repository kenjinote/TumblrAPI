#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "wininet")
#pragma comment(lib, "uxtheme")

#include <windows.h>
#include <wininet.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>
#include <string>
#include "json11.hpp"
#include "resource.h"

TCHAR szClassName[] = TEXT("Window");

BOOL GetPosts(LPCWSTR lpszConsumerKey, LPCWSTR lpszUserName, int nLimitPages, BOOL bPhoto, BOOL bVideo, HWND hEdit)
{
	HINTERNET hInternet = InternetOpenW(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, INTERNET_FLAG_NO_COOKIES);
	if (hInternet == NULL)
	{
		return FALSE;
	}
	HINTERNET hSession = InternetConnectW(hInternet, L"api.tumblr.com", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	if (hSession == NULL)
	{
		InternetCloseHandle(hInternet);
		return FALSE;
	}
	for (int i = 0; i < nLimitPages; i++)
	{
		std::wstring path;
		path += L"/v2/blog/";
		path += lpszUserName;
		path += L"/posts?api_key=";
		path += lpszConsumerKey;
		path += L"&offset=";
		{
			WCHAR szBuffer[256];
			wsprintf(szBuffer, L"%d", i * 20);
			path += szBuffer;
		}
		HINTERNET hRequest = HttpOpenRequestW(hSession, L"GET", path.c_str(), NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
		if (hRequest == NULL)
		{
			break;
		}
		if (!HttpSendRequestW(hRequest, 0, 0, 0, 0))
		{
			InternetCloseHandle(hRequest);
			break;
		}
		WCHAR szBuffer[256] = { 0 };
		DWORD dwBufferSize = _countof(szBuffer);
		HttpQueryInfoW(hRequest, HTTP_QUERY_CONTENT_LENGTH, szBuffer, &dwBufferSize, NULL);
		DWORD dwContentLength = _wtol(szBuffer);
		LPBYTE lpByte = (LPBYTE)GlobalAlloc(0, dwContentLength + 1);
		DWORD dwReadSize;
		InternetReadFile(hRequest, lpByte, dwContentLength, &dwReadSize);
		lpByte[dwReadSize] = 0;
		BOOL bBreak = FALSE;
		{
			std::string src((LPSTR)lpByte);
			std::string err;
			json11::Json json = json11::Json::parse(src, err);
			if (err.size() == 0)
			{
				json11::Json meta = json["meta"];
				if (meta["msg"].string_value() == "OK")
				{
					json11::Json response = json["response"];
					json11::Json posts = response["posts"];
					if (posts.is_array())
					{
						if (posts.array_items().size() == 0)
						{
							bBreak = TRUE;
						}
						else
						{
							for (auto post : posts.array_items())
							{
								if (post["type"].string_value() == "photo")
								{
									if (bPhoto)
									{
										auto photos = post["photos"];
										for (auto photo : photos.array_items())
										{
											json11::Json original_size = photo["original_size"];
											std::string url = original_size["url"].string_value();
											if (url.size())
											{
												WCHAR szMediaURL[1024];
												MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, szMediaURL, _countof(szMediaURL));
												SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)szMediaURL);
												SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)L"\r\n");
											}
										}
									}
								}
								else if (post["type"].string_value() == "video")
								{
									if (bVideo)
									{
										std::string  video_url = post["video_url"].string_value();
										if (video_url.size())
										{
											WCHAR szMediaURL[1024];
											MultiByteToWideChar(CP_UTF8, 0, video_url.c_str(), -1, szMediaURL, _countof(szMediaURL));
											SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)szMediaURL);
											SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)L"\r\n");
										}
									}
								}
							}
						}
					}
					else
					{
						bBreak = TRUE;
					}
				}
				else
				{
					bBreak = TRUE;
				}
			}
			else
			{
				bBreak = TRUE;
			}
		}
		GlobalFree(lpByte);
		InternetCloseHandle(hRequest);
		if (bBreak)
		{
			break;
		}
	}
	InternetCloseHandle(hSession);
	InternetCloseHandle(hInternet);
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hButton;
	static HWND hEdit1;
	static HWND hEdit2;
	static HWND hEdit3;
	static HWND hEdit4;
	static HWND hCheck1;
	static HWND hCheck2;
	static HFONT hFont;
	static DOUBLE dControlHeight = 32.0;
	switch (msg)
	{
	case WM_CREATE:
		{
			HTHEME hTheme = OpenThemeData(hWnd, VSCLASS_AEROWIZARD);
			LOGFONT lf = { 0 };
			GetThemeFont(hTheme, NULL, AW_HEADERAREA, 0, TMT_FONT, &lf);
			hFont = CreateFontIndirectW(&lf);
			dControlHeight = abs(lf.lfHeight * 1.8);
			CloseThemeData(hTheme);
		}
		hEdit1 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT("[ConsumerKey]"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 1024, 32, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit1, WM_SETFONT, (WPARAM)hFont, 0);
		hEdit2 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT("[UserName]"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 1024, 32, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit2, WM_SETFONT, (WPARAM)hFont, 0);
		hEdit3 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT("100"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER, 0, 0, 1024, 32, hWnd, (HMENU)1000, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit3, WM_SETFONT, (WPARAM)hFont, 0);
		hButton = CreateWindow(TEXT("BUTTON"), TEXT("取得"), WS_VISIBLE | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, (HMENU)IDOK, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, 0);
		hCheck1 = CreateWindow(TEXT("BUTTON"), TEXT("画像"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hCheck1, BM_SETCHECK, 1, 0);
		SendMessage(hCheck1, WM_SETFONT, (WPARAM)hFont, 0);
		hCheck2 = CreateWindow(TEXT("BUTTON"), TEXT("動画"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hCheck2, BM_SETCHECK, 1, 0);
		SendMessage(hCheck2, WM_SETFONT, (WPARAM)hFont, 0);
		hEdit4 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_READONLY, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit4, EM_LIMITTEXT, 0, 0);
		SendMessage(hEdit4, WM_SETFONT, (WPARAM)hFont, 0);
		SendMessage(hEdit2, EM_SETSEL, 0, -1);
		SetFocus(hEdit2);
		break;
	case WM_SIZE:
		{
			int nMargin = (int)(dControlHeight / 3.0);
			MoveWindow(hEdit1, nMargin, nMargin, LOWORD(lParam) - nMargin * 2, (int)dControlHeight, TRUE);
			MoveWindow(hEdit2, nMargin, (int)dControlHeight * 1 + nMargin * 2, LOWORD(lParam) - nMargin * 2, (int)dControlHeight, TRUE);
			MoveWindow(hEdit3, nMargin, (int)dControlHeight * 2 + nMargin * 3, LOWORD(lParam) - nMargin * 2, (int)dControlHeight, TRUE);
			MoveWindow(hButton, nMargin, (int)dControlHeight * 3 + nMargin * 4, 256, (int)dControlHeight, TRUE);
			MoveWindow(hCheck1, 256 + nMargin * 2, (int)dControlHeight * 3 + nMargin * 4, 100, (int)dControlHeight, TRUE);
			MoveWindow(hCheck2, 256 + 100 + nMargin * 2, (int)dControlHeight * 3 + nMargin * 4, 100, (int)dControlHeight, TRUE);
			MoveWindow(hEdit4, nMargin, (int)dControlHeight * 4 + nMargin * 5, LOWORD(lParam) - nMargin * 2, HIWORD(lParam) - ((int)dControlHeight * 4 + nMargin * 6), TRUE);
		}
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			SetWindowText(hEdit4, 0);
			TCHAR szConsumerKey[256];
			GetWindowText(hEdit1, szConsumerKey, _countof(szConsumerKey));
			TCHAR szUserName[256];
			GetWindowText(hEdit2, szUserName, _countof(szUserName));
			GetPosts(szConsumerKey, szUserName, GetDlgItemInt(hWnd, 1000, 0, 0), (BOOL)SendMessage(hCheck1,BM_GETCHECK, 0,0), (BOOL)SendMessage(hCheck2, BM_GETCHECK, 0, 0), hEdit4);
			SendMessage(hEdit4, EM_SETSEL, 0, -1);
			SetFocus(hEdit4);
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		DeleteObject(hFont);
		PostQuitMessage(0);
		break;
	default:
		return DefDlgProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		0,
		WndProc,
		0,
		DLGWINDOWEXTRA,
		hInstance,
		LoadIcon(hInstance,(LPCTSTR)IDI_ICON1),
		LoadCursor(0,IDC_ARROW),
		0,
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("Tumblr API を使って投稿に含まれる画像/動画の URL を取得"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	ACCEL Accel[] = { { FVIRTKEY,VK_F5,IDOK } };
	HACCEL hAccel = CreateAcceleratorTable(Accel, _countof(Accel));
	while (GetMessage(&msg, 0, 0, 0))
	{
		if (!TranslateAccelerator(hWnd, hAccel, &msg) && !IsDialogMessage(hWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	DestroyAcceleratorTable(hAccel);
	return (int)msg.wParam;
}
