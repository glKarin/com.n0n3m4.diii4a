#include "../plugin.h"

#include "xml.h"

//fixme
void (*Con_TrySubPrint)(const char *conname, const char *message);

void XML_Destroy(xmltree_t *t);

const char *XML_GetParameter(xmltree_t *t, const char *paramname, const char *def)
{
	xmlparams_t *p;
	if (t)
	{
		for (p = t->params; p; p = p->next)
			if (!strcmp(p->name, paramname))
				return p->val;
	}
	return def;
}
void XML_AddParameter(xmltree_t *t, const char *paramname, const char *value)
{
	xmlparams_t *p = malloc(sizeof(xmlparams_t));
	Q_strlcpy(p->name, paramname, sizeof(p->name));
	Q_strlcpy(p->val, value?value:"", sizeof(p->val));

	if (t->params)	//reverse insert
	{
		xmlparams_t *prev;
		for(prev = t->params; prev->next; prev = prev->next)
			;
		prev->next = p;
		p->next = NULL;
	}
	else
	{
		p->next = t->params;
		t->params = p;
	}
}
void XML_AddParameteri(xmltree_t *t, const char *paramname, int value)
{
	char svalue[64];
	Q_snprintf(svalue, sizeof(svalue), "%i", value);
	XML_AddParameter(t, paramname, svalue);
}
xmltree_t *XML_CreateNode(xmltree_t *parent, const char *name, const char *xmlns, const char *body)
{
	int bodylen;
	struct subtree_s *node = malloc(sizeof(*node));

	if (!body)
		body = "";
	if (!xmlns)
		xmlns = "";

	bodylen = strlen(body);

	//clear out links
	node->params = NULL;
	node->child = NULL;
	node->sibling = NULL;
	//link into parent if we actually have a parent.
	if (parent)
	{
		if (parent->child)
		{	//add at tail
			xmltree_t *prev;
			for(prev = parent->child; prev->sibling; prev = prev->sibling)
				;
			prev->sibling = node;
			node->sibling = NULL;
		}
		else
		{
			node->sibling = parent->child;
			parent->child = node;
		}
	}

	Q_strlcpy(node->name, name, sizeof(node->name));
	Q_strlcpy(node->xmlns, xmlns, sizeof(node->xmlns));
	Q_strlcpy(node->xmlns_dflt, xmlns, sizeof(node->xmlns_dflt));
	node->body = malloc(bodylen+1);
	memcpy(node->body, body, bodylen+1);

	if (*xmlns)
		XML_AddParameter(node, "xmlns", xmlns);

	return node;
}

const struct
{
	int codelen;
	char *code;
	int namelen;
	char *name;
} xmlchars[] =
{
	{1, "<",		2, "lt"},
	{1, ">",		2, "gt"},
	{1, "&",		3, "amp"},
	{1, "\'",		4, "apos"},
	{1, "\"",		4, "quot"},
	{2, "\xC3\x96",	4, "ouml"},
	{0, NULL,		0, NULL}
};
//converts < to &lt; etc.
//returns the end of d.
char *XML_Markup(const char *s, char *d, int dlen)
{
	int i;
	dlen--;
	while(*s)
	{
		for(i = 0; xmlchars[i].code; i++)
		{
			if (!strncmp(s, xmlchars[i].code, xmlchars[i].codelen))
				break;
		}
		if (xmlchars[i].code)
		{
			if (dlen < xmlchars[i].namelen+2)
				break;
			*d++ = '&';
			memcpy(d, xmlchars[i].name, xmlchars[i].namelen);
			d+=xmlchars[i].namelen;
			*d++ = ';';
			s+=xmlchars[i].codelen;
			dlen -= xmlchars[i].namelen+2;
		}
		else
		{
			if (!dlen)
				break;
			dlen--;
			*d++ = *s++;
		}
	}
	*d = 0;
	return d;
}
//inplace. result will always be same length or shorter.
//converts &lt; etc to their original chars
void XML_Unmark(char *s)
{
	char *d;
	int i;

	for (d = s; *s; )
	{
		if (*s == '&')
		{
			s++;
			for (i = 0; xmlchars[i].name; i++)
			{
				if (!strncmp(s, xmlchars[i].name, xmlchars[i].namelen) && s[xmlchars[i].namelen] == ';')
					break;
			}
			if (xmlchars[i].name)
			{
				s += xmlchars[i].namelen+1;

				memcpy(d, xmlchars[i].code, xmlchars[i].codelen);
				d+=xmlchars[i].codelen;
			}
			else
			{
				*d++ = '&';
			}
		}
		else
			*d++ = *s++;
	}
	*d = 0;
}

