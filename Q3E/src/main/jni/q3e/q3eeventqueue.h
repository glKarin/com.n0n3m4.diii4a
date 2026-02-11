#ifndef _Q3E_EVENTQUEUE_H
#define _Q3E_EVENTQUEUE_H

typedef void (* SendAnalog_f)(int down, float x, float y);
typedef void (* SendKey_f)(int down, int keycode, int charcode);
typedef void (* SendMotion_f)(float deltax, float deltay);
typedef void (* SendMouse_f)(float x, float y, int relativeMode);

#ifdef __cplusplus
extern "C" {
#endif
void Q3E_InitEventManager(SendKey_f sendKey, SendMotion_f sendMotion, SendAnalog_f sendAnalog, SendMouse_f sendMouse);
void Q3E_ShutdownEventManager();
int Q3E_EventManagerIsInitialized();
int Q3E_PullEvent(int num);
void Q3E_PushKeyEvent(int down, int keycode, int charcode);
void Q3E_PushMotionEvent(float deltax, float deltay);
void Q3E_PushAnalogEvent(int down, float x, float y);
void Q3E_PushMouseEvent(float x, float y, int relativeMode);

#ifdef __cplusplus
};
#endif

#endif // _Q3E_EVENTQUEUE_H
