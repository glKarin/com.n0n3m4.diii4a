/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004, 2005, 2006, 2007 Olof Naessén and Per Larsson
 *
 *                                                         Js_./
 * Per Larsson a.k.a finalman                          _RqZ{a<^_aa
 * Olof Naessén a.k.a jansem/yakslem                _asww7!uY`>  )\a//
 *                                                 _Qhm`] _f "'c  1!5m
 * Visit: http://guichan.darkbits.org             )Qk<P ` _: :+' .'  "{[
 *                                               .)j(] .d_/ '-(  P .   S
 * License: (BSD)                                <Td/Z <fP"5(\"??"\a.  .L
 * Redistribution and use in source and          _dV>ws?a-?'      ._/L  #'
 * binary forms, with or without                 )4d[#7r, .   '     )d`)[
 * modification, are permitted provided         _Q-5'5W..j/?'   -?!\)cam'
 * that the following conditions are met:       j<<WP+k/);.        _W=j f
 * 1. Redistributions of source code must       .$%w\/]Q  . ."'  .  mj$
 *    retain the above copyright notice,        ]E.pYY(Q]>.   a     J@\
 *    this list of conditions and the           j(]1u<sE"L,. .   ./^ ]{a
 *    following disclaimer.                     4'_uomm\.  )L);-4     (3=
 * 2. Redistributions in binary form must        )_]X{Z('a_"a7'<a"a,  ]"[
 *    reproduce the above copyright notice,       #}<]m7`Za??4,P-"'7. ).m
 *    this list of conditions and the            ]d2e)Q(<Q(  ?94   b-  LQ/
 *    following disclaimer in the                <B!</]C)d_, '(<' .f. =C+m
 *    documentation and/or other materials      .Z!=J ]e []('-4f _ ) -.)m]'
 *    provided with the distribution.          .w[5]' _[ /.)_-"+?   _/ <W"
 * 3. Neither the name of Guichan nor the      :$we` _! + _/ .        j?
 *    names of its contributors may be used     =3)= _f  (_yQmWW$#(    "
 *    to endorse or promote products derived     -   W,  sQQQQmZQ#Wwa]..
 *    from this software without specific        (js, \[QQW$QWW#?!V"".
 *    prior written permission.                    ]y:.<\..          .
 *                                                 -]n w/ '         [.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT       )/ )/           !
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY         <  (; sac    ,    '
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING,               ]^ .-  %
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF            c <   r
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR            aga<  <La
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE          5%  )P'-3L
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR        _bQf` y`..)a
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          ,J?4P'.P"_(\?d'.,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES               _Pa,)!f/<[]/  ?"
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT      _2-..:. .r+_,.. .
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     ?a.<%"'  " -'.a_ _,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                     ^
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * For comments regarding functions please see the header file.
 */

#include "guichan/sfml/sfmlimage.hpp"

#include "guichan/exception.hpp"

namespace gcn
{
	gcn::Color Convert(const sf::Color &c)
	{
		gcn::Color conv;
		conv.r = c.r;
		conv.g = c.g;
		conv.b = c.b;
		conv.a = c.a;
		return conv;
	}
	sf::Color Convert(const gcn::Color &c)
	{
		sf::Color conv;
		conv.r = (sf::Uint8)c.r;
		conv.g = (sf::Uint8)c.g;
		conv.b = (sf::Uint8)c.b;
		conv.a = (sf::Uint8)c.a;
		return conv;
	}

	SFMLImage::SFMLImage(sf::Image* image, bool autoFree)
    {
        mAutoFree = autoFree;
        mImage = image;
    }

    SFMLImage::~SFMLImage()
    {
        if (mAutoFree)
        {
            free();
        }
    }

	sf::Image* SFMLImage::getImage() const
    {
        return mImage;
    }

    int SFMLImage::getWidth() const
    {
        if (mImage == NULL)
        {
            throw GCN_EXCEPTION("Trying to get the width of a non loaded image.");
        }

        return (int)mImage->GetWidth();
    }

    int SFMLImage::getHeight() const
    {
        if (mImage == NULL)
        {
            throw GCN_EXCEPTION("Trying to get the height of a non loaded image.");
        }

        return (int)mImage->GetHeight();
    }

    Color SFMLImage::getPixel(int x, int y)
    {
        if (mImage == NULL)
        {
            throw GCN_EXCEPTION("Trying to get a pixel from a non loaded image.");
        }
        return Convert(mImage->GetPixel(x,y));
    }

    void SFMLImage::putPixel(int x, int y, const Color& color)
    {
        if (mImage == NULL)
        {
            throw GCN_EXCEPTION("Trying to put a pixel in a non loaded image.");
        }

		mImage->SetPixel(x,y,Convert(color));
    }

    void SFMLImage::convertToDisplayFormat()
    {
        if (mImage == NULL)
        {
            throw GCN_EXCEPTION("Trying to convert a non loaded image to display format.");
        }

        bool hasPink = false;
        bool hasAlpha = false;

		for(unsigned int h = 0; h < mImage->GetHeight(); ++h)
		{
			for(unsigned int w = 0; w < mImage->GetWidth(); ++w)
			{
				sf::Color c = mImage->GetPixel(w,h);
				if(c == sf::Color(255,0,255))
					hasPink = true;

				if(c.a != 255)
					hasAlpha = true;

				if(hasPink && hasAlpha)
					break;
			}

			if(hasPink && hasAlpha)
				break;
		}

        /*SDL_Surface *tmp;

        if (hasAlpha)
        {
            tmp = SDL_DisplayFormatAlpha(mSurface);
            SDL_FreeSurface(mSurface);
            mSurface = NULL;
        }
        else
        {
            tmp = SDL_DisplayFormat(mSurface);
            SDL_FreeSurface(mSurface);
            mSurface = NULL;
        }

        if (hasPink)
        {
            SDL_SetColorKey(tmp, SDL_SRCCOLORKEY,
                            SDL_MapRGB(tmp->format,255,0,255));
        }
        if (hasAlpha)
        {
            SDL_SetAlpha(tmp, SDL_SRCALPHA, 255);
        }

        mImage = tmp;*/
    }

    void SFMLImage::free()
    {
       delete mImage;
	   mImage = 0;
    }
}
