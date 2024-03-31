#include "pch.h"
#include "Win32.h"

Win32* Win32::self = NULL;

Win32::Win32() {
	self = this;

	bits = NULL;
	dib  = NULL;
	info = {};

	window   = NULL;
	front_dc = NULL;
	 back_dc = NULL;
	instance = NULL;

	  resize_callback = NULL;
	    draw_callback = NULL;

	width  = 640;
	height = 480;
	rect = { 0, 0, (long)width, (long)height };
	color_depth = 32;
	fullscreen = true;
	cname = L"RPG";
	wname = L"RPG";
	cores = 1;

	memset(&msg, 0, sizeof(msg));
}

// This is a static method. This is my workaround to get it to play nicely within an instanced object.
LRESULT CALLBACK Win32::proc(HWND window, uint msg, WPARAM wparam, LPARAM lparam) {
	return self->_proc(window, msg, wparam, lparam);
}

LRESULT CALLBACK Win32::_proc(HWND window, uint msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
		case WM_SIZE: {
			short raw_w = LOWORD(lparam);
			short raw_h = HIWORD(lparam);

			uint w = raw_w <= 64 ? 64 : raw_w;
			uint h = raw_h <= 64 ? 64 : raw_h;

			if (resize(w, h)) {
				resize_callback(w, h);
			}
			return 0;
		}
		case WM_KEYDOWN: {
			uint mapped = map_key(wparam);
			key_press[mapped] = TRUE;

			return 0;
		}
		case WM_KEYUP: {
			uint mapped = map_key(wparam);
			key_press[mapped] = FALSE;

			return 0;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);

			return 0;
		}
	}

	return DefWindowProc(window, msg, wparam, lparam);
}

uint Win32::map_key(WPARAM wparam) {
	switch(wparam) {
		case VK_ESCAPE:     return 0x01;
		case 0x31:          return 0x02;
		case 0x32:          return 0x03;
		case 0x33:          return 0x04;
		case 0x34:          return 0x05;
		case 0x35:          return 0x06;
		case 0x36:          return 0x07;
		case 0x37:          return 0x08;
		case 0x38:          return 0x09;
		case 0x39:          return 0x0A;
		case 0x30:          return 0x0B;
		case VK_OEM_MINUS:  return 0x0C;
		case VK_OEM_PLUS:   return 0x0D;
		case VK_OEM_5:      return 0x0E;
		case VK_TAB:        return 0x0F;
		case 0x51:          return 0x10;
		case 0x57:          return 0x11;
		case 0x45:          return 0x12;
		case 0x52:          return 0x13;
		case 0x54:          return 0x14;
		case 0x59:          return 0x15;
		case 0x55:          return 0x16;
		case 0x49:          return 0x17;
		case 0x4F:          return 0x18;
		case 0x50:          return 0x19;
		case VK_OEM_4:      return 0x1A;
		case VK_OEM_6:      return 0x1B;
		case VK_RETURN:     return 0x1C;
		case VK_CONTROL:    return 0x1D;
		case 0x41:          return 0x1E;
		case 0x53:          return 0x1F;
		case 0x44:          return 0x20;
		case 0x46:          return 0x21;
		case 0x47:          return 0x22;
		case 0x48:          return 0x23;
		case 0x4A:          return 0x24;
		case 0x4B:          return 0x25;
		case 0x4C:          return 0x26;
		case VK_OEM_1:      return 0x27;
		case VK_OEM_7:      return 0x28;
		case VK_OEM_3:      return 0x29;
		case VK_LSHIFT:     return 0x2A;
		case VK_OEM_102:    return 0x2B;
		case 0x5A:          return 0x2C;
		case 0x58:          return 0x2D;
		case 0x43:          return 0x2E;
		case 0x56:          return 0x2F;
		case 0x42:          return 0x30;
		case 0x4E:          return 0x31;
		case 0x4D:          return 0x32;
		case VK_OEM_COMMA:  return 0x33;
		case VK_OEM_PERIOD: return 0x34;
		case VK_OEM_2:      return 0x35;
		case VK_RSHIFT:     return 0x36;
		case VK_MULTIPLY:   return 0x37;
		case VK_MENU:       return 0x38;
		case VK_SPACE:      return 0x39;
		case VK_CAPITAL:    return 0x3A;
		case VK_F1:         return 0x3B;
		case VK_F2:         return 0x3C;
		case VK_F3:         return 0x3D;
		case VK_F4:         return 0x3E;
		case VK_F5:         return 0x3F;
		case VK_F6:         return 0x40;
		case VK_F7:         return 0x41;
		case VK_F8:         return 0x42;
		case VK_F9:         return 0x43;
		case VK_F10:        return 0x44;
		case VK_NUMLOCK:    return 0x45;
		case VK_SCROLL:     return 0x46;
		case VK_HOME:       return 0x47;
		case VK_UP:         return 0x48;
		case VK_PRIOR:      return 0x49;
		case VK_LEFT:       return 0x4B;
		case VK_RIGHT:      return 0x4D;
		case VK_ADD:        return 0x4E;
		case VK_END:        return 0x4F;
		case VK_DOWN:       return 0x50;
		case VK_NEXT:       return 0x51;
		case VK_INSERT:     return 0x52;
		case VK_DELETE:     return 0x53;
		case VK_F11:        return 0x57;
		case VK_F12:        return 0x58;
		case VK_LWIN:       return 0x7D;
		case VK_RWIN:       return 0x7E;
		//case VK_MENU:     return 0x7F;
	}

	return 0;
}

