#include <stdio.h>
#include <stdint.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "platform/platform.h"

#include "windows/fs.cpp"
#include "windows/keyboard.cpp"
#include "windows/mouse.cpp"
#include "windows/sound.cpp"

#include "platform/mpq.cpp"

#define WIDTH 1600
#define HEIGHT 900

#define FRAMES_TO_UPDATE_FPS 30

static volatile bool gameRunning = false;
static HMODULE dllHandle = NULL;
static DrawFrameFunc drawFrame = NULL;
static KeyboardState keyboardState = {0};
static MouseState mouseState = {0};
static BITMAPINFO bmi = {0};
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

inline uint32_t map_keycode(WPARAM wParam, LPARAM lParam)
{
  uint32_t result = wParam;

  switch (wParam) {
    case VK_SHIFT: {
      uint32_t scancode = (lParam & 0x00ff0000) >> 16;
      result = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
    }; break;
  
    case VK_CONTROL: {
      uint32_t extended  = (lParam & 0x01000000) >> 23;
      result = VK_LCONTROL + extended;
    }; break;

    case VK_MENU: {
      uint32_t extended  = (lParam & 0x01000000) >> 23;
      result = VK_LMENU + extended;
    }; break;

    default:
      break;
  }

  return result;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message) {
    case WM_KEYDOWN: {
      uint32_t keyCode = map_keycode(wParam, lParam);
      keyboard_state_key_down(&keyboardState, KEYBOARD_CODE(keyCode));
    }; break;

    case WM_KEYUP: {
      uint32_t keyCode = map_keycode(wParam, lParam);
      keyboard_state_key_up(&keyboardState, KEYBOARD_CODE(keyCode));
    }; break;

    case WM_SYSKEYDOWN: {
      uint32_t keyCode = map_keycode(wParam, lParam);
      keyboard_state_key_down(&keyboardState, KEYBOARD_CODE(keyCode));
    }; break;

    case WM_SYSKEYUP: {
      uint32_t keyCode = map_keycode(wParam, lParam);
      keyboard_state_key_up(&keyboardState, KEYBOARD_CODE(keyCode));
    }; break;              

    case WM_DESTROY:
      gameRunning = false;
      PostQuitMessage(0);
      break;

    case WM_SYSCOMMAND:
      if (wParam == SC_KEYMENU && (lParam>>16) <= 0) return 0;  
      return DefWindowProc(hWnd, message, wParam, lParam);
      break;

    case WM_SETFOCUS:
      // TODO: Disable sticky keys and other accessibility features when app is active
      break;

    case WM_KILLFOCUS:
      keyboard_clear_state(&keyboardState);
      break;

    case WM_LBUTTONDOWN: {
      SetCapture(hWnd);
      int32_t xpos = GET_X_LPARAM(lParam); 
      int32_t ypos = GET_Y_LPARAM(lParam);
      mouse_state_set_position(&mouseState, (float) xpos, (float) ypos);
      mouse_state_button_down(&mouseState, MB_LEFT);
    } break;

    case WM_LBUTTONUP: {
      ReleaseCapture();
      int32_t xpos = GET_X_LPARAM(lParam); 
      int32_t ypos = GET_Y_LPARAM(lParam);
      mouse_state_set_position(&mouseState, (float) xpos, (float) ypos);
      mouse_state_button_up(&mouseState, MB_LEFT);
    } break;

    case WM_MOUSEMOVE: {
      int32_t xpos = GET_X_LPARAM(lParam); 
      int32_t ypos = GET_Y_LPARAM(lParam);
      mouse_state_set_position(&mouseState, (float) xpos, (float) ypos);
    } break;

    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }

  return 0;
}

