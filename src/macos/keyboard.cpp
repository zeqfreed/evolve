
static kb_t KEY_CODE_MAP[255] = {
  KB_A,			// kVK_ANSI_A (0x0)
  KB_S,			// kVK_ANSI_S (0x1)
  KB_D,			// kVK_ANSI_D (0x2)
  0,			// kVK_ANSI_F (0x3)
  0,			// kVK_ANSI_H (0x4)
  0,			// kVK_ANSI_G (0x5)
  0,			// kVK_ANSI_Z (0x6)
  0,			// kVK_ANSI_X (0x7)
  0,			// kVK_ANSI_C (0x8)
  0,			// kVK_ANSI_V (0x9)
  0,			// 0xA
  0,			// kVK_ANSI_B (0xB)
  0,			// kVK_ANSI_Q (0xC)
  KB_W,			// kVK_ANSI_W (0xD)
  0,			// kVK_ANSI_E (0xE)
  0,			// kVK_ANSI_R (0xF)
  0,			// kVK_ANSI_Y (0x10)
  0,			// kVK_ANSI_T (0x11)
  0,			// kVK_ANSI_1 (0x12)
  0,			// kVK_ANSI_2 (0x13)
  0,			// kVK_ANSI_3 (0x14)
  0,			// kVK_ANSI_4 (0x15)
  0,			// kVK_ANSI_6 (0x16)
  0,			// kVK_ANSI_5 (0x17)
  0,			// kVK_ANSI_Equal (0x18)
  0,			// kVK_ANSI_9 (0x19)
  0,			// kVK_ANSI_7 (0x1A)
  0,			// kVK_ANSI_Minus (0x1B)
  0,			// kVK_ANSI_8 (0x1C)
  0,			// kVK_ANSI_0 (0x1D)
  0,			// kVK_ANSI_RightBracket (0x1E)
  0,			// kVK_ANSI_O (0x1F)
  0,			// kVK_ANSI_U (0x20)
  0,			// kVK_ANSI_LeftBracket (0x21)
  0,			// kVK_ANSI_I (0x22)
  0,			// kVK_ANSI_P (0x23)
  0,			// kVK_Return (0x24)
  0,			// kVK_ANSI_L (0x25)
  0,			// kVK_ANSI_J (0x26)
  0,			// kVK_ANSI_Quote (0x27)
  0,			// kVK_ANSI_K (0x28)
  0,			// kVK_ANSI_Semicolon (0x29)
  0,			// kVK_ANSI_Backslash (0x2A)
  0,			// kVK_ANSI_Comma (0x2B)
  0,			// kVK_ANSI_Slash (0x2C)
  0,			// kVK_ANSI_N (0x2D)
  0,			// kVK_ANSI_M (0x2E)
  0,			// kVK_ANSI_Period (0x2F)
  0,			// kVK_Tab (0x30)
  0,			// kVK_Space (0x31)
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

void keyboard_state_key_down(KeyboardState *state, kb_t kbCode)
{
  if (kbCode) {
    state->keysDowned++;
    state->downedKeys[kbCode] = true;
  }
}

void keyboard_state_key_up(KeyboardState *state, kb_t kbCode)
{
  if (kbCode) {
    state->keysDowned--;
    state->downedKeys[kbCode] = false;
  }
}
