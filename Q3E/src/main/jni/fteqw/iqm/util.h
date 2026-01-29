#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>
#include <float.h>
#include "iqm.h"

#define ASSERT(c) if(c) {}

#ifdef NULL
#undef NULL
#endif
#define NULL 0

#ifdef _WIN32
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif
#endif

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef signed long long int llong;
typedef unsigned long long int ullong;

/*inline void *operator new(size_t size)
{
	void *p = malloc(size);
	if(!p) abort();
	return p;
}
inline void *operator new[](size_t size)
{
	void *p = malloc(size);
	if(!p) abort();
	return p;
}
inline void operator delete(void *p) { if(p) free(p); }
inline void operator delete[](void *p) { if(p) free(p); }
inline void operator delete(void *p, size_t sz) { if(p) free(p); }
inline void operator delete[](void *p, size_t sz) { if(p) free(p); }
*/
inline void *operator new(size_t, void *p) { return p; }
inline void *operator new[](size_t, void *p) { return p; }
inline void operator delete(void *, void *) {}
inline void operator delete[](void *, void *) {}

#ifdef swap
#undef swap
#endif
template<class T>
static inline void swap(T &a, T &b)
{
	T t = a;
	a = b;
	b = t;
}
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
template<class T>
static inline T max(T a, T b)
{
	return a > b ? a : b;
}
template<class T>
static inline T min(T a, T b)
{
	return a < b ? a : b;
}

#ifndef countof
#define countof(n) (sizeof(n)/sizeof(n[0]))
#endif
#define clamp(a,b,c) (max(b, min(a, c)))

#define loop(v,m) for(int v = 0; v<int(m); v++)
#define loopi(m) loop(i,m)
#define loopj(m) loop(j,m)
#define loopk(m) loop(k,m)
#define loopl(m) loop(l,m)

#ifdef WIN32
#ifdef M_PI
#undef M_PI
#endif
#define M_PI 3.14159265

#ifndef __GNUC__
#pragma warning (3: 4189)       // local variable is initialized but not referenced
#pragma warning (disable: 4244) // conversion from 'int' to 'float', possible loss of data
#pragma warning (disable: 4267) // conversion from 'size_t' to 'int', possible loss of data
#pragma warning (disable: 4355) // 'this' : used in base member initializer list
#pragma warning (disable: 4996) // 'strncpy' was declared deprecated
#endif

#define PATHDIV '\\'
#else
#define __cdecl
#define _vsnprintf vsnprintf
#define PATHDIV '/'
#endif

// easy safe strings

#define MAXSTRLEN 260
typedef char string[MAXSTRLEN];

inline void vformatstring(char *d, const char *fmt, va_list v, int len = MAXSTRLEN) { _vsnprintf(d, len, fmt, v); d[len-1] = 0; }
inline char *copystring(char *d, const char *s, size_t len = MAXSTRLEN)
{ 
	size_t slen = min(strlen(s)+1, len);
	memcpy(d, s, slen);
	d[slen-1] = 0;
	return d;
}
inline char *concatstring(char *d, const char *s) { size_t len = strlen(d); return copystring(d+len, s, MAXSTRLEN-len); }

template<size_t N> inline void formatstring(char (&d)[N], const char *fmt, ...)
{
	va_list v;
	va_start(v, fmt);
	vformatstring(d, fmt, v, int(N));
	va_end(v);
}

#define defformatstring(d,...) string d; formatstring(d, __VA_ARGS__)
#define defvformatstring(d,last,fmt) string d; { va_list ap; va_start(ap, last); vformatstring(d, fmt, ap); va_end(ap); }

inline char *newstring(size_t l)                { return new char[l+1]; }
inline char *newstring(const char *s, size_t l) { return copystring(newstring(l), s, l+1); }
inline char *newstring(const char *s)           { size_t l = strlen(s); char *d = newstring(l); memcpy(d, s, l+1); return d; }

#define loopv(v)    for(int i = 0; i<(v).length(); i++)
#define loopvj(v)   for(int j = 0; j<(v).length(); j++)
#define loopvk(v)   for(int k = 0; k<(v).length(); k++)
#define loopvrev(v) for(int i = (v).length()-1; i>=0; i--)

