
#define MOUSE_BUTTON_MASK(BTN) (1 << ((uint8_t) BTN))
#define MOUSE_BUTTON_SET_DOWNED(ST, BTN) (ST->downedButtons |= MOUSE_BUTTON_MASK(BTN))
#define MOUSE_BUTTON_SET_RELEASED(ST, BTN) (ST->downedButtons &= ~MOUSE_BUTTON_MASK(BTN))

static inline void mouse_state_clear_frame_changes(MouseState *state)
{
  memset(&state->frameStateChanges, 0, sizeof(state->frameStateChanges));
}

static inline void mouse_state_set_position(MouseState *state, float posX, float posY)
{
  state->frameDx = posX - state->windowX;
  state->frameDy = posY - state->windowY;
  state->windowX = posX;
  state->windowY = posY;
}

static inline void mouse_state_button_down(MouseState *state, uint8_t button)
{
  if (button >= MAX_MOUSE_BUTTONS) {
    return;
  }

  state->frameStateChanges[button]++;
  MOUSE_BUTTON_SET_DOWNED(state, button);
}

static inline void mouse_state_button_up(MouseState *state, uint8_t button)
{
  if (button >= MAX_MOUSE_BUTTONS) {
    return;
  }

  state->frameStateChanges[button]++;
  MOUSE_BUTTON_SET_RELEASED(state, button);
}
