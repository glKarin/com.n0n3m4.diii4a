/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "quakedef.h"
#include "winquake.h"
#include "shader.h"
#include "cl_master.h"
#ifdef FTE_TARGET_WEB
#include <emscripten/emscripten.h>
#endif

menu_t *topmenu;
menu_t *promptmenu;

void Menu_KeyEvent(qboolean isdown, int deviceid, int key, int unicode)
{
	if (promptmenu && promptmenu->keyevent)
		promptmenu->keyevent(promptmenu, isdown, deviceid, key, unicode);
	else if (topmenu && topmenu->keyevent)
		topmenu->keyevent(topmenu, isdown, deviceid, key, unicode);
	else if (isdown)
		Key_Dest_Remove(kdm_menu);	//it doesn't want it then...
}
static void Menu_UpdateFocus(void)
{
	if (!promptmenu || !promptmenu->keyevent)
		Key_Dest_Remove(kdm_prompt);	//can't take key presses, so don't take keys.
	if (promptmenu && promptmenu->cursor)
		key_dest_absolutemouse |= kdm_prompt;
	else
		key_dest_absolutemouse &= ~kdm_prompt;

	if (!topmenu || !topmenu->keyevent)
		Key_Dest_Remove(kdm_menu);	//can't take key presses, so don't take keys.
	if (topmenu && topmenu->cursor)
		key_dest_absolutemouse |= kdm_menu;
	else
		key_dest_absolutemouse &= ~kdm_menu;
}
qboolean Menu_IsLinked(menu_t *menu)
{
	menu_t *link;
	for (link = topmenu; link; link = link->prev)
	{
		if (menu == link)
			return true;
	}
	return false;
}
menu_t *Menu_FindContext(void *ctx)
{
	menu_t *link;
	for (link = promptmenu; link; link = link->prev)
	{
		if (link->ctx == ctx)
			return link;
	}
	for (link = topmenu; link; link = link->prev)
	{
		if (link->ctx == ctx)
			return link;
	}
	return NULL;
}
void Menu_Unlink(menu_t *menu, qboolean forced)
{
	menu_t **link;
	for (link = &promptmenu; *link; link = &(*link)->prev)
	{
		if (menu == *link)
		{
			*link = menu->prev;
			if (menu->release)
				menu->release(menu, forced);

			Menu_UpdateFocus();
			return;
		}
	}
	for (link = &topmenu; *link; link = &(*link)->prev)
	{
		if (menu == *link)
		{
			*link = menu->prev;
			if (menu->release)
				menu->release(menu, forced);

			Menu_UpdateFocus();
			return;
		}
	}
}
void Menu_Push(menu_t *menu, qboolean prompt)
{
	if (!Menu_IsLinked(menu))
	{	//only link once.
		//annoying logic so that persistent menus always appear on top of other stuff.
		menu_t **prev = prompt?&promptmenu:&topmenu;
		while (menu->lowpriority && *prev && !(*prev)->lowpriority)
			prev = &(*prev)->prev;
		menu->prev = *prev;
		*prev = menu;
	}
	if (menu == promptmenu)
	{
		if (!Key_Dest_Has(kdm_prompt))
			VRUI_SnapAngle();
		Key_Dest_Add(kdm_prompt);
		Menu_UpdateFocus();
	}
	if (menu == topmenu)
	{
		if (!Key_Dest_Has(kdm_menu))
			VRUI_SnapAngle();
		Key_Dest_Add(kdm_menu);
		Menu_UpdateFocus();
	}
}
void Menu_PopAll(void)
{
	menu_t **menus, *menu;
	size_t count, i;
	//first loop to count them
	for (count = 0, menu = topmenu; menu; menu = menu->prev)
	{
		if (menu->persist)
			continue;
		count++;
	}
	menus = alloca(sizeof(*menus)*count);
	//second loop to track them
	for (i = 0, menu = topmenu; i < count && menu; menu = menu->prev)
	{
		if (menu->persist)
			continue;
		menus[i++] = menu;
	}
	//third link to actually unlink them safely without unlinking multiple times etc (grr menuqc mods re-grabbing focus when closing)
	for (i = 0; i < count; i++)
		Menu_Unlink(menus[i], true);
}

int Menu_WantOSK(void)
{
	if (promptmenu)
		return promptmenu->showosk;
	if (topmenu)
		return topmenu->showosk;
	return -1;
}

void Menu_Draw(void)
{
#ifdef MENU_DAT
	//shitty always-drawn crap
	MP_Draw();
#endif

	//draw whichever menu has focus
	if (topmenu && topmenu->drawmenu)
		topmenu->drawmenu(topmenu);
}
void Prompts_Draw(void)
{
	//prompts always appear over the top of everything else, particuarly other menus.
	if (promptmenu && promptmenu->drawmenu)
		promptmenu->drawmenu(promptmenu);
}

void M_DrawScalePic (int x, int y, int w, int h, mpic_t *pic)
{
	R2D_ScalePic (x + ((vid.width - 320)>>1), y, w, h, pic);
}
void M_DrawTextBox (int x, int y, int width, int lines)
{
	mpic_t	*p;
	int		cx, cy;
	int		n, w;

	// draw left side
	cx = x;
	cy = y;
	p = R2D_SafeCachePic ("gfx/box_tl.lmp");
	switch(R_GetShaderSizes(p, NULL, NULL, false))
	{
	case -1:
		return;	//still pending
	case 0:
		R2D_ImageColours(0.0, 0.0, 0.0, 1.0);
		R2D_FillBlock(x + ((vid.width - 320)>>1), y, width*8+16, lines*8+16);
		R2D_ImageColours(1.0, 1.0, 1.0, 1.0);
		return;
	}
	M_DrawScalePic (cx, cy, 8, 8, p);
	p = R2D_SafeCachePic ("gfx/box_ml.lmp");
	if (p)
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			M_DrawScalePic (cx, cy, 8, 8, p);
		}
	p = R2D_SafeCachePic ("gfx/box_bl.lmp");
	if (p)
		M_DrawScalePic (cx, cy+8, 8, 8, p);

	// draw middle
	cy = y;
	cx += 8;
	//top-strip
	p = R2D_SafeCachePic ("gfx/box_tm.lmp");
	if (p) for (w = 0; w < width; w+=2)
		M_DrawScalePic (cx+w*8, cy, 16, 8, p);

	//just-under-top (shadowed region)
	if (lines)
	{
		cy+=8;
		p = R2D_SafeCachePic ("gfx/box_mm.lmp");
		if (p) for (w = 0; w < width; w+=2)
			M_DrawScalePic (cx+w*8, cy, 16, 8, p);
	}

	//2d body
	p = R2D_SafeCachePic ("gfx/box_mm2.lmp");
	for (n = 1; n < lines; n++)
	{
		cy+=8;
		if (p) for (w = 0; w < width; w+=2)
			M_DrawScalePic (cx+w*8, cy, 16, 8, p);
	}

	//bottom strip
	cy+=8;
	p = R2D_SafeCachePic ("gfx/box_bm.lmp");
	if (p) for (w = 0; w < width; w+=2)
		M_DrawScalePic (cx+w*8, cy, 16, 8, p);

	cx += 8*width;

	// draw right side
	cy = y;
	p = R2D_SafeCachePic ("gfx/box_tr.lmp");
	if (p)
		M_DrawScalePic (cx, cy, 8, 8, p);
	p = R2D_SafeCachePic ("gfx/box_mr.lmp");
	if (p)
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			M_DrawScalePic (cx, cy, 8, 8, p);
		}
	p = R2D_SafeCachePic ("gfx/box_br.lmp");
	if (p)
		M_DrawScalePic (cx, cy+8, 8, 8, p);
}