template <class T> struct vector
{
	static const int MINSIZE = 8;

	T *buf;
	int alen, ulen;

	vector() : buf(NULL), alen(0), ulen(0)
	{
	}

	vector(const vector &v) : buf(NULL), alen(0), ulen(0)
	{
		*this = v;
	}

	~vector() { setsize(0); if(buf) delete[] (uchar *)buf; }

	vector<T> &operator=(const vector<T> &v)
	{
		setsize(0);
		if(v.length() > alen) growbuf(v.length());
		loopv(v) add(v[i]);
		return *this;
	}

	T &add(const T &x)
	{
		if(ulen==alen) growbuf(ulen+1);
		new (&buf[ulen]) T(x);
		return buf[ulen++];
	}

	T &add()
	{
		if(ulen==alen) growbuf(ulen+1);
		new (&buf[ulen]) T;
		return buf[ulen++];
	}

	T &dup()
	{
		if(ulen==alen) growbuf(ulen+1);
		new (&buf[ulen]) T(buf[ulen-1]);
		return buf[ulen++];
	}

	bool inrange(uint i) const { return i<uint(ulen); }
	bool inrange(int i) const { return i>=0 && i<ulen; }

	T &pop() { return buf[--ulen]; }
	T &last() { return buf[ulen-1]; }
	void drop() { ulen--; buf[ulen].~T(); }

	bool empty() const { return ulen==0; }
	int capacity() const { return alen; }
	int length() const { return ulen; }
	T &operator[](int i) { ASSERT(i>=0 && i<ulen); return buf[i]; }
	const T &operator[](int i) const { ASSERT(i >= 0 && i<ulen); return buf[i]; }

	void setsize(int i) { ASSERT(i <= ulen); ulen = i; }

	void swap(vector<T> &v)
	{
		::swap(buf, v.buf);
		::swap(ulen, v.ulen);
		::swap(alen, v.alen);
	}

	T *getbuf() { return buf; }
	const T *getbuf() const { return buf; }
	bool inbuf(const T *e) const { return e >= buf && e < &buf[ulen]; }

	void growbuf(int sz)
	{
		int olen = alen;
		if(!alen) alen = max(MINSIZE, sz);
		else while(alen < sz) alen *= 2;
		if(alen <= olen) return;
		uchar *newbuf = new uchar[alen*sizeof(T)];
		if(olen > 0)
		{
			memcpy(newbuf, buf, olen*sizeof(T));
			delete[] (uchar *)buf;
		}
		buf = (T *)newbuf;
	}

	T *reserve(int sz)
	{
		if(ulen+sz > alen) growbuf(ulen+sz);
		return &buf[ulen];
	}

	void advance(int sz)
	{
		ulen += sz;
	}

	void put(const T *v, int n)
	{
		memcpy(reserve(n), v, n*sizeof(T));
		advance(n);
	}
};

static inline uint hthash(const char *key)
{
	uint h = 5381;
	for(int i = 0, k; (k = key[i]); i++) h = ((h<<5)+h)^k;    // bernstein k=33 xor
	return h;
}

static inline bool htcmp(const char *x, const char *y)
{
	return !strcmp(x, y);
}

static inline uint hthash(int key)
{
	return key;
}

static inline bool htcmp(int x, int y)
{
	return x==y;
}

static inline bool htcmp(double x, double y)
{
	return x == y;
}

static inline uint hthash(double k)
{
	union { double f; uint h[sizeof(double)/sizeof(uint)]; } conv;
	conv.f = k;
	uint hash = conv.h[0];
	for(size_t i = 1; i < sizeof(conv.h)/sizeof(uint); i++) hash ^= conv.h[i];
	return hash;
}

template <class K, class T> struct hashtable
{
	typedef K key;
	typedef const K const_key;
	typedef T value;
	typedef const T const_value;

	enum { CHUNKSIZE = 64 };

	struct chain      { T data; K key; chain *next; };
	struct chainchunk { chain chains[CHUNKSIZE]; chainchunk *next; };

	int size;
	int numelems;
	chain **table;

	chainchunk *chunks;
	chain *unused;

	hashtable(int size = 1<<10)
	  : size(size)
	{
		numelems = 0;
		chunks = NULL;
		unused = NULL;
		table = new chain *[size];
		loopi(size) table[i] = NULL;
	}

	~hashtable()
	{
		if(table) delete[] table;
		deletechunks();
	}

	chain *insert(const K &key, uint h)
	{
		if(!unused)
		{
			chainchunk *chunk = new chainchunk;
			chunk->next = chunks;
			chunks = chunk;
			loopi(CHUNKSIZE-1) chunk->chains[i].next = &chunk->chains[i+1];
			chunk->chains[CHUNKSIZE-1].next = unused;
			unused = chunk->chains;
		}
		chain *c = unused;
		unused = unused->next;
		c->key = key;
		c->next = table[h]; 
		table[h] = c;
		numelems++;
		return c;
	}

	#define HTFIND(success, fail) \
		uint h = hthash(key)&(size-1); \
		for(chain *c = table[h]; c; c = c->next) \
		{ \
			if(htcmp(key, c->key)) return (success); \
		} \
		return (fail);

	template<class L>
	T *access(const L &key)
	{
		HTFIND(&c->data, NULL);
	}

	template<class L>
	T &access(const L &key, const T &data)
	{
		HTFIND(c->data, insert(key, h)->data = data);
	}

	template<class L>
	const T &find(const L &key, const T &notfound)
	{
		HTFIND(c->data, notfound);
	}

	template<class L>
	T &operator[](const L &key)
	{
		HTFIND(c->data, insert(key, h)->data);
	}

	#undef HTFIND
   
	template<class L>
	bool remove(const L &key)
	{
		uint h = hthash(key)&(size-1); 
		for(chain **p = &table[h], *c = table[h]; c; p = &c->next, c = c->next)
		{
			if(htcmp(key, c->key))
			{
				*p = c->next;
				c->data.~T();
				c->key.~K();
				new (&c->data) T;
				new (&c->key) K;
				c->next = unused;
				unused = c;
				numelems--;
				return true;
			}
		}
		return false;
	}

	void deletechunks()
	{
		for(chainchunk *nextchunk; chunks; chunks = nextchunk)
		{
			nextchunk = chunks->next;
			delete chunks;
		}
	}

	void clear()
	{
		if(!numelems) return;
		loopi(size) table[i] = NULL;
		numelems = 0;
		unused = NULL;
		deletechunks();
	}
};

