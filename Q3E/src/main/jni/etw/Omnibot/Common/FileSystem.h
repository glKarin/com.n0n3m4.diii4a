#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

// class: FileSystem
//		Wraps filesystem functionality including searching archives and search paths for files.
class FileSystem
{
public:
	
	// function: FindAllFiles
	//		Searches for files matching an extension in the list of search paths, returns the path if found.
	static void FindAllFiles(const String &_path, DirectoryList &_list, const String &_expression = ".*", bool recurse = false);
	//////////////////////////////////////////////////////////////////////////

	static bool InitFileSystem();
	static void LogInit();
	static bool InitRawFileSystem(const String &folder);
	static void ShutdownFileSystem();
	static bool SetWriteDirectory(const fs::path &_dir);

	static bool IsInitialized();

	static void EnumerateFiles(const char *_folder);

	static void MountArchives(const char *_folder, const char *_mountpoint = 0);

	static fs::path GetRealDir(const String &_file);
	static fs::path GetRealPath(const String &_file);

	static bool MakeDirectory(const char *_folder);
	static bool FileDelete(const filePath &_file);
	static bool FileExists(const filePath &_file);
	static obint64 FileModifiedTime(const filePath &_file);

	enum MountOrder { MountFirst,MountLast };
	static bool Mount(const fs::path &_path, const char *_mountpoint = 0, MountOrder _order = MountFirst);
	static bool UnMount(const fs::path &_path);

	static obuint32 GetFileCrc(const String &_file);
	static obuint32 CalculateCrc(const void *_data, obuint32 _size);
private:
	FileSystem() {}

	static void LogAvailableArchives();
};

class File_Private;

class File
{
public:
	enum FileMode
	{
		Binary,
		Text
	};

	FileMode GetFileMode() const { return m_TextMode ? Text : Binary; }

	bool OpenForWrite(const char *_name, FileMode _mode, bool _append = false);
	bool OpenForRead(const char *_name, FileMode _mode);
	void Close();
	bool IsOpen();

	bool WriteInt8(obuint8 i);
	bool WriteInt16(obuint16 i);
	bool WriteInt32(obuint32 i, bool spaceatend = true);
	bool WriteInt64(obuint64 i, bool spaceatend = true);
	bool WriteFloat(float f);
	obuint32 Write(const void *_buffer, obuint32 _size, obuint32 _numitems = 1);
	bool WriteString(const String &_str);
	void Printf(CHECK_PRINTF_ARGS const char* _msg, ...);
	bool WriteNewLine();
	bool WriteIntPk(obuint32 i);
	bool WriteIntPk(obuint64 i);
	bool WriteSignIntPk(obint32 i);
	bool WriteStringPk(const String& _str);

	bool ReadInt8(obuint8 &i);
	bool ReadInt16(obuint16 &i);
	bool ReadInt32(obuint32 &i);
	bool ReadInt64(obuint64 &i);
	bool ReadFloat(float &f);
	obuint32 Read(void *_buffer, obuint32 _size, obuint32 _numitems = 1);
	bool ReadString(String &_str);
	bool ReadLine(String &_str);
	bool ReadIntPk(obuint32 &i);
	bool ReadIntPk(obuint64& i);
	bool ReadSignIntPk(obint32& i);
	bool ReadStringPk(String& _str);

	obuint64 ReadWholeFile(String &_readto);

	bool Seek(obuint64 _pos);
	obint64 Tell();
	bool EndOfFile();
	bool SetBuffer(obuint64 _size);
	bool Flush();
	obint64 FileLength();
	
	String GetLastError();

	File();
	~File();
private:
	File_Private *m_pFile;

	bool		m_TextMode : 1;
	
	bool ReadString(String& _str, obuint32 len);
};

#endif

