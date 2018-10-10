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
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define LINE_LENGTH 76
#define POLLING_INTERVAL 0.033 // seconds
#define BUFFER_SIZE 2000

static char *left =  "L: ";
static char *right = "R: ";
static char DEFAULT_METER_NAME[] = "/home/pi/myfifo";

static void print_vu_meter_ch(char *label, int value) {
	int size = LINE_LENGTH;
	int v = (int)(value/100.0 * size);
	char ch[LINE_LENGTH + 1] = "";
	
	for(int n=0; n < size; n++) {
		ch[n] = (n < v) ? '=' : ' ';
	}
	printf("%s%s\n", label, ch);
	fflush(stdout);	
}

static void print_vu_meter(int left_ch, int right_ch) {
	print_vu_meter_ch(left, left_ch);
	print_vu_meter_ch(right, right_ch);
	printf("\033[1A\033[1A");
}

static void handle_ctrl_c(int n) {
	printf("\n\n");
	printf("\e[?25h");
	exit(0);
}

int main(int argc,char* argv[]) {
	char *mypipe = "";
	printf("Peppy ALSA Client. Goya Edition. 2018/09/08\n");
	if(argc == 1) {
        printf("No pipe name provided, defaulting to %s\n", DEFAULT_METER_NAME);
        mypipe  = DEFAULT_METER_NAME;
	} else if(argc == 2) {
		printf("Using provided pipe name %s\n", argv[1]);
		mypipe  = argv[1];
	} else {
		printf("Error! Too many arguments\n");
		printf("usage:\n");
		exit(0);
	}
	
	sigaction(SIGPIPE, &(struct sigaction){SIG_IGN}, NULL);	
    int fifo = -1;
	unsigned char buffer[BUFFER_SIZE];	
	signal(SIGINT, handle_ctrl_c);
	printf("\e[?25l");
	fflush(stdout);
	long interval = (long)(POLLING_INTERVAL * 1000000);	
    
    while(1) {
		memset(buffer, 0, BUFFER_SIZE);
		if(fifo == -1) {
			fifo = open(mypipe, O_RDONLY | O_NONBLOCK);
		} else {
			int n = read(fifo, buffer, BUFFER_SIZE);
			if(n > 0) {
				int words = n/4;
				for(int m=0; m < words; m++) {
					print_vu_meter(buffer[4 * m] + ((buffer[4 * m + 1]) << 8), 
						buffer[4 * m + 2] + ((buffer[4 * m + 3]) << 8));
				}
			}
		}		
		usleep(interval);
	}	
    close(fifo);
}
