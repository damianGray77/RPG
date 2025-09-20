#include "pch.h"
#include "Win32.h"
#include <thread>

Win32 *Win32::self = NULL;
double Win32::query_perf_freq = 0;

Win32::Win32() {
	self = this;

	bits = NULL;

	renderer = RendererDX11::create_renderer(new RendererDX11());

	window   = NULL;
	instance = NULL;

	    draw_callback = NULL;
	sizemove_callback = NULL;

	buffer_width  = 0;
	buffer_height = 0;
	minmax = {};

	resize_move = false;

	client_dims = {};

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
	LRESULT ret = 0;

	switch (msg) {
		case WM_SIZE: {
			client_dims = get_client_dimensions();
			renderer.resize(client_dims.width, client_dims.height);
			
			// TODO how to enforce the aspect ratio to limit the size of the window while dragging?
			/*if (w > h) {
				h = ceil<float>(w * buffer_height / (float)buffer_width);
			} else if(h > w) {
				w = ceil<float>(h * buffer_width / (float)buffer_height);
			}*/

			break;
		}
		case WM_GETMINMAXINFO: {
			MINMAXINFO *mmi = (MINMAXINFO *)lparam;
			
			mmi->ptMinTrackSize.x = minmax.min.x;
			mmi->ptMinTrackSize.y = minmax.min.y;

			mmi->ptMaxTrackSize.x = minmax.max.x;
			mmi->ptMaxTrackSize.y = minmax.max.y;

			break;
		}
		case WM_PAINT: {
			PAINTSTRUCT paint;

			BeginPaint(window, &paint);
			display_buffer();
			EndPaint(window, &paint);

			break;
		}
		// This is a hack. The MOVE / SIZE message triggers its own loop
		// and doesn't return back to the main Peek loop until it is done.
		// This causes the entire game to halt until the window is finished
		// dragging / resizing. To get around this, We set a timer that
		// runs every 100ms. This triggers a 2nd game loop to run in a new
		// thread for the duration of 100ms or until the event ends.
		case WM_ENTERSIZEMOVE: {
			// execute the callback immediately, otherwise there will be a 100ms delay.
			sizemove();

			SetTimer(window, NULL, 100u, NULL);
			resize_move = true;
			break;
		}
		case WM_EXITSIZEMOVE: {
			KillTimer(window, NULL);
			resize_move = false;
			break;
		}
		case WM_TIMER: {
			sizemove();
			break;
		}
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP: {
			const bool was_pressed = 0 != (lparam & 0x40000000); // 30th bit
			const bool  is_pressed = 0 == (lparam & 0x80000000); // 31st bit
			if (was_pressed == is_pressed) { break; }

			const uint8 mapped = map_key(wparam);
			key_press[mapped] = is_pressed;
			break;
		}
		case WM_CLOSE:
		case WM_DESTROY: {
			PostQuitMessage(0);
			break;
		}
		default: {
			ret = DefWindowProcW(window, msg, wparam, lparam);
			break;
		}
	}

	return ret;
}

ClientDimensions Win32::get_client_dimensions() {
	RECT client_rect;
	GetClientRect(window, &client_rect);

	ClientDimensions ret;
	ret.width  = client_rect.right  - client_rect.left;
	ret.height = client_rect.bottom - client_rect.top;

	return ret;
}

void Win32::sizemove() {
	std::thread t(sizemove_callback);
	t.detach();
}

