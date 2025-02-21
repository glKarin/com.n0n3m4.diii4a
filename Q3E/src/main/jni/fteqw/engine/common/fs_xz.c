#include "quakedef.h"
#include "fs.h"

#ifdef AVAIL_XZDEC

#define XZ_EXTERN static
#define XZ_DEC_DYNALLOC	//data comes in periodically, can't use single mode.

//http://tukaani.org/xz/embedded.html
//we use an amalgamation, just because.
//note that the original code has no stable api nor public library.
#if 0
#include "xz/xz_dec_stream.c"
#include "xz/xz_dec_lzma2.c"
#include "xz/xz_crc32.c"
#else

#	include <stddef.h>

#if __STDC_VERSION__ >= 199901L || defined(__GNUC__)
#	include <stdint.h>
#else
#define uint8_t unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int
#define uint64_t quint64_t
#endif


//BEGIN xz.h
/*
 * XZ decompressor
 *
 * Authors: Lasse Collin <lasse.collin@tukaani.org>
 *          Igor Pavlov <http://7-zip.org/>
 *
 * This file has been put into the public domain.
 * You can do whatever you want with this file.
 */

#ifndef XZ_H
#define XZ_H

#ifdef __KERNEL__
#	include <linux/stddef.h>
#	include <linux/types.h>
#else
#	include <stddef.h>
//#	include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* In Linux, this is used to make extern functions static when needed. */
#ifndef XZ_EXTERN
#	define XZ_EXTERN extern
#endif

/**
 * enum xz_mode - Operation mode
 *
 * @XZ_SINGLE:              Single-call mode. This uses less RAM than
 *                          than multi-call modes, because the LZMA2
 *                          dictionary doesn't need to be allocated as
 *                          part of the decoder state. All required data
 *                          structures are allocated at initialization,
 *                          so xz_dec_run() cannot return XZ_MEM_ERROR.
 * @XZ_PREALLOC:            Multi-call mode with preallocated LZMA2
 *                          dictionary buffer. All data structures are
 *                          allocated at initialization, so xz_dec_run()
 *                          cannot return XZ_MEM_ERROR.
 * @XZ_DYNALLOC:            Multi-call mode. The LZMA2 dictionary is
 *                          allocated once the required size has been
 *                          parsed from the stream headers. If the
 *                          allocation fails, xz_dec_run() will return
 *                          XZ_MEM_ERROR.
 *
 * It is possible to enable support only for a subset of the above
 * modes at compile time by defining XZ_DEC_SINGLE, XZ_DEC_PREALLOC,
 * or XZ_DEC_DYNALLOC. The xz_dec kernel module is always compiled
 * with support for all operation modes, but the preboot code may
 * be built with fewer features to minimize code size.
 */
enum xz_mode {
	XZ_SINGLE,
	XZ_PREALLOC,
	XZ_DYNALLOC
};

/**
 * enum xz_ret - Return codes
 * @XZ_OK:                  Everything is OK so far. More input or more
 *                          output space is required to continue. This
 *                          return code is possible only in multi-call mode
 *                          (XZ_PREALLOC or XZ_DYNALLOC).
 * @XZ_STREAM_END:          Operation finished successfully.
 * @XZ_UNSUPPORTED_CHECK:   Integrity check type is not supported. Decoding
 *                          is still possible in multi-call mode by simply
 *                          calling xz_dec_run() again.
 *                          Note that this return value is used only if
 *                          XZ_DEC_ANY_CHECK was defined at build time,
 *                          which is not used in the kernel. Unsupported
 *                          check types return XZ_OPTIONS_ERROR if
 *                          XZ_DEC_ANY_CHECK was not defined at build time.
 * @XZ_MEM_ERROR:           Allocating memory failed. This return code is
 *                          possible only if the decoder was initialized
 *                          with XZ_DYNALLOC. The amount of memory that was
 *                          tried to be allocated was no more than the
 *                          dict_max argument given to xz_dec_init().
 * @XZ_MEMLIMIT_ERROR:      A bigger LZMA2 dictionary would be needed than
 *                          allowed by the dict_max argument given to
 *                          xz_dec_init(). This return value is possible
 *                          only in multi-call mode (XZ_PREALLOC or
 *                          XZ_DYNALLOC); the single-call mode (XZ_SINGLE)
 *                          ignores the dict_max argument.
 * @XZ_FORMAT_ERROR:        File format was not recognized (wrong magic
 *                          bytes).
 * @XZ_OPTIONS_ERROR:       This implementation doesn't support the requested
 *                          compression options. In the decoder this means
 *                          that the header CRC32 matches, but the header
 *                          itself specifies something that we don't support.
 * @XZ_DATA_ERROR:          Compressed data is corrupt.
 * @XZ_BUF_ERROR:           Cannot make any progress. Details are slightly
 *                          different between multi-call and single-call
 *                          mode; more information below.
 *
 * In multi-call mode, XZ_BUF_ERROR is returned when two consecutive calls
 * to XZ code cannot consume any input and cannot produce any new output.
 * This happens when there is no new input available, or the output buffer
 * is full while at least one output byte is still pending. Assuming your
 * code is not buggy, you can get this error only when decoding a compressed
 * stream that is truncated or otherwise corrupt.
 *
 * In single-call mode, XZ_BUF_ERROR is returned only when the output buffer
 * is too small or the compressed input is corrupt in a way that makes the
 * decoder produce more output than the caller expected. When it is
 * (relatively) clear that the compressed input is truncated, XZ_DATA_ERROR
 * is used instead of XZ_BUF_ERROR.
 */
enum xz_ret {
	XZ_OK,
	XZ_STREAM_END,
	XZ_UNSUPPORTED_CHECK,
	XZ_MEM_ERROR,
	XZ_MEMLIMIT_ERROR,
	XZ_FORMAT_ERROR,
	XZ_OPTIONS_ERROR,
	XZ_DATA_ERROR,
	XZ_BUF_ERROR
};

/**
 * struct xz_buf - Passing input and output buffers to XZ code
 * @in:         Beginning of the input buffer. This may be NULL if and only
 *              if in_pos is equal to in_size.
 * @in_pos:     Current position in the input buffer. This must not exceed
 *              in_size.
 * @in_size:    Size of the input buffer
 * @out:        Beginning of the output buffer. This may be NULL if and only
 *              if out_pos is equal to out_size.
 * @out_pos:    Current position in the output buffer. This must not exceed
 *              out_size.
 * @out_size:   Size of the output buffer
 *
 * Only the contents of the output buffer from out[out_pos] onward, and
 * the variables in_pos and out_pos are modified by the XZ code.
 */
struct xz_buf {
	const uint8_t *in;
	size_t in_pos;
	size_t in_size;

	uint8_t *out;
	size_t out_pos;
	size_t out_size;
};

/**
 * struct xz_dec - Opaque type to hold the XZ decoder state
 */
struct xz_dec;

/**
 * xz_dec_init() - Allocate and initialize a XZ decoder state
 * @mode:       Operation mode
 * @dict_max:   Maximum size of the LZMA2 dictionary (history buffer) for
 *              multi-call decoding. This is ignored in single-call mode
 *              (mode == XZ_SINGLE). LZMA2 dictionary is always 2^n bytes
 *              or 2^n + 2^(n-1) bytes (the latter sizes are less common
 *              in practice), so other values for dict_max don't make sense.
 *              In the kernel, dictionary sizes of 64 KiB, 128 KiB, 256 KiB,
 *              512 KiB, and 1 MiB are probably the only reasonable values,
 *              except for kernel and initramfs images where a bigger
 *              dictionary can be fine and useful.
 *
 * Single-call mode (XZ_SINGLE): xz_dec_run() decodes the whole stream at
 * once. The caller must provide enough output space or the decoding will
 * fail. The output space is used as the dictionary buffer, which is why
 * there is no need to allocate the dictionary as part of the decoder's
 * internal state.
 *
 * Because the output buffer is used as the workspace, streams encoded using
 * a big dictionary are not a problem in single-call mode. It is enough that
 * the output buffer is big enough to hold the actual uncompressed data; it
 * can be smaller than the dictionary size stored in the stream headers.
 *
 * Multi-call mode with preallocated dictionary (XZ_PREALLOC): dict_max bytes
 * of memory is preallocated for the LZMA2 dictionary. This way there is no
 * risk that xz_dec_run() could run out of memory, since xz_dec_run() will
 * never allocate any memory. Instead, if the preallocated dictionary is too
 * small for decoding the given input stream, xz_dec_run() will return
 * XZ_MEMLIMIT_ERROR. Thus, it is important to know what kind of data will be
 * decoded to avoid allocating excessive amount of memory for the dictionary.
 *
 * Multi-call mode with dynamically allocated dictionary (XZ_DYNALLOC):
 * dict_max specifies the maximum allowed dictionary size that xz_dec_run()
 * may allocate once it has parsed the dictionary size from the stream
 * headers. This way excessive allocations can be avoided while still
 * limiting the maximum memory usage to a sane value to prevent running the
 * system out of memory when decompressing streams from untrusted sources.
 *
 * On success, xz_dec_init() returns a pointer to struct xz_dec, which is
 * ready to be used with xz_dec_run(). If memory allocation fails,
 * xz_dec_init() returns NULL.
 */
XZ_EXTERN struct xz_dec *xz_dec_init(enum xz_mode mode, uint32_t dict_max);

/**
 * xz_dec_run() - Run the XZ decoder
 * @s:          Decoder state allocated using xz_dec_init()
 * @b:          Input and output buffers
 *
 * The possible return values depend on build options and operation mode.
 * See enum xz_ret for details.
 *
 * Note that if an error occurs in single-call mode (return value is not
 * XZ_STREAM_END), b->in_pos and b->out_pos are not modified and the
 * contents of the output buffer from b->out[b->out_pos] onward are
 * undefined. This is true even after XZ_BUF_ERROR, because with some filter
 * chains, there may be a second pass over the output buffer, and this pass
 * cannot be properly done if the output buffer is truncated. Thus, you
 * cannot give the single-call decoder a too small buffer and then expect to
 * get that amount valid data from the beginning of the stream. You must use
 * the multi-call decoder if you don't want to uncompress the whole stream.
 */
XZ_EXTERN enum xz_ret xz_dec_run(struct xz_dec *s, struct xz_buf *b);

/**
 * xz_dec_reset() - Reset an already allocated decoder state
 * @s:          Decoder state allocated using xz_dec_init()
 *
 * This function can be used to reset the multi-call decoder state without
 * freeing and reallocating memory with xz_dec_end() and xz_dec_init().
 *
 * In single-call mode, xz_dec_reset() is always called in the beginning of
 * xz_dec_run(). Thus, explicit call to xz_dec_reset() is useful only in
 * multi-call mode.
 */
XZ_EXTERN void xz_dec_reset(struct xz_dec *s);

/**
 * xz_dec_end() - Free the memory allocated for the decoder state
 * @s:          Decoder state allocated using xz_dec_init(). If s is NULL,
 *              this function does nothing.
 */
XZ_EXTERN void xz_dec_end(struct xz_dec *s);

/*
 * Standalone build (userspace build or in-kernel build for boot time use)
 * needs a CRC32 implementation. For normal in-kernel use, kernel's own
 * CRC32 module is used instead, and users of this module don't need to
 * care about the functions below.
 */
#ifndef XZ_INTERNAL_CRC32
#	ifdef __KERNEL__
#		define XZ_INTERNAL_CRC32 0
#	else
#		define XZ_INTERNAL_CRC32 1
#	endif
#endif

/*
 * If CRC64 support has been enabled with XZ_USE_CRC64, a CRC64
 * implementation is needed too.
 */
#ifndef XZ_USE_CRC64
#	undef XZ_INTERNAL_CRC64
#	define XZ_INTERNAL_CRC64 0
#endif
#ifndef XZ_INTERNAL_CRC64
#	ifdef __KERNEL__
#		error Using CRC64 in the kernel has not been implemented.
#	else
#		define XZ_INTERNAL_CRC64 1
#	endif
#endif

#if XZ_INTERNAL_CRC32
/*
 * This must be called before any other xz_* function to initialize
 * the CRC32 lookup table.
 */
XZ_EXTERN void xz_crc32_init(void);

/*
 * Update CRC32 value using the polynomial from IEEE-802.3. To start a new
 * calculation, the third argument must be zero. To continue the calculation,
 * the previously returned value is passed as the third argument.
 */
XZ_EXTERN uint32_t xz_crc32(const uint8_t *buf, size_t size, uint32_t crc);
#endif

#if XZ_INTERNAL_CRC64
/*
 * This must be called before any other xz_* function (except xz_crc32_init())
 * to initialize the CRC64 lookup table.
 */
XZ_EXTERN void xz_crc64_init(void);

