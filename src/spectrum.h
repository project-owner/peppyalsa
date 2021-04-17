/*
* Copyright 2018-2021 Peppy ALSA Plugin peppy.player@gmail.com
* 
* This file is the part of the Peppy ALSA Plugin project.
*
* The changes in the Spectrum Analyzer code.
*   Copyright (c) 2021 by Tobias Dyballa <nixiepeppy@gmail.com>
*
* The Peppy ALSA Plugin project was derived from the project 'pivumeter'
*   pivumeter: level meter ALSA plugin for Raspberry Pi HATs and pHATs
*   Copyright (c) 2017 by Phil Howard <phil@pimoroni.com>
*
* The project 'pivumeter' was in turn derived from the project 'ameter'.
*   ameter :  level meter ALSA plugin with SDL display
*   Copyright (c) 2005 by Laurent Georget <laugeo@free.fr>
*   Copyright (c) 2001 by Abramo Bagnara <abramo@alsa-project.org>
*   Copyright (c) 2002 by Steve Harris <steve@plugin.org.uk>
*
* Peppy ALSA Plugin is free software: you can redistribute it and/or 
* modify it under the terms of the GNU General Public License as 
* published by the Free Software Foundation, either version 3 of the 
* License, or (at your option) any later version.
* 
* Peppy ALSA Plugin is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with Peppy ALSA Plugin. If not, see 
* <http://www.gnu.org/licenses/>.
*/

#include "peppyalsa.h"

#define DEFAULT_SPECTRUM_MAX 100
#define DEFAULT_SPECTRUM_SIZE 30
#define DEFAULT_LOG_F 1
#define DEFAULT_LOG_Y 1
#define DEFAULT_SMOOTH_F 50
#define DEFAULT_WINDOW 3

device spectrum();