int QDECL M_FindKeysForBind (int bindmap, const char *command, int *keylist, int *keymods, int keycount)
{
	int		count;
	int		j, m;
	int		l, p;
	char	*b;
	int		firstmod, lastmod;

	l = strlen(command);
	count = 0;

	if (bindmap > 0 && bindmap <= KEY_MODIFIER_ALTBINDMAP)
	{
		//bindmaps don't support modifiers
		firstmod = (bindmap-1)|KEY_MODIFIER_ALTBINDMAP;
		lastmod = firstmod+1;
	}
	else
	{
		firstmod = 0;
		lastmod = KEY_MODIFIER_ALTBINDMAP;
	}

	for (j=0 ; j<K_MAX ; j++)
	{
		for (m = firstmod; m < lastmod; m++)
		{
			b = keybindings[j][m];
			if (!b)
				continue;
			if (!strncmp (b, command, l) && (!b[l] || b[l] == ' ' || b[l] == ';'))
			{
				//if ctrl_a and ctrl_shift_a do the same thing, don't report ctrl_shift_a because its redundant.
				for (p = firstmod; p < m; p++)
				{
					if (p&~m)	//ignore shift_a if we're checking ctrl_a
						continue;
					if (keybindings[j][p] && !strcmp(keybindings[j][p], b))
						break;	//break+continue
				}
				if (p != m)
					continue;

				keylist[count] = j;
				if (keymods)
					keymods[count] = m;
				count++;
				if (count == keycount)
					return count;
			}
		}
	}
	for (j = count; j < keycount; j++)
	{
		keylist[j] = -1;
		if (keymods)
			keymods[j] = 0;
	}
	return count;
}
int M_FindKeysForCommand (int bindmap, int pnum, const char *command, int *keylist, int *keymods, int keycount)
{
	char prefix[5];

	if (*command == '+' || *command == '-')
	{
		prefix[0] = *command;
		prefix[1] = 0;
		if (pnum != 0)
		{
			prefix[1] = 'p';
			prefix[2] = '0'+pnum;
			prefix[3] = ' ';
			prefix[4] = 0;
		}
		command++;
	}
	else
	{
		prefix[0] = 0;
		if (pnum != 0)
		{
			prefix[0] = 'p';
			prefix[1] = '0'+pnum;
			prefix[2] = ' ';
			prefix[3] = 0;
		}
	}
	return M_FindKeysForBind(bindmap, va("%s%s", prefix, command), keylist, keymods, keycount);
}

/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f (void)
{	
	if (topmenu)
	{
		Key_Dest_Add(kdm_menu);
		return;
	}

#ifdef CSQC_DAT
	if (CSQC_ConsoleCommand(-1, "togglemenu"))
	{
		Key_Dest_Remove(kdm_console|kdm_cwindows);
		return;
	}
#endif
#ifdef MENU_DAT
	if (MP_Toggle(1))
	{
		Key_Dest_Remove(kdm_console|kdm_cwindows);
		return;
	}
#endif
#ifdef MENU_NATIVECODE
	if (mn_entry)
	{
		mn_entry->Toggle(1);
		Key_Dest_Remove(kdm_console|kdm_cwindows);
		return;
	}
#endif
#ifdef VM_UI
	if (q3 && q3->ui.OpenMenu())
		return;
#endif

#ifndef NOBUILTINMENUS
	{
		extern cvar_t cl_disconnectreason;
		if (*cl_disconnectreason.string)
		{
			Menu_Prompt(NULL, NULL, cl_disconnectreason.string, NULL, NULL, "Okay", true);
			Cvar_Set(&cl_disconnectreason, "");
		}
	}

	M_Menu_Main_f ();
	Key_Dest_Remove(kdm_console|kdm_cwindows);
#endif
}

/*
================
M_Restart_f
================
*/
void M_Init_Internal (void);
void M_Restart_f(void)
{
	M_Shutdown(false);

	if (!strcmp(Cmd_Argv(1), "off"))
	{	//explicitly restart the engine's menu. not the menuqc crap
		//don't even start csqc menus.
		M_Init_Internal();
	}
	else
		M_Reinit();

	//start up the ui now we have a renderer
#ifdef VM_UI
	if (q3)
		q3->ui.Start();
#endif
}


//=============================================================================
/* Various callback-based prompts */

