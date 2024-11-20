#include "KeyValueIni.h"
#include <vector>

#include "CodeAnalysis.h"
#include <stdio.h>
#include <stdarg.h>

/*!
**
** Copyright (c) 2007 by John W. Ratcliff mailto:jratcliff@infiniplex.net
**
** Portions of this source has been released with the PhysXViewer application, as well as
** Rocket, CreateDynamics, ODF, and as a number of sample code snippets.
**
** If you find this code useful or you are feeling particularily generous I would
** ask that you please go to http://www.amillionpixels.us and make a donation
** to Troy DeMolay.
**
** DeMolay is a youth group for young men between the ages of 12 and 21.
** It teaches strong moral principles, as well as leadership skills and
** public speaking.  The donations page uses the 'pay for pixels' paradigm
** where, in this case, a pixel is only a single penny.  Donations can be
** made for as small as $4 or as high as a $100 block.  Each person who donates
** will get a link to their own site as well as acknowledgement on the
** donations blog located here http://www.amillionpixels.blogspot.com/
**
** If you wish to contact me you can use the following methods:
**
** Skype Phone: 636-486-4040 (let it ring a long time while it goes through switches)
** Skype ID: jratcliff63367
** Yahoo: jratcliff63367
** AOL: jratcliff1961
** email: jratcliff@infiniplex.net
**
**
** The MIT license:
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <vector>
#include <string>
#include <assert.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace KEYVALUEINI
{

#ifdef _WIN32
#pragma warning(disable:4996) // Disabling stupid .NET deprecated warning.
#endif

#define DEFAULT_BUFFER_SIZE 1000000
#define DEFAULT_GROW_SIZE   2000000

#if defined(__linux__) || ((defined __MACH__) && (defined __APPLE__))
#   define _stricmp(a,b) strcasecmp((a),(b))
#   define _vsnprintf(a,b,c,d) vsnprintf((a),(b),(c),(d))
#endif

	class FILE_INTERFACE
	{
	public:
		FILE_INTERFACE(const char *fname, CHECK_PARAM_VALID const char *spec,void *mem,int len)
		{
			mMyAlloc = false;
			mRead = true; // default is read access.
			mFph = 0;
			mData = (char *) mem;
			mLen  = len;
			mLoc  = 0;

			if ( spec && _stricmp(spec,"wmem") == 0 )
			{
				mRead = false;
				if ( mem == 0 || len == 0 )
				{
					mData = (char *)malloc(DEFAULT_BUFFER_SIZE);
					mLen  = DEFAULT_BUFFER_SIZE;
					mMyAlloc = true;
				}
			}

			if ( mData == 0 )
			{
				mFph = fopen(fname,spec);
			}

			strncpy(mName,fname,512);
		}

		~FILE_INTERFACE()
		{
			if ( mMyAlloc )
			{
				free(mData);
			}
			if ( mFph )
			{
				fclose(mFph);
			}
		}

		int read(char *data,int size)
		{
			int ret = 0;
			if ( (mLoc+size) <= mLen )
			{
				memcpy(data, &mData[mLoc], size );
				mLoc+=size;
				ret = 1;
			}
			return ret;
		}

		int write(const char *data,int size)
		{
			int ret = 0;

			if ( (mLoc+size) >= mLen && mMyAlloc ) // grow it
			{
				int newLen = mLen+DEFAULT_GROW_SIZE;
				if ( size > newLen ) newLen = size+DEFAULT_GROW_SIZE;

				char *d = (char *)malloc(newLen);
				memcpy(d,mData,mLoc);
				free(mData);
				mData = d;
				mLen  = newLen;
			}

			if ( (mLoc+size) <= mLen )
			{
				memcpy(&mData[mLoc],data,size);
				mLoc+=size;
				ret = 1;
			}
			return ret;
		}

		int read(void *buffer,int size,int count)
		{
			int ret = 0;
			if ( mFph )
			{
				ret = (int)fread(buffer,size,count,mFph);
			}
			else
			{
				char *data = (char *)buffer;
				for (int i=0; i<count; i++)
				{
					if ( (mLoc+size) <= mLen )
					{
						read(data,size);
						data+=size;
						ret++;
					}
					else
					{
						break;
					}
				}
			}
			return ret;
		}

		int write(const void *buffer,int size,int count)
		{
			int ret = 0;

			if ( mFph )
			{
				ret = (int)fwrite(buffer,size,count,mFph);
			}
			else
			{
				const char *data = (const char *)buffer;

				for (int i=0; i<count; i++)
				{
					if ( write(data,size) )
					{
						data+=size;
						ret++;
					}
					else
					{
						break;
					}
				}
			}
			return ret;
		}

		int writeString(const char *str)
		{
			int ret = 0;
			if ( str )
			{
				int len = (int)strlen(str);
				ret = write(str,len, 1 );
			}
			return ret;
		}


		int  flush()
		{
			int ret = 0;
			if ( mFph )
			{
				ret = fflush(mFph);
			}
			return ret;
		}


		int seek(long loc,int mode)
		{
			int ret = 0;
			if ( mFph )
			{
				ret = fseek(mFph,loc,mode);
			}
			else
			{
				if ( mode == SEEK_SET )
				{
					if ( loc <= mLen )
					{
						mLoc = loc;
						ret = 1;
					}
				}
				else if ( mode == SEEK_END )
				{
					mLoc = mLen;
				}
				else
				{
					assert(0);
				}
			}
			return ret;
		}

		int tell()
		{
			int ret = 0;
			if ( mFph )
			{
				ret = ftell(mFph);
			}
			else
			{
				ret = mLoc;
			}
			return ret;
		}

		int myputc(char c)
		{
			int ret = 0;
			if ( mFph )
			{
				ret = fputc(c,mFph);
			}
			else
			{
				ret = write(&c,1);
			}
			return ret;
		}

		int eof()
		{
			int ret = 0;
			if ( mFph )
			{
				ret = feof(mFph);
			}
			else
			{
				if ( mLoc >= mLen )
					ret = 1;
			}
			return ret;
		}

		int  error()
		{
			int ret = 0;
			if ( mFph )
			{
				ret = ferror(mFph);
			}
			return ret;
		}


		FILE 	*mFph;
		char  *mData;
		int    mLen;
		int    mLoc;
		bool   mRead;
		char   mName[512];
		bool   mMyAlloc;

	};


	FILE_INTERFACE * fi_fopen(const char *fname,CHECK_PARAM_VALID const char *spec,void *mem=0,int len=0)
	{
		FILE_INTERFACE *ret = 0;

		ret = new FILE_INTERFACE(fname,spec,mem,len);

		if ( mem == 0 && ret->mData == 0)
		{
			if ( ret->mFph == 0 )
			{
				delete ret;
				ret = 0;
			}
		}

		return ret;
	}

	void       fi_fclose(FILE_INTERFACE *file)
	{
		delete file;
	}

	int        fi_fread(void *buffer,int size,int count,FILE_INTERFACE *fph)
	{
		int ret = 0;
		if ( fph )
		{
			ret = fph->read(buffer,size,count);
		}
		return ret;
	}

	int        fi_fwrite(const void *buffer,int size,int count,FILE_INTERFACE *fph)
	{
		int ret = 0;
		if ( fph )
		{
			ret = fph->write(buffer,size,count);
		}
		return ret;
	}

	int        fi_fprintf(FILE_INTERFACE *fph,const char *fmt,...)
	{
		int ret = 0;
		va_list ap;
		va_start(ap, fmt);

		char buffer[2048];
		buffer[2047] = 0;

#ifdef __linux__
		vsnprintf(buffer,2047, fmt, ap);
#else
		_vsnprintf(buffer,2047, fmt, ap);
#endif

		va_end(ap);
		if ( fph )
		{
			ret = fph->writeString(buffer);
		}

		return ret;
	}

	int        fi_fflush(FILE_INTERFACE *fph)
	{
		int ret = 0;
		if ( fph )
		{
			ret = fph->flush();
		}
		return ret;
	}


	int        fi_fseek(FILE_INTERFACE *fph,int loc,int mode)
	{
		int ret = 0;
		if ( fph )
		{
			ret = fph->seek(loc,mode);
		}
		return ret;
	}

	int        fi_ftell(FILE_INTERFACE *fph)
	{
		int ret = 0;
		if ( fph )
		{
			ret = fph->tell();
		}
		return ret;
	}

	int        fi_fputc(char c,FILE_INTERFACE *fph)
	{
		int ret = 0;
		if ( fph )
		{
			ret = fph->myputc(c);
		}
		return ret;
	}

	int        fi_fputs(const char *str,FILE_INTERFACE *fph)
	{
		int ret = 0;
		if ( fph )
		{
			ret = fph->writeString(str);
		}
		return ret;
	}

	int        fi_feof(FILE_INTERFACE *fph)
	{
		int ret = 0;
		if ( fph )
		{
			ret = fph->eof();
		}
		return ret;
	}

	int        fi_ferror(FILE_INTERFACE *fph)
	{
		int ret = 0;
		if ( fph )
		{
			ret = fph->error();
		}
		return ret;
	}

	void *     fi_getMemBuffer(FILE_INTERFACE *fph,unsigned int &outputLength)
	{
		outputLength = 0;
		void * ret = 0;
		if ( fph )
		{
			ret = fph->mData;
			outputLength = fph->mLoc;
		}
		return ret;
	}

	/*******************************************************************/
	/******************** InParser.h  ********************************/
	/*******************************************************************/
	class InPlaceParserInterface
	{
	public:
		virtual int ParseLine(int lineno,int argc,const char **argv) =0;  // return TRUE to continue parsing, return FALSE to abort parsing process
		virtual ~InPlaceParserInterface() {}
	};

	enum SeparatorType
	{
		ST_DATA,        // is data
		ST_HARD,        // is a hard separator
		ST_SOFT,        // is a soft separator
		ST_EOS,          // is a comment symbol, and everything past this character should be ignored
		ST_COMMENT
	};

	class InPlaceParser
	{
	public:
		InPlaceParser()
		{
			Init();
		}

		InPlaceParser(char *data,int len)
		{
			Init();
			SetSourceData(data,len);
		}

		InPlaceParser(const char *fname)
		{
			Init();
			SetFile(fname);
		}

		~InPlaceParser();

		void Init()
		{
			mQuoteChar = 34;
			mData = 0;
			mLen  = 0;
			mMyAlloc = false;
			for (int i=0; i<256; i++)
			{
				mHard[i] = ST_DATA;
				mHardString[i*2] = (char)i;
				mHardString[i*2+1] = 0;
			}
			mHard[0]  = ST_EOS;
			mHard[32] = ST_SOFT;
			mHard[9]  = ST_SOFT;
			mHard[13] = ST_SOFT;
			mHard[10] = ST_SOFT;
		}

		void SetFile(CHECK_PARAM_VALID const char *fname); // use this file as source data to parse.

		void SetSourceData(char *data,int len)
		{
			mData = data;
			mLen  = len;
			mMyAlloc = false;
		};

		void validateMem(const char *data,int len)
		{
			for (int i=0; i<len; i++)
			{
				assert( data[i] );
			}
		}

		void SetSourceDataCopy(const char *data,int len)
		{
			if ( len )
			{
				free(mData);
				mData = (char *)malloc(len+1);
				if ( mData ) {
					memcpy(mData,data,len);
					mData[len] = 0;

					//validateMem(mData,len);
					mLen  = len;
					mMyAlloc = true;
				} else {
					mLen = 0;
					mMyAlloc = false;
				}
			}
		};

		int  Parse(InPlaceParserInterface *callback); // returns true if entire file was parsed, false if it aborted for some reason

		int ProcessLine(int lineno,char *line,InPlaceParserInterface *callback);

		void SetHardSeparator(unsigned char c) // add a hard separator
		{
			mHard[c] = ST_HARD;
		}

		void SetHard(unsigned char c) // add a hard separator
		{
			mHard[c] = ST_HARD;
		}


		void SetCommentSymbol(unsigned char c) // comment character, treated as 'end of string'
		{
			mHard[c] = ST_COMMENT;
		}

		void ClearHardSeparator(unsigned char c)
		{
			mHard[c] = ST_DATA;
		}


		void DefaultSymbols(); // set up default symbols for hard seperator and comment symbol of the '#' character.

		bool EOS(unsigned char c)
		{
			if ( mHard[c] == ST_EOS || mHard[c] == ST_COMMENT )
			{
				return true;
			}
			return false;
		}

		void SetQuoteChar(char c)
		{
			mQuoteChar = c;
		}


		inline bool IsComment(unsigned char c) const;

	private:


		inline char * AddHard(int &argc,const char **argv,char *foo);
		inline bool   IsHard(unsigned char c);
		inline char * SkipSpaces(char *foo);
		inline bool   IsWhiteSpace(unsigned char c);
		inline bool   IsNonSeparator(unsigned char c); // non seperator,neither hard nor soft

		bool   mMyAlloc; // whether or not *I* allocated the buffer and am responsible for deleting it.
		char  *mData;  // ascii data to parse.
		int    mLen;   // length of data
		SeparatorType  mHard[256];
		char   mHardString[256*2];
		char           mQuoteChar;
	};

	/*******************************************************************/
	/******************** InParser.cpp  ********************************/
	/*******************************************************************/
	void InPlaceParser::SetFile(const char *fname)
	{
		if ( mMyAlloc )
		{
			free(mData);
		}
		mData = 0;
		mLen  = 0;
		mMyAlloc = false;

		FILE *fph = fopen(fname,"rb");
		if ( fph )
		{
			fseek(fph,0L,SEEK_END);
			mLen = ftell(fph);
			fseek(fph,0L,SEEK_SET);
			if ( mLen )
			{
				mData = (char *) malloc(sizeof(char)*(mLen+1));
				if ( mData ) {
					int ok = (int)fread(mData, mLen, 1, fph);
					if ( !ok )
					{
						free(mData);
						mData = 0;
					}
					else
					{
						mData[mLen] = 0; // zero byte terminate end of file marker.
						mMyAlloc = true;
					}
				} else {
					mLen = 0;
					mMyAlloc = false;
				}
			}
			fclose(fph);
		}
	}

	InPlaceParser::~InPlaceParser()
	{
		if ( mMyAlloc )
		{
			free(mData);
		}
	}

