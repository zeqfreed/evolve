
static kb_t keyCodesMap[256] = {
  0,
  0, 		        // VK_LBUTTON Left Button **
  0, 		        // VK_RBUTTON Right Button **
  0, 		        // VK_CANCEL Break
  0, 		        // VK_MBUTTON Middle Button **
  0, 		        // VK_XBUTTON1 X Button 1 **
  0, 		        // VK_XBUTTON2 X Button 2 **
  0,
  KB_BACKSPACE,     // VK_BACK Backspace
  KB_TAB,       	// VK_TAB Tab
  0,
  0,
  0, 		        // VK_CLEAR Clear
  KB_RETURN,        // VK_RETURN Enter
  0,
  0,
  0,
  0,
  0,
  0, 		        // VK_PAUSE Pause
  0, 		        // VK_CAPITAL Caps Lock
  0, 		        // VK_KANA Kana
  0,
  0, 		        // VK_JUNJA Junja
  0, 		        // VK_FINAL Final
  0, 		        // VK_KANJI Kanji
  0,
  KB_ESCAPE, 		// VK_ESCAPE Esc
  0, 		        // VK_CONVERT Convert
  0, 		        // VK_NONCONVERT Non Convert
  0, 		        // VK_ACCEPT Accept
  0, 		        // VK_MODECHANGE Mode Change
  KB_SPACE,         // VK_SPACE Space
  0, 		        // VK_PRIOR Page Up
  0, 		        // VK_NEXT Page Down
  0, 		        // VK_END End
  0, 		        // VK_HOME Home
  KB_LEFT_ARROW, 	// VK_LEFT Arrow Left
  KB_UP_ARROW, 		// VK_UP Arrow Up
  KB_RIGHT_ARROW, 	// VK_RIGHT Arrow Right
  KB_DOWN_ARROW, 	// VK_DOWN Arrow Down
  0, 		        // VK_SELECT Select
  0, 		        // VK_PRINT Print
  0, 		        // VK_EXECUTE Execute
  0, 		        // VK_SNAPSHOT Print Screen
  0, 		        // VK_INSERT Insert
  0, 		        // VK_DELETE Delete
  0, 		        // VK_HELP Help
  KB_0, 		    // VK_KEY_0 0
  KB_1, 		    // VK_KEY_1 1
  KB_2, 		    // VK_KEY_2 2
  KB_3, 		    // VK_KEY_3 3
  KB_4, 		    // VK_KEY_4 4
  KB_5, 		    // VK_KEY_5 5
  KB_6, 		    // VK_KEY_6 6
  KB_7, 		    // VK_KEY_7 7
  KB_8, 		    // VK_KEY_8 8
  KB_9, 		    // VK_KEY_9 9
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  KB_A, 		    // VK_KEY_A A
  KB_B, 		    // VK_KEY_B B
  KB_C, 		    // VK_KEY_C C
  KB_D, 		    // VK_KEY_D D
  KB_E, 		    // VK_KEY_E E
  KB_F, 		    // VK_KEY_F F
  KB_G, 		    // VK_KEY_G G
  KB_H, 		    // VK_KEY_H H
  KB_I, 		    // VK_KEY_I I
  KB_J, 		    // VK_KEY_J J
  KB_K, 		    // VK_KEY_K K
  KB_L, 		    // VK_KEY_L L
  KB_M, 		    // VK_KEY_M M
  KB_N, 		    // VK_KEY_N N
  KB_O, 		    // VK_KEY_O O
  KB_P, 		    // VK_KEY_P P
  KB_Q, 		    // VK_KEY_Q Q
  KB_R, 		    // VK_KEY_R R
  KB_S, 		    // VK_KEY_S S
  KB_T, 		    // VK_KEY_T T
  KB_U, 		    // VK_KEY_U U
  KB_V, 		    // VK_KEY_V V
  KB_W, 		    // VK_KEY_W W
  KB_X, 		    // VK_KEY_X X
  KB_Y, 		    // VK_KEY_Y Y
  KB_Z, 		    // VK_KEY_Z Z
  KB_LEFT_GUI,      // VK_LWIN Left Win
  KB_RIGHT_GUI, 	// VK_RWIN Right Win
  0, 		        // VK_APPS Context Menu
  0,
  0, 		        // VK_SLEEP Sleep
  0, 		        // VK_NUMPAD0 Numpad 0
  0, 		        // VK_NUMPAD1 Numpad 1
  0, 		        // VK_NUMPAD2 Numpad 2
  0, 		        // VK_NUMPAD3 Numpad 3
  0, 		        // VK_NUMPAD4 Numpad 4
  0, 		        // VK_NUMPAD5 Numpad 5
  0, 		        // VK_NUMPAD6 Numpad 6
  0, 		        // VK_NUMPAD7 Numpad 7
  0, 		        // VK_NUMPAD8 Numpad 8
  0, 		        // VK_NUMPAD9 Numpad 9
  0, 		        // VK_MULTIPLY Numpad *
  0, 		        // VK_ADD Numpad +
  0, 		        // VK_SEPARATOR Separator
  0, 		        // VK_SUBTRACT Num -
  0, 		        // VK_DECIMAL Numpad .
  0, 		        // VK_DIVIDE Numpad /
  0, 		        // VK_F1 F1
  0, 		        // VK_F2 F2
  0, 		        // VK_F3 F3
  0, 		        // VK_F4 F4
  0, 		        // VK_F5 F5
  0, 		        // VK_F6 F6
  0, 		        // VK_F7 F7
  0, 		        // VK_F8 F8
  0, 		        // VK_F9 F9
  0, 		        // VK_F10 F10
  0, 		        // VK_F11 F11
  0, 		        // VK_F12 F12
  0, 		        // VK_F13 F13
  0, 		        // VK_F14 F14
  0, 		        // VK_F15 F15
  0, 		        // VK_F16 F16
  0, 		        // VK_F17 F17
  0, 		        // VK_F18 F18
  0, 		        // VK_F19 F19
  0, 		        // VK_F20 F20
  0, 		        // VK_F21 F21
  0, 		        // VK_F22 F22
  0, 		        // VK_F23 F23
  0, 		        // VK_F24 F24
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0, 		            // VK_NUMLOCK Num Lock
  0, 		            // VK_SCROLL Scrol Lock
  0, 		            // VK_OEM_FJ_JISHO Jisho
  0, 		            // VK_OEM_FJ_MASSHOU Mashu
  0, 		            // VK_OEM_FJ_TOUROKU Touroku
  0, 		            // VK_OEM_FJ_LOYA Loya
  0, 		            // VK_OEM_FJ_ROYA Roya
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  KB_LEFT_SHIFT, 		// VK_LSHIFT Left Shift
  KB_RIGHT_SHIFT, 		// VK_RSHIFT Right Shift
  KB_LEFT_CONTROL, 		// VK_LCONTROL Left Ctrl
  KB_RIGHT_CONTROL, 	// VK_RCONTROL Right Ctrl
  KB_LEFT_ALT, 		    // VK_LMENU Left Alt
  KB_RIGHT_ALT, 		// VK_RMENU Right Alt
  0, 		            // VK_BROWSER_BACK Browser Back
  0, 		            // VK_BROWSER_FORWARD Browser Forward
  0, 		            // VK_BROWSER_REFRESH Browser Refresh
  0, 		            // VK_BROWSER_STOP Browser Stop
  0, 		            // VK_BROWSER_SEARCH Browser Search
  0, 		            // VK_BROWSER_FAVORITES Browser Favorites
  0, 		            // VK_BROWSER_HOME Browser Home
  0, 		            // VK_VOLUME_MUTE Volume Mute
  0, 		            // VK_VOLUME_DOWN Volume Down
  0, 		            // VK_VOLUME_UP Volume Up
  0, 		            // VK_MEDIA_NEXT_TRACK Next Track
  0, 		            // VK_MEDIA_PREV_TRACK Previous Track
  0, 		            // VK_MEDIA_STOP Stop
  0, 		            // VK_MEDIA_PLAY_PAUSE Play / Pause
  0, 		            // VK_LAUNCH_MAIL Mail
  0, 		            // VK_LAUNCH_MEDIA_SELECT Media
  0, 		            // VK_LAUNCH_APP1 App1
  0, 		            // VK_LAUNCH_APP2 App2
  0,
  0,
  0, 		            // VK_OEM_1 OEM_1 (: ;)
  KB_PLUS, 	            // VK_OEM_PLUS OEM_PLUS (+ =)
  0, 		            // VK_OEM_COMMA OEM_COMMA (< ,)
  KB_MINUS,             // VK_OEM_MINUS OEM_MINUS (_ -)
  0, 		            // VK_OEM_PERIOD OEM_PERIOD (> .)
  0, 		            // VK_OEM_2 OEM_2 (? /)
  0, 		            // VK_OEM_3 OEM_3 (~ `)
  0, 		            // VK_ABNT_C1 Abnt C1
  0, 		            // VK_ABNT_C2 Abnt C2
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0, 		            // VK_OEM_4 OEM_4 ({ [)
  0, 		            // VK_OEM_5 OEM_5 (| \)
  0, 		            // VK_OEM_6 OEM_6 (} ])
  0, 		            // VK_OEM_7 OEM_7 (" ')
  0, 		            // VK_OEM_8 OEM_8 (§ !)
  0,
  0, 		            // VK_OEM_AX Ax
  0, 		            // VK_OEM_102 OEM_102 (> <)
  0, 		            // VK_ICO_HELP IcoHlp
  0, 		            // VK_ICO_00 Ico00 *
  0, 		            // VK_PROCESSKEY Process
  0, 		            // VK_ICO_CLEAR IcoClr
  0, 		            // VK_PACKET Packet
  0,
  0, 		            // VK_OEM_RESET Reset
  0, 		            // VK_OEM_JUMP Jump
  0, 		            // VK_OEM_PA1 OemPa1
  0, 		            // VK_OEM_PA2 OemPa2
  0, 		            // VK_OEM_PA3 OemPa3
  0, 		            // VK_OEM_WSCTRL WsCtrl
  0, 		            // VK_OEM_CUSEL Cu Sel
  0, 		            // VK_OEM_ATTN Oem Attn
  0, 		            // VK_OEM_FINISH Finish
  0, 		            // VK_OEM_COPY Copy
  0, 		            // VK_OEM_AUTO Auto
  0, 		            // VK_OEM_ENLW Enlw
  0, 		            // VK_OEM_BACKTAB Back Tab
  0, 		            // VK_ATTN Attn
  0, 		            // VK_CRSEL Cr Sel
  0, 		            // VK_EXSEL Ex Sel
  0, 		            // VK_EREOF Er Eof
  0, 		            // VK_PLAY Play
  0, 		            // VK_ZOOM Zoom
  0, 		            // VK_NONAME NoName
  0, 		            // VK_PA1 Pa1
  0, 		            // VK_OEM_CLEAR OemClr
  0 		            // VK__none_ no VK mapping
};

#define KEYBOARD_CODE(code) (keyCodesMap[code])

static inline void keyboard_clear_state_changes(KeyboardState *state)
{
  memset(&state->frameStateChanges, 0, sizeof(state->frameStateChanges));
}

static inline void keyboard_clear_state(KeyboardState *state)
{
  memset(state, 0, sizeof(KeyboardState));
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
