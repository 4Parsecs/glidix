/*
	Glidix GUI

	Copyright (c) 2014-2016, Madd Games.
	All rights reserved.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	* Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.
	
	* Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.
	
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <sys/glidix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libddi.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "gui.h"

typedef struct
{
	char c;
	uint8_t	attr;
} ConsoleCell;

extern const struct bitmap_font font_8x16;
extern const unsigned char __font_8x16_bitmap__[];
ConsoleCell grid[80*25];
int cursorX=0, cursorY=0;
uint8_t currentAttr = 0x07;
pthread_spinlock_t consoleLock;
pthread_t ctrlThread;
extern const char font[16*256];
volatile int running = 1;
int selectStart = 0;
int selectEnd = 0;
int selectAnchor = -1;

int fdMaster;
GWMWindow *termWindow;

DDIColor consoleColors[16] = {
	{0x00, 0x00, 0x00, 0xFF},		/* 0 */
	{0x00, 0x00, 0xAA, 0xFF},		/* 1 */
	{0x00, 0xAA, 0x00, 0xFF},		/* 2 */
	{0x00, 0xAA, 0xAA, 0xFF},		/* 3 */
	{0xAA, 0x00, 0x00, 0xFF},		/* 4 */
	{0xAA, 0x00, 0xAA, 0xFF},		/* 5 */
	{0xAA, 0x55, 0x00, 0xFF},		/* 6 */
	{0xAA, 0xAA, 0xAA, 0xFF},		/* 7 */
	{0x55, 0x55, 0x55, 0xFF},		/* 8 */
	{0x55, 0x55, 0xFF, 0xFF},		/* 9 */
	{0x55, 0xFF, 0x55, 0xFF},		/* A */
	{0x55, 0xFF, 0xFF, 0xFF},		/* B */
	{0xFF, 0x55, 0x55, 0xFF},		/* C */
	{0xFF, 0x55, 0xFF, 0xFF},		/* D */
	{0xFF, 0xFF, 0x55, 0xFF},		/* E */
	{0xFF, 0xFF, 0xFF, 0xFF},		/* F */
};

void clearConsole()
{
	cursorX = 0;
	cursorY = 0;
	
	int i;
	for (i=0; i<80*25; i++)
	{
		grid[i].c = 0;
		grid[i].attr = 0x07;
	};
};

void scroll()
{
	int i;
	for (i=0; i<80*24; i++)
	{
		grid[i].c = grid[i+80].c;
		grid[i].attr = grid[i+80].attr;
	};
	
	for (i=80*24; i<80*25; i++)
	{
		grid[i].c = 0;
		grid[i].attr = currentAttr;
	};
	
	cursorY--;
};

void writeConsole(const char *buf, size_t sz)
{
	while (sz--)
	{
		char c = *buf++;

		if (c == '\b')
		{
			if ((cursorX == 0) && (cursorY == 0))
			{
				continue;
			};
			
			if (cursorX == 0)
			{
				cursorX = 79;
				cursorY--;
			}
			else
			{
				cursorX--;
			};
			
			grid[cursorY * 80 + cursorX].c = 0;
		}
		else if (c == '\n')
		{
			cursorY++;
			cursorX = 0;
		}
		else if (c == '\t')
		{
			cursorX = (cursorX & ~7) + 8;
			if (cursorX > 80)
			{
				cursorX = 0;
				cursorY++;
			};
		}
		else if (c == '\r')
		{
			cursorX = 0;
		}
		else
		{
			grid[cursorY * 80 + cursorX].c = c;
			grid[cursorY * 80 + cursorX].attr = currentAttr;
			cursorX++;
			if (cursorX == 80)
			{
				cursorX = 0;
				cursorY++;
			};
		};
	
		if (cursorY == 25) scroll();
	};
};

void renderConsole(DDISurface *surface)
{
	int x, y;
	for (x=0; x<80; x++)
	{
		for (y=0; y<25; y++)
		{	
			ConsoleCell *cell = &grid[y * 80 + x];
			
			uint8_t attr = cell->attr;
			int pos = y * 80 + x;
			if ((pos >= selectStart) && (pos < selectEnd)) attr = 0x20;
			
			DDIColor *background = &consoleColors[attr >> 4];
			DDIColor *foreground = &consoleColors[attr & 0xF];
			
			unsigned int index = (unsigned char) cell->c;
			const char *bmp = &font[index * 16];
			ddiFillRect(surface, x*9, y*16, 9, 16, background);
			ddiExpandBitmap(surface, x*9, y*16, DDI_BITMAP_8x16, bmp, foreground);
		};
	};
	
	// cursor
	ddiFillRect(surface, cursorX*9, cursorY*16+14, 9, 2, &consoleColors[7]);
	gwmPostDirty(termWindow);
};

