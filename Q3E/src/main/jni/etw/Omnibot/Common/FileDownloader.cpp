#ifdef ENABLE_FILE_DOWNLOADER

#include "PrecompCommon.h"
#include "FileDownloader.h"
#include "NavigationManager.h"

#include <boost/asio.hpp>
using boost::asio::ip::tcp;

//////////////////////////////////////////////////////////////////////////

ThreadGroup g_MasterThreadGroup;

//////////////////////////////////////////////////////////////////////////

typedef boost::system::error_code errorcode;

struct FileVars
{
	String		_MapName;
	String		_GameName;
	int			_InterfaceNum;
	obuint32	_MapCrc;

	FileVars(const String &_map, const String &_game, int _interface)
		: _MapName(_map)
		, _GameName(_game)
		, _InterfaceNum(_interface)
		, _MapCrc(0)
	{
	}
};

class DownloadFile
{
public:
	enum KnownErrorCodes
	{
		QuerySuccess = 200,
		QueryFileNotInDatabase = 404,
		QueryFileMatches = 204,
		QueryBadUrl = 400,
	};
	DownloadFile(boost::asio::io_service& io_service,const FileVars& q)
		: mResolver(io_service)
		, mSocket(io_service)
		, mQuery(q)
	{
		String DlServer = "www.omni-bot.de";
		String DlScript = "/e107/e107_plugins/waypointer/wpstream.php";

		//////////////////////////////////////////////////////////////////////////
		String DlServerOverride, DlScriptOverride;
		Options::GetValue("Downloader","Server",DlServerOverride);
		Options::GetValue("Downloader","Script",DlScriptOverride);
		if(!DlServerOverride.empty())
			DlServer = DlServerOverride;
		if(!DlScriptOverride.empty())
			DlScript = DlScriptOverride;
		//////////////////////////////////////////////////////////////////////////

		String sFilePath = va("user/download/%s.zip",mQuery._MapName.c_str());
		mQuery._MapCrc = FileSystem::GetFileCrc(sFilePath);

		StringStr script,args;
		args << "?m=" << mQuery._MapName
			<< "&g=" << mQuery._GameName
			<< "&i=" << mQuery._InterfaceNum
			<< "&c=" << std::hex << mQuery._MapCrc
			;
		script << DlScript << args.str();

		EngineFuncs::ConsoleError(va("Attempting Download wpstream.php%s",args.str().c_str()));
		
		// Form the request. We specify the "Connection: close" header so that the
		// server will close the socket after transmitting the response. This will
		// allow us to treat all data up until the EOF as the content.
		std::ostream request_stream(&mRequest);
		request_stream << "GET " << script.str() << " HTTP/1.0\r\n";
		request_stream << "Host: " << DlServer << "\r\n";
		request_stream << "Accept: */*\r\n";
		request_stream << "Connection: close\r\n\r\n";

		// Start an asynchronous resolve to translate the server and service names
		// into a list of endpoints.
		tcp::resolver::query query(DlServer, "http");
		mResolver.async_resolve(query,
			boost::bind(&DownloadFile::handle_resolve, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::iterator));
	}
private:
	DownloadFile();
	void handle_resolve(const errorcode& err,tcp::resolver::iterator endpoint_iterator)
	{
		if (!err)
		{
			try
			{
				// Attempt a connection to the first endpoint in the list. Each endpoint
				// will be tried until we successfully establish a connection.
				tcp::endpoint endpoint = *endpoint_iterator;
				mSocket.async_connect(endpoint,
					boost::bind(&DownloadFile::handle_connect, this,
					boost::asio::placeholders::error, ++endpoint_iterator));
			}		
			catch (std::exception* e)
			{
				LOGCRIT(e->what());
			}
		}
		else
		{
			EngineFuncs::ConsoleError(va("DownloadFile Error: %s",err.message().c_str()));
		}
	}
	void handle_connect(const errorcode& err,tcp::resolver::iterator endpoint_iterator)
	{
		if (!err)
		{
			// The connection was successful. Send the request.
			boost::asio::async_write(mSocket, mRequest,
				boost::bind(&DownloadFile::handle_write_request, this,
				boost::asio::placeholders::error));
		}
		else if (endpoint_iterator != tcp::resolver::iterator())
		{
			// The connection failed. Try the next endpoint in the list.
			mSocket.close();
			tcp::endpoint endpoint = *endpoint_iterator;
			mSocket.async_connect(endpoint,
				boost::bind(&DownloadFile::handle_connect, this,
				boost::asio::placeholders::error, ++endpoint_iterator));
		}
		else
		{
			EngineFuncs::ConsoleError(va("DownloadFile Error: %s",err.message().c_str()));
		}
	}
	void handle_write_request(const errorcode& err)
	{
		if (!err)
		{
			// Read the response status line.
			boost::asio::async_read_until(mSocket, mResponse, "\r\n",
				boost::bind(&DownloadFile::handle_read_status_line, this,
				boost::asio::placeholders::error));
		}
		else
		{
			EngineFuncs::ConsoleError(va("DownloadFile Error: %s",err.message().c_str()));
		}
	}
	void handle_read_status_line(const errorcode& err)
	{
		if (!err)
		{
			// Check that response is OK.
			std::istream response_stream(&mResponse);
			std::string http_version;
			response_stream >> http_version;
			unsigned int status_code;
			response_stream >> status_code;
			std::string status_message;
			std::getline(response_stream, status_message);
			if (!response_stream || http_version.substr(0, 5) != "HTTP/")
			{
				EngineFuncs::ConsoleError("DownloadFile Error: Invalid Response");
				return;
			}

			switch(status_code)
			{
			case QueryFileMatches:
				EngineFuncs::ConsoleError(va("Map up to date %s",mQuery._MapName.c_str()));
				break;
			case QueryBadUrl:
				EngineFuncs::ConsoleError(va("Bad Url requesting map %s",mQuery._MapName.c_str()));
				break;
			case QueryFileNotInDatabase:
				EngineFuncs::ConsoleError(va("File not available for %s",mQuery._MapName.c_str()));
				break;
			case QuerySuccess:
				{
					// Read the response headers, which are terminated by a blank line.
					boost::asio::async_read_until(mSocket, mResponse, "\r\n\r\n",
						boost::bind(&DownloadFile::handle_read_headers, this,
						boost::asio::placeholders::error));
					break;
				}
			default:
				EngineFuncs::ConsoleError(va("DownloadFile Error: Error code %d",status_code));
				break;
			}
		}
		else
		{
			EngineFuncs::ConsoleError(va("DownloadFile Error: %s",err.message().c_str()));
		}
	}
	void handle_read_headers(const errorcode& err)
	{
		if (!err)
		{
			// Process the response headers.
			std::istream response_stream(&mResponse);
			std::string header;
			while(std::getline(response_stream, header) && header != "\r")
			{
				//mError << header << "\n";
			}
			
			// Write whatever content we already have to output.
			if (mResponse.size() > 0)
				mContent << &mResponse;

			// Start reading remaining data until EOF.
			boost::asio::async_read(mSocket, mResponse,
				boost::asio::transfer_at_least(1),
				boost::bind(&DownloadFile::handle_read_content, this,
				boost::asio::placeholders::error));
		}
		else
		{
			EngineFuncs::ConsoleError(va("DownloadFile Error: %s",err.message().c_str()));
		}
	}
	void handle_read_content(const errorcode& err)
	{
		if (!err)
		{
			// Write all of the data that has been read so far.
			mContent << &mResponse;

			// Continue reading remaining data until EOF.
			boost::asio::async_read(mSocket, mResponse,
				boost::asio::transfer_at_least(1),
				boost::bind(&DownloadFile::handle_read_content, this,
				boost::asio::placeholders::error));
		}
		else if (err == boost::asio::error::eof)
		{
			const int size = mContent.str().length();
			if(size > 0)
			{
				String sFilePath = va("user/download/%s.zip",mQuery._MapName.c_str());

				fs::path sFilePathReal = FileSystem::GetRealDir(sFilePath)/sFilePath;
				if(FileSystem::UnMount(sFilePathReal))
					EngineFuncs::ConsoleMessage(va("UnMounted Archive: %s", sFilePath.c_str()));

				File f;
				f.OpenForWrite(sFilePath.c_str(),File::Binary);
				if(f.IsOpen())
				{
					f.Write(mContent.str().c_str(),size);
					f.Close();

					EngineFuncs::ConsoleMessage(va("Downloaded %s, %s : crc 0x%08X", 
						mQuery._MapName.c_str(), 
						Utils::FormatByteString(size).c_str(),
						FileSystem::CalculateCrc(mContent.str().c_str(),size)));
					
					sFilePathReal = FileSystem::GetRealDir(sFilePath)/sFilePath;
					if(FileSystem::Mount(sFilePathReal,"nav",FileSystem::MountFirst))
						EngineFuncs::ConsoleMessage(va("Mounted Archive: %s", sFilePath.c_str()));
				}
				else
				{
					EngineFuncs::ConsoleError(va("Unable to open file for writing: %s.",
						sFilePathReal.string().c_str()));
				}
			}
			else
			{
				EngineFuncs::ConsoleError(va("Map %s not available from database.",mQuery._MapName.c_str()));
			}
		}
		else if (err != boost::asio::error::eof)
		{
			EngineFuncs::ConsoleError(va("DownloadFile Error: %s",err.message().c_str()));
		}
	}
	tcp::resolver			mResolver;
	tcp::socket				mSocket;
	boost::asio::streambuf	mRequest;
	boost::asio::streambuf	mResponse;

