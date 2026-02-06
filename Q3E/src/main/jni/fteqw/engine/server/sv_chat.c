#include "quakedef.h"

/*
NPC chat:
The idea is that the server reads in a text file, calls qc functions as appropriate, and centerprints the result to the corresponding player.
All play is paused while the chat is in progress.
There are no guarentees that this is useful to anyone. If it does sound sorta what you want and would like a small change then just ask.

The options are specified with commas between. a number specifies a tag, anything else is assumed to be a statement/function,
which is carried out when the text is used (rather than when it is mearly an option).
statements can be functions, addition, multiplication, division, subtraction. Storing is also possible.
To store, you need to name a variable (which will be created when used). The variable must be a float.
Strings will work ONLY when specified without arithmatic, and ONLY in function calls.
You can call QC functions which accept entities on the condition that the entity accepted is the player entity that is being used for the menu.
If you want to keep track of the person you are chatting to, use an entity field for it.
To pass this entity, use the word 'ent', without quotes of any kind.
Do not use operators or calls within a function call. It's not that advanced. You can use operators on the condition that you use and name a temp.





Exmaple dialog script:
dialog/barry.dlg:

	#tag/number, condition (float(entity ent) cond;), text, options.
	#if options has a function, that is called. Multiple options/funcs can be done with commas. No options will terminate. If a reply has multiple options, a random (non function) is picked.
	#no distinction between character speech and player options.
	#char statements ignore condition.

	#the first entry point
	{0}{}{So that slip-gate does work then. Hmph. Maybe they just turned the power back on! So, what can I do you for?}{1, 2, 3}
	{1}{}{Whu? Who are you?! Where am I?!}{10}
	{2}{}{Who ever you are, now you die!}{20}
	{3}{}{Oh, err, nothing. I know the story line already. Sorry.}{4}
	{4}{}{KNOW IT ALL!}{}

	{10}{}{You are in a place that we dub the 'Shambles', a resource drained town. We havn't managed to rebuild our society since the Destroyer dude came through here, killing anyone before they could ask why. We even had anarcy! Boy, that was fun! Anyway, who are you?}{11, 12}
	{11}{}{I'm just some random guy who was walking a dog. Then I got trapped behind a rock fall and found this really old place all boarded up. Next I knew I was here! I think I killed my dog though...}{30}
	{12}{}{Why the hell do you care! I'm just goin' to kill you now!}{20}

	{20}{}{That's just not nice!}{BarryWar(), give_arrogance(ent, 100), give_evil(ent, 500)}

	{30}{}{Do you reckon that you're much good with explosives? It's just that our church yard is a little over-run with zombies. You know how it is when some-one knicks your runes! I'm just asking, cos we don't much like cremation around here. It's a waste of fertilliser!}{31, 32, 33}
	{31}{}{I rekon you could blast 'em into chunky kibbles better than I.}{40}
	{32}{has_grenadelauncher(ent)}{I'll go right now!}{}
	{33}{!has_grenadelauncher(ent)}{Well, I would, but seeing as I ain't got a grenade launcher, I can't}{50}

	{40}{}{Yeah, probably. Bye!}{}

	{50}{}{That could be a problem couldn't it. Ah well, here, have my spare Rocket Launcher. I'm meant to use it against any nasties that come through that gate, but we havn't had anyone except you for years!}{give_weapon_rocketlauncher(ent), give_cunning(ent 100)}

	#second entry point (use a tag parameter of 200)
	{200}{}{I'm sorry, I can't help you in any way.}{}




the qc code:
	void(string name, float tag, entity player) Chat = #214;

	void() SomeMonstersTouchFunction =
	{
		Chat("barry", 0, other);
	}
*/

#if defined(SVCHAT) && !defined(CLIENTONLY)

#define SENDDELAY 1