/*
 * Update CRC64 value using the polynomial from ECMA-182. To start a new
 * calculation, the third argument must be zero. To continue the calculation,
 * the previously returned value is passed as the third argument.
 */
XZ_EXTERN uint64_t xz_crc64(const uint8_t *buf, size_t size, uint64_t crc);
#endif

#ifdef __cplusplus
}
#endif

#endif
//END xz.h

//BEGIN xz_config.h
/*
 * Private includes and definitions for userspace use of XZ Embedded
 *
 * Author: Lasse Collin <lasse.collin@tukaani.org>
 *
 * This file has been put into the public domain.
 * You can do whatever you want with this file.
 */

#ifndef XZ_CONFIG_H
#define XZ_CONFIG_H

/* Uncomment to enable CRC64 support. */
/* #define XZ_USE_CRC64 */

/* Uncomment as needed to enable BCJ filter decoders. */
/* #define XZ_DEC_X86 */
/* #define XZ_DEC_POWERPC */
/* #define XZ_DEC_IA64 */
/* #define XZ_DEC_ARM */
/* #define XZ_DEC_ARMTHUMB */
/* #define XZ_DEC_SPARC */

/*
 * MSVC doesn't support modern C but XZ Embedded is mostly C89
 * so these are enough.
 */
#ifdef _MSC_VER
typedef unsigned char bool;
#	define true 1
#	define false 0
#	ifndef inline
#		define inline __inline
#	endif
#else
#	include <stdbool.h>
#endif

#include <stdlib.h>
#include <string.h>

//#include "xz.h"

#define kmalloc(size, flags) malloc(size)
#define kfree(ptr) free(ptr)
#define vmalloc(size) malloc(size)
#define vfree(ptr) free(ptr)

#define memeq(a, b, size) (memcmp(a, b, size) == 0)
#define memzero(buf, size) memset(buf, 0, size)

#ifndef min
#	define min(x, y) ((x) < (y) ? (x) : (y))
#endif
#define min_t(type, x, y) min(x, y)

/*
 * Some functions have been marked with __always_inline to keep the
 * performance reasonable even when the compiler is optimizing for
 * small code size. You may be able to save a few bytes by #defining
 * __always_inline to plain inline, but don't complain if the code
 * becomes slow.
 *
 * NOTE: System headers on GNU/Linux may #define this macro already,
 * so if you want to change it, you need to #undef it first.
 */
#ifndef always_inline
#	ifdef __GNUC__
#		define always_inline \
			inline __attribute__((__always_inline__))
#	else
#		define always_inline inline
#	endif
#endif

/* Inline functions to access unaligned unsigned 32-bit integers */
#ifndef get_unaligned_le32
static inline uint32_t get_unaligned_le32(const uint8_t *buf)
{
	return (uint32_t)buf[0]
			| ((uint32_t)buf[1] << 8)
			| ((uint32_t)buf[2] << 16)
			| ((uint32_t)buf[3] << 24);
}
#endif

/*#ifndef get_unaligned_be32
static inline uint32_t get_unaligned_be32(const uint8_t *buf)
{
	return (uint32_t)(buf[0] << 24)
			| ((uint32_t)buf[1] << 16)
			| ((uint32_t)buf[2] << 8)
			| (uint32_t)buf[3];
}
#endif

#ifndef put_unaligned_le32
static inline void put_unaligned_le32(uint32_t val, uint8_t *buf)
{
	buf[0] = (uint8_t)val;
	buf[1] = (uint8_t)(val >> 8);
	buf[2] = (uint8_t)(val >> 16);
	buf[3] = (uint8_t)(val >> 24);
}
#endif

#ifndef put_unaligned_be32
static inline void put_unaligned_be32(uint32_t val, uint8_t *buf)
{
	buf[0] = (uint8_t)(val >> 24);
	buf[1] = (uint8_t)(val >> 16);
	buf[2] = (uint8_t)(val >> 8);
	buf[3] = (uint8_t)val;
}
#endif*/

/*
 * Use get_unaligned_le32() also for aligned access for simplicity. On
 * little endian systems, #define get_le32(ptr) (*(const uint32_t *)(ptr))
 * could save a few bytes in code size.
 */
#ifndef get_le32
#	define get_le32 get_unaligned_le32
#endif

#endif
//END xz_config.h

//BEGIN xz_private.h
/*
 * Private includes and definitions
 *
 * Author: Lasse Collin <lasse.collin@tukaani.org>
 *
 * This file has been put into the public domain.
 * You can do whatever you want with this file.
 */

#ifndef XZ_PRIVATE_H
#define XZ_PRIVATE_H

#ifdef __KERNEL__
#	include <linux/xz.h>
#	include <linux/kernel.h>
#	include <asm/unaligned.h>
	/* XZ_PREBOOT may be defined only via decompress_unxz.c. */
#	ifndef XZ_PREBOOT
#		include <linux/slab.h>
#		include <linux/vmalloc.h>
#		include <linux/string.h>
#		ifdef CONFIG_XZ_DEC_X86
#			define XZ_DEC_X86
#		endif
#		ifdef CONFIG_XZ_DEC_POWERPC
#			define XZ_DEC_POWERPC
#		endif
#		ifdef CONFIG_XZ_DEC_IA64
#			define XZ_DEC_IA64
#		endif
#		ifdef CONFIG_XZ_DEC_ARM
#			define XZ_DEC_ARM
#		endif
#		ifdef CONFIG_XZ_DEC_ARMTHUMB
#			define XZ_DEC_ARMTHUMB
#		endif
#		ifdef CONFIG_XZ_DEC_SPARC
#			define XZ_DEC_SPARC
#		endif
#		define memeq(a, b, size) (memcmp(a, b, size) == 0)
#		define memzero(buf, size) memset(buf, 0, size)
#	endif
#	define get_le32(p) le32_to_cpup((const uint32_t *)(p))
#else
	/*
	 * For userspace builds, use a separate header to define the required
	 * macros and functions. This makes it easier to adapt the code into
	 * different environments and avoids clutter in the Linux kernel tree.
	 */
//#	include "xz_config.h"
#endif

/* If no specific decoding mode is requested, enable support for all modes. */
#if !defined(XZ_DEC_SINGLE) && !defined(XZ_DEC_PREALLOC) \
		&& !defined(XZ_DEC_DYNALLOC)
#	define XZ_DEC_SINGLE
#	define XZ_DEC_PREALLOC
#	define XZ_DEC_DYNALLOC
#endif

/*
 * The DEC_IS_foo(mode) macros are used in "if" statements. If only some
 * of the supported modes are enabled, these macros will evaluate to true or
 * false at compile time and thus allow the compiler to omit unneeded code.
 */
#ifdef XZ_DEC_SINGLE
#	define DEC_IS_SINGLE(mode) ((mode) == XZ_SINGLE)
#else
#	define DEC_IS_SINGLE(mode) (false)
#endif

#ifdef XZ_DEC_PREALLOC
#	define DEC_IS_PREALLOC(mode) ((mode) == XZ_PREALLOC)
#else
#	define DEC_IS_PREALLOC(mode) (false)
#endif

#ifdef XZ_DEC_DYNALLOC
#	define DEC_IS_DYNALLOC(mode) ((mode) == XZ_DYNALLOC)
#else
#	define DEC_IS_DYNALLOC(mode) (false)
#endif

#if !defined(XZ_DEC_SINGLE)
#	define DEC_IS_MULTI(mode) (true)
#elif defined(XZ_DEC_PREALLOC) || defined(XZ_DEC_DYNALLOC)
#	define DEC_IS_MULTI(mode) ((mode) != XZ_SINGLE)
#else
#	define DEC_IS_MULTI(mode) (false)
#endif

/*
 * If any of the BCJ filter decoders are wanted, define XZ_DEC_BCJ.
 * XZ_DEC_BCJ is used to enable generic support for BCJ decoders.
 */
#ifndef XZ_DEC_BCJ
#	if defined(XZ_DEC_X86) || defined(XZ_DEC_POWERPC) \
			|| defined(XZ_DEC_IA64) || defined(XZ_DEC_ARM) \
			|| defined(XZ_DEC_ARM) || defined(XZ_DEC_ARMTHUMB) \
			|| defined(XZ_DEC_SPARC)
#		define XZ_DEC_BCJ
#	endif
#endif

/*
 * Allocate memory for LZMA2 decoder. xz_dec_lzma2_reset() must be used
 * before calling xz_dec_lzma2_run().
 */
XZ_EXTERN struct xz_dec_lzma2 *xz_dec_lzma2_create(enum xz_mode mode,
						   uint32_t dict_max);

/*
 * Decode the LZMA2 properties (one byte) and reset the decoder. Return
 * XZ_OK on success, XZ_MEMLIMIT_ERROR if the preallocated dictionary is not
 * big enough, and XZ_OPTIONS_ERROR if props indicates something that this
 * decoder doesn't support.
 */
XZ_EXTERN enum xz_ret xz_dec_lzma2_reset(struct xz_dec_lzma2 *s,
					 uint8_t props);

/* Decode raw LZMA2 stream from b->in to b->out. */
XZ_EXTERN enum xz_ret xz_dec_lzma2_run(struct xz_dec_lzma2 *s,
				       struct xz_buf *b);

/* Free the memory allocated for the LZMA2 decoder. */
XZ_EXTERN void xz_dec_lzma2_end(struct xz_dec_lzma2 *s);

#ifdef XZ_DEC_BCJ
/*
 * Allocate memory for BCJ decoders. xz_dec_bcj_reset() must be used before
 * calling xz_dec_bcj_run().
 */
XZ_EXTERN struct xz_dec_bcj *xz_dec_bcj_create(bool single_call);

/*
 * Decode the Filter ID of a BCJ filter. This implementation doesn't
 * support custom start offsets, so no decoding of Filter Properties
 * is needed. Returns XZ_OK if the given Filter ID is supported.
 * Otherwise XZ_OPTIONS_ERROR is returned.
 */
XZ_EXTERN enum xz_ret xz_dec_bcj_reset(struct xz_dec_bcj *s, uint8_t id);

/*
 * Decode raw BCJ + LZMA2 stream. This must be used only if there actually is
 * a BCJ filter in the chain. If the chain has only LZMA2, xz_dec_lzma2_run()
 * must be called directly.
 */
XZ_EXTERN enum xz_ret xz_dec_bcj_run(struct xz_dec_bcj *s,
				     struct xz_dec_lzma2 *lzma2,
				     struct xz_buf *b);

/* Free the memory allocated for the BCJ filters. */
#define xz_dec_bcj_end(s) kfree(s)
#endif

#endif
//END xz_private.h
//BEGIN xz_stream.h
/*
 * Definitions for handling the .xz file format
 *
 * Author: Lasse Collin <lasse.collin@tukaani.org>
 *
 * This file has been put into the public domain.
 * You can do whatever you want with this file.
 */

#ifndef XZ_STREAM_H
#define XZ_STREAM_H

#if defined(__KERNEL__) && !XZ_INTERNAL_CRC32
#	include <linux/crc32.h>
#	undef crc32
#	define xz_crc32(buf, size, crc) \
		(~crc32_le(~(uint32_t)(crc), buf, size))
#endif

/*
 * See the .xz file format specification at
 * http://tukaani.org/xz/xz-file-format.txt
 * to understand the container format.
 */

#define STREAM_HEADER_SIZE 12

#define HEADER_MAGIC "\375""7zXZ"
#define HEADER_MAGIC_SIZE 6

#define FOOTER_MAGIC "YZ"
#define FOOTER_MAGIC_SIZE 2

/*
 * Variable-length integer can hold a 63-bit unsigned integer or a special
 * value indicating that the value is unknown.
 *
 * Experimental: vli_type can be defined to uint32_t to save a few bytes
 * in code size (no effect on speed). Doing so limits the uncompressed and
 * compressed size of the file to less than 256 MiB and may also weaken
 * error detection slightly.
 */
typedef uint64_t vli_type;

#define VLI_MAX ((vli_type)-1 / 2)
#define VLI_UNKNOWN ((vli_type)-1)

/* Maximum encoded size of a VLI */
#define VLI_BYTES_MAX (sizeof(vli_type) * 8 / 7)

/* Integrity Check types */
enum xz_check {
	XZ_CHECK_NONE = 0,
	XZ_CHECK_CRC32 = 1,
	XZ_CHECK_CRC64 = 4,
	XZ_CHECK_SHA256 = 10
};

/* Maximum possible Check ID */
#define XZ_CHECK_MAX 15

#endif
//END xz_stream.h

//BEGIN "xz_lzma2.h"
/*
 * LZMA2 definitions
 *
 * Authors: Lasse Collin <lasse.collin@tukaani.org>
 *          Igor Pavlov <http://7-zip.org/>
 *
 * This file has been put into the public domain.
 * You can do whatever you want with this file.
 */

