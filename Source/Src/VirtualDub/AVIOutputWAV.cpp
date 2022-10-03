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

#include "Error.h"

#include "AVIOutputWAV.h"
#include "FastWriteStream.h"

//////////////////////////////////////////////////////////////////////
//																	//
//		AVIOutputWAV (WAV output with AVI interface)				//
//																	//
//////////////////////////////////////////////////////////////////////

AVIOutputWAV::AVIOutputWAV() {
	hFile				= INVALID_HANDLE_VALUE;
	fHeaderOpen			= false;
	dwBytesWritten		= 0;
	fastIO				= NULL;
}

AVIOutputWAV::~AVIOutputWAV() {
	DPRINTF("~AVIOutputWAV:Calling Finalize")
	finalize();
}

BOOL AVIOutputWAV::initOutputStreams() {
	if (!(audioOut = new AVIAudioOutputStream(this))) return FALSE;

	return TRUE;
}

BOOL AVIOutputWAV::init(const char *szFile, LONG xSize, LONG ySize, BOOL videoIn, BOOL audioIn, LONG bufferSize, BOOL is_interleaved) {
	DWORD dwHeader[5];

	if (!audioOut) return FALSE;

	hFile = CreateFile(szFile,
			GENERIC_WRITE,
			FILE_SHARE_READ|FILE_SHARE_WRITE,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		DPRINTF("init hFile == INVALID_HANDLE_VALUE, THROW MyWin32Error")
		throw MyWin32Error("AVIOutputWAV: %%s", GetLastError());
	}
	fastIO = new FastWriteStream(szFile, bufferSize, bufferSize/2);

	dwHeader[0]	= FOURCC_RIFF;
	dwHeader[1] = 0x7F000000;
	dwHeader[2] = mmioFOURCC('W', 'A', 'V', 'E');
	dwHeader[3] = mmioFOURCC('f', 'm', 't', ' ');
	dwHeader[4] = audioOut->getFormatLen();

	_write(dwHeader, 20);
	_write(audioOut->getFormat(), dwHeader[4]);

	if (dwHeader[4] & 1)
		_write("", 1);

	// The 'fact' chunk is required for compressed WAVs and indicates the
	// number of uncompressed samples in the audio.  It is in fact rather
	// useless as it is usually computed from the ratios in the wave header
	// and the size of the compressed data -- check the output of Sound
	// Recorder -- but we must write it out anyway.

	if (audioOut->getWaveFormat()->wFormatTag != WAVE_FORMAT_PCM) {
		dwHeader[0] = mmioFOURCC('f', 'a', 'c', 't');
		dwHeader[1] = 4;
		dwHeader[2] = 0;		// This will be filled in later.
		_write(dwHeader, 12);
	}

	dwHeader[0] = mmioFOURCC('d', 'a', 't', 'a');
	dwHeader[1] = 0x7E000000;

	_write(dwHeader, 8);

	fHeaderOpen = true;

	return TRUE;
}

BOOL AVIOutputWAV::finalize() {
	DPRINTF("AVIOutputWAV::finalize()")
	long len = audioOut->getFormatLen();

	if (hFile != INVALID_HANDLE_VALUE) {
		DWORD dwTemp;
		DWORD dwErr;

		DPRINTF("hFile != INVALID_HANDLE_VALUE")

		if (fastIO) {
			fastIO->Flush1();
			fastIO->Flush2(hFile);
			delete fastIO;
			fastIO = NULL;
		}

		if (fHeaderOpen) {
			DPRINTF("fHeaderOpen is open, Closing")

			const WAVEFORMATEX& wfex = *audioOut->getWaveFormat();
			len = (len+1)&-2;

			if (wfex.wFormatTag == WAVE_FORMAT_PCM) {
				dwTemp = dwBytesWritten + len + 20;
				_seekHdr(4);
				_writeHdr(&dwTemp, 4);

				_seekHdr(24 + len);
				_writeHdr(&dwBytesWritten, 4);
			} else {
				dwTemp = dwBytesWritten + len + 32;
				_seekHdr(4);
				_writeHdr(&dwTemp, 4);

				DWORD dwHeaderRewrite[3];

				dwHeaderRewrite[0] = (DWORD)((__int64)dwBytesWritten * wfex.nSamplesPerSec / wfex.nAvgBytesPerSec);
				dwHeaderRewrite[1] = mmioFOURCC('d', 'a', 't', 'a');
				dwHeaderRewrite[2] = dwBytesWritten;

				_seekHdr(28 + len);
				_writeHdr(dwHeaderRewrite, 12);

				len += 12;		// offset the end of file by length of 'fact' chunk
			}

			fHeaderOpen = false;
		}

		LONG lLo = dwBytesWritten + len + 28;
		LONG lHi = 0;
		DWORD dwError;

		if (0xFFFFFFFF != SetFilePointer(hFile, (LONG)lLo, &lHi, FILE_BEGIN)
			|| (dwError = GetLastError()) != NO_ERROR) {
			DPRINTF("finalize GetLastError!=NO_ERROR (%d)",GetLastError())
			SetEndOfFile(hFile);
		}

		DPRINTF("CloseHandle hFile")
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
		if (dwErr = GetLastError()) {
			DPRINTF("throw MyWin32Error dwErr=%d",dwErr)
			throw MyWin32Error("AVIOutputWAV: %%s", dwErr);
		}
	}

	return TRUE;
}

BOOL AVIOutputWAV::isPreview() { return FALSE; }

void AVIOutputWAV::writeIndexedChunk(FOURCC ckid, LONG dwIndexFlags, LPVOID lpBuffer, LONG cbBuffer) {
	if (ckid != mmioFOURCC('0','1','w','b')) return;

	_write(lpBuffer, cbBuffer);

	dwBytesWritten += cbBuffer;
}

///////////////////////////////////////////////////////////////////////////

void AVIOutputWAV::_writeHdr(void *data, long len) {
	DWORD dwActual;

	if (!WriteFile(hFile, data, len, &dwActual, NULL)|| (long)dwActual != len) {
		DPRINTF("!WriteFile throw MyWin32Error")
		throw MyWin32Error("%s: %%s", GetLastError(), szME);
	}
}

void AVIOutputWAV::_seekHdr(__int64 i64NewPos) {
	LONG lHi = (LONG)(i64NewPos>>32);
	DWORD dwError;

	if (0xFFFFFFFF == SetFilePointer(hFile, (LONG)i64NewPos, &lHi, FILE_BEGIN))
		if ((dwError = GetLastError()) != NO_ERROR) {
			DPRINTF("_seekHdr throw MyWin32Error")
			throw MyWin32Error("%s: %%s", dwError, szME);
		}
}

void AVIOutputWAV::_write(void *data, int len) {
	if (fastIO) {
		fastIO->Put(data,len);
	} else
		_writeHdr(data,len);
}
