/*
* Copyright 2018-2021 Peppy ALSA Plugin peppy.player@gmail.com
*
* This file is the part of the Peppy ALSA Plugin project.
*
* DoP (DSD over PCM) meter support.
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

#ifndef PEPPYALSA_DOP_H
#define PEPPYALSA_DOP_H

#include <stdint.h>
#include <alsa/asoundlib.h>

/*
* Set the frame count averaged per decimated sample from the stream rate, so the
* averaging duration - and therefore the measurement bandwidth - stays the same
* from DSD64 to DSD512. Call once per update, before dop_level.
*/
void dop_set_rate(unsigned int rate);

/*
* Does the buffer look like a DoP stream? Probes the leading frames for the
* alternating 0x05/0xFA marker byte. Cheap: returns on the first frame that
* breaks the pattern, so an ordinary PCM buffer is rejected in a comparison or two.
*/
int dop_is_stream(int16_t *buf, snd_pcm_uframes_t frames);

/*
* Meter level (0..32767) recovered from the local density of ones in the DoP
* stream. Only meaningful when dop_is_stream returned true for the same buffer.
*/
int dop_level(int16_t *buf, snd_pcm_uframes_t frames);

#endif /* PEPPYALSA_DOP_H */
