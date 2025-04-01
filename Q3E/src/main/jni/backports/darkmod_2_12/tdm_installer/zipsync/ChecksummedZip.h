#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include "Hash.h"


namespace ZipSync {

extern const char *CHECKSUMMED_HASH_FILENAME;
extern const char *CHECKSUMMED_HASH_PREFIX;

class Downloader;

//saves zip archive at zipPath with single file named dataFilename and containing [data..data+size] bytes
//it is prepended with small txt file containing hash of the data
void WriteChecksummedZip(const char *zipPath, const void *data, uint32_t size, const char *dataFilename);

//loads zip archive at zipPath with single file named dataFilename, checks hash
//it must be previously saved by WriteChecksummedZip
std::vector<uint8_t> ReadChecksummedZip(const char *zipPath, const char *dataFilename);

//reads only the hash from archive at zipPath
HashDigest GetHashOfChecksummedZip(const char *zipPath);

//gets hashes of the zips at specified URLs
//downloader must be default-constructed object --- it will be used for download
//note: zero hash digest is returned for a remote file without embedded hash
std::vector<HashDigest> GetHashesOfRemoteChecksummedZips(Downloader &downloader, const std::vector<std::string> &urls);

//downloads a set of zips at urls[i], using information about hashes and available cache of zips
//remoteHashes[i] must be hash extracted from urls[i], or null if hash is not availble (returned by GetHashesOfRemoteChecksummedZips)
//cachedZipPaths are paths to locally existing zips: if this set already contains same zip, then remote download is optimized away
//outputPaths[i] is path to which thne zip downloaded from urls[i] will be saved; it can be modified to cachedZipPaths[j] if optimized away
//downloader must be default-constructed object --- it will be used for download
std::vector<int> DownloadChecksummedZips(Downloader &downloader,
    const std::vector<std::string> &urls, const std::vector<HashDigest> &remoteHashes,
    const std::vector<std::string> &cachedZipPaths,
    std::vector<std::string> &outputPaths
);

}
