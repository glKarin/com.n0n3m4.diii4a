#include "mpg123.h"
#include <jni.h>
#include <fcntl.h>
#include <android/log.h>
#include <string.h>
#include "android/mpg123.h"
#include "me_rosuh_libmpg123_MPG123.h"

typedef struct _MP3File {
    mpg123_handle *handle;
    int channels;
    long rate;
    long num_samples;
    int samples_per_frame;
    double secs_per_frame;
    long num_frames;
    float duration;
    size_t buffer_size;
    unsigned char *buffer;
    size_t leftSamples;
    size_t offset;
} MP3File;


void printerr(const char *message, int err) {
    __android_log_print(ANDROID_LOG_ERROR, "podax-jni", "error - %s: %d %s", message, err,
                        mpg123_plain_strerror(err));
}

MP3File *mp3file_init(mpg123_handle *handle) {
    MP3File *mp3file = malloc(sizeof(MP3File));
    memset(mp3file, 0, sizeof(MP3File));
    mp3file->handle = handle;
    return mp3file;
}

void mp3file_delete(MP3File *mp3file) {
    mpg123_close(mp3file->handle);
    mpg123_delete(mp3file->handle);
    free(mp3file->buffer);
    free(mp3file);
}

int mp3file_determineStats(MP3File *mp3) {
    int err = -1;
    if (mp3 == NULL)
        return err;

    int encoding;
    mpg123_handle *mh = mp3->handle;
    err = mpg123_getformat(mh, &mp3->rate, &mp3->channels, &encoding);
    if (err == MPG123_NEED_MORE)
        return err;
    if (err != MPG123_OK) {
        printerr("mpg123_getformat", err);
        return err;
    }

    mpg123_format_none(mh);
    mpg123_format(mh, mp3->rate, mp3->channels, encoding);

    mp3->num_samples = mpg123_length(mh);
    mp3->samples_per_frame = mpg123_spf(mh);
    mp3->secs_per_frame = mpg123_tpf(mh);

    if (mp3->num_samples == MPG123_ERR || mp3->samples_per_frame < 0)
        mp3->num_frames = 0;
    else
        mp3->num_frames = mp3->num_samples / mp3->samples_per_frame;

    if (mp3->num_samples == MPG123_ERR || mp3->samples_per_frame < 0 || mp3->secs_per_frame < 0)
        mp3->duration = 0;
    else
        mp3->duration = mp3->num_samples / mp3->samples_per_frame * mp3->secs_per_frame;
    return err;
}

JNIEXPORT jint JNICALL Java_me_rosuh_libmpg123_MPG123_init
        (JNIEnv *env, jclass c) {
    return mpg123_init();
}

/*
JNIEXPORT jlong JNICALL Java_me_rosuh_libmpg123_MPG123_new
	(JNIEnv *env, jclass c)
{
	int err;
	return (long)mpg123_new(NULL, &err);
}
*/

JNIEXPORT jstring JNICALL Java_me_rosuh_libmpg123_MPG123_getErrorMessage
        (JNIEnv *env, jclass c, jint error) {
    return (*env)->NewStringUTF(env, mpg123_plain_strerror(error));
}

JNIEXPORT jlong JNICALL Java_me_rosuh_libmpg123_MPG123_openStream
        (JNIEnv *env, jclass c) {
    // init mpg123 handle
    int err = MPG123_OK;
    mpg123_handle *mh = mpg123_new(NULL, &err);
    if (err != MPG123_OK) {
        __android_log_print(ANDROID_LOG_INFO, "podax-jni", "mpg123_new error: %s",
                            mpg123_plain_strerror(err));
        return 0;
    }

    // set handle up as stream
    err = mpg123_open_feed(mh);
    if (err != MPG123_OK) {
        __android_log_print(ANDROID_LOG_INFO, "podax-jni", "mpg123_open_feed error: %s",
                            mpg123_plain_strerror(err));
        return 0;
    }

    MP3File *stream = mp3file_init(mh);
    return (jlong) stream;
}

