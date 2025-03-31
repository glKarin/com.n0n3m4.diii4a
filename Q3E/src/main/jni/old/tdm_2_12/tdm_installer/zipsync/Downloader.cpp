#include "Downloader.h"
#include <curl/curl.h>
#include <algorithm>
#include "Logging.h"
#include "StdString.h"
#include <string.h>
#undef min
#undef max

struct SpeedProfile {
    //try to avoid CURL requests of total size less than this
    int maxRequestSize;
    //forbid multipart requests with more that this number of chunks
    int maxPartsPerRequest;
    //drop request after X seconds of very slow download (CURLOPT_LOW_SPEED_TIME)
    int lowSpeedTime;
    //stop trying to connect after X seconds (CURLOPT_CONNECTTIMEOUT)
    int connectTimeout;
};

//if request fails due to timeout, we retry it with progressively softer limits
static const SpeedProfile SPEED_PROFILES[] = {
    {10<<20, 20, 10, 10},
    {512<<10, 1, 10, 10},
    {32<<10, 1, 10, 30},
    {4<<10, 1, 10, 30},
    {4<<10, 1, 60, 60}
};
static const int SPEED_PROFILES_NUM = sizeof(SPEED_PROFILES) / sizeof(SPEED_PROFILES[0]);

//download is slower than X bytes per second => halt as too slow (CURLOPT_LOW_SPEED_LIMIT)
static const int LOW_SPEED_LIMIT = 1000;

//overhead per download in bytes --- for progress callback only
static const int ESTIMATED_DOWNLOAD_OVERHEAD = 100;

