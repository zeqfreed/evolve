#include <stdint.h>

static kb_t keyCodesMap[255] = {
  KB_A,			// kVK_ANSI_A (0x0)
  KB_S,			// kVK_ANSI_S (0x1)
  KB_D,			// kVK_ANSI_D (0x2)
  KB_F,			// kVK_ANSI_F (0x3)
  KB_H,			// kVK_ANSI_H (0x4)
  KB_G,			// kVK_ANSI_G (0x5)
  KB_Z,			// kVK_ANSI_Z (0x6)
  KB_X,			// kVK_ANSI_X (0x7)
  KB_C,			// kVK_ANSI_C (0x8)
  KB_V,			// kVK_ANSI_V (0x9)
  0,			  // 0xA
  KB_B,			// kVK_ANSI_B (0xB)
  KB_Q,			// kVK_ANSI_Q (0xC)
  KB_W,			// kVK_ANSI_W (0xD)
  KB_E,			// kVK_ANSI_E (0xE)
  KB_R,			// kVK_ANSI_R (0xF)
  KB_Y,			// kVK_ANSI_Y (0x10)
  KB_T,			// kVK_ANSI_T (0x11)
  KB_1,			// kVK_ANSI_1 (0x12)
  KB_2,			// kVK_ANSI_2 (0x13)
  KB_3,			// kVK_ANSI_3 (0x14)
  KB_4,			// kVK_ANSI_4 (0x15)
  KB_6,			// kVK_ANSI_6 (0x16)
  KB_5,			// kVK_ANSI_5 (0x17)
  KB_PLUS,		// kVK_ANSI_Equal (0x18)
  KB_9,			// kVK_ANSI_9 (0x19)
  KB_7,			// kVK_ANSI_7 (0x1A)
  KB_MINUS,		// kVK_ANSI_Minus (0x1B)
  KB_8,			// kVK_ANSI_8 (0x1C)
  KB_0,			// kVK_ANSI_0 (0x1D)
  0,			// kVK_ANSI_RightBracket (0x1E)
  KB_O,			// kVK_ANSI_O (0x1F)
  KB_U,			// kVK_ANSI_U (0x20)
  0,			// kVK_ANSI_LeftBracket (0x21)
  KB_I,			// kVK_ANSI_I (0x22)
  KB_P,			// kVK_ANSI_P (0x23)
  0,			// kVK_Return (0x24)
  KB_L,			// kVK_ANSI_L (0x25)
  KB_J,			// kVK_ANSI_J (0x26)
  0,			// kVK_ANSI_Quote (0x27)
  KB_K,			// kVK_ANSI_K (0x28)
  0,			// kVK_ANSI_Semicolon (0x29)
  0,			// kVK_ANSI_Backslash (0x2A)
  0,			// kVK_ANSI_Comma (0x2B)
  0,			// kVK_ANSI_Slash (0x2C)
  KB_N,			// kVK_ANSI_N (0x2D)
  KB_M,			// kVK_ANSI_M (0x2E)
  0,			// kVK_ANSI_Period (0x2F)
  KB_TAB,			// kVK_Tab (0x30)
  KB_SPACE,			// kVK_Space (0x31)
  0,			// kVK_ANSI_Grave (0x32)
  0,			// kVK_Delete (0x33)
  0,			// 0x34
  KB_ESCAPE,		// kVK_Escape (0x35)
  0,			// 0x36
  0,			// kVK_Command (0x37)
  0,			// kVK_Shift (0x38)
  0,			// kVK_CapsLock (0x39)
  0,			// kVK_Option (0x3A)
  0,			// kVK_Control (0x3B)
  0,			// kVK_RightShift (0x3C)
  0,			// kVK_RightOption (0x3D)
  0,			// kVK_RightControl (0x3E)
  0,			// kVK_Function (0x3F)
  0,			// kVK_F17 (0x40)
  0,			// kVK_ANSI_KeypadDecimal (0x41)
  0,			// 0x42
  0,			// kVK_ANSI_KeypadMultiply (0x43)
  0,			// 0x44
  0,			// kVK_ANSI_KeypadPlus (0x45)
  0,			// 0x46
  0,			// kVK_ANSI_KeypadClear (0x47)
  0,			// kVK_VolumeUp (0x48)
  0,			// kVK_VolumeDown (0x49)
  0,			// kVK_Mute (0x4A)
  0,			// kVK_ANSI_KeypadDivide (0x4B)
  0,			// kVK_ANSI_KeypadEnter (0x4C)
  0,			// 0x4D
  0,			// kVK_ANSI_KeypadMinus (0x4E)
  0,			// kVK_F18 (0x4F)
  0,			// kVK_F19 (0x50)
  0,			// kVK_ANSI_KeypadEquals (0x51)
  0,			// kVK_ANSI_Keypad0 (0x52)
  0,			// kVK_ANSI_Keypad1 (0x53)
  0,			// kVK_ANSI_Keypad2 (0x54)
  0,			// kVK_ANSI_Keypad3 (0x55)
  0,			// kVK_ANSI_Keypad4 (0x56)
  0,			// kVK_ANSI_Keypad5 (0x57)
  0,			// kVK_ANSI_Keypad6 (0x58)
  0,			// kVK_ANSI_Keypad7 (0x59)
  0,			// kVK_F20 (0x5A)
  0,			// kVK_ANSI_Keypad8 (0x5B)
  0,			// kVK_ANSI_Keypad9 (0x5C)
  0,			// 0x5D
  0,			// 0x5E
  0,			// 0x5F
  0,			// kVK_F5 (0x60)
  0,			// kVK_F6 (0x61)
  0,			// kVK_F7 (0x62)
  0,			// kVK_F3 (0x63)
  0,			// kVK_F8 (0x64)
  0,			// kVK_F9 (0x65)
  0,			// 0x66
  0,			// kVK_F11 (0x67)
  0,			// 0x68
  0,			// kVK_F13 (0x69)
  0,			// kVK_F16 (0x6A)
  0,			// kVK_F14 (0x6B)
  0,			// 0x6C
  0,			// kVK_F10 (0x6D)
  0,			// 0x6E
  0,			// kVK_F12 (0x6F)
  0,			// 0x70
  0,			// kVK_F15 (0x71)
  0,			// kVK_Help (0x72)
  0,			// kVK_Home (0x73)
  0,			// kVK_PageUp (0x74)
  0,			// kVK_ForwardDelete (0x75)
  0,			// kVK_F4 (0x76)
  0,			// kVK_End (0x77)
  0,			// kVK_F2 (0x78)
  0,			// kVK_PageDown (0x79)
  0,			// kVK_F1 (0x7A)
  KB_LEFT_ARROW,	// kVK_LeftArrow (0x7B)
  KB_RIGHT_ARROW,	// kVK_RightArrow (0x7C)
  KB_DOWN_ARROW,	// kVK_DownArrow (0x7D)
  KB_UP_ARROW	// kVK_UpArrow (0x7E)
};

#define KEYBOARD_CODE(code) (keyCodesMap[code])

static inline void keyboard_clear_state_changes(KeyboardState *state)
{
  memset(&state->frameStateChanges, 0, sizeof(state->frameStateChanges));
}

static inline void keyboard_state_key_down(KeyboardState *state, kb_t kbCode)
{
  if (kbCode) {
    state->keysDowned++;
    state->frameStateChanges[kbCode]++;
    state->downedKeys[kbCode] = true;
  }
}

static inline void keyboard_state_key_up(KeyboardState *state, kb_t kbCode)
{
  if (kbCode) {
    state->keysDowned--;
    state->frameStateChanges[kbCode]++;
    state->downedKeys[kbCode] = false;
  }
}
