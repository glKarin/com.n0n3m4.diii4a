//mp3 menu and track selector.
//was origonally an mp3 track selector, now handles lots of media specific stuff - like q3 films!
//should rename to m_media.c

/*«11:56:05 am» «@Spikester» EBUILTIN(void, Menu_Control, (int mnum));
«11:56:05 am» «@Spikester» #define MENU_CLEAR 0
«11:56:05 am» «@Spikester» #define MENU_GRAB 1
«11:56:05 am» «@Spikester» EBUILTIN(int, Key_GetKeyCode, (char *keyname));
«11:56:13 am» «@Spikester» that's how you do menus. :)*/

#define BUILD 1

#include <stdlib.h> // needed for itoi
#include <stdio.h> // needed for itoi?

#include <windows.h>

#include "../plugin.h"

#include "winamp.h"
HWND hwnd_winamp;

//int media_playing=true;//try to continue from the standard playlist
//cvar_t winamp_dir = {"winamp_dir", "c:/program files/winamp5/"};
//cvar_t winamp_exe = {"winamp_exe", "winamp.exe"};
//cvar_t media_shuffle = {"media_shuffle", "1"};
//cvar_t media_repeat = {"media_repeat", "1"};

qboolean WinAmp_GetHandle (void)
{
	if ((hwnd_winamp = FindWindow("Winamp", NULL)))
		return true;
	if ((hwnd_winamp = FindWindow("Winamp v1.x", NULL)))
		return true;
	if ((hwnd_winamp = FindWindow("winamp", NULL)))
		return true;

	return false;
}

// Start Moodles Attempt at Winamp Commands
// Note strange bug, load up FTE normally. And type winamp_version, for me the output is 0, but if I do winamp_version a second time it says 24604 (which is hex for 5010, my version of winamp is 5.10)

void Winamp_Play_f(void)
{
	if (!WinAmp_GetHandle())
	{
		Con_Printf("Winamp not loaded\n");
		return;
	}

	//SendMessage(hwnd_winamp, WM_COMMAND, WINAMP_BUTTON2, 0); <- is below fails, uncomment this.

	SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_STARTPLAY);
	Con_Printf("Attempting to start playback\n");
}

void Winamp_Version_f(void)
{
	int version;

	if (!WinAmp_GetHandle())
	{
		Con_Printf("Winamp not loaded\n");
		return;
	}

	version = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETVERSION);

	//itoa (version, temp, 16); // should convert it to hex

	Con_Printf("Winamp Version: %d\n",version);
}

void Winamp_TimeLeft_f(void)
{
	int tracklength;
	int trackposition;
	int timeleft;

	if (!WinAmp_GetHandle())
	{
		Con_Printf("Winamp not loaded\n");
		return;
	}

	tracklength = SendMessage(hwnd_winamp,WM_WA_IPC,1,IPC_GETOUTPUTTIME);
	trackposition = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETOUTPUTTIME);

	timeleft = tracklength-(trackposition/1000);

	Con_Printf("Time Left: %d seconds\n",timeleft); // convert it to h:m:s later
}

void Winamp_JumpTo_f(void) // input is a percentage
{
	int tracklength;
	float inputpercent;
	double trackpercent;
	char input[20];
	int res;

	if (!WinAmp_GetHandle())
	{
		Con_Printf("Winamp not loaded\n");
		return;
	}

	tracklength = SendMessage(hwnd_winamp,WM_WA_IPC,1,IPC_GETOUTPUTTIME);

	cmdfuncs->Argv(1,input,sizeof(input));

	inputpercent = atoi(input);

	if (inputpercent > 100)
	{
		Con_Printf("ERROR: Choose a percent between 0 and 100\n");
		return;
	}

	inputpercent = inputpercent/100;

	trackpercent = (tracklength*1000)*inputpercent;

	res = SendMessage(hwnd_winamp,WM_WA_IPC,trackpercent,IPC_JUMPTOTIME);

	if (res == 0)
	{
		Con_Printf("Successfully jumped to %s percent\n",input);
		return;
	}
	else if (res == -1)
	{
		Con_Printf("There are no songs playing\n");
		return;
	}
	else if (res == 1)
	{
		Con_Printf("End of file\n");
	}

	Con_Printf("Oh oh spagettioes you shouldn't see this");
}