#ifndef XZ_LZMA2_H
#define XZ_LZMA2_H

/* Range coder constants */
#define RC_SHIFT_BITS 8
#define RC_TOP_BITS 24
#define RC_TOP_VALUE (1 << RC_TOP_BITS)
#define RC_BIT_MODEL_TOTAL_BITS 11
#define RC_BIT_MODEL_TOTAL (1 << RC_BIT_MODEL_TOTAL_BITS)
#define RC_MOVE_BITS 5

/*
 * Maximum number of position states. A position state is the lowest pb
 * number of bits of the current uncompressed offset. In some places there
 * are different sets of probabilities for different position states.
 */
#define POS_STATES_MAX (1 << 4)

/*
 * This enum is used to track which LZMA symbols have occurred most recently
 * and in which order. This information is used to predict the next symbol.
 *
 * Symbols:
 *  - Literal: One 8-bit byte
 *  - Match: Repeat a chunk of data at some distance
 *  - Long repeat: Multi-byte match at a recently seen distance
 *  - Short repeat: One-byte repeat at a recently seen distance
 *
 * The symbol names are in from STATE_oldest_older_previous. REP means
 * either short or long repeated match, and NONLIT means any non-literal.
 */
enum lzma_state {
	STATE_LIT_LIT,
	STATE_MATCH_LIT_LIT,
	STATE_REP_LIT_LIT,
	STATE_SHORTREP_LIT_LIT,
	STATE_MATCH_LIT,
	STATE_REP_LIT,
	STATE_SHORTREP_LIT,
	STATE_LIT_MATCH,
	STATE_LIT_LONGREP,
	STATE_LIT_SHORTREP,
	STATE_NONLIT_MATCH,
	STATE_NONLIT_REP
};

/* Total number of states */
#define STATES 12

/* The lowest 7 states indicate that the previous state was a literal. */
#define LIT_STATES 7

/* Indicate that the latest symbol was a literal. */
static inline void lzma_state_literal(enum lzma_state *state)
{
	if (*state <= STATE_SHORTREP_LIT_LIT)
		*state = STATE_LIT_LIT;
	else if (*state <= STATE_LIT_SHORTREP)
		*state -= 3;
	else
		*state -= 6;
}

/* Indicate that the latest symbol was a match. */
static inline void lzma_state_match(enum lzma_state *state)
{
	*state = *state < LIT_STATES ? STATE_LIT_MATCH : STATE_NONLIT_MATCH;
}

/* Indicate that the latest state was a long repeated match. */
static inline void lzma_state_long_rep(enum lzma_state *state)
{
	*state = *state < LIT_STATES ? STATE_LIT_LONGREP : STATE_NONLIT_REP;
}

/* Indicate that the latest symbol was a short match. */
static inline void lzma_state_short_rep(enum lzma_state *state)
{
	*state = *state < LIT_STATES ? STATE_LIT_SHORTREP : STATE_NONLIT_REP;
}

/* Test if the previous symbol was a literal. */
static inline bool lzma_state_is_literal(enum lzma_state state)
{
	return state < LIT_STATES;
}

/* Each literal coder is divided in three sections:
 *   - 0x001-0x0FF: Without match byte
 *   - 0x101-0x1FF: With match byte; match bit is 0
 *   - 0x201-0x2FF: With match byte; match bit is 1
 *
 * Match byte is used when the previous LZMA symbol was something else than
 * a literal (that is, it was some kind of match).
 */
#define LITERAL_CODER_SIZE 0x300

/* Maximum number of literal coders */
#define LITERAL_CODERS_MAX (1 << 4)

/* Minimum length of a match is two bytes. */
#define MATCH_LEN_MIN 2

/* Match length is encoded with 4, 5, or 10 bits.
 *
 * Length   Bits
 *  2-9      4 = Choice=0 + 3 bits
 * 10-17     5 = Choice=1 + Choice2=0 + 3 bits
 * 18-273   10 = Choice=1 + Choice2=1 + 8 bits
 */
#define LEN_LOW_BITS 3
#define LEN_LOW_SYMBOLS (1 << LEN_LOW_BITS)
#define LEN_MID_BITS 3
#define LEN_MID_SYMBOLS (1 << LEN_MID_BITS)
#define LEN_HIGH_BITS 8
#define LEN_HIGH_SYMBOLS (1 << LEN_HIGH_BITS)
#define LEN_SYMBOLS (LEN_LOW_SYMBOLS + LEN_MID_SYMBOLS + LEN_HIGH_SYMBOLS)

/*
 * Maximum length of a match is 273 which is a result of the encoding
 * described above.
 */
#define MATCH_LEN_MAX (MATCH_LEN_MIN + LEN_SYMBOLS - 1)

/*
 * Different sets of probabilities are used for match distances that have
 * very short match length: Lengths of 2, 3, and 4 bytes have a separate
 * set of probabilities for each length. The matches with longer length
 * use a shared set of probabilities.
 */
#define DIST_STATES 4

/*
 * Get the index of the appropriate probability array for decoding
 * the distance slot.
 */
static inline uint32_t lzma_get_dist_state(uint32_t len)
{
	return len < DIST_STATES + MATCH_LEN_MIN
			? len - MATCH_LEN_MIN : DIST_STATES - 1;
}

/*
 * The highest two bits of a 32-bit match distance are encoded using six bits.
 * This six-bit value is called a distance slot. This way encoding a 32-bit
 * value takes 6-36 bits, larger values taking more bits.
 */
#define DIST_SLOT_BITS 6
#define DIST_SLOTS (1 << DIST_SLOT_BITS)

/* Match distances up to 127 are fully encoded using probabilities. Since
 * the highest two bits (distance slot) are always encoded using six bits,
 * the distances 0-3 don't need any additional bits to encode, since the
 * distance slot itself is the same as the actual distance. DIST_MODEL_START
 * indicates the first distance slot where at least one additional bit is
 * needed.
 */
#define DIST_MODEL_START 4

/*
 * Match distances greater than 127 are encoded in three pieces:
 *   - distance slot: the highest two bits
 *   - direct bits: 2-26 bits below the highest two bits
 *   - alignment bits: four lowest bits
 *
 * Direct bits don't use any probabilities.
 *
 * The distance slot value of 14 is for distances 128-191.
 */
#define DIST_MODEL_END 14

/* Distance slots that indicate a distance <= 127. */
#define FULL_DISTANCES_BITS (DIST_MODEL_END / 2)
#define FULL_DISTANCES (1 << FULL_DISTANCES_BITS)

/*
 * For match distances greater than 127, only the highest two bits and the
 * lowest four bits (alignment) is encoded using probabilities.
 */
#define ALIGN_BITS 4
#define ALIGN_SIZE (1 << ALIGN_BITS)
#define ALIGN_MASK (ALIGN_SIZE - 1)

/* Total number of all probability variables */
#define PROBS_TOTAL (1846 + LITERAL_CODERS_MAX * LITERAL_CODER_SIZE)

/*
 * LZMA remembers the four most recent match distances. Reusing these
 * distances tends to take less space than re-encoding the actual
 * distance value.
 */
#define REPS 4

#endif

//END xz_lzma2.h
//BEGIN xz_dec_stream.c

/*
 * .xz Stream decoder
 *
 * Author: Lasse Collin <lasse.collin@tukaani.org>
 *
 * This file has been put into the public domain.
 * You can do whatever you want with this file.
 */

#ifdef XZ_USE_CRC64
#	define IS_CRC64(check_type) ((check_type) == XZ_CHECK_CRC64)
#else
#	define IS_CRC64(check_type) false
#endif

/* Hash used to validate the Index field */
struct xz_dec_hash {
	vli_type unpadded;
	vli_type uncompressed;
	uint32_t crc32;
};

struct xz_dec {
	/* Position in dec_main() */
	enum {
		SEQ_STREAM_HEADER,
		SEQ_BLOCK_START,
		SEQ_BLOCK_HEADER,
		SEQ_BLOCK_UNCOMPRESS,
		SEQ_BLOCK_PADDING,
		SEQ_BLOCK_CHECK,
		SEQ_INDEX,
		SEQ_INDEX_PADDING,
		SEQ_INDEX_CRC32,
		SEQ_STREAM_FOOTER
	} sequence;

	/* Position in variable-length integers and Check fields */
	uint32_t pos;

	/* Variable-length integer decoded by dec_vli() */
	vli_type vli;

	/* Saved in_pos and out_pos */
	size_t in_start;
	size_t out_start;

#ifdef XZ_USE_CRC64
	/* CRC32 or CRC64 value in Block or CRC32 value in Index */
	uint64_t crc;
#else
	/* CRC32 value in Block or Index */
	uint32_t crc;
#endif

	/* Type of the integrity check calculated from uncompressed data */
	enum xz_check check_type;

	/* Operation mode */
	enum xz_mode mode;

	/*
	 * True if the next call to xz_dec_run() is allowed to return
	 * XZ_BUF_ERROR.
	 */
	bool allow_buf_error;

	/* Information stored in Block Header */
	struct {
		/*
		 * Value stored in the Compressed Size field, or
		 * VLI_UNKNOWN if Compressed Size is not present.
		 */
		vli_type compressed;

		/*
		 * Value stored in the Uncompressed Size field, or
		 * VLI_UNKNOWN if Uncompressed Size is not present.
		 */
		vli_type uncompressed;

		/* Size of the Block Header field */
		uint32_t size;
	} block_header;

	/* Information collected when decoding Blocks */
	struct {
		/* Observed compressed size of the current Block */
		vli_type compressed;

		/* Observed uncompressed size of the current Block */
		vli_type uncompressed;

		/* Number of Blocks decoded so far */
		vli_type count;

		/*
		 * Hash calculated from the Block sizes. This is used to
		 * validate the Index field.
		 */
		struct xz_dec_hash hash;
	} block;

	/* Variables needed when verifying the Index field */
	struct {
		/* Position in dec_index() */
		enum {
			SEQ_INDEX_COUNT,
			SEQ_INDEX_UNPADDED,
			SEQ_INDEX_UNCOMPRESSED
		} sequence;

		/* Size of the Index in bytes */
		vli_type size;

		/* Number of Records (matches block.count in valid files) */
		vli_type count;

		/*
		 * Hash calculated from the Records (matches block.hash in
		 * valid files).
		 */
		struct xz_dec_hash hash;
	} index;

	/*
	 * Temporary buffer needed to hold Stream Header, Block Header,
	 * and Stream Footer. The Block Header is the biggest (1 KiB)
	 * so we reserve space according to that. buf[] has to be aligned
	 * to a multiple of four bytes; the size_t variables before it
	 * should guarantee this.
	 */
	struct {
		size_t pos;
		size_t size;
		uint8_t buf[1024];
	} temp;

	struct xz_dec_lzma2 *lzma2;

#ifdef XZ_DEC_BCJ
	struct xz_dec_bcj *bcj;
	bool bcj_active;
#endif
};

#ifdef XZ_DEC_ANY_CHECK
/* Sizes of the Check field with different Check IDs */
static const uint8_t check_sizes[16] = {
	0,
	4, 4, 4,
	8, 8, 8,
	16, 16, 16,
	32, 32, 32,
	64, 64, 64
};
#endif

/*
 * Fill s->temp by copying data starting from b->in[b->in_pos]. Caller
 * must have set s->temp.pos to indicate how much data we are supposed
 * to copy into s->temp.buf. Return true once s->temp.pos has reached
 * s->temp.size.
 */
static bool fill_temp(struct xz_dec *s, struct xz_buf *b)
{
	size_t copy_size = min_t(size_t,
			b->in_size - b->in_pos, s->temp.size - s->temp.pos);

	memcpy(s->temp.buf + s->temp.pos, b->in + b->in_pos, copy_size);
	b->in_pos += copy_size;
	s->temp.pos += copy_size;

	if (s->temp.pos == s->temp.size) {
		s->temp.pos = 0;
		return true;
	}

	return false;
}

/* Decode a variable-length integer (little-endian base-128 encoding) */
static enum xz_ret dec_vli(struct xz_dec *s, const uint8_t *in,
			   size_t *in_pos, size_t in_size)
{
	uint8_t byte;

	if (s->pos == 0)
		s->vli = 0;

	while (*in_pos < in_size) {
		byte = in[*in_pos];
		++*in_pos;

		s->vli |= (vli_type)(byte & 0x7F) << s->pos;

		if ((byte & 0x80) == 0) {
			/* Don't allow non-minimal encodings. */
			if (byte == 0 && s->pos != 0)
				return XZ_DATA_ERROR;

			s->pos = 0;
			return XZ_STREAM_END;
		}

		s->pos += 7;
		if (s->pos == 7 * VLI_BYTES_MAX)
			return XZ_DATA_ERROR;
	}

	return XZ_OK;
}