bool Win32::init() {
	timeBeginPeriod(1);

	HWND console = GetConsoleWindow();
	ShowWindow(console, SW_SHOW); // SW_HIDE);

	if (!init_window()) {
		MessageBoxW(window, L"Cannot init window!", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	if (!init_buffer()) {
		MessageBoxW(window, L"Cannot init buffer!", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	ShowWindow(window, fullscreen ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
	UpdateWindow(window);

	SetFocus(window);
	if (!fullscreen) {
		SetForegroundWindow(window);
	}

	GetClientRect(window, &rect);

	return true;
}

bool Win32::init_window() {
	instance = (HINSTANCE)GetModuleHandleW(NULL);

	WNDCLASSEXW wc = {
		  sizeof(WNDCLASSEX)
		, CS_HREDRAW | CS_VREDRAW | CS_OWNDC
		, proc
		, 0
		, 0
		, instance
		, LoadIconW(instance, MAKEINTRESOURCE(IDI_RENDERER))
		, LoadCursorW(nullptr, IDC_ARROW)
		, (HBRUSH)(COLOR_WINDOW + 1)
		, MAKEINTRESOURCEW(IDC_RENDERER)
		, cname
		, LoadIconW(instance, MAKEINTRESOURCE(IDI_SMALL))
	};

	RegisterClassExW(&wc);

	fullscreen = IDYES == MessageBoxW(NULL, L"Click Yes to go to full screen (Recommended)", L"Options", MB_YESNO | MB_ICONQUESTION);

	ulong style, style_ex;
	if (fullscreen) {
		style = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		style_ex = 0;

		if (!full_screen()) { return false; }

		ShowCursor(false);
	} else {
		style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		style_ex = WS_EX_CLIENTEDGE;

		AdjustWindowRectEx(&rect, style, false, style_ex);
	}

	window = CreateWindowExW(style_ex, wc.lpszClassName, wname, style, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, wc.hInstance, NULL);
	if (!window) { return false; }

	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	cores = sysinfo.dwNumberOfProcessors;

	front_dc = GetDC(window);

	memset(&info, 0, sizeof(info));

	info.bmiHeader = {
		  sizeof(BITMAPINFOHEADER)
		,  (long)width
		, -(long)height
		, 1
		, color_depth
		, BI_RGB
		, width * height * 4UL
		, 0
		, 0
		, 0
		, 0
	};

	return true;
}

bool Win32::init_buffer() {
	info.bmiHeader.biWidth = (long)width;
	info.bmiHeader.biHeight = -(long)height;
	info.bmiHeader.biSizeImage = width * height * 4UL;

	dib = CreateDIBSection(front_dc, &info, DIB_RGB_COLORS, bits, NULL, 0);
	if (NULL == dib) { return false; }

	back_dc = CreateCompatibleDC(front_dc);
	if (NULL == back_dc) { return false; }

	SelectObject(back_dc, dib);

	return true;
}

void Win32::unload() {
	timeEndPeriod(1);

	if (fullscreen) {
		ChangeDisplaySettingsW(NULL, 0);
		ShowCursor(true);
	}

	unload_buffer();

	UnregisterClassW(cname, instance);

	GetConsoleWindow();
}

void Win32::unload_buffer() {
	if (NULL != back_dc) {
		DeleteDC(back_dc);
		back_dc = NULL;
	}

	if (NULL != dib) {
		DeleteObject(dib);
		dib = NULL;
	}
}

bool Win32::full_screen() {
	DEVMODE settings;
	memset(&settings, 0, sizeof(settings));

	if (!EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &settings)) {
		MessageBoxW(NULL, L"Could Not Enum Display Settings", L"Error", MB_OK);
		return false;
	}

	settings.dmPelsWidth = width;
	settings.dmPelsHeight = height;
	settings.dmColor = color_depth;

	int result = ChangeDisplaySettingsW(&settings, CDS_FULLSCREEN);

	if (DISP_CHANGE_SUCCESSFUL != result) {
		MessageBoxW(NULL, L"Display Mode Not Compatible", L"Error", MB_OK);
		return false;
	}

	return true;
}

bool Win32::swap_buffers() {
	/*return 0 == StretchDIBits(front_dc
		, 0, 0, width, height
		, 0, 0, width, height
		, *bits
		, &info
		, DIB_RGB_COLORS
		, SRCCOPY
	);*/

	return BitBlt(front_dc, 0, 0, width, height, back_dc, 0, 0, SRCCOPY);
}

bool Win32::resize(ulong w, ulong h) {
	width  = w;
	height = h;

	GetWindowRect(window, &rect);

	unload_buffer();
	return init_buffer();
}

bool Win32::update() {
	while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (WM_QUIT == msg.message) { return false; }

		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return true;
}

void Win32::close() {
	SendMessageW(window, WM_CLOSE, 0, 0);
}

void Win32::set_title(wchar_t* str) {
	SetWindowTextW(window, str);
}