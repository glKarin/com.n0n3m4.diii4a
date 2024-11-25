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

#ifndef GCN_SLIDER_HPP
#define GCN_SLIDER_HPP

#include "guichan/keylistener.hpp"
#include "guichan/mouselistener.hpp"
#include "guichan/platform.hpp"
#include "guichan/widget.hpp"

namespace gcn
{
    /**
     * An implementation of a slider where a user can select different values by
     * sliding between a start value and an end value of a scale. 
     *
     * If the selected value is changed an action event will be sent to all
     * action listeners of the slider.
     */
    class GCN_CORE_DECLSPEC Slider :
        public Widget,
        public MouseListener,
        public KeyListener
    {
    public:

        /**
         * Draw orientations for the slider. A slider can be drawn vertically or
         * horizontally.
         */
        enum Orientation
        {
            HORIZONTAL = 0,
            VERTICAL
        };

        /**
         * Constructor. The default start value of the slider scale is zero.
         *
         * @param scaleEnd The end value of the slider scale.
         */
        Slider(float scaleEnd = 1.0);

        /**
         * Constructor.
         *
         * @param scaleStart The start value of the slider scale.
         * @param scaleEnd The end value of the slider scale.
         */
        Slider(float scaleStart, float scaleEnd);

        /**
         * Destructor.
         */
        virtual ~Slider() { }

        /**
         * Sets the scale of the slider.
         *
         * @param scaleStart The start value of the scale.
         * @param scaleEnd tThe end of value the scale.
         * @see getScaleStart, getScaleEnd
         */
        void setScale(float scaleStart, float scaleEnd);

        /**
         * Gets the start value of the scale.
         *
         * @return The start value of the scale.
         * @see setScaleStart, setScale
         */
        float getScaleStart() const;

        /**
         * Sets the start value of the scale.
         *
         * @param scaleStart The start value of the scale.
         * @see getScaleStart
         */
        void setScaleStart(float scaleStart);

        /**
         * Gets the end value of the scale.
         *
         * @return The end value of the scale.
         * @see setScaleEnd, setScale
         */
        float getScaleEnd() const;

        /**
         * Sets the end value of the scale.
         *
         * @param scaleEnd The end value of the scale.
         * @see getScaleEnd
         */
        void setScaleEnd(float scaleEnd);

        /**
         * Gets the current selected value.
         *
         * @return The current selected value.
         * @see setValue
         */
        float getValue() const;
		int getValueInt() const;

        /**
         * Sets the current selected value.
         *
         * @param value The current selected value.
         * @see getValue
         */
        void setValue(float value);
		void setValue(int value);

        /**
         * Sets the length of the marker.
         *
         * @param length The length for the marker.
         * @see getMarkerLength
         */
        void setMarkerLength(int length);

        /**
         * Gets the length of the marker.
         *
         * @return The length of the marker.
         * @see setMarkerLength
         */
        int getMarkerLength() const;

        /**
         * Sets the orientation of the slider. A slider can be drawn vertically
         * or horizontally.
         *
         * @param orientation The orientation of the slider.
         * @see getOrientation
         */
        void setOrientation(Orientation orientation);

        /**
         * Gets the orientation of the slider. A slider can be drawn vertically
         * or horizontally.
         *
         * @return The orientation of the slider.
         * @see setOrientation
         */
        Orientation getOrientation() const;

        /**
         * Sets the step length. The step length is used when the keys LEFT 
         * and RIGHT are pressed to step in the scale.
         *
         * @param length The step length.
         * @see getStepLength
         */
        void setStepLength(float length);

        /**
         * Gets the step length. The step length is used when the keys LEFT 
         * and RIGHT are pressed to step in the scale.
         *
         * @return the step length.
         * @see setStepLength
         */
        float getStepLength() const;

		bool getDrawValue() const;

        // Inherited from Widget

        virtual void draw(Graphics* graphics);


        // Inherited from MouseListener.

        virtual void mousePressed(MouseEvent& mouseEvent);

        virtual void mouseDragged(MouseEvent& mouseEvent);

        virtual void mouseWheelMovedUp(MouseEvent& mouseEvent);

        virtual void mouseWheelMovedDown(MouseEvent& mouseEvent);


        // Inherited from KeyListener

        virtual void keyPressed(KeyEvent& keyEvent);

    protected:
        /**
         * Draws the marker.
         *
         * @param graphics A graphics object to draw with.
         */
        virtual void drawMarker(gcn::Graphics* graphics);

        /**
         * Converts a marker position to a value in the scale.
         *
         * @param position The position to convert.
         * @return A scale value corresponding to the position.
         * @see valueToMarkerPosition
         */
        virtual float markerPositionToValue(int position) const;

        /**
         * Converts a value to a marker position.
         *
         * @param value The value to convert.
         * @return A marker position corresponding to the value.
         * @see markerPositionToValue
         */
        virtual int valueToMarkerPosition(float value) const;

        /**
         * Gets the marker position of the current selected value.
         *
         * @return The marker position of the current selected value.
         */
        virtual int getMarkerPosition() const;

        /**
         * Holds the current selected value.
         */
        float mValue;

        /**
         * Holds the step length. The step length is used when the keys LEFT 
         * and RIGHT are pressed to step in the scale.
         */
        float mStepLength;

        /**
         * Holds the length of the marker.
         */
        int mMarkerLength;

        /**
         * Holds the start value of the scale.
         */
        float mScaleStart;

        /**
         * Holds the end value of the scale.
         */
        float mScaleEnd;

        /**
         * Holds the orientation of the slider. A slider can be drawn 
         * vertically or horizontally.
         */
        Orientation mOrientation;


		/**
		* True if the slider is dragged, false otherwise.
		*/
		bool mDragged;

		/**
		* True if the value should be drawn on the marker.
		*/
		bool mDrawValue;
    };
}

#endif // end GCN_SLIDER_HPP