typedef struct
{
	menu_t m;
	void (*callback)(void *, promptbutton_t);
	void *ctx;

	conchar_t *msg;
	size_t msglen;
	const char *buttons[3];
	int kbutton, mbutton;
	qboolean mousedown;
} promptmenu_t;
static qboolean Prompt_MenuKeyEvent(struct menu_s *gm, qboolean isdown, unsigned int devid, int key, int unicode)
{
	promptmenu_t *m = (promptmenu_t*)gm;
	promptbutton_t action;
	void (*callback)(void *, promptbutton_t) = m->callback;
	void *ctx = m->ctx;

	if (key == K_MOUSE1 || key == K_TOUCHTAP)
	{	//mouse events fire their action on release.
		if (isdown)
		{
			m->mousedown = true;	//so we don't respond to stray release events.
			return true;
		}
		else if (!m->mousedown)
			return false;			//looks like a stray release event. ignore it.
	}
	else
	{	//keyboard events fire on press.
		if (!isdown)
			return false;
	}

	if (key == 'n' || key == 'N')
		action = PROMPT_NO;
	else if (key == 'y' || key == 'Y')
		action = PROMPT_YES;
	else if (key==K_RIGHTARROW || key==K_GP_DPAD_RIGHT || key==K_GP_LEFT_THUMB_RIGHT || key==K_DOWNARROW || key==K_GP_DPAD_DOWN || key==K_GP_LEFT_THUMB_DOWN || key == K_GP_DIAMOND_ALTCONFIRM || (key == K_TAB && !keydown[K_LSHIFT] && !keydown[K_RSHIFT]))
	{
		int start = m->kbutton;
		for(;;)
		{
			m->kbutton++;
			if (start == m->kbutton)
				break;
			if (m->kbutton >= 3)
				m->kbutton -= 3;
			if (m->buttons[m->kbutton])
				break;
		}
		return true;
	}
	else if (key == K_LEFTARROW || key == K_GP_DPAD_LEFT || key==K_GP_LEFT_THUMB_LEFT || key==K_UPARROW || key==K_GP_DPAD_UP || key==K_GP_LEFT_THUMB_UP || key==K_TAB)
	{
		int start = m->kbutton;
		for(;;)
		{
			m->kbutton--;
			if (start == m->kbutton)
				break;
			if (m->kbutton < 0)
				m->kbutton += 3;
			if (m->buttons[m->kbutton])
				break;
		}
		return true;
	}
	else if (key == K_ESCAPE || key == K_GP_BACK || key == K_MOUSE2 || key == K_MOUSE4 || key == K_GP_DIAMOND_CANCEL)
		action = PROMPT_CANCEL;
	else if (key == K_ENTER || key == K_KP_ENTER || key == K_MOUSE1 || key == K_TOUCHTAP || key == K_GP_DIAMOND_CONFIRM)
	{
		int button;
		if (key == K_MOUSE1 || key == K_TOUCHTAP)
			button = m->mbutton;
		else
			button = m->kbutton;

		switch(button)
		{
		case 0:
			action = PROMPT_YES;
			break;
		case 1:
			action = PROMPT_NO;
			break;
		case 2:
			action = PROMPT_CANCEL;
			break;
		default:
			return false;	//nothing focused.
		}
	}
	else
		return false; // no idea what that is

	m->callback = NULL;	//so the remove handler can't fire.
	Menu_Unlink(&m->m, false);
	if (callback)
		callback(ctx, action);

	return true;
}
static void Prompt_Draw(struct menu_s *g)
{
	promptmenu_t *m = (promptmenu_t*)g;
	int x = 64;
	int y = 76;
	int px, py;
	float scale = Font_CharVHeight(font_console);
	int w = 320*scale/8;
	int h, lines;
	int i;
	int bx[4];
	conchar_t *linestart[16];
	conchar_t *lineend[countof(linestart)];

	x = (((int)vid.width-w)>>1);

	Font_BeginString(font_console, w, 0, &px, &py);
	lines = Font_LineBreaks(m->msg, m->msg+m->msglen, px, countof(linestart), linestart, lineend);
	h = (lines+3)*scale;
	Font_EndString(font_console);

	Draw_ApproxTextBox(x, y, w, h);
	y+=scale;

	Font_BeginString(font_console, x, y, &px, &py);
	for (i = 0; i < lines; i++)
	{
		int xoffset = (w*vid.rotpixelwidth/vid.width) - Font_LineWidth(linestart[i], lineend[i]);
		Font_LineDraw(px + xoffset/2, py, linestart[i], lineend[i]);
		py+=Font_CharHeight();
	}
	Font_EndString(font_console);

	y+=scale*lines;
	y+=scale;
	m->mbutton = -1;
	bx[0] = x;
	bx[1] = x+w/3;
	bx[2] = x+w-w/3;
	bx[3] = x+w;
	if (mousecursor_y >= y && mousecursor_y <= y+scale)
	{
		for (i = 0; i < 3; i++)
		{
			if (m->buttons[i] && mousecursor_x >= bx[i] && mousecursor_x < bx[i+1])
				m->mbutton = i;
		}
	}
	for (i = 0; i < 3; i++)
	{
		if (m->buttons[i])
		{
			if (m->mbutton == i)
			{
				float alphamax = 0.5, alphamin = 0.2;
				R2D_ImageColours(.5,.4,0,(sin(realtime*2)+1)*0.5*(alphamax-alphamin)+alphamin);
				R2D_FillBlock(bx[i], y, bx[i+1]-bx[i], scale);
				R2D_ImageColours(1,1,1,1);
			}
			Draw_FunStringWidthFont(font_console, bx[i], y, m->buttons[i], bx[i+1]-bx[i], 2, m->kbutton==i);
		}
	}
}
static void Prompt_Release(struct menu_s *gm, qboolean forced)
{
	promptmenu_t *m = (promptmenu_t*)gm;
	void (*callback)(void *, promptbutton_t) = m->callback;
	void *ctx = m->ctx;
	m->callback = NULL;

	if (callback)
		callback(ctx, PROMPT_CANCEL);
	Z_Free(m);
}
void Menu_Prompt (void (*callback)(void *, promptbutton_t), void *ctx, const char *messages, const char *optionyes, const char *optionno, const char *optioncancel, qboolean highpri)
{
	promptmenu_t *m;
	char *t;
	conchar_t message[8192], *e;

	if (optionyes)
		optionyes = localtext(optionyes);
	if (optionno)
		optionno = localtext(optionno);
	if (optioncancel)
		optioncancel = localtext(optioncancel);

	e = COM_ParseFunString(CON_WHITEMASK, messages, message, sizeof(message)-sizeof(conchar_t), false);

	m = (promptmenu_t*)Z_Malloc(sizeof(*m) + (e-message)*sizeof(conchar_t)+(optionyes?strlen(optionyes):0)+(optionno?strlen(optionno):0)+(optioncancel?strlen(optioncancel):0)+7);

	m->m.cursor = &key_customcursor[kc_console];
	/*void (*videoreset)	(struct menu_s *);	//called after a video mode switch / shader reload.
	void (*release)		(struct menu_s *);	//
	qboolean (*keyevent)(struct menu_s *, qboolean isdown, unsigned int devid, int key, int unicode);	//true if key was handled
	qboolean (*mousemove)(struct menu_s *, qboolean abs, unsigned int devid, float x, float y);
	qboolean (*joyaxis)	(struct menu_s *, unsigned  int devid, int axis, float val);
	void (*drawmenu)	(struct menu_s *);
	*/
	m->m.drawmenu = Prompt_Draw;
	m->m.keyevent = Prompt_MenuKeyEvent;
	m->m.release = Prompt_Release;
	m->mbutton = -1;
	m->kbutton = -1;
	m->m.persist = true;
	Menu_Push(&m->m, highpri);

	m->callback = callback;
	m->ctx = ctx;

	t = (char*)(m+1);
	if (optionyes)
	{
		m->buttons[0] = t;
		strcpy(t, optionyes);
		t += strlen(t)+1;
	}
	if (optionno)
	{
		m->buttons[1] = t;
		strcpy(t, optionno);
		t += strlen(t)+1;
	}
	if (optioncancel)
	{
		m->buttons[2] = t;
		strcpy(t, optioncancel);
		t += strlen(t)+1;
	}

	m->msglen = e-message;
	m->msg = memcpy(t, message, (m->msglen)*sizeof(conchar_t));
}

#ifndef NOBUILTINMENUS

