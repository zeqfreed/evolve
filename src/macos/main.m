#import <Cocoa/Cocoa.h>
#include <OpenGL/gl.h>

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <mach/mach_time.h>
#include <dlfcn.h>
#include <signal.h>

#include "platform/platform.h"

#include "keyboard.cpp"
#include "mouse.cpp"

static KeyboardState keyboardState;
static MouseState mouseState;

@interface MacOSAppDelegate : NSObject<NSApplicationDelegate, NSWindowDelegate>
{
}
@end

@interface MacOSWindowView : NSView
{
}
- (void) drawRect: (NSRect) bounds;
@end

@interface MacOSWindow : NSWindow
{
}

@end

#define WIDTH 1600
#define HEIGHT 900
#define TEX_WIDTH 2048
#define TEX_HEIGHT 1024
#define TEX_U ((float) WIDTH / (float) TEX_WIDTH)
#define TEX_V ((float) HEIGHT / (float) TEX_HEIGHT)

static BOOL gameRunning = false;
static MacOSWindow *window = nil;
static MacOSWindowView *view = nil;
static NSOpenGLContext* openGLContext = nil;

@implementation MacOSAppDelegate
- (BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
  return YES;
}

- (void) windowDidResize: (id)sender
{
  NSWindow* window = [sender object];
  NSRect frame = [window frame];

  [openGLContext update];

  glDisable(GL_DEPTH_TEST);
  glViewport(0, 0, WIDTH, HEIGHT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1, 1, -1, 1, -1, 1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

- (void) windowWillClose: (id)sender
{
  gameRunning = false;
}
@end

@implementation MacOSWindowView
-(void) drawRect: (NSRect) bounds
{
  //printf("%s\n",__FUNCTION__);
}

- (void)keyDown:(NSEvent *)theEvent {
  if ([theEvent isARepeat]) {
    return;
  }

  keyboard_state_key_down(&keyboardState, KEYBOARD_CODE([theEvent keyCode]));
}

- (void)keyUp:(NSEvent *)theEvent {
  if ([theEvent isARepeat]) {
    return;
  }

  keyboard_state_key_up(&keyboardState, KEYBOARD_CODE([theEvent keyCode]));
}

#define MASK_RSHIFT 0x04
#define MASK_RALT 0x40
#define MASK_RCMD 0x08

- (void)flagsChanged:(NSEvent *)theEvent
{
  NSUInteger flags = [theEvent modifierFlags];

  if (flags & NSCommandKeyMask) {
    if (flags & MASK_RCMD) {
      keyboard_state_key_down(&keyboardState, KB_RIGHT_GUI);
    } else {
      keyboard_state_key_down(&keyboardState, KB_LEFT_GUI);
    }
  } else {
    keyboard_state_key_up(&keyboardState, KB_LEFT_GUI);
    keyboard_state_key_up(&keyboardState, KB_RIGHT_GUI);
  }

  if (flags & NSShiftKeyMask) {
    if (flags & MASK_RSHIFT) {
      keyboard_state_key_down(&keyboardState, KB_RIGHT_SHIFT);
    } else {
      keyboard_state_key_down(&keyboardState, KB_LEFT_SHIFT);
    }
  } else {
    keyboard_state_key_up(&keyboardState, KB_LEFT_SHIFT);
    keyboard_state_key_up(&keyboardState, KB_RIGHT_SHIFT);
  }

  if (flags & NSAlternateKeyMask) {
    if (flags & MASK_RALT) {
      keyboard_state_key_down(&keyboardState, KB_RIGHT_ALT);
    } else {
      keyboard_state_key_down(&keyboardState, KB_LEFT_ALT);
    }
  } else {
    keyboard_state_key_up(&keyboardState, KB_LEFT_ALT);
    keyboard_state_key_up(&keyboardState, KB_RIGHT_ALT);
  }

  // TODO: Right control!
  if (flags & NSControlKeyMask) {
    keyboard_state_key_down(&keyboardState, KB_LEFT_CONTROL);
  } else {
    keyboard_state_key_up(&keyboardState, KB_LEFT_CONTROL);
  }

  if (flags & NSFunctionKeyMask) {
    keyboard_state_key_down(&keyboardState, KB_FUNCTION);
  } else {
    keyboard_state_key_up(&keyboardState, KB_FUNCTION);
  }

  // TODO: Update modifier flags
}

- (void) mouseDown:(NSEvent *)theEvent
{
  mouse_state_button_down(&mouseState, MB_LEFT);
}

- (void) mouseUp:(NSEvent *)theEvent
{
  mouse_state_button_up(&mouseState, MB_LEFT);
}
@end

@implementation MacOSWindow
- (id) initWithWidth: (int)width height:(int)height
{
  NSRect contentRect = NSMakeRect(0, 0, width, height);
  unsigned int styleMask = NSTitledWindowMask |
                           NSClosableWindowMask |
                           NSMiniaturizableWindowMask |
                           NSResizableWindowMask;

  self = [super initWithContentRect:contentRect styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
  if (!self) {
    return nil;
  }

  NSRect frameRect = NSMakeRect(0, 0, WIDTH, HEIGHT);
  view = [[MacOSWindowView alloc] initWithFrame:frameRect];

  [self setContentView:view];
  [self makeFirstResponder:view];

  NSOpenGLPixelFormatAttribute pixelFormatAttributes[] = {
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFADepthSize, 24,
    0
  };

  NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes];
  openGLContext = [[NSOpenGLContext alloc] initWithFormat:format shareContext:NULL];

  [openGLContext setView:view];
  [openGLContext makeCurrentContext];

  return self;
}
@end

// static void MacOS_AddApplicationMenu(void)
// {
//   @autoreleasepool {
//     NSMenu *menubar = [[NSMenu alloc] init];
//     NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
//     [menubar addItem:appMenuItem];
//     [NSApp setMainMenu:menubar];

//     // NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
//     // [fileMenuQuit setTarget:NSApp];
//     // [fileSubMenu addItem:fileMenuQuit];
//   }
// }

// static void DebugApplicationPath(void)
// {
//     NSAutoreleasePool *pool=[[NSAutoreleasePool alloc] init];

//     char cwd[256];
//     getcwd(cwd,255);
//     printf("CWD(Initial): %s\n",cwd);

//     NSString *path;
//     path=[[NSBundle mainBundle] bundlePath];
//     printf("BundlePath:%s\n",[path UTF8String]);

//     [[NSFileManager defaultManager] changeCurrentDirectoryPath:path];

//     getcwd(cwd,255);
//     printf("CWD(Changed): %s\n",cwd);

//     [pool release];
// }

void MacOS_CreateWindow()
{
  @autoreleasepool {

    [NSApplication sharedApplication];

    MacOSAppDelegate *appDelegate = [[MacOSAppDelegate alloc] init];

    [NSApp setDelegate: appDelegate];
    [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];
    [NSApp finishLaunching];

    window = [[MacOSWindow alloc] initWithWidth:WIDTH height:HEIGHT];
    [window setDelegate: appDelegate];
    [window center];

    [window setTitle:@"Evolve -.-"];

    [window makeKeyAndOrderFront:nil];
    [window makeMainWindow];

    [NSApp activateIgnoringOtherApps:YES];
  }
}

void MacOS_HandleEvents(void)
{
  keyboard_clear_state_changes(&keyboardState);
  mouse_state_clear_frame_changes(&mouseState);

  CGPoint pos = (CGPoint) [window mouseLocationOutsideOfEventStream];
  mouse_state_set_position(&mouseState, pos.x, (float) HEIGHT - pos.y);

  @autoreleasepool {
    while(true) {
      NSEvent *event = [NSApp nextEventMatchingMask:NSAnyEventMask
                              untilDate: [NSDate distantPast]
                              inMode: NSDefaultRunLoopMode
                              dequeue:YES];

      if (event == nil) {
        break;
      }

      [NSApp sendEvent:event];
      [NSApp updateWindows];
    }
  }
}

void macos_usleep(int microsecs)
{
  usleep(microsecs);
}

typedef struct {
    float TexCoord[2];
    float Position[2];
} Vertex;

static void *framePixelData = nil;
static GLuint texId = 0;

static void DEBUG_DrawScene() {
  const static Vertex vertices[] = {
      {{TEX_U, 0}, {1, -1}},
      {{TEX_U, TEX_V}, {1, 1}},
      {{0, TEX_V}, {-1, 1}},
      {{0, 0}, {-1, -1}}
  };
  const static int indices[] = {0, 1, 2, 2, 3, 0};

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glColor3f(1.0, 0.0, 0.0);

  glBindTexture(GL_TEXTURE_2D, texId);
  glBegin(GL_TRIANGLES);
  for (int i = 0; i <= 5; i++) {
    Vertex v = vertices[indices[i]];
    glTexCoord2f(v.TexCoord[0], v.TexCoord[1]);
    glVertex2f(v.Position[0], v.Position[1]);
  }
  glEnd();
}

static inline double mach_time_in_seconds(uint64_t interval)
{
  mach_timebase_info_data_t timebase;
  mach_timebase_info(&timebase);
  return (double) interval * (double)timebase.numer / (double)timebase.denom / 1e9;
}

static DrawFrameFunc draw_frame = NULL;
static bool shouldReloadDylib = false;
static void *dylibHandle = NULL;

static void load_dylib(char *path)
{
  if (dylibHandle) {
    dlclose(dylibHandle);
    draw_frame = NULL;
  }

  dylibHandle = dlopen(path, RTLD_LOCAL | RTLD_LAZY);
  if (!dylibHandle) {
    printf("[%s] Unable to load library: %s\n", __FILE__, dlerror());
    exit(1);
  }

  draw_frame = dlsym(dylibHandle, "draw_frame");
  if (!draw_frame) {
    printf("[%s] Unable to get symbol: %s\n", __FILE__, dlerror());
    exit(1);
  }

  shouldReloadDylib = false;
}

static void handle_signal(int signal) {
  if (signal == SIGUSR1) {
    shouldReloadDylib = true;
  }
}

static void setup_signal_handlers()
{
  struct sigaction sa;
  sa.sa_handler = &handle_signal;
  sa.sa_flags = SA_RESTART;
  sigaddset(&sa.sa_mask, SIGUSR1);
  if (sigaction(SIGUSR1, &sa, NULL) == -1) {
    printf("Failed to setup USR1 signal handler\n");
  }
}

C_LINKAGE void *macos_allocate_memory(size_t size)
{
  void *result = calloc(size, 1);
  return result;
}

C_LINKAGE void macos_free_memory(void *memory)
{
  free(memory);
}

C_LINKAGE void macos_terminate()
{
  gameRunning = false;
}

static void update_fps(float frameMs, float fps)
{
  char buf[255];
  snprintf(buf, 255, "Evolve - %.2f ms / %.1f FPS", frameMs, fps);

  NSString *title = [[NSString alloc] initWithUTF8String:buf];
  [window setTitle:title];
}

int main(int argc, char *argv[])
{
  char dylib_path[255];
  if (argc > 1) {
    snprintf(dylib_path, 255, "bin/%s.dylib", argv[1]);
  } else {
    printf("Usage: %s MODULE\n", argv[0]);
    exit(1);
  };

  setup_signal_handlers();
  load_dylib(dylib_path);

  //DebugApplicationPath();
  MacOS_CreateWindow();

  srand(123);

  void *framePixelData = calloc(WIDTH*HEIGHT, sizeof(float) * 4); // width * height * RGBA

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &texId);
  glBindTexture(GL_TEXTURE_2D, texId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_WIDTH, TEX_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  uint64_t start;
  uint64_t end;
  uint64_t elapsed;

  uint64_t teststart;
  uint64_t testend;

  GLint swapInt = 0;
  [openGLContext setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

  DrawingBuffer drawing_buffer;
  drawing_buffer.pixels = framePixelData;
  drawing_buffer.width = WIDTH;
  drawing_buffer.height = HEIGHT;
  drawing_buffer.pitch = WIDTH;
  drawing_buffer.bytes_per_pixel = 4;

  GlobalState state = {};
  state.platform_api = PLATFORM_API;

  state.keyboard = &keyboardState;
  state.mouse = &mouseState;

  gameRunning = true;
  start = mach_absolute_time();

  float targetMsPerFrame = 1000 / 60.0;
  double lastFrameTotal = targetMsPerFrame;

  int frames = 0;
  float averageFps = 0;
  float acc = 0;

  mpq_registry_init(&MPQ_REGISTRY, (char *) "data/misc");

  while(gameRunning) {
    if (frames >= 100) {
      averageFps = 100.0 / acc;
      update_fps((acc / 100.0) * 1000.0, averageFps);
      acc = 0;
      frames = 0;

      if (averageFps >= 60.0) {
        targetMsPerFrame = 1000 / 60.0;
      } else {
        targetMsPerFrame = 1000 / 30.0;
      }
    }

    if (shouldReloadDylib) {
      load_dylib(dylib_path);
    }

    MacOS_HandleEvents();

    if (draw_frame) {
      draw_frame(&state, &drawing_buffer, lastFrameTotal);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, framePixelData);
    }

    DEBUG_DrawScene();

    acc += mach_time_in_seconds(mach_absolute_time() - start);
    frames++;

#if 0
    uint64_t now = mach_absolute_time();
    while ((targetMsPerFrame - (mach_time_in_seconds(now - start) * 1000.0)) > 0.0) {
      now = mach_absolute_time();
    }
#endif

    // This code needs to be directly below busy loop / sleep to account for the entire frame's worth of computations
    end = mach_absolute_time();
    lastFrameTotal = mach_time_in_seconds(end - start);
    start = end;

    [openGLContext flushBuffer];
  }

  return 0;
}