void Winamp_GoToPlayListPosition_f(void) // the playlist selecter doesn't actually work
{
	//int length = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETLISTLENGTH); //set a max
	char input[20];
	int inputnumber;

	if (!WinAmp_GetHandle())
	{
		Con_Printf("Winamp not loaded\n");
		return;
	}

	cmdfuncs->Argv(1,input,sizeof(input));

	inputnumber = atoi(input);

	SendMessage(hwnd_winamp,WM_WA_IPC,inputnumber,IPC_SETPLAYLISTPOS);

	SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_STARTPLAY); // the above only selects it, doesn't actually play it.

	Con_Printf("Attemped to set playlist position %s\n",input);
}

void Winamp_Volume_f(void) // I think this only works when the client did the winamp_play
{
	char input[20];
	int inputnumber;

	if (!WinAmp_GetHandle())
	{
		Con_Printf("Winamp not loaded\n");
		return;
	}

	cmdfuncs->Argv(1,input,sizeof(input));

	inputnumber = atoi(input);

	if ((!input[0]) || (inputnumber > 255))
	{
		Con_Printf("Choose a number between 0 and 255\n");
		return;
	}

	SendMessage(hwnd_winamp,WM_WA_IPC,inputnumber,IPC_SETVOLUME);

	Con_Printf("Winamp volume set to: %s\n",input);
}

void Winamp_ChannelPanning_f(void) // doesn't seem to work for me
{
	char input[20];
	int inputnumber;

	if (!WinAmp_GetHandle())
	{
		Con_Printf("Winamp not loaded\n");
		return;
	}

	cmdfuncs->Argv(1,input,sizeof(input));

	inputnumber = atoi(input);

	if ((!input[0]) || (inputnumber > 255))
	{
		Con_Printf("Choose a number between 0 (left) and 255 (right). Center is about 127\n");
		return;
	}

	SendMessage(hwnd_winamp,WM_WA_IPC,inputnumber,IPC_SETPANNING);

	Con_Printf("Winamp channel panning set to: %s\n",input);
}

void Winamp_PlayListLength_f(void) // has a habit of returning 0 when you dont use winamp_play to start off playing
{
	int length;

	if (!WinAmp_GetHandle())
	{
		Con_Printf("Winamp not loaded\n");
		return;
	}

	length = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETLISTLENGTH);

	Con_Printf("Winamp playlist length: %d\n",length);
}

void Winamp_PlayListPosition_f(void) // has a habit of return 0 of 0
{
	int pos;
	int length;

	if (!WinAmp_GetHandle())
	{
		Con_Printf("Winamp not loaded\n");
		return;
	}

	pos = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETLISTPOS);
	length = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETLISTLENGTH);

	Con_Printf("Winamp currently on position '%d' of '%d'\n",pos,length);
}

void Winamp_SongInfo_f(void)
{
	char title[255];
	int res;
	int samplerate;
	int bitrate;
	int channels;

	if (!WinAmp_GetHandle())
	{
		Con_Printf("Winamp not loaded\n");
		return;
	}

	res = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_ISPLAYING);
	samplerate = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETINFO);
	bitrate = SendMessage(hwnd_winamp,WM_WA_IPC,1,IPC_GETINFO);
	channels = SendMessage(hwnd_winamp,WM_WA_IPC,2,IPC_GETINFO);

	GetWindowText(hwnd_winamp, title, sizeof(title));

	if (res == 0)
	{
		Con_Printf("WinAmp is off\n");
		return;
	}
	else if (res == 1)
	{
		Con_Printf("Currently playing: %s\nSamplerate: %dkHz\nBitrate: %dkbps \nChannels: %d\n",title,samplerate,bitrate,channels);
		return;
	}
	else if (res == 3)
	{
		Con_Printf("Winamp is paused\n");
		return;
	}
}

void Winamp_Restart_f(void)
{
	if (!WinAmp_GetHandle())
	{
		Con_Printf("Winamp not loaded\n");
		return;
	}

	SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_RESTARTWINAMP);

	Con_Printf("Attempting to restart winamp\n");
}

void Winamp_Shuffle_f(void) //it works, thats all i can say lol
{
	char input[20];
	int inputnumber;
	int inputnumber2;
	int get;

	if (!WinAmp_GetHandle())
	{
		Con_Printf("Winamp not loaded\n");
		return;
	}

	get = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GET_SHUFFLE);

	cmdfuncs->Argv(1,input,sizeof(input));

	inputnumber2 = atoi(input);

	//inputnumber = Cmd_Argc();
	inputnumber = 1; // fix later

	if (inputnumber2 == 1)
	{
		SendMessage(hwnd_winamp,WM_WA_IPC,1,IPC_SET_SHUFFLE);
		Con_Printf("Winamp shuffle turned on\n");
		return;
	}
	else if ((inputnumber2 == 0) && (inputnumber == 2))
	{
		SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_SET_SHUFFLE);
		Con_Printf("Winamp shuffle turned off\n");
		return;
	}
	else if (get == 1)
	{
		Con_Printf("Winamp shuffle is currently on\n");
	}
	else if (get == 0)
	{
		Con_Printf("Winamp shuffle is currently off\n");
	}

		Con_Printf("Enter 1 to to turn Winamp shuffle on, 0 to turn it off\n");
		return;
}