void M_Menu_Audio_f (void);
void M_Menu_Demos_f (void);
void M_Menu_Mods_f (void);
void M_Menu_ModelViewer_f(void);
void M_Menu_ModelViewer_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx);

void M_ConfigureNetSubsystem(void);

cvar_t m_helpismedia = CVAR("m_helpismedia", "0");
cvar_t m_preset_chosen = CVARF("m_preset_chosen", "0", CVAR_ARCHIVE);

//=============================================================================
/* Support Routines */

void M_Print (int cx, int cy, qbyte *str)
{
	Draw_AltFunString(cx + ((vid.width - 320)>>1), cy, str);
}
void M_PrintWhite (int cx, int cy, qbyte *str)
{
	Draw_FunString(cx + ((vid.width - 320)>>1), cy, str);
}

void M_BuildTranslationTable(unsigned int pc, unsigned int top, unsigned int bottom, unsigned int *translationTable)
{
	int		j;
#ifdef HEXEN2
	if (h2playertranslations && pc)
	{
		int i;
		unsigned int color_offsets[5] = {2*14*256,0,1*14*256,2*14*256,2*14*256};
		unsigned char *colorA, *colorB, *sourceA, *sourceB;
		colorA = h2playertranslations + 256 + color_offsets[pc-1];
		colorB = colorA + 256;
		sourceA = colorB + (top * 256);
		sourceB = colorB + (bottom * 256);
		for(i=0;i<255;i++)
		{
			if (bottom > 0 && (colorB[i] != 255))
			{
				if (bottom >= 16)
				{
					unsigned int v = d_8to24rgbtable[colorB[i]];
					v = max(max((v>>0)&0xff, (v>>8)&0xff), (v>>16)&0xff);
					*((unsigned char*)&translationTable[i]+0) = (((bottom&0xff0000)>>16)*v)>>8;
					*((unsigned char*)&translationTable[i]+1) = (((bottom&0x00ff00)>> 8)*v)>>8;
					*((unsigned char*)&translationTable[i]+2) = (((bottom&0x0000ff)>> 0)*v)>>8;
					*((unsigned char*)&translationTable[i]+3) = 0xff;
				}
				else
					translationTable[i] = d_8to24rgbtable[sourceB[i]] | 0xff000000;
			}
			else if (top > 0 && (colorA[i] != 255))
			{
				if (top >= 16)
				{
					unsigned int v = d_8to24rgbtable[colorA[i]];
					v = max(max((v>>0)&0xff, (v>>8)&0xff), (v>>16)&0xff);
					*((unsigned char*)&translationTable[i]+0) = (((top&0xff0000)>>16)*v)>>8;
					*((unsigned char*)&translationTable[i]+1) = (((top&0x00ff00)>> 8)*v)>>8;
					*((unsigned char*)&translationTable[i]+2) = (((top&0x0000ff)>> 0)*v)>>8;
					*((unsigned char*)&translationTable[i]+3) = 0xff;
				}
				else
					translationTable[i] = d_8to24rgbtable[sourceA[i]] | 0xff000000;
			}
			else
				translationTable[i] = d_8to24rgbtable[i] | 0xff000000;
		}
	}
	else
#endif
	{
		for(j=0;j<255;j++)
		{
			if (j >= TOP_RANGE && j < TOP_RANGE + (1<<4))
			{
				if (top >= 16)
				{
					*((unsigned char*)&translationTable[j]+0) = (((top&0xff0000)>>16)**((unsigned char*)&d_8to24rgbtable[j&15]+0))>>8;
					*((unsigned char*)&translationTable[j]+1) = (((top&0x00ff00)>> 8)**((unsigned char*)&d_8to24rgbtable[j&15]+1))>>8;
					*((unsigned char*)&translationTable[j]+2) = (((top&0x0000ff)>> 0)**((unsigned char*)&d_8to24rgbtable[j&15]+2))>>8;
					*((unsigned char*)&translationTable[j]+3) = 0xff;
				}
				else
					translationTable[j] = d_8to24rgbtable[top<8?j-TOP_RANGE+(top<<4):(top<<4)+15-(j-TOP_RANGE)] | 0xff000000;
			}
			else if (j >= BOTTOM_RANGE && j < BOTTOM_RANGE + (1<<4))
			{
				if (bottom >= 16)
				{
					*((unsigned char*)&translationTable[j]+0) = (((bottom&0xff0000)>>16)**((unsigned char*)&d_8to24rgbtable[j&15]+0))>>8;
					*((unsigned char*)&translationTable[j]+1) = (((bottom&0x00ff00)>> 8)**((unsigned char*)&d_8to24rgbtable[j&15]+1))>>8;
					*((unsigned char*)&translationTable[j]+2) = (((bottom&0x0000ff)>> 0)**((unsigned char*)&d_8to24rgbtable[j&15]+2))>>8;
					*((unsigned char*)&translationTable[j]+3) = 0xff;
				}
				else
					translationTable[j] = d_8to24rgbtable[bottom<8?j-BOTTOM_RANGE+(bottom<<4):(bottom<<4)+15-(j-BOTTOM_RANGE)] | 0xff000000;
			}
			else
				translationTable[j] = d_8to24rgbtable[j] | 0xff000000;
		}
	}
	translationTable[255] = 0;	//alpha
}

//=============================================================================

void M_CloseMenu_f (void)
{
	if (!Key_Dest_Has(kdm_menu))
		return;
	M_RemoveAllMenus(false);
}

//=============================================================================
/* KEYS MENU */

typedef struct {
	char *command;
	char *name;
} bindnames_t;

bindnames_t qwbindnames[] =
{
{"+attack", 		"attack        "},
{"impulse 10", 		"change weapon "},
{"impulse 12", 		"prev weapon   "},
{"+jump", 			"jump / swim up"},
{"+forward", 		"walk forward  "},
{"+back", 			"backpedal     "},
{"+left", 			"turn left     "},
{"+right", 			"turn right    "},
{"+speed", 			"run           "},
{"+moveleft", 		"step left     "},
{"+moveright", 		"step right    "},
{"+strafe", 		"sidestep      "},
{"+lookup", 		"look up       "},
{"+lookdown", 		"look down     "},
{"centerview", 		"center view   "},
{"+mlook", 			"mouse look    "},
{"+klook", 			"keyboard look "},
{"+moveup",			"swim up       "},
{"+movedown",		"swim down     "},
#ifdef VOICECHAT
{"+voip",			"voice chat    "},
#endif
{NULL}
};