struct buf_ctx
{
	char *buf;
	int len;
	int maxlen;
};
static void buf_cat(struct buf_ctx *buf, char *data, int datalen)
{
	int newlen = buf->len + datalen+1;
	if (newlen > buf->maxlen)
	{
		char *newd;
		newlen *= 2;
		newd = malloc(newlen);
		memcpy(newd, buf->buf, buf->len);
		free(buf->buf);
		buf->buf = newd;
		buf->maxlen = newlen;
	}

	memcpy(buf->buf + buf->len, data, datalen);
	buf->len += datalen;
}
static void XML_DumpToBuf(struct buf_ctx *buf, xmltree_t *t, int indent)
{
	xmltree_t *c;
	xmlparams_t *p;
	int i;
	for (i = 0; i < indent; i++)
		buf_cat(buf, " ", 1);

	buf_cat(buf, "<", 1);
	buf_cat(buf, t->name, strlen(t->name));

	for (p = t->params; p; p = p->next)
	{
		buf_cat(buf, " ", 1);
		buf_cat(buf, p->name, strlen(p->name));
		buf_cat(buf, "=\'", 2);
		buf_cat(buf, p->val, strlen(p->val));
		buf_cat(buf, "\'", 1);
	}

	if (t->child)
	{
		buf_cat(buf, ">", 1);
		if (indent>=0)
			buf_cat(buf, "\n", 1);
		for (c = t->child; c; c = c->sibling)
			XML_DumpToBuf(buf, c, ((indent<0)?indent:(indent+2)));
		for (i = 0; i < indent; i++)
			buf_cat(buf, " ", 1);
		buf_cat(buf, "</", 2);
		buf_cat(buf, t->name, strlen(t->name));
		buf_cat(buf, ">", 1);
	}
	else if (*t->body)
	{
		buf_cat(buf, ">", 1);
		buf_cat(buf, t->body, strlen(t->body));
		buf_cat(buf, "</", 2);
		buf_cat(buf, t->name, strlen(t->name));
		buf_cat(buf, ">", 1);
	}
	else
	{
		buf_cat(buf, "/>", 2);
	}
	if (indent>=0)
		buf_cat(buf, "\n", 1);
}

