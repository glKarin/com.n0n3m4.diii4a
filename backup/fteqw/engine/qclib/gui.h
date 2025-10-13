void GoToDefinition(const char *name);
int Grep(const char *filename, const char *string);
void EditFile(const char *name, int line, pbool setcontrol);

void GUI_SetDefaultOpts(void);
int GUI_BuildParms(const char *args, const char **argv, int argv_size, pbool quick);

//unsigned char *PDECL QCC_ReadFile (const char *fname, void *buffer, int len, size_t *sz);
int QCC_RawFileSize (const char *fname);
pbool QCC_WriteFile (const char *name, void *data, int len);
void GUI_DialogPrint(const char *title, const char *text);

void *GUIReadFile(const char *fname, unsigned char *(*buf_get)(void *ctx, size_t len), void *buf_ctx, size_t *out_size, pbool issourcefile);
int GUIFileSize(const char *fname);

int GUI_ParseCommandLine(const char *args, pbool keepsrcanddir); //0=gui, 1=commandline
void GUI_SaveConfig(void);
void GUI_RevealOptions(void);
int GUIprintf(const char *msg, ...);

pbool GenBuiltinsList(char *buffer, int buffersize);
pbool GenAutoCompleteList(char *prefix, char *buffer, int buffersize);

extern char parameters[16384];

extern char progssrcname[256];
extern char progssrcdir[256];

extern pbool fl_nondfltopts;
extern pbool fl_hexen2;
extern pbool fl_ftetarg;
extern pbool fl_autohighlight;
extern pbool fl_compileonstart;
extern pbool fl_showall;
extern pbool fl_log;