#define MAXARGS 512

	bool InPlaceParser::IsHard(unsigned char c)
	{
		return mHard[c] == ST_HARD;
	}

	char * InPlaceParser::AddHard(int &argc,const char **argv,char *foo)
	{
		while ( IsHard(*foo) )
		{
			unsigned char c = *foo;
			const char *hard = &mHardString[c*2];
			if ( argc < MAXARGS )
			{
				argv[argc++] = hard;
			}
			foo++;
		}
		return foo;
	}

	bool   InPlaceParser::IsWhiteSpace(unsigned char c)
	{
		return mHard[c] == ST_SOFT;
	}

	char * InPlaceParser::SkipSpaces(char *foo)
	{
		while ( !EOS(*foo) && IsWhiteSpace(*foo) ) foo++;
		return foo;
	}

	bool InPlaceParser::IsNonSeparator(unsigned char c)
	{
		if ( !IsHard(c) && !IsWhiteSpace(c) && c != 0 ) return true;
		return false;
	}


	bool InPlaceParser::IsComment(unsigned char c) const
	{
		if ( mHard[c] == ST_COMMENT ) return true;
		return false;
	}

	int InPlaceParser::ProcessLine(int lineno,char *line,InPlaceParserInterface *callback)
	{
		int ret = 0;

		const char *argv[MAXARGS];
		int argc = 0;

		char *foo = SkipSpaces(line); // skip leading spaces...

		if ( IsComment(*foo) )  // if the leading character is a comment symbol.
			return 0;


		if ( !EOS(*foo) )  // if we are not at the end of string then..
		{
			argv[argc++] = foo;  // this is the key
			foo++;

			while ( !EOS(*foo) )  // now scan forward until we hit an equal sign.
			{
				if ( *foo == '=' ) // if this is the equal sign then...
				{
					*foo = 0; // stomp a zero byte on the equal sign to terminate the key we should search for trailing spaces too...
					// look for trailing whitespaces and trash them.
					char *scan = foo-1;
					while ( IsWhiteSpace(*scan) )
					{
						*scan = 0;
						scan--;
					}

					foo++;
					foo = SkipSpaces(foo);
					if ( !EOS(*foo) )
					{
						argv[argc++] = foo;
						foo++;
						while ( !EOS(*foo) )
						{
							foo++;
						}
						*foo = 0;
						scan = foo-1;
						while ( IsWhiteSpace(*scan) )
						{
							*scan = 0;
							scan--;
						}
						break;
					}
				}
				if ( *foo )
					foo++;
			}
		}

		*foo = 0;

		if ( argc )
		{
			ret = callback->ParseLine(lineno, argc, argv );
		}

		return ret;
	}

	int  InPlaceParser::Parse(InPlaceParserInterface *callback) // returns true if entire file was parsed, false if it aborted for some reason
	{
		assert( callback );
		if ( !mData ) return 0;

		int ret = 0;

		int lineno = 0;

		char *foo   = mData;
		char *begin = foo;


		while ( *foo )
		{
			if ( *foo == 10 || *foo == 13 )
			{
				lineno++;
				*foo = 0;

				if ( *begin ) // if there is any data to parse at all...
				{
					int v = ProcessLine(lineno,begin,callback);
					if ( v ) ret = v;
				}

				foo++;
				if ( *foo == 10 ) foo++; // skip line feed, if it is in the carraige-return line-feed format...
				begin = foo;
			}
			else
			{
				foo++;
			}
		}

		lineno++; // lasst line.

		int v = ProcessLine(lineno,begin,callback);
		if ( v ) ret = v;
		return ret;
	}


	void InPlaceParser::DefaultSymbols()
	{
		SetHardSeparator(',');
		SetHardSeparator('(');
		SetHardSeparator(')');
		SetHardSeparator('=');
		SetHardSeparator('[');
		SetHardSeparator(']');
		SetHardSeparator('{');
		SetHardSeparator('}');
		SetCommentSymbol('#');
	}


} // END KEYVALUE INI NAMESPACE