#ifdef Q2CLIENT
bindnames_t q2bindnames[] =
{
{"+attack", 		"attack        "},
{"cmd weapnext", 	"next weapon   "},
{"cmd weapprev", 	"prev weapon   "},
{"+forward", 		"walk forward  "},
{"+back", 			"backpedal     "},
{"+left", 			"turn left     "},
{"+right", 			"turn right    "},
{"+speed", 			"run           "},
{"+moveleft", 		"step left     "},
{"+moveright", 		"step right    "},
{"+strafe", 		"sidestep      "},
{"+lookup", 		"look up       "},
{"+lookdown", 		"look down     "},
{"centerview", 		"center view   "},
{"+mlook", 			"mouse look    "},
{"+klook", 			"keyboard look "},
{"+moveup",			"up / jump     "},
{"+movedown",		"down / crouch "},

{"cmd inven",		"inventory     "},
{"cmd invuse",		"use item      "},
{"cmd invdrop",		"drop item     "},
{"cmd invprev",		"prev item     "},
{"cmd invnext",		"next item     "},

{"cmd help", 		"help computer "},
{NULL}
};
#endif


bindnames_t h2bindnames[] =
{
{"+attack", 		"attack        "},
{"impulse 10", 		"change weapon "},
{"+jump", 			"jump / swim up"},
{"+forward", 		"walk forward  "},
{"+back", 			"backpedal     "},
{"+left", 			"turn left     "},
{"+right", 			"turn right    "},
{"+speed", 			"run           "},
{"+moveleft", 		"step left     "},
{"+moveright", 		"step right    "},
{"+strafe", 		"sidestep      "},
{"+crouch",			"crouch        "},
{"+lookup", 		"look up       "},
{"+lookdown", 		"look down     "},
{"centerview", 		"center view   "},
{"+mlook", 			"mouse look    "},
{"+klook", 			"keyboard look "},
{"+moveup",			"swim up       "},
{"+movedown",		"swim down     "},
{"impulse 13", 		"lift object   "},
{"invuse",			"use inv item  "},
{"impulse 44",		"drop inv item "},
{"+showinfo",		"full inventory"},
{"+showdm",			"info / frags  "},
//{"toggle_dm",		"toggle frags  "},
{"+infoplaque",		"objectives    "},	//requires pulling info out of the mod... on the client.
{"invleft",			"inv move left "},
{"invright",		"inv move right"},
{"impulse 100",		"inv:torch     "},
{"impulse 101",		"inv:qrtz flask"},
{"impulse 102",		"inv:mystic urn"},
{"impulse 103",		"inv:krater    "},
{"impulse 104",		"inv:chaos devc"},
{"impulse 105",		"inv:tome power"},
{"impulse 106",		"inv:summon stn"},
{"impulse 107",		"inv:invisiblty"},
{"impulse 108",		"inv:glyph     "},
{"impulse 109",		"inv:boots     "},
{"impulse 110",		"inv:repulsion "},
{"impulse 111",		"inv:bo peep   "},
{"impulse 112",		"inv:flight    "},
{"impulse 113",		"inv:force cube"},
{"impulse 114",		"inv:icon defn "},
#ifdef VOICECHAT
{"+voip",			"voice chat    "},
#endif
{NULL}
};

bindnames_t *bindnames;
int numbindnames;

int		keys_cursor;
int		bind_grab;

void M_Menu_Keys_f (void)
{
	int y;
	emenu_t *menu;
	vfsfile_t *bindslist;

	menu = M_CreateMenu(0);
	switch(M_GameType())
	{
#ifdef Q2CLIENT
	case MGT_QUAKE2:
		MC_AddCenterPicture(menu, 0, 24, "pics/m_banner_customize.pcx");
		y = 48;
		bindnames = q2bindnames;
		break;
#endif
#ifdef HEXEN2
	case MGT_HEXEN2:
		MC_AddCenterPicture(menu, 0, 60, "gfx/menu/title6.lmp");
		y = 64;
		bindnames = h2bindnames;
		break;
#endif
	default:
		MC_AddCenterPicture(menu, 4, 24, "gfx/ttl_cstm.lmp");
		y = 48;
		bindnames = qwbindnames;
		break;
	}

#if MAX_SPLITS > 1
	if (cl.splitclients || cl_splitscreen.ival || cl_forceseat.ival)
	{
		static char *texts[MAX_SPLITS+2] =
		{
			"Depends on device",
			"Player 1",
			"Player 2",
#if MAX_SPLITS >= 3
			"Player 3",
#endif
#if MAX_SPLITS >= 4
			"Player 4",
#endif
			NULL
		};
		static char *values[MAX_SPLITS+1] =
		{
			"0",
			"1",
			"2",
#if MAX_SPLITS >= 3
			"3",
#endif
#if MAX_SPLITS >= 4
			"4"
#endif
		};
		MC_AddCvarCombo(menu, 16, 170, y, localtext("Force client"), &cl_forceseat, (const char **)texts, (const char **)values);
		y+=8;
	}
#endif

	bindslist = FS_OpenVFS("bindlist.lst", "rb", FS_GAME);
	if (bindslist)
	{
		char line[1024];
		while(VFS_GETS(bindslist, line, sizeof(line)))
		{
			char *cmd, *desc, *tip;
			Cmd_TokenizeString(line, false, false);
			cmd = Cmd_Argv(0);
			desc = Cmd_Argv(1);
			tip = Cmd_Argv(2);
			if (*cmd)
			{
				if (strcmp(cmd, "-"))	//lines with a command of "-" are spacers/comments.
					MC_AddBind(menu, (320-(int)vid.width)/2, 170, y, desc, cmd, tip);
				else if (*desc)
					MC_AddRedText(menu, (320-(int)vid.width)/2, 170, y, desc, true);
				y += 8;
			}
		}
		VFS_CLOSE(bindslist);
		return;
	}

	MC_AddFrameStart(menu, 48+8);
	while (bindnames->name)
	{
		MC_AddBind(menu, 16, 170, y, localtext(bindnames->name), bindnames->command, NULL);
		y += 8;
		bindnames++;
	}
	MC_AddFrameEnd(menu, 48+8);
}

void M_UnbindCommand (const char *command)
{
	int		j;
	int		l;
	char	*b;
	int m;

	l = strlen(command);

	for (j=0 ; j<K_MAX ; j++)
	{	//FIXME: not sure what to do about bindmaps here. oh well.
		for (m = 0; m < KEY_MODIFIERSTATES; m++)
		{
			b = keybindings[j][m];
			if (!b)
				continue;
			if (!strncmp (b, command, l) )
				Key_SetBinding (j, m, "", RESTRICT_LOCAL);
		}
	}
}

//=============================================================================
/* HELP MENU */

static int		help_page;
static int		num_help_pages;

static struct
{
	char *pattern;
	int base;
} helpstyles[] =
{
	{"gfx/help%i.dxt",0},			//quake extended
	{"gfx/help%i.tga",0},			//quake extended
	{"gfx/help%i.png",0},			//quake extended
	{"gfx/help%i.jpeg",0},			//quake extended
	{"gfx/help%i.lmp",0},			//quake
	{"gfx/menu/help%02i.lmp",1}		//hexen2
};

