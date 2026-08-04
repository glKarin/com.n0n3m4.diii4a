#pragma once


namespace ZipSync {

/**
 * Represents the local updater cache (physically present near installation).
 * Usually it contains:
 *   1. old files which are no longer used, but may be helpful to return back to previous version
 *   2. provided manifest for all the files in from p.1
 *   3. various manifests from remote places --- to avoid downloading them again
 */
class LocalCache {
    //TODO
};

}