float SV_ChatFunc(const char *func);
void Chat_GetTag(const char *filename, float tag, char **text, char **condition, char **options)
{	
	char *file; char *s;
	file = COM_LoadTempFile(va("dialog/%s.dlg", filename), NULL);
	if (!file)
	{
		*text = "ERROR: File not found";
		*condition = "";
		*options = "";
		return;
	}
	for (;*file;file++)
	{
		if (*file == '\n')
		{
			if (file[1] == '{')
			{
				if(atof(file+2)==tag)
				{
					*condition = strchr(file+2, '{')+1;

					s = strchr(file, '|');
					if (s && s < *condition)
					{
						*text = strchr(s, '}');
						**text = '\0';

						if (!SV_ChatFunc(s+1))
						{
							**text='}';
							continue;
						}
					}

					s = strchr(*condition, '}');
					*s = '\0';

					*text = strchr(s+1, '{')+1;	
					s = strchr(*text, '}');
					*s = '\0';

					*options = strchr(s+1, '{')+1;
					s = strchr(*options, '}');
					*s = '\0';
					return;
				}
			}
		}		
	}
	*text = va("Chat Tag %f not found in file %s", tag, host_client->chat.filename);
	*condition = "";
	*options = "";
	return;
}

chatvar_t *SV_ChatFindVariable(char *name)
{
	chatvar_t *cvar;
	cvar = host_client->chat.vars;
	while (cvar)
	{
		if (!strcmp(cvar->varname, name))
			return cvar;

		cvar = cvar->next;
	}
	return NULL;
}

float SV_ChatGetValue(char *name)
{
	chatvar_t *cvar;	
	if ((*name >= '0' && *name <= '9') || *name == '-')
		return atof(name);
	if (!strcmp(name, "true"))
		return 1;
	if (!strcmp(name, "false"))
		return 0;
	cvar = SV_ChatFindVariable(name);
	if (cvar)
		return cvar->value;
	return 0;
}

float SV_ChatSetValue(char *name, float newval)
{
	chatvar_t *cvar;
	cvar = SV_ChatFindVariable(name);
	if (cvar)
	{
		cvar->value = newval;
		return newval;
	}
	cvar = Z_Malloc(sizeof(chatvar_t));
	strcpy(cvar->varname, name);
	cvar->value = newval;
	cvar->next = host_client->chat.vars;
	host_client->chat.vars = cvar;

	return newval;
}