#define enumerate(ht,k,e,t,f,b) loopi((ht).size)  for(hashtable<k,t>::chain *enumc = (ht).table[i]; enumc;) { hashtable<k,t>::const_key &e = enumc->key; t &f = enumc->data; enumc = enumc->next; b; }

template<class T>
struct unionfind
{
	struct ufval
	{
		int rank, next;
		T val;

		ufval(const T &val) : rank(0), next(-1), val(val) {}
	};

	vector<ufval> ufvals;

	void clear()
	{
		ufvals.setsize(0);
	}

	const T &find(int k, const T &noval, const T &initval)
	{
		if(k>=ufvals.length()) return initval;
		while(ufvals[k].next>=0) k = ufvals[k].next;
		if(ufvals[k].val == noval) ufvals[k].val = initval;
		return ufvals[k].val;
	}

	int compressfind(int k)
	{
		if(ufvals[k].next<0) return k;
		return ufvals[k].next = compressfind(ufvals[k].next);
	}

	void unite (int x, int y, const T &noval)
	{
		while(ufvals.length() <= max(x, y)) ufvals.add(ufval(noval));
		x = compressfind(x);
		y = compressfind(y);
		if(x==y) return;
		ufval &xval = ufvals[x], &yval = ufvals[y];
		if(xval.rank < yval.rank) xval.next = y;
		else
		{
			yval.next = x;
			if(xval.rank==yval.rank) yval.rank++;
		}
	}
};

template<class T>
struct listnode
{
	T *prev, *next;
};

template<class T>
struct list
{
	typedef listnode<T> node;

	int size;
	listnode<T> nodes;

	list() { clear(); }

	bool empty() const { return nodes.prev == nodes.next; }
	bool notempty() const { return nodes.prev != nodes.next; }

	T *first() const { return nodes.next; }
	T *last() const { return nodes.prev; }
	T *end() const { return (T *)&nodes; }

	void clear()
	{
		size = 0;
		nodes.prev = nodes.next = (T *)&nodes;
	}

	T *remove(T *node)
	{
		size--;
		node->prev->next = node->next;
		node->next->prev = node->prev;
		return node;
	}

	T *insertafter(T *node, T *pos)
	{
		size++;
		node->next = pos->next;
		node->next->prev = node;
		node->prev = pos;
		pos->next = node;
		return node;
	}

	T *insertbefore(T *node, T *pos)
	{
		size++;
		node->prev = pos->prev;
		node->prev->next = node;
		node->next = pos;
		pos->prev = node;
		return node;
	} 

	T *insertfirst(T *node) { return insertafter(node, end()); }
	T *insertlast(T *node) { return insertbefore(node, end()); }

	T *removefirst() { return remove(first()); }
	T *removelast() { return remove(last()); }
};

static inline bool islittleendian() { union { int i; uchar b[sizeof(int)]; } conv; conv.i = 1; return conv.b[0] != 0; }
inline ushort endianswap16(ushort n) { return (n<<8) | (n>>8); }
inline uint endianswap32(uint n) { return (n<<24) | (n>>24) | ((n>>8)&0xFF00) | ((n<<8)&0xFF0000); }
inline ullong endianswap64(ullong n) { return endianswap32(uint(n >> 32)) | ((ullong)endianswap32(uint(n)) << 32); }
template<class T> inline T endianswap(T n) { union { T t; uint i; } conv; conv.t = n; conv.i = endianswap32(conv.i); return conv.t; }
template<> inline uchar endianswap<uchar>(uchar n) { return n; }
template<> inline char endianswap<char>(char n) { return n; }
template<> inline ushort endianswap<ushort>(ushort n) { return endianswap16(n); }
template<> inline short endianswap<short>(short n) { return endianswap16(n); }
template<> inline uint endianswap<uint>(uint n) { return endianswap32(n); }
template<> inline int endianswap<int>(int n) { return endianswap32(n); }
template<> inline ullong endianswap<ullong>(ullong n) { return endianswap64(n); }
template<> inline llong endianswap<llong>(llong n) { return endianswap64(n); }
template<> inline double endianswap<double>(double n) { union { double t; uint i; } conv; conv.t = n; conv.i = endianswap64(conv.i); return conv.t; }
template<class T> inline void endianswap(T *buf, int len) { for(T *end = &buf[len]; buf < end; buf++) *buf = endianswap(*buf); }
template<class T> inline T endiansame(T n) { return n; }
template<class T> inline void endiansame(T *buf, int len) {}
template<class T> inline T lilswap(T n) { return islittleendian() ? n : endianswap(n); }
template<class T> inline void lilswap(T *buf, int len) { if(!islittleendian()) endianswap(buf, len); }
template<class T> inline T bigswap(T n) { return islittleendian() ? endianswap(n) : n; }
template<class T> inline void bigswap(T *buf, int len) { if(islittleendian()) endianswap(buf, len); }

