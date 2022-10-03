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

#include "..\Compiler.h"

#include <windows.h>
#include <mmsystem.h>

#include "Error.h"
#include "wrappedMMIO.h"

HMMIO wrappedOpen(char *me, LPSTR szFilename, LPMMIOINFO lpmmioinfo, DWORD dwOpenFlags) {
	HMMIO hmmio;
	MMIOINFO mmi;

	if (lpmmioinfo)
		hmmio = mmioOpen(szFilename, lpmmioinfo, dwOpenFlags);
	else {
		memset(&mmi,0,sizeof mmi);
		hmmio = mmioOpen(szFilename, &mmi, dwOpenFlags);
	}

	if (!hmmio) //throw "Unable to open file.";
		throw MyMMIOError(me, lpmmioinfo ? lpmmioinfo->wErrorRet : mmi.wErrorRet);

	return hmmio;
}

void wrappedClose(char *me, HMMIO mmio, UINT flags) {
	MMRESULT err;

	if (MMSYSERR_NOERROR != (err = mmioClose(mmio, flags)))
		throw MyMMIOError(me, err);
}

void wrappedFlush(char *me, HMMIO mmio, UINT flags) {
	MMRESULT err;

	if (MMSYSERR_NOERROR != (err = mmioFlush(mmio, flags)))
		throw MyMMIOError(me, err);
}

void wrappedCreateChunk(char *me, HMMIO mmio, MMCKINFO *ckinfo, UINT flags) {
	MMRESULT err;

	if (MMSYSERR_NOERROR != (err = mmioCreateChunk(mmio, ckinfo, flags)))
//		throw "Error creating chunk!";
		throw MyMMIOError(me,err);
}

void wrappedStartChunk(char *me, HMMIO mmio, MMCKINFO *mmi, FOURCC ckid) {
	mmi->cksize = 0;
	mmi->ckid = ckid;

	wrappedCreateChunk(me, mmio, mmi, 0);
}

void wrappedStartListChunk(char *me, HMMIO mmio, MMCKINFO *mmi, FOURCC ckid) {
	mmi->cksize = 0;
	mmi->fccType = ckid;

	wrappedCreateChunk(me, mmio, mmi, MMIO_CREATELIST);
}

void wrappedAscend(char *me, HMMIO mmio, MMCKINFO *ckinfo, UINT flags) {
	MMRESULT err;

	if (MMSYSERR_NOERROR != (err = mmioAscend(mmio, ckinfo, flags)))
//		throw "Error finishing chunk!";
		throw MyMMIOError(me,err);
}

void wrappedDescend(char *me, HMMIO hmmio, LPMMCKINFO lpck, LPMMCKINFO lpckParent, UINT wFlags) {
	MMRESULT err;

	if (MMSYSERR_NOERROR != (err = mmioDescend(hmmio, lpck, lpckParent, wFlags)))
//		throw "Error finding chunk!";
		throw MyMMIOError(me,err);
}

void wrappedRead(char *me, HMMIO mmio, char *data, LONG len) {
	LONG lActual;

	if (len != (lActual = mmioRead(mmio, data, len)))
//		throw "Error reading data!";
		if (lActual == -1)
			throw MyError("%s error: couldn't read from file",me);
		else
			throw MyError("%s error:\npremature end of file\n(%ld bytes requested, %ld actually read)",me,len,lActual);
}

void wrappedWrite(char *me, HMMIO mmio, char *data, LONG len) {
	LONG lActual;

	if (len != (lActual = mmioWrite(mmio, data, len)))
//		throw "Error writing data!";
		if (lActual == -1)
			throw MyError("%s error: couldn't write to file",me);
		else
			throw MyError("%s error: only %ld of %ld actually written",me,len,lActual);
}

void wrappedWriteChunk(char *me, HMMIO mmio, FOURCC ckid, char *ckdata, LONG len) {
	MMCKINFO mmi;

	mmi.ckid = ckid;
	mmi.cksize=0;
	wrappedCreateChunk(me, mmio, &mmi, 0);
	wrappedWrite(me, mmio, ckdata, len);
	wrappedAscend(me, mmio, &mmi, 0);
}

LONG wrappedSeek(char *me, HMMIO mmio, LONG lOffset, int iOrigin) {
	LONG lOldOffset;

	lOldOffset = mmioSeek(mmio, lOffset, iOrigin);

	if (lOldOffset == -1)
		throw MyError("%s error: seek failure", me);

	return lOldOffset;
}

LONG wrappedGetPosition(char *me, HMMIO mmio) {
	MMRESULT err;
	MMIOINFO mmi;

	wrappedFlush(me, mmio, MMIO_EMPTYBUF);

	if (err = mmioGetInfo(mmio, &mmi, 0))
		throw MyMMIOError(me, err);

	return mmi.lDiskOffset;
}

