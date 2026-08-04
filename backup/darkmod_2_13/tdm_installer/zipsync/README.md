# zipsync

This library allows to update a set of zips in local directory to match the set of zips stored on remote HTTP servers.
In order to reduce amount of data downloaded, zipsync uses multipart byterange requests from HTTP 1.1 to download individual files from remote zip archives.
Note that every file inside a zip archive takes contiguous segment of bytes, which makes such partial downloads possible.
In order to detect which files must be downloaded, remote set of zips must have a "manifest", storing attributes and BLAKE2 hash of every file.
The only steps necessary to share a set of zips by zipsync are 1) creating a manifest and 2) performing some normalization of zips.
If several versions of the set are shared, then it is possible to store all versions except one as differentials, reducing storage requirements on server.

The system was originally designed and implemented for [TheDarkMod](www.thedarkmod.com) game, but contains no code specific to it.
The game is based on Doom 3 engine, which stores all its assets in zips archives. As of now, TDM contains more than 20000 files of total size 3.5 GB.
The zipsync library allows to publish frequent beta and developer versions without wasting 3 GB per version both on server storage and download bandwidth.

The library is fully included into TheDarkMod source code repository as subdirectory.