/* workaround for some C platforms that have these two functions as macros - not used anywhere */
#ifdef getchar
#undef getchar
#endif
#ifdef putchar
#undef putchar
#endif

struct stream
{
	virtual ~stream() {}
	virtual void close() = 0;
	virtual bool end() = 0;
	virtual long tell() { return -1; }
	virtual bool seek(long offset, int whence = SEEK_SET) { return false; }
	virtual long size();
	virtual size_t read(void *buf, size_t len) { return 0; }
	virtual size_t write(const void *buf, size_t len) { return 0; }
	virtual int getchar() { uchar c; return read(&c, 1) == 1 ? c : -1; }
	virtual bool putchar(int n) { uchar c = n; return write(&c, 1) == 1; }
	virtual bool getline(char *str, size_t len);
	virtual bool putstring(const char *str) { size_t len = strlen(str); return write(str, len) == len; }
	virtual bool putline(const char *str) { return putstring(str) && putchar('\n'); }
	virtual int printf(const char *fmt, ...) { return -1; }

	template<class T> bool put(T n) { return write(&n, sizeof(n)) == sizeof(n); }
	template<class T> bool putlil(T n) { return put<T>(lilswap(n)); }
	template<class T> bool putbig(T n) { return put<T>(bigswap(n)); }

	template<class T> T get() { T n; return read(&n, sizeof(n)) == sizeof(n) ? n : 0; }
	template<class T> T getlil() { return lilswap(get<T>()); }
	template<class T> T getbig() { return bigswap(get<T>()); }
};

long stream::size()
{
	long pos = tell(), endpos;
	if(pos < 0 || !seek(0, SEEK_END)) return -1;
	endpos = tell();
	return pos == endpos || seek(pos, SEEK_SET) ? endpos : -1;
}

bool stream::getline(char *str, size_t len)
{
	loopi(len-1)
	{
		if(read(&str[i], 1) != 1) { str[i] = '\0'; return i > 0; }
		else if(str[i] == '\n') { str[i+1] = '\0'; return true; }
	}
	if(len > 0) str[len-1] = '\0';
	return true;
}

struct filestream : stream
{
	FILE *file;

	filestream() : file(NULL) {}
	~filestream() { close(); }

	bool open(const char *name, const char *mode)
	{
		if(file) return false;
		file = fopen(name, mode);
		return file!=NULL;
	}

	void close()
	{
		if(file) { fclose(file); file = NULL; }
	}

	bool end() { return feof(file)!=0; }
	long tell() { return ftell(file); }
	bool seek(long offset, int whence) { return fseek(file, offset, whence) >= 0; }
	size_t read(void *buf, size_t len) { return fread(buf, 1, len, file); }
	size_t write(const void *buf, size_t len) { return fwrite(buf, 1, len, file); }
	int getchar() { return fgetc(file); }
	bool putchar(int c) { return fputc(c, file)!=EOF; }
	bool getline(char *str, int len) { return fgets(str, len, file)!=NULL; }
	bool putstring(const char *str) { return fputs(str, file)!=EOF; }

	int printf(const char *fmt, ...)
	{
		va_list v;
		va_start(v, fmt);
		int result = vfprintf(file, fmt, v);
		va_end(v);
		return result;
	}
};