using namespace KEYVALUEINI;


class KeyValue
{
public:
	KeyValue(const char *key,const char *value,unsigned int lineno)
		: mLineNo(lineno)
		, mKey(key)
		, mValue(value)
	{
	}

	const char * getKey() const { return mKey.c_str(); };
	const char * getValue() const { return mValue.c_str(); };
	unsigned int getLineNo() const { return mLineNo; };

	void save(FILE_INTERFACE *fph) const
	{
		fi_fprintf(fph,"%-30s = %s\r\n", getKey(), getValue() );
	}

	void setValue(const char *value)
	{
		mValue = value;
	}

private:
	unsigned int mLineNo;
	std::string mKey;
	std::string mValue;
};

typedef std::vector< KeyValue > KeyValueVector;

class KeyValueSection
{
public:
	KeyValueSection(const char *section,unsigned int lineno)
		: mLineNo(lineno)
		, mSection(section)
	{
	}

	unsigned int getKeyCount() const { return (unsigned int)mKeys.size(); };
	const char * getSection() const { return mSection.c_str(); };
	unsigned int getLineNo() const { return mLineNo; };

	const char * locateValue(const char *key,unsigned int lineno) const
	{
		const char *ret = 0;

		for (unsigned int i=0; i<mKeys.size(); i++)
		{
			const KeyValue &v = mKeys[i];
			if ( _stricmp(key,v.getKey()) == 0 )
			{
				ret = v.getValue();
				lineno = v.getLineNo();
				break;
			}
		}
		return ret;
	}

