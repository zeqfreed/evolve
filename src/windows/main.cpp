#include <cstdio>
#include <cstdint>

#include <Windows.h>

#include "platform/platform.h"

#define WIDTH 1024
#define HEIGHT 768

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

HWND create_window(HINSTANCE hInstance, uint16_t width, uint16_t height, char *title)
{
	WNDCLASSEX wndClass = {};
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  wndClass.hInstance = hInstance;
  wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.lpfnWndProc = WndProc;
	wndClass.lpszClassName = "evolveWindowClass";

	if (!RegisterClassEx(&wndClass)) {
		MessageBox(NULL, "RegisterClassEx failed!", "Evolve", NULL);
		return false;
	}

	HWND wnd = CreateWindowEx(
		WS_EX_APPWINDOW,
		wndClass.lpszClassName,
		title,
    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
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

static HMODULE dllHandle = NULL;
static DrawFrameFunc drawFrame = NULL;

bool load_library(char *filename)
{
  printf("Loading library: %s\n", filename);
  HMODULE newHandle = LoadLibrary(filename);
  if (!newHandle) {
    printf("Failed to load library: %s\n", filename);
    return false;
  }

  DrawFrameFunc newDrawFrame = (DrawFrameFunc) GetProcAddress(newHandle, "draw_frame");
  if (!newDrawFrame) {
    printf("GetProcAddress for draw_frame failed\n");
    FreeLibrary(newHandle);
    return false;
  }

  if (dllHandle) {
    FreeLibrary(dllHandle);
  }

  dllHandle = newHandle;
  drawFrame = newDrawFrame;
  return true;
}

inline void prepare_console()
{
  AllocConsole();
  freopen("CONOUT$", "w", stdout);
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
  prepare_console();

  char module[255] = {};
  if (strlen(lpCmdLine) > 0) {
    uint32_t idx = 0;
    while (idx < sizeof(module) && lpCmdLine[idx] != ' ') {
      module[idx] = lpCmdLine[idx];
      idx++;
    }
  }

  char dllPath[255] = {};
  if (strlen(module) > 0) {
    snprintf(dllPath, sizeof(dllPath), "%s.dll", (char *) module);
  } else {
    printf("Supply a module name\n");
    exit(1);
  };

  if (load_library(dllPath)) {
    printf("Loaded module: %s\n", dllPath);
  }

  HWND wnd = create_window(hInstance, WIDTH, HEIGHT, "evolve");
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
