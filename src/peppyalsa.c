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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <alsa/asoundlib.h>

#include "peppyalsa.h"
#include "fifo.h"

/* milliseconds to go from 32767 to 0 */
#define DECAY_MS 200
#define MAX_METERS 2

struct device output_device;

int num_meters, num_scopes;

static int level_enable(snd_pcm_scope_t * scope)
{
    snd_pcm_scope_peppyalsa_t *level =
        snd_pcm_scope_get_callback_private(scope);
    level->channels =
        calloc(snd_pcm_meter_get_channels(level->pcm),
                sizeof(*level->channels));
    if (!level->channels) {
        if (level) free(level); 
        return -ENOMEM;
    }

    snd_pcm_scope_set_callback_private(scope, level);
    return (0);

}

static void level_disable(snd_pcm_scope_t * scope)
{
    snd_pcm_scope_peppyalsa_t *level =
        snd_pcm_scope_get_callback_private(scope);

    if(level->channels) free(level->channels);
}

static void level_close(snd_pcm_scope_t * scope)
{

    snd_pcm_scope_peppyalsa_t *level =
        snd_pcm_scope_get_callback_private(scope);
    if (level) free(level); 
}

static void level_start(snd_pcm_scope_t * scope ATTRIBUTE_UNUSED)
{
    sigset_t s;
    sigemptyset(&s);
    sigaddset(&s, SIGINT);
    pthread_sigmask(SIG_BLOCK, &s, NULL); 
}

static void level_stop(snd_pcm_scope_t * scope)
{
}

static int get_channel_level(
	int channel, 
	snd_pcm_scope_peppyalsa_t *level, 
	snd_pcm_uframes_t offset, 
	snd_pcm_uframes_t size1, 
	snd_pcm_uframes_t size2,
    int max_decay, int max_decay_temp)
{
    int16_t *ptr;
    int s, lev = 0;
    snd_pcm_uframes_t n;
    snd_pcm_scope_peppyalsa_channel_t *l;
    l = &level->channels[channel];

    // Iterate through the channel buffer and find the highest level value
    ptr = snd_pcm_scope_s16_get_channel_buffer(level->s16, channel) + offset;
    for (n = size1; n > 0; n--) {
        s = *ptr;
        if (s < 0) s = -s;
        if (s > lev) lev = s;
        ptr++;
    }

    ptr = snd_pcm_scope_s16_get_channel_buffer(level->s16, channel);
    for (n = size2; n > 0; n--) {
        s = *ptr;
        if (s < 0) s = -s;
        if (s > lev) lev = s;
        ptr++;
    }
    
    /* limit the decay */
    if (lev < l->levelchan) {     
        /* make max_decay go lower with level */
        max_decay_temp = max_decay / (32767 / (l->levelchan));
        lev = l->levelchan - max_decay_temp;
        max_decay_temp = max_decay;
    }

    l->levelchan = lev;
    l->previous =lev;

    return lev;
}

static void level_update(snd_pcm_scope_t * scope) {
    snd_pcm_scope_peppyalsa_t *level = snd_pcm_scope_get_callback_private(scope);
    snd_pcm_t *pcm = level->pcm;
    snd_pcm_sframes_t size;
    snd_pcm_uframes_t size1, size2;
    snd_pcm_uframes_t offset, cont;
    unsigned int channels;
    unsigned int ms;
    int max_decay, max_decay_temp;

    int meter_level_l = 0;
    int meter_level_r = 0; 

    size = snd_pcm_meter_get_now(pcm) - level->old;
    if (size < 0){
        size += snd_pcm_meter_get_boundary(pcm);
    }

    offset = level->old % snd_pcm_meter_get_bufsize(pcm);
    cont = snd_pcm_meter_get_bufsize(pcm) - offset;
    size1 = size;
    if (size1 > cont){
        size1 = cont;
    }

    size2 = size - size1;
    ms = size * 1000 / snd_pcm_meter_get_rate(pcm);
    max_decay = 32768 * ms / level->decay_ms;

    channels = snd_pcm_meter_get_channels(pcm);

    if(channels > 2){channels = 2;}

    meter_level_l = get_channel_level(0, level, offset, size1, size2, max_decay, max_decay_temp);
    meter_level_r = meter_level_l;
    if(channels > 1){
        meter_level_r = get_channel_level(1, level, offset, size1, size2, max_decay, max_decay_temp);
    }

    output_device.update(meter_level_l, meter_level_r);

    level->old = snd_pcm_meter_get_now(pcm);
}

static void level_reset(snd_pcm_scope_t * scope) {
    snd_pcm_scope_peppyalsa_t *level = snd_pcm_scope_get_callback_private(scope);
    snd_pcm_t *pcm = level->pcm;
    memset(level->channels, 0, snd_pcm_meter_get_channels(pcm) * sizeof(*level->channels));
    level->old = snd_pcm_meter_get_now(pcm);
}