	const char *getKey(unsigned int index,unsigned int &lineno) const
	{
		const char * ret  = 0;
		if ( index >= 0 && index < mKeys.size() )
		{
			const KeyValue &v = mKeys[index];
			ret = v.getKey();
			lineno = v.getLineNo();
		}
		return ret;
	}

	const char *getValue(unsigned int index,unsigned int &lineno) const
	{
		const char * ret  = 0;
		if ( index >= 0 && index < mKeys.size() )
		{
			const KeyValue &v = mKeys[index];
			ret = v.getValue();
			lineno = v.getLineNo();
		}
		return ret;
	}

	void addKeyValue(const char *key,const char *value,unsigned int lineno)
	{
		KeyValue kv(key,value,lineno);
		mKeys.push_back(kv);
	}

	void save(FILE_INTERFACE *fph) const
	{
		bool bHeader = false;
		if ( strcmp(getSection(),"@HEADER") == 0 )
		{
			bHeader = true;
		}
		else
		{
			fi_fprintf(fph,"[%s]\r\n", getSection() );
		}
		for (unsigned int i=0; i<mKeys.size(); i++)
		{
			mKeys[i].save(fph);
		}

		if(!bHeader || !mKeys.empty())
		{
			//fi_fprintf(fph,"\r\n");
			fi_fprintf(fph,"\r\n");
		}
	}