/*
 * Decode the Compressed Data field from a Block. Update and validate
 * the observed compressed and uncompressed sizes of the Block so that
 * they don't exceed the values possibly stored in the Block Header
 * (validation assumes that no integer overflow occurs, since vli_type
 * is normally uint64_t). Update the CRC32 or CRC64 value if presence of
 * the CRC32 or CRC64 field was indicated in Stream Header.
 *
 * Once the decoding is finished, validate that the observed sizes match
 * the sizes possibly stored in the Block Header. Update the hash and
 * Block count, which are later used to validate the Index field.
 */
static enum xz_ret dec_block(struct xz_dec *s, struct xz_buf *b)
{
	enum xz_ret ret;

	s->in_start = b->in_pos;
	s->out_start = b->out_pos;

#ifdef XZ_DEC_BCJ
	if (s->bcj_active)
		ret = xz_dec_bcj_run(s->bcj, s->lzma2, b);
	else
#endif
		ret = xz_dec_lzma2_run(s->lzma2, b);

	s->block.compressed += b->in_pos - s->in_start;
	s->block.uncompressed += b->out_pos - s->out_start;

	/*
	 * There is no need to separately check for VLI_UNKNOWN, since
	 * the observed sizes are always smaller than VLI_UNKNOWN.
	 */
	if (s->block.compressed > s->block_header.compressed
			|| s->block.uncompressed
				> s->block_header.uncompressed)
		return XZ_DATA_ERROR;

	if (s->check_type == XZ_CHECK_CRC32)
		s->crc = xz_crc32(b->out + s->out_start,
				b->out_pos - s->out_start, s->crc);
#ifdef XZ_USE_CRC64
	else if (s->check_type == XZ_CHECK_CRC64)
		s->crc = xz_crc64(b->out + s->out_start,
				b->out_pos - s->out_start, s->crc);
#endif

	if (ret == XZ_STREAM_END) {
		if (s->block_header.compressed != VLI_UNKNOWN
				&& s->block_header.compressed
					!= s->block.compressed)
			return XZ_DATA_ERROR;

		if (s->block_header.uncompressed != VLI_UNKNOWN
				&& s->block_header.uncompressed
					!= s->block.uncompressed)
			return XZ_DATA_ERROR;

		s->block.hash.unpadded += s->block_header.size
				+ s->block.compressed;

#ifdef XZ_DEC_ANY_CHECK
		s->block.hash.unpadded += check_sizes[s->check_type];
#else
		if (s->check_type == XZ_CHECK_CRC32)
			s->block.hash.unpadded += 4;
		else if (IS_CRC64(s->check_type))
			s->block.hash.unpadded += 8;
#endif

		s->block.hash.uncompressed += s->block.uncompressed;
		s->block.hash.crc32 = xz_crc32(
				(const uint8_t *)&s->block.hash,
				sizeof(s->block.hash), s->block.hash.crc32);

		++s->block.count;
	}

	return ret;
}

/* Update the Index size and the CRC32 value. */
static void index_update(struct xz_dec *s, const struct xz_buf *b)
{
	size_t in_used = b->in_pos - s->in_start;
	s->index.size += in_used;
	s->crc = xz_crc32(b->in + s->in_start, in_used, s->crc);
}

/*
 * Decode the Number of Records, Unpadded Size, and Uncompressed Size
 * fields from the Index field. That is, Index Padding and CRC32 are not
 * decoded by this function.
 *
 * This can return XZ_OK (more input needed), XZ_STREAM_END (everything
 * successfully decoded), or XZ_DATA_ERROR (input is corrupt).
 */
static enum xz_ret dec_index(struct xz_dec *s, struct xz_buf *b)
{
	enum xz_ret ret;

	do {
		ret = dec_vli(s, b->in, &b->in_pos, b->in_size);
		if (ret != XZ_STREAM_END) {
			index_update(s, b);
			return ret;
		}

		switch (s->index.sequence) {
		case SEQ_INDEX_COUNT:
			s->index.count = s->vli;

			/*
			 * Validate that the Number of Records field
			 * indicates the same number of Records as
			 * there were Blocks in the Stream.
			 */
			if (s->index.count != s->block.count)
				return XZ_DATA_ERROR;

			s->index.sequence = SEQ_INDEX_UNPADDED;
			break;

		case SEQ_INDEX_UNPADDED:
			s->index.hash.unpadded += s->vli;
			s->index.sequence = SEQ_INDEX_UNCOMPRESSED;
			break;

		case SEQ_INDEX_UNCOMPRESSED:
			s->index.hash.uncompressed += s->vli;
			s->index.hash.crc32 = xz_crc32(
					(const uint8_t *)&s->index.hash,
					sizeof(s->index.hash),
					s->index.hash.crc32);
			--s->index.count;
			s->index.sequence = SEQ_INDEX_UNPADDED;
			break;
		}
	} while (s->index.count > 0);

	return XZ_STREAM_END;
}

/*
 * Validate that the next four or eight input bytes match the value
 * of s->crc. s->pos must be zero when starting to validate the first byte.
 * The "bits" argument allows using the same code for both CRC32 and CRC64.
 */
static enum xz_ret crc_validate(struct xz_dec *s, struct xz_buf *b,
				uint32_t bits)
{
	do {
		if (b->in_pos == b->in_size)
			return XZ_OK;

		if (((s->crc >> s->pos) & 0xFF) != b->in[b->in_pos++])
			return XZ_DATA_ERROR;

		s->pos += 8;

	} while (s->pos < bits);

	s->crc = 0;
	s->pos = 0;

	return XZ_STREAM_END;
}

#ifdef XZ_DEC_ANY_CHECK
/*
 * Skip over the Check field when the Check ID is not supported.
 * Returns true once the whole Check field has been skipped over.
 */
static bool check_skip(struct xz_dec *s, struct xz_buf *b)
{
	while (s->pos < check_sizes[s->check_type]) {
		if (b->in_pos == b->in_size)
			return false;

		++b->in_pos;
		++s->pos;
	}

	s->pos = 0;

	return true;
}
#endif

/* Decode the Stream Header field (the first 12 bytes of the .xz Stream). */
static enum xz_ret dec_stream_header(struct xz_dec *s)
{
	if (!memeq(s->temp.buf, HEADER_MAGIC, HEADER_MAGIC_SIZE))
		return XZ_FORMAT_ERROR;

	if (xz_crc32(s->temp.buf + HEADER_MAGIC_SIZE, 2, 0)
			!= get_le32(s->temp.buf + HEADER_MAGIC_SIZE + 2))
		return XZ_DATA_ERROR;

	if (s->temp.buf[HEADER_MAGIC_SIZE] != 0)
		return XZ_OPTIONS_ERROR;

	/*
	 * Of integrity checks, we support none (Check ID = 0),
	 * CRC32 (Check ID = 1), and optionally CRC64 (Check ID = 4).
	 * However, if XZ_DEC_ANY_CHECK is defined, we will accept other
	 * check types too, but then the check won't be verified and
	 * a warning (XZ_UNSUPPORTED_CHECK) will be given.
	 */
	s->check_type = s->temp.buf[HEADER_MAGIC_SIZE + 1];

#ifdef XZ_DEC_ANY_CHECK
	if (s->check_type > XZ_CHECK_MAX)
		return XZ_OPTIONS_ERROR;

	if (s->check_type > XZ_CHECK_CRC32 && !IS_CRC64(s->check_type))
		return XZ_UNSUPPORTED_CHECK;
#else
	if (s->check_type > XZ_CHECK_CRC32 && !IS_CRC64(s->check_type))
		return XZ_OPTIONS_ERROR;
#endif

	return XZ_OK;
}

/* Decode the Stream Footer field (the last 12 bytes of the .xz Stream) */
static enum xz_ret dec_stream_footer(struct xz_dec *s)
{
	if (!memeq(s->temp.buf + 10, FOOTER_MAGIC, FOOTER_MAGIC_SIZE))
		return XZ_DATA_ERROR;

	if (xz_crc32(s->temp.buf + 4, 6, 0) != get_le32(s->temp.buf))
		return XZ_DATA_ERROR;

	/*
	 * Validate Backward Size. Note that we never added the size of the
	 * Index CRC32 field to s->index.size, thus we use s->index.size / 4
	 * instead of s->index.size / 4 - 1.
	 */
	if ((s->index.size >> 2) != get_le32(s->temp.buf + 4))
		return XZ_DATA_ERROR;

	if (s->temp.buf[8] != 0 || s->temp.buf[9] != s->check_type)
		return XZ_DATA_ERROR;

	/*
	 * Use XZ_STREAM_END instead of XZ_OK to be more convenient
	 * for the caller.
	 */
	return XZ_STREAM_END;
}

/* Decode the Block Header and initialize the filter chain. */
static enum xz_ret dec_block_header(struct xz_dec *s)
{
	enum xz_ret ret;

	/*
	 * Validate the CRC32. We know that the temp buffer is at least
	 * eight bytes so this is safe.
	 */
	s->temp.size -= 4;
	if (xz_crc32(s->temp.buf, s->temp.size, 0)
			!= get_le32(s->temp.buf + s->temp.size))
		return XZ_DATA_ERROR;

	s->temp.pos = 2;

	/*
	 * Catch unsupported Block Flags. We support only one or two filters
	 * in the chain, so we catch that with the same test.
	 */
#ifdef XZ_DEC_BCJ
	if (s->temp.buf[1] & 0x3E)
#else
	if (s->temp.buf[1] & 0x3F)
#endif
		return XZ_OPTIONS_ERROR;

	/* Compressed Size */
	if (s->temp.buf[1] & 0x40) {
		if (dec_vli(s, s->temp.buf, &s->temp.pos, s->temp.size)
					!= XZ_STREAM_END)
			return XZ_DATA_ERROR;

		s->block_header.compressed = s->vli;
	} else {
		s->block_header.compressed = VLI_UNKNOWN;
	}

	/* Uncompressed Size */
	if (s->temp.buf[1] & 0x80) {
		if (dec_vli(s, s->temp.buf, &s->temp.pos, s->temp.size)
				!= XZ_STREAM_END)
			return XZ_DATA_ERROR;

		s->block_header.uncompressed = s->vli;
	} else {
		s->block_header.uncompressed = VLI_UNKNOWN;
	}

#ifdef XZ_DEC_BCJ
	/* If there are two filters, the first one must be a BCJ filter. */
	s->bcj_active = s->temp.buf[1] & 0x01;
	if (s->bcj_active) {
		if (s->temp.size - s->temp.pos < 2)
			return XZ_OPTIONS_ERROR;

		ret = xz_dec_bcj_reset(s->bcj, s->temp.buf[s->temp.pos++]);
		if (ret != XZ_OK)
			return ret;

		/*
		 * We don't support custom start offset,
		 * so Size of Properties must be zero.
		 */
		if (s->temp.buf[s->temp.pos++] != 0x00)
			return XZ_OPTIONS_ERROR;
	}
#endif

	/* Valid Filter Flags always take at least two bytes. */
	if (s->temp.size - s->temp.pos < 2)
		return XZ_DATA_ERROR;

	/* Filter ID = LZMA2 */
	if (s->temp.buf[s->temp.pos++] != 0x21)
		return XZ_OPTIONS_ERROR;

	/* Size of Properties = 1-byte Filter Properties */
	if (s->temp.buf[s->temp.pos++] != 0x01)
		return XZ_OPTIONS_ERROR;

	/* Filter Properties contains LZMA2 dictionary size. */
	if (s->temp.size - s->temp.pos < 1)
		return XZ_DATA_ERROR;

	ret = xz_dec_lzma2_reset(s->lzma2, s->temp.buf[s->temp.pos++]);
	if (ret != XZ_OK)
		return ret;

	/* The rest must be Header Padding. */
	while (s->temp.pos < s->temp.size)
		if (s->temp.buf[s->temp.pos++] != 0x00)
			return XZ_OPTIONS_ERROR;

	s->temp.pos = 0;
	s->block.compressed = 0;
	s->block.uncompressed = 0;

	return XZ_OK;
}

static enum xz_ret dec_main(struct xz_dec *s, struct xz_buf *b)
{
	enum xz_ret ret;

	/*
	 * Store the start position for the case when we are in the middle
	 * of the Index field.
	 */
	s->in_start = b->in_pos;

