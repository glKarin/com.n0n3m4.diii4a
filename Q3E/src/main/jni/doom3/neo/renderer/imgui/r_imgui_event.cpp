#include "imgui.h"

static float im_cursorX = 0;
static float im_cursorY = 0;

#define IMGUI_WINDOW_WIDTH glConfig.vidWidth
#define IMGUI_WINDOW_HEIGHT glConfig.vidHeight

void R_ImGui_SyncMousePosition(float dx, float dy)
{
    im_cursorX += dx;
    im_cursorY += dy;
    if(im_cursorX < 0)
        im_cursorX = 0;
    else if(im_cursorX >= IMGUI_WINDOW_WIDTH)
        im_cursorX = IMGUI_WINDOW_WIDTH - 1;
    if(im_cursorY < 0)
        im_cursorY = 0;
    else if(im_cursorY >= IMGUI_WINDOW_HEIGHT)
        im_cursorY = IMGUI_WINDOW_HEIGHT - 1;
}

void ImGui_MouseMotionEvent(float dx, float dy)
{
#ifdef _MULTITHREAD
    if(multithreadActive)
    {
        R_ImGui_PushMouseEvent(im_cursorX, im_cursorY);
    }
    else
#endif
    {
        ImGuiIO& io = ImGui::GetIO();
        io.AddMousePosEvent(im_cursorX, im_cursorY);
    }
}

ImGuiKey ImGui_ConvertKeyEvent(int key)
{
#define D3_TO_IMGUI_KEY(dk, ik) case K_##dk: imkey = ImGuiKey_##ik; break;
    ImGuiKey imkey;
    switch(key)
    {
        D3_TO_IMGUI_KEY(MOUSE1, MouseLeft);
        D3_TO_IMGUI_KEY(MOUSE2, MouseRight);
        D3_TO_IMGUI_KEY(MOUSE3, MouseMiddle);
        D3_TO_IMGUI_KEY(MOUSE4, MouseX1);
        D3_TO_IMGUI_KEY(MOUSE5, MouseX2);
        D3_TO_IMGUI_KEY(ENTER, Enter);
        D3_TO_IMGUI_KEY(UPARROW, UpArrow);
        D3_TO_IMGUI_KEY(DOWNARROW, DownArrow);
        D3_TO_IMGUI_KEY(LEFTARROW, LeftArrow);
        D3_TO_IMGUI_KEY(RIGHTARROW, RightArrow);
        D3_TO_IMGUI_KEY(TAB, Tab);
        D3_TO_IMGUI_KEY(ESCAPE, Escape);
        D3_TO_IMGUI_KEY(SPACE, Space);
        D3_TO_IMGUI_KEY(BACKSPACE, Backspace);
        D3_TO_IMGUI_KEY(INS, Insert);
        D3_TO_IMGUI_KEY(DEL, Delete);
        D3_TO_IMGUI_KEY(PGDN, PageUp);
        D3_TO_IMGUI_KEY(PGUP, PageDown);
        D3_TO_IMGUI_KEY(HOME, Home);
        D3_TO_IMGUI_KEY(END, End);
        D3_TO_IMGUI_KEY(F1, F1);
        D3_TO_IMGUI_KEY(F2, F2);
        D3_TO_IMGUI_KEY(F3, F3);
        D3_TO_IMGUI_KEY(F4, F4);
        D3_TO_IMGUI_KEY(F5, F5);
        D3_TO_IMGUI_KEY(F6, F6);
        D3_TO_IMGUI_KEY(F7, F7);
        D3_TO_IMGUI_KEY(F8, F8);
        D3_TO_IMGUI_KEY(F9, F9);
        D3_TO_IMGUI_KEY(F10, F10);
        D3_TO_IMGUI_KEY(F11, F11);
        D3_TO_IMGUI_KEY(F12, F12);
        D3_TO_IMGUI_KEY(F13, F13);
        D3_TO_IMGUI_KEY(F14, F14);
        D3_TO_IMGUI_KEY(F15, F15);
        D3_TO_IMGUI_KEY(KP_HOME, Keypad7);
        D3_TO_IMGUI_KEY(KP_UPARROW, Keypad8);
        D3_TO_IMGUI_KEY(KP_PGUP, Keypad9);
        D3_TO_IMGUI_KEY(KP_LEFTARROW, Keypad4);
        D3_TO_IMGUI_KEY(KP_5, Keypad5);
        D3_TO_IMGUI_KEY(KP_RIGHTARROW, Keypad6);
        D3_TO_IMGUI_KEY(KP_END, Keypad1);
        D3_TO_IMGUI_KEY(KP_DOWNARROW, Keypad2);
        D3_TO_IMGUI_KEY(KP_PGDN, Keypad3);
        D3_TO_IMGUI_KEY(KP_INS, Keypad0);
        D3_TO_IMGUI_KEY(KP_DEL, KeypadDecimal);
        D3_TO_IMGUI_KEY(KP_SLASH, KeypadDivide);
        D3_TO_IMGUI_KEY(KP_PLUS, KeypadAdd);
        D3_TO_IMGUI_KEY(KP_MINUS, KeypadSubtract);
        D3_TO_IMGUI_KEY(KP_STAR, KeypadMultiply);
        D3_TO_IMGUI_KEY(KP_ENTER, KeypadEnter);
        D3_TO_IMGUI_KEY(KP_EQUALS, KeypadEqual);
        D3_TO_IMGUI_KEY(ALT, LeftAlt);
        D3_TO_IMGUI_KEY(CTRL, LeftCtrl);
        D3_TO_IMGUI_KEY(SHIFT, LeftShift);
        D3_TO_IMGUI_KEY(PAUSE, Pause);
        D3_TO_IMGUI_KEY(CAPSLOCK, CapsLock);
        D3_TO_IMGUI_KEY(SCROLL, ScrollLock);
        D3_TO_IMGUI_KEY(RIGHT_ALT, RightAlt);
        D3_TO_IMGUI_KEY(PRINT_SCR, PrintScreen);
        default:
            if(key >= '0' && key <= 'Z')
            {
                imkey = (ImGuiKey)(key - '0' + ImGuiKey_0);
            }
            else
                imkey = ImGuiKey_None;
            break;
    }

    return imkey;
}