	bool  addKeyValue(const char *key,const char *value) // adds a key-value pair.  These pointers *must* be persistent for the lifetime of the INI file!
	{
		bool ret = false;

		for (unsigned int i=0; i<mKeys.size(); i++)
		{
			KeyValue &kv = mKeys[i];
			if ( strcmp(kv.getKey(),key) == 0 )
			{
				kv.setValue(value);
				ret = true;
				break;
			}
		}

		if ( !ret )
		{
			KeyValue kv(key,value,0);
			mKeys.push_back(kv);
			ret = true;
		}

		return ret;
	}

	void reset()
	{
		mKeys.clear();
	}

private:
	unsigned int	mLineNo;
	std::string		mSection;
	KeyValueVector	mKeys;
};

typedef std::vector< KeyValueSection *> KeyValueSectionVector;

class KeyValueIni : public InPlaceParserInterface
{
public:
	KeyValueIni(const char *fname)
	{
		mData.SetFile(fname);
		mData.SetCommentSymbol('#');
		mData.SetCommentSymbol('!');
		mData.SetCommentSymbol(';');
		mData.SetHard('=');
		mCurrentSection = 0;
		KeyValueSection *kvs = new KeyValueSection("@HEADER",0);
		mSections.push_back(kvs);
		mData.Parse(this);
	}