char *XML_GenerateString(xmltree_t *root, qboolean readable)
{
	struct buf_ctx buf = {NULL, 0, 0};
	XML_DumpToBuf(&buf, root, readable?0:-1);
	buf_cat(&buf, "", 1);
	return buf.buf;
}
xmltree_t *XML_Parse(const char *buffer, int *startpos, int maxpos, qboolean headeronly, const char *defaultnamespace)
{
	xmlparams_t *p;
	xmltree_t *child;
	xmltree_t **childlink;	//to add at end, retaining order
	xmltree_t *ret;
	int bodypos;
	int bodymax = 0;
	int pos, i;
	char *tagend;
	const char *tagstart;
	const char *ns;
	char token[1024];
	pos = *startpos;
	while (buffer[pos] >= '\0' && buffer[pos] <= ' ')
	{
		if (pos >= maxpos)
			break;
		pos++;
	}

	if (pos == maxpos)
	{
		*startpos = pos;
		return NULL;	//nothing anyway.
	}

skippedcomment:

	//expect a <

	if (buffer[pos] != '<')
	{
		Con_Printf("Missing open bracket\n");
		return NULL;	//should never happen
	}

	if (buffer[pos+1] == '/')
	{
		Con_Printf("Unexpected close tag.\n");
		return NULL;	//err, terminating a parent tag
	}

	if (pos+4 < maxpos && buffer[pos+1] == '!' && buffer[pos+2] == '-' && buffer[pos+3] == '-')
	{
		//this looks like a comment. scan forwards until we find the -->
		pos += 4;
		while (pos+3 < maxpos)
		{
			if (buffer[pos+0] == '-' && buffer[pos+1] == '-' && buffer[pos+2] == '>')
			{
				pos+=3;
				goto skippedcomment;
			}
			pos++;
		}
		Con_Printf("Missing comment end\n");
		return NULL;	//should never happen
	}

	tagend = strchr(buffer+pos, '>');
	if (!tagend)
	{
		Con_Printf("Missing close bracket\n");
		return NULL;	//should never happen
	}
	*tagend = '\0';
	tagend++;

	//assume no nulls in the tag header.
	tagstart = buffer+pos+1;
	while (*tagstart == ' ' || *tagstart == '\n' || *tagstart == '\r' || *tagstart == '\t')
		tagstart++;
	for (i = 0; i < sizeof(token)-1 && *tagstart; )
	{
		if (*tagstart == ' ' || (i&&*tagstart == '/') || (i&&*tagstart == '?') || *tagstart == '\n' || *tagstart == '\r' || *tagstart == '\t')
			break;
		token[i++] = *tagstart++;
	}
	token[i] = 0;

	pos = tagend - buffer;

	ret = malloc(sizeof(xmltree_t));
	memset(ret, 0, sizeof(*ret));

	ns = strchr(token, ':');
	if (ns)
	{
		memcpy(ret->xmlns, "xmlns:", 6);
		Q_strlncpy(ret->xmlns+6, token, sizeof(ret->xmlns)-6, ns-token);
		ns++;
		Q_strlcpy(ret->name, ns, sizeof(ret->name));
	}
	else
	{
		Q_strlcpy(ret->xmlns, "xmlns", sizeof(ret->xmlns));
		Q_strlcpy(ret->name, token, sizeof(ret->name));
	}

	while(*tagstart)
	{
		int nlen;


		while(*(unsigned char*)tagstart <= ' ' && *tagstart)
			tagstart++;	//skip whitespace (note that we know there is a null terminator before the end of the buffer)

		if (!*tagstart)
			break;

		p = malloc(sizeof(xmlparams_t));
		nlen = 0;
		while (nlen < sizeof(p->name)-2)
		{
			if (*(unsigned char*)tagstart <= ' ' || *tagstart == '/' || *tagstart == '?')
				break;

			if (*tagstart == '=')
				break;
			p->name[nlen++] = *tagstart++;
		}
		p->name[nlen++] = '\0';

		while(*(unsigned char*)tagstart <= ' ' && *tagstart)
			tagstart++;	//skip whitespace (note that we know there is a null terminator before the end of the buffer)

		if (*tagstart != '=')
			break;
		tagstart++;

		while(*(unsigned char*)tagstart <= ' ' && *tagstart)
			tagstart++;	//skip whitespace (note that we know there is a null terminator before the end of the buffer)

		nlen = 0;
		if (*tagstart == '\'')
		{
			tagstart++;
			while (*tagstart && nlen < sizeof(p->name)-2)
			{
				if(*tagstart == '\'')
					break;

				p->val[nlen++] = *tagstart++;
			}
			tagstart++;
			p->val[nlen++] = '\0';
		}
		else if (*tagstart == '\"')
		{
			tagstart++;
			while (*tagstart && nlen < sizeof(p->name)-2)
			{
				if(*tagstart == '\"')
					break;

				p->val[nlen++] = *tagstart++;
			}
			tagstart++;
			p->val[nlen++] = '\0';
		}
		else
		{
			while (*tagstart && nlen < sizeof(p->name)-2)
			{
				if(*tagstart <= ' ')
					break;

				p->val[nlen++] = *tagstart++;
			}
			p->val[nlen++] = '\0';
		}
		XML_Unmark(p->val);
		p->next = ret->params;
		ret->params = p;
	}

	ns = XML_GetParameter(ret, ret->xmlns, "");
	Q_strlcpy(ret->xmlns, ns, sizeof(ret->xmlns));

	ns = XML_GetParameter(ret, "xmlns", NULL);
	Q_strlcpy(ret->xmlns_dflt, ns?ns:defaultnamespace, sizeof(ret->xmlns_dflt));

	tagend[-1] = '>';

	if (tagend[-2] == '/')
	{	//no body
		ret->body = malloc(1);
		*ret->body = 0;
		*startpos = pos;
		return ret;
	}
	if (ret->name[0] == '?')
	{
		//no body either
		if (tagend[-2] == '?')
		{
			ret->body = malloc(1);
			*ret->body = 0;
			*startpos = pos;
			return ret;
		}
	}

	if (headeronly)
	{
		*startpos = pos;
		return ret;
	}

	//does it have a body, or is it child tags?

	childlink = &ret->child;
	bodypos = 0;
	while(1)
	{
		if (pos == maxpos)
		{	//malformed
			Con_Printf("tree is malfored\n");
			XML_Destroy(ret);
			return NULL;
		}

		if (buffer[pos] == '<')
		{
			if (buffer[pos+1] == '/')
			{	//the end of this block
				//FIXME: check name

				tagend = strchr(buffer+pos, '>');
				if (!tagend)
				{
					Con_Printf("No close tag\n");
					XML_Destroy(ret);
					return NULL;	//should never happen
				}
				tagend++;
				pos = tagend - buffer;
				break;
			}

			if (!strncmp(&buffer[pos], "<![CDATA[", 9))
			{
				pos += 9;
				while(1)
				{
					char c;
					if (pos == maxpos)
					{	//malformed
						Con_Printf("CDATA is malfored (inside %s)\n", ret->name);
						XML_Destroy(ret);
						return NULL;
					}

					if (buffer[pos+0] == ']' && buffer[pos+1] == ']' && buffer[pos+2] == '>')
					{
						pos += 3;
						break;
					}

					c = buffer[pos++];
					if (bodypos == bodymax)
					{
						int nlen = bodypos*2 + 64;
						char *nb = malloc(nlen);
						memcpy(nb, ret->body, bodypos);
						free(ret->body);
						ret->body = nb;
						bodymax = nlen;
					}
					ret->body[bodypos++] = c;
				}
			}
			else
			{
				child = XML_Parse(buffer, &pos, maxpos, false, ret->xmlns_dflt);
				if (!child)
				{
					Con_Printf("Child block is unparsable (within %s)\n", ret->name);
					XML_Destroy(ret);
					return NULL;
				}

				child->sibling = *childlink;
				*childlink = child;
				childlink = &child->sibling;
			}
		}
		else 
		{
			char c = buffer[pos++];
			if (bodypos == bodymax)
			{
				int nlen = bodypos*2 + 64;
				char *nb = malloc(nlen);
				memcpy(nb, ret->body, bodypos);
				free(ret->body);
				ret->body = nb;
				bodymax = nlen;
			}
			ret->body[bodypos++] = c;
		}
	}
	if (bodypos == bodymax)
	{
		int nlen = bodypos+1;
		char *nb = malloc(nlen);
		memcpy(nb, ret->body, bodypos);
		free(ret->body);
		ret->body = nb;
		bodymax = nlen;
	}
	ret->body[bodypos++] = '\0';

	XML_Unmark(ret->body);

	*startpos = pos;

	return ret;
}