uint8 Win32::map_key(WPARAM wparam) {
	switch(wparam) {
		case VK_ESCAPE:     return 0x01;
		case VK_1:          return 0x02;
		case VK_2:          return 0x03;
		case VK_3:          return 0x04;
		case VK_4:          return 0x05;
		case VK_5:          return 0x06;
		case VK_6:          return 0x07;
		case VK_7:          return 0x08;
		case VK_8:          return 0x09;
		case VK_9:          return 0x0A;
		case VK_0:          return 0x0B;
		case VK_OEM_MINUS:  return 0x0C;
		case VK_OEM_PLUS:   return 0x0D;
		case VK_OEM_5:      return 0x0E;
		case VK_TAB:        return 0x0F;
		case VK_Q:          return 0x10;
		case VK_W:          return 0x11;
		case VK_E:          return 0x12;
		case VK_R:          return 0x13;
		case VK_T:          return 0x14;
		case VK_Y:          return 0x15;
		case VK_U:          return 0x16;
		case VK_I:          return 0x17;
		case VK_O:          return 0x18;
		case VK_P:          return 0x19;
		case VK_OEM_4:      return 0x1A;
		case VK_OEM_6:      return 0x1B;
		case VK_RETURN:     return 0x1C;
		case VK_CONTROL:    return 0x1D;
		case VK_A:          return 0x1E;
		case VK_S:          return 0x1F;
		case VK_D:          return 0x20;
		case VK_F:          return 0x21;
		case VK_G:          return 0x22;
		case VK_H:          return 0x23;
		case VK_J:          return 0x24;
		case VK_K:          return 0x25;
		case VK_L:          return 0x26;
		case VK_OEM_1:      return 0x27;
		case VK_OEM_7:      return 0x28;
		case VK_OEM_3:      return 0x29;
		case VK_LSHIFT:     return 0x2A;
		case VK_OEM_102:    return 0x2B;
		case VK_Z:          return 0x2C;
		case VK_X:          return 0x2D;
		case VK_C:          return 0x2E;
		case VK_V:          return 0x2F;
		case VK_B:          return 0x30;
		case VK_N:          return 0x31;
		case VK_M:          return 0x32;
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

bool Win32::init(const uint32 width, const uint32 height) {
	buffer_width  = width;
	buffer_height = height;

	timeBeginPeriod(1);

	LARGE_INTEGER li;
	if (QueryPerformanceFrequency(&li)) {
		Win32::query_perf_freq = 1000000.0 / double(li.QuadPart);
	}

	if (!init_window(width, height)) {
		MessageBoxW(window, L"Cannot init window!", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	if (!renderer.init_buffer(window, bits, width, height)) {
		MessageBoxW(window, L"Cannot init buffer!", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	ShowWindow(window, fullscreen ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
	UpdateWindow(window);

	SetFocus(window);
	if (!fullscreen) {
		SetForegroundWindow(window);
	}

	return true;
}

void Win32::show_console() {
	HWND console = GetConsoleWindow();
	HWND owner = GetWindow(console, GW_OWNER);

	ShowWindow(NULL == owner
		? console // Windows 10
		: owner   // Windows 11
	, SW_SHOW);
}

void Win32::hide_console() {
	HWND console = GetConsoleWindow();
	HWND owner = GetWindow(console, GW_OWNER);

	ShowWindow(NULL == owner
		? console // Windows 10
		: owner   // Windows 11
	, SW_HIDE);
}

bool Win32::init_window(const int32 width, const int32 height) {
	instance = (HINSTANCE)GetModuleHandleW(NULL);
	
	WNDCLASSEXW wc = {};
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc   = proc;
	wc.hInstance     = instance;
	wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
	wc.lpszClassName = cname;

	if (!RegisterClassExW(&wc)) { return false; }

	fullscreen = false; // IDYES == MessageBoxW(NULL, L"Click Yes to go to full screen (Recommended)", L"Options", MB_YESNO | MB_ICONQUESTION);

	RECT main_rect = { 0, 0, width,     height };
	RECT  min_rect = { 0, 0, width / 2, height / 2 };
	RECT  max_rect = { 0, 0, width * 2, height * 2 };

	uint32 style, style_ex;
	if (fullscreen) {
		style    = WS_POPUP;
		style_ex = 0;

		if (!full_screen()) { return false; }

		ShowCursor(false);
	} else {
		style    = WS_OVERLAPPEDWINDOW;
		style_ex = WS_EX_CLIENTEDGE;

		AdjustWindowRectEx(&main_rect, style, false, style_ex);
		AdjustWindowRectEx( &min_rect, style, false, style_ex);
		AdjustWindowRectEx( &max_rect, style, false, style_ex);
	}

	minmax = {};
	minmax.min.x = (uint16)(min_rect.right  - min_rect.left);
	minmax.min.y = (uint16)(min_rect.bottom - min_rect.top );
	minmax.max.x = (uint16)(max_rect.right  - max_rect.left);
	minmax.max.y = (uint16)(max_rect.bottom - max_rect.top );

	const uint32 window_width  = main_rect.right  - main_rect.left;
	const uint32 window_height = main_rect.bottom - main_rect.top;

	window = CreateWindowExW(
		  style_ex
		, wc.lpszClassName
		, wname
		, style
		, CW_USEDEFAULT
		, CW_USEDEFAULT
		, window_width
		, window_height
		, NULL
		, NULL
		, wc.hInstance
		, NULL
	);
	if (!window) { return false; }

	client_dims = get_client_dimensions();

	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	cores = sysinfo.dwNumberOfProcessors;

	return true;
}

void Win32::unload() {
	timeEndPeriod(1);

	if (fullscreen) {
		ChangeDisplaySettingsW(NULL, 0);
		ShowCursor(true);
	}

	renderer.unload_buffer();

	UnregisterClassW(cname, instance);
}

bool Win32::full_screen() {
	DEVMODEW settings = {};

	if (!EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &settings)) {
		MessageBoxW(NULL, L"Could Not Enum Display Settings", L"Error", MB_OK);
		return false;
	}

	settings.dmPelsWidth  = client_dims.width;
	settings.dmPelsHeight = client_dims.height;
	settings.dmColor      = color_depth;

	int result = ChangeDisplaySettingsW(&settings, CDS_FULLSCREEN);

	if (DISP_CHANGE_SUCCESSFUL != result) {
		MessageBoxW(NULL, L"Display Mode Not Compatible", L"Error", MB_OK);
		return false;
	}

	return true;
}

bool Win32::display_buffer() {
	return renderer.display_buffer(client_dims.width, client_dims.height);
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