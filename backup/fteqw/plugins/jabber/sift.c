#include "xmpp.h"

#ifdef FILETRANSFERS

void XMPP_FT_Frame(jclient_t *jcl)
{
	static char *hex = "0123456789abcdef";
	char digest[20];
	char domain[sizeof(digest)*2+1];
	int j;
	char *req;
	struct ft_s *ft;
	for (ft = jcl->ft; ft; ft = ft->next)
	{
		if (ft->streamstatus == STRM_IDLE)
			continue;

		if (ft->stream < 0)
		{
			if (ft->nexthost > 0)
			{
				ft->nexthost--;
				ft->stream = netfuncs->TCPConnect(ft->streamhosts[ft->nexthost].host, ft->streamhosts[ft->nexthost].port);
				if (ft->stream == -1)
					continue;

				ft->streamstatus = STRM_AUTH;

				//'authenticate' with socks5 proxy. tell it that we only support 'authless'.
				netfuncs->Send(ft->stream, "\x05\x01\x00", 3);
			}
			else
			{
				JCL_AddClientMessagef(jcl,
					"<iq id='%s' to='%s' type='error'>"
						"<error type='cancel'>"
							"<item-not-found xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
						"</error>"
					"</iq>", ft->iqid, ft->with);
				ft->streamstatus = STRM_IDLE;
			}
			continue;
		}
		else
		{
			char data[8192];
			int len;
			if (ft->streamstatus == STRM_ACTIVE)
			{
				len = netfuncs->Recv(ft->stream, data, sizeof(data)-1);
				if (len > 0)
					filefuncs->Write(ft->file, data, len);
			}
			else
			{
				len = netfuncs->Recv(ft->stream, data, sizeof(data)-1);
				if (len > 0)
				{
					if (ft->streamstatus == STRM_AUTH)
					{
						if (len == 2 && data[0] == 0x05 && data[1] == 0x00)
						{
							//server has accepted us, woo.
							//sid+requester(them)+target(us)
							req = va("%s%s%s", ft->sid, ft->with, jcl->fulljid);
							CalcHash(&hash_sha1, digest, sizeof(digest), req, strlen(req));
							//in hex
							for (req = domain, j=0; j < 20; j++)
							{
								*req++ = hex[(digest[j]>>4) & 0xf];
								*req++ = hex[(digest[j]>>0) & 0xf];
							}
							*req = 0;

							//connect with hostname(3).
							req = va("\x05\x01%c\x03""%c%s%c%c", 0, (int)strlen(domain), domain, 0, 0);
							netfuncs->Send(ft->stream, req, strlen(domain)+7);
							ft->streamstatus = STRM_AUTHED;
						}
						else
							len = -1;
					}
					else if (ft->streamstatus == STRM_AUTHED)
					{
						if (data[0] == 0x05 && data[1] == 0x00)
						{
							if (filefuncs->Open(ft->fname, &ft->file, 2) < 0)
							{
								len = -1;
								JCL_AddClientMessagef(jcl, "<iq id='%s' to='%s' type='error'/>", ft->iqid, ft->with);
							}
							else
							{
								ft->streamstatus = STRM_ACTIVE;
								JCL_AddClientMessagef(jcl,
										"<iq id='%s' to='%s' type='result'>"
											"<query xmlns='http://jabber.org/protocol/bytestreams' sid='%s'>"
												"<streamhost-used jid='%s'/>"
											"</query>"
										"</iq>", ft->iqid, ft->with, ft->sid, ft->streamhosts[ft->nexthost].jid);
							}
						}
						else
							len = -1;	//err, this is someone else...
					}
				}
			}

			if (len == -1)
			{
				netfuncs->Close(ft->stream);
				ft->stream = -1;
				if (ft->streamstatus == STRM_ACTIVE)
				{
					int size;
					if (ft->file != -1)
						filefuncs->Close(ft->file);
					ft->file = -1;

					size = filefuncs->Open(ft->fname, &ft->file, 1);
					if (ft->file != -1)
						filefuncs->Close(ft->file);
					if (size == ft->size)
					{
						Con_Printf("File Transfer Completed\n");
						continue;
					}
					else
					{
						Con_Printf("File Transfer Aborted by peer\n");
						continue;
					}
				}
				Con_Printf("File Transfer connection to %s:%i failed\n", ft->streamhosts[ft->nexthost].host, ft->streamhosts[ft->nexthost].port);
			}
		}
	}
}