namespace ZipSync {

//note: HTTP header field names are case-insensitive
//however, we cannot just lowercase all headers, since multipart boundary is case-sensitive
const char *CheckHttpPrefix(const std::string &line, const std::string &prefix) {
    if (!stdext::istarts_with(line, prefix))
        return nullptr;
    const char *rest = line.c_str() + prefix.size();
    return rest;
}

DownloadSource::DownloadSource() { byterange[0] = byterange[1] = 0; }
DownloadSource::DownloadSource(const std::string &url) : url(url) { byterange[0] = 0; byterange[1] = UINT32_MAX; }
DownloadSource::DownloadSource(const std::string &url, uint32_t from, uint32_t to) : url(url) { byterange[0] = from; byterange[1] = to; }


Downloader::~Downloader() {}
Downloader::Downloader() : _curlHandle(nullptr, curl_easy_cleanup) {}

void Downloader::EnqueueDownload(const DownloadSource &source, const DownloadFinishedCallback &finishedCallback) {
    Download down;
    down.src = source;
    down.finishedCallback = finishedCallback;

    //note: we save our initial estimates here and use it throughout the whole run
    //even though we will detect file size for whole-file downloads later, we still use initial estimates for computing progress
    down.progressSize = down.src.byterange[1] - down.src.byterange[0];
    if (down.src.byterange[1] == UINT_MAX)
        down.progressSize = (1<<20);    //rough estimate of unknown size
    down.progressSize += ESTIMATED_DOWNLOAD_OVERHEAD;

    _downloads.push_back(down);
}

void Downloader::SetProgressCallback(const GlobalProgressCallback &progressCallback) {
    _progressCallback = progressCallback;
}
void Downloader::SetErrorMode(bool silent) {
    _silentErrors = silent;
}
void Downloader::SetUserAgent(const char *useragent) {
    if (useragent)
        _useragent.reset(new std::string(useragent));
    else
        _useragent.reset();
}

void Downloader::SetMultipartBlocked(bool blocked) {
    _blockMultipart = blocked;
}

void Downloader::DownloadAll() {
    if (_progressCallback)
        _progressCallback(0.0, "Downloading started");

    _curlHandle.reset(curl_easy_init());

    //distribute downloads across remote files / urls
    for (int i = 0; i <  _downloads.size(); i++)
        _urlStates[_downloads[i].src.url].downloadsIds.push_back(i);
    for (auto &pKV : _urlStates) {
        std::string url = pKV.first;
        std::vector<int> &ids = pKV.second.downloadsIds;
        std::sort(ids.begin(), ids.end(), [this](int a, int b) {
           return _downloads[a].src.byterange[0] < _downloads[b].src.byterange[0];
        });
    }

    //go over remote files and process them one by one
    for (const auto &pKV : _urlStates) {
        std::string url = pKV.first;
        try {
            DownloadAllForUrl(url);
        }
        catch(const ErrorException &e) {
            if (!_silentErrors)
                throw e;    //rethrow further to caller
            else
                {}          //supress exception, continue with other urls
        }
    }

    ZipSyncAssert(_curlHandle.get_deleter() == curl_easy_cleanup);
    _curlHandle.reset();

    if (_progressCallback)
        _progressCallback(1.0, "Downloading finished");
}

void Downloader::DownloadAllForUrl(const std::string &url) {
    UrlState &state = _urlStates.find(url)->second;
    int n = state.downloadsIds.size();

    //used to occasionally restore faster speed profiles
    int64_t speedLastFailedAt[SPEED_PROFILES_NUM];
    memset(speedLastFailedAt, -1, sizeof(speedLastFailedAt));

    while (state.doneCnt < n) {
        //select speed profile
        ZipSyncAssertF(state.speedProfile < SPEED_PROFILES_NUM, "Repeated timeout on URL %s", url.c_str());
        SpeedProfile profile = SPEED_PROFILES[state.speedProfile];
        if (_blockMultipart)
            profile.maxPartsPerRequest = 1;

        std::vector<SubTask> subtasks;  //set of chunks scheduled as one request
        uint64_t totalSize = 0;         //total number of bytes scheduled into request
        int rangesCnt = 0;              //number of separate byteranges scheduled
        uint32_t last = UINT32_MAX;     //end of the last byterange

        int end = state.doneCnt;
        //grab a few next downloads for the next HTTP request
        while (end < n) {
            //what if we add the whole next download? (or what remains of it)
            int idx = state.downloadsIds[end];
            const Download &down = _downloads[idx];
            uint32_t downStart = down.src.byterange[0] + (subtasks.empty() ? state.doneBytesNext : 0);
            uint32_t downEnd = down.src.byterange[1];

            //estimate quantities if we add this download
            uint64_t newTotalSize = totalSize + (downEnd - downStart);
            int newRangesCnt = rangesCnt + (last != downStart);

            //stop before this download if it exceeds ranges limit
            if (newRangesCnt > profile.maxPartsPerRequest)
                break;
            //does it exceed size limit?
            if (newTotalSize > profile.maxRequestSize) {
                if (subtasks.size() > 0) {
                    //we have added at least one download already,
                    //don't take a new one with size limit overflow
                    break;
                }
                if (downEnd != UINT32_MAX) {
                    //this download is larger than limit: split it and download only a part of it
                    SubTask st = {idx, {downStart, downStart + profile.maxRequestSize}};
                    subtasks.push_back(st);
                    break;
                }
                //single request with unknown size: never split...
                //note that we will soon discover its size from HTTP headers
                //so if timeout happens, then we will be able to split it on retry
            }

            //no limit exceeded -> add this full download to scheduled request
            end++;
            SubTask st = {idx, {downStart, downEnd}};
            subtasks.push_back(st);

            //update stats for limit checks on next iterations
            last = downEnd;
            totalSize = newTotalSize;
            rangesCnt = newRangesCnt;
        }

        //perform the HTTP request
        bool ok = DownloadOneRequest(url, subtasks, profile.lowSpeedTime, profile.connectTimeout);

        if (ok) {
            //update number of fully finished downloads
            state.doneCnt = end;
            //update progress in the next download
            if (end < state.downloadsIds.size() && subtasks.back().downloadIdx == state.downloadsIds[end]) {
                //partly finished
                int idx = subtasks.back().downloadIdx;
                state.doneBytesNext = subtasks.back().byterange[1] - _downloads[idx].src.byterange[0];
            }
            else {
                //fully finished
                state.doneBytesNext = 0;
            }
            //reset speed profile
            for (int i = 0; i < state.speedProfile; i++)
                if (speedLastFailedAt[i] < 0 || _totalBytesDownloaded - speedLastFailedAt[i] > SPEED_PROFILES[i].maxRequestSize) {
                    //last time when we failed with this profile was long time ago
                    //so let's try this speed again, maybe it will work now
                    state.speedProfile = i;
                    break;
                }
        }
        else {
            //soft fail: retry with less strict limits
            speedLastFailedAt[state.speedProfile] = _totalBytesDownloaded;
            state.speedProfile++;
        }
    }
}

bool Downloader::DownloadOneRequest(const std::string &url, const std::vector<SubTask> &subtasks, int lowSpeedTime, int connectTimeout) {
    if (subtasks.empty())
        return true;    //scheduling algorithm should never even create such requests...

    //generate byterange string with all adjacent chunks merged
    std::vector<std::pair<uint32_t, uint32_t>> coaslescedRanges;
    for (const SubTask &st : subtasks) {
        if (!coaslescedRanges.empty() && coaslescedRanges.back().second >= st.byterange[0])
            coaslescedRanges.back().second = std::max(coaslescedRanges.back().second, st.byterange[1]);
        else
            coaslescedRanges.push_back(std::make_pair(st.byterange[0], st.byterange[1]));
    }
    std::string byterangeStr;
    for (auto rng : coaslescedRanges) {
        if (!byterangeStr.empty())
            byterangeStr += ",";
        byterangeStr += std::to_string(rng.first) + "-";
        if (rng.second != UINT32_MAX)   //it means "up to the end"
            byterangeStr += std::to_string(rng.second - 1);
    }

    //compute "progressWeight": which portion of the whole job this particular request is?
    int64_t totalEstimate = 0;
    int64_t thisEstimate = 0;
    for (const SubTask &st : subtasks) {
        const Download &down = _downloads[st.downloadIdx];
        //subtask includes [from..to) chunk of [0..full) range
        int64_t from = st.byterange[0] - down.src.byterange[0];
        int64_t to = std::min(st.byterange[1], down.src.byterange[1]) - down.src.byterange[0];
        int64_t full = down.src.byterange[1] - down.src.byterange[0];
        //such way guarantees that sum of "thisEstimate" over all chunks of download
        //will be exactly equal to "full", regardless of how download was split
        thisEstimate += to * down.progressSize / full - from * down.progressSize / full;
    }
    for (const auto &down : _downloads)
        totalEstimate += down.progressSize;
    double progressWeight = double(thisEstimate) / totalEstimate;

//------------------- CURL callbacks: begin -------------------
    auto header_callback = [](char *buffer, size_t size, size_t nitems, void *userdata) {
        size *= nitems;
        auto &resp = *((Downloader*)userdata)->_currResponse;
        std::string str(buffer, buffer + size);
        size_t from, to, all;
        if (const char *tail = CheckHttpPrefix(str, "Content-Range: bytes ")) {
            //this is an ordinary byterange response
            if (sscanf(tail, "%zu-%zu/%zu", &from, &to, &all) == 3) {
                //memorize which byterange is actually returned by server
                resp.onerange[0] = from;
                resp.onerange[1] = to + 1;
                //memorize size of the file (which we don't know initially for whole-file downloads)
                resp.totalSize = all;
            }
        }
        char boundary[128] = {0};
        if (const char *tail = CheckHttpPrefix(str, "Content-Type: multipart/byteranges; boundary=")) {
            //this is a multipart byterange request
            //we will have to manually parse response content later
            if (sscanf(tail, "%s", boundary) == 1) {
                //memorize boundary between parts
                resp.boundary = std::string("\r\n--") + boundary;// + "\r\n";
            }
        }
        return size;
    };
    auto write_callback = [](char *buffer, size_t size, size_t nitems, void *userdata) -> size_t {
        size *= nitems;
        auto &resp = *((Downloader*)userdata)->_currResponse;
        if (resp.onerange[0] == resp.onerange[1] && resp.boundary.empty())
            return 0;  //neither range nor multipart response -> stop
        resp.data.insert(resp.data.end(), buffer, buffer + size);
        return size;
    };
    auto xferinfo_callback = [](void *userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
        auto &resp = *((Downloader*)userdata)->_currResponse;
        if (dltotal > 0 && dlnow > 0) {
            resp.progressRatio = double(dlnow) / std::max(dltotal, dlnow);
            resp.bytesDownloaded = dlnow;
            if (int code = ((Downloader*)userdata)->UpdateProgress())
                return code;   //interrupt!
        }
        return 0;
    };
//-------------------- CURL callbacks: end --------------------

    //prepare temporary structure for response
    _currResponse.reset(new CurlResponse());
    _currResponse->url = url;
    _currResponse->progressWeight = progressWeight;

    //set up CURL request
    CURL *curl = _curlHandle.get();
    std::string reprocmd = "curl";
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    reprocmd += formatMessage(" %s", url.c_str());
    curl_easy_setopt(curl, CURLOPT_RANGE, byterangeStr.c_str());
    reprocmd += formatMessage(" -r %s", byterangeStr.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (curl_write_callback)write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, (curl_write_callback)header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, (curl_xferinfo_callback)xferinfo_callback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, LOW_SPEED_LIMIT);
    reprocmd += formatMessage(" -Y %d", LOW_SPEED_LIMIT);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, lowSpeedTime);
    reprocmd += formatMessage(" -y %d", lowSpeedTime);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connectTimeout);
    reprocmd += formatMessage(" --connect-timeout %d", connectTimeout);
    if (_useragent) {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, _useragent->c_str());
        reprocmd += formatMessage(" -A \"%s\"", _useragent->c_str());
    }
    //log request as CURL command
    //it can be used to save and reproduce the problematic request with curl executable
    int reqIdx = _curlRequestIdx++;
    reprocmd += formatMessage(" -o out%d.bin", reqIdx);
    g_logger->debugf("[curl-cmd] %s", reprocmd.c_str());
    //notify user that we start downloading from this URL
    UpdateProgress();

    //perform the request
    CURLcode ret = curl_easy_perform(curl);
    long httpRes = 0;
    curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &httpRes);

    //handle return/error codes
    if (_currResponse->totalSize != UINT_MAX && _downloads[subtasks.front().downloadIdx].src.byterange[1] == UINT_MAX) {
        //even if we have failed, now we know the size of this file (thanks to HTTP header)
        _downloads[subtasks.front().downloadIdx].src.byterange[1] = _currResponse->totalSize;
    }
    if (ret != 0 || (httpRes != 200 && httpRes != 206)) {
        //log down atypical error codes
        g_logger->debugf("[curl-res] ret:%d http:%d", ret, httpRes);
    }
    if (ret == CURLE_ABORTED_BY_CALLBACK) {
        //logger error must throw exception, which stops whole job and returns control back to caller
        g_logger->errorf(lcUserInterrupt, "Interrupted by user");
    }
    if (ret == CURLE_OPERATION_TIMEDOUT) {
        //all kind of timeouts get here
        //they happen all the time on problematic networks,
        //so we should retry this request again (or maybe a smaller piece of it)
        g_logger->warningf(lcDownloadTooSlow,
            "Timeout for request with %d segments of total size %lld on URL %s",
            int(subtasks.size()), thisEstimate, url.c_str()
        );
        return false;   //soft fail: retry is welcome
    }
    //handle a few more errors with clear reasons
    ZipSyncAssertF(httpRes != 404, "Not found result for URL %s", url.c_str());
    ZipSyncAssertF(ret != CURLE_WRITE_ERROR, "Response without byteranges for URL %s", url.c_str());
    //handle all the unexpected errors
    ZipSyncAssertF(ret == CURLE_OK, "Unexpected CURL error %d on URL %s", ret, url.c_str());
    ZipSyncAssertF(httpRes == 200 || httpRes == 206, "Unexpected HTTP return code %d for URL %s", httpRes, url.c_str());

    //update progress indicator given that whole request is done
    _currResponse->progressRatio = 1.0;
    UpdateProgress();
    _totalBytesDownloaded += _currResponse->bytesDownloaded;
    _totalProgress += _currResponse->progressWeight;

    //parse multipart response, producing many single-range responses instead
    std::vector<CurlResponse> results;
    if (_currResponse->boundary.empty())
        results.push_back(std::move(*_currResponse));
    else
        BreakMultipartResponse(*_currResponse, results);

    //we have already pulled out all we need from this structure, break it down
    _currResponse.reset();

    std::sort(results.begin(), results.end(), [](const CurlResponse &a, const CurlResponse &b) {
        return a.onerange[0] < b.onerange[0];
    });

    //handle downloaded data: fire download callbacks, append data for partial subtasks
    for (const SubTask &st : subtasks) {
        int idx = st.downloadIdx;
        const auto &downSrc = _downloads[idx].src;
        std::vector<uint8_t> &answer = _downloads[idx].resultData;

        //find all pieces in the downloaded results which are about this subtask
        for (const auto &resp : results) {
            //intersect byterange intervals of the subtask and response (remaining part of it)
            uint32_t currPos = downSrc.byterange[0] + (uint32_t)answer.size();
            uint32_t left = std::max(currPos, resp.onerange[0]);
            uint32_t right = std::min(downSrc.byterange[1], resp.onerange[1]);
            if (right <= left)
                continue;   //no intersection

            ZipSyncAssertF(left == currPos, "Missing chunk %u..%u (%u bytes) after downloading URL %s", left, currPos, currPos - left, url.c_str());
            //take data from response in the intersection range
            answer.insert(answer.end(),
                resp.data.data() + (left - resp.onerange[0]),
                resp.data.data() + (right - resp.onerange[0])
            );
        }

        //note: st.byterange[1] may be UINT_MAX for whole-file downloads
        if (st.byterange[1] >= downSrc.byterange[1]) {
            //we have just appended the very last bits of this download
            if (downSrc.byterange[1] != UINT32_MAX) {
                uint32_t totalSize = downSrc.byterange[1] - downSrc.byterange[0];
                ZipSyncAssertF(answer.size() == totalSize, "Missing end chunk %zu..%u (%u bytes) after downloading URL %s", answer.size(), totalSize, totalSize - (uint32_t)answer.size(), url.c_str());
            }
            //pass full data to user via callback
            _downloads[idx].finishedCallback(answer.data(), answer.size());
            //drop the data from memory (to avoid using gigabytes of virtual memory)
            answer.clear();
            answer.shrink_to_fit();
        }
    }

    return true;
}

