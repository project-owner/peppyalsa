/*
* Copyright 2018-2021 Peppy ALSA Plugin peppy.player@gmail.com
*
* This file is the part of the Peppy ALSA Plugin project.
*
* DoP (DSD over PCM) meter support.
*
* A DoP stream is ordinary PCM as far as ALSA is concerned - 16 DSD bits sitting
* under an alternating 0x05/0xFA marker byte - so the s16 scope converts it
* happily and hands us the top 16 bits of every word. That is the marker plus the
* leading 8 DSD bits. Read as amplitude, as the plain PCM path does, those bytes
* carry no envelope at all and the needle sits at a constant ~4.
*
* Counting the ones instead recovers the signal: in DSD the local density of ones
* IS the amplitude, 50% being silence. Eight bits per frame at 176.4 kHz still
* leaves 1.4 Mbit/s to average over, far more than a needle needs.
*
* This does nothing for native DSD, where the s16 scope cannot convert at all and
* asserts before we ever see a sample.
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

#include <math.h>
#include "dop.h"

#define DOP_MARKER_A 0x05
#define DOP_MARKER_B 0xFA
#define DOP_PROBE    64     /* frames examined to recognise the stream */
#define DOP_WIN 2           /* groups per sliding window, ~0.25 ms total */
#define DOP_GROUP_HZ 8000   /* group length, so also the measurement bandwidth */
#define DOP_FULL_SCALE 2.0  /* 0 dBFS = 50% modulation index, see below */

/* Frames averaged per decimated sample, set from the rate in dop_set_rate so that
   the duration - and therefore the measurement bandwidth - stays the same from
   DSD64 to DSD512. A fixed frame count would shrink in time as the DSD rate rises. */
static unsigned int dop_group = 8;

static const unsigned char popcount8[256] = {
#define P2(n) n, n + 1, n + 1, n + 2
#define P4(n) P2(n), P2(n + 1), P2(n + 1), P2(n + 2)
#define P6(n) P4(n), P4(n + 1), P4(n + 1), P4(n + 2)
    P6(0), P6(1), P6(1), P6(2)
#undef P6
#undef P4
#undef P2
};

void dop_set_rate(unsigned int rate) {
    /* Short, because the group average is what limits the envelope bandwidth;
       DOP_WIN restores the bit count that keeps the measurement noise down. */
    dop_group = rate / DOP_GROUP_HZ;
    if (dop_group < 2) {
        dop_group = 2;
    }
}

int dop_is_stream(int16_t *buf, snd_pcm_uframes_t frames) {
    snd_pcm_uframes_t n;
    int prev = -1;

    if (frames > DOP_PROBE) {
        frames = DOP_PROBE;
    }
    if (frames < 8) {
        return 0;
    }
    for (n = 0; n < frames; n++) {
        int m = (buf[n] >> 8) & 0xFF;

        if ((m != DOP_MARKER_A && m != DOP_MARKER_B) || m == prev) {
            return 0;
        }
        prev = m;
    }
    return 1;
}

/*
* Level from the local bit density. Two things decide the quality here.
*
* The averaging window. Counting ones over N bits carries a measurement noise of
* 0.5/sqrt(N) if that noise is white, which argues for a long window; but a long
* window flattens transients, and measured on a plucked-string envelope a 1 ms window
* loses a third of the attack. It turns out the noise is NOT white - the modulator
* shapes it towards the top of the band, where the group average removes it - so the
* floor stays at zero even at a quarter of a millisecond. Hence a short window, and
* it is split into DOP_WIN groups rather than being one boxcar, which would also be
* a low-pass on the envelope and would read the high end far too low.
*
* And what is reported: the PCM path returns the peak of the period, so this has to
* as well, or the two read differently on the same music. A peak of raw group values
* would report the measurement noise; a peak of a short sliding RMS reports the music.
*/
int dop_level(int16_t *buf, snd_pcm_uframes_t frames) {
    snd_pcm_uframes_t g, groups = frames / dop_group;
    unsigned int n, bits = dop_group * 8;
    double ring[DOP_WIN] = { 0.0 };
    double running = 0.0, best = 0.0, lev;

    if (groups == 0) {
        return 0;
    }
    for (g = 0; g < groups; g++) {
        int ones = 0;
        double d;

        for (n = 0; n < dop_group; n++) {
            ones += popcount8[buf[g * dop_group + n] & 0xFF];
        }
        d = (ones - (double)bits / 2.0) / ((double)bits / 2.0);
        running += d * d - ring[g % DOP_WIN];
        ring[g % DOP_WIN] = d * d;
        if (g + 1 >= DOP_WIN && running > best) {
            best = running;
        }
    }
    if (groups < DOP_WIN) {
        best = running;
        lev = sqrt(best / (double)groups);
    } else {
        lev = sqrt(best / (double)DOP_WIN);
    }
    /* No noise-floor subtraction: the modulator shapes its noise towards the top of
       the band, and the window average is already a low-pass that removes it.
       DOP_FULL_SCALE: 0 dBFS in DSD is a 50% modulation index, so the density of a
       full-scale signal swings between 25% and 75% and never uses the whole range.
       Referring to that range rather than to 0-100% is what puts the needle on the
       same scale as the PCM path; without it everything reads 6 dB low. */
    lev *= DOP_FULL_SCALE * 32767.0;
    return lev > 32767.0 ? 32767 : (int)lev;
}
