#include "qtv.h"

#define CENTERTIME 1.5

void Menu_Enter(cluster_t *cluster, viewer_t *viewer, int buttonnum)
{
	//build a possible message, even though it'll probably not be sent

	sv_t *sv;
	int i, min;

	switch(viewer->menunum)
	{
	default:
		if (buttonnum < 0)
			QW_SetMenu(viewer, MENU_MAIN);	//no other sort of back button...
		break;

	case MENU_MAIN:
		if (buttonnum < 0)
			viewer->menuop -= (MENU_MAIN_ITEMCOUNT + 1)/2;
		else if (buttonnum > 0)
			viewer->menuop += (MENU_MAIN_ITEMCOUNT + 1)/2;
		else if (buttonnum == 0)
		{
			switch(viewer->menuop)
			{
			case MENU_MAIN_STREAMS: //Streams
				QW_SetMenu(viewer, MENU_SERVERS);
				break;
			case MENU_MAIN_CLIENTLIST://Client List
				QW_SetMenu(viewer, MENU_CLIENTS);
				break;

			case MENU_MAIN_NEWSTREAM://New Stream
				QW_PrintfToViewer(viewer, "Not implemented yet\n");
				break;
			case MENU_MAIN_DEMOS://Demos
				Cluster_BuildAvailableDemoList(cluster);
				QW_SetMenu(viewer, MENU_DEMOS);
				break;

			case MENU_MAIN_SERVERBROWSER://Server Browser
				QW_PrintfToViewer(viewer, "Not implemented yet\n");
				break;
			case MENU_MAIN_ADMIN://Admin
				QW_SetMenu(viewer, MENU_ADMIN);
				break;

			case MENU_MAIN_PREVPROX://Previous Proxy
				if (viewer->isproxy)
				{
					QW_SetMenu(viewer, MENU_NONE);
					QW_StuffcmdToViewer(viewer, "say proxy:menu\n");
				}
				else
					QW_PrintfToViewer(viewer, "No client proxy detected\n");
				break;
			case MENU_MAIN_NEXTPROX://Next Proxy
				if (viewer->server && viewer->server->serverisproxy && viewer->server->controller == viewer)
				{
					viewer->server->proxyisselected = false;
					QW_SetMenu(viewer, MENU_NONE);
					SendClientCommand(viewer->server, "say .menu");
				}
				else
					QW_PrintfToViewer(viewer, "No server proxy detected\n");
				break;
			
			case MENU_MAIN_HELP://Help Menu
				QW_PrintfToViewer(viewer, "Not implemented yet\n");
				break;
			}
		}
		break;

	case MENU_CLIENTS:
		if (buttonnum >= 0)
		{
			viewer_t *v = cluster->viewers;
			for (i = 0; i < viewer->menuop && v; i++)
				v = v->next;
			if (!v)
				break;
			if (v == viewer)
			{
				if (viewer->commentator)
					QW_SetCommentator(cluster, viewer, NULL);
				else
					QW_PrintfToViewer(viewer, "Please stop touching yourself\n");
			}
			else
				QW_SetCommentator(cluster, viewer, v);
		}
		else
			QW_SetMenu(viewer, MENU_MAIN);
		break;

	case MENU_DEMOS:
		if (buttonnum >= 0)
			QW_StuffcmdToViewer(viewer, "say .demo %s\n", cluster->availdemos[viewer->menuop].name);
		else
			QW_SetMenu(viewer, MENU_MAIN);
		break;

	case MENU_ADMINSERVER:
		if (viewer->server)
		{
			i = 0;
			sv = viewer->server;
			if (i++ == viewer->menuop)
			{	//auto disconnect
				sv->autodisconnect = sv->autodisconnect?AD_NO:AD_WHENEMPTY;
			}
			if (i++ == viewer->menuop)
			{	//disconnect
				QTV_ShutdownStream(viewer->server);
			}
			if (i++ == viewer->menuop)
			{
				if (sv->controller == viewer)
					sv->controller = NULL;
				else
				{
					sv->controller = viewer;
					sv->controllersquencebias = viewer->netchan.outgoing_sequence - sv->netchan.outgoing_sequence;
				}
			}
			if (i++ == viewer->menuop)
			{	//back
				QW_SetMenu(viewer, MENU_ADMIN);
			}
			break;
		}
		//fallthrough
	case MENU_SERVERS:
		if (buttonnum < 0)
			QW_SetMenu(viewer, MENU_MAIN);
		else if (!cluster->servers)
		{
			QW_StuffcmdToViewer(viewer, "echo Please enter a server ip\nmessagemode\n");
			strcpy(viewer->expectcommand, "insecadddemo");
		}
		else
		{
			if (viewer->menuop < 0)
				viewer->menuop = 0;
			i = 0;
			min = viewer->menuop - 10;
			if (min < 0)
				min = 0;
			for (sv = cluster->servers; sv && i<min; sv = sv->next, i++)
			{//skip over the early connections.
			}
			min+=20;
			for (; sv && i < min; sv = sv->next, i++)
			{
				if (i == viewer->menuop)
				{
					/*if (sv->parsingconnectiondata || !sv->modellist[1].name[0])
					{
						QW_PrintfToViewer(viewer, "But that stream isn't connected\n");
					}
					else*/
					{
						QW_SetViewersServer(cluster, viewer, sv);
						QW_SetMenu(viewer, MENU_NONE);
						viewer->thinksitsconnected = false;
					}
					break;
				}
			}
		}
		break;
	case MENU_ADMIN:
		i = 0;
		if (i++ == viewer->menuop)
		{	//connection stuff
			QW_SetMenu(viewer, MENU_ADMINSERVER);
		}
		if (i++ == viewer->menuop)
		{	//qw port
			QW_StuffcmdToViewer(viewer, "echo You will need to reconnect\n");
			cluster->qwlistenportnum += (buttonnum<0)?-1:1;
		}
		if (i++ == viewer->menuop)
		{	//hostname
			strcpy(viewer->expectcommand, "hostname");
			QW_StuffcmdToViewer(viewer, "echo Please enter the new hostname\nmessagemode\n");
		}
		if (i++ == viewer->menuop)
		{	//master
			strcpy(viewer->expectcommand, "master");
			QW_StuffcmdToViewer(viewer, "echo Please enter the master dns or ip\necho Enter '.' for masterless mode\nmessagemode\n");
		}
		if (i++ == viewer->menuop)
		{	//password
			strcpy(viewer->expectcommand, "password");
			QW_StuffcmdToViewer(viewer, "echo Please enter the new rcon password\nmessagemode\n");
		}
		if (i++ == viewer->menuop)
		{	//add server
			strcpy(viewer->expectcommand, "messagemode");
			QW_StuffcmdToViewer(viewer, "echo Please enter the new qtv server dns or ip\naddserver\n");
		}
		if (i++ == viewer->menuop)
		{	//add demo
			strcpy(viewer->expectcommand, "adddemo");
			QW_StuffcmdToViewer(viewer, "echo Please enter the name of the demo to play\nmessagemode\n");
		}
		if (i++ == viewer->menuop)
		{	//choke
			cluster->chokeonnotupdated ^= 1;
		}
		if (i++ == viewer->menuop)
		{	//late forwarding
			cluster->lateforward ^= 1;
		}
		if (i++ == viewer->menuop)
		{	//no talking
			cluster->notalking ^= 1;
		}
		if (i++ == viewer->menuop)
		{	//nobsp
			cluster->nobsp ^= 1;
		}
		if (i++ == viewer->menuop)
		{	//back
			QW_SetMenu(viewer, MENU_NONE);
		}

		break;
	}
}