	StringStr				mContent;
	FileVars				mQuery;
};

//////////////////////////////////////////////////////////////////////////

String	gGameAbbrev;
int		gNavFileVersion = 0;

bool FileDownloader::Init()
{
	gGameAbbrev = IGameManager::GetInstance()->GetGame()->GetGameDatabaseAbbrev();
	gNavFileVersion = NavigationManager::GetInstance()->GetCurrentPathPlanner()->GetLatestFileVersion();

	FileSystem::MakeDirectory("user/download");

	return true;
}

void FileDownloader::Shutdown()
{
	g_MasterThreadGroup.join_all();
}

bool gReloadNavigation = false;
void FileDownloader::Poll()
{
	if(gReloadNavigation)
	{
		gReloadNavigation = false;
		NavigationManager::GetInstance()->GetCurrentPathPlanner()->Load(false);
		IGameManager::GetInstance()->GetGame()->InitMapScript();
	}
}

struct AsIOThread
{
	void operator()()
	{
#if __cplusplus >= 201103L //karin: using C++11 instead of boost
		typedef std::shared_ptr<DownloadFile> DlFilePtr;
#else
		typedef boost::shared_ptr<DownloadFile> DlFilePtr;
#endif
		typedef std::list<DlFilePtr> DlFileList;

		DlFileList dlList;

		boost::asio::io_service IO_Svc;

		StringList::iterator it = _DownloadList.begin();
		for(; it != _DownloadList.end(); ++it)
		{
			FileVars vars(*it,_Abbrev,_FileVersion);
			dlList.push_back(DlFilePtr(new DownloadFile(IO_Svc, vars)));
		}

		try
		{
			IO_Svc.run();
			dlList.clear();

			if(_LoadWaypoints)
			{
				gReloadNavigation = true;
			}
		}		
		catch (std::exception* e)
		{
			LOGCRIT(e->what());
		}
	}
	AsIOThread(const String &_map, const String &_abbrev, const int _version, bool _load = false)
		: _Abbrev(_abbrev)
		, _FileVersion(_version)
		, _LoadWaypoints(_load)
	{
		_DownloadList.push_back(_map);
	}
	AsIOThread(const StringList &dlList, const String &_abbrev, const int _version, bool _load = false)
		: _DownloadList(dlList)
		, _Abbrev(_abbrev)
		, _FileVersion(_version)
		, _LoadWaypoints(_load)
	{
	}
private:
	StringList	_DownloadList;

