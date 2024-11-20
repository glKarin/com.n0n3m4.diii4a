#ifndef __FILEDOWNLOADER__
#define __FILEDOWNLOADER__

#ifdef ENABLE_FILE_DOWNLOADER

class FileDownloader
{
public:

	static bool Init();
	static void Shutdown();
	static void Poll();

	static bool NavigationMissing(const String &_name);

	static void UpdateWaypoints(const String &_mapname, bool _load = false);
	static void UpdateAllWaypoints(bool _getnew = false);

protected:
private:
};

#endif

#endif
