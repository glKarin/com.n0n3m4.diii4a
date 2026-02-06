//emscripten's download mechanism lacks usable progress indicators.
void emscriptenfte_async_wget_data2(const char *url, void *postdata, int postlen, char *postmimetype, void *ctx, void (*onload)(void*ctx,int buf), void (*onerror)(void*ctx,int code), void (*onprogress)(void*ctx,int prog,int total));

//changes the page away from quake (oh noes!) or downloads something.
void emscriptenfte_window_location(const char *url);

//filesystem buffers are implemented in javascript so that we are not bound by power-of-two heap limitations quite so much.
//also, we can't use emscripten's stdio because it reserves 16m file handles or something.
//these buffers do not track file offsets nor file access permissions.
int emscriptenfte_buf_create(void);
int emscriptenfte_buf_open(const char *name, int createifneeded);	//open
int emscriptenfte_buf_rename(const char *oldname, const char *newname);	//rename files (if it was open, the handle now refers to the new file instead)
int emscriptenfte_buf_delete(const char *fname);			//delete the named file. there may be problems if its currently open
void emscriptenfte_buf_release(int handle);				//close
void emscriptenfte_buf_pushtolocalstore(int handle);			//make a copy in the browser's local storage, if possible.
unsigned int emscriptenfte_buf_getsize(int handle);			//get the size of the file buffer
int emscriptenfte_buf_read(int handle, int offset, void *data, int len);//read data
int emscriptenfte_buf_write(int handle, int offset, const void *data, int len);//write data. no access checks.
void emscritenfte_buf_enumerate(void (*Sys_EnumeratedFile)(void *ctx, size_t fsize), void *ctx, size_t namesize);
void emscriptenfte_openfile(void);

//websocket is implemented in javascript because there is no usable C api (emscripten's javascript implementation is shite and has fatal errors).
int emscriptenfte_ws_connect(const char *url, const char *wsprotocol);	//open a websocket connection to a specific host
void emscriptenfte_ws_close(int sockid);								//close it again
int emscriptenfte_ws_cansend(int sockid, int extra, int maxpending);	//returns false if we're blocking for some reason. avoids overflowing. everything is otherwise reliable.
int emscriptenfte_ws_send(int sockid, const void *data, int len);		//send data to the peer. queues data. never dropped.
int emscriptenfte_ws_recv(int sockid, void *data, int len);				//receive data from the peer.

int emscriptenfte_rtc_create(int clientside, void *ctxp, int ctxi, void(*cb)(void *ctxp, int ctxi, int type, const char *data), const char *json_config);					//open a webrtc connection to a specific broker url
void emscriptenfte_rtc_offer(int sock, const char *offer, const char *sdptype);//sets the remote sdp.
int emscriptenfte_rtc_candidate(int sock, const char *offer);				//adds a remote candidate.

//misc stuff for printf replacements
void emscriptenfte_alert(const char *msg);
void emscriptenfte_print(const char *msg);
void emscriptenfte_setupmainloop(int(*mainloop)(double timestamp));
NORETURN void emscriptenfte_abortmainloop(const char *caller, int fatal);

//we're trying to avoid including libpng+libjpeg+libogg in javascript due to it being redundant bloat.
//to use such textures/sounds, we can just 'directly' load them via webgl
void emscriptenfte_gl_loadtexturefile(int gltexid, int *width, int *height, void *data, int datasize, const char *fname, int premul, int genmips);
void emscriptenfte_al_loadaudiofile(int al_buf, void *data, int datasize);
int emscriptenfte_pcm_loadaudiofile(void *ctx, void(*callbackfunc)(void *ctx, void *dataptr, int frames, int channels, float rate), void *dataptr, size_t datasize, int snd_speed);

//avoid all of emscripten's sdl emulation.
//this resolves input etc issues.
unsigned long emscriptenfte_ticks_ms(void);
double emscriptenfte_uptime_ms(void);
void emscriptenfte_updatepointerlock(int wantpointerlock, int hidecursor);
void emscriptenfte_polljoyevents(void);
void emscriptenfte_settitle(const char *text);
int emscriptenfte_setupcanvas(
	int width,
	int height,
	void(*Resized)(int newwidth, int newheight, float scale),
	void(*Mouse)(unsigned int devid,int abs,float x,float y,float z,float size),
	void(*Button)(unsigned int devid, int down, int mbutton),
	int(*Keyboard)(unsigned int devid, int down, int keycode, int unicode),
	void(*LoadFile)(char *url, char *mime, int filehandle),
	void(*CbufAdd)(const char *text),
	void(*buttonevent)(int joydev, int button, int ispressed, int isstandard),
	void(*axisevent)(int joydev, int axis, float value, int isstandard),
	void(*orientation)(int joydev, float px,float py,float pz, float qx,float qy,float qz,float qw),
	int (*ShouldSwitchToFullscreen)(void)
	);

struct webxrinfo_s {
	int fbo;	//panic!
	int viewport[4];
	float projmatrix[16];
	float transform[16];
};
int emscriptenfte_xr_geteyeinfo(int maxeyes, struct webxrinfo_s *vreyes);
unsigned int emscriptenfte_xr_issupported(void);		//0 for can't support it, -1 for unknown support, otherwise bitmask of supported modes
int emscriptenfte_xr_isactive(void);	//true if we're actually using webvr or not.
void emscriptenfte_xr_setup(int mode);
	#define WXRM_TOGGLE -3
	#define WXRM_AUTO -2
	#define WXRM_DISABLE -1
	#define WXRM_INLINE 0
	#define WXRM_VR 1
	#define WXRM_AR 2
void emscriptenfte_xr_shutdown(void);