	const String	_Abbrev;
	const int		_FileVersion;

	const bool		_LoadWaypoints;

	AsIOThread();
};

//////////////////////////////////////////////////////////////////////////

TryMutex			gDlMapMutex;
StringList			gDlMapList;

bool FileDownloader::NavigationMissing(const String &_name)
{
	bool DownloadMissingNavigation = false;
	Options::GetValue("Downloader","DownloadMissingNav",DownloadMissingNavigation);
	if(DownloadMissingNavigation)
	{
		EngineFuncs::ConsoleError(va("No nav for %s, attempting to download...", _name.c_str()));
		UpdateWaypoints(_name,true);
		return true;
	}
	EngineFuncs::ConsoleError(va("No nav for %s, auto-download disabled.", _name.c_str()));
	return false;
}

void FileDownloader::UpdateWaypoints(const String &_mapname, bool _load)
{
	//g_MasterThreadGroup.create_thread(AsIOThread(_mapname,gGameAbbrev,gNavFileVersion,_load));
}

void FileDownloader::UpdateAllWaypoints(bool _getnew)
{
	StringList maplist;

	DirectoryList navFiles;
	FileSystem::FindAllFiles("nav/", navFiles, ".*.way");
	for(obuint32 i = 0; i < navFiles.size(); ++i)
	{
		const String &mapname = navFiles[i].stem()
#if BOOST_FILESYSTEM_VERSION > 2
			.string()
#endif
		;
		maplist.push_back(mapname);
	}
	g_MasterThreadGroup.create_thread(AsIOThread(maplist,gGameAbbrev,gNavFileVersion));
}

#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