JNIEXPORT void JNICALL Java_me_rosuh_libmpg123_MPG123_feed
        (JNIEnv *env, jclass c, jlong handle, jbyteArray in_buffer, jint in_size) {
    MP3File *mp3 = (MP3File *) handle;
    mpg123_handle *mh = mp3->handle;
    jboolean isCopy;
    jbyte *b = (*env)->GetByteArrayElements(env, in_buffer, &isCopy);

    int err = mpg123_feed(mh, b, in_size);
    if (err != MPG123_OK)
        __android_log_print(ANDROID_LOG_INFO, "podax-jni", "mpg123_feed error: %s",
                            mpg123_plain_strerror(err));
    (*env)->ReleaseByteArrayElements(env, in_buffer, b, JNI_ABORT);

    if (mp3->rate == 0) {
        off_t frame_offset;
        unsigned char *audio;
        size_t bytes_done;
        err = mpg123_decode_frame(mh, &frame_offset, &audio, &bytes_done);
        if (err == MPG123_NEW_FORMAT) {
            int encoding;
            err = mpg123_getformat(mh, &mp3->rate, &mp3->channels, &encoding);
            if (err != MPG123_NEED_MORE && err != MPG123_OK) {
                printerr("mpg123_getformat", err);
                return;
            }

            mp3->samples_per_frame = mpg123_spf(mh);
            mp3->secs_per_frame = mpg123_tpf(mh);
        }
        if (err != MPG123_OK && err != MPG123_NEED_MORE)
            __android_log_print(ANDROID_LOG_INFO, "podax-jni", "cannot get rate: %s",
                                mpg123_plain_strerror(err));
    }
}


JNIEXPORT jlong JNICALL Java_me_rosuh_libmpg123_MPG123_openFile
        (JNIEnv *env, jclass c, jstring filename) {
    int err = MPG123_OK;
    mpg123_handle *mh = mpg123_new(NULL, &err);
    if (err == MPG123_OK && mh != NULL) {
        MP3File *mp3 = mp3file_init(mh);
        const char *fileString = (*env)->GetStringUTFChars(env, filename, NULL);
        err = mpg123_open(mh, fileString);
        (*env)->ReleaseStringUTFChars(env, filename, fileString);

        if (err == MPG123_OK) {
            int encoding;
            if (mpg123_getformat(mh, &mp3->rate, &mp3->channels, &encoding) == MPG123_OK) {
                if (encoding == MPG123_ENC_SIGNED_16) {
                    // Signed 16 is the default output format anyway;
                    // it would actually by only different if we forced it.
                    // So this check is here just for this explanation.

                    // Ensure that this output format will not change
                    // (it could, when we allow it).
                    mpg123_format_none(mh);
                    mpg123_format(mh, mp3->rate, mp3->channels, encoding);

                    mp3->buffer_size = mpg123_outblock(mh);
                    mp3->buffer = (unsigned char *) malloc(mp3->buffer_size);

                    mp3->num_samples = mpg123_length(mh);
                    mp3->samples_per_frame = mpg123_spf(mh);
                    mp3->secs_per_frame = mpg123_tpf(mh);

                    if (mp3->num_samples == MPG123_ERR || mp3->samples_per_frame < 0)
                        mp3->num_frames = 0;
                    else
                        mp3->num_frames = mp3->num_samples / mp3->samples_per_frame;

                    if (mp3->num_samples == MPG123_ERR || mp3->samples_per_frame < 0 ||
                        mp3->secs_per_frame < 0)
                        mp3->duration = 0;
                    else
                        mp3->duration = (float) (mp3->num_samples / mp3->samples_per_frame *
                                                 mp3->secs_per_frame);

                    return (jlong) mp3;
                }
            }
        }
        mp3file_delete(mp3);
    } else {
        __android_log_write(ANDROID_LOG_INFO, "mp3decoders-jni", mpg123_plain_strerror(err));
    }
    return 0;
}

JNIEXPORT void JNICALL Java_me_rosuh_libmpg123_MPG123_delete
        (JNIEnv *env, jclass c, jlong handle) {
    MP3File *mp3 = (MP3File *) handle;
    mp3file_delete(mp3);
}

