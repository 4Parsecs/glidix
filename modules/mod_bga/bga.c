/*
	Glidix kernel

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

#include <glidix/module.h>
#include <glidix/console.h>
#include <glidix/sched.h>
#include <glidix/storage.h>
#include <glidix/string.h>
#include <glidix/pci.h>
#include <glidix/memory.h>
#include <glidix/port.h>
#include <glidix/time.h>
#include <glidix/semaphore.h>
#include <glidix/idt.h>
#include <glidix/video.h>
#include <glidix/isp.h>
#include <glidix/port.h>

#define	BGA_NUM_MODES			4

#define VBE_DISPI_TOTAL_VIDEO_MEMORY_MB  16
#define VBE_DISPI_4BPP_PLANE_SHIFT       22

#define VBE_DISPI_BANK_ADDRESS           0xA0000
#define VBE_DISPI_BANK_SIZE_KB           64

#define VBE_DISPI_MAX_XRES               2560
#define VBE_DISPI_MAX_YRES               1600
#define VBE_DISPI_MAX_BPP                32

#define VBE_DISPI_IOPORT_INDEX           0x01CE
#define VBE_DISPI_IOPORT_DATA            0x01CF

#define VBE_DISPI_INDEX_ID               0x0
#define VBE_DISPI_INDEX_XRES             0x1
#define VBE_DISPI_INDEX_YRES             0x2
#define VBE_DISPI_INDEX_BPP              0x3
#define VBE_DISPI_INDEX_ENABLE           0x4
#define VBE_DISPI_INDEX_BANK             0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH       0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT      0x7
#define VBE_DISPI_INDEX_X_OFFSET         0x8
#define VBE_DISPI_INDEX_Y_OFFSET         0x9
#define VBE_DISPI_INDEX_VIDEO_MEMORY_64K 0xa

#define VBE_DISPI_ID0                    0xB0C0
#define VBE_DISPI_ID1                    0xB0C1
#define VBE_DISPI_ID2                    0xB0C2
#define VBE_DISPI_ID3                    0xB0C3
#define VBE_DISPI_ID4                    0xB0C4
#define VBE_DISPI_ID5                    0xB0C5

#define VBE_DISPI_BPP_4                  0x04
#define VBE_DISPI_BPP_8                  0x08
#define VBE_DISPI_BPP_15                 0x0F
#define VBE_DISPI_BPP_16                 0x10
#define VBE_DISPI_BPP_24                 0x18
#define VBE_DISPI_BPP_32                 0x20

#define VBE_DISPI_DISABLED               0x00
#define VBE_DISPI_ENABLED                0x01
#define VBE_DISPI_GETCAPS                0x02
#define VBE_DISPI_8BIT_DAC               0x20
#define VBE_DISPI_LFB_ENABLED            0x40
#define VBE_DISPI_NOCLEARMEM             0x80

#define	VBE_DISPI_IOPORT_INDEX		0x01CE
#define	VBE_DISPI_IOPORT_DATA		0x01CF

const char font[16*256] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x7E, 0x81, 0xA5, 0x81, 0x81, 0xBD, 0x99, 0x81, 0x81, 0x7E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x7E, 0xFF, 0xDB, 0xFF, 0xFF, 0xC3, 0xE7, 0xFF, 0xFF, 0x7E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x36, 0x7F, 0x7F, 0x7F, 0x7F, 0x3E, 0x1C, 0x08, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x08, 0x1C, 0x3E, 0x7F, 0x3E, 0x1C, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x18, 0x3C, 0x3C, 0xE7, 0xE7, 0xE7, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x18, 0x3C, 0x7E, 0xFF, 0xFF, 0x7E, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x3C, 0x3C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE7, 0xC3, 0xC3, 0xE7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x66, 0x42, 0x42, 0x66, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC3, 0x99, 0xBD, 0xBD, 0x99, 0xC3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x78, 0x70, 0x58, 0x4C, 0x1E, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xFC, 0xCC, 0xFC, 0x0C, 0x0C, 0x0C, 0x0C, 0x0E, 0x0F, 0x07, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xFE, 0xC6, 0xFE, 0xC6, 0xC6, 0xC6, 0xC6, 0xE6, 0xE7, 0x67, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x18, 0x18, 0xDB, 0x3C, 0xE7, 0x3C, 0xDB, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x7F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x40, 0x60, 0x70, 0x78, 0x7C, 0x7F, 0x7C, 0x78, 0x70, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18, 0x7E, 0x3C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xFE, 0xDB, 0xDB, 0xDB, 0xDE, 0xD8, 0xD8, 0xD8, 0xD8, 0xD8, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x3E, 0x63, 0x06, 0x1C, 0x36, 0x63, 0x63, 0x36, 0x1C, 0x30, 0x63, 0x3E, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x7F, 0x7F, 0x7F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18, 0x7E, 0x3C, 0x18, 0x7E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x3C, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x30, 0x7F, 0x30, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x06, 0x7F, 0x06, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x66, 0xFF, 0x66, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x08, 0x1C, 0x1C, 0x3E, 0x3E, 0x7F, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x7F, 0x7F, 0x3E, 0x3E, 0x1C, 0x1C, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x18, 0x3C, 0x3C, 0x3C, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x66, 0x66, 0x66, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x36, 0x36, 0x7F, 0x36, 0x36, 0x36, 0x7F, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00,
	0x18, 0x18, 0x3E, 0x63, 0x43, 0x03, 0x3E, 0x60, 0x60, 0x61, 0x63, 0x3E, 0x18, 0x18, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x43, 0x63, 0x30, 0x18, 0x0C, 0x06, 0x63, 0x61, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x1C, 0x36, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x33, 0x33, 0x6E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0C, 0x0C, 0x0C, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x0C, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x40, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3C, 0x66, 0xC3, 0xC3, 0xDB, 0xDB, 0xC3, 0xC3, 0x66, 0x3C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x18, 0x1C, 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3E, 0x63, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x63, 0x7F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3E, 0x63, 0x60, 0x60, 0x3C, 0x60, 0x60, 0x60, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x30, 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x30, 0x30, 0x78, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x7F, 0x03, 0x03, 0x03, 0x3F, 0x60, 0x60, 0x60, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x1C, 0x06, 0x03, 0x03, 0x3F, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x7F, 0x63, 0x60, 0x60, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3E, 0x63, 0x63, 0x63, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3E, 0x63, 0x63, 0x63, 0x7E, 0x60, 0x60, 0x60, 0x30, 0x1E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x18, 0x18, 0x0C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3E, 0x63, 0x63, 0x30, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x3E, 0x63, 0x63, 0x7B, 0x7B, 0x7B, 0x3B, 0x03, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x08, 0x1C, 0x36, 0x63, 0x63, 0x7F, 0x63, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3F, 0x66, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x66, 0x66, 0x3F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3C, 0x66, 0x43, 0x03, 0x03, 0x03, 0x03, 0x43, 0x66, 0x3C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x1F, 0x36, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x7F, 0x66, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x46, 0x66, 0x7F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x7F, 0x66, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x06, 0x06, 0x0F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3C, 0x66, 0x43, 0x03, 0x03, 0x7B, 0x63, 0x63, 0x66, 0x5C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x63, 0x63, 0x63, 0x63, 0x7F, 0x63, 0x63, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x33, 0x33, 0x33, 0x1E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x67, 0x66, 0x66, 0x36, 0x1E, 0x1E, 0x36, 0x66, 0x66, 0x67, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x0F, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xC3, 0xE7, 0xFF, 0xFF, 0xDB, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x63, 0x67, 0x6F, 0x7F, 0x7B, 0x73, 0x63, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3F, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x06, 0x06, 0x0F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x6B, 0x7B, 0x3E, 0x30, 0x70, 0x00, 0x00,
	0x00, 0x00, 0x3F, 0x66, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x66, 0x66, 0x67, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3E, 0x63, 0x63, 0x06, 0x1C, 0x30, 0x60, 0x63, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xFF, 0xDB, 0x99, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x66, 0x3C, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xDB, 0xDB, 0xFF, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xC3, 0xC3, 0x66, 0x3C, 0x18, 0x18, 0x3C, 0x66, 0xC3, 0xC3, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xC3, 0xC3, 0xC3, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xFF, 0xC3, 0x61, 0x30, 0x18, 0x0C, 0x06, 0x83, 0xC3, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x0E, 0x1C, 0x38, 0x70, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00,
	0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x33, 0x33, 0x6E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x07, 0x06, 0x06, 0x1E, 0x36, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x63, 0x03, 0x03, 0x03, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x38, 0x30, 0x30, 0x3C, 0x36, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x63, 0x7F, 0x03, 0x03, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x1C, 0x36, 0x26, 0x06, 0x0F, 0x06, 0x06, 0x06, 0x06, 0x0F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x6E, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x33, 0x1E, 0x00,
	0x00, 0x00, 0x07, 0x06, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x66, 0x66, 0x67, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x18, 0x18, 0x00, 0x1C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x60, 0x60, 0x00, 0x70, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x66, 0x66, 0x3C, 0x00,
	0x00, 0x00, 0x07, 0x06, 0x06, 0x66, 0x36, 0x1E, 0x1E, 0x36, 0x66, 0x67, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x1C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0xFF, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x6E, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x30, 0x78, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x06, 0x06, 0x0F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x63, 0x06, 0x1C, 0x30, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x08, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xC3, 0xC3, 0xC3, 0xC3, 0x66, 0x3C, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xC3, 0xC3, 0xC3, 0xDB, 0xDB, 0xFF, 0x66, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xC3, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0xC3, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x7E, 0x60, 0x30, 0x1F, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x33, 0x18, 0x0C, 0x06, 0x63, 0x7F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x70, 0x18, 0x18, 0x18, 0x0E, 0x18, 0x18, 0x18, 0x18, 0x70, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x0E, 0x18, 0x18, 0x18, 0x70, 0x18, 0x18, 0x18, 0x18, 0x0E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x08, 0x1C, 0x36, 0x63, 0x63, 0x63, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x3C, 0x66, 0x43, 0x03, 0x03, 0x03, 0x43, 0x66, 0x3C, 0x30, 0x60, 0x3E, 0x00, 0x00,
	0x00, 0x00, 0x33, 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x30, 0x18, 0x0C, 0x00, 0x3E, 0x63, 0x7F, 0x03, 0x03, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x08, 0x1C, 0x36, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x33, 0x33, 0x6E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x33, 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x33, 0x33, 0x6E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x06, 0x0C, 0x18, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x33, 0x33, 0x6E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x1C, 0x36, 0x1C, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x33, 0x33, 0x6E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x3C, 0x66, 0x06, 0x06, 0x66, 0x3C, 0x30, 0x60, 0x3C, 0x00, 0x00, 0x00,
	0x00, 0x08, 0x1C, 0x36, 0x00, 0x3E, 0x63, 0x7F, 0x03, 0x03, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x63, 0x00, 0x00, 0x3E, 0x63, 0x7F, 0x03, 0x03, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x06, 0x0C, 0x18, 0x00, 0x3E, 0x63, 0x7F, 0x03, 0x03, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x66, 0x00, 0x00, 0x1C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x18, 0x3C, 0x66, 0x00, 0x1C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x06, 0x0C, 0x18, 0x00, 0x1C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x63, 0x00, 0x08, 0x1C, 0x36, 0x63, 0x63, 0x7F, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00,
	0x1C, 0x36, 0x1C, 0x00, 0x1C, 0x36, 0x63, 0x63, 0x7F, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00,
	0x18, 0x0C, 0x06, 0x00, 0x7F, 0x66, 0x06, 0x3E, 0x06, 0x06, 0x66, 0x7F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0xDC, 0xD8, 0x7E, 0x1B, 0x3B, 0xEE, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x7C, 0x36, 0x33, 0x33, 0x7F, 0x33, 0x33, 0x33, 0x33, 0x73, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x08, 0x1C, 0x36, 0x00, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x63, 0x00, 0x00, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x06, 0x0C, 0x18, 0x00, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0C, 0x1E, 0x33, 0x00, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x06, 0x0C, 0x18, 0x00, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x63, 0x00, 0x00, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x7E, 0x60, 0x30, 0x1E, 0x00,
	0x00, 0x63, 0x00, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x63, 0x00, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x18, 0x18, 0x7E, 0xC3, 0x03, 0x03, 0x03, 0xC3, 0x7E, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x1C, 0x36, 0x26, 0x06, 0x0F, 0x06, 0x06, 0x06, 0x06, 0x67, 0x3F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xC3, 0x66, 0x3C, 0x18, 0xFF, 0x18, 0xFF, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x3F, 0x66, 0x66, 0x3E, 0x46, 0x66, 0xF6, 0x66, 0x66, 0x66, 0xCF, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x70, 0xD8, 0x18, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1B, 0x0E, 0x00, 0x00,
	0x00, 0x18, 0x0C, 0x06, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x33, 0x33, 0x6E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x30, 0x18, 0x0C, 0x00, 0x1C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x18, 0x0C, 0x06, 0x00, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x18, 0x0C, 0x06, 0x00, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x6E, 0x3B, 0x00, 0x3B, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00,
	0x6E, 0x3B, 0x00, 0x63, 0x67, 0x6F, 0x7F, 0x7B, 0x73, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x3C, 0x36, 0x36, 0x7C, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x1C, 0x36, 0x36, 0x1C, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x06, 0x03, 0x63, 0x63, 0x3E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x60, 0x60, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x03, 0x03, 0x43, 0x63, 0x33, 0x18, 0x0C, 0x06, 0x73, 0xD9, 0x60, 0x30, 0xF8, 0x00, 0x00,
	0x00, 0x03, 0x03, 0x43, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x73, 0x69, 0x7C, 0x60, 0x60, 0x00, 0x00,
	0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x3C, 0x3C, 0x3C, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x6C, 0x36, 0x1B, 0x36, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x1B, 0x36, 0x6C, 0x36, 0x1B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22,
	0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
	0xBB, 0xEE, 0xBB, 0xEE, 0xBB, 0xEE, 0xBB, 0xEE, 0xBB, 0xEE, 0xBB, 0xEE, 0xBB, 0xEE, 0xBB, 0xEE,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1F, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x1F, 0x18, 0x1F, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6F, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x18, 0x1F, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6F, 0x60, 0x6F, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C,
	0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x60, 0x6F, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C,
	0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6F, 0x60, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x1F, 0x18, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xF8, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xFF, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18, 0xF8, 0x18, 0xF8, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0xEC, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C,
	0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0xEC, 0x0C, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x0C, 0xEC, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C,
	0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0xEF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xEF, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C,
	0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0xEC, 0x0C, 0xEC, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0xEF, 0x00, 0xEF, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C,
	0x18, 0x18, 0x18, 0x18, 0x18, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C,
	0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x18, 0x18, 0x18, 0x18, 0x18, 0xF8, 0x18, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x18, 0xF8, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C,
	0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0xFF, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C,
	0x18, 0x18, 0x18, 0x18, 0x18, 0xFF, 0x18, 0xFF, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
	0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x6E, 0x3B, 0x1B, 0x1B, 0x1B, 0x3B, 0x6E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1B, 0x33, 0x63, 0x63, 0x63, 0x33, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x7F, 0x63, 0x63, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x7F, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x7F, 0x63, 0x06, 0x0C, 0x18, 0x0C, 0x06, 0x63, 0x7F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x0E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x6E, 0x3B, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x7E, 0x18, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x7E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x1C, 0x36, 0x63, 0x63, 0x7F, 0x63, 0x63, 0x36, 0x1C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x36, 0x36, 0x36, 0x77, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x78, 0x0C, 0x18, 0x30, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0xDB, 0xDB, 0xDB, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0xC0, 0x60, 0x7E, 0xDB, 0xDB, 0xCF, 0x7E, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x38, 0x0C, 0x06, 0x06, 0x3E, 0x06, 0x06, 0x06, 0x0C, 0x38, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x0C, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0C, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x30, 0x18, 0x0C, 0x06, 0x0C, 0x18, 0x30, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x70, 0xD8, 0xD8, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1B, 0x1B, 0x1B, 0x0E, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x7E, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x6E, 0x3B, 0x00, 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x1C, 0x36, 0x36, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xF0, 0x30, 0x30, 0x30, 0x30, 0x30, 0x37, 0x36, 0x36, 0x3C, 0x38, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x1B, 0x36, 0x36, 0x36, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0E, 0x1B, 0x0C, 0x06, 0x13, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint64_t				bgaBaseFrame;
static uint64_t				bgaOffset;
static LGIDeviceInterface		bgaInterface;
static size_t				bgaBufferSize;
static FrameList*			bgaFramebuffer = NULL;
static int				bgaWidth, bgaHeight;

static void bgaWriteRegister(unsigned short IndexValue, unsigned short DataValue)
{
	outw(VBE_DISPI_IOPORT_INDEX, IndexValue);
	outw(VBE_DISPI_IOPORT_DATA, DataValue);
}
 
unsigned short bgaReadRegister(unsigned short IndexValue)
{
	outw(VBE_DISPI_IOPORT_INDEX, IndexValue);
	return inw(VBE_DISPI_IOPORT_DATA);
}

void bgaSetVideoMode(unsigned int width, unsigned int height)
{
	bgaWriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
	bgaWriteRegister(VBE_DISPI_INDEX_BPP, VBE_DISPI_BPP_32);
	bgaWriteRegister(VBE_DISPI_INDEX_XRES, width);
	bgaWriteRegister(VBE_DISPI_INDEX_YRES, height);
	bgaWriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

	if (bgaFramebuffer != NULL)
	{
		pdownref(bgaFramebuffer);
	};

	bgaBufferSize = (size_t)(width * height * 4);
	int pages = (width * height * 4)/0x1000;
	if (bgaBufferSize % 0x1000) pages++;

	bgaFramebuffer = pmap(bgaBaseFrame, pages);
	bgaWidth = width;
	bgaHeight = height;
}

static LGIDisplayMode displayModes[BGA_NUM_MODES] = {
	{0, 640, 480},
	{1, 800, 600},
	{2, 1024, 720},
	{3, 1280, 1024}
};

static int bga_getModeInfo(LGIDeviceInterface *intf, LGIDisplayMode *mode)
{
	if (mode->index >= BGA_NUM_MODES)
	{
		return -1;
	};

	memcpy(mode, &displayModes[mode->index], sizeof(LGIDisplayMode));
	return 0;
};

static void bga_setMode(LGIDeviceInterface *intf, LGIDisplayMode *mode)
{
	if (mode->index >= BGA_NUM_MODES)
	{
		return;
	};

	bgaSetVideoMode(mode->width, mode->height);
};

static void bga_write(LGIDeviceInterface *intf, const void *data, size_t size)
{
	uint64_t frame = bgaBaseFrame;
	uint64_t offset = bgaOffset;

	ispLock();
	ispSetFrame(frame);
	char *out = (char*) ispGetPointer();
	const char *in = (const char*) data;

	while (size--)
	{
		out[offset++] = *in++;
		if (offset == 0x1000)
		{
			offset = 0;
			ispSetFrame(++frame);
		};
	};

	ispUnlock();
};

static FrameList *bga_getFramebuffer(LGIDeviceInterface *intf, size_t len, int prot, int flags, off_t offset)
{
	if (offset != 0) return NULL;
	if (len != bgaBufferSize) return NULL;
	if (prot != PROT_WRITE) return NULL;
	if (flags != MAP_SHARED) return NULL;

	pupref(bgaFramebuffer);
	return bgaFramebuffer;
};

static void expandBitmap(uint32_t *framebuffer, int x, int y, const char *bmp)
{
	int xoff, yoff;
	for (yoff=0; yoff<16; yoff++)
	{
		for (xoff=0; xoff<8; xoff++)
		{
			int off = yoff * 8 + xoff;
			int byteidx = off / 8;
			int bitidx = off % 8;
			
			if (bmp[byteidx] & (1 << bitidx))
			{
				framebuffer[(y+yoff) * bgaWidth + (x+xoff)] = 0xFFFFFFFF;
			};
		};
	};
};

static void bga_renderConsole(LGIDeviceInterface *intf, const unsigned char *buffer, int width, int height)
{
	bgaBufferSize = (size_t)(bgaWidth * bgaHeight * 4);
	uint32_t *framebuffer = (uint32_t*) mapPhysMemory(bgaBaseFrame*0x1000, bgaWidth * bgaHeight * 4);
	
	memset(framebuffer, 0, bgaWidth * bgaHeight * 4);
	
	int x, y;
	for (x=0; x<width; x++)
	{
		for (y=0; y<height; y++)
		{
			unsigned int index = (unsigned int) buffer[2 * (y * width + x)];
			const char *bmp = &font[index * 16];
			expandBitmap(framebuffer, x*9, y*16, bmp);
		};
	};
};

static void initBGA(uint32_t bar0)
{
	uint64_t addr = (uint64_t) bar0 & ~0xF;
	bgaBaseFrame = addr / 0x1000;
	bgaOffset = addr % 0x1000;

	bgaInterface.drvdata = NULL;
	bgaInterface.dstat.numModes = BGA_NUM_MODES;
	bgaInterface.getModeInfo = bga_getModeInfo;
	bgaInterface.setMode = bga_setMode;
	bgaInterface.write = bga_write;
	bgaInterface.getFramebuffer = bga_getFramebuffer;
	bgaInterface.renderConsole = bga_renderConsole;

	if (lgiKAddDevice("bga", &bgaInterface) == 0)
	{
		kprintf("bga: registered succesfully with the LGI\n");
	}
	else
	{
		kprintf("bga: failed to register with the LGI\n");
	};
};

static int bgaFound = 0;

static int bga_enumerator(PCIDevice *dev, void *ignore)
{
	(void)ignore;
	
	if (
		((dev->vendor == 0x1234) && (dev->device == 0x1111))
	||	((dev->vendor == 0x80EE) && (dev->device == 0xBEEF))
	)
	{
		kprintf("bga: found BGA\n");
		bgaFound = 1;
		strcpy(dev->deviceName, "Bochs Graphics Adapter (BGA)");
		initBGA(dev->bar[0]);
		return 1;
	};
	
	return 0;
};

MODULE_INIT()
{
	kprintf("bga: enumerating BGA-compatible devices\n");
	pciEnumDevices(THIS_MODULE, bga_enumerator, NULL);

	if (!bgaFound)
	{
		kprintf("bga: device not found\n");
		return MODINIT_CANCEL;
	};

	return MODINIT_OK;
};

MODULE_FINI()
{
	return 0;
};