void Winamp_Repeat_f(void) // it works, thats all i can say lol
{
	char input[20];
	int inputnumber;
	int inputnumber2;
	int get;

	if (!WinAmp_GetHandle())
	{
		Con_Printf("Winamp not loaded\n");
		return;
	}

	get = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GET_REPEAT);

	cmdfuncs->Argv(1,input,sizeof(input));

	inputnumber2 = atoi(input);

	//inputnumber = Cmd_Argc();
	inputnumber = 2; // fix later

	if (inputnumber2 == 1)
	{
		SendMessage(hwnd_winamp,WM_WA_IPC,1,IPC_SET_REPEAT);
		Con_Printf("Winamp repeat turned on\n");
		return;
	}
	else if ((inputnumber2 == 0) && (inputnumber == 2))
	{
		SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_SET_REPEAT);
		Con_Printf("Winamp repeat turned off\n");
		return;
	}
	else if (get == 1)
	{
		Con_Printf("Winamp repeat is currently on\n");
	}
	else if (get == 0)
	{
		Con_Printf("Winamp repeat is currently off\n");
	}

		Con_Printf("Enter 1 to to turn Winamp repeat on, 0 to turn it off\n");
		return;
}

void Winamp_VolumeUp_f(void)
{
	SendMessage(hwnd_winamp, WM_COMMAND, WINAMP_VOLUMEUP, 0);

	Con_Printf("Winamp volume incremented\n");
}

void Winamp_VolumeDown_f(void)
{
	SendMessage(hwnd_winamp, WM_COMMAND, WINAMP_VOLUMEDOWN, 0);

	Con_Printf("Winamp volume decremented\n");
}

void Winamp_FastForward5Seconds_f(void)
{
	SendMessage(hwnd_winamp, WM_COMMAND, WINAMP_FFWD5S, 0);

	Con_Printf("Winamp fast forwarded 5 seconds\n");
}

void Winamp_Rewind5Seconds_f(void)
{
	SendMessage(hwnd_winamp, WM_COMMAND, WINAMP_REW5S, 0);

	Con_Printf("Winamp rewinded 5 seconds\n");
}

// End Moodles Attempt at Winamp Commands

void Winamp_InitCommands(void)
{
	cmdfuncs->AddCommand("winamp_play", Winamp_Play_f, "");
	cmdfuncs->AddCommand("winamp_version", Winamp_Version_f, "");
	cmdfuncs->AddCommand("winamp_timeleft", Winamp_TimeLeft_f, "");
	cmdfuncs->AddCommand("winamp_jumpto", Winamp_JumpTo_f, "");
	cmdfuncs->AddCommand("winamp_gotoplaylistposition", Winamp_GoToPlayListPosition_f, "");
	cmdfuncs->AddCommand("winamp_volume", Winamp_Volume_f, "");
	cmdfuncs->AddCommand("winamp_channelpanning", Winamp_ChannelPanning_f, "");
	cmdfuncs->AddCommand("winamp_playlistlength", Winamp_PlayListLength_f, "");
	cmdfuncs->AddCommand("winamp_playlistposition", Winamp_PlayListPosition_f, "");
	cmdfuncs->AddCommand("winamp_songinfo", Winamp_SongInfo_f, "");
	cmdfuncs->AddCommand("winamp_restart", Winamp_Restart_f, "");
	cmdfuncs->AddCommand("winamp_shuffle", Winamp_Shuffle_f, "");
	cmdfuncs->AddCommand("winamp_repeat", Winamp_Repeat_f, "");
	cmdfuncs->AddCommand("winamp_volumeup", Winamp_VolumeUp_f, "");
	cmdfuncs->AddCommand("winamp_volumedown", Winamp_VolumeDown_f, "");
	cmdfuncs->AddCommand("winamp_fastforward5seconds", Winamp_FastForward5Seconds_f, "");
	cmdfuncs->AddCommand("winamp_rewind5seconds", Winamp_Rewind5Seconds_f, "");
}

qboolean Plug_Init(void)
{
	Winamp_InitCommands();
	return true;
}