void ImGui_KeyEvent(ImGuiKey key, bool state)
{
#ifdef _MULTITHREAD
    if(multithreadActive)
    {
        R_ImGui_PushKeyEvent(key, state);
    }
    else
#endif
    {
        ImGuiIO& io = ImGui::GetIO();
        if(key >= ImGuiKey_MouseLeft && key <= ImGuiKey_MouseX2)
            io.AddMouseButtonEvent(key - ImGuiKey_MouseLeft, state);
        else
            io.AddKeyEvent(key, state);
    }
}

void ImGui_MouseWheelEvent(float wx, float wy)
{
#ifdef _MULTITHREAD
    if(multithreadActive)
    {
        R_ImGui_PushWheelEvent(wx, wy);
    }
    else
#endif
    {
        ImGuiIO& io = ImGui::GetIO();
        io.AddMouseWheelEvent(wx, wy);
    }
}

void ImGui_InputEvent(char ch)
{
#ifdef _MULTITHREAD
    if(multithreadActive)
    {
        R_ImGui_PushInputEvent(ch);
    }
    else
#endif
    {
        ImGuiIO& io = ImGui::GetIO();
        io.AddInputCharacter(ch);
    }
}

void ImGui_HandleEvent(const sysEvent_t *ev)
{
    if(!R_ImGui_IsInitialized())
        return;
    ImGuiKey imkey;
    IG_LOCK();
    switch (ev->evType) {
        case SE_MOUSE:
            R_ImGui_SyncMousePosition(ev->evValue, ev->evValue2);
            R_ImGui_PushMouseEvent(im_cursorX, im_cursorY);
            break;
        case SE_KEY:
            imkey = ImGui_ConvertKeyEvent(ev->evValue);
            if(imkey != ImGuiKey_None)
                R_ImGui_PushKeyEvent(imkey, ev->evValue2);
            break;
        case SE_CHAR:
            R_ImGui_PushInputEvent(ev->evValue);
            break;
        default:
            break;
    }
    IG_UNLOCK();
}
