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

#ifndef f_FASTWRITESTREAM_H
#define f_FASTWRITESTREAM_H

#include "..\Compiler.h"

class FastWriteStream {
private:
	HANDLE hFile, hFileSlow, hFileClose;
	long lBufferSize;
	long lChunkSize;

	void *lpBuffer;
	long lReadPointer;
	long lWritePointer;
	volatile long lDataPoint;

	HANDLE hThread;
	HANDLE hEventOkRead, hEventOkWrite;

	volatile DWORD dwErrorRet;
	volatile BOOL fDie, fFlush;

	bool fSynchronous;

	/////////

	void _construct(bool);
	void _destruct();

	void ThrowError();

	static unsigned __stdcall BackgroundThreadStart(void *thisPtr);
	void BackgroundWrite(HANDLE, long lOffset, long lSize);
	void BackgroundThread();

public:
	FastWriteStream(const char *lpszFile, long lBufferSize, long lChunkSize, bool fLaunchThread = true);
	FastWriteStream(HANDLE hFile, long lBufferSize, long lChunkSize, bool fLaunchThread = true);
	~FastWriteStream();

	void _Put(void *data, long len);
	void Put(void *data, long len);
	void Putc(char c);
	void Putl(long l);
	long Flush1();
	void Flush2(HANDLE);
	void FlushStart();
	void Seek(__int64 llPos);

	long getBufferStatus(long *lplBufferSize);
	HANDLE getSyncHandle() {
		return hEventOkRead;
	}
	void putError(DWORD);
	bool BackgroundCheck();

	void setSynchronous(bool f) {
		fSynchronous = f;
	}
};

#endif
