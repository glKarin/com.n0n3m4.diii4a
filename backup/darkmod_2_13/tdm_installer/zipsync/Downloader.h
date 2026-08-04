#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <limits.h>


typedef void CURL;

namespace ZipSync {

/**
 * A chunk of data which we want to download
 */
struct DownloadSource {
    //URL to download file from
    std::string url;
    //the range of bytes to be downloaded
    //byterange[1] == UINT_MAX means: download whole file of unknown size
    uint32_t byterange[2];

    DownloadSource();
    DownloadSource(const std::string &url); //download whole file
    DownloadSource(const std::string &url, uint32_t from, uint32_t to); //download range of file
};

//called when download is complete
typedef std::function<void(const void*, uint32_t)> DownloadFinishedCallback;
//called during download to report progress: returning nonzero value interrupts download
typedef std::function<int(double, const char*)> GlobalProgressCallback;


/**
 * Smart downloader over HTTP protocol.
 * Utilizes byteranges and multipart byteranges requests to download many chunks quickly.
 * On a problematic network, can split download into many small pieces to cope with occasional timeouts.
 */
class Downloader {
    bool _silentErrors = false;
    std::unique_ptr<std::string> _useragent;
    bool _blockMultipart = false;
    GlobalProgressCallback _progressCallback;

    //user-specified chunk of data to be downloaded
    struct Download {
        DownloadSource src;
        DownloadFinishedCallback finishedCallback;
        std::vector<uint8_t> resultData;    //temporary storage (used in case download is split)
        int64_t progressSize = 0;           //estimated size in bytes (for progress indicator)
    };
    std::vector<Download> _downloads;

    //state of one remote file processed
    //usually contains several user-specified "Download"-s
    struct UrlState {
        std::vector<int> downloadsIds;      //indices in _downloads (sorted by starting offset)
        int doneCnt = 0;                    //how many FULL downloads done
        uint32_t doneBytesNext = 0;         //how many bytes done in the current download
        int speedProfile = 0;               //index in SPEED_PROFILES
    };
    std::map<std::string, UrlState> _urlStates;

    //designates user-specified "Download" or a piece of it
    //every HTTP request contains one or several SubTasks
    struct SubTask {
        int downloadIdx;                    //index in _downloads
        uint32_t byterange[2];              //can be part of download's byterange
    };
    //state of the HTTP request currently active
    struct CurlResponse {
        std::string url;

        std::vector<uint8_t> data;          //downloaded file data is appended to here
        uint32_t totalSize = UINT_MAX;      //size of file as reported by HTTP header (used for whole-file downloads)
        uint32_t onerange[2] = {UINT_MAX, UINT_MAX};    //byterange actually provided by HTTP server (may differ from what we asked for)
        std::string boundary;               //boundary between responses in multipart response

        double progressRatio = 0.0;         //which portion of this CURL request is done
        int64_t bytesDownloaded = 0;        //how many bytes actually downloaded (as reported by CURL)
        double progressWeight = 0.0;        //this request size / total size of all downloads
    };
    std::unique_ptr<CurlResponse> _currResponse;

    double _totalProgress = 0.0;            //which portion of DownloadAll is complete (without current request)
    int64_t _totalBytesDownloaded = 0;      //how many bytes downloaded in total (without current request)

    std::unique_ptr<CURL, void (*)(CURL*)> _curlHandle; //CURL handle reused between request in order to exploit connection pool
    int _curlRequestIdx = 0;                //sequental number of HTTP request (used for logging curl commands)

public:
    ~Downloader();
    Downloader();

    //schedule download of specified chunk of data
    //the obtained data will be passed to the specified callback when it is available
    void EnqueueDownload(const DownloadSource &source, const DownloadFinishedCallback &finishedCallback);

    //progress callback is useful for two things:
    // * showing progress indicator to user (use passed argument)
    // * allowing user to interrupt download (return nonzero value)
    void SetProgressCallback(const GlobalProgressCallback &progressCallback);
    // * silent == false: throw exception on any error up to the caller, stopping the whole run
    // * silent == true: just don't call callback function for failed requests (grouped by url), no stopping, no exception
    void SetErrorMode(bool silent);
    //set user-agent to be put into HTTP request
    void SetUserAgent(const char *useragent);
    //blocked = true: only use ordinary byterange requests, never use multipart ones
    void SetMultipartBlocked(bool blocked);

    //when everything is set up, call this method to actually perform all downloads
    //it blocks until the job is done (progress callback is the only way to interrupt it)
    void DownloadAll();

    //returns total number of bytes downloaded by this object
    //(as reported by CURL)
    int64_t TotalBytesDownloaded() const { return _totalBytesDownloaded; }

private:
    void DownloadAllForUrl(const std::string &url);
    bool DownloadOneRequest(const std::string &url, const std::vector<SubTask> &subtasks, int lowSpeedTime, int connectTimeout);
    void BreakMultipartResponse(const CurlResponse &response, std::vector<CurlResponse> &parts);
    int UpdateProgress();
};

}