void WriteStringSelection(netmsg_t *b, qboolean selected, const char *str)
{
	if (selected)
	{
		WriteByte(b, 13);
		while(*str)
			WriteByte(b, 128|*str++);
	}
	else
	{
		WriteByte(b, ' ');
		while(*str)
			WriteByte(b, *str++);
	}
}

void Menu_Draw(cluster_t *cluster, viewer_t *viewer)
{
	char buffer[2048];
	char str[256];
	sv_t *sv;
	int i, min;
	unsigned char *s;

	netmsg_t m;

	if (viewer->backbuffered)
		return;

	if (viewer->menunum == MENU_FORWARDING)
		return;

	if (viewer->menuspamtime > cluster->curtime && viewer->menuspamtime < cluster->curtime + CENTERTIME*2000)
		return;
	viewer->menuspamtime = cluster->curtime + CENTERTIME*1000;

	InitNetMsg(&m, buffer, sizeof(buffer));

	WriteByte(&m, svc_centerprint);

	WriteString2(&m, "/PFTEQTV "QTV_VERSION_STRING"\n");
	WriteString2(&m, PROXYWEBSITE"\n");
	WriteString2(&m, "-------------\n");
	
	if (strcmp(cluster->hostname, DEFAULT_HOSTNAME))
		WriteString2(&m, cluster->hostname);

	switch(viewer->menunum)
	{
	default:
		WriteString2(&m, "bad menu");
		break;

	case MENU_MAIN:
		{
			WriteString2(&m, "\n\x1d\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1f\n");
			while (viewer->menuop < 0)
				viewer->menuop += MENU_MAIN_ITEMCOUNT;
			while (viewer->menuop >= MENU_MAIN_ITEMCOUNT)
				viewer->menuop -= MENU_MAIN_ITEMCOUNT;
			i = viewer->menuop;	

			WriteStringSelection(&m, i==MENU_MAIN_STREAMS,		"Streams          ");
			WriteStringSelection(&m, i==MENU_MAIN_CLIENTLIST,	"Client List      ");
			WriteByte(&m, '\n');
			WriteStringSelection(&m, i==MENU_MAIN_NEWSTREAM,	"New Stream       ");
			WriteStringSelection(&m, i==MENU_MAIN_DEMOS,		"Demos            ");
			WriteByte(&m, '\n');
			WriteStringSelection(&m, i==MENU_MAIN_SERVERBROWSER,"Server Browser   ");
			WriteStringSelection(&m, i==MENU_MAIN_ADMIN,		"Admin            ");
			WriteByte(&m, '\n');
			WriteStringSelection(&m, i==MENU_MAIN_PREVPROX,		"Previous Proxy   ");
			WriteStringSelection(&m, i==MENU_MAIN_NEXTPROX,		"Next Proxy       ");
			WriteByte(&m, '\n');
			WriteStringSelection(&m, i==MENU_MAIN_HELP,			"Help             ");
			WriteString2(&m,									"                  ");

			WriteString2(&m, "\n\x1d\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1f\n");
		}
		break;

	case MENU_CLIENTS:
		{
			int start;
			viewer_t *v;
			char *srv;
			int c;
			v = cluster->viewers;

			if (viewer->menuop < 0)
				viewer->menuop = 0;
			if (viewer->menuop > cluster->numviewers - 1)
				viewer->menuop = cluster->numviewers - 1;

			WriteString2(&m, "\nActive Clients\n\n");

			start = viewer->menuop & ~7;
			for (i = 0; i < start && v; i++)
				v = v->next;
			for (i = start; i < start+8 && v; i++, v = v->next)
			{
				for (c = strlen(v->name); c < 14; c++)
					WriteByte(&m, ' ');
				WriteStringSelection(&m, viewer->menuop == i, v->name);
				WriteString2(&m, ": ");
				if (v->server)
				{
					if (!v->server->sourcefile && !v->server->parsingconnectiondata)
						srv = v->server->map.hostname;
					else
						srv = v->server->server;
				}
				else
					srv = "None";
				for (c = 0; c < 20; c++)
				{
					if (*srv)
						WriteByte(&m, *srv++);
					else
						WriteByte(&m, ' ');
				}

				WriteByte(&m, '\n');
			}
			for (; i < start+8; i++)
				WriteByte(&m, '\n');
		}
		break;


	case MENU_DEMOS:
		{
			int start;

			WriteString2(&m, "\nAvailable Demos\n\n");

			if (cluster->availdemoscount == 0)
			{
				WriteString2(&m, "No demos are available");
				break;
			}
			
			if (viewer->menuop < 0)
				viewer->menuop = 0;
			if (viewer->menuop > cluster->availdemoscount-1)
				viewer->menuop = cluster->availdemoscount-1;

			start = viewer->menuop & ~7;
			for (i = start; i < start+8; i++)
			{
				char cleanname[128];
				char *us;
				strlcpy(cleanname, cluster->availdemos[i].name, sizeof(cleanname));
				for (us = cleanname; *us; us++)
					if (*us == '_')
						*us = ' ';
				WriteStringSelection(&m, i == viewer->menuop,	cleanname);
				WriteByte(&m, '\n');
			}
		}
		break;

	case MENU_ADMINSERVER:	//per-connection options
		if (viewer->server)
		{
			sv = viewer->server;
			WriteString2(&m, "\n\nConnection Admin\n");
			WriteString2(&m, sv->map.hostname);
			if (sv->sourcefile)
				WriteString2(&m, " (demo)");
			WriteString2(&m, "\n\n");

			if (viewer->menuop < 0)
				viewer->menuop = 0;

			i = 0;

			WriteString2(&m, " auto disconnect");
			WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
			switch(viewer->server->autodisconnect)
			{
			default:
			case AD_NO:
				sprintf(str, "%-20s", "permanent connection");
				break;
			case AD_REVERSECONNECT:
				sprintf(str, "%-20s", "when server disconnects");
				break;
			case AD_WHENEMPTY:
				sprintf(str, "%-20s", "when inactive");
				break;
			case AD_STATUSPOLL:
				sprintf(str, "%-20s", "idle when empty");
				break;
			}
			WriteString2(&m, str);
			WriteString2(&m, "\n");

			WriteString2(&m, "force disconnect");
			WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
			sprintf(str, "%-20s", "...");
			WriteString2(&m, str);
			WriteString2(&m, "\n");

			WriteString2(&m, "    take control");
			WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
			sprintf(str, "%-20s", "...");
			WriteString2(&m, str);
			WriteString2(&m, "\n");

			WriteString2(&m, "            back");
			WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
			sprintf(str, "%-20s", "...");
			WriteString2(&m, str);
			WriteString2(&m, "\n");

			if (viewer->menuop >= i)
				viewer->menuop = i - 1;

			WriteString2(&m, "\n");

			WriteString2(&m, "          status");
			WriteString2(&m, " : ");
			sprintf(str, "%-20s", viewer->server->status);
			WriteString2(&m, str);
			WriteString2(&m, "\n");

			break;
		}
		//fallthrough
	case MENU_SERVERS:	//connections list

		WriteString2(&m, "\n\nServers\n\n");

		if (!cluster->servers)
		{
			WriteString2(&m, "No active connections");
		}
		else
		{
			if (viewer->menuop < 0)
				viewer->menuop = 0;
			i = 0;
			min = viewer->menuop - 10;
			if (min < 0)
				min = 0;
			for (sv = cluster->servers; sv && i<min; sv = sv->next, i++)
			{//skip over the early connections.
			}
			min+=20;
			for (; sv && i < min; sv = sv->next, i++)
			{
				//Info_ValueForKey(sv->serverinfo, "hostname", str, sizeof(str));
				//if (sv->parsingconnectiondata || !sv->modellist[1].name[0])
				//	snprintf(str, sizeof(str), "%s", sv->server);
				snprintf(str, sizeof(str), "%s", *sv->map.hostname?sv->map.hostname:sv->server);

				if (i == viewer->menuop)
					for (s = (unsigned char *)str; *s; s++)
					{
						if ((unsigned)*s >= ' ')
							*s = 128 | (*s&~128);
					}
				WriteString2(&m, str);
				WriteString2(&m, "\n");
			}
		}
		break;

	case MENU_ADMIN:	//admin menu

		WriteString2(&m, "\n\nCluster Admin\n\n");

		if (viewer->menuop < 0)
			viewer->menuop = 0;
		i = 0;

		WriteString2(&m, " this connection");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", "...");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "            port");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20i", cluster->qwlistenportnum);
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "        hostname");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->hostname);
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "          master");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->master);
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "        password");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", "...");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "      add server");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", "...");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "        add demo");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", "...");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "           choke");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->chokeonnotupdated?"yes":"no");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "delay forwarding");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->lateforward?"yes":"no");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "         talking");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->notalking?"no":"yes");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "           nobsp");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->nobsp?"yes":"no");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "            back");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", "...");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		if (viewer->menuop >= i)
			viewer->menuop = i - 1;
		break;
	}


	WriteByte(&m, 0);
	SendBufferToViewer(viewer, m.data, m.cursize, true);
}