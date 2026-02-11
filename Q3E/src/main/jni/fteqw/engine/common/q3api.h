
#if defined(Q3CLIENT) || defined(Q3SERVER)
struct sfx_s;
struct server_static_s;
struct server_s;
struct usercmd_s;
struct q3gamecode_s
{
	struct
	{
		void (*SendAuthPacket)(struct ftenet_connections_s *socket, netadr_t *gameserver);
		void (*SendConnectPacket)(struct ftenet_connections_s *socket, netadr_t *to, int challenge, int qport, infobuf_t *userinfo);
		void (*Established)(void);
		void (VARGS *SendClientCommand)(const char *fmt, ...) LIKEPRINTF(1);
		void (*SendCmd)(struct ftenet_connections_s *socket, struct usercmd_s *cmd, unsigned int movesequence, double gametime);
		int (*ParseServerMessage) (sizebuf_t *msg);
		void (*Disconnect) (struct ftenet_connections_s *socket);	//disconnects from the server, killing all connection+cgame state.
	} cl;

	struct
	{
		void			(*VideoRestarted)		(void);
		int				(*Redraw)				(double time);
		qboolean		(*ConsoleCommand)		(void);
		qboolean		(*KeyPressed)			(int key, int unicode, int down);
		unsigned int	(*GatherLoopingSounds)	(vec3_t *positions, unsigned int *entnums, struct sfx_s **sounds, unsigned int max);
	} cg;

	struct
	{
		qboolean (*IsRunning)(void);
		qboolean (*ConsoleCommand)(void);
		void (*Start) (void);
		qboolean (*OpenMenu)(void);
		void (*Reset)(void);
	} ui;

//server stuff
	struct
	{
		void		(*ShutdownGame)				(qboolean restart);
		qboolean	(*InitGame)					(struct server_static_s *server_state_static, struct server_s *server_state, qboolean restart);
		qboolean	(*ConsoleCommand)			(void);
		qboolean	(*PrefixedConsoleCommand)	(void);
		qboolean	(*HandleClient)				(netadr_t *from, sizebuf_t *msg);
		void		(*DirectConnect)			(netadr_t *from, sizebuf_t *msg);
		void		(*NewMapConnects)			(void);
		void		(*DropClient)				(struct client_s *cl);
		void		(*RunFrame)					(void);
		void		(*SendMessage)				(struct client_s  *client);
		qboolean	(*RestartGamecode)			(void);
		void		(*ServerinfoChanged)		(const char *key);
	} sv;
};

extern struct q3gamecode_s *q3;
#endif