JNIEXPORT jshortArray JNICALL Java_me_rosuh_libmpg123_MPG123_readFrame
        (JNIEnv *env, jclass c, jlong handle) {
    MP3File *mp3 = (MP3File *) handle;
    mpg123_handle *mh = mp3->handle;

    off_t frame_offset;
    unsigned char *audio;
    size_t bytes_done;

    int err = mpg123_decode_frame(mh, &frame_offset, &audio, &bytes_done);
    jshortArray buffer = (*env)->NewShortArray(env, bytes_done / 2);
    if (err != MPG123_OK) {
        printerr("decode_frame ok", err);
        return (*env)->NewShortArray(env, 0);
//        printerr("mpg123_decode_frame", err);
    }

    short *c_array = (*env)->GetShortArrayElements(env, buffer, 0);
    memcpy(c_array, audio, bytes_done);
    (*env)->ReleaseShortArrayElements(env, buffer, c_array, 0);

    return buffer;
}

JNIEXPORT jboolean JNICALL Java_me_rosuh_libmpg123_MPG123_skipFrame
        (JNIEnv *env, jclass c, jlong handle) {
    MP3File *mp3 = (MP3File *) handle;
    mpg123_handle *mh = mp3->handle;

    off_t frame_offset;
    unsigned char *audio;
    size_t bytes_done;
    int err = mpg123_decode_frame(mh, &frame_offset, &audio, &bytes_done);
    return (err == MPG123_OK) ? JNI_TRUE : JNI_FALSE;
}

static inline int readBuffer(MP3File *mp3) {
    size_t done = 0;
    int err = mpg123_read(mp3->handle, mp3->buffer, mp3->buffer_size, &done);

    mp3->leftSamples = done / 2;
    mp3->offset = 0;

    return err != MPG123_OK ? 0 : done;
}

JNIEXPORT jint JNICALL Java_me_rosuh_libmpg123_MPG123_readSamples
        (JNIEnv *env, jclass c, jlong handle, jshortArray obj_buffer, jint offset,
         jint numSamples) {
    MP3File *mp3 = (MP3File *) handle;
    short *buffer = (short *) (*env)->GetPrimitiveArrayCritical(env, obj_buffer, 0);
    short *target = buffer + offset;

    int idx = 0;
    while (idx != numSamples) {
        if (mp3->leftSamples > 0) {
            short *src = ((short *) mp3->buffer) + mp3->offset;
            while (idx < numSamples && mp3->offset < mp3->buffer_size / 2) {
                *target = *src;
                mp3->leftSamples--;
                mp3->offset++;
                target++;
                src++;
                idx++;
            }
        } else if (readBuffer(mp3) == 0) {
            (*env)->ReleasePrimitiveArrayCritical(env, obj_buffer, buffer, 0);
            return 0;
        }
    }

    (*env)->ReleasePrimitiveArrayCritical(env, obj_buffer, buffer, 0);
    return idx > numSamples ? 0 : idx;
}

JNIEXPORT jint JNICALL Java_me_rosuh_libmpg123_MPG123_skipSamples
        (JNIEnv *env, jclass c, jlong handle, jint numSamples) {
    MP3File *mp3 = (MP3File *) handle;
    int idx = 0;
    while (idx != numSamples) {
        if (mp3->leftSamples > 0) {
            while (idx < numSamples && mp3->offset < mp3->buffer_size / 2) {
                mp3->leftSamples--;
                mp3->offset++;
                idx++;
            }
        } else if (readBuffer(mp3) == 0)
            return 0;
    }

    return idx > numSamples ? 0 : idx;
}

JNIEXPORT jint JNICALL Java_me_rosuh_libmpg123_MPG123_seek
        (JNIEnv *env, jclass c, jlong handle, jfloat seconds, jint mode) {
    MP3File *mp3 = (MP3File *) handle;
    if (mode != SEEK_SET && mode != SEEK_CUR && mode != SEEK_END) {
        mode = SEEK_SET;
    }
    return mpg123_seek(mp3->handle, (int) seconds / mp3->secs_per_frame * mp3->samples_per_frame,
                       mode);
}

