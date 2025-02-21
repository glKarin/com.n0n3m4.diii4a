#include "quakedef.h"

#ifdef FTPCLIENT

#include "iweb.h"

#include "netinc.h"

typedef struct FTPclientconn_s{
	char server[256];
	char name[64];
	char pwd[64];
	char path[256];
	char pathprefix[256];	//Urhum.. Without this we can browse various entire hard drives too easily.
	char file[64];
	char localfile[MAX_QPATH];

	int transfersize;
	int transfered;

	int controlsock;
	int datasock;	//FTP only allows one transfer per connection.

	enum {ftp_control, ftp_listing, ftp_getting, ftp_putting} type;
	int stage;

	vfsfile_t *f;

	struct FTPclientconn_s *next;

	void (*NotifyFunction)(vfsfile_t *file, char *localfile, qboolean sucess);	//called when failed or succeeded, and only if it got a connection in the first place.
																//ftp doesn't guarentee it for anything other than getting though. :(
} FTPclientconn_t;

FTPclientconn_t *FTPclientconn;

FTPclientconn_t *FTP_CreateConnection(char *addy)
{
	unsigned long _true = true;
	struct sockaddr_qstorage	from;
	FTPclientconn_t *con;



	con = IWebMalloc(sizeof(FTPclientconn_t));

	

	if ((con->controlsock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		Sys_Error ("FTP_CreateConnection: socket: %s\n", strerror(qerrno));
	}


	{//quake routines using dns and stuff (Really, I wanna keep quake and ftp fairly seperate)
		netadr_t qaddy;		
		NET_StringToAdr (addy, &qaddy);
		if (!qaddy.port)
			qaddy.port = htons(21);
		NetadrToSockadr(&qaddy, &from);
	}

	//not yet blocking.
	if (connect(con->controlsock, (struct sockaddr *)&from, sizeof(from)) == -1)
	{
		IWebWarnPrintf ("FTP_TCP_OpenSocket: connect: %i %s\n", qerrno, strerror(qerrno));
		closesocket(con->controlsock);		
		IWebFree(con);
		return NULL;
	}
	
	if (ioctlsocket (con->controlsock, FIONBIO, &_true) == -1)	//now make it non blocking.
	{
		Sys_Error ("FTP_TCP_OpenSocket: ioctl FIONBIO: %s\n", strerror(qerrno));
	}

	Q_strncpyz(con->server, addy, sizeof(con->server));
	strcpy(con->name, "anonymous");

	con->next = FTPclientconn;
	FTPclientconn = con;
	con->stage = 1;
	con->type = ftp_control;

	strcpy(con->path, "/");
	con->datasock = INVALID_SOCKET;	
	con->transfersize = -1;
	con->transfered = 0;

	return FTPclientconn;
}
//duplicate a connection to get multiple data channels with a server.
FTPclientconn_t *FTP_DuplicateConnection(FTPclientconn_t *old)
{
	FTPclientconn_t *newf;
	newf = FTP_CreateConnection(old->server);
	*newf->server = '\0';	//mark it as non control
	strcpy(newf->name, old->name);
	strcpy(newf->pwd, old->pwd);
	strcpy(newf->path, old->path);
	strcpy(newf->pathprefix, old->pathprefix);

	return newf;
}

int FTP_CL_makelistensocket(void)
{
	char name[256];
	unsigned long _true = true;
	int sock;
	struct hostent *hent;
	
	struct sockaddr_in	address;
//	int fromlen;

	address.sin_family = AF_INET;
	if (gethostname(name, sizeof(name)) == -1)
		return INVALID_SOCKET;
	hent = gethostbyname(name);
	if (!hent)
		return INVALID_SOCKET;
	address.sin_addr.s_addr = *(int *)(hent->h_addr_list[0]);
	address.sin_port = 0;



	if ((sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		Sys_Error ("FTP_TCP_OpenSocket: socket: %s", strerror(qerrno));
	}

	if (ioctlsocket (sock, FIONBIO, &_true) == -1)
	{
		Sys_Error ("FTP_TCP_OpenSocket: ioctl FIONBIO: %s", strerror(qerrno));
	}
	
	if( bind (sock, (void *)&address, sizeof(address)) == -1)
	{
		closesocket(sock);
		return INVALID_SOCKET;
	}
	
	listen(sock, 1);

	return sock;
}
int FTP_CL_makeconnectsocket(char *ftpdest)
{
	unsigned long _true = true;
	int sock;
	
	struct sockaddr_in	address;

	if (!ftpdest)
		return 0;
	if (*ftpdest == '(')
		ftpdest++;

	if ((sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		IWebWarnPrintf ("FTP_CL_makeconnectsocket: socket: %s", strerror(qerrno));
		return INVALID_SOCKET;
	}

	if (ioctlsocket (sock, FIONBIO, &_true) == -1)
	{
		closesocket(sock);
		IWebWarnPrintf ("FTP_CL_makeconnectsocket: ioctl FIONBIO: %s", strerror(qerrno));
		return INVALID_SOCKET;
	}

	address.sin_family = AF_INET;

	address.sin_addr.s_addr = INADDR_ANY;

	address.sin_port = 0;
	
	if( bind (sock, (void *)&address, sizeof(address)) == -1)
	{
		closesocket(sock);

		IWebWarnPrintf ("FTP_CL_makeconnectsocket: bind: %s", strerror(qerrno));
		return INVALID_SOCKET;
	}

	FTP_StringToAdr(ftpdest, (qbyte *)&address.sin_addr, (qbyte *)&address.sin_port);

	//this is commented out because connect always reports would_block, no matter what happens. So why check?
	//if (
		connect(sock, (struct sockaddr *)&address, sizeof(address));// == -1)
/*	{
		closesocket(sock);

		Con_Printf ("FTP_CL_makeconnectsocket: ioctl FIONBIO: %s", strerror(qerrno));
		return INVALID_SOCKET;
	}
*/
	return sock;
}

iwboolean	FTP_SocketToString (int socket, char *s)
{
	struct sockaddr_in addr;
	int adrlen = sizeof(addr);

	if (getsockname(socket, (struct sockaddr*)&addr, &adrlen) == -1)
		return false;
	
	sprintf(s, "%i,%i,%i,%i,%i,%i", ((qbyte*)&addr.sin_addr)[0], ((qbyte*)&addr.sin_addr)[1], ((qbyte*)&addr.sin_addr)[2], ((qbyte*)&addr.sin_addr)[3], ((qbyte *)&addr.sin_port)[0], ((qbyte *)&addr.sin_port)[1]);
	return true;
}

iwboolean FTP_ClientConnThink (FTPclientconn_t *con)	//true to kill con
{
	unsigned long _true = true;
	char *line, *msg;
	int ret;

	char readdata[8192];
	char tempbuff[8192];

	if (con->stage == 6)
	{
		int len;
		if (con->type == ftp_getting)
		{
			if (!cls.downloadmethod || (cls.downloadmethod == DL_FTP && !strcmp(cls.downloadlocalname, con->localfile)))
			{
				strcpy(cls.downloadlocalname, con->localfile);
				strcpy(cls.downloadremotename, con->localfile);
				cls.downloadmethod = DL_FTP;
				if (con->transfersize == -1)
					cls.downloadpercent=50;
				else
					cls.downloadpercent = con->transfered*100.0f/con->transfersize;
			}
			while((len = recv(con->datasock, readdata, sizeof(readdata), 0)) >0 )
			{			
				VFS_WRITE(con->f, readdata, len);
				con->transfered += len;
			}
			if (len == 0)
			{
				closesocket(con->datasock);
				con->datasock = INVALID_SOCKET;
			}
		}
		else if (con->type == ftp_putting)
		{
			int pos, sent;
			int ammount, wanted = sizeof(readdata);

			pos = VFS_TELL(con->f);
			ammount = VFS_READ(con->f, readdata, wanted);
			sent = send(con->datasock, readdata, ammount, 0);
			if (sent == -1)
				VFS_SEEK(con->f, pos);	//go back. Too much data
			else
			{
				VFS_SEEK(con->f, pos + sent);	//written this much

				if (!ammount)	//file is over
				{
					closesocket(con->datasock);
					con->datasock = INVALID_SOCKET;

	//				msg = "226 Transfer complete.\r\n";
	//				send (con->controlsock, msg, strlen(msg), 0);
				}
			}
		}
	}

	ret = recv(con->controlsock, (char *)readdata, sizeof(readdata)-1, 0);
	if (ret == -1)
	{
		if (qerrno == EWOULDBLOCK)
			return false;

		if (qerrno == ECONNABORTED || qerrno == ECONNRESET)
		{
			Con_TPrintf ("Connection lost or aborted\n");
			return true;
		}

//		Con_Printf ("NET_GetPacket: %s\n", strerror(qerrno));
		return true;
	}	

	readdata[ret] = '\0';	//null terminate. (it's a string)

	//we now have a message.
	//We've got to work out what has happened already.

	//a server can send many lines of text for one reply
	//220-hello
	// this
	//220-is
	// 220-all
	//220 one reply
	//so we only read lines that contain number space words, without any leading space
	line = readdata;
	while (1)
	{		
		msg = line;
		while (*line)
		{
			if (*line == '\n')
				break;
			if (*line == '\r')
				break;
			line++;
		}					
		if (!*line)	//broken message
			break;
		*line = '\0';
		line++;

		if (*con->server)
			IWebDPrintf("FTP: %s\n", COM_TrimString(msg));

		if (*msg < '0' || *msg > '9')	//make sure it starts with number
			continue;
		ret = atoi(msg);
		while(*msg >= '0' && *msg <= '9')	//find next non number
			msg++;
		if (*msg != ' ')	//must be a space (definatly not a '-')
			continue;
		msg++;		

		if (ret == 220)
		{
			sprintf(tempbuff, "USER %s\r\n", con->name);
			send(con->controlsock, tempbuff, strlen(tempbuff), 0);
			con->stage = 1;
		}
		else if (ret == 331)
		{
			if (con->type == ftp_control)
				sprintf(tempbuff, "PASS %s\r\nPWD %s\r\n", con->pwd, con->path);
			else
				sprintf(tempbuff, "PASS %s\r\n", con->pwd);
			send(con->controlsock, tempbuff, strlen(tempbuff), 0);
			con->stage = 2;
		}
		else if (ret == 230)	//we must now do something useful
		{
			char adr[64];
			if (con->type == ftp_control)	//control is for browsing and duplicating
				continue;

			con->datasock = FTP_CL_makelistensocket();
			if (!con->datasock || !FTP_SocketToString(con->datasock, adr))
			{
				return true;
			}

			sprintf(tempbuff, "CWD %s%s\r\n", con->pathprefix, con->path);
			send(con->controlsock, tempbuff, strlen(tempbuff), 0);

			goto usepasv;
			sprintf(tempbuff, "PORT %s\r\n", adr);
			send(con->controlsock, tempbuff, strlen(tempbuff), 0);

			con->stage = 3;
		}
		else if (ret == 200)
		{
			struct sockaddr addr;
			int addrlen = sizeof(addr);
			int temp;
			if (con->type == ftp_control)
				continue;
			if (con->stage == 3)
			{
				temp = accept(con->datasock, &addr, &addrlen);
				closesocket(con->datasock);
				con->datasock = temp;

				if (temp != INVALID_SOCKET)
				{
					ioctlsocket(temp, FIONBIO, &_true);
					con->stage = 6;
					if (con->type == ftp_getting)
					{
						con->f = FS_OpenVFS(con->localfile, "wb", FS_GAME);
						if (con->f)
						{
							sprintf(tempbuff, "RETR %s\r\n", con->file);
							con->stage = 6;
							con->transfered = 0;
							con->transfersize = -1;
						}
						else
						{
							sprintf(tempbuff, "QUIT\r\n");
							con->stage = 7;
						}
					}
					else if (con->type == ftp_putting)
					{
						con->f = FS_OpenVFS (con->localfile, "rb", FS_GAME);
						if (con->f)
						{
							sprintf(tempbuff, "STOR %s\r\n", con->file);
							con->stage = 6;
							con->transfered = 0;
							con->transfersize = VFS_GETLEN(con->f);
						}
						else
						{
							sprintf(tempbuff, "QUIT\r\n");
							con->stage = 7;
						}
					}
					else
						sprintf(tempbuff, "LIST %s\r\n", con->pwd);
					send(con->controlsock, tempbuff, strlen(tempbuff), 0);
				}
				else
				{
usepasv:
					Con_Printf("FTP: Trying passive server mode\n");
					msg = va("PASV\r\n");
					send(con->controlsock, msg, strlen(msg), 0);
					con->stage = 4;
				}
			}
		}
		else if (ret == 213)
		{
			con->transfersize = atoi(msg);
			msg = va("RETR %s\r\n", con->file);
			con->stage = 6;
			con->transfered = 0;
			send(con->controlsock, msg, strlen(msg), 0);
		}
		else if (ret == 125)	//begining transfer
		{
			if (con->type == ftp_getting)
			{
				char tempname[MAX_OSPATH];
				COM_StripExtension(con->localfile, tempname, MAX_OSPATH);
				strcat(tempname, ".tmp");
				con->f = FS_OpenVFS (tempname, "wb", FS_GAME);
				if (!con->f)
				{
					msg = va("ABOR\r\nQUIT\r\n");	//bummer. we couldn't open this file to output to.
					send(con->controlsock, msg, strlen(msg), 0);
					con->stage = 7;
					return true;
				}
			}
//			msg = va("LIST\r\n");
//			send(con->controlsock, msg, strlen(msg), 0);
			con->stage = 6;
		}
		else if (ret == 226)	//transfer complete
		{
			int len;
			char data[1024];
			if (con->f)
			{
				if (con->type == ftp_getting)
				{
					while(1)	//this is potentially dodgy.
					{
						len = recv(con->datasock, data, sizeof(data), 0);
						if (len == 0)
							break;
						if (len == -1)
						{
							if (qerrno != EWOULDBLOCK)
								break;
							continue;
						}
						con->transfered+=len;
						data[len] = 0;
						VFS_WRITE(con->f, data, len);
					}
				}
				VFS_CLOSE(con->f);
				con->f = NULL;
				closesocket(con->datasock);
				con->datasock = INVALID_SOCKET;

				if (con->NotifyFunction)
				{
					con->NotifyFunction(con->localfile, true);
					con->NotifyFunction = NULL;
				}

				if (con->transfersize != -1 && con->transfered != con->transfersize)
				{
					IWebPrintf("Transfer corrupt\nTransfered %i of %i bytes\n", con->transfered, con->transfersize);
				}
				else
					IWebPrintf("Transfer compleate\n");
			}
			else
			{
				while((len = recv(con->datasock, data, sizeof(data), 0)) >0 )
				{
					data[len] = 0;
					if (strchr(data, '\r'))
					{
						line = data;
						for(;;)
						{
							msg = strchr(line, '\r');
							if (!msg)
								break;
							*msg = '\0';
							Con_Printf("%s", line);
							line = msg+1;
						}
						Con_Printf("%s", line);
					}
					else
						Con_Printf("%s", data);
				}
				closesocket(con->datasock);
				con->datasock = INVALID_SOCKET;
			}
			msg = va("QUIT\r\n");
			send(con->controlsock, msg, strlen(msg), 0);

			con->stage = 7;
		}
		else if (ret == 227)
		{
//			Con_Printf("FTP: Got passive server mode\n");
			if (con->datasock != INVALID_SOCKET)
				closesocket(con->datasock);	
			con->datasock = INVALID_SOCKET;

			con->datasock = FTP_CL_makeconnectsocket(strchr(msg, '('));
			if (con->datasock != INVALID_SOCKET)
			{
				if (con->type == ftp_getting)
				{
					con->f = FS_OpenVFS(con->localfile, "wb", FS_GAME);
					if (con->f)
					{
						con->stage = 8;
						msg = va("TYPE I\r\nSIZE %s\r\n", con->file);
						con->transfersize = -1;

						/*
						msg = va("RETR %s\r\n", con->file);
						con->stage = 6;
						con->transfered = 0;
						*/
					}
					else
					{
						msg = va("QUIT\r\n");
						con->stage = 7;
						Con_Printf("FTP: Failed to open local file %s\n", con->localfile);
					}
				}
				else if (con->type == ftp_putting)
				{
					con->f = FS_OpenVFS(con->localfile, "rb", FS_GAME);
					if (con->f)
					{
						msg = va("STOR %s\r\n", con->file);
						con->stage = 6;
						con->transfered = 0;
						con->transfersize = VFS_GETLEN(con->f);
					}
					else
					{
						msg = va("QUIT\r\n");
						con->stage = 7;
						Con_Printf("FTP: Failed to open local file %s\n", con->localfile);
					}
				}
				else
				{
					msg = "LIST\r\n";
					con->stage = 6;
				}
			}
			else
			{
				msg = "QUIT\r\n";
				con->stage = 7;
				Con_Printf("FTP: Didn't connect\n");
			}

			send (con->controlsock, msg, strlen(msg), 0);
		}
		else if (ret == 250)
		{
			Con_Printf("FTP: %i %s\n", ret, msg);
		}
		else if (ret == 257)
		{	//stick it on the beginning.
			Con_Printf("FTP: %i %s\n", ret, msg);
			msg = strchr(msg, '"');
			if (msg)
			{
				Q_strncpyz(con->pathprefix, msg+1, sizeof(con->pathprefix));
				msg = strchr(con->pathprefix, '"');
				if (msg)
					*msg = '\0';
			}
			else
				Q_strcpyline(con->pathprefix, msg+4, sizeof(con->pathprefix)-1);
		}
		else
		{
			if (ret < 200)
				continue;
			if (con->stage == 5)
			{
				Con_DPrintf("FTP: Trying passive server mode\n");
				msg = va("PASV\r\n");
				send(con->controlsock, msg, strlen(msg), 0);
				con->stage = 4;
				continue;
			}
			if (ret != 221)
				Con_Printf(CON_ERROR "FTP: %i %s\n", ret, msg);
			return true;
		}

		continue;
	}
	return false;
}

void FTP_ClientThink (void)
{
	FTPclientconn_t *con, *old=NULL;
	for (con = FTPclientconn; con; con = con->next)
	{
		if (FTP_ClientConnThink(con))
		{
			if (con->NotifyFunction)
				con->NotifyFunction(con->localfile, false);

			if (cls.downloadmethod == DL_FTP && !strcmp(cls.downloadlocalname, con->localfile))
			{	//this was us
				cls.downloadmethod = DL_NONE;
			}
			if (con->f)
				VFS_CLOSE(con->f);
			if (con->controlsock != INVALID_SOCKET)
				closesocket(con->controlsock);
			if (con->datasock != INVALID_SOCKET)
				closesocket(con->datasock);
			if (!old)
			{
				FTPclientconn = con->next;
				IWebFree(con);				
				break;
			}
			else
			{
				old->next = con->next;
				IWebFree(con);

				break;
			}
		}
		old = con;
	}
}

FTPclientconn_t *FTP_FindControl(void)
{
	FTPclientconn_t *con;

	for (con = FTPclientconn; con; con = con->next)
	{
		if (*con->server)
			return con;
	}
	return NULL;
}

void FTP_FixupPath(char *out)
{	//convert \[ to [
	char *in;
	in = out;
	while (*in)
	{
		if (*in == '\\')
		{
			in++;
			if (*in == '\0')
				break;
		}
		*out++ = *in++;
	}
	*out++ = '\0';
}

qboolean FTP_Client_Command (char *cmd, void (*NotifyFunction)(char *localfile, qboolean sucess))
{
	char command[64];
	char server[MAX_OSPATH];
	FTPclientconn_t *con;

	cmd = COM_ParseOut(cmd, command, sizeof(command));
	if (!stricmp(command, "open"))
	{
		if (FTP_FindControl())
			Con_Printf("You are already connected\n");
		else
		{
			cmd = COM_ParseOut(cmd, server, sizeof(server));
			if ((con = FTP_CreateConnection(server)))
			{
				Con_Printf("FTP connect succeded\n");
				cmd = COM_ParseOut(cmd, command, sizeof(command));
				if (cmd)
				{
					Q_strncpyz(con->name, command, sizeof(con->name));
					cmd = COM_ParseOut(cmd, command, sizeof(command));
					if (cmd)
						Q_strncpyz(con->pwd, command, sizeof(con->pwd));
				}

				return true;
			}
			else
				Con_Printf("FTP connect failed\n");
		}

		return false;
	}
	else if (!stricmp(command, "download"))
	{
		cmd = COM_ParseOut(cmd, server, sizeof(server));
		con = FTP_CreateConnection(server);
		if (!con)
		{
			Con_Printf("FTP: Couldn't connect\n");
			return false;
		}
		con->NotifyFunction = NotifyFunction;
		*con->server = '\0';
		con->type = ftp_getting;
		cmd = COM_ParseOut(cmd, server, sizeof(server));
		Q_strncpyz(con->file, server, sizeof(con->file));
		Q_strncpyz(con->localfile, server, sizeof(con->localfile));

		if ((cmd = COM_ParseOut(cmd, server, sizeof(server))))
			Q_strncpyz(con->localfile, server, sizeof(con->localfile));

		FTP_FixupPath(con->file);
		FTP_FixupPath(con->localfile);

		return true;
	}
	else if (!stricmp(command, "quit"))
	{
		con = FTP_FindControl();
		if (con)
		{
			char *msg;
			msg = va("QUIT\r\n");
			send(con->controlsock, msg, strlen(msg), 0);
//			if (con->datasock)
//				closesocket(con->datasock);
//			closesocket(con->controlsock);
		}
		else
			Con_Printf("No main FTP connection\n");

		return true;
	}
	else if (!stricmp(command, "list"))
	{
		FTPclientconn_t *newf, *con = FTP_FindControl();
		if (!con)
		{
			Con_Printf("Not connected\n");
			return false;
		}

		newf = FTP_DuplicateConnection(con);
		if (!newf)
		{
			Con_Printf("Failed duplicate connection\n");
			return false;
		}
		newf->type = ftp_listing;
		newf->NotifyFunction = NotifyFunction;
		return true;
	}
	else if (!stricmp(command, "get"))
	{
		FTPclientconn_t *newf, *con = FTP_FindControl();
		if (!con)
		{
			Con_Printf("Not connected\n");
			return false;
		}

		cmd = COM_ParseOut(cmd, command, sizeof(command));
		if (!cmd)
		{
			Con_Printf("No file specified\n");
			return false;
		}

		newf = FTP_DuplicateConnection(con);
		if (!newf)
		{
			Con_Printf("Failed duplicate connection\n");
			return false;
		}
		newf->NotifyFunction = NotifyFunction;
		newf->type = ftp_getting;
		sprintf(newf->file, command);
		sprintf(newf->localfile, "%s%s", newf->path, command);
		return true;
	}
	else if (!stricmp(command, "put"))
	{
		FTPclientconn_t *newf, *con = FTP_FindControl();
		if (!con)
		{
			Con_Printf("Not connected\n");
			return false;
		}

		cmd = COM_ParseOut(cmd, command, sizeof(command));
		if (!cmd)
		{
			Con_Printf("No file specified\n");
			return false;
		}

		newf = FTP_DuplicateConnection(con);
		if (!newf)
		{
			Con_Printf("Failed duplicate connection\n");
			return false;
		}
		newf->NotifyFunction = NotifyFunction;
		newf->type = ftp_putting;
		sprintf(newf->file, command);
		sprintf(newf->localfile, "%s%s", newf->path, command);

		return true;
	}
	else if (!stricmp(command, "cwd"))
	{
		FTPclientconn_t *con = FTP_FindControl();
		if (!con)
		{
			Con_Printf("Not connected\n");
			return false;
		}
		Con_Printf("%s\n", con->path);
		return true;
	}
	else if (!stricmp(command, "cd"))
	{
		char *msg;
		FTPclientconn_t *con = FTP_FindControl();
		if (!con)
		{
			Con_Printf("Not connected\n");
			return false;
		}

		cmd = COM_ParseOut(cmd, command, sizeof(command));

		if (*command == '/')	//absolute
			Q_strncpyz(con->path, command, sizeof(con->path));
		else	//bung it on the end
		{
			strncat(con->path, "/", sizeof(con->path)-1);
			strncat(con->path, command, sizeof(con->path)-1);
		}

		msg = va("CWD %s%s\r\n", con->pathprefix, con->path);
		if (send(con->controlsock, msg, strlen(msg), 0)==strlen(msg))
			return true;
	}
	else
		Con_Printf("Unrecognised FTP command\n");
	/*
	com = COM_ParseOut(com, command, sizeof(command));
	com = COM_ParseOut(com, command, sizeof(command));
	com = COM_ParseOut(com, command, sizeof(command));
	*/

	return false;
}

#endif