	KeyValueIni(const char *mem,unsigned int len)
	{
		if ( len )
		{
			mCurrentSection = 0;
			mData.SetSourceDataCopy(mem,len);

			mData.SetCommentSymbol('#');
			mData.SetCommentSymbol('!');
			mData.SetCommentSymbol(';');
			mData.SetHard('=');
			KeyValueSection *kvs = new KeyValueSection("@HEADER",0);
			mSections.push_back(kvs);
			mData.Parse(this);
		}
	}

	KeyValueIni()
	{
		mCurrentSection = 0;
		KeyValueSection *kvs = new KeyValueSection("@HEADER",0);
		mSections.push_back(kvs);
	}

	~KeyValueIni()
	{
		reset();
	}

	void reset()
	{
		KeyValueSectionVector::iterator i;
		for (i=mSections.begin(); i!=mSections.end(); ++i)
		{
			KeyValueSection *kvs = (*i);
			delete kvs;
		}
		mSections.clear();
		mCurrentSection = 0;
	}

	unsigned int getSectionCount() const { return (unsigned int)mSections.size(); };

	int ParseLine(int lineno,int argc,const char **argv)  // return TRUE to continue parsing, return FALSE to abort parsing process
	{
		if ( argc )
		{
			const char *key = argv[0];
			if ( key[0] == '[' )
			{
				key++;
				char *scan = (char *) key;
				while ( *scan )
				{
					if ( *scan == ']')
					{
						*scan = 0;
						break;
					}
					scan++;
				}
				mCurrentSection = -1;
				for (unsigned int i=0; i<mSections.size(); i++)
				{
					KeyValueSection &kvs = *mSections[i];
					if ( _stricmp(kvs.getSection(),key) == 0 )
					{
						mCurrentSection = (int) i;
						break;
					}
				}
				//...
				if ( mCurrentSection < 0 )
				{
					mCurrentSection = (int)mSections.size();
					KeyValueSection *kvs = new KeyValueSection(key,lineno);
					mSections.push_back(kvs);
				}
			}
			else
			{
				const char *value = argc >= 2 ? argv[1] : "";
				mSections[mCurrentSection]->addKeyValue(key,value,lineno);
			}
		}

		return 0;
	}