void XMPP_FT_AcceptFile(jclient_t *jcl, int fileid, qboolean accept)
{
	struct ft_s *ft, **link;
	char *s;
	xmltree_t *repiq, *repsi, *c;

	for (link = &jcl->ft; (ft=*link); link = &(*link)->next)
	{
		if (ft->privateid == fileid)
			break;
	}
	if (!ft)
	{
		Con_Printf("File not known\n");
		return;
	}

	if (!accept)
	{
		Con_Printf("Declining file \"%s\" from \"%s\" (%i bytes)\n", ft->fname, ft->with, ft->size);

		if (!ft->allowed)
		{
			JCL_AddClientMessagef(jcl,
						"<iq type='error' to='%s' id='%s'>"
							"<error type='cancel'>"
							"<forbidden xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
								"<text>Offer Declined</text>"
							"</error>"
						"</iq>", ft->with, ft->iqid);
		}
		else
		{
			//FIXME: send a proper cancel
		}

		if (ft->file != -1)
			filefuncs->Close(ft->file);
		*link = ft->next;
		free(ft);
	}
	else
	{
		Con_Printf("Receiving file \"%s\" from \"%s\" (%i bytes)\n", ft->fname, ft->with, ft->size);
		ft->allowed = true;

		//generate a response.
		//FIXME: we ought to delay response until after we prompt.
		repiq = XML_CreateNode(NULL, "iq", "", "");
		XML_AddParameter(repiq, "type", "result");
		XML_AddParameter(repiq, "to", ft->with);
		XML_AddParameter(repiq, "id", ft->iqid);
		repsi = XML_CreateNode(repiq, "si", "http://jabber.org/protocol/si", "");
		XML_CreateNode(repsi, "file", "http://jabber.org/protocol/si/profile/file-transfer", "");	//I don't really get the point of this.
		c = XML_CreateNode(repsi, "feature", "http://jabber.org/protocol/feature-neg", "");
		c = XML_CreateNode(c, "x", "jabber:x:data", "");
		XML_AddParameter(c, "type", "submit");
		c = XML_CreateNode(c, "field", "", "");
		XML_AddParameter(c, "var", "stream-method");
		if (ft->method == FT_IBB)
			c = XML_CreateNode(c, "value", "", "http://jabber.org/protocol/ibb");
		else if (ft->method == FT_BYTESTREAM)
			c = XML_CreateNode(c, "value", "", "http://jabber.org/protocol/bytestreams");

		s = XML_GenerateString(repiq, false);
		JCL_AddClientMessageString(jcl, s);
		free(s);
		XML_Destroy(repiq);
	}
}

static qboolean XMPP_FT_IBBChunked(jclient_t *jcl, xmltree_t *x, struct iq_s *iq)
{
	const char *from = XML_GetParameter(x, "from", jcl->domain);
	struct ft_s *ft = iq->usrptr, **link, *v;
	for (link = &jcl->ft; (v=*link); link = &(*link)->next)
	{
		if (v == ft && !strcmp(ft->with, from))
		{
			//its still valid
			if (x)
			{
				char *base64;
				char rawbuf[4096];
				int sz;
				sz = filefuncs->Read(ft->file, rawbuf, ft->blocksize);
				Base64_Add(rawbuf, sz);
				base64 = Base64_Finish();

				if (sz > 0)
				{
					ft->sizedone += sz;
					if (ft->sizedone == ft->size)
						ft->eof = true;
				}

				if (sz && strlen(base64))
				{
					x = XML_CreateNode(NULL, "data", "http://jabber.org/protocol/ibb", base64);
					XML_AddParameteri(x, "seq", ft->seq++);
					XML_AddParameter(x, "sid", ft->sid);
					JCL_SendIQNode(jcl, XMPP_FT_IBBChunked, "set", from, x, true)->usrptr = ft;
					return true;
				}
				//else eof
			}

			//errored or ended

			if (x)
				Con_Printf("Transfer of %s to %s completed\n", ft->fname, ft->with);
			else
				Con_Printf("%s aborted %s\n", ft->with, ft->fname);
			x = XML_CreateNode(NULL, "close", "http://jabber.org/protocol/ibb", "");
			XML_AddParameter(x, "sid", ft->sid);
			JCL_SendIQNode(jcl, NULL, "set", from, x, true)->usrptr = ft;

			//errored
			if (ft->file != -1)
				filefuncs->Close(ft->file);
			*link = ft->next;
			free(ft);
			return true;
		}
	}
	return true;	//the ack can come after the bytestream has already finished sending. don't warn about that.
}
static qboolean XMPP_FT_IBBBegun(jclient_t *jcl, xmltree_t *x, struct iq_s *iq)
{
	struct ft_s *ft = iq->usrptr, **link, *v;
	const char *from = XML_GetParameter(x, "from", jcl->domain);
	for (link = &jcl->ft; (v=*link); link = &(*link)->next)
	{
		if (v == ft && !strcmp(ft->with, from))
		{
			//its still valid
			if (!x)
			{
				Con_Printf("%s aborted %s\n", ft->with, ft->fname);
				//errored
				if (ft->file != -1)
					filefuncs->Close(ft->file);
				*link = ft->next;
				free(ft);
			}
			else
			{
				ft->begun = true;
				ft->method = FT_IBB;
				XMPP_FT_IBBChunked(jcl, x, iq);
			}
			return true;
		}
	}
	return false;
}
qboolean XMPP_FT_OfferAcked(jclient_t *jcl, xmltree_t *x, struct iq_s *iq)
{
	struct ft_s *ft = iq->usrptr, **link, *v;
	const char *from = XML_GetParameter(x, "from", jcl->domain);
	for (link = &jcl->ft; (v=*link); link = &(*link)->next)
	{
		if (v == ft && !strcmp(ft->with, from))
		{
			//its still valid
			if (!x)
			{
				Con_Printf("%s doesn't want %s\n", ft->with, ft->fname);
				//errored
				if (ft->file != -1)
					filefuncs->Close(ft->file);
				*link = ft->next;
				free(ft);
			}
			else
			{
				xmltree_t *xo;
				Con_Printf("%s accepted %s\n", ft->with, ft->fname);
				xo = XML_CreateNode(NULL, "open", "http://jabber.org/protocol/ibb", "");
				XML_AddParameter(xo, "sid", ft->sid);
				XML_AddParameteri(xo, "block-size", ft->blocksize);
				//XML_AddParameter(xo, "stanza", "iq");

				JCL_SendIQNode(jcl, XMPP_FT_IBBBegun, "set", XML_GetParameter(x, "from", jcl->domain), xo, true)->usrptr = ft;
			}
			return true;
		}
	}
	return false;
}

