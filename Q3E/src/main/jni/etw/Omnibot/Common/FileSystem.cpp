#include "PrecompCommon.h"
#include "FileSystem.h"

#include "physfs.h"
#if __cplusplus < 201103L
#include <boost/version.hpp>
#endif

extern "C"
{
#include "7zCrc.h"
};

bool g_FileSystemInitialized = false;

namespace IOAssertions
{
	BOOST_STATIC_ASSERT(sizeof(float) == sizeof(obuint32));
}

struct FindInfo
{
	DirectoryList	&m_DirList;
	String			m_Expression;
	bool			m_Recursive;

	FindInfo(DirectoryList &_list, const String & _exp, bool recurse) :
		m_DirList(_list),
		m_Expression(_exp),
		m_Recursive(recurse)
	{
	}
};

void _FindAllCallback(void *data, const char *origdir, const char *filename)
{
	FindInfo *pInfo = static_cast<FindInfo*>(data);

	try
	{
		char fullname[512] = {};
		sprintf(fullname, "%s/%s", origdir, filename);

		if(PHYSFS_isDirectory(fullname))
		{
			if(pInfo->m_Recursive)
				PHYSFS_enumerateFilesCallback(filename, _FindAllCallback, pInfo);
			return;
		}

		if( Utils::RegexMatch( pInfo->m_Expression.c_str(),filename ) )
		{
			if(std::find(pInfo->m_DirList.begin(), pInfo->m_DirList.end(), fullname) == pInfo->m_DirList.end())
				pInfo->m_DirList.push_back(fullname);
		}
	}
	catch(const std::exception &)
	{
	}
}

void FileSystem::FindAllFiles(const String &_path, DirectoryList &_list, const String &_expression, bool recurse)
{
	FindInfo inf(_list, _expression, recurse);
	PHYSFS_enumerateFilesCallback(_path.c_str(), _FindAllCallback, &inf);
}

//////////////////////////////////////////////////////////////////////////

bool FileSystem::InitFileSystem()
{
	fs::path basePath = Utils::GetBaseFolder();
	if(!PHYSFS_init(basePath.string().c_str()))
	{
		OBASSERT(0, "PhysFS: Error Initializing: %s", PHYSFS_getLastError());
		return false;
	}

	PHYSFS_permitSymbolicLinks(1);

	//////////////////////////////////////////////////////////////////////////
	fs::path globalMapGoalPath = basePath / "global_scripts/mapgoals";
	PHYSFS_mount(globalMapGoalPath.string().c_str(), "scripts/mapgoals", 0);

	fs::path modPath = Utils::GetModFolder();
	if(!PHYSFS_mount(modPath.string().c_str(), NULL, 1))
	{
		LOGERR("Can't mount folder: " << modPath.string().c_str());
		OBASSERT(0, "PhysFS: Error Mounting Directory: %s", PHYSFS_getLastError());
		PHYSFS_deinit();
		return false;
	}

	fs::path globalGuiPath = basePath / "gui";
	PHYSFS_mount(globalGuiPath.string().c_str(), "gui", 0);

	fs::path globalScriptsPath = basePath / "global_scripts";
	PHYSFS_mount(globalScriptsPath.string().c_str(), "global_scripts", 0);

	fs::path configPath = basePath / "config";
	PHYSFS_mount(configPath.string().c_str(), "config", 0);

	const char* homePath = g_EngineFuncs->GetLogPath();
	if(homePath && *homePath)  PHYSFS_mount(homePath, "homepath", 0);

	CrcGenerateTable();
	g_FileSystemInitialized = true;
	return true;
}

void FileSystem::LogInit()
{
	PHYSFS_Version compiled;
	PHYSFS_VERSION(&compiled);
	LOG("Initializing PhysFS: Version " <<
		(int)compiled.major << "." <<
		(int)compiled.minor << "." <<
		(int)compiled.patch);

	LOGFUNC;
	fs::path basePath = Utils::GetBaseFolder();
	LOG("Your base directory is: "<<basePath.string().c_str());
	LOG("Your user directory is: "<<PHYSFS_getUserDir());
	fs::path modPath = Utils::GetModFolder();
	LOG("Your mod directory is: "<<modPath.string().c_str());

	LogAvailableArchives();
}

//////////////////////////////////////////////////////////////////////////

