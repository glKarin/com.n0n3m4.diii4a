#include "HttpServer.h"
#include "microhttpd.h"
#include "Logging.h"
#include "Path.h"
#include "Utils.h"
#include "StdString.h"
#include <algorithm>
#include <string.h>
#include <chrono>
#include <thread>

namespace ZipSync {

//stupid GCC complains about printing uint64_t as "%llu"
typedef unsigned long long uint64;

HttpServer::~HttpServer() {
    Stop();
}
HttpServer::HttpServer() {
    SetBlockSize();
    SetPortNumber();
    SetPauseModel();
    SetDropMultipart();
}

void HttpServer::SetRootDir(const std::string &root) {
    _rootDir = root;
}
void HttpServer::SetPortNumber(int port) {
    _port = port;
}

std::string HttpServer::GetRootUrl() const {
    return "http://localhost:" + std::to_string(_port) + "/";
}

void HttpServer::SetBlockSize(int blockSize) {
    _blockSize = blockSize;
}

void HttpServer::SetPauseModel(const PauseModel &model) {
    _pauseModel = model;
}

void HttpServer::SetDropMultipart(bool drop) {
    _dropMultipart = drop;
}

void HttpServer::CloseSuspendedSocket() {
    if (_suspendedSocket) {
        MHD_socket socket = *(MHD_socket*)_suspendedSocket;
#ifdef _WIN32
        closesocket(socket);
#else
        close(socket);
#endif
        free(_suspendedSocket);
        _suspendedSocket = nullptr;
    }
}

void HttpServer::Stop() {
    if (!_daemon)
        return;
    MHD_stop_daemon(_daemon);
    _daemon = nullptr;
    CloseSuspendedSocket();
}

void HttpServer::Start() {
    if (_daemon)
        return;
    _daemon = MHD_start_daemon(
        MHD_USE_THREAD_PER_CONNECTION,
        _port,
        NULL,
        NULL,
        &MhdFunction,
        this,
        MHD_OPTION_END
    );
    ZipSyncAssertF(_daemon, "Failed to start microhttpd on port %d", _port);
}

void HttpServer::StartButIgnoreConnections() {
    Start();
    MHD_socket socket = MHD_quiesce_daemon(_daemon);
    ZipSyncAssertF(socket != (~0), "Failed to stop daemon listening");
    CloseSuspendedSocket();
    _suspendedSocket = new MHD_socket(socket);
}

struct ThreadState {
    int callCount = 0;
};
thread_local ThreadState threadState;

int HttpServer::MhdFunction(
    void *cls,
    MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **ptr
) {
    //only accept GET requests
    if (0 != strcmp(method, "GET"))
        return MHD_NO;

    //clear thread state and associate it with request
    if (*ptr == NULL)
        threadState = ThreadState();
    *ptr = &threadState;

    threadState.callCount++;
    if (threadState.callCount == 1) {
        //first call only shows headers
        return MHD_YES;
    }

    return ((HttpServer*)cls)->AcceptCallback(connection, url, method, version);
}

class PauseState {
    const HttpServer::PauseModel *_model = nullptr;
    uint64_t _clearTime = 0;

public:
    PauseState(const HttpServer::PauseModel *model) {
        _model = model;
    };
    void Think(uint64_t added) {
        if (!_model)
            return;
        if (_model->pauseSeconds <= 0)
            return;
        _clearTime += added;
        if (_clearTime >= _model->bytesBetweenPauses) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(_model->pauseSeconds * 1000ms);
            _clearTime = 0;
        }
    }
};

class HttpServer::FileDownload {
    StdioFileHolder _file;
    uint64_t _base = 0;
    PauseState _pauseState;
public:
    FileDownload(StdioFileHolder &&file, uint64_t base, const PauseModel *pauseModel)
        : _file(std::move(file)), _base(base), _pauseState(pauseModel)
    {}
    static ssize_t FileReaderCallback(void *cls, uint64_t pos, char *buf, size_t max) {
        auto *down = (FileDownload*)cls;
        fseek(down->_file, down->_base + pos, SEEK_SET);
        size_t readBytes = fread(buf, 1, max, down->_file);
        down->_pauseState.Think(readBytes);
        return readBytes;
    }
    static void FileReaderFinalize(void *cls) {
        auto *down = (FileDownload*)cls;
        delete down;
    }
};