HWND create_window(HINSTANCE hInstance, uint16_t width, uint16_t height, char *title)
{
	WNDCLASSEX wndClass = {0};
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  wndClass.hInstance = hInstance;
  wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.lpfnWndProc = WndProc;
	wndClass.lpszClassName = "evolveWindowClass";

	if (!RegisterClassEx(&wndClass)) {
		MessageBox(NULL, "RegisterClassEx failed!", "Evolve", 0);
		return 0;
	}

  uint32_t style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;
  uint32_t exStyle = WS_EX_APPWINDOW;
  RECT rect = {0};
  rect.right = width;
  rect.bottom = height;
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

C_LINKAGE void windows_free_memory(void *memory)
{
  free(memory);
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

  if (state->keyboard->downedKeys[KB_ESCAPE]) {
    state->platform_api.terminate();
    return;
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

MPQFileId windows_get_asset_id(char *name)
{
  return mpq_file_id(name);
}

MPQFile windows_load_asset(char *name)
{
  int32_t size = windows_fs_size(name);
  if (size >= 0) {
    MPQFile result = {0};
    result.id = mpq_file_id(name);
    result.data = windows_allocate_memory(size);
    result.size = size;
    windows_fs_read(name, result.data, size);
    return result;
  }

  return mpq_load_file(&MPQ_REGISTRY, name);
}

void windows_release_asset(MPQFile *file)
{
  mpq_release_file(&MPQ_REGISTRY, file);
}

PlatformAPI PLATFORM_API = {
  (GetFileSizeFunc) windows_fs_size,
  (ReadFileContentsFunc) windows_fs_read,
  (AllocateMemoryFunc) windows_allocate_memory,
  (FreeMemoryFunc) windows_free_memory,
  (TerminateFunc) windows_terminate,
  (GetAssetIdFunc) windows_get_asset_id,
  (LoadAssetFunc) windows_load_asset,
  (ReleaseAssetFunc) windows_release_asset,
  (FileOpenFunc) windows_file_open,
  (FileReadFunc) windows_file_read,
  (DirectoryListingBeginFunc) windows_directory_listing_begin,
  (DirectoryListingNextEntryFunc) windows_directory_listing_next_entry,
  (DirectoryListingEndFunc) windows_directory_listing_end,
  (SoundBufferInitFunc) windows_sound_buffer_init,
  (SoundBufferFinalizeFunc) windows_sound_buffer_finalize,
  (SoundBufferPlayFunc) windows_sound_buffer_play,
  (SoundBufferStopFunc) windows_sound_buffer_stop,
  (SoundBufferLockFunc) windows_sound_buffer_lock,
  (SoundBufferUnlockFunc) windows_sound_buffer_unlock
};

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
  prepare_console();

  char module[255] = {0};
  if (strlen(lpCmdLine) > 0) {
    uint32_t idx = 0;
    while (idx < sizeof(module) && lpCmdLine[idx] != ' ') {
      module[idx] = lpCmdLine[idx];
      idx++;
    }
  }

  char dllPath[MAX_PATH] = {0};
  size_t len = strlen(module);
  if (len > 3) {
    if (module[len - 3] == 'd' && module[len - 2] == 'l' && module[len - 1] == 'l') {
      memcpy(dllPath, module, len);
    } else {
      snprintf(dllPath, MAX_PATH, "%s.dll", (char *) module);
    }
  };

  if (load_library(dllPath)) {
    printf("Loaded module: %s\n", dllPath);
  } else {
    drawFrame = test_draw_frame;
  }

  gameWindow = create_window(hInstance, WIDTH, HEIGHT, (char *) "Evolve");
  if (!gameWindow) {
    return 1;
  }

  windows_sound_hwnd = gameWindow;
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

  GlobalState state = {0};
  state.platform_api = PLATFORM_API;
  state.keyboard = &keyboardState; 
  state.mouse = &mouseState;

  mpq_registry_init(&MPQ_REGISTRY, (char *) "G:\\Games\\WoW Classic\\Data");

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

    keyboard_clear_state_changes(&keyboardState);
    mouse_state_clear_frame_changes(&mouseState);    
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