void XML_Destroy(xmltree_t *t)
{
	xmlparams_t *p, *np;

	if (t->child)
		XML_Destroy(t->child);
	if (t->sibling)
		XML_Destroy(t->sibling);

	for (p = t->params; p; p = np)
	{
		np = p->next;
		free(p);
	}
	free(t->body);
	free(t);
}

xmltree_t *XML_ChildOfTree(xmltree_t *t, const char *name, int childnum)
{
	if (t)
	{
		for (t = t->child; t; t = t->sibling)
		{
			if (!strcmp(t->name, name))
			{
				if (childnum-- == 0)
					return t;
			}
		}
	}
	return NULL;
}
xmltree_t *XML_ChildOfTreeNS(xmltree_t *t, const char *xmlns, const char *name, int childnum)
{
	if (t)
	{
		for (t = t->child; t; t = t->sibling)
		{
			if (!strcmp(t->xmlns, xmlns) && !strcmp(t->name, name))
			{
				if (childnum-- == 0)
					return t;
			}
		}
	}
	return NULL;
}
const char *XML_GetChildBody(xmltree_t *t, const char *paramname, const char *def)
{
	xmltree_t *c = XML_ChildOfTree(t, paramname, 0);
	if (c)
		return c->body;
	return def;
}

void XML_ConPrintTree(xmltree_t *t, const char *subconsole, int indent)
{
	int start, c, chunk;
	struct buf_ctx buf = {NULL, 0, 0};
	if (!t)
		return;
	XML_DumpToBuf(&buf, t, indent);
	buf_cat(&buf, "", 1);

	for (start = 0; start < buf.len; )
	{
		chunk = buf.len - start;
		if (chunk > 128)
			chunk = 128;
		c = buf.buf[start+chunk];
		buf.buf[start+chunk] = 0;
		Con_TrySubPrint(subconsole, buf.buf+start);
		buf.buf[start+chunk] = c;

		start += chunk;
	}

	free(buf.buf);
}


