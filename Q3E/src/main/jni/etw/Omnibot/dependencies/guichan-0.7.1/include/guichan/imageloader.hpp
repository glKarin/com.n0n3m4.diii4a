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

#ifndef GCN_IMAGELOADER_HPP
#define GCN_IMAGELOADER_HPP

#include <iosfwd>

#include "guichan/platform.hpp"

namespace gcn
{
    class Image;

    /**
     * Abstract class for providing functions for loading images.
     *
     * Guichan contains implementations of ImageLoader for common 
     * libraries like the Allegro library, the HGE library, 
     * he OpenLayer library, and the SDL library. 
     * To make Guichan usable with other libraries, an ImageLoader 
     * class must be implemented.
     *
     * To make Guichan use an image loader, the image loader needs
     * to be passed to the Image class using the static method
     * Image::setImageLoader.
     *
     * @see Image::setImageLoader, Image::getImageLoader, 
     *      AllegroImageLoader, HGEImageLoader, OpenLayerImageLoader, 
     *      OpenGLAllegroImageLoader, OpenGLSDLImageLoader,
     *      SDLImageLoader     
     * @since 0.1.0
     */
    class GCN_CORE_DECLSPEC ImageLoader
    {
    public:

        /**
         * Destructor.
         */
        virtual ~ImageLoader() { }

        /**
         * Loads an image.
         *
         * NOTE: The functions Image::getPixel and Image::putPixel 
         *       are only guaranteed to work before an image has 
         *       been converted to display format.
         *
         * @param filename The filename of the image to load.
         * @param convertToDisplayFormat True if the image should be converted
         *                               to display, false otherwise.
         */
        virtual Image* load(const std::string& filename, bool convertToDisplayFormat = true) = 0;
		virtual Image *load(const char* Data, std::size_t SizeInBytes, bool convertToDisplayFormat = true) = 0;
    };
}

#endif // end GCN_IMAGELOADER_HPP
