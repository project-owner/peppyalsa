/*
* Copyright 2018 Peppy ALSA Plugin peppy.player@gmail.com
* 
* This file is the part of the Peppy ALSA Plugin project.
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

#ifndef PEPPYMETER_HEADER
#define PEPPYMETER_HEADER

#include <alsa/asoundlib.h>

typedef struct _snd_pcm_scope_peppyalsa_channel {
  int16_t levelchan;
  int16_t peak;
  unsigned int peak_age;
  int16_t previous;
} snd_pcm_scope_peppyalsa_channel_t;

typedef struct _snd_pcm_scope_peppyalsa {
  snd_pcm_t *pcm;
  snd_pcm_scope_t *s16;
  snd_pcm_scope_peppyalsa_channel_t *channels;
  snd_pcm_uframes_t old;
  int top;
  unsigned int decay_ms;
  char* fifo_vu_name;
  int fifo_vu_max_value;
  unsigned int fifo_vu_resampler;
  int show_tty_vu_meter;
} snd_pcm_scope_peppyalsa_t;

typedef struct device {
	int (*init)(
		const char *fifo_vu_name, 
		int fifo_vu_max_value, 
		unsigned int resampler, 
		int show_tty_vu_meter);
	void (*update)(
		int meter_level_l, 
		int meter_level_r);
} device;
#endif
