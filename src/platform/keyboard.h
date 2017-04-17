#include <stdint.h>

typedef uint8_t kb_t;
enum {
  KB_NONE = 0,
  KB_A = 0x04,
  KB_B = 0x05,
  KB_C = 0x06,
  KB_D = 0x07,
  KB_E = 0x08,
  KB_F = 0x09,
  KB_G = 0x0A,
  KB_H = 0x0B,
  KB_I = 0x0C,
  KB_J = 0x0D,
  KB_K = 0x0E,
  KB_L = 0x0F,
  KB_M = 0x10,
  KB_N = 0x11,
  KB_O = 0x12,
  KB_P = 0x13,
  KB_Q = 0x14,
  KB_R = 0x15,
  KB_S = 0x16,
  KB_T = 0x17,
  KB_U = 0x18,
  KB_V = 0x19,
  KB_W = 0x1A,
  KB_X = 0x1B,
  KB_Y = 0x1C,
  KB_Z = 0x1D,
  KB_1 = 0x1E,
  KB_2 = 0x1F,
  KB_3 = 0x20,
  KB_4 = 0x21,
  KB_5 = 0x22,
  KB_6 = 0x23,
  KB_7 = 0x24,
  KB_8 = 0x25,
  KB_9 = 0x26,
  KB_0 = 0x27,
  KB_RETURN = 0x28,
  KB_ESCAPE = 0x29,
  KB_BACKSPACE = 0x2A,
  KB_TAB = 0x2B,
  KB_SPACE = 0x2C,
  KB_MINUS = 0x2D,
  KB_PLUS = 0x2E,
  KB_UP_ARROW = 0x52,
  KB_LEFT_ARROW = 0x50,
  KB_DOWN_ARROW = 0x51,
  KB_RIGHT_ARROW = 0x4F,
  KB_FUNCTION = 0xDE,
  KB_LEFT_CONTROL = 0xE0,
  KB_LEFT_SHIFT = 0xE1,
  KB_LEFT_ALT = 0xE2,
  KB_LEFT_GUI = 0xE3,
  KB_RIGHT_CONTROL = 0xE4,
  KB_RIGHT_SHIFT = 0xE5,
  KB_RIGHT_ALT = 0xE6,
  KB_RIGHT_GUI = 0xE7
};

#define MAX_KEYBOARD_STATE_KEYS 255
typedef struct KeyboardState {
  uint8_t keysDowned;
  bool downedKeys[MAX_KEYBOARD_STATE_KEYS]; // TODO: More compact storage?
  uint8_t frameStateChanges[MAX_KEYBOARD_STATE_KEYS];
  uint8_t modifiers;
} KeyboardState;

#define KEY_STATE_CHANGED(ST, KEY) (!!ST->frameStateChanges[KEY])
#define KEY_STATE_UNCHANGED(ST, KEY) (!ST->frameStateChanges[KEY])
#define KEY_IS_DOWN(ST, KEY) (ST->downedKeys[KEY])
#define KEY_IS_UP(ST, KEY) (!KEY_IS_DOWN(ST, KEY))
#define KEY_WAS_PRESSED(ST, KEY) (KEY_IS_DOWN(ST, KEY) && KEY_STATE_CHANGED(ST, KEY))
#define KEY_WAS_RELEASED(ST, KEY) (KEY_IS_UP(ST, KEY) && KEY_STATE_CHANGED(ST, KEY))