void *ctrlThreadFunc(void *context)
{
	char **argv = (char**) context;

	// this thread accepts all signals
	sigset_t sigset;
	sigfillset(&sigset);
	pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
	
	pid_t pid = fork();
	if (pid == -1)
	{
		perror("fork");
		close(fdMaster);
		gwmQuit();
		exit(1);
	}
	else if (pid == 0)
	{
		setsid();
		int fd = open(ptsname(fdMaster), O_RDWR);
		if (fd == -1)
		{
			exit(1);
		};
		
		close(0);
		close(1);
		close(2);
		
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
		
		if (argv[1] == NULL)
		{
			execl("/bin/sh", "sh", NULL);
		}
		else
		{
			argv[0] = "/usr/bin/env";
			execv("/usr/bin/env", argv);
		};
		
		exit(1);
	};
	
	while (1)
	{
		char buffer[4096];
		ssize_t sz = read(fdMaster, buffer, 4096);
		if (sz > 0L)
		{
			pthread_spin_lock(&consoleLock);
			writeConsole(buffer, sz);
			pthread_spin_unlock(&consoleLock);
			gwmPostUpdate(termWindow);
		};
		
		if (!running)
		{
			gwmPostUpdate(termWindow);
			waitpid(pid, NULL, 0);
			break;
		};
	};
	
	return NULL;
};

void onSigChld(int sig)
{
	running = 0;
};

static int getPositionFromCoords(int cx, int cy)
{
	int tx = cx / 9;
	int ty = cy / 16;
	return ty * 80 + tx;
};

int terminalHandler(GWMEvent *ev, GWMWindow *ignore)
{
	DDISurface *surface = gwmGetWindowCanvas(termWindow);
	
	if (ev->type == GWM_EVENT_CLOSE)
	{
		return -1;
	}
	else if (ev->type == GWM_EVENT_UPDATE)
	{
		pthread_spin_lock(&consoleLock);
		renderConsole(surface);
		pthread_spin_unlock(&consoleLock);
		
		if (!running) return -1;
	}
	else if (ev->type == GWM_EVENT_DOWN)
	{
		if (ev->keymod & GWM_KM_CTRL)
		{
			char put = 0;
			if (ev->keycode == 'c')
			{
				put = 0x83;	// CC_VINTR
			}
			else if (ev->keycode == 'k')
			{
				put = 0x84;	// CC_VKILL
			};
			
			if (put != 0)
			{
				write(fdMaster, &put, 1);
			};
		}
		else if (ev->scancode == GWM_SC_MOUSE_LEFT)
		{
			selectStart = selectEnd = selectAnchor = getPositionFromCoords(ev->x, ev->y);
			pthread_spin_lock(&consoleLock);
			renderConsole(surface);
			pthread_spin_unlock(&consoleLock);
		}
		else
		{
			char c = (char) ev->keychar;
			if (c != 0)
			{
				write(fdMaster, &c, 1);
			};
		};
	}
	else if (ev->type == GWM_EVENT_MOTION)
	{
		if (selectAnchor != -1)
		{	
			int nowAt = getPositionFromCoords(ev->x, ev->y);
			
			if (nowAt < selectAnchor)
			{
				selectStart = nowAt;
				selectEnd = selectAnchor;
			}
			else
			{
				selectStart = selectAnchor;
				selectEnd = nowAt;
			};

			pthread_spin_lock(&consoleLock);
			renderConsole(surface);
			pthread_spin_unlock(&consoleLock);
		};
	}
	else if (ev->type == GWM_EVENT_UP)
	{
		if (ev->scancode == GWM_SC_MOUSE_LEFT)
		{
			selectAnchor = -1;
		};
	};
	
	return 0;
};

int main(int argc, char *argv[])
{	
	gwmInit();
	GWMWindow *top = gwmCreateWindow(NULL, "Terminal", 10, 10, 720, 400, GWM_WINDOW_MKFOCUSED);
	GWMWindow *wnd = gwmCreateWindow(top, "body", 0, 0, 720, 400, 0);
	termWindow = wnd;
	
	gwmSetWindowCursor(wnd, GWM_CURSOR_TEXT);
	
	DDISurface *surface = gwmGetWindowCanvas(wnd);
	
	const char *error;
	DDISurface *icon = ddiLoadAndConvertPNG(&surface->format, "/usr/share/images/terminal.png", &error);
	if (icon == NULL)
	{
		printf("Failed to load terminal icon: %s\n", error);
	}
	else
	{
		gwmSetWindowIcon(top, icon);
	};
	
	clearConsole();
	surface = gwmGetWindowCanvas(wnd);
	renderConsole(surface);
	
	fdMaster = getpt();
	if (fdMaster == -1)
	{
		perror("getpt");
		gwmQuit();
		return 1;
	};
	
	grantpt(fdMaster);
	unlockpt(fdMaster);
	
	// this thread will not accept any signals
	sigset_t sigset;
	sigfillset(&sigset);
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	
	signal(SIGCHLD, onSigChld);
	pthread_create(&ctrlThread, NULL, ctrlThreadFunc, argv);
	
	gwmSetEventHandler(top, terminalHandler);
	gwmSetEventHandler(wnd, terminalHandler);
	gwmMainLoop();
	gwmQuit();
	
	return 0;
};
