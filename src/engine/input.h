

#ifndef ENGINE_INPUT_H
#define ENGINE_INPUT_H

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <stdbool.h>

#define INPUT_MAX_MOUSE_BUTTONS 16

typedef struct {
    bool down;
    bool pressed;
    bool released;
} InputButton;

typedef struct {
    InputButton keys[SDL_SCANCODE_COUNT];
    InputButton buttons[INPUT_MAX_MOUSE_BUTTONS];

    float mouseX, mouseY;
    float mouseDX, mouseDY;
    float scrollX, scrollY;
} Input;

void Input_Init(void);
void Input_BeginFrame(void);
void Input_HandleEvent(const SDL_Event *event);

bool Input_KeyDown(SDL_Scancode key);
bool Input_KeyPressed(SDL_Scancode key);
bool Input_KeyReleased(SDL_Scancode key);

bool Input_MouseDown(int button);
bool Input_MousePressed(int button);
bool Input_MouseReleased(int button);

void Input_MousePosition(float *x, float *y);
void Input_MouseDelta(float *x, float *y);
void Input_MouseScroll(float *x, float *y);

#endif // ENGINE_INPUT_H