void M_Help_Draw (emenu_t *m)
{
	int i;
	mpic_t *pic = NULL;
	for (i = 0; i < sizeof(helpstyles)/sizeof(helpstyles[0]) && !pic; i++)
	{
		pic = R2D_SafeCachePic(va(helpstyles[i].pattern, help_page+helpstyles[i].base));
		if (R_GetShaderSizes(pic, NULL, NULL, true) <= 0)
			pic = NULL;
	}
	if (!pic)
	{
		m->postdraw = M_RemoveMenu;
		M_Menu_Main_f ();
	}
	else
	{
		//define default aspect ratio
		int width = 320;
		int height = 200;

		//figure out which axis we're meeting.
		if (vid.width/(float)width > vid.height/(float)height)
		{
			width = width * (vid.height/(float)height);
			height = vid.height;
		}
		else
		{
			height = height * (vid.width/(float)width);
			width = vid.width;
		}
		R2D_ScalePic ((vid.width-width)/2, (vid.height-height)/2, width, height, pic);
	}
}
qboolean M_Help_Key (struct emenu_s *m, int key, unsigned int unicode)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_GP_DIAMOND_CANCEL:
	case K_GP_START:
	case K_MOUSE2:
	case K_MOUSE4:
		M_RemoveMenu(m);
		return true;

	case K_UPARROW:
	case K_RIGHTARROW:
	case K_KP_RIGHTARROW:
	case K_GP_DPAD_RIGHT:
	case K_MOUSE1:
	case K_GP_DIAMOND_CONFIRM:
		S_LocalSound ("misc/menu2.wav");
		if (++help_page >= num_help_pages)
			help_page = 0;
		return true;

	case K_DOWNARROW:
	case K_LEFTARROW:
	case K_KP_LEFTARROW:
	case K_GP_DPAD_LEFT:
	case K_GP_DIAMOND_ALTCONFIRM:
		S_LocalSound ("misc/menu2.wav");
		if (--help_page < 0)
			help_page = num_help_pages-1;
		return true;
	default:
		return false;
	}
}

void M_Menu_Help_f (void)
{
	int i;
	emenu_t *helpmenu;
#ifdef CSQC_DAT
	if (CSQC_ConsoleCommand(CL_TargettedSplit(false), Cmd_Argv(0)))
		;//return;
#endif

	helpmenu = M_CreateMenu(0);

	helpmenu->predraw = M_Help_Draw;
	helpmenu->key = M_Help_Key;

	help_page = 0;

	num_help_pages = 1;
	while(num_help_pages < 100)
	{
		for (i = 0; i < sizeof(helpstyles)/sizeof(helpstyles[0]); i++)
		{
			if (COM_FCheckExists(va(helpstyles[i].pattern, num_help_pages+helpstyles[i].base)))
				break;
		}
		if (i == sizeof(helpstyles)/sizeof(helpstyles[0]))
			break;
		num_help_pages++;
	}
}

//=============================================================================
/* QUIT MENU */

static char *quitMessage [] =
{
/* .........1.........2.... */
	"Are you gonna quit\n"
	"this game just like\n"
	"everything else?",

	"Milord, methinks that\n"
	"thou art a lowly\n"
	"quitter. Is this true?",

	"Do I need to bust your\n"
	"face open for trying\n"
	"to quit?",

	"Man, I oughta smack you\n"
	"for trying to quit!\n"
	"Press Y to get\n"
	"smacked out.",

	"Press Y to quit like a\n"
	"big loser in life.\n"
	"Press N to stay proud\n"
	"and successful!",

	//this is a vanilla message... but I'm having enough issues with false-positives in malware scanners right now that I really don't want to risk an already-suspicious actual hunam seeing stuff like this... :)
/*	"If you press Y to\n"
	"quit, I will summon\n"
	"Satan all over your\n"
	"hard drive!",
*/
	"Um, Asmodeus dislikes\n"
	"his children trying to\n"
	"quit. Press Y to return\n"
	"to your Tinkertoys.",

	"If you quit now, I'll\n"
	"throw a blanket-party\n"
	"for you next time!",




//	"Only cowards press Y here.\n",

//	"Is your life empty and\n"
//	"devoid of enjoyment?\n",

//	"Are you off to play with\n"
//	"your joy stick now?\n",
};



/*	char *cmsg[] = {
//   0123456789012345678901234567890123456789
	"0            QuakeWorld",
	"1          version " VSTR2(VERSION),
	"1modified by Forethought Entertainment",
	"0Based on QuakeWorld Version 2.40",
	"1",
	"0Additional Programming",
	"1 David Walton",
	"1",
	"0Id Software is not responsible for",
    "0providing technical support for",
	"0QUAKEWORLD(tm). (c)1996 Id Software,",
	"0Inc.  All Rights Reserved.",
	"0QUAKEWORLD(tm) is a trademark of Id",
	"0Software, Inc.",
	"1NOTICE: THE COPYRIGHT AND TRADEMARK",
	"1NOTICES APPEARING  IN YOUR COPY OF",
	"1QUAKE(r) ARE NOT MODIFIED BY THE USE",
	"1OF QUAKEWORLD(tm) AND REMAIN IN FULL",
	"1FORCE.",
	"0NIN(r) is a registered trademark",
	"0licensed to Nothing Interactive, Inc.",
	"0All rights reserved. Press y to exit",
	NULL };*/

void Cmd_WriteConfig_f(void);
static void M_Menu_DoQuit(void *ctx, promptbutton_t option)
{
	if (option == PROMPT_YES)	//'yes - quit'
		Cmd_ExecuteString("menu_quit force\n", RESTRICT_LOCAL);
//	else if (option == PROMPT_NO)	//'no - don't quit'
//	else if (option == PROMPT_CANCEL)	//'cancel - don't quit'
}
static void M_Menu_DoQuitSave(void *ctx, promptbutton_t option)
{
	if (option == PROMPT_YES)	//'yes - save-and-quit'
		Cmd_ExecuteString("menu_quit forcesave\n", RESTRICT_LOCAL);
	else if (option == PROMPT_NO)	//'no - nosave-and-quit'
		Cmd_ExecuteString("menu_quit force\n", RESTRICT_LOCAL);
//	else if (option == PROMPT_CANCEL)	//'cancel - don't quit'
}

