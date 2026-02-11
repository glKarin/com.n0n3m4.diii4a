#define MAX_ARGS 8
#define ARG_LEN 512

typedef struct cmdctxt_s cmdctxt_t;
struct cmdctxt_s {
	cluster_t *cluster;
	sv_t *qtv;
	int streamid;	//streamid, which is valid even if qtv is not, for specifying the streamid to use on connects
	char *arg[MAX_ARGS];
	int argc;
	void (*printfunc)(cmdctxt_t *ctx, char *str);
	void *printcookie;
	int printcookiesize;	//tis easier
	qboolean localcommand;
};

typedef void (*consolecommand_t) (cmdctxt_t *ctx);

void Cmd_Printf(cmdctxt_t *ctx, char *fmt, ...) PRINTFWARNING(2);
#define Cmd_Argc(ctx) ctx->argc
#define Cmd_Argv(ctx, num) (((unsigned int)ctx->argc <= (unsigned int)(num))?"": ctx->arg[num])
#define Cmd_IsLocal(ctx) ctx->localcommand


void Cmd_ExecuteNow(cmdctxt_t *ctx, char *command);
char *Rcon_Command(cluster_t *cluster, sv_t *source, char *command, char *resultbuffer, int resultbuffersize, int islocalcommand);//prints the command prints to an internal buffer