	KeyValueSection * locateSection(const char *section,unsigned int &keys,unsigned int &lineno) const
	{
		KeyValueSection *ret = 0;
		for (unsigned int i=0; i<mSections.size(); i++)
		{
			KeyValueSection *s = mSections[i];
			if ( _stricmp(section,s->getSection()) == 0 )
			{
				ret = s;
				lineno = s->getLineNo();
				keys = s->getKeyCount();
				break;
			}
		}
		return ret;
	}

	const KeyValueSection * getSection(unsigned int index,unsigned int &keys,unsigned int &lineno) const
	{
		const KeyValueSection *ret=0;
		if ( index >= 0 && index < mSections.size() )
		{
			const KeyValueSection &s = *mSections[index];
			ret = &s;
			lineno = s.getLineNo();
			keys = s.getKeyCount();
		}
		return ret;
	}

	/*bool save(const char *fname) const
	{
		bool ret = false;
		FILE_INTERFACE *fph = fi_fopen(fname,"wb");
		if ( fph )
		{
			for (unsigned int i=0; i<mSections.size(); i++)
			{
				mSections[i]->save(fph);
			}
			fi_fclose(fph);
			ret = true;
		}
		return ret;
	}*/

	void * saveMem(unsigned int &len) const
	{
		void *ret = 0;
		FILE_INTERFACE *fph = fi_fopen("mem","wmem");
		if ( fph )
		{
			for (unsigned int i=0; i<mSections.size(); i++)
			{
				mSections[i]->save(fph);
			}

			void *temp = fi_getMemBuffer(fph,len);
			if ( temp )
			{
				ret = malloc(len);
				memcpy(ret,temp,len);
			}

			fi_fclose(fph);
		}
		return ret;
	}


	KeyValueSection  *createKeyValueSection(const char *section_name,bool reset)  // creates, or locates and existing section for editing.  If reset it true, will erase previous contents of the section.
	{
		KeyValueSection *ret = 0;

		for (unsigned int i=0; i<mSections.size(); i++)
		{
			KeyValueSection *kvs = mSections[i];
			if ( strcmp(kvs->getSection(),section_name) == 0 )
			{
				ret = kvs;
				if ( reset )
				{
					ret->reset();
				}
				break;
			}
		}
		if ( ret == 0 )
		{
			ret = new KeyValueSection(section_name,0);
			mSections.push_back(ret);
		}

		return ret;
	}

private:
	int                   mCurrentSection;
	KeyValueSectionVector mSections;
	InPlaceParser         mData;
};



//KeyValueIni *loadKeyValueIni(const char *fname,unsigned int &sections)
//{
//	KeyValueIni *ret = 0;
//
//	ret = new KeyValueIni(fname);
//	sections = ret->getSectionCount();
//	if ( sections < 2 )
//	{
//		delete ret;
//		ret = 0;
//	}
//
//	return ret;
//}

KeyValueIni *     loadKeyValueIni(const char *mem,unsigned int len,unsigned int &sections)
{
	KeyValueIni *ret = 0;

	ret = new KeyValueIni(mem,len);
	sections = ret->getSectionCount();
	if ( sections < 2 )
	{
		delete ret;
		ret = 0;
	}

	return ret;
}