JNIEXPORT float JNICALL Java_me_rosuh_libmpg123_MPG123_getPosition
        (JNIEnv *env, jclass c, jlong handle) {
    MP3File *mp3 = (MP3File *) handle;
    return mpg123_tellframe(mp3->handle) * mp3->secs_per_frame;
}

JNIEXPORT jint JNICALL Java_me_rosuh_libmpg123_MPG123_getNumChannels
        (JNIEnv *env, jclass c, jlong handle) {
    MP3File *mp3 = (MP3File *) handle;
    if (mp3->channels == 0)
        mp3file_determineStats(mp3);
    return mp3->channels;
}

JNIEXPORT jint JNICALL Java_me_rosuh_libmpg123_MPG123_getRate
        (JNIEnv *env, jclass c, jlong handle) {
    MP3File *mp3 = (MP3File *) handle;
    if (mp3->rate == 0)
        mp3file_determineStats(mp3);
    return mp3->rate;
}

JNIEXPORT jlong JNICALL Java_me_rosuh_libmpg123_MPG123_getNumFrames
        (JNIEnv *env, jclass c, jlong handle) {
    MP3File *mp3 = (MP3File *) handle;
    return mp3->num_frames;
}

JNIEXPORT jfloat JNICALL Java_me_rosuh_libmpg123_MPG123_getDuration
        (JNIEnv *env, jclass c, jlong handle) {
    MP3File *mp3 = (MP3File *) handle;
    if (mp3->duration == 0)
        mp3file_determineStats(mp3);
    return mp3->duration;
}

JNIEXPORT jdouble JNICALL Java_me_rosuh_libmpg123_MPG123_getSecondsPerFrame
        (JNIEnv *env, jclass c, jlong handle) {
    MP3File *mp3 = (MP3File *) handle;
    return mp3->secs_per_frame;
}

JNIEXPORT jlong JNICALL Java_me_rosuh_libmpg123_MPG123_getOutputBlockSize
        (JNIEnv *env, jclass c, jlong handle) {
    MP3File *mp3 = (MP3File *) handle;
    return mpg123_outblock(mp3->handle);
}

JNIEXPORT jintArray JNICALL Java_me_rosuh_libmpg123_MPG123_getSupportedRates
        (JNIEnv *env, jclass c) {
    const long *list;
    size_t i, number;

    mpg123_rates(&list, &number);
    jintArray result = (*env)->NewIntArray(env, number);
    jint *resultData = (jint *) (*env)->GetPrimitiveArrayCritical(env, result, 0);
    for (i = 0; i < number; i++)
        resultData[i] = list[i];
    (*env)->ReleasePrimitiveArrayCritical(env, result, resultData, 0);
    return result;
}

JNIEXPORT jint JNICALL Java_me_rosuh_libmpg123_MPG123_getSeekFrameOffset
        (JNIEnv *env, jclass c, jlong mp3file, jfloat seconds) {
    MP3File *mp3 = (MP3File *) mp3file;
    mpg123_handle *mh = mp3->handle;

    size_t fill;
    off_t step;
    off_t *offsets;
    mpg123_index(mh, &offsets, &step, &fill);

    int target_frame = (int) (seconds / mp3->secs_per_frame); // the frame number to seek to
    int target_index = target_frame / step; // the closest index to the target frame
    // say so if there aren't enough entries in the index
    if (target_index >= fill)
        return -1;
    return offsets[target_index];
}

JNIEXPORT jlong JNICALL
Java_me_rosuh_libmpg123_MPG123_getTimeFrame(JNIEnv *env, jclass clazz, jlong handle, jdouble sec) {
    MP3File *mp3 = (MP3File *) handle;
    return mpg123_timeframe(mp3->handle, sec);
}

JNIEXPORT jlong JNICALL
Java_me_rosuh_libmpg123_MPG123_seekFrame(JNIEnv *env, jclass clazz, jlong handle, jfloat offset,
                                         jint mode) {
    MP3File *mp3 = (MP3File *) handle;
    if (mode != SEEK_SET && mode != SEEK_CUR && mode != SEEK_END) {
        mode = SEEK_SET;
    }
    return mpg123_seek_frame(mp3->handle, offset, mode);
}