struct ChunkInfo {
    uint64_t responseStart;
    uint64_t size;
    std::string rawData;        //case 1
    uint64_t fileStart;           //case 2
    bool operator< (const ChunkInfo &other) const {
        return responseStart < other.responseStart;
    }
    static ChunkInfo CreateWithData(uint64_t &pos, const std::string &data) {
        ChunkInfo res;
        res.responseStart = pos;
        res.fileStart = UINT64_MAX;
        res.rawData = data;
        res.size = res.rawData.size();
        pos += res.size;
        return res;
    }
    static ChunkInfo CreateAsFileRange(uint64_t &pos, uint64_t rngStart, uint64_t rngLen) {
        ChunkInfo res;
        res.responseStart = pos;
        res.size = rngLen;
        res.fileStart = rngStart;
        pos += res.size;
        return res;
    }
};
class HttpServer::MultipartDownload {
    StdioFileHolder _file;
    uint64_t _fileSize = UINT64_MAX;
    uint64_t _totalContentSize = 0;
    std::string _boundary;   //includes leading EOL
    std::vector<ChunkInfo> _chunks;
    PauseState _pauseState;
public:
    MultipartDownload(
        StdioFileHolder &&file, uint64_t fileSize,
        std::vector<std::pair<uint64_t, uint64_t>> arr,
        const PauseModel *pauseModel
    ) : _file(std::move(file)), _fileSize(fileSize), _pauseState(pauseModel) {
        //note: we do NOT check that boundary does not occur in data
        _boundary = std::string("********") + "72FFC411326F7C93";
        int n = arr.size();
        uint64_t outPos = 0;
        for (int i = 0; i < n; i++) {
            std::string header;
            header += "\r\n";
            header += "--";
            header += _boundary;
            header += "\r\n";
            char buff[64];
            sprintf(buff, "Content-Range: bytes %llu-%llu/%llu\r\n\r\n", uint64(arr[i].first), uint64(arr[i].second), uint64(_fileSize));
            header += buff;
            _chunks.push_back(ChunkInfo::CreateWithData(outPos, header));
            _chunks.push_back(ChunkInfo::CreateAsFileRange(outPos, arr[i].first, arr[i].second - arr[i].first + 1));
        }
        std::string tail = "\r\n";
        tail += "--";
        tail += _boundary;
        tail += "--\r\n";
        _chunks.push_back(ChunkInfo::CreateWithData(outPos, tail));
        _totalContentSize = outPos;
    }
    const char *GetBoundary() const {
        return _boundary.c_str();
    }
    uint64_t GetTotalSize() const {
        return _totalContentSize; 
    }
    void FileReaderCallback(uint64_t pos, size_t &len, char *buf) {
        ChunkInfo aux = {pos};
        int idx = std::upper_bound(_chunks.begin(), _chunks.end(), aux) - _chunks.begin() - 1;
        ZipSyncAssert(idx >= 0 && pos >= _chunks[idx].responseStart);
        const ChunkInfo &chunk = _chunks[idx];
        uint64_t offset = pos - chunk.responseStart;
        uint64_t remains = chunk.size - offset;
        if (len > remains) {
            len = remains;
            ZipSyncAssert(len > 0);
        }
        if (chunk.fileStart != UINT64_MAX) {
            fseek(_file, chunk.fileStart + offset, SEEK_SET);
            len = fread(buf, 1, len, _file);
        }
        else {
            memcpy(buf, chunk.rawData.data() + offset, len);
        }
        _pauseState.Think(len);
    }
    static ssize_t FileReaderCallback(void *cls, uint64_t pos, char *buf, size_t max) {
        auto *down = (MultipartDownload*)cls;
        down->FileReaderCallback(pos, max, buf);
        if (max == 0)
            return ssize_t(-1);
        return max;
    }
    static void FileReaderFinalize(void *cls) {
        auto *down = (MultipartDownload*)cls;
        delete down;
    }
};



#define PAGE_NOT_FOUND "<html><head><title>File not found</title></head><body>File not found</body></html>"
#define PAGE_NOT_SATISFIABLE "<html><head><title>Range error</title></head><body>Range not satisfiable</body></html>"

static int ReturnWithErrorResponse(MHD_Connection *connection, int httpCode, const char *content) {
    MHD_Response *response = MHD_create_response_from_buffer(
        strlen(content),
        (void*)content,
        MHD_RESPMEM_MUST_COPY
    );
    int ret = MHD_queue_response(
        connection,
        httpCode,
        response
    );
    return ret;
}