void XMPP_FT_SendFile(jclient_t *jcl, const char *console, const char *to, const char *fname)
{
	xmltree_t *xsi, *xfile, *c;
	struct ft_s *ft;

	Con_SubPrintf(console, "Offering %s to %s.\n", fname, to);

	ft = malloc(sizeof(*ft));
	memset(ft, 0, sizeof(*ft));
	ft->stream = -1;
	ft->allowed = true;
	ft->transmitting = true;
	ft->blocksize = 4096;
	Q_strlcpy(ft->fname, fname, sizeof(ft->fname));
	if (Q_snprintfz(ft->sid, sizeof(ft->sid), "%x%s", rand(), ft->fname))
		/*doesn't matter so long as its unique*/;
	Q_strlcpy(ft->md5hash, "", sizeof(ft->md5hash));
	ft->size = filefuncs->Open(ft->fname, &ft->file, 1);
	ft->with = strdup(to);
	ft->method = FT_IBB;
	ft->begun = false;

	ft->next = jcl->ft;
	jcl->ft = ft;

	//generate an offer.
	xsi = XML_CreateNode(NULL, "si", "http://jabber.org/protocol/si", "");
		XML_AddParameter(xsi, "profile", "http://jabber.org/protocol/si/profile/file-transfer");
		XML_AddParameter(xsi, "id", ft->sid);
	//XML_AddParameter(xsi, "mime-type", "text/plain");
	xfile = XML_CreateNode(xsi, "file", "http://jabber.org/protocol/si/profile/file-transfer", "");	//I don't really get the point of this.
		XML_AddParameter(xfile, "name", ft->fname);
		XML_AddParameteri(xfile, "size", ft->size);
	c = XML_CreateNode(xsi, "feature", "http://jabber.org/protocol/feature-neg", "");
		c = XML_CreateNode(c, "x", "jabber:x:data", "");
			XML_AddParameter(c, "type", "form");
			c = XML_CreateNode(c, "field", "", "");
				XML_AddParameter(c, "var", "stream-method");
				XML_AddParameter(c, "type", "listsingle");
					XML_CreateNode(XML_CreateNode(c, "option", "", ""), "value", "", "http://jabber.org/protocol/ibb");
//					XML_CreateNode(XML_CreateNode(c, "option", "", ""), "value", "", "http://jabber.org/protocol/bytestreams");

	JCL_SendIQNode(jcl, XMPP_FT_OfferAcked, "set", to, xsi, true)->usrptr = ft;
}

