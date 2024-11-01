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

#ifndef GCN_EXCEPTION_HPP
#define GCN_EXCEPTION_HPP

#include <string>

#include "guichan/platform.hpp"


#ifdef _MSC_VER
#if _MSC_VER <= 1200
#define __FUNCTION__ "?"
#endif
#endif

/*
 * A macro used to create a standard exception object.
 * What it basicly does is that it creates a new exception
 * and automatically sets the filename and line number where
 * the exception occured by using other compiler macros.
 */
#define GCN_EXCEPTION(mess) gcn::Exception(mess,   \
                            __FUNCTION__,          \
                            __FILE__,              \
                            __LINE__)

namespace gcn
{

    /**
     * An exception containing a message, a file and a line number
     * where the exception occured. Guichan will only throw exceptions 
     * of this class. 
     * 
     * You can use this class for your own exceptions that has
     * something to do with a GUI exception. A nifty feature of the
     * excpetion class is that it can tell you from which line and
     * file it was thrown. To make things easier when throwing
     * exceptions there exists a macro for creating exceptions
     * which automatically sets the filename and line number.
     *
     * EXAMPLE: @code
     *          throw GCN_EXCEPTION("my error message");
     *          @endcode
     */
    class GCN_CORE_DECLSPEC Exception
    {
    public:

        /**
         * Constructor.
         */
        Exception();

        /**
         * Constructor.
         *
         * @param message The error message of the exception.
         */
        Exception(const std::string& message);

        /**
         * Constructor.
         *
         * NOTE: Don't use this constructor. Use the GCN_EXCEPTION macro instead.
         *       This constructor merely exists for the GCN_EXCEPTION macro to
         *       use.
         *
         * @param message The error message of the exception.
         * @param function The function name where the exception occured.
         * @param filename The name of the file where the exception occured.
         * @param line The line number in the source code where the exception
         *             occured.
         */
        Exception(const std::string& message,
                  const std::string& function,
                  const std::string& filename,
                  unsigned int line);

        /**
         * Gets the function name where the exception occured.
         *
         * @return The function name where the exception occured.
         */
        const std::string& getFunction() const;

        /**
         * Gets the error message of the exception.
         *
         * @return The error message of the exception.
         */
        const std::string& getMessage() const;

        /**
         * Gets the filename where the exception occured.
         *
         * @return The filename where the exception occured.
         */
        const std::string& getFilename() const;

        /**
         * Gets the line number where the exception occured.
         *
         * @return The line number where the exception occured.
         */
        unsigned int getLine() const;

    protected:
        /**
         * Holds the name of the function name where the
         * exception occured.
         */
        std::string mFunction;

        /**
         * Holds the error message of the exception.
         */
        std::string mMessage;

        /**
         * Holds the filename where the exception occured.
         */
        std::string mFilename;

        /**
         * Holds the line number where the exception occured.
         */
        unsigned int mLine;
    };
}

#endif // end GCN_EXCEPTION_HPP

/*
 * "Final Fantasy XI is the BEST!... It's even better then water!"
 *  - Astrolite
 * I believe it's WoW now days.
 */
