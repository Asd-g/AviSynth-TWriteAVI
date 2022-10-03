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

#include "VirtualDub.h"

#include <process.h>

#include <windows.h>

#include "cpuaccel.h"

#include "Error.h"

#include "FastWriteStream.h"
#include "tls.h"

/*extern*/ CRITICAL_SECTION g_diskcs;
/*extern*/ bool g_disklockinited;

//////////////////////////////////////////////////////////////////////

FastWriteStream::FastWriteStream(const char *lpszFile, long lBufferSize, long lChunkSize, bool fLaunchThread) {

	hFile = hFileClose = CreateFile(
			lpszFile,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING,
			NULL
		);

	if (INVALID_HANDLE_VALUE == hFile) {

		// ARGH.  Attempt open without FILE_FLAG_NO_BUFFERING

		hFile = hFileClose = CreateFile(
				lpszFile,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_WRITE,
				NULL,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_SEQUENTIAL_SCAN,
				NULL
			);

		if (INVALID_HANDLE_VALUE == hFile)
			throw MyError("FastWriteStream: open failed");
	}

	this->lBufferSize		= lBufferSize;
	this->lChunkSize		= lChunkSize;

	_construct(fLaunchThread);
}

FastWriteStream::FastWriteStream(HANDLE hFile, long lBufferSize, long lChunkSize, bool fLaunchThread) {
	this->hFile				= hFile;
	this->lBufferSize		= lBufferSize;
	this->lChunkSize		= lChunkSize;
	this->hFileClose		= NULL;

	_construct(fLaunchThread);
}

void FastWriteStream::_construct(bool fLaunchThread) {
	unsigned uiThreadId;

	lReadPointer			= 0;
	lWritePointer			= 0;
	lDataPoint				= 0;
	lpBuffer				= NULL;

	hEventOkRead			= NULL;
	hEventOkWrite			= NULL;
	hThread					= NULL;

	dwErrorRet				= 0;
	fDie					= FALSE;
	fFlush					= FALSE;
	fSynchronous			= false;

	try {
		// #1:	lChunkSize *must* be a multiple of the disk sector size.
		//
		//		There isn't a reliable way to determine the disk sector size,
		//		because GetDiskFreeSpace() lies for disks >4Gb (FAT32).  So
		//		we're just going to assume a 2K sector size.

		lChunkSize = (lChunkSize + 2047) & -2048;

		// #2:	lBufferSize *must* be a multiple of lChunkSize.
		//
		//		We partition our buffer into a number of chunks, so that we
		//		never have to write subchunks when the buffer wraps, which
		//		could seriously hurt performance.  So let's round up the
		//		buffer size to an integral number of chunks.

		lBufferSize += lChunkSize - 1;

		lBufferSize -= lBufferSize % lChunkSize;

		// Allocate the buffer.  VirtualAlloc() guarantees that the buffer will
		// be aligned to the nearest page boundary, which is 4K or 4Mb on Intel
		// CPUs.

		lpBuffer = VirtualAlloc(NULL, lBufferSize, MEM_COMMIT, PAGE_READWRITE);

		if (!lpBuffer) throw MyMemoryError();

		memset(lpBuffer, 0, lBufferSize);

		// Allocate events.

		if (	!(hEventOkRead  = CreateEvent(NULL, FALSE, FALSE, NULL))
			||	!(hEventOkWrite = CreateEvent(NULL, FALSE, FALSE, NULL)))

			throw MyMemoryError();

	if (!g_disklockinited) {
		g_disklockinited=true;
		InitializeCriticalSection(&g_diskcs);
	}
		// Launch background I/O thread if necessary

		if (fLaunchThread)
			if (!(hThread = (HANDLE)_beginthreadex(NULL, 0, FastWriteStream::BackgroundThreadStart, (void *)this, 0, &uiThreadId)))
				throw MyError("FastWriteStream: couldn't start background I/O thread");

		// All done!

	} catch(...) {
		_destruct();
		throw;
	}
}