qboolean XMPP_FT_ParseIQSet(jclient_t *jcl, const char *iqfrom, const char *iqid, xmltree_t *tree)
{
	xmltree_t *ot;

	//if file transfers are not enabled in this build, reject all iqs
	if (!(jcl->enabledcapabilities & CAP_SIFT))
		return false;

	ot = XML_ChildOfTreeNS(tree, "http://jabber.org/protocol/bytestreams", "query", 0);
	if (ot)
	{
		xmltree_t *c;
		struct ft_s *ft;
		const char *sid = XML_GetParameter(ot, "sid", "");
		for (ft = jcl->ft; ft; ft = ft->next)
		{
			if (!strcmp(ft->sid, sid) && !strcmp(ft->with, iqfrom))
			{
				if (ft->allowed && !ft->begun && ft->transmitting == false)
				{
					const char *jid;
					const char *host;
					int port;
					const char *mode = XML_GetParameter(ot, "mode", "tcp");
					int i;
					if (strcmp(mode, "tcp"))
						break;
					for (i = 0; ; i++)
					{
						c = XML_ChildOfTree(ot, "streamhost", i);
						if (!c)
							break;

						jid = XML_GetParameter(c, "jid", "");
						host = XML_GetParameter(c, "host", "");
						port = atoi(XML_GetParameter(c, "port", "0"));
						if (port <= 0 || port > 65535)
							continue;
						if (ft->nexthost < sizeof(ft->streamhosts) / sizeof(ft->streamhosts[ft->nexthost]))
						{
							Q_strlcpy(ft->streamhosts[ft->nexthost].jid, jid, sizeof(ft->streamhosts[ft->nexthost].jid));
							Q_strlcpy(ft->streamhosts[ft->nexthost].host, host, sizeof(ft->streamhosts[ft->nexthost].host));
							ft->streamhosts[ft->nexthost].port = port;
							ft->nexthost++;
						}
					}
					ft->streamstatus = STRM_AUTH;
					//iq gets acked when we finished connecting to the proxy. *then* the data can flow
					Q_strlcpy(ft->iqid, iqid, sizeof(ft->iqid));
					return true;
				}
			}
		}
		JCL_AddClientMessagef(jcl, "<iq id='%s' to='%s' type='error'/>", iqid, iqfrom);
		return true;
	}

	ot = XML_ChildOfTreeNS(tree, "http://jabber.org/protocol/ibb", "open", 0);
	if (ot)
	{
		struct ft_s *ft;
		const char *sid = XML_GetParameter(ot, "sid", "");
		int blocksize = atoi(XML_GetParameter(ot, "block-size", "4096"));	//technically this is required.
		const char *stanza = XML_GetParameter(ot, "stanza", "iq");
		for (ft = jcl->ft; ft; ft = ft->next)
		{
			if (!strcmp(ft->sid, sid) && !strcmp(ft->with, iqfrom))
			{
				if (ft->allowed && !ft->begun && ft->transmitting == false)
				{
					if (blocksize > 65536 || strcmp(stanza, "iq"))
					{	//blocksize: MUST NOT be greater than 65535
						JCL_AddClientMessagef(jcl, 
								"<iq id='%s' to='%s' type='error'>"
									"<error type='modify'>"
										"<not-acceptable xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
									"</error>"
								"</iq>"
								, iqid, iqfrom);
					}
					else if (blocksize > 4096)
					{	//ask for smaller chunks
						JCL_AddClientMessagef(jcl, 
							"<iq id='%s' to='%s' type='error'>"
								"<error type='modify'>"
									"<resource-constraint xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
								"</error>"
							"</iq>"
							, iqid, iqfrom);
					}
					else
					{	//it looks okay
						filefuncs->Open(ft->fname, &ft->file, 2);
						ft->method = FT_IBB;
						ft->blocksize = blocksize;
						ft->begun = true;
						//if its okay...
						JCL_AddClientMessagef(jcl, "<iq id='%s' to='%s' type='result'/>", iqid, iqfrom);
					}
					break;
				}
			}
		}
		return false;
	}
	ot = XML_ChildOfTreeNS(tree, "http://jabber.org/protocol/ibb", "close", 0);
	if (ot)
	{
		struct ft_s **link, *ft;
		const char *sid = XML_GetParameter(ot, "sid", "");
		for (link = &jcl->ft; *link; link = &(*link)->next)
		{
			ft = *link;
			if (!strcmp(ft->sid, sid) && !strcmp(ft->with, iqfrom))
			{
				if (ft->begun && ft->method == FT_IBB)
				{
					int size;
					if (ft->file != -1)
						filefuncs->Close(ft->file);
					if (ft->transmitting)
					{
						if (ft->eof)
							Con_Printf("Sent \"%s\" to \"%s\"\n", ft->fname, ft->with);
						else
							Con_Printf("%s aborted transfer of \"%s\"\n", iqfrom, ft->fname);
					}
					else
					{
						size = filefuncs->Open(ft->fname, &ft->file, 1);
						if (ft->file != -1)
							filefuncs->Close(ft->file);
						if (size == ft->size)
							Con_Printf("Received file \"%s\" successfully\n", ft->fname);
						else
							Con_Printf("%s aborted transfer of \"%s\"\n", iqfrom, ft->fname);
					}
					*link = ft->next;
					free(ft);
					//if its okay...
					JCL_AddClientMessagef(jcl, "<iq id='%s' to='%s' type='result'/>", iqid, iqfrom);
					return true;
				}
			}
		}
		return false;	//some kind of error
	}
	ot = XML_ChildOfTreeNS(tree, "http://jabber.org/protocol/ibb", "data", 0);
	if (ot)
	{
		char block[65536];
		const char *sid = XML_GetParameter(ot, "sid", "");
//		unsigned short seq = atoi(XML_GetParameter(ot, "seq", "0"));
		int blocksize;
		struct ft_s *ft;
		//FIXME: validate the sequence!
		for (ft = jcl->ft; ft; ft = ft->next)
		{
			if (!strcmp(ft->sid, sid) && !ft->transmitting)
			{
				blocksize = Base64_Decode(block, sizeof(block), ot->body, strlen(ot->body));
				if (blocksize && blocksize <= ft->blocksize)
				{
					filefuncs->Write(ft->file, block, blocksize);
					JCL_AddClientMessagef(jcl, "<iq id='%s' to='%s' type='result'/>", iqid, iqfrom);
					return true;
				}
				else
					Con_Printf("XMPP: Invalid blocksize in file transfer from %s\n", iqfrom);
				break;
			}
		}
		return true;
	}

	ot = XML_ChildOfTreeNS(tree, "http://jabber.org/protocol/si", "si", 0);
	if (ot)
	{
		const char *profile = XML_GetParameter(ot, "profile", "");

		if (!strcmp(profile, "http://jabber.org/protocol/si/profile/file-transfer"))
		{
//			char *mimetype = XML_GetParameter(ot, "mime-type", "text/plain");
			const char *sid = XML_GetParameter(ot, "id", "");
			xmltree_t *file = XML_ChildOfTreeNS(ot, "http://jabber.org/protocol/si/profile/file-transfer", "file", 0);
			const char *fname = XML_GetParameter(file, "name", "file.txt");
//			char *date = XML_GetParameter(file, "date", "");
			const char *md5hash = XML_GetParameter(file, "hash", "");
			int fsize = strtoul(XML_GetParameter(file, "size", "0"), NULL, 0);
//			char *desc = XML_GetChildBody(file, "desc", "");
			char authlink[512];
			char denylink[512];
			
			//file transfer offer
			struct ft_s *ft = malloc(sizeof(*ft) + strlen(iqfrom)+1);
			memset(ft, 0, sizeof(*ft));
			ft->stream = -1;
			ft->next = jcl->ft;
			jcl->ft = ft;
			ft->privateid = ++jcl->privateidseq;

			ft->transmitting = false;
			Q_strlcpy(ft->iqid, iqid, sizeof(ft->iqid));
			Q_strlcpy(ft->sid, sid, sizeof(ft->sid));
			Q_strlcpy(ft->fname, fname, sizeof(ft->sid));
			Base64_Decode(ft->md5hash, sizeof(ft->md5hash), md5hash, strlen(md5hash));
			ft->size = fsize;
			ft->file = -1;
			ft->with = (char*)(ft+1);
			strcpy(ft->with, iqfrom);
			ft->method = (fsize > 1024*128)?FT_BYTESTREAM:FT_IBB;	//favour bytestreams for large files. for small files, just use ibb as it saves sorting out proxies.
			ft->begun = false;

			JCL_GenLink(jcl, authlink, sizeof(authlink), "fauth", iqfrom, NULL, va("%i", ft->privateid), "%s", "Accept");
			JCL_GenLink(jcl, denylink, sizeof(denylink), "fdeny", iqfrom, NULL, va("%i", ft->privateid), "%s", "Deny");

			Con_Printf("%s has offered to send you \"%s\" (%i bytes). %s %s\n", iqfrom, fname, fsize, authlink, denylink);
		}
		else
		{
			//profile not understood
			JCL_AddClientMessagef(jcl,
					"<iq type='error' to='%s' id='%s'>"
						"<error code='400' type='cancel'>"
							"<bad-request xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
							"<bad-profile xmlns='http://jabber.org/protocol/si'/>"
						"</error>"
					"</iq>", iqfrom, iqid);
		}
		return true;
	}

	return false;
}
#endif