const KeyValueSection * locateSection(const KeyValueIni *ini,const char *section,unsigned int &keys,unsigned int &lineno)
{
	KeyValueSection *ret = 0;

	if ( ini )
	{
		ret = ini->locateSection(section,keys,lineno);
	}

	return ret;
}

const KeyValueSection * getSection(const KeyValueIni *ini,unsigned int index,unsigned int &keycount,unsigned int &lineno)
{
	const KeyValueSection *ret = 0;

	if ( ini )
	{
		ret = ini->getSection(index,keycount,lineno);
	}

	return ret;
}

const char *      locateValue(const KeyValueSection *section,const char *key,unsigned int &lineno)
{
	const char *ret = 0;

	if ( section )
	{
		ret = section->locateValue(key,lineno);
	}

	return ret;
}

const char *      getKey(const KeyValueSection *section,unsigned int keyindex,unsigned int &lineno)
{
	const char *ret = 0;

	if ( section )
	{
		ret = section->getKey(keyindex,lineno);
	}

	return ret;
}

const char *      getValue(const KeyValueSection *section,unsigned int keyindex,unsigned int &lineno)
{
	const char *ret = 0;

	if ( section )
	{
		ret = section->getValue(keyindex,lineno);
	}

	return ret;
}

void              releaseKeyValueIni(KeyValueIni *ini)
{
	delete ini;
}


const char *    getSectionName(const KeyValueSection *section)
{
	const char *ret = 0;
	if ( section )
	{
		ret = section->getSection();
	}
	return ret;
}


//bool  saveKeyValueIni(const KeyValueIni *ini,const char *fname)
//{
//	bool ret = false;
//
//	if ( ini )
//		ret = ini->save(fname);
//
//	return ret;
//}

void *  saveKeyValueIniMem(const KeyValueIni *ini,unsigned int &len)
{
	void *ret = 0;

	if ( ini )
		ret = ini->saveMem(len);

	return ret;
}

KeyValueSection  *createKeyValueSection(KeyValueIni *ini,const char *section_name,bool reset)
{
	KeyValueSection *ret = 0;

	if ( ini )
	{
		ret = ini->createKeyValueSection(section_name,reset);
	}
	return ret;
}

bool  addKeyValue(KeyValueSection *section,const char *key,const char *value)
{
	bool ret = false;

	if ( section )
	{
		ret = section->addKeyValue(key,value);
	}

	return ret;
}


KeyValueIni      *createKeyValueIni() // create an empty .INI file in memory for editing.
{
	KeyValueIni *ret = new KeyValueIni;
	return ret;
}

bool              releaseIniMem(void *mem)
{
	bool ret = false;
	if ( mem )
	{
		free(mem);
		ret = true;
	}
	return ret;
}


#define TEST_MAIN 0

#if TEST_MAIN
void main(int argc,const char **argv) // test to see if INI files work.
{
	const char *fname = "test.ini";
	unsigned int sections;
	const KeyValueIni *ini = loadKeyValueIni(fname,sections);
	if ( ini )
	{
		printf("INI file '%s' has %d sections.\r\n", fname, sections);
		for (unsigned int i=0; i<sections; i++)
		{
			unsigned int lineno;
			unsigned int keycount;
			const KeyValueSection *s = getSection(ini,i,keycount,lineno);
			assert(s);
			const char *sname  = getSectionName(s);
			assert(sname);
			printf("Section %d=%s starts at line number %d and contains %d keyvalue pairs\r\n", i+1, sname, lineno, keycount );
			for (unsigned int j=0; j<keycount; j++)
			{
				unsigned int lineno;
				const char *key = getKey(s,j,lineno);
				const char *value = getValue(s,j,lineno);
				if ( key && value )
				{
					printf("           %d  %s=%s\r\n", j, key , value );
				}
				else if ( key )
				{
					printf("           %d Key=%s\r\n", j, key );
				}
				else if ( value )
				{
					printf("           %d Value=%s\r\n", j, value );
				}
			}
		}
		releaseKeyValueIni(ini);
	}
	else
	{
		printf("Failed to load ini file '%s'\r\n", fname );
	}
}



#endif


