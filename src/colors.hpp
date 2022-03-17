/*-------------------------------------------------------------------------------

	BARONY
	File: colors.hpp
	Desc: I can see the rainbow.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "draw.hpp"

/*
 * SDL_Color colors.
 */
constexpr SDL_Color sdlColorWhite = { 255, 255, 255, 255 };

/*
 * 32-bit color defines
 */
constexpr Uint32 uint32ColorWhite = makeColorRGB(255, 255, 255);
constexpr Uint32 uint32ColorGray = makeColorRGB(127, 127, 127);
constexpr Uint32 uint32ColorBlue = makeColor(0, 92, 255, 255);
constexpr Uint32 uint32ColorLightBlue = makeColor(0, 255, 255, 255);
constexpr Uint32 uint32ColorBaronyBlue = makeColor(0, 192, 255, 255); //Dodger Blue. Apparently.
constexpr Uint32 uint32ColorRed = makeColor(255, 0, 0, 255);
constexpr Uint32 uint32ColorGreen = makeColor(0, 255, 0, 255);
constexpr Uint32 uint32ColorOrange = makeColor(255, 128, 0, 255);
constexpr Uint32 uint32ColorYellow = makeColor(255, 255, 0, 255);