	while (true) {
		switch (s->sequence) {
		case SEQ_STREAM_HEADER:
			/*
			 * Stream Header is copied to s->temp, and then
			 * decoded from there. This way if the caller
			 * gives us only little input at a time, we can
			 * still keep the Stream Header decoding code
			 * simple. Similar approach is used in many places
			 * in this file.
			 */
			if (!fill_temp(s, b))
				return XZ_OK;

			/*
			 * If dec_stream_header() returns
			 * XZ_UNSUPPORTED_CHECK, it is still possible
			 * to continue decoding if working in multi-call
			 * mode. Thus, update s->sequence before calling
			 * dec_stream_header().
			 */
			s->sequence = SEQ_BLOCK_START;

			ret = dec_stream_header(s);
			if (ret != XZ_OK)
				return ret;

		case SEQ_BLOCK_START:
			/* We need one byte of input to continue. */
			if (b->in_pos == b->in_size)
				return XZ_OK;

			/* See if this is the beginning of the Index field. */
			if (b->in[b->in_pos] == 0) {
				s->in_start = b->in_pos++;
				s->sequence = SEQ_INDEX;
				break;
			}

			/*
			 * Calculate the size of the Block Header and
			 * prepare to decode it.
			 */
			s->block_header.size
				= ((uint32_t)b->in[b->in_pos] + 1) * 4;

			s->temp.size = s->block_header.size;
			s->temp.pos = 0;
			s->sequence = SEQ_BLOCK_HEADER;

		case SEQ_BLOCK_HEADER:
			if (!fill_temp(s, b))
				return XZ_OK;

			ret = dec_block_header(s);
			if (ret != XZ_OK)
				return ret;

			s->sequence = SEQ_BLOCK_UNCOMPRESS;

		case SEQ_BLOCK_UNCOMPRESS:
			ret = dec_block(s, b);
			if (ret != XZ_STREAM_END)
				return ret;

			s->sequence = SEQ_BLOCK_PADDING;

		case SEQ_BLOCK_PADDING:
			/*
			 * Size of Compressed Data + Block Padding
			 * must be a multiple of four. We don't need
			 * s->block.compressed for anything else
			 * anymore, so we use it here to test the size
			 * of the Block Padding field.
			 */
			while (s->block.compressed & 3) {
				if (b->in_pos == b->in_size)
					return XZ_OK;

				if (b->in[b->in_pos++] != 0)
					return XZ_DATA_ERROR;

				++s->block.compressed;
			}

			s->sequence = SEQ_BLOCK_CHECK;

		case SEQ_BLOCK_CHECK:
			if (s->check_type == XZ_CHECK_CRC32) {
				ret = crc_validate(s, b, 32);
				if (ret != XZ_STREAM_END)
					return ret;
			}
			else if (IS_CRC64(s->check_type)) {
				ret = crc_validate(s, b, 64);
				if (ret != XZ_STREAM_END)
					return ret;
			}
#ifdef XZ_DEC_ANY_CHECK
			else if (!check_skip(s, b)) {
				return XZ_OK;
			}
#endif

			s->sequence = SEQ_BLOCK_START;
			break;

		case SEQ_INDEX:
			ret = dec_index(s, b);
			if (ret != XZ_STREAM_END)
				return ret;

			s->sequence = SEQ_INDEX_PADDING;

		case SEQ_INDEX_PADDING:
			while ((s->index.size + (b->in_pos - s->in_start))
					& 3) {
				if (b->in_pos == b->in_size) {
					index_update(s, b);
					return XZ_OK;
				}

				if (b->in[b->in_pos++] != 0)
					return XZ_DATA_ERROR;
			}

			/* Finish the CRC32 value and Index size. */
			index_update(s, b);

			/* Compare the hashes to validate the Index field. */
			if (!memeq(&s->block.hash, &s->index.hash,
					sizeof(s->block.hash)))
				return XZ_DATA_ERROR;

			s->sequence = SEQ_INDEX_CRC32;

		case SEQ_INDEX_CRC32:
			ret = crc_validate(s, b, 32);
			if (ret != XZ_STREAM_END)
				return ret;

			s->temp.size = STREAM_HEADER_SIZE;
			s->sequence = SEQ_STREAM_FOOTER;

		case SEQ_STREAM_FOOTER:
			if (!fill_temp(s, b))
				return XZ_OK;

			return dec_stream_footer(s);
		}
	}

	/* Never reached */
}

/*
 * xz_dec_run() is a wrapper for dec_main() to handle some special cases in
 * multi-call and single-call decoding.
 *
 * In multi-call mode, we must return XZ_BUF_ERROR when it seems clear that we
 * are not going to make any progress anymore. This is to prevent the caller
 * from calling us infinitely when the input file is truncated or otherwise
 * corrupt. Since zlib-style API allows that the caller fills the input buffer
 * only when the decoder doesn't produce any new output, we have to be careful
 * to avoid returning XZ_BUF_ERROR too easily: XZ_BUF_ERROR is returned only
 * after the second consecutive call to xz_dec_run() that makes no progress.
 *
 * In single-call mode, if we couldn't decode everything and no error
 * occurred, either the input is truncated or the output buffer is too small.
 * Since we know that the last input byte never produces any output, we know
 * that if all the input was consumed and decoding wasn't finished, the file
 * must be corrupt. Otherwise the output buffer has to be too small or the
 * file is corrupt in a way that decoding it produces too big output.
 *
 * If single-call decoding fails, we reset b->in_pos and b->out_pos back to
 * their original values. This is because with some filter chains there won't
 * be any valid uncompressed data in the output buffer unless the decoding
 * actually succeeds (that's the price to pay of using the output buffer as
 * the workspace).
 */
XZ_EXTERN enum xz_ret xz_dec_run(struct xz_dec *s, struct xz_buf *b)
{
	size_t in_start;
	size_t out_start;
	enum xz_ret ret;

	if (DEC_IS_SINGLE(s->mode))
		xz_dec_reset(s);

	in_start = b->in_pos;
	out_start = b->out_pos;
	ret = dec_main(s, b);

	if (DEC_IS_SINGLE(s->mode)) {
		if (ret == XZ_OK)
			ret = b->in_pos == b->in_size
					? XZ_DATA_ERROR : XZ_BUF_ERROR;

		if (ret != XZ_STREAM_END) {
			b->in_pos = in_start;
			b->out_pos = out_start;
		}

	} else if (ret == XZ_OK && in_start == b->in_pos
			&& out_start == b->out_pos) {
		if (s->allow_buf_error)
			ret = XZ_BUF_ERROR;

		s->allow_buf_error = true;
	} else {
		s->allow_buf_error = false;
	}

	return ret;
}

XZ_EXTERN struct xz_dec *xz_dec_init(enum xz_mode mode, uint32_t dict_max)
{
	struct xz_dec *s = kmalloc(sizeof(*s), GFP_KERNEL);
	if (s == NULL)
		return NULL;

	s->mode = mode;

#ifdef XZ_DEC_BCJ
	s->bcj = xz_dec_bcj_create(DEC_IS_SINGLE(mode));
	if (s->bcj == NULL)
		goto error_bcj;
#endif

	s->lzma2 = xz_dec_lzma2_create(mode, dict_max);
	if (s->lzma2 == NULL)
		goto error_lzma2;

	xz_dec_reset(s);
	return s;

error_lzma2:
#ifdef XZ_DEC_BCJ
	xz_dec_bcj_end(s->bcj);
error_bcj:
#endif
	kfree(s);
	return NULL;
}

XZ_EXTERN void xz_dec_reset(struct xz_dec *s)
{
	s->sequence = SEQ_STREAM_HEADER;
	s->allow_buf_error = false;
	s->pos = 0;
	s->crc = 0;
	memzero(&s->block, sizeof(s->block));
	memzero(&s->index, sizeof(s->index));
	s->temp.pos = 0;
	s->temp.size = STREAM_HEADER_SIZE;
}

XZ_EXTERN void xz_dec_end(struct xz_dec *s)
{
	if (s != NULL) {
		xz_dec_lzma2_end(s->lzma2);
#ifdef XZ_DEC_BCJ
		xz_dec_bcj_end(s->bcj);
#endif
		kfree(s);
	}
}

//END xz_dec_stream.c
//BEGIN xz_dec_lzma2.c
/*
 * LZMA2 decoder
 *
 * Authors: Lasse Collin <lasse.collin@tukaani.org>
 *          Igor Pavlov <http://7-zip.org/>
 *
 * This file has been put into the public domain.
 * You can do whatever you want with this file.
 */

/*
 * Range decoder initialization eats the first five bytes of each LZMA chunk.
 */
#define RC_INIT_BYTES 5

/*
 * Minimum number of usable input buffer to safely decode one LZMA symbol.
 * The worst case is that we decode 22 bits using probabilities and 26
 * direct bits. This may decode at maximum of 20 bytes of input. However,
 * lzma_main() does an extra normalization before returning, thus we
 * need to put 21 here.
 */
#define LZMA_IN_REQUIRED 21

/*
 * Dictionary (history buffer)
 *
 * These are always true:
 *    start <= pos <= full <= end
 *    pos <= limit <= end
 *
 * In multi-call mode, also these are true:
 *    end == size
 *    size <= size_max
 *    allocated <= size
 *
 * Most of these variables are size_t to support single-call mode,
 * in which the dictionary variables address the actual output
 * buffer directly.
 */
struct dictionary {
	/* Beginning of the history buffer */
	uint8_t *buf;

	/* Old position in buf (before decoding more data) */
	size_t start;

	/* Position in buf */
	size_t pos;

	/*
	 * How full dictionary is. This is used to detect corrupt input that
	 * would read beyond the beginning of the uncompressed stream.
	 */
	size_t full;

	/* Write limit; we don't write to buf[limit] or later bytes. */
	size_t limit;

	/*
	 * End of the dictionary buffer. In multi-call mode, this is
	 * the same as the dictionary size. In single-call mode, this
	 * indicates the size of the output buffer.
	 */
	size_t end;

	/*
	 * Size of the dictionary as specified in Block Header. This is used
	 * together with "full" to detect corrupt input that would make us
	 * read beyond the beginning of the uncompressed stream.
	 */
	uint32_t size;

	/*
	 * Maximum allowed dictionary size in multi-call mode.
	 * This is ignored in single-call mode.
	 */
	uint32_t size_max;

	/*
	 * Amount of memory currently allocated for the dictionary.
	 * This is used only with XZ_DYNALLOC. (With XZ_PREALLOC,
	 * size_max is always the same as the allocated size.)
	 */
	uint32_t allocated;

	/* Operation mode */
	enum xz_mode mode;
};

/* Range decoder */
struct rc_dec {
	uint32_t range;
	uint32_t code;

	/*
	 * Number of initializing bytes remaining to be read
	 * by rc_read_init().
	 */
	uint32_t init_bytes_left;

	/*
	 * Buffer from which we read our input. It can be either
	 * temp.buf or the caller-provided input buffer.
	 */
	const uint8_t *in;
	size_t in_pos;
	size_t in_limit;
};

/* Probabilities for a length decoder. */
struct lzma_len_dec {
	/* Probability of match length being at least 10 */
	uint16_t choice;

	/* Probability of match length being at least 18 */
	uint16_t choice2;

	/* Probabilities for match lengths 2-9 */
	uint16_t low[POS_STATES_MAX][LEN_LOW_SYMBOLS];

	/* Probabilities for match lengths 10-17 */
	uint16_t mid[POS_STATES_MAX][LEN_MID_SYMBOLS];

	/* Probabilities for match lengths 18-273 */
	uint16_t high[LEN_HIGH_SYMBOLS];
};

struct lzma_dec {
	/* Distances of latest four matches */
	uint32_t rep0;
	uint32_t rep1;
	uint32_t rep2;
	uint32_t rep3;

	/* Types of the most recently seen LZMA symbols */
	enum lzma_state state;

	/*
	 * Length of a match. This is updated so that dict_repeat can
	 * be called again to finish repeating the whole match.
	 */
	uint32_t len;

	/*
	 * LZMA properties or related bit masks (number of literal
	 * context bits, a mask dervied from the number of literal
	 * position bits, and a mask dervied from the number
	 * position bits)
	 */
	uint32_t lc;
	uint32_t literal_pos_mask; /* (1 << lp) - 1 */
	uint32_t pos_mask;         /* (1 << pb) - 1 */

	/* If 1, it's a match. Otherwise it's a single 8-bit literal. */
	uint16_t is_match[STATES][POS_STATES_MAX];

	/* If 1, it's a repeated match. The distance is one of rep0 .. rep3. */
	uint16_t is_rep[STATES];

	/*
	 * If 0, distance of a repeated match is rep0.
	 * Otherwise check is_rep1.
	 */
	uint16_t is_rep0[STATES];

	/*
	 * If 0, distance of a repeated match is rep1.
	 * Otherwise check is_rep2.
	 */
	uint16_t is_rep1[STATES];

	/* If 0, distance of a repeated match is rep2. Otherwise it is rep3. */
	uint16_t is_rep2[STATES];

	/*
	 * If 1, the repeated match has length of one byte. Otherwise
	 * the length is decoded from rep_len_decoder.
	 */
	uint16_t is_rep0_long[STATES][POS_STATES_MAX];