void FastWriteStream::_destruct() {
	_RPT0(0,"FastWriteStream::_destruct()\n");

	if (hThread) {
		fDie = TRUE;
		SetEvent(hEventOkRead);
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
	}
	if (hEventOkRead) { CloseHandle(hEventOkRead); hEventOkRead = NULL; }
	if (hEventOkWrite) { CloseHandle(hEventOkWrite); hEventOkWrite = NULL; }
	if (lpBuffer) {
		VirtualFree(lpBuffer, 0, MEM_RELEASE);
		lpBuffer = NULL;
	}
	if (hFileClose && hFileClose != INVALID_HANDLE_VALUE) {
		CloseHandle(hFileClose);
		hFileClose = NULL;
	}
}

FastWriteStream::~FastWriteStream() {
	_destruct();
}

//////////////////////////////////////////////////////////////////////

void FastWriteStream::ThrowError() {
//	throw MyError("FastWriteStream: write error %08lx\n", dwErrorRet);
	DPRINTF("FastWriteStream::ThrowError:")
	throw MyWin32Error("FastWriteStream: %%s\n", dwErrorRet);
}

#pragma function(memcpy)

void FastWriteStream::_Put(void *data, long len) {

	if (dwErrorRet) ThrowError();

	while(len > 0) {
		long buffree;

		while((buffree = lBufferSize - lDataPoint)<=0) {
			if (fSynchronous)
				BackgroundCheck();
			else
				WaitForSingleObject(hEventOkWrite, INFINITE);

			if (dwErrorRet) ThrowError();
		}

		if (lWritePointer + buffree > lBufferSize)
			buffree = lBufferSize - lWritePointer;

		if (buffree > len)
			buffree = len;

		memcpy((char *)lpBuffer + lWritePointer, data, buffree);

		len -= buffree;
		data = (char *)data + buffree;
		lWritePointer += buffree;

		if (lWritePointer >= lBufferSize)
			lWritePointer -= lBufferSize;

		// atomic add

		__asm mov eax,this
		__asm mov ebx,buffree
		__asm lock add [eax]FastWriteStream.lDataPoint,ebx

		// Signal the background thread if there might be enough to write.
		//
		// There's a chance that the background thread flips in here and
		// takes out the data before we get to this point.  But if that
		// happens, well, it just did what we wanted it to anyway, so why
		// bother signalling it?

		if (lDataPoint >= lChunkSize)
			SetEvent(hEventOkRead);
	}
}

void FastWriteStream::Put(void *lpData, long len) {
	_Put(lpData, len);
}

void FastWriteStream::Putc(char c) {
	Put(&c, sizeof c);
}

void FastWriteStream::Putl(long l) {
	Put(&l, sizeof l);
}

long FastWriteStream::Flush1() {

	if (dwErrorRet) ThrowError();

	while(lDataPoint >= lChunkSize) {

		if (fSynchronous)
			BackgroundCheck();
		else
			WaitForSingleObject(hEventOkWrite, INFINITE);

		if (dwErrorRet) ThrowError();

	}

	return lDataPoint;
}

void FastWriteStream::Flush2(HANDLE hFileSlow) {
	fFlush = TRUE;

	this->hFileSlow = hFileSlow;

	if (dwErrorRet) ThrowError();

	while(lDataPoint > 0) {

		SetEvent(hEventOkRead);

		if (fSynchronous)
			BackgroundCheck();
		else
			WaitForSingleObject(hEventOkWrite, INFINITE);

		if (dwErrorRet) ThrowError();

	}
}

void FastWriteStream::FlushStart() {
	FlushFileBuffers(hFile);
}

void FastWriteStream::Seek(__int64 llPos) {
	LONG lLow, lHigh;

	lLow	= (LONG)llPos;
	lHigh	= (LONG)(llPos >> 32);

	// make sure output thread is finished

	Flush1();

	SetLastError(0);

//	_RPT2(0,"FastWriteStream::Seek(%08lx%08lx)\n", lHigh, lLow);

	SetFilePointer(hFile, lLow, &lHigh, FILE_BEGIN);

	if (dwErrorRet = GetLastError())
		ThrowError();

//	_RPT0(0,"FastWriteStream::Seek() exit\n");
}

long FastWriteStream::getBufferStatus(long *lplBufferSize) {
	*lplBufferSize = lBufferSize;

	return lBufferSize - lDataPoint;
}

