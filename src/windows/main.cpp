#include <cstdio>
#include <cstdint>

#define UNICODE

#include <Windows.h>
#include <wchar.h>

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message) {
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);
      FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
      EndPaint(hWnd, &ps);
    }; break;

    case WM_DESTROY:
      PostQuitMessage(0);
      break;

    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }

  return 0;
}

HWND create_window(HINSTANCE hInstance, uint16_t width, uint16_t height, wchar_t *title)
{
	WNDCLASSEX wndClass = {};
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
  wndClass.hInstance = hInstance;
	wndClass.lpfnWndProc = WndProc;
	wndClass.lpszClassName = L"evolveWindowClass";

	if (!RegisterClassEx(&wndClass)) {
		MessageBox(NULL, L"RegisterClassEx failed!", L"Evolve", NULL);
		return false;
	}

	HWND wnd = CreateWindowEx(
		WS_EX_APPWINDOW,
		wndClass.lpszClassName,
		title,
    WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_MINIMIZEBOX | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		0,
		0,
		0,
		0
	);

  return wnd;
}


int WINAPI WinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  HWND wnd = create_window(hInstance, 800, 600, L"evolve");
  if (!wnd) {
    return 1;
  }

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

	return 0;
}
