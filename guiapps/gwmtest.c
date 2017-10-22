/*
	Glidix GUI

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

#include <libgwm.h>
#include <stdio.h>

int main()
{
	if (gwmInit() != 0)
	{
		fprintf(stderr, "Failed to initialize GWM!\n");
		return 1;
	};
	
	GWMWindow *win = gwmCreateWindow(NULL, "GWM Test", GWM_POS_UNSPEC, GWM_POS_UNSPEC, 0, 0,
					GWM_WINDOW_HIDDEN | GWM_WINDOW_NOTASKBAR);
	
	GWMLayout *box = gwmCreateBoxLayout(GWM_BOX_HORIZONTAL);
	gwmSetWindowLayout(win, box);
	
	gwmBoxLayoutAddWindow(box, gwmCreateStockButton(win, GWM_SYM_YES), 0, 5, GWM_BOX_ALL | GWM_BOX_FILL);
	gwmBoxLayoutAddWindow(box, gwmCreateStockButton(win, GWM_SYM_NO), 1, 5, GWM_BOX_ALL | GWM_BOX_FILL);
	gwmBoxLayoutAddWindow(box, gwmCreateStockButton(win, GWM_SYM_CANCEL), 0, 5, GWM_BOX_ALL | GWM_BOX_FILL);

	gwmFit(win);
	gwmSetWindowFlags(win, GWM_WINDOW_MKFOCUSED | GWM_WINDOW_RESIZEABLE);
	
	GWMGlobWinRef ref;
	gwmGetGlobRef(win, &ref);
	DDISurface *shot = gwmScreenshotWindow(&ref);
	if (shot == NULL)
	{
		fprintf(stderr, "screenshot failed!\n");
	}
	else
	{
		ddiSavePNG(shot, "shot.png", NULL);
	};
	
	gwmMainLoop();	
	gwmQuit();
	return 0;
};