	/*
	 * Probability tree for the highest two bits of the match
	 * distance. There is a separate probability tree for match
	 * lengths of 2 (i.e. MATCH_LEN_MIN), 3, 4, and [5, 273].
	 */
	uint16_t dist_slot[DIST_STATES][DIST_SLOTS];

	/*
	 * Probility trees for additional bits for match distance
	 * when the distance is in the range [4, 127].
	 */
	uint16_t dist_special[FULL_DISTANCES - DIST_MODEL_END];

	/*
	 * Probability tree for the lowest four bits of a match
	 * distance that is equal to or greater than 128.
	 */
	uint16_t dist_align[ALIGN_SIZE];

	/* Length of a normal match */
	struct lzma_len_dec match_len_dec;

	/* Length of a repeated match */
	struct lzma_len_dec rep_len_dec;

	/* Probabilities of literals */
	uint16_t literal[LITERAL_CODERS_MAX][LITERAL_CODER_SIZE];
};

struct lzma2_dec {
	/* Position in xz_dec_lzma2_run(). */
	enum lzma2_seq {
		SEQ_CONTROL,
		SEQ_UNCOMPRESSED_1,
		SEQ_UNCOMPRESSED_2,
		SEQ_COMPRESSED_0,
		SEQ_COMPRESSED_1,
		SEQ_PROPERTIES,
		SEQ_LZMA_PREPARE,
		SEQ_LZMA_RUN,
		SEQ_COPY
	} sequence;

	/* Next position after decoding the compressed size of the chunk. */
	enum lzma2_seq next_sequence;

	/* Uncompressed size of LZMA chunk (2 MiB at maximum) */
	uint32_t uncompressed;

	/*
	 * Compressed size of LZMA chunk or compressed/uncompressed
	 * size of uncompressed chunk (64 KiB at maximum)
	 */
	uint32_t compressed;

	/*
	 * True if dictionary reset is needed. This is false before
	 * the first chunk (LZMA or uncompressed).
	 */
	bool need_dict_reset;

	/*
	 * True if new LZMA properties are needed. This is false
	 * before the first LZMA chunk.
	 */
	bool need_props;
};

struct xz_dec_lzma2 {
	/*
	 * The order below is important on x86 to reduce code size and
	 * it shouldn't hurt on other platforms. Everything up to and
	 * including lzma.pos_mask are in the first 128 bytes on x86-32,
	 * which allows using smaller instructions to access those
	 * variables. On x86-64, fewer variables fit into the first 128
	 * bytes, but this is still the best order without sacrificing
	 * the readability by splitting the structures.
	 */
	struct rc_dec rc;
	struct dictionary dict;
	struct lzma2_dec lzma2;
	struct lzma_dec lzma;

	/*
	 * Temporary buffer which holds small number of input bytes between
	 * decoder calls. See lzma2_lzma() for details.
	 */
	struct {
		uint32_t size;
		uint8_t buf[3 * LZMA_IN_REQUIRED];
	} temp;
};

/**************
 * Dictionary *
 **************/

/*
 * Reset the dictionary state. When in single-call mode, set up the beginning
 * of the dictionary to point to the actual output buffer.
 */
static void dict_reset(struct dictionary *dict, struct xz_buf *b)
{
	if (DEC_IS_SINGLE(dict->mode)) {
		dict->buf = b->out + b->out_pos;
		dict->end = b->out_size - b->out_pos;
	}

	dict->start = 0;
	dict->pos = 0;
	dict->limit = 0;
	dict->full = 0;
}

/* Set dictionary write limit */
static void dict_limit(struct dictionary *dict, size_t out_max)
{
	if (dict->end - dict->pos <= out_max)
		dict->limit = dict->end;
	else
		dict->limit = dict->pos + out_max;
}

/* Return true if at least one byte can be written into the dictionary. */
static inline bool dict_has_space(const struct dictionary *dict)
{
	return dict->pos < dict->limit;
}

/*
 * Get a byte from the dictionary at the given distance. The distance is
 * assumed to valid, or as a special case, zero when the dictionary is
 * still empty. This special case is needed for single-call decoding to
 * avoid writing a '\0' to the end of the destination buffer.
 */
static inline uint32_t dict_get(const struct dictionary *dict, uint32_t dist)
{
	size_t offset = dict->pos - dist - 1;

	if (dist >= dict->pos)
		offset += dict->end;

	return dict->full > 0 ? dict->buf[offset] : 0;
}

/*
 * Put one byte into the dictionary. It is assumed that there is space for it.
 */
static inline void dict_put(struct dictionary *dict, uint8_t byte)
{
	dict->buf[dict->pos++] = byte;

	if (dict->full < dict->pos)
		dict->full = dict->pos;
}

/*
 * Repeat given number of bytes from the given distance. If the distance is
 * invalid, false is returned. On success, true is returned and *len is
 * updated to indicate how many bytes were left to be repeated.
 */
static bool dict_repeat(struct dictionary *dict, uint32_t *len, uint32_t dist)
{
	size_t back;
	uint32_t left;

	if (dist >= dict->full || dist >= dict->size)
		return false;

	left = min_t(size_t, dict->limit - dict->pos, *len);
	*len -= left;

	back = dict->pos - dist - 1;
	if (dist >= dict->pos)
		back += dict->end;

	do {
		dict->buf[dict->pos++] = dict->buf[back++];
		if (back == dict->end)
			back = 0;
	} while (--left > 0);

	if (dict->full < dict->pos)
		dict->full = dict->pos;

	return true;
}

/* Copy uncompressed data as is from input to dictionary and output buffers. */
static void dict_uncompressed(struct dictionary *dict, struct xz_buf *b,
			      uint32_t *left)
{
	size_t copy_size;

	while (*left > 0 && b->in_pos < b->in_size
			&& b->out_pos < b->out_size) {
		copy_size = min(b->in_size - b->in_pos,
				b->out_size - b->out_pos);
		if (copy_size > dict->end - dict->pos)
			copy_size = dict->end - dict->pos;
		if (copy_size > *left)
			copy_size = *left;

		*left -= copy_size;

		memcpy(dict->buf + dict->pos, b->in + b->in_pos, copy_size);
		dict->pos += copy_size;

		if (dict->full < dict->pos)
			dict->full = dict->pos;

		if (DEC_IS_MULTI(dict->mode)) {
			if (dict->pos == dict->end)
				dict->pos = 0;

			memcpy(b->out + b->out_pos, b->in + b->in_pos,
					copy_size);
		}

		dict->start = dict->pos;

		b->out_pos += copy_size;
		b->in_pos += copy_size;
	}
}

/*
 * Flush pending data from dictionary to b->out. It is assumed that there is
 * enough space in b->out. This is guaranteed because caller uses dict_limit()
 * before decoding data into the dictionary.
 */
static uint32_t dict_flush(struct dictionary *dict, struct xz_buf *b)
{
	size_t copy_size = dict->pos - dict->start;

	if (DEC_IS_MULTI(dict->mode)) {
		if (dict->pos == dict->end)
			dict->pos = 0;

		memcpy(b->out + b->out_pos, dict->buf + dict->start,
				copy_size);
	}

	dict->start = dict->pos;
	b->out_pos += copy_size;
	return copy_size;
}

/*****************
 * Range decoder *
 *****************/

/* Reset the range decoder. */
static void rc_reset(struct rc_dec *rc)
{
	rc->range = (uint32_t)-1;
	rc->code = 0;
	rc->init_bytes_left = RC_INIT_BYTES;
}

/*
 * Read the first five initial bytes into rc->code if they haven't been
 * read already. (Yes, the first byte gets completely ignored.)
 */
static bool rc_read_init(struct rc_dec *rc, struct xz_buf *b)
{
	while (rc->init_bytes_left > 0) {
		if (b->in_pos == b->in_size)
			return false;

		rc->code = (rc->code << 8) + b->in[b->in_pos++];
		--rc->init_bytes_left;
	}

	return true;
}

/* Return true if there may not be enough input for the next decoding loop. */
static inline bool rc_limit_exceeded(const struct rc_dec *rc)
{
	return rc->in_pos > rc->in_limit;
}

/*
 * Return true if it is possible (from point of view of range decoder) that
 * we have reached the end of the LZMA chunk.
 */
static inline bool rc_is_finished(const struct rc_dec *rc)
{
	return rc->code == 0;
}

/* Read the next input byte if needed. */
static always_inline void rc_normalize(struct rc_dec *rc)
{
	if (rc->range < RC_TOP_VALUE) {
		rc->range <<= RC_SHIFT_BITS;
		rc->code = (rc->code << RC_SHIFT_BITS) + rc->in[rc->in_pos++];
	}
}

/*
 * Decode one bit. In some versions, this function has been splitted in three
 * functions so that the compiler is supposed to be able to more easily avoid
 * an extra branch. In this particular version of the LZMA decoder, this
 * doesn't seem to be a good idea (tested with GCC 3.3.6, 3.4.6, and 4.3.3
 * on x86). Using a non-splitted version results in nicer looking code too.
 *
 * NOTE: This must return an int. Do not make it return a bool or the speed
 * of the code generated by GCC 3.x decreases 10-15 %. (GCC 4.3 doesn't care,
 * and it generates 10-20 % faster code than GCC 3.x from this file anyway.)
 */
static always_inline int rc_bit(struct rc_dec *rc, uint16_t *prob)
{
	uint32_t bound;
	int bit;

	rc_normalize(rc);
	bound = (rc->range >> RC_BIT_MODEL_TOTAL_BITS) * *prob;
	if (rc->code < bound) {
		rc->range = bound;
		*prob += (RC_BIT_MODEL_TOTAL - *prob) >> RC_MOVE_BITS;
		bit = 0;
	} else {
		rc->range -= bound;
		rc->code -= bound;
		*prob -= *prob >> RC_MOVE_BITS;
		bit = 1;
	}

	return bit;
}

/* Decode a bittree starting from the most significant bit. */
static always_inline uint32_t rc_bittree(struct rc_dec *rc,
					   uint16_t *probs, uint32_t limit)
{
	uint32_t symbol = 1;

	do {
		if (rc_bit(rc, &probs[symbol]))
			symbol = (symbol << 1) + 1;
		else
			symbol <<= 1;
	} while (symbol < limit);

	return symbol;
}

/* Decode a bittree starting from the least significant bit. */
static always_inline void rc_bittree_reverse(struct rc_dec *rc,
					       uint16_t *probs,
					       uint32_t *dest, uint32_t limit)
{
	uint32_t symbol = 1;
	uint32_t i = 0;

	do {
		if (rc_bit(rc, &probs[symbol])) {
			symbol = (symbol << 1) + 1;
			*dest += 1 << i;
		} else {
			symbol <<= 1;
		}
	} while (++i < limit);
}

/* Decode direct bits (fixed fifty-fifty probability) */
static inline void rc_direct(struct rc_dec *rc, uint32_t *dest, uint32_t limit)
{
	uint32_t mask;

	do {
		rc_normalize(rc);
		rc->range >>= 1;
		rc->code -= rc->range;
		mask = (uint32_t)0 - (rc->code >> 31);
		rc->code += rc->range & mask;
		*dest = (*dest << 1) + (mask + 1);
	} while (--limit > 0);
}

/********
 * LZMA *
 ********/

/* Get pointer to literal coder probability array. */
static uint16_t *lzma_literal_probs(struct xz_dec_lzma2 *s)
{
	uint32_t prev_byte = dict_get(&s->dict, 0);
	uint32_t low = prev_byte >> (8 - s->lzma.lc);
	uint32_t high = (s->dict.pos & s->lzma.literal_pos_mask) << s->lzma.lc;
	return s->lzma.literal[low + high];
}

/* Decode a literal (one 8-bit byte) */
static void lzma_literal(struct xz_dec_lzma2 *s)
{
	uint16_t *probs;
	uint32_t symbol;
	uint32_t match_byte;
	uint32_t match_bit;
	uint32_t offset;
	uint32_t i;

	probs = lzma_literal_probs(s);

	if (lzma_state_is_literal(s->lzma.state)) {
		symbol = rc_bittree(&s->rc, probs, 0x100);
	} else {
		symbol = 1;
		match_byte = dict_get(&s->dict, s->lzma.rep0) << 1;
		offset = 0x100;

		do {
			match_bit = match_byte & offset;
			match_byte <<= 1;
			i = offset + match_bit + symbol;

			if (rc_bit(&s->rc, &probs[i])) {
				symbol = (symbol << 1) + 1;
				offset &= match_bit;
			} else {
				symbol <<= 1;
				offset &= ~match_bit;
			}
		} while (symbol < 0x100);
	}

	dict_put(&s->dict, (uint8_t)(symbol&0xff));
	lzma_state_literal(&s->lzma.state);
}

