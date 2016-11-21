#include <stdint.h>

typedef uint8_t kb_t;
enum {
  KB_NONE = 0,
  KB_W = 0x1A,
  KB_A = 0x04,
  KB_S = 0x16,
  KB_D = 0x07,
  KB_MINUS = 0x2D,
  KB_PLUS = 0x2E,
  KB_ESCAPE = 0x29,
  KB_SPACEBAR = 0x2C,
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
  uint8_t modifiers;
} KeyboardState;

