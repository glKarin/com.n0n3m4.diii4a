#include <quakedef.h>

//because cake.
#include "sys_linux.c"

#include <dos.h>
#include <dpmi.h>
_go32_dpmi_registers regs;
int dos_int86(int vec)
{
	int rc;
	regs.x.ss = regs.x.sp = 0;
	rc = _go32_dpmi_simulate_int(vec, &regs);
	return rc || (regs.x.flags & 1);
}


static int mouse_buttons;
static int mouse_numbuttons;
static unsigned int dosmousedeviceid;

static unsigned int doskeyboarddeviceid;

#define KBRINGSIZE 256
static struct
{
	unsigned char buf[KBRINGSIZE];
	int write;
	int read;
} kbring;

void TheKBHandler(void)
{	//this needs to be kept simple and small.
	//we write to a really simple ringbuffer to avoid needing to lock various code/data pages.
	kbring.buf[kbring.write++&(KBRINGSIZE-1)] = inportb(0x60);
	outportb(0x20, 0x20);
}

unsigned char keymap[256] =
{
	//this is a copy of the US keymap from vanilla quake.
	//its so very tempting to switch it to a UK keymap...

//	0			1		2		3			4		5			6			7 
//	8			9		A		B			C		D			E			F 
	0  ,		27,		'1',	'2',		'3',	'4',		'5',		'6',
	'7',		'8',	'9',	'0',		'-',	'=',		K_BACKSPACE,K_TAB,		// 0
	'q',		'w',	'e',	'r',		't',	'y',		'u',		'i',
	'o',		'p',	'[',	']',		13 ,	K_CTRL,		'a',		's',	// 1
	'd',		'f',	'g',	'h',		'j',	'k',		'l',		';',
	'\'' ,		'`',	K_LSHIFT,'\\',		'z',	'x',		'c',		'v',	// 2
	'b',		'n',	'm',	',',		'.',	'/',		K_RSHIFT,	'*',
	K_ALT,		' ',	0  ,	K_F1,		K_F2,	K_F3,		K_F4,		K_F5,	// 3
	K_F6,		K_F7,	K_F8,	K_F9,		K_F10,	0  ,		0  ,		K_HOME,
	K_UPARROW,	K_PGUP,	'-',	K_LEFTARROW,'5',	K_RIGHTARROW,'+',		K_END,	// 4
	K_DOWNARROW,K_PGDN,	K_INS,	K_DEL,		0,		0  ,		0,			K_F11,
	K_F12,		0  ,	0  ,	0  ,		0  ,	0  ,		0  ,		0,		// 5
	0  ,		0  ,	0  ,	0  ,		0  ,	0  ,		0  ,		0,
	0  ,		0  ,	0  ,	0  ,		0  ,	0  ,		0  ,		0,		// 6
	0  ,		0  ,	0  ,	0  ,		0  ,	0  ,		0  ,		0,
	0  ,		0  ,	0  ,	0  ,		0  ,	0  ,		0  ,		0,		// 7

//	0			1		2		3			4		5			6			7
//	8			9		A		B			C		D			E			F
	0  ,		27,		'!',	'@',		'#',	'$',		'%',		'^',
	'&',		'*',	'(',	')',		'_',	'+',		K_BACKSPACE,K_TAB,		// 0
	'Q',		'W',	'E',	'R',		'T',	'Y',		'U',		'I',
	'O',		'P',	'{',	'}',		13 ,	K_CTRL,		'A',		'S',	// 1
	'D',		'F',	'G',	'H',		'J',	'K',		'L',		':',
	'\"',		'~',	K_LSHIFT,'|',		'Z',	'X',		'C',		'V',	// 2
	'B',		'N',	'M',	'<',		'>',	'?',		K_RSHIFT,	'*',
	K_ALT,		' ',	0  ,	K_F1,		K_F2,	K_F3,		K_F4,		K_F5,	// 3
	K_F6,		K_F7,	K_F8,	K_F9,		K_F10,	0  ,		0  ,		K_HOME,
	K_UPARROW,	K_PGUP,	'_',	K_LEFTARROW,'%',	K_RIGHTARROW,'+',		K_END,	// 4
	K_DOWNARROW,K_PGDN,	K_INS,	K_DEL,		0,		0,			0,			K_F11,
	K_F12,		0  ,	0  ,	0  ,		0  ,	0  ,		0  ,		0,		// 5
	0  ,		0  ,	0  ,	0  ,		0  ,	0  ,		0  ,		0,
	0  ,		0  ,	0  ,	0  ,		0  ,	0  ,		0  ,		0,		// 6
	0  ,		0  ,	0  ,	0  ,		0  ,	0  ,		0  ,		0,
	0  ,		0  ,	0  ,	0  ,		0  ,	0  ,		0  ,		0		// 7
};

