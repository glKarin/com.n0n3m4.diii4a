typedef struct xmlparams_s
{
	char val[256];	//FIXME: make pointer
	struct xmlparams_s *next;
	char name[128];	//FIXME: make variable sized
} xmlparams_t;

typedef struct subtree_s
{
	char name[64];												//FIXME: make pointer to tail of structure
	char xmlns[64];			//namespace of the element			//FIXME: make pointer to tail of structure
	char xmlns_dflt[64];	//default namespace of children		//FIXME: make pointer to tail of structure
	char *body;//[2048];											//FIXME: make pointer+variablesized

	xmlparams_t *params;

	struct subtree_s *child;
	struct subtree_s *sibling;
} xmltree_t;



const char *XML_GetParameter(xmltree_t *t, const char *paramname, const char *def);
void XML_AddParameter(xmltree_t *t, const char *paramname, const char *value);
void XML_AddParameteri(xmltree_t *t, const char *paramname, int value);
xmltree_t *XML_CreateNode(xmltree_t *parent, const char *name, const char *xmlns, const char *body);
char *XML_Markup(const char *s, char *d, int dlen);
void XML_Unmark(char *s);
char *XML_GenerateString(xmltree_t *root, qboolean readable);
xmltree_t *XML_Parse(const char *buffer, int *startpos, int maxpos, qboolean headeronly, const char *defaultnamespace);
void XML_Destroy(xmltree_t *t);
xmltree_t *XML_ChildOfTree(xmltree_t *t, const char *name, int childnum);
xmltree_t *XML_ChildOfTreeNS(xmltree_t *t, const char *xmlns, const char *name, int childnum);
const char *XML_GetChildBody(xmltree_t *t, const char *paramname, const char *def);
void XML_ConPrintTree(xmltree_t *t, const char *subconsole, int indent);

xmltree_t *XML_FromJSON(xmltree_t *t, const char *name, const char *json, int *jsonpos, int jsonlen);
