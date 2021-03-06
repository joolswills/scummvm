/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef SCI_GRAPHICS_TEXT32_H
#define SCI_GRAPHICS_TEXT32_H

#include "sci/graphics/celobj32.h"
#include "sci/graphics/frameout.h"

namespace Sci {

enum TextAlign {
	kTextAlignLeft   = 0,
	kTextAlignCenter = 1,
	kTextAlignRight  = 2
};

/**
 * This class handles text calculation and rendering for
 * SCI32 games. The text calculation system in SCI32 is
 * nearly the same as SCI16, which means this class behaves
 * similarly. Notably, GfxText32 maintains drawing
 * parameters across multiple calls.
 */
class GfxText32 {
private:
	SegManager *_segMan;
	GfxCache *_cache;
	GfxScreen *_screen;

	/**
	 * The resource ID of the default font used by the game.
	 *
	 * @todo Check all SCI32 games to learn what their
	 * default font is.
	 */
	static int16 _defaultFontId;

	/**
	 * The width and height of the currently active text
	 * bitmap, in text-system coordinates.
	 *
	 * @note These are unsigned in the actual engine.
	 */
	int16 _width, _height;

	/**
	 * The color used to draw text.
	 */
	uint8 _foreColor;

	/**
	 * The background color of the text box.
	 */
	uint8 _backColor;

	/**
	 * The transparent color of the text box. Used when
	 * compositing the bitmap onto the screen.
	 */
	uint8 _skipColor;

	/**
	 * The rect where the text is drawn within the bitmap.
	 * This rect is clipped to the dimensions of the bitmap.
	 */
	Common::Rect _textRect;

	/**
	 * The text being drawn to the currently active text
	 * bitmap.
	 */
	Common::String _text;

	/**
	 * The font being used to draw the text.
	 */
	GuiResourceId _fontId;

	/**
	 * The color of the text box border.
	 */
	int16 _borderColor;

	/**
	 * TODO: Document
	 */
	bool _dimmed;

	/**
	 * The text alignment for the drawn text.
	 */
	TextAlign _alignment;

	/**
	 * The memory handle of the currently active bitmap.
	 */
	reg_t _bitmap;

	int16 _field_20;

	/**
	 * TODO: Document
	 */
	int16 _field_22;

	int _field_2C, _field_30, _field_34, _field_38;

	int16 _field_3C;

	/**
	 * The position of the text draw cursor.
	 */
	Common::Point _drawPosition;

	void drawFrame(const Common::Rect &rect, const int16 size, const uint8 color, const bool doScaling);
	void drawTextBox();
	void erase(const Common::Rect &rect, const bool doScaling);

	void drawChar(const uint8 charIndex);
	uint16 getCharWidth(const uint8 charIndex, const bool doScaling) const;
	void drawText(const uint index, uint length);

	inline int scaleUpWidth(int value) const {
		const int scriptWidth = g_sci->_gfxFrameout->getCurrentBuffer().scriptWidth;
		return (value * scriptWidth + _scaledWidth - 1) / _scaledWidth;
	}

	/**
	 * Gets the length of the longest run of text available
	 * within the currently loaded text, starting from the
	 * given `charIndex` and running for up to `maxWidth`
	 * pixels. Returns the number of characters that can be
	 * written, and mutates the value pointed to by
	 * `charIndex` to point to the index of the next
	 * character to render.
	 */
	uint getLongest(uint *charIndex, const int16 maxWidth);

	/**
	 * Gets the pixel width of a substring of the currently
	 * loaded text, without scaling.
	 */
	int16 getTextWidth(const uint index, uint length) const;

	/**
	 * Gets the pixel width of a substring of the currently
	 * loaded text, with scaling.
	 */
	int16 getTextWidth(const Common::String &text, const uint index, const uint length);

	inline Common::Rect scaleRect(const Common::Rect &rect) {
		Common::Rect scaledRect(rect);
		int16 scriptWidth = g_sci->_gfxFrameout->getCurrentBuffer().scriptWidth;
		int16 scriptHeight = g_sci->_gfxFrameout->getCurrentBuffer().scriptHeight;
		Ratio scaleX(_scaledWidth, scriptWidth);
		Ratio scaleY(_scaledHeight, scriptHeight);
		mul(scaledRect, scaleX, scaleY);
		return scaledRect;
	}

public:
	GfxText32(SegManager *segMan, GfxCache *fonts, GfxScreen *screen);

	/**
	 * The size of the x-dimension of the coordinate system
	 * used by the text renderer.
	 */
	int16 _scaledWidth;

	/**
	 * The size of the y-dimension of the coordinate system
	 * used by the text renderer.
	 */
	int16 _scaledHeight;

	/**
	 * The currently active font resource used to write text
	 * into the bitmap.
	 *
	 * @note SCI engine builds the font table directly
	 * inside of FontMgr; we use GfxFont instead.
	 */
	GfxFont *_font;

	/**
	 * Creates a plain font bitmap with a flat color
	 * background.
	 */
	reg_t createFontBitmap(int16 width, int16 height, const Common::Rect &rect, const Common::String &text, const uint8 foreColor, const uint8 backColor, const uint8 skipColor, const GuiResourceId fontId, TextAlign alignment, const int16 borderColor, bool dimmed, const bool doScaling, reg_t *outBitmapObject);

	/**
	 * Creates a font bitmap with a view background.
	 */
	reg_t createFontBitmap(const CelInfo32 &celInfo, const Common::Rect &rect, const Common::String &text, const int16 foreColor, const int16 backColor, const GuiResourceId fontId, const int16 skipColor, const int16 borderColor, const bool dimmed, reg_t *outBitmapObject);

	/**
	 * Sets the font to be used for rendering and
	 * calculation of text dimensions.
	 */
	void setFont(const GuiResourceId fontId);

	/**
	 * Retrieves the width and height of a block of text.
	 */
	Common::Rect getTextSize(const Common::String &text, const int16 maxWidth, bool doScaling);

	/**
	 * Retrieves the width of a line of text.
	 */
	int16 getStringWidth(const Common::String &text);
};

} // End of namespace Sci

#endif