void INS_Init(void)
{
	//make sure the kb handler and its data won't get paged out.
	_go32_dpmi_lock_code((void *) TheKBHandler, 512);
	_go32_dpmi_lock_data((void *) &kbring, sizeof(kbring));

	//set up our interrupt handler.
	_go32_dpmi_seginfo info;
	info.pm_offset = (int) TheKBHandler;
	_go32_dpmi_allocate_iret_wrapper(&info);
	_go32_dpmi_set_protected_mode_interrupt_vector(9, &info);
}

void Sys_SendKeyEvents(void)
{	//this is kinda silly, but the handler can't do it if we want to be correct with respect to virtual memory.
	static int shift_down;
	while (kbring.read != kbring.write)
	{	//keyboard maps are complicated, annoyingly so. left/right/extended.... and don't get me started on sysreq. some keys don't even have release events!
		unsigned char	c = kbring.buf[kbring.read++&(KBRINGSIZE-1)];
		unsigned int	qkey, ukey;

		qkey = keymap[c&0x7f];
		ukey = (qkey >= 32 && qkey < 127)?keymap[(c&0x7f)|(shift_down?128:0)]:0;
		if (c == 0xe0)
		{	//extended keys...
			if (kbring.buf[kbring.read&(KBRINGSIZE-1)] == 0x1d)
				qkey = K_RSHIFT;
			else
				continue;	//annoying extended keys.
			ukey = 0;
			kbring.read++;
		}
		if (qkey == K_LSHIFT)
			shift_down = (shift_down&~1) | ((c&0x80)?0:1);
		if (qkey == K_RSHIFT)
			shift_down = (shift_down&~2) | ((c&0x80)?0:2);
//		Con_Printf("Keyboard: %x\n", c);
		IN_KeyEvent(doskeyboarddeviceid, !(c&0x80), qkey, ukey);
	}
}

void INS_Move(float *movements, int pnum)
{
}
void INS_Commands(void)
{
	if (!mouse_numbuttons)
		return;

	regs.x.ax = 11;		// read move
	dos_int86(0x33);
	if (regs.x.cx || regs.x.dx)
	{
		IN_MouseMove(dosmousedeviceid, false, (short)regs.x.cx, (short)regs.x.dx, 0, 0);

		Con_Printf("Mouse Move: %i %i\n", (short)regs.x.cx, (short)regs.x.dx);
	}

	regs.x.ax = 3;		// read buttons
	dos_int86(0x33);
	int b = mouse_buttons ^ regs.x.bx;
	mouse_buttons = regs.x.bx;
	for (int i = 0; i < mouse_numbuttons; i++)
	{
		if (b&(1u<<i))
			IN_KeyEvent(dosmousedeviceid, !!(mouse_buttons&(1u<<i)), K_MOUSE1+i, 0);
	}

	Sys_SendKeyEvents();
}
void INS_ReInit(void)
{
	dosmousedeviceid = 0;
	mouse_buttons = 0;
	mouse_numbuttons = 0;

	regs.x.ax = 0;
	dos_int86(0x33);
	if (!regs.x.ax)
	{
		Con_Printf ("No mouse found\n");
		return;
	}
	mouse_numbuttons = regs.x.bx;
	if (mouse_numbuttons >= 255)
		mouse_numbuttons = 2;
	if (mouse_numbuttons > 10)
		mouse_numbuttons = 10;
	Con_Printf("%d-button mouse available\n", mouse_numbuttons);
}
void INS_Shutdown(void)
{
}
void INS_EnumerateDevices(void *ctx, void(*callback)(void *ctx, const char *type, const char *devicename, unsigned int *qdevid))
{
	callback(ctx, "mouse", 		"dosmouse", 	&dosmousedeviceid);
	callback(ctx, "keyboard", 	"doskeyboard", 	&doskeyboarddeviceid);
}

void Sys_Sleep (double seconds)
{
	usleep(seconds * 1000000);
}