//////////////////////////////////////////////////////////////////////

void FastWriteStream::putError(DWORD dwErrorRet) {
	this->dwErrorRet = dwErrorRet;
}

unsigned __stdcall FastWriteStream::BackgroundThreadStart(void *lpThisPtr) {
	FastWriteStream *thisPtr = (FastWriteStream *)lpThisPtr;

	InitThreadData("FastWriteStream");

	_RPT2(0,"FastWriteStream thread start: thread=%p, this=%p\n", GetCurrentThreadId(), thisPtr);

	try {

		thisPtr->BackgroundThread();

	} catch(DWORD dwError) {
		thisPtr->putError(dwError);
	}

	while(!thisPtr->fDie) {
		WaitForSingleObject(thisPtr->hEventOkRead, INFINITE);

		SetEvent(thisPtr->hEventOkWrite);
	}

	DeinitThreadData();

	return 0;
}

void FastWriteStream::BackgroundWrite(HANDLE hFile, long lOffset, long lSize) {
	DWORD dwActual;

	EnterCriticalSection(&g_diskcs);
	if (!WriteFile(hFile,(char *)lpBuffer + lOffset,lSize,&dwActual,NULL)) {
		LeaveCriticalSection(&g_diskcs);
		throw GetLastError();
	}
		LeaveCriticalSection(&g_diskcs);

	if ((long)dwActual != lSize)
		throw ERROR_WRITE_FAULT;
}

bool FastWriteStream::BackgroundCheck() {
	// If we can, write an integral number of chunks to disk

	if ((lDataPoint < lChunkSize) && (!fFlush || !lDataPoint))
		return false;

	if (dwErrorRet) {
		SetEvent(hEventOkWrite);
		return true;
	}

	long len;
	HANDLE hFOut = hFile;

	// Our minimal write will be one chunk's size.  However,
	// if there's more data in the buffer (we're playing catch-up),
	// then we may need to split the write, although the minimal
	// size will be at least 1 chunk and still be aligned.
	//
	// The exception is if a flush is asked.  When this happens,
	// a split write may still be required, but the last write will
	// be sector-aligned if the user sector-aligned the data.

	if (fFlush) {
		len = lDataPoint;
/*
#ifdef _DEBUG
		if (len && len < 2048)
			_RPT0(0,"FastWriteStream: Error! Final flushed write is not a 2K multiple!\n");
#endif
*/
		if (len >= 2048)
			len -= len % 2048;

	} else {
		len = lDataPoint - lDataPoint % lChunkSize;

		if (len > lChunkSize)
			len = lChunkSize;
	}

	if (len < 2048) {
		LONG dwLow, dwHigh = 0;
		DWORD dwErrorRet;

		// Determine fast path location

		SetLastError(0);
		dwLow = SetFilePointer(hFile, 0, &dwHigh, FILE_CURRENT);

		if (dwErrorRet = GetLastError())
			throw dwErrorRet;

		// Seek slow path up

		SetFilePointer(hFileSlow, dwLow, &dwHigh, FILE_BEGIN);

		if (dwErrorRet = GetLastError())
			throw dwErrorRet;

		hFOut = hFileSlow;
	}

	if (lReadPointer + len > lBufferSize) {
		long fraction;

		// split write

		fraction = lBufferSize - lReadPointer;

		BackgroundWrite(hFOut, lReadPointer, fraction);
		BackgroundWrite(hFOut, 0, len - fraction);

		lReadPointer = len-fraction;

	} else {

		BackgroundWrite(hFOut, lReadPointer, len);
		lReadPointer += len;

		if (lReadPointer >= lBufferSize)
			lReadPointer -= lBufferSize;

	}

	// Atomically update data point and signal write thread

	__asm mov eax,this
	__asm mov ebx,len
	__asm lock sub [eax]FastWriteStream.lDataPoint,ebx

	SetEvent(hEventOkWrite);

	return true;
}

void FastWriteStream::BackgroundThread() {
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

	while(!fDie) {
		if (!BackgroundCheck())
			WaitForSingleObject(hEventOkRead, INFINITE);
	}
}