float SV_ChatFunc(const char *func)	//parse a condition/function
{
	globalvars_t *pr_globals;
	float result;
	int noted = false;
	const char *s, *os;
	char namebuf[64];
	func_t f;
	int parm;
	while (*func <= ' ')
		func++;
	if (*func == '!')
	{
		noted = true;
		func++;
	}
	s = COM_ParseToken(func, NULL);
	if (*com_token == '(')
	{//open bracket
		//find correct close
		parm = 1;
		for (s = func+1; ; s++)
		{
			if (!*s)
				Sys_Error("No close bracket");
			if (*s == ')')
			{
				parm--;
				if (!parm)break;
			}
			if (*s == '(')
				parm++;
		}
		func = strchr(func, '(');
		s=COM_ParseToken(s+1, NULL);
		if (!strncmp(com_token, "&&", 2))
			result = SV_ChatFunc(func+1) && SV_ChatFunc(s);
		else if (!strncmp(com_token, "||", 2))
			result = SV_ChatFunc(func+1) || SV_ChatFunc(s);
		else
			result = SV_ChatFunc(func+1);
	}
	else
	{
	strcpy(namebuf, com_token);	//get first word	

	while (*s <= ' ')
		s++;
	
	if (*s == '(')	//if followed by brackets
	{
		s++;
		pr_globals = PR_globals(svprogfuncs, PR_CURRENT);
		parm = OFS_PARM0;
		while((s=COM_ParseToken(os = s, NULL)))
		{
			if (*com_token == ')')
				break;
			if (*com_token == ',')
				continue;
			if (com_tokentype == TTP_STRING)
				G_INT(parm) = PR_NewString(svprogfuncs, com_token);
			else if (!strcmp(com_token, "ent"))
				G_INT(parm) = EDICT_TO_PROG(svprogfuncs, host_client->chat.edict);
			else
				G_FLOAT(parm) = SV_ChatFunc(os);

			parm+=OFS_PARM1-OFS_PARM0;
		}

		f = PR_FindFunction(svprogfuncs, namebuf, PR_CURRENT);
		if (f)
		{		
			PR_ExecuteProgram(svprogfuncs, f);
		}
		else
			Con_Printf("Failed to find function %s\n", namebuf);

		pr_globals = PR_globals(svprogfuncs, PR_CURRENT);
		result = G_FLOAT(OFS_RETURN);
	}
	else
	{
		//comparision operators
		if (!strncmp(s, "==", 2))
			result = (SV_ChatGetValue(namebuf) == SV_ChatFunc(s+2));
		else if (!strncmp(s, ">=", 2))
			result = (SV_ChatGetValue(namebuf) >= SV_ChatFunc(s+2));
		else if (!strncmp(s, "<=", 2))
			result = (SV_ChatGetValue(namebuf) <= SV_ChatFunc(s+2));
		else if (!strncmp(s, "!=", 2))
			result = (SV_ChatGetValue(namebuf) != SV_ChatFunc(s+2));
		else if (!strncmp(s, ">", 1))
			result = (SV_ChatGetValue(namebuf) >= SV_ChatFunc(s+2));
		else if (!strncmp(s, "<", 1))
			result = (SV_ChatGetValue(namebuf) <= SV_ChatFunc(s+2));

		//asignment operators
		else if (!strncmp(s, "=", 1))
			result = (SV_ChatSetValue(namebuf, SV_ChatFunc(s+1)));
		else if (!strncmp(s, "+=", 2))
			result = (SV_ChatSetValue(namebuf, SV_ChatGetValue(namebuf)+SV_ChatFunc(s+2)));
		else if (!strncmp(s, "-=", 2))
			result = (SV_ChatSetValue(namebuf, SV_ChatGetValue(namebuf)-SV_ChatFunc(s+2)));
		else if (!strncmp(s, "*=", 2))
			result = (SV_ChatSetValue(namebuf, SV_ChatGetValue(namebuf)*SV_ChatFunc(s+2)));
		else if (!strncmp(s, "/=", 2))
			result = (SV_ChatSetValue(namebuf, SV_ChatGetValue(namebuf)/SV_ChatFunc(s+2)));
		else if (!strncmp(s, "|=", 2))
			result = (SV_ChatSetValue(namebuf, (int)SV_ChatGetValue(namebuf)|(int)SV_ChatFunc(s+2)));
		else if (!strncmp(s, "&=", 2))
			result = (SV_ChatSetValue(namebuf, (int)SV_ChatGetValue(namebuf)&(int)SV_ChatFunc(s+2)));

		//mathematical operators
		else if (!strncmp(s, "+", 1))
			result = ( SV_ChatGetValue(namebuf)+SV_ChatFunc(s+1));
		else if (!strncmp(s, "-", 1))
			result = (SV_ChatGetValue(namebuf)-SV_ChatFunc(s+1));
		else if (!strncmp(s, "*", 1))
			result = (SV_ChatGetValue(namebuf)*SV_ChatFunc(s+1));
		else if (!strncmp(s, "/", 1))
			result = (SV_ChatGetValue(namebuf)/SV_ChatFunc(s+1));
		else if (!strncmp(s, "|", 1))
			result = ((int)SV_ChatGetValue(namebuf)|(int)SV_ChatFunc(s+1));
		else if (!strncmp(s, "&", 1))
			result = ((int)SV_ChatGetValue(namebuf)&(int)SV_ChatFunc(s+1));

		//operatorless
		else
			result = SV_ChatGetValue(namebuf);
	}
	}

	if (noted)
		result = !result;

	return result;
}


