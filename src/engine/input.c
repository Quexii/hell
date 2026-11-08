#include "input.h"

#include <string.h>

static Input input = { 0 };

static bool ValidKey(SDL_Scancode key) {
    return key > SDL_SCANCODE_UNKNOWN && key < SDL_SCANCODE_COUNT;
}

static bool ValidButton(int button) {
    return button > 0 && button < INPUT_MAX_MOUSE_BUTTONS;
}

static void Button_Update(InputButton *button, bool down) {
    if (down) {
        if (!button->down)
            button->pressed = true;
    } else {
        if (button->down)
            button->released = true;
    }

    button->down = down;
}

void Input_Init(void) {
    memset(&input, 0, sizeof(input));
}

void Input_BeginFrame(void) {
    for (int i = 0; i < SDL_SCANCODE_COUNT; i++) {
        input.keys[i].pressed = false;
        input.keys[i].released = false;
    }

    for (int i = 0; i < INPUT_MAX_MOUSE_BUTTONS; i++) {
        input.buttons[i].pressed = false;
        input.buttons[i].released = false;
    }

    input.mouseDX = 0.f;
    input.mouseDY = 0.f;
    input.scrollX = 0.f;
    input.scrollY = 0.f;
}

void Input_HandleEvent(const SDL_Event *event) {
    if (!event)
        return;

    switch (event->type) {
        case SDL_EVENT_KEY_DOWN:
            if (!event->key.repeat && ValidKey(event->key.scancode)) {
                Button_Update(&input.keys[event->key.scancode], true);
            }
            break;

        case SDL_EVENT_KEY_UP:
            if (ValidKey(event->key.scancode)) {
                Button_Update(&input.keys[event->key.scancode], false);
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (ValidButton(event->button.button)) {
                Button_Update(
                    &input.buttons[event->button.button], event->type == SDL_EVENT_MOUSE_BUTTON_DOWN
                );
            }

            input.mouseX = event->button.x;
            input.mouseY = event->button.y;
            break;

        case SDL_EVENT_MOUSE_MOTION:
            input.mouseX = event->motion.x;
            input.mouseY = event->motion.y;
            input.mouseDX += event->motion.xrel;
            input.mouseDY += event->motion.yrel;
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            input.scrollX += event->wheel.x;
            input.scrollY += event->wheel.y;
            break;

        case SDL_EVENT_WINDOW_FOCUS_LOST:
            for (int i = 0; i < SDL_SCANCODE_COUNT; i++)
                Button_Update(&input.keys[i], false);

            for (int i = 0; i < INPUT_MAX_MOUSE_BUTTONS; i++)
                Button_Update(&input.buttons[i], false);
            break;
    }
}

bool Input_KeyDown(SDL_Scancode key) {
    return ValidKey(key) && input.keys[key].down;
}

bool Input_KeyPressed(SDL_Scancode key) {
    return ValidKey(key) && input.keys[key].pressed;
}

bool Input_KeyReleased(SDL_Scancode key) {
    return ValidKey(key) && input.keys[key].released;
}

bool Input_MouseDown(int button) {
    return ValidButton(button) && input.buttons[button].down;
}

bool Input_MousePressed(int button) {
    return ValidButton(button) && input.buttons[button].pressed;
}

bool Input_MouseReleased(int button) {
    return ValidButton(button) && input.buttons[button].released;
}

void Input_MousePosition(float *x, float *y) {
    if (x)
        *x = input.mouseX;
    if (y)
        *y = input.mouseY;
}

void Input_MouseDelta(float *x, float *y) {
    if (x)
        *x = input.mouseDX;
    if (y)
        *y = input.mouseDY;
}

void Input_MouseScroll(float *x, float *y) {
    if (x)
        *x = input.scrollX;
    if (y)
        *y = input.scrollY;
}