/* Decode the length of the match into s->lzma.len. */
static void lzma_len(struct xz_dec_lzma2 *s, struct lzma_len_dec *l,
		     uint32_t pos_state)
{
	uint16_t *probs;
	uint32_t limit;

	if (!rc_bit(&s->rc, &l->choice)) {
		probs = l->low[pos_state];
		limit = LEN_LOW_SYMBOLS;
		s->lzma.len = MATCH_LEN_MIN;
	} else {
		if (!rc_bit(&s->rc, &l->choice2)) {
			probs = l->mid[pos_state];
			limit = LEN_MID_SYMBOLS;
			s->lzma.len = MATCH_LEN_MIN + LEN_LOW_SYMBOLS;
		} else {
			probs = l->high;
			limit = LEN_HIGH_SYMBOLS;
			s->lzma.len = MATCH_LEN_MIN + LEN_LOW_SYMBOLS
					+ LEN_MID_SYMBOLS;
		}
	}

	s->lzma.len += rc_bittree(&s->rc, probs, limit) - limit;
}

/* Decode a match. The distance will be stored in s->lzma.rep0. */
static void lzma_match(struct xz_dec_lzma2 *s, uint32_t pos_state)
{
	uint16_t *probs;
	uint32_t dist_slot;
	uint32_t limit;

	lzma_state_match(&s->lzma.state);

	s->lzma.rep3 = s->lzma.rep2;
	s->lzma.rep2 = s->lzma.rep1;
	s->lzma.rep1 = s->lzma.rep0;

	lzma_len(s, &s->lzma.match_len_dec, pos_state);

	probs = s->lzma.dist_slot[lzma_get_dist_state(s->lzma.len)];
	dist_slot = rc_bittree(&s->rc, probs, DIST_SLOTS) - DIST_SLOTS;

	if (dist_slot < DIST_MODEL_START) {
		s->lzma.rep0 = dist_slot;
	} else {
		limit = (dist_slot >> 1) - 1;
		s->lzma.rep0 = 2 + (dist_slot & 1);

		if (dist_slot < DIST_MODEL_END) {
			s->lzma.rep0 <<= limit;
			probs = s->lzma.dist_special + s->lzma.rep0
					- dist_slot - 1;
			rc_bittree_reverse(&s->rc, probs,
					&s->lzma.rep0, limit);
		} else {
			rc_direct(&s->rc, &s->lzma.rep0, limit - ALIGN_BITS);
			s->lzma.rep0 <<= ALIGN_BITS;
			rc_bittree_reverse(&s->rc, s->lzma.dist_align,
					&s->lzma.rep0, ALIGN_BITS);
		}
	}
}

/*
 * Decode a repeated match. The distance is one of the four most recently
 * seen matches. The distance will be stored in s->lzma.rep0.
 */
static void lzma_rep_match(struct xz_dec_lzma2 *s, uint32_t pos_state)
{
	uint32_t tmp;

	if (!rc_bit(&s->rc, &s->lzma.is_rep0[s->lzma.state])) {
		if (!rc_bit(&s->rc, &s->lzma.is_rep0_long[
				s->lzma.state][pos_state])) {
			lzma_state_short_rep(&s->lzma.state);
			s->lzma.len = 1;
			return;
		}
	} else {
		if (!rc_bit(&s->rc, &s->lzma.is_rep1[s->lzma.state])) {
			tmp = s->lzma.rep1;
		} else {
			if (!rc_bit(&s->rc, &s->lzma.is_rep2[s->lzma.state])) {
				tmp = s->lzma.rep2;
			} else {
				tmp = s->lzma.rep3;
				s->lzma.rep3 = s->lzma.rep2;
			}

			s->lzma.rep2 = s->lzma.rep1;
		}

		s->lzma.rep1 = s->lzma.rep0;
		s->lzma.rep0 = tmp;
	}

	lzma_state_long_rep(&s->lzma.state);
	lzma_len(s, &s->lzma.rep_len_dec, pos_state);
}

/* LZMA decoder core */
static bool lzma_main(struct xz_dec_lzma2 *s)
{
	uint32_t pos_state;

	/*
	 * If the dictionary was reached during the previous call, try to
	 * finish the possibly pending repeat in the dictionary.
	 */
	if (dict_has_space(&s->dict) && s->lzma.len > 0)
		dict_repeat(&s->dict, &s->lzma.len, s->lzma.rep0);

	/*
	 * Decode more LZMA symbols. One iteration may consume up to
	 * LZMA_IN_REQUIRED - 1 bytes.
	 */
	while (dict_has_space(&s->dict) && !rc_limit_exceeded(&s->rc)) {
		pos_state = s->dict.pos & s->lzma.pos_mask;

		if (!rc_bit(&s->rc, &s->lzma.is_match[
				s->lzma.state][pos_state])) {
			lzma_literal(s);
		} else {
			if (rc_bit(&s->rc, &s->lzma.is_rep[s->lzma.state]))
				lzma_rep_match(s, pos_state);
			else
				lzma_match(s, pos_state);

			if (!dict_repeat(&s->dict, &s->lzma.len, s->lzma.rep0))
				return false;
		}
	}

	/*
	 * Having the range decoder always normalized when we are outside
	 * this function makes it easier to correctly handle end of the chunk.
	 */
	rc_normalize(&s->rc);

	return true;
}

/*
 * Reset the LZMA decoder and range decoder state. Dictionary is nore reset
 * here, because LZMA state may be reset without resetting the dictionary.
 */
static void lzma_reset(struct xz_dec_lzma2 *s)
{
	uint16_t *probs;
	size_t i;

	s->lzma.state = STATE_LIT_LIT;
	s->lzma.rep0 = 0;
	s->lzma.rep1 = 0;
	s->lzma.rep2 = 0;
	s->lzma.rep3 = 0;

	/*
	 * All probabilities are initialized to the same value. This hack
	 * makes the code smaller by avoiding a separate loop for each
	 * probability array.
	 *
	 * This could be optimized so that only that part of literal
	 * probabilities that are actually required. In the common case
	 * we would write 12 KiB less.
	 */
	probs = s->lzma.is_match[0];
	for (i = 0; i < PROBS_TOTAL; ++i)
		probs[i] = RC_BIT_MODEL_TOTAL / 2;

	rc_reset(&s->rc);
}

/*
 * Decode and validate LZMA properties (lc/lp/pb) and calculate the bit masks
 * from the decoded lp and pb values. On success, the LZMA decoder state is
 * reset and true is returned.
 */
static bool lzma_props(struct xz_dec_lzma2 *s, uint8_t props)
{
	if (props > (4 * 5 + 4) * 9 + 8)
		return false;

	s->lzma.pos_mask = 0;
	while (props >= 9 * 5) {
		props -= 9 * 5;
		++s->lzma.pos_mask;
	}

	s->lzma.pos_mask = (1 << s->lzma.pos_mask) - 1;

	s->lzma.literal_pos_mask = 0;
	while (props >= 9) {
		props -= 9;
		++s->lzma.literal_pos_mask;
	}

	s->lzma.lc = props;

	if (s->lzma.lc + s->lzma.literal_pos_mask > 4)
		return false;

	s->lzma.literal_pos_mask = (1 << s->lzma.literal_pos_mask) - 1;

	lzma_reset(s);

	return true;
}

/*********
 * LZMA2 *
 *********/

/*
 * The LZMA decoder assumes that if the input limit (s->rc.in_limit) hasn't
 * been exceeded, it is safe to read up to LZMA_IN_REQUIRED bytes. This
 * wrapper function takes care of making the LZMA decoder's assumption safe.
 *
 * As long as there is plenty of input left to be decoded in the current LZMA
 * chunk, we decode directly from the caller-supplied input buffer until
 * there's LZMA_IN_REQUIRED bytes left. Those remaining bytes are copied into
 * s->temp.buf, which (hopefully) gets filled on the next call to this
 * function. We decode a few bytes from the temporary buffer so that we can
 * continue decoding from the caller-supplied input buffer again.
 */
static bool lzma2_lzma(struct xz_dec_lzma2 *s, struct xz_buf *b)
{
	size_t in_avail;
	uint32_t tmp;

	in_avail = b->in_size - b->in_pos;
	if (s->temp.size > 0 || s->lzma2.compressed == 0) {
		tmp = 2 * LZMA_IN_REQUIRED - s->temp.size;
		if (tmp > s->lzma2.compressed - s->temp.size)
			tmp = s->lzma2.compressed - s->temp.size;
		if (tmp > in_avail)
			tmp = in_avail;

		memcpy(s->temp.buf + s->temp.size, b->in + b->in_pos, tmp);

		if (s->temp.size + tmp == s->lzma2.compressed) {
			memzero(s->temp.buf + s->temp.size + tmp,
					sizeof(s->temp.buf)
						- s->temp.size - tmp);
			s->rc.in_limit = s->temp.size + tmp;
		} else if (s->temp.size + tmp < LZMA_IN_REQUIRED) {
			s->temp.size += tmp;
			b->in_pos += tmp;
			return true;
		} else {
			s->rc.in_limit = s->temp.size + tmp - LZMA_IN_REQUIRED;
		}

		s->rc.in = s->temp.buf;
		s->rc.in_pos = 0;

		if (!lzma_main(s) || s->rc.in_pos > s->temp.size + tmp)
			return false;

		s->lzma2.compressed -= s->rc.in_pos;

		if (s->rc.in_pos < s->temp.size) {
			s->temp.size -= s->rc.in_pos;
			memmove(s->temp.buf, s->temp.buf + s->rc.in_pos,
					s->temp.size);
			return true;
		}

		b->in_pos += s->rc.in_pos - s->temp.size;
		s->temp.size = 0;
	}

	in_avail = b->in_size - b->in_pos;
	if (in_avail >= LZMA_IN_REQUIRED) {
		s->rc.in = b->in;
		s->rc.in_pos = b->in_pos;

		if (in_avail >= s->lzma2.compressed + LZMA_IN_REQUIRED)
			s->rc.in_limit = b->in_pos + s->lzma2.compressed;
		else
			s->rc.in_limit = b->in_size - LZMA_IN_REQUIRED;

		if (!lzma_main(s))
			return false;

		in_avail = s->rc.in_pos - b->in_pos;
		if (in_avail > s->lzma2.compressed)
			return false;

		s->lzma2.compressed -= in_avail;
		b->in_pos = s->rc.in_pos;
	}

	in_avail = b->in_size - b->in_pos;
	if (in_avail < LZMA_IN_REQUIRED) {
		if (in_avail > s->lzma2.compressed)
			in_avail = s->lzma2.compressed;

		memcpy(s->temp.buf, b->in + b->in_pos, in_avail);
		s->temp.size = in_avail;
		b->in_pos += in_avail;
	}

	return true;
}

/*
 * Take care of the LZMA2 control layer, and forward the job of actual LZMA
 * decoding or copying of uncompressed chunks to other functions.
 */
XZ_EXTERN enum xz_ret xz_dec_lzma2_run(struct xz_dec_lzma2 *s,
				       struct xz_buf *b)
{
	uint32_t tmp;