void SV_SendChat(void)
{
	int l;
	int i;
	char *s, *s2, *text;

	l = 2 + strlen(host_client->chat.maintext)+2;
	for (i = 0; i < host_client->chat.options; i++)
		l += strlen(host_client->chat.option[i].text)+2;

	s = Z_Malloc(l+1);
	s2 = s;
	text = host_client->chat.maintext;
	while(*text)
	{
		*s2 = *text ^ 128;
		text++;
		s2++;
	}
	*s2++='\n';
	*s2++='\n';
	*s2='\0';
	for (i = 0; i < host_client->chat.options; i++)
	{
		strcat(s2, host_client->chat.option[i].text);
		strcat(s2, "\n\n");
	}


	ClientReliableWrite_Begin (host_client, svc_centerprint, 2 + l);
	ClientReliableWrite_String (host_client, s);

/*
	MSG_WriteByte(&sv.reliable_datagram, svc_chat);
	if (!*host_client->chat.maintext)	//no text. don't mix with ends
		MSG_WriteString(&sv.reliable_datagram, " ");
	else
		MSG_WriteString(&sv.reliable_datagram, host_client->chat.maintext);

	MSG_WriteByte(&sv.reliable_datagram, i);
	for (i = 0; i < host_client->chat.options; i++)
		MSG_WriteString(&sv.reliable_datagram, host_client->chat.option[i].text);
*/

	host_client->chat.time = sv.time+SENDDELAY;
}
void SV_EndChat(void)
{
	host_client->chat.active = false;
	host_client->chat.time=0;

	ClientReliableWrite_Begin (host_client, svc_centerprint, 2);
	ClientReliableWrite_String (host_client, "");
/*
	MSG_WriteByte(&sv.reliable_datagram, svc_chat);
	MSG_WriteString(&sv.reliable_datagram, "");
*/
}


void SV_Chat(const char *filename, float starttag, edict_t *edict)
{
	int i, tag;
	char optiontext[1024];
	char *text; char *condition; char *options;
	char *s, *s2;
	if (host_client->chat.active)
		return;
	host_client->chat.active = true;

	host_client->chat.edict = edict;
	strcpy(host_client->chat.filename, filename);
	Chat_GetTag(filename, starttag, &text, &condition, &options);
	strcpy(host_client->chat.maintext, text);

	strcpy(optiontext, options);


	s = optiontext;	
	for (i = 0; i < 6 && s; )
	{
		if (!*s)
			break;
		for (s2=s;;s2++)
		{
			if (*s2 == ',')
			{
				*s2='\0';
				s2++;
				break;	
			}
			if (*s2 == '\0')
			{
				s2=NULL;
				break;
			}
		}

		while (*s <= ' ')
			s++;

		if ((*s >= '0' && *s <= '9') || (*s == '-' && s[1] >= '0' && s[1] <= '9'))
		{
			tag = atof(s);
			Chat_GetTag(filename, tag, &text, &condition, &options);
			strcpy(host_client->chat.option[i].text, text);
			host_client->chat.option[i].tag = tag;			

			if (*condition)
			{
				if (SV_ChatFunc(condition))
					i++;
				/*
				if (*condition == '!'){noted=true;condition++;}else noted=false;
				f = PR_FindFunction(condition, PR_CURRENT);
				if (f)
				{
					SetGlobalEdict(sv.chat.edict, OFS_PARM0);
					PR_ExecuteProgram(f);
					pr_globals = PR_globals(PR_CURRENT);
					if (noted)
					{
						if (!G_FLOAT(OFS_RETURN))
							i++;
					}
					else 
					{
						if (G_FLOAT(OFS_RETURN))
							i++;
					}
				}
				else
				{
					Con_Printf("Function \"%s\" was not found (for chat condition). Assumed true.\n", condition);
					i++;
				}
				*/
			}
			else
				i++;
		}
		else
		{

			SV_ChatFunc(s);
			/*
			f = PR_FindFunction(s, PR_CURRENT);
			if (f)
			{
				SetGlobalEdict(sv.chat.edict, OFS_PARM0);
				PR_ExecuteProgram(f);
			}
			else
				Con_Printf("Function \"%s\" was not found (for chat)\n", s);
			*/
		}

		s = s2;
	}

	host_client->chat.options=i;

SV_SendChat();
}