static void XML_SkipWhite(const char *msg, int *pos, int max)
{
	while (*pos < max && (
		msg[*pos] == ' ' ||
		msg[*pos] == '\t' ||
		msg[*pos] == '\r' ||
		msg[*pos] == '\n'
		))
		*pos+=1;
}
static qboolean XML_ParseString(const char *msg, int *pos, int max, char *out, int outlen)
{
	*out = 0;
	if (*pos < max && msg[*pos] == '\"')
	{
		*pos+=1;

		outlen--;
		while (*pos < max && msg[*pos] != '\"')
		{
			if (!outlen)
				return false;
			*out = msg[*pos];
			out++;
			outlen--;
			*pos+=1;
		}
		if (*pos < max && msg[*pos] == '\"')
		{
			*out = 0;
			*pos+=1;
			return true;
		}
	}
	else
	{
		outlen--;
		while (*pos < max
			&& msg[*pos] != ' '
			&& msg[*pos] != '\t'
			&& msg[*pos] != '\r'
			&& msg[*pos] != '\n'
			&& msg[*pos] != ':'
			&& msg[*pos] != ','
			&& msg[*pos] != '}'
			&& msg[*pos] != '{')
		{
			if (!outlen)
				return false;
			*out = msg[*pos];
			out++;
			outlen--;
			*pos+=1;
		}
		*out = 0;
		return true;
	}
	return false;
}
xmltree_t *XML_FromJSON(xmltree_t *t, const char *name, const char *json, int *jsonpos, int jsonlen)
{
	char child[4096];
	XML_SkipWhite(json, jsonpos, jsonlen);

	if (*jsonpos < jsonlen && json[*jsonpos] == '{')
	{
		*jsonpos+=1;
		XML_SkipWhite(json, jsonpos, jsonlen);

		t = XML_CreateNode(t, name, "", "");

		while (*jsonpos < jsonlen && json[*jsonpos] == '\"')
		{
			if (!XML_ParseString(json, jsonpos, jsonlen, child, sizeof(child)))
				break;
			XML_SkipWhite(json, jsonpos, jsonlen);
			if (*jsonpos < jsonlen && json[*jsonpos] == ':')
			{
				*jsonpos+=1;
				if (!XML_FromJSON(t, child, json, jsonpos, jsonlen))
					break;
			}
			XML_SkipWhite(json, jsonpos, jsonlen);

			if (*jsonpos < jsonlen && json[*jsonpos] == ',')
			{
				*jsonpos+=1;
				XML_SkipWhite(json, jsonpos, jsonlen);
				continue;
			}
			break;
		}

		if (*jsonpos < jsonlen && json[*jsonpos] == '}')
		{
			*jsonpos+=1;
			return t;
		}
		XML_Destroy(t);
	}
	else if (*jsonpos < jsonlen)
	{
		if (XML_ParseString(json, jsonpos, jsonlen, child, sizeof(child)))
			return XML_CreateNode(t, name, "", child);
	}
	return NULL;
}