char *path(char *s)
{
	for(char *curpart = s;;)
	{
		char *endpart = strchr(curpart, '&');
		if(endpart) *endpart = '\0';
		if(curpart[0]=='<')
		{
			char *file = strrchr(curpart, '>');
			if(!file) return s;
			curpart = file+1;
		}
		for(char *t = curpart; (t = strpbrk(t, "/\\")); *t++ = PATHDIV);
		for(char *prevdir = NULL, *curdir = s;;)
		{
			prevdir = curdir[0]==PATHDIV ? curdir+1 : curdir;
			curdir = strchr(prevdir, PATHDIV);
			if(!curdir) break;
			if(prevdir+1==curdir && prevdir[0]=='.')
			{
				memmove(prevdir, curdir+1, strlen(curdir+1)+1);
				curdir = prevdir;
			}
			else if(curdir[1]=='.' && curdir[2]=='.' && curdir[3]==PATHDIV)
			{
				if(prevdir+2==curdir && prevdir[0]=='.' && prevdir[1]=='.') continue;
				memmove(prevdir, curdir+4, strlen(curdir+4)+1);
				curdir = prevdir;
			}
		}
		if(endpart)
		{
			*endpart = '&';
			curpart = endpart+1;
		}
		else break;
	}
	return s;
}

char *path(const char *s, bool copy)
{
	static string tmp;
	copystring(tmp, s);
	path(tmp);
	return tmp;
}

const char *parentdir(const char *directory)
{
	const char *p = directory + strlen(directory);
	while(p > directory && *p != '/' && *p != '\\') p--;
	static string parent;
	size_t len = p-directory+1;
	copystring(parent, directory, len);
	return parent;
}

stream *openfile(const char *filename, const char *mode)
{
	filestream *file = new filestream;
	if(!file->open(path(filename, true), mode)) { delete file; return NULL; }
	return file;
}

struct Vec4;

struct Vec3
{
	union
	{
		struct { double x, y, z; };
		double v[3];
		uint h[3*sizeof(double)/sizeof(uint)];
	};

	Vec3() {}
	Vec3(double x, double y, double z) : x(x), y(y), z(z) {}
	explicit Vec3(const double *v) : x(v[0]), y(v[1]), z(v[2]) {}
	explicit Vec3(const float *v) : x(v[0]), y(v[1]), z(v[2]) {}
	explicit Vec3(const Vec4 &v);

	double &operator[](int i) { return v[i]; }
	double operator[](int i) const { return v[i]; }

	bool operator==(const Vec3 &o) const { return x == o.x && y == o.y && z == o.z; }
	bool operator!=(const Vec3 &o) const { return x != o.x || y != o.y || z != o.z; }
	bool operator<(const Vec3 &o) const { return x < o.x || y < o.y || z < o.z; }
	bool operator>(const Vec3 &o) const { return x > o.x || y > o.y || z > o.z; }

	Vec3 operator+(const Vec3 &o) const { return Vec3(x+o.x, y+o.y, z+o.z); }
	Vec3 operator-(const Vec3 &o) const { return Vec3(x-o.x, y-o.y, z-o.z); }
	Vec3 operator+(double k) const { return Vec3(x+k, y+k, z+k); }
	Vec3 operator-(double k) const { return Vec3(x-k, y-k, z-k); }
	Vec3 operator-() const { return Vec3(-x, -y, -z); }
	Vec3 operator*(const Vec3 &o) const { return Vec3(x*o.x, y*o.y, z*o.z); }
	Vec3 operator/(const Vec3 &o) const { return Vec3(x/o.x, y/o.y, z/o.z); }
	Vec3 operator*(double k) const { return Vec3(x*k, y*k, z*k); }
	Vec3 operator/(double k) const { return Vec3(x/k, y/k, z/k); }