snd_pcm_scope_ops_t level_ops = {
enable:level_enable,
       disable:level_disable,
       close:level_close,
       start:level_start,
       stop:level_stop,
       update:level_update,
       reset:level_reset,
};

int snd_pcm_scope_peppyalsa_open(snd_pcm_t * pcm,
        const char *name,
        unsigned int decay_ms,
        const char *fifo_vu_name,
        int fifo_vu_max_value,
        unsigned int resampler,
        int show_tty_vu_meter,
        snd_pcm_scope_t ** scopep)
{
    snd_pcm_scope_t *scope, *s16;
    snd_pcm_scope_peppyalsa_t *level;
    int err = snd_pcm_scope_malloc(&scope);
    if (err < 0){
        return err;
    }
    level = calloc(1, sizeof(*level));
    if (!level) {
        if (scope) free(scope);
        return -ENOMEM;
    }
    level->pcm = pcm;
    level->decay_ms = decay_ms;
    level->fifo_vu_name = strdup(fifo_vu_name);
    level->fifo_vu_max_value = fifo_vu_max_value;
    level->fifo_vu_resampler = resampler;
    level->show_tty_vu_meter = show_tty_vu_meter;
    
    s16 = snd_pcm_meter_search_scope(pcm, "s16");
    if (!s16) {
        err = snd_pcm_scope_s16_open(pcm, "s16", &s16);
        if (err < 0) {
            if (scope) free(scope);
            if (level)free(level);
            return err;
        }
    }
    level->s16 = s16;
    snd_pcm_scope_set_ops(scope, &level_ops);
    snd_pcm_scope_set_callback_private(scope, level);
    if (name){
        snd_pcm_scope_set_name(scope, strdup(name));
    }
    snd_pcm_meter_add_scope(pcm, scope);
    *scopep = scope;
    return 0;
}

int _snd_pcm_scope_peppyalsa_open(
	snd_pcm_t * pcm, 
	const char *name,
    snd_config_t * root, 
    snd_config_t * conf)
{
    snd_config_iterator_t i, next;
    snd_pcm_scope_t *scope;
    long decay_ms = -1;
    const char *fifo_vu_name = "";
    int fifo_vu_max_value = -1;
    unsigned int fifo_vu_resampler = 0;
    int show_tty_vu_meter = -1;
    int err;

    num_meters = MAX_METERS;
    num_scopes = MAX_METERS;

    snd_config_for_each(i, next, conf) {
        snd_config_t *n = snd_config_iterator_entry(i);
        const char *id;
        if (snd_config_get_id(n, &id) < 0)
            continue;
        if (strcmp(id, "comment") == 0)
            continue;
        if (strcmp(id, "type") == 0)
            continue;
        if (strcmp(id, "decay_ms") == 0) {
            err = snd_config_get_integer(n, &decay_ms);
            if (err < 0) {
                SNDERR("Invalid type for %s", id);
                return -EINVAL;
            }
            continue;
        }
        if (strcmp(id, "fifo_vu_name") == 0) {
            err = snd_config_get_string(n, &fifo_vu_name);
            if (err < 0) {
                SNDERR("Invalid type for %", id);
                return -EINVAL;  
            }
            continue;
        }
        if (strcmp(id, "fifo_vu_max_value") == 0) {
            err = snd_config_get_integer(n, &fifo_vu_max_value);
            if (err < 0) {
                SNDERR("Invalid type for %", id);
                return -EINVAL;
            }
            continue;
        }
        if (strcmp(id, "fifo_vu_resampler") == 0) {
            err = snd_config_get_integer(n, &fifo_vu_resampler);
            if (err < 0) {
                SNDERR("Invalid type for %", id);
                return -EINVAL;
            }
            continue;
        }
        if (strcmp(id, "show_tty_vu_meter") == 0) {
            err = snd_config_get_integer(n, &show_tty_vu_meter);
            if (err < 0) {
                SNDERR("Invalid type for %", id);
                return -EINVAL;
            }
            continue;
        }
        SNDERR("Unknown field %s", id);
        return -EINVAL;
    }

    if (decay_ms < 0) {
        decay_ms = DECAY_MS;
    }
    if (strlen(fifo_vu_name) == 0) {
        fifo_vu_name = DEFAULT_FIFO_VU_NAME;
    }
	if (fifo_vu_max_value < 0) {
		fifo_vu_max_value = DEFAULT_FIFO_MAX_VALUE;
    }
    if (fifo_vu_resampler == 0) {
		fifo_vu_resampler = DEFAULT_FIFO_RESAMPLER;
    }
    if (show_tty_vu_meter == -1) {
		show_tty_vu_meter = 0;
    }

    output_device = fifo();
    output_device.init(fifo_vu_name, fifo_vu_max_value, fifo_vu_resampler, show_tty_vu_meter);

    return snd_pcm_scope_peppyalsa_open(
            pcm,
            name, 
            decay_ms,
            fifo_vu_name,
            fifo_vu_max_value,
            fifo_vu_resampler,
            show_tty_vu_meter,
            &scope);
}
