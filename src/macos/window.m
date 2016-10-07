#import <Cocoa/Cocoa.h>
#include <OpenGL/gl.h>

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <mach/mach_time.h>
#include <dlfcn.h>
#include <signal.h>

#include "game.h"
#include "assets.h"

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
  glLoadIdentity();
  glViewport(0, 0, 800, 600);
  //glOrtho(-1, 1, -1, 1, -1, 1);
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

  NSRect frameRect = NSMakeRect(0, 0, 800, 600);
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
    
    window = [[MacOSWindow alloc] initWithWidth:800 height:600];
    [window setDelegate: appDelegate];
    
    [window setTitle:@"Evolve -.-"];

    [window makeKeyAndOrderFront:nil];
    [window makeMainWindow];

    [NSApp activateIgnoringOtherApps:YES];
  }
}

void MacOS_HandleEvents(void)
{
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
      {{0.78125, 0}, {1, -1}},
      {{0.78125, 0.5859375}, {1, 1}},
      {{0, 0.5859375}, {-1, 1}},
      {{0, 0}, {-1, -1}}
  };
  const static int indices[] = {0, 1, 2, 2, 3, 0};

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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

static void load_dylib()
{
  if (dylibHandle) {
    dlclose(dylibHandle);
    draw_frame = NULL;
  }

  dylibHandle = dlopen("bin/renderer.dylib", RTLD_LOCAL | RTLD_LAZY);
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

int main(int argc, char *argv[])
{
  setup_signal_handlers();
  load_dylib();

  //DebugApplicationPath();
  MacOS_CreateWindow();

  srand(123);

  void *framePixelData = calloc(800*600, 4); // width * height * RGBA

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &texId);
  glBindTexture(GL_TEXTURE_2D, texId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  uint64_t start;
  uint64_t end;
  uint64_t elapsed;

  uint64_t teststart;
  uint64_t testend;

  GLint swapInt = 0;
  [openGLContext setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

  DrawingBuffer drawing_buffer;
  drawing_buffer.pixels = framePixelData;
  drawing_buffer.width = 800;
  drawing_buffer.height = 600;
  drawing_buffer.pitch = 800;
  drawing_buffer.bits_per_pixel = 4;

  GlobalState state;
  state.platform_api.read_file_contents = macos_read_file_contents;

  gameRunning = true;
  start = mach_absolute_time();  

  while(gameRunning) {
    if (shouldReloadDylib) {
      load_dylib();
    }

    MacOS_HandleEvents();

    if (draw_frame) {
      draw_frame(&state, &drawing_buffer);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 800, 600, GL_RGBA, GL_UNSIGNED_BYTE, framePixelData);
    }

    DEBUG_DrawScene();

    teststart = mach_absolute_time();
    uint64_t now = mach_absolute_time();
    double targetMsPerFrame = 33.333333333;
    while ((targetMsPerFrame - (mach_time_in_seconds(now - start) * 1000.0)) > 0.0) {
      now = mach_absolute_time();
    }

    // This code needs to be directly below busy loop / sleep to account for the entire frame worth of computations
    end = mach_absolute_time();
    elapsed = end - start;
    start = end;

    testend = mach_absolute_time();
    double waitMs = mach_time_in_seconds(testend - teststart) * 1000.0;
    //printf("waited %2.6g\n", waitMs);

    [openGLContext flushBuffer];

    double frameTime = mach_time_in_seconds(elapsed) * 1000.0;
    //printf("%.6g ms\n", frameTime);
  }

  return 0;
}
