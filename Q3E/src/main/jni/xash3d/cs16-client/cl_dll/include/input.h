#pragma once
#ifndef INPUT_H
#define INPUT_H

extern kbutton_t	in_strafe;
extern kbutton_t	in_mlook;
extern kbutton_t	in_speed;
extern kbutton_t	in_jlook;
extern kbutton_t	in_forward;
extern kbutton_t	in_back;
extern kbutton_t	in_moveleft;
extern kbutton_t	in_moveright;

extern cvar_t	*m_pitch;
extern cvar_t	*m_yaw;
extern cvar_t	*m_forward;
extern cvar_t	*m_side;

extern cvar_t *lookstrafe;
extern cvar_t *lookspring;
extern cvar_t *cl_pitchdown;
extern cvar_t *cl_pitchup;
extern cvar_t *cl_yawspeed;
extern cvar_t *cl_sidespeed;
extern cvar_t *cl_forwardspeed;
extern cvar_t *cl_pitchspeed;
extern cvar_t *cl_movespeedkey;

#endif // INPUT_H