	Vec3 &operator+=(const Vec3 &o) { x += o.x; y += o.y; z += o.z; return *this; }
	Vec3 &operator-=(const Vec3 &o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	Vec3 &operator+=(double k) { x += k; y += k; z += k; return *this; }
	Vec3 &operator-=(double k) { x -= k; y -= k; z -= k; return *this; }
	Vec3 &operator*=(const Vec3 &o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
	Vec3 &operator/=(const Vec3 &o) { x /= o.x; y /= o.y; z /= o.z; return *this; }
	Vec3 &operator*=(double k) { x *= k; y *= k; z *= k; return *this; }
	Vec3 &operator/=(double k) { x /= k; y /= k; z /= k; return *this; }

	double dot(const Vec3 &o) const { return x*o.x + y*o.y + z*o.z; }
	double magnitude() const { return sqrt(dot(*this)); }
	double squaredlen() const { return dot(*this); }
	double dist(const Vec3 &o) const { return (*this - o).magnitude(); }
	Vec3 normalize() const { return *this * (1.0 / magnitude()); }
	Vec3 cross(const Vec3 &o) const { return Vec3(y*o.z-z*o.y, z*o.x-x*o.z, x*o.y-y*o.x); }
	Vec3 reflect(const Vec3 &n) const { return *this - n*2.0*dot(n); }
	Vec3 project(const Vec3 &n) const { return *this - n*dot(n); }

	Vec3 zxy() const { return Vec3(z, x, y); }
	Vec3 zyx() const { return Vec3(z, y, x); }
	Vec3 yxz() const { return Vec3(y, x, z); }
	Vec3 yzx() const { return Vec3(y, z, x); }
	Vec3 xzy() const { return Vec3(x, z, y); }
};

static inline bool htcmp(const Vec3 &x, const Vec3 &y)
{
	return x == y;
}

static inline uint hthash(const Vec3 &k)
{
	uint hash = k.h[0];
	for(size_t i = 1; i < sizeof(k.h)/sizeof(uint); i++) hash ^= k.h[i];
	return hash;
}

struct Vec4
{
	union
	{
		struct { double x, y, z, w; };
		double v[4];
		uint h[4*sizeof(double)/sizeof(uint)];
	};

	Vec4() {}
	Vec4(double x, double y, double z, double w) : x(x), y(y), z(z), w(w) {}
	explicit Vec4(const Vec3 &p, double w = 0) : x(p.x), y(p.y), z(p.z), w(w) {}
	explicit Vec4(const double *v) : x(v[0]), y(v[1]), z(v[2]), w(v[3]) {}
	explicit Vec4(const float *v) : x(v[0]), y(v[1]), z(v[2]), w(v[3]) {}

	double &operator[](int i)       { return v[i]; }
	double  operator[](int i) const { return v[i]; }

	bool operator==(const Vec4 &o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
	bool operator!=(const Vec4 &o) const { return x != o.x || y != o.y || z != o.z || w != o.w; }
	bool operator<(const Vec4 &o) const { return x < o.x || y < o.y || z < o.z || w < o.w; }
	bool operator>(const Vec4 &o) const { return x > o.x || y > o.y || z > o.z || w > o.w; }

	Vec4 operator+(const Vec4 &o) const { return Vec4(x+o.x, y+o.y, z+o.z, w+o.w); }
	Vec4 operator-(const Vec4 &o) const { return Vec4(x-o.x, y-o.y, z-o.z, w-o.w); }
	Vec4 operator+(double k) const { return Vec4(x+k, y+k, z+k, w+k); }
	Vec4 operator-(double k) const { return Vec4(x-k, y-k, z-k, w-k); }
	Vec4 operator-() const { return Vec4(-x, -y, -z, -w); }
	Vec4 operator*(double k) const { return Vec4(x*k, y*k, z*k, w*k); }
	Vec4 operator/(double k) const { return Vec4(x/k, y/k, z/k, w/k); }
	Vec4 addw(double f) const { return Vec4(x, y, z, w + f); }

	Vec4 &operator+=(const Vec4 &o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
	Vec4 &operator+=(const Vec3 &o) { x += o.x; y += o.y; z += o.z; return * this; }
	Vec4 &operator-=(const Vec4 &o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
	Vec4 &operator-=(const Vec3 &o) { x -= o.x; y -= o.y; z -= o.z; return * this; }
	Vec4 &operator+=(double k) { x += k; y += k; z += k; w += k; return *this; }
	Vec4 &operator-=(double k) { x -= k; y -= k; z -= k; w -= k; return *this; }
	Vec4 &operator*=(double k) { x *= k; y *= k; z *= k; w *= k; return *this; }
	Vec4 &operator/=(double k) { x /= k; y /= k; z /= k; w /= k; return *this; }

	double dot3(const Vec4 &o) const { return x*o.x + y*o.y + z*o.z; }
	double dot3(const Vec3 &o) const { return x*o.x + y*o.y + z*o.z; }
	double dot(const Vec4 &o) const { return dot3(o) + w*o.w; }
	double dot(const Vec3 &o) const  { return x*o.x + y*o.y + z*o.z + w; }
	double magnitude() const  { return sqrt(dot(*this)); }
	double magnitude3() const { return sqrt(dot3(*this)); }
	Vec4 normalize() const { return *this * (1.0 / magnitude()); }
	Vec3 cross3(const Vec4 &o) const { return Vec3(y*o.z-z*o.y, z*o.x-x*o.z, x*o.y-y*o.x); }
	Vec3 cross3(const Vec3 &o) const { return Vec3(y*o.z-z*o.y, z*o.x-x*o.z, x*o.y-y*o.x); }

	void setxyz(const Vec3 &o) { x = o.x; y = o.y; z = o.z; }
};

inline Vec3::Vec3(const Vec4 &v) : x(v.x), y(v.y), z(v.z) {}

static inline bool htcmp(const Vec4 &x, const Vec4 &y)
{
	return x == y;
}

static inline uint hthash(const Vec4 &k)
{
	uint hash = k.h[0];
	for(size_t i = 1; i < sizeof(k.h)/sizeof(uint); i++) hash ^= k.h[i];
	return hash;
}

struct Matrix3x3;
struct Matrix3x4;

struct Quat : Vec4
{
	Quat() {}
	Quat(double x, double y, double z, double w) : Vec4(x, y, z, w) {}
	Quat(const float *ptr) : Vec4(ptr) {}
	Quat(double angle, const Vec3 &axis)
	{
		double s = sin(0.5*angle);
		x = s*axis.x;
		y = s*axis.y;
		z = s*axis.z;
		w = cos(0.5*angle);
	}
	explicit Quat(const Vec3 &v) : Vec4(v.x, v.y, v.z, -sqrt(max(1.0 - v.squaredlen(), 0.0))) {}
	explicit Quat(const Matrix3x3 &m) { convertmatrix(m); }
	explicit Quat(const Matrix3x4 &m) { convertmatrix(m); }

	void restorew()
	{
		w = -sqrt(max(1.0 - dot3(*this), 0.0));
	}

	Quat operator*(const Quat &o) const
	{
		return Quat(w*o.x + x*o.w + y*o.z - z*o.y,
					w*o.y - x*o.z + y*o.w + z*o.x,
					w*o.z + x*o.y - y*o.x + z*o.w,
					w*o.w - x*o.x - y*o.y - z*o.z);
	}
	Quat &operator*=(const Quat &o) { return (*this = *this * o); }

	Quat operator+(const Vec4 &o) const { return Quat(x+o.x, y+o.y, z+o.z, w+o.w); }
	Quat &operator+=(const Vec4 &o) { return (*this = *this + o); }
	Quat operator-(const Vec4 &o) const { return Quat(x-o.x, y-o.y, z-o.z, w-o.w); }
	Quat &operator-=(const Vec4 &o) { return (*this = *this - o); }

	Quat operator-() const { return Quat(-x, -y, -z, w); }

	void flip() { x = -x; y = -y; z = -z; w = -w; }

	Vec3 transform(const Vec3 &p) const
	{
		return p + cross3(cross3(p) + p*w)*2.0;
	}

	template<class M>
	void convertmatrix(const M &m)
	{
		double trace = m.a.x + m.b.y + m.c.z;
		if(trace>0)
		{
			double r = sqrt(1 + trace), inv = 0.5/r;
			w = 0.5*r;
			x = (m.c.y - m.b.z)*inv;
			y = (m.a.z - m.c.x)*inv;
			z = (m.b.x - m.a.y)*inv;
		}
		else if(m.a.x > m.b.y && m.a.x > m.c.z)
		{
			double r = sqrt(1 + m.a.x - m.b.y - m.c.z), inv = 0.5/r;
			x = 0.5*r;
			y = (m.b.x + m.a.y)*inv;
			z = (m.a.z + m.c.x)*inv;
			w = (m.c.y - m.b.z)*inv;
		}
		else if(m.b.y > m.c.z)
		{
			double r = sqrt(1 + m.b.y - m.a.x - m.c.z), inv = 0.5/r;
			x = (m.b.x + m.a.y)*inv;
			y = 0.5*r;
			z = (m.c.y + m.b.z)*inv;
			w = (m.a.z - m.c.x)*inv;
		}
		else
		{
			double r = sqrt(1 + m.c.z - m.a.x - m.b.y), inv = 0.5/r;
			x = (m.a.z + m.c.x)*inv;
			y = (m.c.y + m.b.z)*inv;
			z = 0.5*r;
			w = (m.b.x - m.a.y)*inv;
		}
	}

	static Quat fromangles(const Vec3 &rot)
	{
		double cx = cos(rot.x/2), sx = sin(rot.x/2),
			   cy = cos(rot.y/2), sy = sin(rot.y/2),
			   cz = cos(rot.z/2), sz = sin(rot.z/2);
		Quat q(sx*cy*cz - cx*sy*sz,
			   cx*sy*cz + sx*cy*sz,
			   cx*cy*sz - sx*sy*cz,
			   cx*cy*cz + sx*sy*sz);
		if(q.w > 0) q.flip();
		return q;
	}

	static Quat fromdegrees(const Vec3 &rot) { return fromangles(rot * (M_PI / 180)); }
};

struct Matrix3x3
{
	Vec3 a, b, c;

	Matrix3x3() {}
	Matrix3x3(const Vec3 &a, const Vec3 &b, const Vec3 &c) : a(a), b(b), c(c) {}
	explicit Matrix3x3(const Quat &q) { convertquat(q); }
	explicit Matrix3x3(const Quat &q, const Vec3 &scale)
	{
		convertquat(q);
		a *= scale;
		b *= scale;
		c *= scale;
	}

	void convertquat(const Quat &q)
	{
		double x = q.x, y = q.y, z = q.z, w = q.w,
			   tx = 2*x, ty = 2*y, tz = 2*z,
			   txx = tx*x, tyy = ty*y, tzz = tz*z,
			   txy = tx*y, txz = tx*z, tyz = ty*z,
			   twx = w*tx, twy = w*ty, twz = w*tz;
		a = Vec3(1 - (tyy + tzz), txy - twz, txz + twy);
		b = Vec3(txy + twz, 1 - (txx + tzz), tyz - twx);
		c = Vec3(txz - twy, tyz + twx, 1 - (txx + tyy));
	}

	Matrix3x3 operator*(const Matrix3x3 &o) const
	{
		return Matrix3x3(
			o.a*a.x + o.b*a.y + o.c*a.z,
			o.a*b.x + o.b*b.y + o.c*b.z,
			o.a*c.x + o.b*c.y + o.c*c.z);
	}
	Matrix3x3 &operator*=(const Matrix3x3 &o) { return (*this = *this * o); }

	void transpose(const Matrix3x3 &o)
	{
		a = Vec3(o.a.x, o.b.x, o.c.x);
		b = Vec3(o.a.y, o.b.y, o.c.y);
		c = Vec3(o.a.z, o.b.z, o.c.z);
	}
	void transpose() { transpose(Matrix3x3(*this)); }

	Vec3 transform(const Vec3 &o) const { return Vec3(a.dot(o), b.dot(o), c.dot(o)); }

	float determinant()
	{
		return
			a.x * b.y * c.z +
			a.y * b.z * c.x +
			a.z * b.x * c.y -
			a.z * b.y * c.x -
			a.y * b.x * c.z -
			a.x * b.z * c.y;
	}
};

struct Matrix3x4
{
	Vec4 a, b, c;

	Matrix3x4() {}
	Matrix3x4(const Vec4 &a, const Vec4 &b, const Vec4 &c) : a(a), b(b), c(c) {}
	explicit Matrix3x4(const Matrix3x3 &rot, const Vec3 &trans)
		: a(Vec4(rot.a, trans.x)), b(Vec4(rot.b, trans.y)), c(Vec4(rot.c, trans.z))
	{
	}
	explicit Matrix3x4(const Quat &rot, const Vec3 &trans)
	{
		*this = Matrix3x4(Matrix3x3(rot), trans);
	}
	explicit Matrix3x4(const Quat &rot, const Vec3 &trans, const Vec3 &scale)
	{
		*this = Matrix3x4(Matrix3x3(rot, scale), trans);
	}

	Matrix3x4 operator*(float k) const { return Matrix3x4(*this) *= k; }
	Matrix3x4 &operator*=(float k)
	{
		a *= k;
		b *= k;
		c *= k;
		return *this;
	}

	Matrix3x4 operator+(const Matrix3x4 &o) const { return Matrix3x4(*this) += o; }
	Matrix3x4 &operator+=(const Matrix3x4 &o)
	{
		a += o.a;
		b += o.b;
		c += o.c;
		return *this;
	}
	Matrix3x4 operator+(const Vec3 &o) const { return Matrix3x4(*this) += o; }
	Matrix3x4 &operator+=(const Vec3 &o)
	{
		a[3] += o[0];
		b[3] += o[1];
		c[3] += o[2];
		return *this;
	}

	void invert(const Matrix3x4 &o)
	{
		Matrix3x3 invrot(Vec3(o.a.x, o.b.x, o.c.x), Vec3(o.a.y, o.b.y, o.c.y), Vec3(o.a.z, o.b.z, o.c.z));
		invrot.a /= invrot.a.squaredlen();
		invrot.b /= invrot.b.squaredlen();
		invrot.c /= invrot.c.squaredlen();
		Vec3 trans(o.a.w, o.b.w, o.c.w);
		a = Vec4(invrot.a, -invrot.a.dot(trans));
		b = Vec4(invrot.b, -invrot.b.dot(trans));
		c = Vec4(invrot.c, -invrot.c.dot(trans));
	}
	void invert() { invert(Matrix3x4(*this)); }

	Matrix3x4 operator*(const Matrix3x4 &o) const
	{
		return Matrix3x4(
			(o.a*a.x + o.b*a.y + o.c*a.z).addw(a.w),
			(o.a*b.x + o.b*b.y + o.c*b.z).addw(b.w),
			(o.a*c.x + o.b*c.y + o.c*c.z).addw(c.w));
	}
	Matrix3x4 &operator*=(const Matrix3x4 &o) { return (*this = *this * o); }

	Vec3 transform(const Vec3 &o) const { return Vec3(a.dot(o), b.dot(o), c.dot(o)); }
	Vec3 transform3(const Vec3 &o) const { return Vec3(a.dot3(o), b.dot3(o), c.dot3(o)); }
};

void conoutf(const char *s, ...)
{
	defvformatstring(msg,s,s);
	printf("%s\n", msg);
}

void fatal(const char *s, ...)    // failure exit
{
	defvformatstring(msg,s,s);
	fprintf(stderr, "%s\n", msg);

	exit(EXIT_FAILURE);
}

//
// According to IQM file spec, all field offsets must be 4-byte aligned.
// Given a desired destination pointer to write data to, add pad bytes
// to ensure 4-byte alignment. 
// 
unsigned int pad_field_ofs(unsigned int field_ofs) 
{
	return (field_ofs - 1) + 4 - ((field_ofs - 1) % 4);
}