	while (b->in_pos < b->in_size || s->lzma2.sequence == SEQ_LZMA_RUN) {
		switch (s->lzma2.sequence) {
		case SEQ_CONTROL:
			/*
			 * LZMA2 control byte
			 *
			 * Exact values:
			 *   0x00   End marker
			 *   0x01   Dictionary reset followed by
			 *          an uncompressed chunk
			 *   0x02   Uncompressed chunk (no dictionary reset)
			 *
			 * Highest three bits (s->control & 0xE0):
			 *   0xE0   Dictionary reset, new properties and state
			 *          reset, followed by LZMA compressed chunk
			 *   0xC0   New properties and state reset, followed
			 *          by LZMA compressed chunk (no dictionary
			 *          reset)
			 *   0xA0   State reset using old properties,
			 *          followed by LZMA compressed chunk (no
			 *          dictionary reset)
			 *   0x80   LZMA chunk (no dictionary or state reset)
			 *
			 * For LZMA compressed chunks, the lowest five bits
			 * (s->control & 1F) are the highest bits of the
			 * uncompressed size (bits 16-20).
			 *
			 * A new LZMA2 stream must begin with a dictionary
			 * reset. The first LZMA chunk must set new
			 * properties and reset the LZMA state.
			 *
			 * Values that don't match anything described above
			 * are invalid and we return XZ_DATA_ERROR.
			 */
			tmp = b->in[b->in_pos++];

			if (tmp == 0x00)
				return XZ_STREAM_END;

			if (tmp >= 0xE0 || tmp == 0x01) {
				s->lzma2.need_props = true;
				s->lzma2.need_dict_reset = false;
				dict_reset(&s->dict, b);
			} else if (s->lzma2.need_dict_reset) {
				return XZ_DATA_ERROR;
			}

			if (tmp >= 0x80) {
				s->lzma2.uncompressed = (tmp & 0x1F) << 16;
				s->lzma2.sequence = SEQ_UNCOMPRESSED_1;

				if (tmp >= 0xC0) {
					/*
					 * When there are new properties,
					 * state reset is done at
					 * SEQ_PROPERTIES.
					 */
					s->lzma2.need_props = false;
					s->lzma2.next_sequence
							= SEQ_PROPERTIES;

				} else if (s->lzma2.need_props) {
					return XZ_DATA_ERROR;

				} else {
					s->lzma2.next_sequence
							= SEQ_LZMA_PREPARE;
					if (tmp >= 0xA0)
						lzma_reset(s);
				}
			} else {
				if (tmp > 0x02)
					return XZ_DATA_ERROR;

				s->lzma2.sequence = SEQ_COMPRESSED_0;
				s->lzma2.next_sequence = SEQ_COPY;
			}

			break;

		case SEQ_UNCOMPRESSED_1:
			s->lzma2.uncompressed
					+= (uint32_t)b->in[b->in_pos++] << 8;
			s->lzma2.sequence = SEQ_UNCOMPRESSED_2;
			break;

		case SEQ_UNCOMPRESSED_2:
			s->lzma2.uncompressed
					+= (uint32_t)b->in[b->in_pos++] + 1;
			s->lzma2.sequence = SEQ_COMPRESSED_0;
			break;

		case SEQ_COMPRESSED_0:
			s->lzma2.compressed
					= (uint32_t)b->in[b->in_pos++] << 8;
			s->lzma2.sequence = SEQ_COMPRESSED_1;
			break;

		case SEQ_COMPRESSED_1:
			s->lzma2.compressed
					+= (uint32_t)b->in[b->in_pos++] + 1;
			s->lzma2.sequence = s->lzma2.next_sequence;
			break;

		case SEQ_PROPERTIES:
			if (!lzma_props(s, b->in[b->in_pos++]))
				return XZ_DATA_ERROR;

			s->lzma2.sequence = SEQ_LZMA_PREPARE;

		case SEQ_LZMA_PREPARE:
			if (s->lzma2.compressed < RC_INIT_BYTES)
				return XZ_DATA_ERROR;

			if (!rc_read_init(&s->rc, b))
				return XZ_OK;

			s->lzma2.compressed -= RC_INIT_BYTES;
			s->lzma2.sequence = SEQ_LZMA_RUN;

		case SEQ_LZMA_RUN:
			/*
			 * Set dictionary limit to indicate how much we want
			 * to be encoded at maximum. Decode new data into the
			 * dictionary. Flush the new data from dictionary to
			 * b->out. Check if we finished decoding this chunk.
			 * In case the dictionary got full but we didn't fill
			 * the output buffer yet, we may run this loop
			 * multiple times without changing s->lzma2.sequence.
			 */
			dict_limit(&s->dict, min_t(size_t,
					b->out_size - b->out_pos,
					s->lzma2.uncompressed));
			if (!lzma2_lzma(s, b))
				return XZ_DATA_ERROR;

			s->lzma2.uncompressed -= dict_flush(&s->dict, b);

			if (s->lzma2.uncompressed == 0) {
				if (s->lzma2.compressed > 0 || s->lzma.len > 0
						|| !rc_is_finished(&s->rc))
					return XZ_DATA_ERROR;

				rc_reset(&s->rc);
				s->lzma2.sequence = SEQ_CONTROL;

			} else if (b->out_pos == b->out_size
					|| (b->in_pos == b->in_size
						&& s->temp.size
						< s->lzma2.compressed)) {
				return XZ_OK;
			}

			break;

		case SEQ_COPY:
			dict_uncompressed(&s->dict, b, &s->lzma2.compressed);
			if (s->lzma2.compressed > 0)
				return XZ_OK;

			s->lzma2.sequence = SEQ_CONTROL;
			break;
		}
	}

	return XZ_OK;
}

XZ_EXTERN struct xz_dec_lzma2 *xz_dec_lzma2_create(enum xz_mode mode,
						   uint32_t dict_max)
{
	struct xz_dec_lzma2 *s = kmalloc(sizeof(*s), GFP_KERNEL);
	if (s == NULL)
		return NULL;

	s->dict.mode = mode;
	s->dict.size_max = dict_max;

	if (DEC_IS_PREALLOC(mode)) {
		s->dict.buf = vmalloc(dict_max);
		if (s->dict.buf == NULL) {
			kfree(s);
			return NULL;
		}
	} else if (DEC_IS_DYNALLOC(mode)) {
		s->dict.buf = NULL;
		s->dict.allocated = 0;
	}

	return s;
}

XZ_EXTERN enum xz_ret xz_dec_lzma2_reset(struct xz_dec_lzma2 *s, uint8_t props)
{
	/* This limits dictionary size to 3 GiB to keep parsing simpler. */
	if (props > 39)
		return XZ_OPTIONS_ERROR;

	s->dict.size = 2 + (props & 1);
	s->dict.size <<= (props >> 1) + 11;

	if (DEC_IS_MULTI(s->dict.mode)) {
		if (s->dict.size > s->dict.size_max)
			return XZ_MEMLIMIT_ERROR;

		s->dict.end = s->dict.size;

		if (DEC_IS_DYNALLOC(s->dict.mode)) {
			if (s->dict.allocated < s->dict.size) {
				vfree(s->dict.buf);
				s->dict.buf = vmalloc(s->dict.size);
				if (s->dict.buf == NULL) {
					s->dict.allocated = 0;
					return XZ_MEM_ERROR;
				}
			}
		}
	}

	s->lzma.len = 0;

	s->lzma2.sequence = SEQ_CONTROL;
	s->lzma2.need_dict_reset = true;

	s->temp.size = 0;

	return XZ_OK;
}

XZ_EXTERN void xz_dec_lzma2_end(struct xz_dec_lzma2 *s)
{
	if (DEC_IS_MULTI(s->dict.mode))
		vfree(s->dict.buf);

	kfree(s);
}

//END xz_dec_lzma2.c
//BEGIN xz_crc32.c

/*
 * CRC32 using the polynomial from IEEE-802.3
 *
 * Authors: Lasse Collin <lasse.collin@tukaani.org>
 *          Igor Pavlov <http://7-zip.org/>
 *
 * This file has been put into the public domain.
 * You can do whatever you want with this file.
 */

/*
 * This is not the fastest implementation, but it is pretty compact.
 * The fastest versions of xz_crc32() on modern CPUs without hardware
 * accelerated CRC instruction are 3-5 times as fast as this version,
 * but they are bigger and use more memory for the lookup table.
 */

/*
 * STATIC_RW_DATA is used in the pre-boot environment on some architectures.
 * See <linux/decompress/mm.h> for details.
 */
#ifndef STATIC_RW_DATA
#	define STATIC_RW_DATA static
#endif

STATIC_RW_DATA uint32_t xz_crc32_table[256];

XZ_EXTERN void xz_crc32_init(void)
{
	const uint32_t poly = 0xEDB88320;

	uint32_t i;
	uint32_t j;
	uint32_t r;

	for (i = 0; i < 256; ++i) {
		r = i;
		for (j = 0; j < 8; ++j)
			r = (r >> 1) ^ (poly & ~((r & 1) - 1));

		xz_crc32_table[i] = r;
	}

	return;
}

XZ_EXTERN uint32_t xz_crc32(const uint8_t *buf, size_t size, uint32_t crc)
{
	crc = ~crc;

	while (size != 0) {
		crc = xz_crc32_table[*buf++ ^ (crc & 0xFF)] ^ (crc >> 8);
		--size;
	}

	return ~crc;
}
//END xz_crc32.c
#endif









//returns an unseekable write-only file that will decompress any data that is written to it.
//decompressed data will be written to the output
typedef struct
{
	vfsfile_t vf;
	vfsfile_t *outfile;

	qbyte out[65536];
	struct xz_buf b;
	struct xz_dec *s;
} vf_xz_dec_t;

static vfsfile_t *FS_XZ_Finish(vfsfile_t *f)
{
	vf_xz_dec_t *n = (vf_xz_dec_t*)f;
	vfsfile_t *r = n->outfile;
	if (n->s)
		xz_dec_end(n->s);
	Z_Free(n);
	return r;
}
static qboolean QDECL FS_XZ_Dec_Close(vfsfile_t *f)
{
	vfsfile_t *orig = FS_XZ_Finish(f);
	VFS_CLOSE(orig);
	return true;
}
static int QDECL FS_XZ_Dec_Write(vfsfile_t *f, const void *buffer, int len)
{
	enum xz_ret ret;
	vf_xz_dec_t *n = (vf_xz_dec_t*)f;

	n->b.in = buffer;
	n->b.in_size = len;
	n->b.in_pos = 0;

	while(n->b.in_pos < n->b.in_size)
	{
		ret = xz_dec_run(n->s, &n->b);

		if (n->b.out_pos == sizeof(n->out))
		{
			if (VFS_WRITE(n->outfile, n->out, n->b.out_pos) != n->b.out_pos)
				return -1;

			n->b.out_pos = 0;
		}

		if (ret == XZ_OK)
			continue;

#ifdef XZ_DEC_ANY_CHECK
		if (ret == XZ_UNSUPPORTED_CHECK)
		{
			Con_Printf("XZ: Unsupported check; not verifying "
					"file integrity\n");
			continue;
		}
#endif

		if (VFS_WRITE(n->outfile, n->out, n->b.out_pos) != n->b.out_pos)
			return -1;
		n->b.out_pos = 0;

		if (ret == XZ_STREAM_END)
			continue;


		switch (ret)
		{
		case XZ_STREAM_END:
			xz_dec_end(n->s);
			n->s = NULL;
			break;

		case XZ_MEM_ERROR:
			Con_Printf("XZ: Memory allocation failed\n");
			break;

		case XZ_MEMLIMIT_ERROR:
			Con_Printf("XZ: Memory usage limit reached\n");
			break;

		case XZ_FORMAT_ERROR:
			Con_Printf("Not a .xz file\n");
			break;

		case XZ_OPTIONS_ERROR:
			Con_Printf("Unsupported options in the .xz headers\n");
			break;

		case XZ_DATA_ERROR:
		case XZ_BUF_ERROR:
			Con_Printf("XZ: File is corrupt\n");
			break;

		default:
			Con_Printf("Bug!\n");
			break;
		}
		return -1;
	}

	return n->b.in_pos;
}

//return a write-only wrapper around another file. .xz data written to the wrapper will be seen as decompressed for the wrapee.
vfsfile_t *FS_XZ_DecompressWriteFilter(vfsfile_t *outfile)
{
	vf_xz_dec_t *n = Z_Malloc(sizeof(*n));
	xz_crc32_init();
#ifdef XZ_USE_CRC64
	xz_crc64_init();
#endif

	n->outfile = outfile;

	n->s = xz_dec_init(XZ_DYNALLOC, 1 << 27);
	if (!n->s)
	{
		Z_Free(n);
		return NULL;
	}

	n->b.in			= NULL;
	n->b.in_pos		= 0;
	n->b.in_size	= 0;
	n->b.out		= n->out;
	n->b.out_pos	= 0;
	n->b.out_size	= sizeof(n->out);
	n->vf.Flush				= NULL;
	n->vf.GetLen			= NULL;
	n->vf.ReadBytes			= NULL;
	n->vf.Seek				= NULL;
	n->vf.Tell				= NULL;
	n->vf.Close				= FS_XZ_Dec_Close;
	n->vf.WriteBytes		= FS_XZ_Dec_Write;
	n->vf.seekstyle			= SS_UNSEEKABLE;

	return &n->vf;
}

//returns a read-only filter around a compressed .xz file
vfsfile_t *FS_XZ_DecompressReadFilter(vfsfile_t *srcfile)
{
	char block[65536];
	int blocksize = VFS_READ(srcfile, block, HEADER_MAGIC_SIZE);

	if (blocksize == HEADER_MAGIC_SIZE && !memcmp(block, HEADER_MAGIC, HEADER_MAGIC_SIZE))
	{	//okay, looks like an xz.
		vfsfile_t *xzpipe = FS_OpenTemp();
		if (!xzpipe)
			xzpipe = VFSPIPE_Open(1, true);
		xzpipe = FS_XZ_DecompressWriteFilter(xzpipe);
		for (;blocksize;)
		{
			if (blocksize < 0 || blocksize != VFS_WRITE(xzpipe, block, blocksize))
			{
				VFS_CLOSE(xzpipe);
				xzpipe = NULL;
				break;
			}
			blocksize = VFS_READ(srcfile, block, sizeof(block));
		}
		VFS_CLOSE(srcfile);

		if (xzpipe)
			xzpipe = FS_XZ_Finish(xzpipe);
		VFS_SEEK(xzpipe, 0);
		return xzpipe;
	}
	VFS_SEEK(srcfile, 0);
	return srcfile;
}
#endif

