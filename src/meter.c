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
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include "peppyalsa.h"

static char *mypipe = NULL;
static int fifo_fd = -1;
static int meter_max;
static int meter_show = 0;
static int exit_hook_set = 0;
static char *left =  "L: ";
static char *right = "R: ";

#define LINE_LENGTH 76

static void close_pipe(void) {
	if(fifo_fd != -1) {
		close(fifo_fd);
		fifo_fd = -1;
	}
}

static void open_pipe(void) {
	close_pipe();
	fifo_fd = open(mypipe, O_WRONLY | O_NONBLOCK);
}

static void print_vu_meter_ch(char *label, int value) {
	int size = LINE_LENGTH;
	int v = (int)(value/100.0 * size);
	char ch[LINE_LENGTH + 1] = "";
	int n;

	for(n=0; n < size; n++) {
		ch[n] = (n < v) ? '=' : ' ';
	}
	fprintf(stdout, "%s%s\n", label, ch);
	fflush(stdout);	
}

static void send_to_pipe(int left_ch, int right_ch) {
	if(fifo_fd == -1) {
		return;
	}
	unsigned int stereo = left_ch + (right_ch << 16);
	int n = write(fifo_fd, &stereo, sizeof(stereo));
	if(n < 0 && errno == EPIPE) {
		/* The reader is gone. Drop the descriptor; update() opens it again
		 * once a reader is back. A full pipe (EAGAIN) is not an error. */
		close_pipe();
		return;
	}
	if(meter_show == 1 && n > 0) {
		print_vu_meter_ch(left, left_ch);
		print_vu_meter_ch(right, right_ch);
		fprintf(stdout, "\033[1A\033[1A");
	}			
}

/* Registered with atexit() from a shared object, this also runs when the
 * host unloads the library. Some players do that on every device close,
 * so the descriptor has to be released here or it leaks once per open. */
static void clean_pipe(void) {
	send_to_pipe(0, 0);
	close_pipe();
	free(mypipe);
	mypipe = NULL;
}

/* The host creates one scope per PCM it opens. The pipe state is
 * process-wide, so a later init() reuses the open descriptor and the
 * exit hook instead of stacking new ones. */
static int init(const char *name, int max, int show,
                int spectrum_max, int spectrum_size,
                int log_f, int log_y, int window) {
	(void)spectrum_max;
	(void)spectrum_size;
	(void)log_f;
	(void)log_y;
	(void)window;

	peppyalsa_ignore_sigpipe();

	meter_max = max;
	meter_show = show;

	if(mypipe == NULL || strcmp(mypipe, name) != 0) {
		free(mypipe);
		mypipe = (char*)malloc(strlen(name) + 1);
		if(mypipe == NULL) {
			return -1;
		}
		strcpy(mypipe, name);
		mkfifo(mypipe, 0666);
		close_pipe();
	}
	if(fifo_fd == -1) {
		open_pipe();
	}
	if(!exit_hook_set) {
		atexit(clean_pipe);
		exit_hook_set = 1;
	}
	return 0;
}

static void update(int meter_level_l, int meter_level_r, snd_pcm_scope_peppyalsa_t *level) {	
	if(fifo_fd == -1) {
		open_pipe();
		if(fifo_fd == -1) {
			return;
		}			
	}
	
    int left_ch = (meter_level_l / 32767.0f) * meter_max;
    int right_ch = (meter_level_r / 32767.0f) * meter_max;
    send_to_pipe(left_ch, right_ch);
    
    return;
}

device meter() {
    struct device _meter;
    _meter.init = &init;
    _meter.update = &update;
    return _meter;
};