//quit menu
void M_Menu_Quit_f (void)
{
	int mode;
	extern cvar_t cfg_save_auto;
	char *arg = Cmd_Argv(1);

#ifdef CL_MASTER
	MasterInfo_WriteServers();
#endif

	if (!strcmp(arg, "force"))
		mode = 0;
	else if (!strcmp(arg, "forcesave") || cfg_save_auto.ival)
	{
		Cmd_ExecuteString("cfg_save", RESTRICT_LOCAL);
		if (!strcmp(arg, "prompt"))
			mode = 1;
		else
			mode = 0;
	}
	else if (!strcmp(arg, "save"))
		mode = 2;
	else
	{	//prompt to save, but not otherwise.
		if (Cvar_UnsavedArchive())
			mode = 2;
		else
		{
			if (!strcmp(arg, "prompt"))
				mode = 1;
			else if (!strcmp(arg, "noprompt"))
				mode = 0;
			else
				mode = 1;
		}
	}

	switch(mode)
	{
	case 0:
		CL_Disconnect (NULL);
		Sys_Quit ();
		break;
	case 2:
		Menu_Prompt (M_Menu_DoQuitSave, NULL, localtext("You have unsaved settings\nWould you like to\nsave them now?"), "Yes", "No", "Cancel", true);
		break;
	case 1:
		Menu_Prompt (M_Menu_DoQuit, NULL, localtext(quitMessage[rand()%countof(quitMessage)]), "Quit", NULL, "Cancel", true);
		break;
	}
}

#ifdef HAVE_LEGACY
void M_Menu_Credits_f (void)
{
	Menu_Prompt (NULL, NULL, localtext("That's all folks!\nTry a different mod now."), NULL, NULL, "Sure!", false);
}
#endif

//=============================================================================
/* Menu Subsystem */

void M_Menu_ServerList2_f(void);
void M_QuickConnect_f(void);

void M_Menu_MediaFiles_f (void);
void M_Menu_FPS_f (void);
void M_Menu_Lighting_f (void);
void M_Menu_Render_f (void);
void M_Menu_Textures_f (void);
void M_Menu_Teamplay_f (void);
void M_Menu_Teamplay_Locations_f (void);
void M_Menu_Teamplay_Needs_f (void);
void M_Menu_Teamplay_Items_f (void);
void M_Menu_Teamplay_Items_Armor_f (void);
void M_Menu_Teamplay_Items_Weapons_f (void);
void M_Menu_Teamplay_Items_Powerups_f (void);
void M_Menu_Teamplay_Items_Ammo_Health_f (void);
void M_Menu_Teamplay_Items_Team_Fortress_f (void);
void M_Menu_Teamplay_Items_Status_Location_Misc_f (void);
void M_Menu_Network_f(void);
void M_Menu_Singleplayer_Cheats_f (void);
void M_Menu_Particles_f (void);
void M_Menu_Audio_Speakers_f (void);
void Menu_DownloadStuff_f (void);
static qboolean internalmenusregistered;
void M_Init_Internal (void)
{
#ifdef MENU_DAT
	MP_Shutdown();
#endif

	if (internalmenusregistered)
		return;
	internalmenusregistered = true;

#if !defined(CLIENTONLY) && defined(SAVEDGAMES)
	Cmd_AddCommand ("menu_save", M_Menu_Save_f);
	Cmd_AddCommand ("menu_load", M_Menu_Load_f);
	Cmd_AddCommand ("menu_loadgame", M_Menu_Load_f);	//q2...
#endif
	Cmd_AddCommand ("menu_single", M_Menu_SinglePlayer_f);
	Cmd_AddCommand ("menu_multi", M_Menu_MultiPlayer_f);

	Cmd_AddCommand ("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand ("help", M_Menu_Help_f);
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
	Cmd_AddCommand ("menu_mods", M_Menu_Mods_f);

#ifdef CL_MASTER
	Cmd_AddCommand ("menu_slist", M_Menu_ServerList2_f);
#endif
	Cmd_AddCommand ("menu_setup", M_Menu_Setup_f);
	Cmd_AddCommand ("menu_newmulti", M_Menu_GameOptions_f);

	Cmd_AddCommand ("menu_main", M_Menu_Main_f);	//I've moved main to last because that way tab give us main and not quit.

	Cmd_AddCommand ("menu_options", M_Menu_Options_f);
	Cmd_AddCommand ("menu_video", M_Menu_Video_f);
	Cmd_AddCommand ("menu_audio", M_Menu_Audio_f);
#ifndef __CYGWIN__
	Cmd_AddCommand ("menu_speakers", M_Menu_Audio_Speakers_f);
#endif
	Cmd_AddCommand ("menu_spcheats", M_Menu_Singleplayer_Cheats_f);
	Cmd_AddCommand ("menu_fps", M_Menu_FPS_f);
	Cmd_AddCommand ("menu_render" , M_Menu_Render_f);
	Cmd_AddCommand ("menu_lighting", M_Menu_Lighting_f);
	Cmd_AddCommand ("menu_textures", M_Menu_Textures_f);
	Cmd_AddCommand ("menu_teamplay", M_Menu_Teamplay_f);
	Cmd_AddCommand ("menu_teamplay_locations", M_Menu_Teamplay_Locations_f);
	Cmd_AddCommand ("menu_teamplay_needs", M_Menu_Teamplay_Needs_f);
	Cmd_AddCommand ("menu_teamplay_items", M_Menu_Teamplay_Items_f);
	Cmd_AddCommand ("menu_teamplay_armor", M_Menu_Teamplay_Items_Armor_f);
	Cmd_AddCommand ("menu_teamplay_weapons", M_Menu_Teamplay_Items_Weapons_f);
	Cmd_AddCommand ("menu_teamplay_powerups", M_Menu_Teamplay_Items_Powerups_f);
	Cmd_AddCommand ("menu_teamplay_ammo_health", M_Menu_Teamplay_Items_Ammo_Health_f);
	Cmd_AddCommand ("menu_teamplay_team_fortress", M_Menu_Teamplay_Items_Team_Fortress_f);
	Cmd_AddCommand ("menu_teamplay_status_location_misc", M_Menu_Teamplay_Items_Status_Location_Misc_f);
	Cmd_AddCommand ("menu_particles", M_Menu_Particles_f);
	Cmd_AddCommand ("menu_network", M_Menu_Network_f);

#ifdef HAVE_LEGACY
	Cmd_AddCommand ("menu_credits", M_Menu_Credits_f);
#endif

#if defined(CL_MASTER) && defined(HAVE_PACKET)
	Cmd_AddCommand ("quickconnect", M_QuickConnect_f);
#endif
}

void M_DeInit_Internal (void)
{
	M_RemoveAllMenus(true);

	if (!internalmenusregistered)
		return;
	internalmenusregistered = false;

#ifndef CLIENTONLY
	Cmd_RemoveCommand ("menu_save");
	Cmd_RemoveCommand ("menu_load");
	Cmd_RemoveCommand ("menu_loadgame");	//q2...
#endif
	Cmd_RemoveCommand ("menu_single");
	Cmd_RemoveCommand ("menu_multi");

	Cmd_RemoveCommand ("menu_keys");
	Cmd_RemoveCommand ("help");
	Cmd_RemoveCommand ("menu_quit");
	Cmd_RemoveCommand ("menu_mods");

#ifdef CL_MASTER
	Cmd_RemoveCommand ("menu_slist");
#endif
	Cmd_RemoveCommand ("menu_setup");
	Cmd_RemoveCommand ("menu_newmulti");

	Cmd_RemoveCommand ("menu_options");
	Cmd_RemoveCommand ("menu_video");
	Cmd_RemoveCommand ("menu_audio");
	Cmd_RemoveCommand ("menu_speakers");
	Cmd_RemoveCommand ("menu_teamplay");
	Cmd_RemoveCommand ("menu_teamplay_locations");
	Cmd_RemoveCommand ("menu_teamplay_needs");
	Cmd_RemoveCommand ("menu_teamplay_items");
	Cmd_RemoveCommand ("menu_teamplay_armor");
	Cmd_RemoveCommand ("menu_teamplay_weapons");
	Cmd_RemoveCommand ("menu_teamplay_powerups");
	Cmd_RemoveCommand ("menu_teamplay_ammo_health");
	Cmd_RemoveCommand ("menu_teamplay_team_fortress");
	Cmd_RemoveCommand ("menu_teamplay_status_location_misc");
	Cmd_RemoveCommand ("menu_spcheats");
	Cmd_RemoveCommand ("menu_fps");
	Cmd_RemoveCommand ("menu_render");
	Cmd_RemoveCommand ("menu_lighting");
	Cmd_RemoveCommand ("menu_textures");
	Cmd_RemoveCommand ("menu_particles");
	Cmd_RemoveCommand ("menu_network");

#ifdef HAVE_LEGACY
	Cmd_RemoveCommand ("menu_credits");
#endif

	Cmd_RemoveCommand ("menu_main");	//I've moved main to last because that way tab gives us main and not quit.
	Cmd_RemoveCommand ("quickconnect");
}

void M_Shutdown(qboolean total)
{
#ifdef MENU_NATIVECODE
	MN_Shutdown();
#endif
#ifdef MENU_DAT
	MP_Shutdown();
#endif
	M_RemoveAllMenus(!total);
	M_DeInit_Internal();
}

void M_Reinit(void)
{
#ifdef MENU_NATIVECODE
	if (!MN_Init())
#endif
#ifdef MENU_DAT
	if (!MP_Init())
#endif
	{
		M_Init_Internal();

		(void)CSQC_UnconnectedInit();
	}
}

void FPS_Preset_f(void);
void FPS_Preset_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx);
void M_MenuPop_f(void);

