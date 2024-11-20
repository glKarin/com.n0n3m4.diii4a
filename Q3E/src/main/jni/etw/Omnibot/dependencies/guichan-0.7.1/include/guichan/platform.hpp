/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004 - 2008 Olof Naessén and Per Larsson
 *
 *
 * Per Larsson a.k.a finalman
 * Olof Naessén a.k.a jansem/yakslem
 *
 * Visit: http://guichan.sourceforge.net
 *
 * License: (BSD)
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Guichan nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GCN_PLATFORM_HPP
#define GCN_PLATFORM_HPP

#if defined (__MINGW32__) && defined(GUICHAN_BUILD)
#define GCN_CORE_DECLSPEC __declspec(dllexport)

#elif defined (__MINGW32__) && defined(GUICHAN_EXTENSION_BUILD)
#define GCN_EXTENSION_DECLSPEC __declspec(dllexport)
#define GCN_CORE_DECLSPEC __declspec(dllimport)

#elif defined (__MINGW32__) && defined(GUICHAN_DLL_IMPORT)
#define GCN_CORE_DECLSPEC __declspec(dllimport)
#define GCN_EXTENSION_DECLSPEC __declspec(dllimport)

#elif defined(_MSC_VER) && defined(GUICHAN_BUILD)
#define GCN_CORE_DECLSPEC _declspec(dllexport)

#elif defined(_MSC_VER) && defined(GUICHAN_EXTENSION_BUILD)
#define GCN_CORE_DECLSPEC _declspec(dllimport)
#define GCN_EXTENSION_DECLSPEC _declspec(dllexport)

#endif

#ifndef GCN_CORE_DECLSPEC
#define GCN_CORE_DECLSPEC
#endif

#ifndef GCN_EXTENSION_DECLSPEC
#define GCN_EXTENSION_DECLSPEC
#endif

#ifndef NULL
#define NULL 0
#endif

#endif // end GCN_PLATFORM_HPP
