#include <stdint.h>

#define MB_LEFT 0
#define MB_RIGHT 1
#define MB_MIDDLE 2
#define MAX_MOUSE_BUTTONS 24

typedef struct MouseButtonState {
  float dragStartX;
  float dragStartY;
} MouseButtonState;

typedef struct MouseState {
  float windowX;
  float windowY;
  float frameDx;
  float frameDy;
  uint32_t downedButtons;
  uint8_t frameStateChanges[MAX_MOUSE_BUTTONS];
  MouseButtonState buttons[MAX_MOUSE_BUTTONS];
} MouseState;

#define MOUSE_BUTTON_STATE_CHANGED(ST, BTN) (!!ST->frameStateChanges[BTN])
#define MOUSE_BUTTON_STATE_UNCHANGES(ST, BTN) (!ST->frameStateChanges[BTN])
#define MOUSE_BUTTON_IS_DOWN(ST, BTN) ((ST->downedButtons & (1 << BTN)) == (1 << BTN))
#define MOUSE_BUTTON_IS_UP(ST, BTN) (!MOUSE_BUTTON_IS_DOWN(ST, BTN))
#define MOUSE_BUTTON_WAS_PRESSED(ST, BTN) (MOUSE_BUTTON_IS_DOWN(ST, BTN) && MOUSE_BUTTON_STATE_CHANGED(ST, BTN))
#define MOUSE_BUTTON_WAS_RELEASED(ST, BTN) (MOUSE_BUTTON_IS_UP(ST, BTN) && MOUSE_BUTTON_STATE_CHANGED(ST, BTN))