void Downloader::BreakMultipartResponse(const CurlResponse &response, std::vector<CurlResponse> &parts) {
    const auto &data = response.data;
    const std::string &bound = response.boundary;

    //find all occurences of boundary
    std::vector<size_t> boundaryPos;
    for (size_t pos = 0; pos + bound.size() <= data.size(); pos++)
        if (memcmp(&data[pos], &bound[0], bound.size()) == 0)
            boundaryPos.push_back(pos);

    for (size_t i = 0; i+1 < boundaryPos.size(); i++) {
        size_t left = boundaryPos[i] + bound.size() + 2;        //+2 for "\r\n" or "--"
        size_t right = boundaryPos[i+1];

        //parse header into sequence of lines
        std::vector<std::string> header;
        size_t lineStart = left;
        while (1) {
            size_t lineEnd = lineStart;
            while (strncmp((char*)&data[lineEnd], "\r\n", 2))
                lineEnd++;
            header.emplace_back((char*)&data[lineStart], (char*)&data[lineEnd]);
            lineStart = lineEnd + 2;
            if (header.back().empty())
                break;  //empty line: header has ended
        }

        //find range in headers
        CurlResponse part;
        for (const auto &h : header) {
            size_t from, to, all;
            if (const char *tail = CheckHttpPrefix(h, "Content-Range: bytes ")) {
                if (sscanf(tail, "%zu-%zu/%zu", &from, &to, &all) == 3) {
                    part.onerange[0] = from;
                    part.onerange[1] = to + 1;
                }
            }
        }
        ZipSyncAssertF(part.onerange[0] != part.onerange[1], "Failed to find range in part headers");

        part.data.assign(&data[lineStart], &data[right]);
        parts.push_back(std::move(part));
    }
}

int Downloader::UpdateProgress() {
    char buffer[256] = "Downloading...";
    double progress = _totalProgress;
    if (_currResponse) {
        sprintf(buffer, "Downloading \"%s\"...", _currResponse->url.c_str());
        progress += _currResponse->progressWeight * _currResponse->progressRatio;
    }
    if (_progressCallback) {
        int code = _progressCallback(progress, buffer);
        return code;
    }
    return 0;
}

}
