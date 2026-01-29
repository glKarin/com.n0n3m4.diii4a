/*
Copyright (C) 2005 David Walton.

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



As a special exception, you may incorpotate patents and libraries regarding only hashing and security, on the conditions that it is also open source.
This means md4/5, rsa, ssl and similar.
*/

#include "../plugin.h"

//code to sit on an imap server and check for new emails every now and then.





char *STR_Parse(char *str, char *out, int outlen, char *punctuation)
{
	char *s = str;
	char *f;
	
skipwhite:
	//skip over the whitespace
	while (*s <= ' ' && *s)
		s++;

	if (*s == '/')
	{
		if (s[1] == '/')	//c++ style comment
		{
			while(*s != '\n' && *s)
				s++;

			goto skipwhite;
		}
		if (s[1] == '*')
		{
			s+=2;
			while(*s)
			{
				if (s[0] == '*' && s[1] == '/')
				{
					s+=2;
					break;
				}
				s++;
			}
			goto skipwhite;
		}
	}

	if (*s == '\"')
	{
		s++;
		while(*s && outlen>1)
		{
			if (*s == '\"')
			{
				s++;
				break;
			}
			*out++ = *s++;
			outlen--;
		}
		*out++ = '\0';
		return s;
	}

	if (strchr(punctuation, *s))
	{	//starts with punctuation, so return only the first char
		if (outlen < 2)
			return NULL;	//aaaah!
		*out++ = *s;
		*out++ = '\0';
		s++;

		return s;
	}
	//skip over non-white
	for (f = s; outlen>1 && *(unsigned char*)f > ' '; f++, outlen--)
	{
		if (strchr(punctuation, *f))
		{	//found punctuation, so return up to here
			break;
		}
		*out++ = *f;
	}
	*out++ = '\0';

	return f;
}






//exported.
void IMAP_CreateConnection(char *servername, char *username, char *password);
int imap_checkfrequency=60*1000;
void IMAP_Think (void);
//end export list.




#define IMAP_PORT 143


typedef struct imap_con_s {
	char server[128];
	char username[128];
	char password[128];

	unsigned int lastnoop;

	//these are used so we can fail a send.
	//or recieve only part of an input.
	//FIXME:	make dynamically sizable, as it could drop if the send is too small (That's okay.
	//			but if the read is bigger than one command we suddenly fail entirly.
	int sendlen;
	int sendbuffersize;
	char *sendbuffer;
	int readlen;
	int readbuffersize;
	char *readbuffer;

	qboolean drop;

	qhandle_t socket;

	enum {
		IMAP_WAITINGFORINITIALRESPONCE,
		IMAP_AUTHING,
		IMAP_AUTHED,
		IMAP_INBOX
	} state;

	struct imap_con_s *next;
} imap_con_t;

static imap_con_t *imapsv;

void IMAP_CreateConnection(char *addy, char *username, char *password)
{
	unsigned long _true = true;
	imap_con_t *con;

	for (con = imapsv; con; con = con->next)
	{
		if (!strcmp(con->server, addy))
		{
			Con_Printf("Already connected to that imap server\n");
			return;
		}
	}

	con = malloc(sizeof(imap_con_t));

	con->socket = Net_TCPConnect(addy, IMAP_PORT);

	if (!con->socket)
	{
		Con_Printf ("IMAP_CreateConnection: connect failed\n");
		free(con);
		return;
	}

	strlcpy(con->server, addy, sizeof(con->server));
	strlcpy(con->username, username, sizeof(con->username));
	strlcpy(con->password, password, sizeof(con->password));

	con->next = imapsv;
	imapsv = con;

	Con_Printf ("Connected to %s (%s)\n", addy, username);
}