bool FileSystem::InitRawFileSystem(const String &folder)
{
	PHYSFS_Version compiled;
	PHYSFS_VERSION(&compiled);
	LOG("Initializing PhysFS: Version " <<
		(int)compiled.major << "." <<
		(int)compiled.minor << "." <<
		(int)compiled.patch);

	LOGFUNC;
	fs::path basePath = fs::path(folder.c_str());
	LOG("Your base directory is: " << folder.c_str());
	if(!PHYSFS_init(basePath.string().c_str()))
	{
		OBASSERT(0, "PhysFS: Error Initializing: %s", PHYSFS_getLastError());
		return false;
	}

	PHYSFS_permitSymbolicLinks(1);

	//////////////////////////////////////////////////////////////////////////

	LogAvailableArchives();

	CrcGenerateTable();
	g_FileSystemInitialized = true;
	return true;
}

//////////////////////////////////////////////////////////////////////////

void FileSystem::LogAvailableArchives()
{
	const PHYSFS_ArchiveInfo **rc = PHYSFS_supportedArchiveTypes();
	LOG("Supported Archive Types");
	if (*rc == NULL)
	{
		LOG("None!");
	}
	else
	{
		for(const PHYSFS_ArchiveInfo **i = rc; *i != NULL; i++)
		{
			LOG(" * " << (*i)->extension << " : " << (*i)->description);
			LOG("Written by " << (*i)->author << " @ " << (*i)->url );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void FileSystem::ShutdownFileSystem()
{
	if(PHYSFS_deinit())
	{
		LOG("PhysFS shut down successfully.");
	}
	else
	{
		OBASSERT(0, "Error Shutting Down PhysFS: %s", PHYSFS_getLastError());
		LOG("Error Shutting Down PhysFS: "<<PHYSFS_getLastError());
	}
}

//////////////////////////////////////////////////////////////////////////

bool FileSystem::IsInitialized()
{
	return g_FileSystemInitialized;
}

//////////////////////////////////////////////////////////////////////////

bool FileSystem::Mount(const fs::path &_path, const char *_mountpoint, MountOrder _order)
{
	if(!PHYSFS_mount(_path.string().c_str(), _mountpoint, _order==MountLast))
	{
		LOG("Error Mounting " << _path.string().c_str() << " : " << PHYSFS_getLastError());
		return false;
	}
	return true;
}

bool FileSystem::UnMount(const fs::path &_path)
{
	if(!PHYSFS_removeFromSearchPath(_path.string().c_str()))
	{
		LOG("Error UnMounting " << _path.string().c_str() << " : " << PHYSFS_getLastError());
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

bool FileSystem::SetWriteDirectory(const fs::path &_dir)
{
	if (!PHYSFS_setWriteDir(_dir.string().c_str()))
	{
		LOG("PhysFS: Error Setting Write Directory: " << PHYSFS_getLastError());
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

bool FileSystem::MakeDirectory(const char *_folder)
{
	if(!PHYSFS_mkdir(_folder))
	{
		LOG("Error Creating Directory " << _folder << " : " << PHYSFS_getLastError());
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

bool FileSystem::FileExists(const filePath &_file)
{
	return PHYSFS_exists(_file) != 0;
}

//////////////////////////////////////////////////////////////////////////

obint64 FileSystem::FileModifiedTime(const filePath &_file)
{
	return PHYSFS_getLastModTime(_file);
}

//////////////////////////////////////////////////////////////////////////

bool FileSystem::FileDelete(const filePath &_file)
{
	return PHYSFS_delete(_file) != 0;
}

//////////////////////////////////////////////////////////////////////////

fs::path FileSystem::GetRealDir(const String &_file)
{
	const char *pPath = PHYSFS_getRealDir(_file.c_str());

	try
	{
		return fs::path(pPath ? pPath : "");
	}
	catch(const std::exception & ex)
	{
		SOFTASSERTALWAYS(0, "Filesystem: %s", ex.what());
	}
	return fs::path("");
}

//////////////////////////////////////////////////////////////////////////

fs::path FileSystem::GetRealPath(const String &_file)
{
	try
	{
		fs::path filepath(_file);
		return GetRealDir(_file) / filepath.filename();
	}
	catch(const std::exception & ex)
	{
		SOFTASSERTALWAYS(0, "Filesystem: %s", ex.what());
	}
	return fs::path("");
}

//////////////////////////////////////////////////////////////////////////
obuint32 FileSystem::CalculateCrc(const void *_data, obuint32 _size)
{
	obuint32 crc = CRC_INIT_VAL;
	crc = CrcUpdate(crc, _data, (size_t)_size);
	crc = CRC_GET_DIGEST(crc);
	return crc;
}

obuint32 FileSystem::GetFileCrc(const String &_file)
{
	obuint32 crc = 0;

	File f;
	if(f.OpenForRead(_file.c_str(), File::Binary) && f.IsOpen())
	{
		const int iBufferSize = 4096;
		char buffer[iBufferSize] = {};

		crc = CRC_INIT_VAL;

		obuint64 rd = 0;
		while((rd = f.Read(buffer, 1, iBufferSize)) != 0)
		{
			crc = CrcUpdate(crc, buffer, (size_t)rd);
		}
		crc = CRC_GET_DIGEST(crc);

		f.Close();
	}

	return crc;
}

//////////////////////////////////////////////////////////////////////////

bool _SupportsArchiveType(const char* fileName)
{
	const char *ext = strrchr(fileName, '.');
	if(ext)
	{
		ext++;
		for(const PHYSFS_ArchiveInfo **i = PHYSFS_supportedArchiveTypes(); *i != NULL; i++)
		{
			if(!Utils::StringCompareNoCase(ext, (*i)->extension))
				return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
struct MntFile
{
	fs::path	FilePath;
	String		OrigDir;
};
typedef std::vector<MntFile> MountFiles;
//////////////////////////////////////////////////////////////////////////

void _MountAllCallback(void *data, const char *origdir, const char *str)
{
	try
	{
		if(_SupportsArchiveType(str))
		{
			MountFiles *FileList = (MountFiles*)data;

			char fullname[512] ={};
			sprintf(fullname, "%s/%s", origdir, str);
			const char *pDir = PHYSFS_getRealDir(fullname);
			if(pDir)
			{
				fs::path filepath(pDir);
				filepath /= origdir;
				filepath /= str;

				if(!fs::is_directory(filepath))
				{
					MntFile fl;
					fl.FilePath = filepath;
					fl.OrigDir = origdir;
					FileList->push_back(fl);
				}
			}
		}
	}
	catch(const std::exception & ex)
	{
		//ex;
		SOFTASSERTALWAYS(0, "Filesystem: %s", ex.what());
	}
}

bool _FileNameGT(const MntFile &_pt1, const MntFile &_pt2)
{
	return _pt1.FilePath.string() > _pt2.FilePath.string();
}

void FileSystem::MountArchives(const char *_folder, const char *_mountpoint)
{
	try
	{
		MountFiles files;
		PHYSFS_enumerateFilesCallback(_folder, _MountAllCallback, &files);
		std::sort(files.begin(), files.end(), _FileNameGT);

		MountFiles::const_iterator cIt = files.begin();
		for(; cIt != files.end(); ++cIt)
		{
			if(PHYSFS_mount(cIt->FilePath.string().c_str(), _mountpoint?_mountpoint:cIt->OrigDir.c_str(), 1))
			{
				LOG("Mounted: " << cIt->FilePath.string().c_str() << " to " << cIt->OrigDir.c_str());
			}
			else
			{
				const char *pError = PHYSFS_getLastError();
				SOFTASSERTALWAYS(0, "PhysFS: %s", pError ? pError : "Unknown Error");
			}
		}
	}
	catch(const std::exception & ex)
	{
		//ex;
		SOFTASSERTALWAYS(0, "Filesystem: ", ex.what());
	}
}

//////////////////////////////////////////////////////////////////////////

void EchoFileCallback(void *data, const char *origdir, const char *filename)
{
	if(PHYSFS_isDirectory(filename))
		PHYSFS_enumerateFilesCallback(filename, EchoFileCallback, 0);
	else
	{
		char fullname[512] = {};
		sprintf(fullname, "%s/%s", origdir, filename);
		const char *pDir = PHYSFS_getRealDir(fullname);

		EngineFuncs::ConsoleMessage(va("%s/%s : %s", origdir, filename, pDir?pDir:"<->"));
		Utils::OutputDebug(kInfo,"%s/%s : %s", origdir, filename, pDir?pDir:"<->");
	}
}

void FileSystem::EnumerateFiles(const char *_folder)
{
	Utils::OutputDebug(kNormal, "--------------------\n");
	PHYSFS_enumerateFilesCallback(_folder, EchoFileCallback, 0);
	Utils::OutputDebug(kNormal, "--------------------\n");
}

//////////////////////////////////////////////////////////////////////////
class File_Private
{
public:
	File_Private() : m_pPrivate(0) {}
	PHYSFS_File *m_pPrivate;
};
//////////////////////////////////////////////////////////////////////////

File::File() :
	m_pFile(new File_Private)
{
	m_TextMode = false;
}

File::~File()
{
	Close();
	OB_DELETE(m_pFile->m_pPrivate);
	OB_DELETE(m_pFile);
}

bool File::OpenForWrite(const char *_name, FileMode _mode, bool _append /*= false*/)
{
	m_pFile->m_pPrivate = _append ? PHYSFS_openAppend(_name) : PHYSFS_openWrite(_name);
	m_TextMode = _mode == Text;
	return m_pFile->m_pPrivate != NULL;
}

bool File::OpenForRead(const char *_name, FileMode _mode)
{
	m_pFile->m_pPrivate = PHYSFS_openRead(_name);
	m_TextMode = _mode == Text;
	return m_pFile->m_pPrivate != NULL;
}

void File::Close()
{
	if(m_pFile->m_pPrivate)
	{
		PHYSFS_close(m_pFile->m_pPrivate);
		m_pFile->m_pPrivate = 0;
	}
}

bool File::IsOpen()
{
	return m_pFile->m_pPrivate != NULL;
}

bool File::WriteInt8(obuint8 i)
{
	if(m_pFile->m_pPrivate)
	{
		if(m_TextMode)
		{
			StringStr str;
			str << i;
			String st;
			str >> st;
			st += " ";
			return WriteString(st);
		}
		else
		{
			return Write(&i, sizeof(i), 1) != 0;
		}
	}
	return false;
}

bool File::WriteInt16(obuint16 i)
{
	if(m_pFile->m_pPrivate)
	{
		if(m_TextMode)
		{
			StringStr str;
			str << i;
			String st;
			str >> st;
			st += " ";
			return WriteString(st);
		}
		else
		{
			return PHYSFS_writeULE16(m_pFile->m_pPrivate, i) != 0;
		}
	}
	return false;
}

bool File::WriteInt32(obuint32 i, bool spaceatend)
{
	if(m_pFile->m_pPrivate)
	{
		if(m_TextMode)
		{
			StringStr str;
			str << i;
			String st;
			str >> st;
			if(spaceatend)
				st += " ";
			return WriteString(st);
		}
		else
		{
			return PHYSFS_writeULE32(m_pFile->m_pPrivate, i) != 0;
		}
	}
	return false;
}

bool File::WriteInt64(obuint64 i, bool spaceatend)
{
	if(m_pFile->m_pPrivate)
	{
		if(m_TextMode)
		{
			StringStr str;
			str << i;
			String st;
			str >> st;
			if(spaceatend)
				st += " ";
			return WriteString(st);
		}
		else
		{
			return PHYSFS_writeULE64(m_pFile->m_pPrivate, i) != 0;
		}
	}
	return false;
}

bool File::WriteFloat(float f)
{
	if(m_pFile->m_pPrivate)
	{
		if(m_TextMode)
		{
			StringStr str;
			str << f;
			String st;
			str >> st;
			st += " ";
			return WriteString(st);
		}
		else
		{
			obuint32 tmp;
			memcpy(&tmp, &f, sizeof(tmp));
			return WriteInt32(tmp);
		}
	}
	return false;
}

obuint32 File::Write(const void *_buffer, obuint32 _size, obuint32 _numitems /*= 1*/)
{
	if (m_pFile->m_pPrivate && _size > 0) {
		PHYSFS_sint64 result = PHYSFS_write(m_pFile->m_pPrivate, _buffer, _size, _numitems);
		if (result > 0) return (obuint32)result;
	}
	return 0;
}

bool File::WriteString(const String &_str)
{
	if(m_pFile->m_pPrivate)
	{
		if(m_TextMode)
		{
			obuint32 len = (obuint32)_str.length();
			if(!Write(_str.c_str(), len, 1)) return false;
		}
		else
		{
			obuint32 len = (obuint32)_str.length();
			if(!WriteInt32(len)) return false;
			if(len > 0)
			{
				if(!Write(_str.c_str(), len, 1)) return false;
			}
		}
		return true;
	}
	return false;
}

bool File::WriteIntPk(obuint32 i)
{
	if (!m_pFile->m_pPrivate) return false;
	obuint8 buf[5];
	obuint32 len = 0;
	do {
		buf[len++] = ((obuint8)i & 0x7f);
		i >>= 7;
	} while (i);
	buf[len - 1] |= 0x80;
	return PHYSFS_write(m_pFile->m_pPrivate, buf, len, 1) == 1;
}

bool File::WriteIntPk(obuint64 i)
{
	if (!m_pFile->m_pPrivate) return false;
	obuint8 buf[10];
	obuint32 len = 0;
	do {
		buf[len++] = ((obuint8)i & 0x7f);
		i >>= 7;
	} while (i);
	buf[len - 1] |= 0x80;
	return PHYSFS_write(m_pFile->m_pPrivate, buf, len, 1) == 1;
}

bool File::WriteSignIntPk(obint32 i)
{
	return WriteIntPk(i>=0 ? (obuint32)i<<1 : ((obuint32)(~i)<<1) | 1);
}

bool File::WriteStringPk(const String& _str)
{
	obuint32 len = (obuint32)_str.length();
	if (!WriteIntPk(len)) return false;
	if (len > 0)
		if (!Write(_str.c_str(), len, 1)) return false;
	return true;
}

void File::Printf(const char* _msg, ...)
{
	static char buffer[8192] = {0};
	va_list list;
	va_start(list, _msg);
#ifdef WIN32
	_vsnprintf(buffer, 8192, _msg, list);
#else
	vsnprintf(buffer, 8192, _msg, list);
#endif
	va_end(list);

	WriteString(buffer);
}

bool File::WriteNewLine()
{
	const obuint8 cr = 0x0D;
	const obuint8 lf = 0x0A;
	return Write(&cr, sizeof(obuint8)) && Write(&lf, sizeof(obuint8));
}

bool File::ReadInt8(obuint8 &i)
{
	if(m_pFile->m_pPrivate)
	{
		if(m_TextMode)
		{
			String st;
			return ReadString(st) && Utils::ConvertString<obuint8>(st, i);
		}
		else
		{
			return Read(&i, sizeof(i), 1) != 0;
		}
	}
	return false;
}

bool File::ReadInt16(obuint16 &i)
{
	if(m_pFile->m_pPrivate)
	{
		if(m_TextMode)
		{
			String st;
			return ReadString(st) && Utils::ConvertString<obuint16>(st, i);
		}
		else if(PHYSFS_readULE16(m_pFile->m_pPrivate, &i))
		{
			return true;
		}
	}
	return false;
}

bool File::ReadInt32(obuint32 &i)
{
	if(m_pFile->m_pPrivate)
	{
		if(m_TextMode)
		{
			String st;
			return ReadString(st) && Utils::ConvertString<obuint32>(st, i);
		}
		else if(PHYSFS_readULE32(m_pFile->m_pPrivate, &i))
		{
			return true;
		}
	}
	return false;
}

bool File::ReadInt64(obuint64 &i)
{
	if(m_pFile->m_pPrivate)
	{
		if(m_TextMode)
		{
			String st;
			return ReadString(st) && Utils::ConvertString<obuint64>(st, i);
		}
		else if(PHYSFS_readULE64(m_pFile->m_pPrivate, &i))
		{
			return true;
		}
	}
	return false;
}

bool File::ReadFloat(float &f)
{
	if(m_pFile->m_pPrivate)
	{
		obuint32 tmp;
		if(m_TextMode)
		{
			String st;
			return ReadString(st) && Utils::ConvertString<float>(st, f);
		}
		else if(PHYSFS_readULE32(m_pFile->m_pPrivate, &tmp))
		{
			memcpy(&f, &tmp, sizeof(f));
			return true;
		}
	}
	return false;
}

obuint32 File::Read(void *_buffer, obuint32 _size, obuint32 _numitems /*= 1*/)
{
	if(m_pFile->m_pPrivate && _size > 0)
	{
		PHYSFS_sint64 result = PHYSFS_read(m_pFile->m_pPrivate, _buffer, _size, _numitems);
		if (result > 0) return (obuint32)result;
	}
	return 0;
}

obuint64 File::ReadWholeFile(String &_readto)
{
	enum { BufferSize = 4096 };
	char buffer[BufferSize] = {};

	obuint32 readBytes = 0, totalBytes = 0;
	while((readBytes = (obuint32)Read(buffer,1,BufferSize)))
	{
		_readto.append(buffer,readBytes);
		totalBytes += readBytes;
	}
	return totalBytes;
}

bool File::ReadString(String& _str, obuint32 len)
{
	if (len > 0)
	{
#if __cplusplus >= 201103L //karin: using std::shared_ptr<T[]> instead of boost::shared_array<T>
		compat::shared_array<char> pBuffer(new char[len + 1]);
#else
		boost::shared_array<char> pBuffer(new char[len + 1]);
#endif
		if (!Read(pBuffer.get(), len, 1)) return false;
		pBuffer.get()[len] = 0;
		_str = pBuffer.get();
	}
	return true;
}

bool File::ReadString(String &_str)
{
	if(m_pFile->m_pPrivate)
	{
		_str.clear();
		if(m_TextMode)
		{
			char ch;
			while(Read(&ch, sizeof(ch), 1) && !EndOfFile() && !Utils::IsWhiteSpace(ch))
				_str.push_back(ch);

			// eat white space.
			while(Read(&ch, sizeof(ch), 1) && !EndOfFile() && Utils::IsWhiteSpace(ch)) { }
			// go back by 1
			Seek(Tell()-1);
			return true;
		}
		else
		{
			obuint32 len;
			return ReadInt32(len) && ReadString(_str, len);
		}
	}
	return false;
}

bool File::ReadIntPk(obuint32& i)
{
	if (!m_pFile->m_pPrivate) return false;
	obuint32 j = 0;
	int len = 0;
	for(;;)
	{
		obuint8 c;
		if (PHYSFS_read(m_pFile->m_pPrivate, &c, 1, 1) <= 0) return false;
		j |= (obuint32)(c & 0x7f) << len;
		if (c & 0x80) break;
		len += 7;
	}
	i = j;
	return true;
}

bool File::ReadIntPk(obuint64& i)
{
	if (!m_pFile->m_pPrivate) return false;
	obuint64 j = 0;
	int len = 0;
	for (;;)
	{
		obuint8 c;
		if (PHYSFS_read(m_pFile->m_pPrivate, &c, 1, 1) <= 0) return false;
		j |= (obuint64)(c & 0x7f) << len;
		if (c & 0x80) break;
		len += 7;
	}
	i = j;
	return true;
}

bool File::ReadSignIntPk(obint32& i)
{
	obuint32 u;
	if (!ReadIntPk(u)) return false;
	i = (u & 1) ? ~(u >> 1) : (u >> 1);
	return true;
}

bool File::ReadStringPk(String& _str)
{
	_str.clear();
	obuint32 len;
	return ReadIntPk(len) && ReadString(_str, len);
}

bool File::ReadLine(String &_str)
{
	_str.resize(0);
	if(m_pFile->m_pPrivate)
	{
		OBASSERT(m_TextMode, "Function Only for Text Mode");
		if(m_TextMode)
		{
			if(EndOfFile())
				return false;

			char ch;
			while(Read(&ch, sizeof(ch), 1) && !EndOfFile() && ch != '\r' && ch != '\n')
				_str.push_back(ch);

			// eat white space.
			while(Read(&ch, sizeof(ch), 1) && !EndOfFile() && Utils::IsWhiteSpace(ch)) { }

			// go back by 1
			Seek(Tell()-1);
		}
	}
	return !_str.empty();
}

bool File::Seek(obuint64 _pos)
{
	return m_pFile->m_pPrivate ? PHYSFS_seek(m_pFile->m_pPrivate, _pos) != 0 : 0;
}

obint64 File::Tell()
{
	return m_pFile->m_pPrivate ? PHYSFS_tell(m_pFile->m_pPrivate) : -1;
}

bool File::EndOfFile()
{
	return m_pFile->m_pPrivate ? (PHYSFS_eof(m_pFile->m_pPrivate)!=0) : true;
}

bool File::SetBuffer(obuint64 _size)
{
	return m_pFile->m_pPrivate ? (PHYSFS_setBuffer(m_pFile->m_pPrivate, _size)!=0) : false;
}

bool File::Flush()
{
	return m_pFile->m_pPrivate ? (PHYSFS_flush(m_pFile->m_pPrivate)!=0) : false;
}

String File::GetLastError()
{
	const char *pError = PHYSFS_getLastError();
	return pError ? pError : "Unknown";
}

obint64 File::FileLength()
{
	return m_pFile->m_pPrivate ? PHYSFS_fileLength(m_pFile->m_pPrivate) : -1;
}