//menu.dat is loaded later... after the video and everything is up.
void M_Init (void)
{
	Cmd_AddCommand("menu_restart", M_Restart_f);
	Cmd_AddCommand("togglemenu", M_ToggleMenu_f);
	Cmd_AddCommand("closemenu", M_CloseMenu_f);
	Cmd_AddCommandAD("fps_preset", FPS_Preset_f, FPS_Preset_c, "Apply a preset");
	Cmd_AddCommand("menupop", M_MenuPop_f);

	//server browser is kinda complex, and has clipboard integration which we need to sandbox a little
#ifdef CL_MASTER
	Cmd_AddCommand ("menu_servers", M_Menu_ServerList2_f);
#endif
	//downloads menu needs sandboxing, so cannot be provided by qc.
#ifdef PACKAGEMANAGER
	Cmd_AddCommand ("menu_download", Menu_DownloadStuff_f);
#endif
#ifndef MINIMAL
	Cmd_AddCommandAD ("modelviewer", M_Menu_ModelViewer_f, M_Menu_ModelViewer_c, "View a model...");
#endif
	//demo menu is allowed to see outside of the quakedir. you can't replicate that in qc's sandbox.
	Cmd_AddCommand ("menu_demo", M_Menu_Demos_f);
#ifdef HAVE_JUKEBOX
	Cmd_AddCommand ("menu_mediafiles", M_Menu_MediaFiles_f);
#endif



	Cvar_Register(&m_preset_chosen, "Menu thingumiebobs");
	Cvar_Register(&m_helpismedia, "Menu thingumiebobs");

	Media_Init();
#ifdef CL_MASTER
	M_Serverlist_Init();
#endif
	M_Script_Init();

	M_Reinit();
}
//end builtin-menu code.
#else
void M_Init_Internal (void){}
void M_DeInit_Internal (void){}
void M_Shutdown(qboolean total)
{
#ifdef MENU_NATIVECODE
	MN_Shutdown();
#endif
#ifdef MENU_DAT
	MP_Shutdown();
#endif
}
void M_Reinit(void)
{
#ifdef MENU_NATIVECODE
	if (!MN_Init())
#endif
#ifdef MENU_DAT
	if (!MP_Init())
#endif
	{
		(void)CSQC_UnconnectedInit();
	}
}
void M_Init (void)
{
	Cmd_AddCommand("menu_restart", M_Restart_f);
	Cmd_AddCommand("togglemenu", M_ToggleMenu_f);

	Media_Init();
	M_Reinit();
}
#endif

void M_Window_ClosePrompt(void)
{	//someone clicked our window's 'close' button or system menu or alt+f4 or etc. we blocked it for now, but don't just ignore it...
	COM_AssertMainThread("M_Window_ClosePrompt");
	Key_Dest_Remove(kdm_console);
	if (Cmd_Exists("menu_quit") || Cmd_AliasExist("menu_quit", RESTRICT_LOCAL))
		Cmd_ExecuteString("menu_quit prompt", RESTRICT_LOCAL);	//our builtin menus use this form
	else if (Cmd_Exists("m_quit") || Cmd_AliasExist("m_quit", RESTRICT_LOCAL))
		Cmd_ExecuteString("m_quit", RESTRICT_LOCAL);	//some menuqc mods use a different name for the command to avoid conflicts.
	else
		Cmd_ExecuteString("quit", RESTRICT_LOCAL);	//fall back to the engine's version
}


// Generic function to choose which game menu to draw
int M_GameType (void)
{
	static int cached;
	static unsigned int cachedrestarts;

	if (FS_Restarted(&cachedrestarts))
	{
		struct
		{
			int gametype;
			char *path;
		} configs[] =
		{
			{MGT_QUAKE1, "gfx/sp_menu.lmp"},
#ifdef Q2CLIENT
			{MGT_QUAKE2, "pics/m_banner_game.pcx"},
#endif
#ifdef HEXEN2
			{MGT_HEXEN2, "gfx/menu/title2.lmp"},
#endif
			{0, NULL}
		};
		int bd = COM_FDepthFile(configs[0].path, true);
		int i;
		cached = configs[0].gametype;
		for (i = 1; configs[i].path; i++)
		{
			int gd = COM_FDepthFile(configs[i].path, true);
			if (bd > gd)
			{
				bd = gd;
				cached = configs[i].gametype;
			}
		}
	}

	return cached;
}