static void IMAP_EmitCommand(imap_con_t *imap, char *text)
{
	int newlen;
	
	//makes a few things easier though

	newlen = imap->sendlen + 2 + strlen(text) + 2;

	if (newlen >= imap->sendbuffersize || !imap->sendbuffer)	//pre-length check.
	{
		char *newbuf;
		imap->sendbuffersize = newlen*2;
		newbuf = malloc(imap->sendbuffersize);	//the null terminator comes from the >=
		if (!newbuf)
		{
			Con_Printf("Memory is low\n");
			imap->drop = true;	//failed.
			return;
		}
		if (imap->sendbuffer)
		{
			memcpy(newbuf, imap->sendbuffer, imap->sendlen);
			free(imap->sendbuffer);
		}
		imap->sendbuffer = newbuf;
	}

	snprintf(imap->sendbuffer+imap->sendlen, newlen+1, "* %s\r\n", text);
	imap->sendlen = newlen;
}

static char *IMAP_AddressStructure(char *msg, char *out, int outsize)
{
	char name[256];
	char mailbox[64];
	char hostname[128];
	char route[128];
	int indents=0;
	while(*msg == ' ')
		msg++;
	while(*msg == '(')	//do it like this, we can get 2... I'm not sure if that's always true..
	{
		msg++;
		indents++;
	}

	msg = STR_Parse(msg, name, sizeof(name), "");	//name
	msg = STR_Parse(msg, route, sizeof(route), "");	//smtp route (ignored normally)
	msg = STR_Parse(msg, mailbox, sizeof(mailbox), "");	//mailbox
	msg = STR_Parse(msg, hostname, sizeof(hostname), "");	//hostname

	while(indents && *msg == ')')
		msg++;

	if (out)
	{
		if (!strcmp(name, "NIL"))
			snprintf(out, outsize, "%s@%s", mailbox, hostname);
		else
			snprintf(out, outsize, "%s <%s@%s>", name, mailbox, hostname);
	}

	return msg;
}