void SV_RecieveChat(int sel)
{
	char *text;
	char *condition;
	char *options, *s, *s2;
	float tag;
	if (!host_client->chat.active)
		return;
	if (host_client->chat.options == 0)
	{
		SV_EndChat();
		return;
	}

	sel = host_client->chat.option[sel-1].tag;

	Chat_GetTag(host_client->chat.filename, sel, &text, &condition, &options);
	tag = -1;
	for (s = options;s; )
	{
		if (!*s)
			break;
		for (s2=s;;s2++)
		{
			if (*s2 == ',')
			{
				*s2='\0';
				s2++;
				break;	
			}
			if (*s2 == '\0')
			{
				s2=NULL;
				break;
			}
		}

		if ((*s >= '0' && *s <= '9') || *s <= ' ')
		{
			tag = atof(s);
		}
		else
		{
			SV_ChatFunc(s);
			/*
			f = PR_FindFunction(s, PR_CURRENT);
			if (f)
			{
//				*progfuncs->pr_trace = 2;
				SetGlobalEdict(sv.chat.edict, OFS_PARM0);
				PR_ExecuteProgram(f);
			}
			else
				Con_Printf("Function \"%s\" was not found (for chat)\n", s);
				*/
		}

		s = s2;
	}

	if (tag < 0)
	{
		SV_EndChat();
		return;
	}

	host_client->chat.active=false;
	SV_Chat(host_client->chat.filename, tag, host_client->chat.edict);
}

//new game
void SV_WipeChat(client_t *client)
{
	chatvar_t *var;
	chatvar_t *lastvar = NULL;
	for (var = client->chat.vars; var; var=var->next)
	{
		if (lastvar)
			Z_Free(lastvar);
		lastvar = var;
	}
	if (lastvar)
		Z_Free(lastvar);

	client->chat.active = false;
	client->chat.vars = NULL;
}

//end of level. Maybe not though.
char *SV_SaveChat(void)	//is malloc. Clear old before call.
{
	chatvar_t *var;
	char *buf, *start;
	int bufsize=0;
	char tbuf[64];
	buf = tbuf;
	for (var = host_client->chat.vars; var; var=var->next)
	{
		bufsize += strlen(var->varname);
		sprintf(buf, "=%f;", var->value);
		bufsize += strlen(buf);
	}
	bufsize++;
	start = buf = BZ_Malloc(bufsize);

	for (var = host_client->chat.vars; var; var=var->next)
	{
		strcpy(buf, var->varname);		
		buf += strlen(var->varname);		
		sprintf(buf, "=%f;", var->value);
		buf += strlen(buf);
	}
	*buf = 0;

	return start;
}

//beginning of new level.
void SV_LoadChat(char *buffer)
{
	char namebuf[64], value[64];
	char *start, *eq, *end;
	SV_WipeChat(host_client);
	if (!buffer)
		return;

	start = buffer;
	for(;;)
	{		
		eq = strchr(start, '=');
		if (!eq)
			break;
		end = strchr(eq, ';');
		if (!end)
			break;

		Q_strncpyz(namebuf, start, eq-start);
		Q_strncpyz(value, eq+1, end-(eq+1));
		SV_ChatSetValue(namebuf, atof(value));
		start = end + 1;
		while (*start <= ' ')
		{
			if (*start == '\0')
				break;
			start++;
		}
	}
}


int SV_ChatMove(int impulse)
{
	if (host_client->chat.active && impulse)
		SV_RecieveChat(impulse);
	return false;
}
void SV_ChatThink(client_t *client)
{
	if (client->chat.active && client->chat.time < sv.time)	//centerprint wears off. Resend every few secs.
	{
		host_client = client;
		SV_SendChat();
	}
}

#endif