int HttpServer::AcceptCallback(
    MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version
) const {

    std::string filepath = _rootDir + url;

    StdioFileHolder file(fopen(filepath.c_str(), "rb"));
    if (!file)
        return ReturnWithErrorResponse(connection, MHD_HTTP_NOT_FOUND, PAGE_NOT_FOUND);

    fseek(file, 0, SEEK_END);
    uint64_t fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint64_t flast = fsize - 1; 

    std::vector<std::pair<uint64_t, uint64_t>> ranges;
    if (const char *rangeStr = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Range")) {
        bool bad = false;
        static const char *BYTES_PREFIX = "bytes=";
        for (int i = 0; BYTES_PREFIX[i]; i++)
            if (tolower(BYTES_PREFIX[i]) != tolower(rangeStr[i]))
                bad = true;
        if (bad)
            return ReturnWithErrorResponse(connection, MHD_HTTP_RANGE_NOT_SATISFIABLE, PAGE_NOT_SATISFIABLE);
        std::vector<std::string> segs;
        stdext::split(segs, rangeStr + 6, ",");
        for (const std::string &s : segs) {
            int len = s.size();
            int pos = s.find('-');
            if (pos < 0 || pos == 0) {
                bad = true;
                break;
            }
            std::string fromStr = s.substr(0, pos);
            std::string toStr = s.substr(pos+1);
            uint64 from = 0;
            uint64 to = UINT64_MAX;
            if (sscanf(fromStr.c_str(), "%llu", &from) != 1)
                bad = true;
            if (sscanf(toStr.c_str(), "%llu", &to) != 1)
                to = fsize - 1;
            ranges.emplace_back(from, to);
        }
        if (bad)
            return ReturnWithErrorResponse(connection, MHD_HTTP_RANGE_NOT_SATISFIABLE, PAGE_NOT_SATISFIABLE);
        for (int i = 0; i < ranges.size(); i++) {
            if (i && ranges[i-1].second >= ranges[i].first)
                bad = true;
            if (ranges[i].first > ranges[i].second)
                bad = true;
            if (ranges[i].second >= fsize)
                bad = true;
        }
        if (bad)
            return ReturnWithErrorResponse(connection, MHD_HTTP_RANGE_NOT_SATISFIABLE, PAGE_NOT_SATISFIABLE);
    }

    MHD_Response *response = nullptr;
    int httpCode = MHD_HTTP_OK;
    if (ranges.size() == 0) {
        std::unique_ptr<FileDownload> down(new FileDownload(std::move(file), 0, &_pauseModel));
        response = MHD_create_response_from_callback(fsize, _blockSize, FileDownload::FileReaderCallback, down.release(), FileDownload::FileReaderFinalize);
    }
    else if (ranges.size() == 1) {
        std::unique_ptr<FileDownload> down(new FileDownload(std::move(file), ranges[0].first, &_pauseModel));
        response = MHD_create_response_from_callback(ranges[0].second - ranges[0].first + 1, _blockSize, FileDownload::FileReaderCallback, down.release(), FileDownload::FileReaderFinalize);
        httpCode = MHD_HTTP_PARTIAL_CONTENT;
        char buff[64];
        sprintf(buff, "bytes %llu-%llu/%llu", uint64(ranges[0].first), uint64(ranges[0].second), uint64(fsize));
        MHD_add_response_header(response, "Content-Range", buff);
    }
    else if (!_dropMultipart) {
        std::unique_ptr<MultipartDownload> down(new MultipartDownload(std::move(file), fsize, ranges, &_pauseModel));
        uint64_t totalContentSize = down->GetTotalSize();
        char buff[64];
        sprintf(buff, "multipart/byteranges; boundary=%s", down->GetBoundary());
        response = MHD_create_response_from_callback(totalContentSize, _blockSize, MultipartDownload::FileReaderCallback, down.release(), MultipartDownload::FileReaderFinalize);
        httpCode = MHD_HTTP_PARTIAL_CONTENT;
        MHD_add_response_header(response, "Content-Type", buff);
    }
    if (!response)
        return MHD_NO;

    int ret = MHD_queue_response(
        connection,
        httpCode,
        response
    );
    MHD_destroy_response(response);

    return ret;
}

}
