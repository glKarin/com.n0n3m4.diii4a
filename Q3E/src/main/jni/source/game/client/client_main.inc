
typedef struct Q3E_Client_Callback_s
{
	bool (*AnalogEvent)(float *x, float *y);
} Q3E_Client_Callback_t;

static bool (*AnalogEvent_f)(float *x, float *y);

extern ConVar cl_sidespeed;
extern ConVar cl_upspeed;
extern ConVar cl_forwardspeed;
extern ConVar cl_backspeed;

inline void Q3E_AnalogForwardMove1(float &forwardmove)
{
    if(AnalogEvent_f)
    {
        float x, y;
        if(AnalogEvent_f(&x, &y))
        {
		    forwardmove += cl_forwardspeed.GetFloat() * y;
        }
    }
}

inline void Q3E_AnalogForwardMove2(float &forwardmove, float ideal_sin, float ideal_cos)
{
    if(AnalogEvent_f)
    {
        float x, y;
        if(AnalogEvent_f(&x, &y))
        {
		    forwardmove += cl_forwardspeed.GetFloat() * (ideal_cos * y);
        }
    }
}

inline void Q3E_AnalogForwardMove3(float &forwardmove)
{
    if(AnalogEvent_f)
    {
        float x, y;
        if(AnalogEvent_f(&x, &y))
        {
            if(y > 0)
		        forwardmove += cl_forwardspeed.GetFloat() * y;
            else if(y < 0)
		        forwardmove += cl_backspeed.GetFloat() * y;
        }
    }
}

inline void Q3E_AnalogSideMove2(float &sidemove, float ideal_sin, float ideal_cos)
{
    if(AnalogEvent_f)
    {
        float x, y;
        if(AnalogEvent_f(&x, &y))
        {
		    sidemove += cl_sidespeed.GetFloat() * (ideal_cos * x);
        }
    }
}

inline void Q3E_AnalogSideMove3(float &sidemove)
{
    if(AnalogEvent_f)
    {
        float x, y;
        if(AnalogEvent_f(&x, &y))
        {
            sidemove += cl_sidespeed.GetFloat() * x;
        }
    }
}

static inline void Q3E_CalcButtonBits( int& bits, int in_button, bool on )
{
    // special key has pressed
    if(bits & in_button)
        return;

	if ( on )
	{
		bits |= in_button;
	}
}

#define _AVA_DEG 67.5f // 60
#define _INCR_AVA_DEG(x) ((x) + _AVA_DEG)
#define _DECR_AVA_DEG(x) ((x) - _AVA_DEG)
#define GAME_SETUPCMDDIRECTION(x, y, _forward, _backward, _left, _right) \
{ \
	if(y != 0 || x != 0) { \
		float a = (float)atan2(x, y); \
		a = RAD2DEG(a); \
		if (a >= 360.0f || a < 0.0f) { \
			a -= floor(a / 360.0f) * 360.0f; \
			if (a >= 360.0f) { \
				a -= 360.0f; \
			} \
			else if (a < 0.0f) { \
				a += 360.0f; \
			} \
		} \
		_forward = ((a >= 0 && a <= _INCR_AVA_DEG(0)) || (a >= _DECR_AVA_DEG(360) && a <= 360)); \
		_backward = (a >= _DECR_AVA_DEG(180) && a <= _INCR_AVA_DEG(180)); \
		_left = (a > _DECR_AVA_DEG(270) && a < _INCR_AVA_DEG(270)); \
		_right = (a > _DECR_AVA_DEG(90) && a < _INCR_AVA_DEG(90)); \
	} else { \
		_forward = false; \
		_backward = false; \
		_left = false; \
		_right = false; \
	} \
}

inline void Q3E_AnalogMoveButtonBit(int &bits, int bResetState)
{
    if(AnalogEvent_f)
    {
        float x, y;
        if(AnalogEvent_f(&x, &y))
        {
			bool AI_FORWARD, AI_BACKWARD, AI_STRAFE_LEFT, AI_STRAFE_RIGHT;
			GAME_SETUPCMDDIRECTION(x, y, AI_FORWARD, AI_BACKWARD, AI_STRAFE_LEFT, AI_STRAFE_RIGHT);
            Q3E_CalcButtonBits( bits, IN_FORWARD, AI_FORWARD );
            Q3E_CalcButtonBits( bits, IN_BACK, AI_BACKWARD );
            Q3E_CalcButtonBits( bits, IN_MOVERIGHT, AI_STRAFE_RIGHT );
            Q3E_CalcButtonBits( bits, IN_MOVELEFT, AI_STRAFE_LEFT );
        }
    }
}

#define DECLSPEC __attribute__ ((visibility("default")))
extern "C" DECLSPEC
void * Q3E_Client_Interaction(const void *callback)
{
    auto cb = (const Q3E_Client_Callback_t *)callback;
    AnalogEvent_f = cb->AnalogEvent;
    printf("Q3E_Client_Interaction: callback=%p, interface=%p\n", callback, nullptr);
    return nullptr;
}