static qboolean IMAP_ThinkCon(imap_con_t *imap)	//false means drop the connection.
{
	char *ending;
	int len;

	//get the buffer, stick it in our read holder
	if (imap->readlen+32 >= imap->readbuffersize || !imap->readbuffer)
	{
		len = imap->readbuffersize;
		if (!imap->readbuffer)
			imap->readbuffersize = 256;
		else
			imap->readbuffersize*=2;

		ending = malloc(imap->readbuffersize);
		if (!ending)
		{
			Con_Printf("Memory is low\n");
			return false;
		}
		if (imap->readbuffer)
		{
			memcpy(ending, imap->readbuffer, len);
			free(imap->readbuffer);
		}
		imap->readbuffer = ending;
	}

	len = Net_Recv(imap->socket, imap->readbuffer+imap->readlen, imap->readbuffersize-imap->readlen-1);
	if (len>0)
	{
		imap->readlen+=len;
		imap->readbuffer[imap->readlen] = '\0';
	}

	if (imap->readlen>0)
	{
		ending = strstr(imap->readbuffer, "\r\n");

		if (ending)	//pollable text.
		{
			*ending = '\0';
//			Con_Printf("%s\n", imap->readbuffer);

			ending+=2;
			if (imap->state == IMAP_WAITINGFORINITIALRESPONCE)
			{
				//can be one of two things.
				if (!strncmp(imap->readbuffer, "* OK", 4))
				{
					IMAP_EmitCommand(imap, va("LOGIN %s %s", imap->username, imap->password));
					imap->state = IMAP_AUTHING;
				}
				else if (!strncmp(imap->readbuffer, "* PREAUTH", 9))
				{
					Con_Printf("Logged on to %s\n", imap->server);
					IMAP_EmitCommand(imap, "SELECT INBOX");
					imap->state = IMAP_AUTHED;
					imap->lastnoop = Sys_Milliseconds();
				}
				else
				{
					Con_Printf("Unexpected response from IMAP server\n");
					return false;
				}
			}
			else if (imap->state == IMAP_AUTHING)
			{
				if (!strncmp(imap->readbuffer, "* OK", 4))
				{
					Con_Printf("Logged on to %s\n", imap->server);
					IMAP_EmitCommand(imap, "SELECT INBOX");
					imap->state = IMAP_AUTHED;
					imap->lastnoop = Sys_Milliseconds();
				}
				else
				{
					Con_Printf("Unexpected response from IMAP server\n");
					return false;
				}
			}
			else if (imap->state == IMAP_AUTHED)
			{
				char *num;
				num = imap->readbuffer;
				if (!strncmp(imap->readbuffer, "* SEARCH ", 8))	//we only ever search for recent messages. So we fetch them and get sender and subject.
				{
					char *s;
					s = imap->readbuffer+8;
					num = NULL;
					while(*s)
					{
						s++;
						num = s;
						while (*s >= '0' && *s <= '9')
							s++;

						IMAP_EmitCommand(imap, va("FETCH %i ENVELOPE", atoi(num)));	//envelope so that it's all one line.
					}
				}
				else if (imap->readbuffer[0] == '*' && imap->readbuffer[1] == ' ')
				{
					num = imap->readbuffer+2;
					while(*num >= '0' && *num <= '9')
					{
						num++;
					}
					if (!strcmp(num, " RECENT"))
					{
						if (atoi(imap->readbuffer+2) > 0)
						{
							IMAP_EmitCommand(imap, "SEARCH RECENT");
						}
					}
					else if (!strncmp(num, " FETCH (ENVELOPE (", 18))
					{
						char from[256];
						char subject[256];
						char date[256];

						num += 18;

						num = STR_Parse(num, date, sizeof(date), "");
						num = STR_Parse(num, subject, sizeof(subject), "");
//						Con_Printf("Date/Time: %s\n", date);

						num = IMAP_AddressStructure(num, from, sizeof(from));


						if ((rand() & 3) == 3)
						{
							if (rand()&1)
								Con_Printf("\n^2New spam has arrived\n");
							else
								Con_Printf("\n^2You have new spam\n");
						}
						else if (rand()&1)
							Con_Printf("\n^2New mail has arrived\n");
						else
							Con_Printf("\n^2You have new mail\n");

						Con_Printf("Subject: %s\n", subject);
						Con_Printf("From: %s\n", from);

						SCR_CenterPrint(va("NEW MAIL HAS ARRIVED\n\nTo: %s@%s\nFrom: %s\nSubject: %s", imap->username, imap->server, from, subject));

						//throw the rest away.
					}
				}
			}
			else
			{
				Con_Printf("Bad client state\n");
				return false;
			}
			imap->readlen -= ending - imap->readbuffer;
			memmove(imap->readbuffer, ending, strlen(ending)+1);
		}
	}
	if (imap->drop)
		return false;

	if (imap->state == IMAP_AUTHED)
	{
		if (imap->lastnoop + imap_checkfrequency < Sys_Milliseconds())
		{	//we need to keep the connection reasonably active

			IMAP_EmitCommand(imap, "SELECT INBOX");	//this causes the recent flags to be reset. This is the only way I found.
			imap->lastnoop = Sys_Milliseconds();
		}
	}

	if (imap->sendlen)
	{
		len = Net_Send(imap->socket, imap->sendbuffer, imap->sendlen);
		if (len>0)
		{
			imap->sendlen-=len;
			memmove(imap->sendbuffer, imap->sendbuffer+len, imap->sendlen+1);
		}
	}
	return true;
}

void IMAP_Think (void)
{
	imap_con_t *prev = NULL;
	imap_con_t *imap;

	for (imap = imapsv; imap; imap = imap->next)
	{
		if (imap->drop || !IMAP_ThinkCon(imap))
		{
			if (!prev)
				imapsv = imap->next;
			else
				prev->next = imap->next;
			Net_Close(imap->socket);
			free(imap);
			if (!prev)
				break;
		}

		prev = imap;
	}
}
