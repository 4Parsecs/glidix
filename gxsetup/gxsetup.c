/*
	Glidix Installer

	Copyright (c) 2014-2017, Madd Games.
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

#include <sys/stat.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "render.h"
#include "welcome.h"
#include "msgbox.h"
#include "partition.h"
#include "user.h"
#include "pkg.h"

void doInstall()
{
	partEditor();
	promptUserInfo();
	pkgSelection();
	
	partFlush();
	userSetup();
	pkgInstall();
	partUnmount();
	
	msgbox("COMPLETED", "Glidix has been installed. Press ENTER to reboot");
	
	execl("/usr/bin/halt", "reboot", NULL);
	exit(1);
};

int main()
{
	struct termios tc;
	struct termios tcOrig;
	tcgetattr(0, &tcOrig);
	
	tcgetattr(0, &tc);
	tc.c_lflag &= ~(ECHO | ECHONL | ICANON);
	tcsetattr(0, TCSANOW, &tc);
	
	int option;
	while (1)
	{
		option = displayWelcome();
		
		if (option == WELCOME_OPT_INSTALL)
		{
			if (partInit() == 0)
			{
				break;
			};
		}
		else if (option == WELCOME_OPT_SHELL)
		{
			msgbox("NOTICE", "Run 'gxsetup' to return to installer.");
			setColor(0x07);
			write(1, "\e!", 2);
			tcsetattr(0, TCSANOW, &tcOrig);
			
			printf("gxsetup: Dropping to shell (/bin/sh)...\n");
			execl("/bin/sh", "sh", NULL);
			perror("execl");
			
			while (1) pause();
		}
		else if (option == WELCOME_OPT_GWM)
		{
			setColor(0x07);
			write(1, "\e!", 2);
			tcsetattr(0, TCSANOW, &tcOrig);
			
			execl("/usr/bin/gui", "gui", "/usr/bin/gwm-live-start.sh", NULL);
			perror("execl");
			
			while (1) pause();
		}
		else
		{
			msgbox("ERROR", "Feature not yet implemented.");
		};
	};
	
	if (option == WELCOME_OPT_INSTALL)
	{
		doInstall();
	};
	
	return 0;
};
