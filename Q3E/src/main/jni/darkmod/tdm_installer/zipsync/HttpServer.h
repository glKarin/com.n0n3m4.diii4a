#pragma once

#include <string>
#include <stdint.h>

struct MHD_Daemon;
struct MHD_Connection;
enum MHD_Result;

//workaround for ssize_t-included errors on MSVC
#ifdef _MSC_VER
    #define _SSIZE_T_DEFINED
    #define ssize_t size_t
#endif

namespace ZipSync {

/**
 * Simple embedded HTTP server.
 * Used only for tests.
 */
class HttpServer {
public:
    struct PauseModel {
        //make pause every B bytes
        uint64_t bytesBetweenPauses = UINT64_MAX;
        //pause lasts for T seconds
        int pauseSeconds = 0;

        PauseModel();
        PauseModel(uint64_t bbp, int ps);
    };

private:
    std::string _rootDir;
    MHD_Daemon *_daemon = nullptr;
    void *_suspendedSocket = nullptr;       //only for StartButIgnoreConnections
    int _port = -1;
    int _blockSize = -1;
    bool _dropMultipart = false;
    PauseModel _pauseModel;

public:
    static const int PORT_DEFAULT = 8090;

    HttpServer();
    ~HttpServer();
    HttpServer(const HttpServer &) = delete;
    HttpServer& operator=(const HttpServer &) = delete;

    //set the root directory so serve files inside
    void SetRootDir(const std::string &root);
    void SetPortNumber(int port = PORT_DEFAULT);
    void SetBlockSize(int blockSize = 128*1024);
    void SetDropMultipart(bool drop = false);
    void SetPauseModel(const PauseModel &model = PauseModel());
    std::string GetRootUrl() const;

    void Start();
    void Stop();
    void StartButIgnoreConnections();   //for testing connection timeout

private:
    class FileDownload;
    class MultipartDownload;

    static MHD_Result MhdFunction(
        void *cls,
        MHD_Connection *connection,
        const char *url,
        const char *method,
        const char *version,
        const char *upload_data,
        size_t *upload_data_size,
        void **ptr
    );
    MHD_Result AcceptCallback(
        MHD_Connection *connection,
        const char *url,
        const char *method,
        const char *version
    ) const;

    void CloseSuspendedSocket();
};

}
