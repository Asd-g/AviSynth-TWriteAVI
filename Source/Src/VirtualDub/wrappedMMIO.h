//	VirtualDub - Video processing and capture application
//	Copyright (C) 1998-2001 Avery Lee
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef f_WRAPPEDMMIO_H
#define f_WRAPPEDMMIO_H

#include "..\Compiler.h"

#include <mmsystem.h>

HMMIO wrappedOpen			(char *me, LPSTR szFilename, LPMMIOINFO lpmmioinfo, DWORD dwOpenFlags);
void wrappedClose			(char *me, HMMIO mmio, UINT flags);
void wrappedFlush			(char *me, HMMIO mmio, UINT flags);
void wrappedCreateChunk		(char *me, HMMIO mmio, MMCKINFO *ckinfo, UINT flags);
void wrappedStartChunk		(char *me, HMMIO mmio, MMCKINFO *ckinfo, FOURCC ckid);
void wrappedStartListChunk	(char *me, HMMIO mmio, MMCKINFO *ckinfo, FOURCC ckid);
void wrappedAscend			(char *me, HMMIO mmio, MMCKINFO *ckinfo, UINT flags);
void wrappedDescend			(char *me, HMMIO hmmio, LPMMCKINFO lpck, LPMMCKINFO lpckParent, UINT wFlags);
void wrappedRead			(char *me, HMMIO mmio, char *data, LONG len);
void wrappedWrite			(char *me, HMMIO mmio, char *data, LONG len);
void wrappedWriteChunk		(char *me, HMMIO mmio, FOURCC ckid, char *ckdata, LONG len);
LONG wrappedSeek			(char *me, HMMIO mmio, LONG lOffset, int iOrigin);
LONG wrappedGetPosition		(char *me, HMMIO mmio);

#endif
