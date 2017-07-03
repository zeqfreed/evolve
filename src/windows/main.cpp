#include <cstdio>
#include <cstdint>

#define _USE_MATH_DEFINES
#include <cmath>

#include <Windows.h>

#include "platform/platform.h"

#include "windows/fs.cpp"

#define WIDTH 1024
#define HEIGHT 768

#define FRAMES_TO_UPDATE_FPS 30

static volatile bool gameRunning = false;
static HMODULE dllHandle = NULL;
static DrawFrameFunc drawFrame = NULL;
static KeyboardState keyboardState;
static BITMAPINFO bmi = {};
static HDC targetDC = NULL;
static HWND gameWindow = NULL;

inline void blit_buffer(DrawingBuffer *buffer)
{
  StretchDIBits(
    targetDC,
    0, 0, WIDTH, HEIGHT,
    0, 0, buffer->width, buffer->height,
    buffer->pixels,
    &bmi,
    DIB_RGB_COLORS,
    SRCCOPY
  );
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message) {
    case WM_DESTROY:
      gameRunning = false;
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

  uint32_t style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;
  uint32_t exStyle = WS_EX_APPWINDOW;
  RECT rect = {0, 0, width, height};
  AdjustWindowRectEx(&rect, style, false, exStyle);

	HWND wnd = CreateWindowEx(
		exStyle,
		wndClass.lpszClassName,
		title,
    style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		0,
		0,
		0,
		0
	);

  return wnd;
}

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

static void windows_handle_events()
{
  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

C_LINKAGE void *windows_allocate_memory(size_t size)
{
  void *result = calloc(size, 1);
  return result;
}

C_LINKAGE void windows_terminate()
{
  gameRunning = false;
}

static void update_fps(float frameMs, float fps)
{
  char buf[255];
  snprintf(buf, 255, "Evolve - %.2f ms / %.1f FPS", frameMs, fps);
  SetWindowText(gameWindow, buf);
}

void test_draw_frame(GlobalState *state, DrawingBuffer *buffer, float dt)
{
  static float start = 0.0f;
  start -= dt * (float) (M_PI / 2.0f);
  if (start > 2.0f * M_PI) {
    start -= (float) (2.0f * M_PI);
  }

  uint32_t clearColor = 0x00FFFFFF;
  uint32_t color = 0x00FF0000;

  for (uint32_t x = 0; x < buffer->width; x++) {
    for (uint32_t y = 0; y < buffer->height; y++) {
      ((uint32_t *) buffer->pixels)[y * buffer->pitch + x] = clearColor;
    }
  }

  float piPerPx = (float) M_PI * 4.0f / (float) buffer->width;

  float hh = (float) buffer->height / 2.0f;
  float fheight = 100.0f;

  for (uint32_t x = 0; x < buffer->width; x++) {
    float fx = sinf(start + piPerPx * x) + cosf(start - piPerPx * x);
    uint32_t y = (uint32_t) (fx * fheight + hh);
    ((uint32_t *) buffer->pixels)[y * buffer->pitch + x] = color;
  }
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

  //drawFrame = test_draw_frame;

  gameWindow = create_window(hInstance, WIDTH, HEIGHT, "Evolve");
  if (!gameWindow) {
    return 1;
  }

  targetDC = GetDC(gameWindow);

  uint32_t bytesPerPixel = 4;

  DrawingBuffer drawingBuffer;
  drawingBuffer.pixels = calloc(WIDTH * HEIGHT, bytesPerPixel);
  drawingBuffer.width = WIDTH;
  drawingBuffer.height = HEIGHT;
  drawingBuffer.pitch = WIDTH;
  drawingBuffer.bytes_per_pixel = 4;

  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = WIDTH;
  bmi.bmiHeader.biHeight = HEIGHT;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = (uint16_t) bytesPerPixel * 8;
  bmi.bmiHeader.biCompression = BI_RGB;

  GlobalState state = {};
  state.platform_api.get_file_size = windows_fs_size;
  state.platform_api.read_file_contents = windows_fs_read;
  state.platform_api.allocate_memory = windows_allocate_memory;
  state.platform_api.terminate = windows_terminate;
  
  state.keyboard = &keyboardState; 

  gameRunning = true;

  uint64_t freq;
  QueryPerformanceFrequency((LARGE_INTEGER *) &freq);

  uint64_t start;
  uint64_t end;
  float countsToSeconds = 1.0f / (float) freq;
  float lastFrameTotal = 0.0f;

  QueryPerformanceCounter((LARGE_INTEGER *) &start);

  uint32_t frames = 0;
  float averageFps = 0;
  float acc = 0;

  while(gameRunning) {
    if (frames >= FRAMES_TO_UPDATE_FPS) {
      averageFps = (float) FRAMES_TO_UPDATE_FPS / acc;
      update_fps((acc / (float) FRAMES_TO_UPDATE_FPS) * 1000.0f, averageFps);
      acc = 0.0f;
      frames = 0;

      // if (averageFps >= 60.0) {
      //   targetMsPerFrame = 1000 / 60.0;
      // } else {
      //   targetMsPerFrame = 1000 / 30.0;
      // }
    }

    windows_handle_events();

    if (drawFrame) {
      drawFrame(&state, &drawingBuffer, lastFrameTotal);
      blit_buffer(&drawingBuffer);
    }

    QueryPerformanceCounter((LARGE_INTEGER *) &end);
    lastFrameTotal = (end - start) * countsToSeconds;
    start = end;

    acc += lastFrameTotal;
    frames++;
  }

	return 0;
}
