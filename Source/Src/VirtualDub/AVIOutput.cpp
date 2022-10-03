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

//#include "AudioSource.h"
//#include "VideoSource.h"
#include "AVIIndex.h"
#include "FastWriteStream.h"

#include "wrappedMMIO.h"
#include "Error.h"
#include "AVIOutput.h"
#include "oshelper.h"
#include "misc.h"

///////////////////////////////////////////

extern "C" unsigned long version_num = 1411;

///////////////////////////////////////////

typedef __int64 QUADWORD;

// The following comes from the OpenDML 1.0 spec for extended AVI files

// bIndexType codes
//
#define AVI_INDEX_OF_INDEXES 0x00	// when each entry in aIndex
									// array points to an index chunk

#define AVI_INDEX_OF_CHUNKS 0x01	// when each entry in aIndex
									// array points to a chunk in the file

#define AVI_INDEX_IS_DATA	0x80	// when each entry is aIndex is
									// really the data

// bIndexSubtype codes for INDEX_OF_CHUNKS

#define AVI_INDEX_2FIELD	0x01	// when fields within frames
									// are also indexed
	struct _avisuperindex_entry {
		QUADWORD qwOffset;		// absolute file offset, offset 0 is
								// unused entry??
		DWORD dwSize;			// size of index chunk at this offset
		DWORD dwDuration;		// time span in stream ticks
	};
	struct _avistdindex_entry {
		DWORD dwOffset;			// qwBaseOffset + this is absolute file offset
		DWORD dwSize;			// bit 31 is set if this is NOT a keyframe
	};

#pragma pack(push)
#pragma pack(2)
#pragma warning( disable : 4200 ) // ssS, avoid "warning C4200: nonstandard extension used : zero-sized array in struct/union"

typedef struct _avisuperindex_chunk {
	FOURCC fcc;					// ’ix##’
	DWORD cb;					// size of this structure
	WORD wLongsPerEntry;		// must be 4 (size of each entry in aIndex array)
	BYTE bIndexSubType;			// must be 0 or AVI_INDEX_2FIELD
	BYTE bIndexType;			// must be AVI_INDEX_OF_INDEXES
	DWORD nEntriesInUse;		// number of entries in aIndex array that
								// are used
	DWORD dwChunkId;			// ’##dc’ or ’##db’ or ’##wb’, etc
	DWORD dwReserved[3];		// must be 0
	struct _avisuperindex_entry aIndex[];
} AVISUPERINDEX, * PAVISUPERINDEX;

typedef struct _avistdindex_chunk {
	FOURCC fcc;					// ’ix##’
	DWORD cb;
	WORD wLongsPerEntry;		// must be sizeof(aIndex[0])/sizeof(DWORD)
	BYTE bIndexSubType;			// must be 0
	BYTE bIndexType;			// must be AVI_INDEX_OF_CHUNKS
	DWORD nEntriesInUse;		//
	DWORD dwChunkId;			// ’##dc’ or ’##db’ or ’##wb’ etc..
	QUADWORD qwBaseOffset;		// all dwOffsets in aIndex array are
								// relative to this
	DWORD dwReserved3;			// must be 0
	struct _avistdindex_entry aIndex[];
} AVISTDINDEX, * PAVISTDINDEX;

#pragma pack(pop)

///////////////////////////////////////////

AVIOutputStream::AVIOutputStream(class AVIOutput *output) {
	this->output = output;
	format = NULL;
}

AVIOutputStream::~AVIOutputStream() {
	delete format;
}

BOOL AVIOutputStream::_write(FOURCC ckid, LONG dwIndexFlags, LPVOID lpBuffer, LONG cbBuffer) {
	output->writeIndexedChunk(ckid, dwIndexFlags, lpBuffer, cbBuffer);
//	output->writeIndexedChunk('bd00', dwIndexFlags, lpBuffer, cbBuffer);
	return TRUE;
}

BOOL AVIOutputStream::finalize() {
	_RPT0(0,"AVIOutputStream: finalize()\n");
	return TRUE;
}

////////////////////////////////////

AVIAudioOutputStream::AVIAudioOutputStream(class AVIOutput *out) : AVIOutputStream(out) {
	lTotalSamplesWritten = 0;
}

BOOL AVIAudioOutputStream::write(LONG dwIndexFlags, LPVOID lpBuffer, LONG cbBuffer, LONG lSamples) {
	BOOL success;

	success = _write(mmioFOURCC('0','1','w','b'), dwIndexFlags, lpBuffer, cbBuffer);

	if (success) lTotalSamplesWritten += lSamples;

	return success;
}

BOOL AVIAudioOutputStream::finalize() {
	_RPT0(0,"AVIAudioOutputStream: finalize()\n");

	if (!lTotalSamplesWritten)
		streamInfo.dwLength = 1;
	else
		streamInfo.dwLength = lTotalSamplesWritten;
	return TRUE;
}

BOOL AVIAudioOutputStream::flush() {
	return TRUE;
}

////////////////////////////////////

AVIVideoOutputStream::AVIVideoOutputStream(class AVIOutput *out) : AVIOutputStream(out) {
	lTotalSamplesWritten = 0;
}

BOOL AVIVideoOutputStream::write(LONG dwIndexFlags, LPVOID lpBuffer, LONG cbBuffer, LONG lSamples) {
	BOOL success;

	success = _write(id, dwIndexFlags, lpBuffer, cbBuffer);

	if (success) lTotalSamplesWritten += lSamples;

	return success;
}

BOOL AVIVideoOutputStream::finalize() {
	_RPT0(0,"AVIVideoOutputStream: finalize()\n");

	if (!lTotalSamplesWritten)
		streamInfo.dwLength = 1;
	else
		streamInfo.dwLength = lTotalSamplesWritten;
	return TRUE;
}

////////////////////////////////////

char AVIOutput::szME[]="AVIOutput";

AVIOutput::AVIOutput() {
	audioOut			= NULL;
	videoOut			= NULL;
}

AVIOutput::~AVIOutput() {
	_RPT0(0,"AVIOutput::~AVIOutput()\n");
	delete audioOut;
	delete videoOut;
}

AVIOutputFile::AVIOutputFile() {
	hFile				= NULL;
	fastIO				= NULL;
	index				= NULL;
	index_audio			= NULL;
	index_video			= NULL;
	fCaching			= TRUE;
	i64FilePosition		= 0;
	i64XBufferLevel		= 0;
	xblock				= 0;
	fExtendedAVI		= true;
	lAVILimit			= 0x7F000000L;
	fCaptureMode		= false;
	iPadOffset			= 0;
	pSegmentHint		= NULL;
	cbSegmentHint		= 0;
	fInitComplete		= false;

	pHeaderBlock		= (char *)allocmem(65536);
	nHeaderLen			= 0;
	i64FarthestWritePoint	= 0;
	lLargestIndexDelta[0]	= 0;
	lLargestIndexDelta[1]	= 0;
	i64FirstIndexedChunk[0] = 0;
	i64FirstIndexedChunk[1] = 0;
	i64LastIndexedChunk[0] = 0;
	i64LastIndexedChunk[1] = 0;
	lIndexedChunkCount[0]	= 0;
	lIndexedChunkCount[1]	= 0;
	lIndexSize			= 0;
	fPreemptiveExtendFailed = false;
}

AVIOutputFile::~AVIOutputFile() {
	delete index;
	delete index_audio;
	delete index_video;
	delete pSegmentHint;
	freemem(pHeaderBlock);

	_RPT0(0,"AVIOutputFile: destructor called\n");

	if (hFile) {
		LONG lHi = (LONG)(i64FarthestWritePoint>>32);
		DWORD dwError;

		if (0xFFFFFFFF != SetFilePointer(hFile, (LONG)i64FarthestWritePoint, &lHi, FILE_BEGIN)
			|| (dwError = GetLastError()) != NO_ERROR) {

			SetEndOfFile(hFile);
		}
	}

	delete fastIO;

	if (hFile)
		CloseHandle(hFile);
}

//////////////////////////////////

BOOL AVIOutputFile::initOutputStreams() {
	if (!(audioOut = new AVIAudioOutputStream(this))) return FALSE;
	if (!(videoOut = new AVIVideoOutputStream(this))) return FALSE;

	return TRUE;
}

void AVIOutputFile::disable_os_caching() {
	fCaching = FALSE;
	lChunkSize = 0;
}

void AVIOutputFile::disable_extended_avi() {
	fExtendedAVI = false;
}

void AVIOutputFile::set_1Gb_limit() {
	lAVILimit = 0x3F000000L;
}

void AVIOutputFile::set_chunk_size(long l) {
	lChunkSize = l;
}

void AVIOutputFile::set_capture_mode(bool b) {
	fCaptureMode = b;
}

void AVIOutputFile::setSegmentHintBlock(bool fIsFinal, const char *pszNextPath, int cbBlock) {
	if (!pSegmentHint)
		if (!(pSegmentHint = new char[cbBlock]))
			throw MyMemoryError();

	cbSegmentHint = cbBlock;

	memset(pSegmentHint, 0, cbBlock);

	pSegmentHint[0] = !fIsFinal;
	if (pszNextPath)
		strcpy(pSegmentHint+1, pszNextPath);
}

// I don't like to bitch about other programs (well, okay, so I do), but
// Windows Media Player deserves special attention here.  The ActiveMovie
// implementation of OpenDML hierarchial indexing >2Gb *SUCKS*.  It can't
// cope with a JUNK chunk at the end of the hdrl chunk (even though
// the Microsoft documentation in AVIRIFF.H shows one), requires that
// all standard indexes be the same size except for the last one, and
// requires buffer size information for streams.  NONE of this is required
// by ActiveMovie when an extended index is absent (easily verified by
// changing the 'indx' chunks to JUNK chunks).  While diagnosing these
// problems I got an interesting array of error messages from WMP,
// including:
//
//	o Downloading codec from activex.microsoft.com
//		(Because of an extended index!?)
//	o "Cannot allocate memory because no size has been set"
//		???
//	o "The file format is invalid."
//		Detail: "The file format is invalid. (Error=8004022F)"
//		Gee, that clears everything up.
//	o My personal favorite: recursion of the above until the screen
//		has 100+ dialogs and WMP crashes with a stack fault.
//
// Basically, supporting WMP (or as I like to call it, WiMP) was an
// absolute 100% pain in the ass.

BOOL AVIOutputFile::init(const char *szFile, LONG xSize, LONG ySize, BOOL videoIn, BOOL audioIn, LONG bufferSize, BOOL is_interleaved) {
	DPRINTF("AVIOutputFile::init: path=%s",szFile)
	return _init(szFile, xSize, ySize, videoIn, audioIn, bufferSize, is_interleaved, true);
}

FastWriteStream *AVIOutputFile::initCapture(const char *szFile, LONG xSize, LONG ySize, BOOL videoIn, BOOL audioIn, LONG bufferSize, BOOL is_interleaved) {
	return _init(szFile, xSize, ySize, videoIn, audioIn, bufferSize, is_interleaved, false)
		? fastIO : NULL;
}

BOOL AVIOutputFile::_init(const char *szFile, LONG xSize, LONG ySize, BOOL videoIn, BOOL audioIn, LONG bufferSize, BOOL is_interleaved, bool fThreaded) {
	DPRINTF("AVIOutputFile::_init: path=%s",szFile)
	AVISUPERINDEX asi={0};
	struct _avisuperindex_entry asie_dumb[MAX_SUPERINDEX_ENTRIES];

	fLimitTo4Gb = IsFilenameOnFATVolume(szFile);

	if (audioIn) {
		if (!audioOut) return FALSE;
	} else {
		delete audioOut;
		audioOut = NULL;
	}

	if (!videoOut) return FALSE;

	// Allocate indexes

	if (!(index = new AVIIndex())) return FALSE;

	if (fExtendedAVI) {
		if (!(index_audio = new AVIIndex())) return FALSE;
		if (!(index_video = new AVIIndex())) return FALSE;
	}
DPRINTF("Init main AVI Header")
	// Initialize main AVI header (avih)

	memset(&avihdr, 0, sizeof avihdr);
	avihdr.dwMicroSecPerFrame		= MulDivUnsigned(videoOut->streamInfo.dwScale, 1000000U, videoOut->streamInfo.dwRate);
	avihdr.dwMaxBytesPerSec			= 0;
	avihdr.dwPaddingGranularity		= 0;
	avihdr.dwFlags					= AVIF_HASINDEX | (is_interleaved ? AVIF_ISINTERLEAVED : 0);
	avihdr.dwTotalFrames			= videoOut->streamInfo.dwLength;
	avihdr.dwInitialFrames			= 0;
	avihdr.dwStreams				= audioIn ? 2 : 1;
	avihdr.dwSuggestedBufferSize	= 0;
	avihdr.dwWidth					= xSize;
	avihdr.dwHeight					= ySize;

DPRINTF("Init file")
	// Initialize file

	if (!fCaching) {

		hFile = CreateFile(szFile, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

		if (INVALID_HANDLE_VALUE == hFile) {
DPRINTF("AVIOutputFile::_init Throw MyWin32Error")
			throw MyWin32Error("%s: %%s", GetLastError(), szME);
		}

		if (!(fastIO = new FastWriteStream(szFile, bufferSize, lChunkSize ? lChunkSize : bufferSize/4, fThreaded))) {
DPRINTF("AVIOutputFile::_init Throw MyMemoryError")
			throw MyMemoryError();
		}
	} else {
		hFile = CreateFile(szFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_WRITE_THROUGH, NULL);

		if (INVALID_HANDLE_VALUE == hFile)
			throw MyWin32Error("%s: %%s", GetLastError(), szME);
	}
DPRINTF("AVIOutputFile::_init DONE Createfile")

	i64FilePosition = 0;

	////////// Initialize the first 'AVI ' chunk //////////

	__int64 hdrl_pos;
	__int64 odml_pos;

	DWORD dw[64];

	// start RIFF chunk

	dw[0]	= FOURCC_RIFF;
	dw[1]	= 0;
	dw[2]	= formtypeAVI;

	_writeHdr(dw, 12);

	// start header chunk

	hdrl_pos = _beginList(listtypeAVIHEADER);

	// write out main AVI header

	main_hdr_pos = (long) _writeHdrChunk(ckidAVIMAINHDR, &avihdr, sizeof avihdr);

	_RPT1(0,"Main header is at %08lx\n", main_hdr_pos);

	// start video stream headers

	strl_pos =  (long) _beginList(listtypeSTREAMHEADER);

	// write out video stream header and format

	video_hdr_pos	=  (long) _writeHdrChunk(ckidSTREAMHEADER, &videoOut->streamInfo, sizeof videoOut->streamInfo);
	_writeHdrChunk(ckidSTREAMFORMAT, videoOut->getFormat(), videoOut->getFormatLen());

	_RPT1(0,"Video header is at %08lx\n", video_hdr_pos);

	// write out video superindex (but make it a JUNK chunk for now).

	if (fExtendedAVI) {
		memset(asie_dumb, 0, sizeof asie_dumb);
		video_indx_pos =  (long) _getPosition();
		asi.fcc = ckidAVIPADDING;
		asi.cb = (sizeof asi)-8 + MAX_SUPERINDEX_ENTRIES*sizeof(_avisuperindex_entry);
		_writeHdr(&asi, sizeof asi);
		_writeHdr(asie_dumb, MAX_SUPERINDEX_ENTRIES*sizeof(_avisuperindex_entry));
	}

	// finish video stream header

	_closeList(strl_pos);

	videoOut->streamInfo.dwSuggestedBufferSize = 0;

	// if there is audio...

	if (audioIn) {
		// start audio stream headers

		strl_pos =  (long) _beginList(listtypeSTREAMHEADER);

		// write out audio stream header and format

		audio_hdr_pos	=  (long) _writeHdrChunk(ckidSTREAMHEADER, &audioOut->streamInfo, sizeof audioOut->streamInfo);
		audio_format_pos =  (long) _writeHdrChunk(ckidSTREAMFORMAT, audioOut->getFormat(), audioOut->getFormatLen());

		_RPT1(0,"Audio header is at %08lx\n", audio_hdr_pos);

		// write out audio superindex (but make it a JUNK chunk for now).

		if (fExtendedAVI) {
			audio_indx_pos =  (long) _getPosition();
			asi.fcc = ckidAVIPADDING;
			asi.cb = (sizeof asi)-8 + MAX_SUPERINDEX_ENTRIES*sizeof(_avisuperindex_entry);
			_writeHdr(&asi, sizeof asi);
			_writeHdr(asie_dumb, MAX_SUPERINDEX_ENTRIES*sizeof(_avisuperindex_entry));
		}

		// finish audio stream header

		_closeList(strl_pos);

		audioOut->streamInfo.dwSuggestedBufferSize = 0;
	}

	// write out dmlh header (indicates real # of frames)

	if (fExtendedAVI) {
		odml_pos = _beginList('lmdo');

		memset(dw, 0, sizeof dw);
		dmlh_pos =  (long) _writeHdrChunk('hlmd', dw, 62*4);

		_closeList(odml_pos);
	}

	// write out segment hint block

	if (pSegmentHint)
		seghint_pos =  (long) _writeHdrChunk('mges', pSegmentHint, cbSegmentHint);

	_closeList(hdrl_pos);
//	_flushHdr();

	// pad out to a multiple of 2048 bytes
	//
	// WARNING: ActiveMovie/WMP can't handle a trailing JUNK chunk in hdrl
	//			if an extended index is in use.  It says the file format
	//			is invalid!

	{
		char *s;
		long pad;

		pad = (2048 - ((_getPosition()+8)&2047))&2047;

		if (pad) {
			if (!(s = (char *)allocmem(pad)))
				return FALSE;

			memset(s,0,pad);

			if (pad > 80)
				sprintf(s, "VirtualDub build %d/%s", version_num,
#ifdef _DEBUG
		"debug"
#else
		"release"
#endif
				);

			_writeHdrChunk(ckidAVIPADDING, s, pad);

			freemem(s);
		}

//		// If we are using a fast path, sync the fast path to the slow path

//		if (fastIO)
//			fastIO->Seek(i64FilePosition);
	}

	if (fastIO)
//		fastIO->FlushStart();
		fastIO->Put(pHeaderBlock, nHeaderLen);
	else
		_flushHdr();

	// If we're using the fast path, we're aligned to a sector boundary.
	// Write out the 12 header bytes.

	_openXblock();


	{
		DWORD dwLo, dwHi;

		dwLo = GetFileSize(hFile, &dwHi);

		if (dwLo != 0xFFFFFFFF || GetLastError()==NO_ERROR)
			i64EndOfFile = dwLo | ((__int64)dwHi << 32);
		else
			i64EndOfFile = 0;
	}

	fInitComplete = true;

	return TRUE;
}

BOOL AVIOutputFile::finalize() {
	AVISUPERINDEX asi_video;
	AVISUPERINDEX asi_audio;
	struct _avisuperindex_entry asie_video[MAX_SUPERINDEX_ENTRIES];
	struct _avisuperindex_entry asie_audio[MAX_SUPERINDEX_ENTRIES];
	DWORD dw;
	int i;

	if (!fInitComplete)
		return TRUE;

	if (videoOut) if (!videoOut->finalize()) return FALSE;
	if (audioOut) if (!audioOut->finalize()) return FALSE;

	// fast path: clean it up and resync slow path.

	// create extended indices

	if (fExtendedAVI && xblock != 0) {
		_createNewIndices(index_video, &asi_video, asie_video, false);
		if (audioOut)
			_createNewIndices(index_audio, &asi_audio, asie_audio, true);
	}

	// finish last Xblock

	_closeXblock();

	if (fastIO) {
		char pad[2048+8];

		// pad to next boundary

		_RPT1(0,"AVIOutputFile: starting pad at position %08I64x\n", i64FilePosition);

		*(long *)(pad + 0) = 'KNUJ';
		*(long *)(pad + 4) = (2048 - ((i64FilePosition+8)&2047))&2047;
		memset(pad+8, 0, 2048);
		_write(pad, *(long *)(pad+4) + 8);

		// flush fast path, get disk position

		fastIO->Flush1();
		fastIO->Flush2(hFile);

		// seek slow path up

		_seekDirect(i64FilePosition);

	}	

	// truncate file

	SetEndOfFile(hFile);

	_RPT0(0,"AVIOutputFile: Writing main AVI header...\n");
	_seekHdr(main_hdr_pos+8);
	_writeHdr(&avihdr, sizeof avihdr);

	_RPT0(0,"AVIOutputFile: Writing video header...\n");
	_seekHdr(video_hdr_pos+8);
	_writeHdr(&videoOut->streamInfo, sizeof(AVIStreamHeader_fixed));

	if (audioOut) {
		_RPT0(0,"AVIOutputFile: Writing audio header...\n");
		_seekHdr(audio_hdr_pos+8);
		_writeHdr(&audioOut->streamInfo, sizeof(AVIStreamHeader_fixed));

		// we have to rewrite the audio format, in case someone
		// fixed fields in the format afterward (MPEG-1/L3)

		_RPT0(0,"AVIOutputFile: Writing audio format...\n");

		_seekHdr(audio_format_pos+8);
		_writeHdr(audioOut->getFormat(), audioOut->getFormatLen());
	}

	if (fExtendedAVI) {
		_RPT0(0,"AVIOutputFile: writing dmlh header...\n");
		_seekHdr(dmlh_pos+8);
		dw = videoOut->streamInfo.dwLength;
		_writeHdr(&dw, 4);

		if (xblock > 1) {
			_RPT0(0,"AVIOutputFile: writing video superindex...\n");

			_seekHdr(video_indx_pos);
			_writeHdr(&asi_video, sizeof asi_video);
			_writeHdr(asie_video, sizeof(_avisuperindex_entry)*MAX_SUPERINDEX_ENTRIES);

			if (audioOut) {
				_seekHdr(audio_indx_pos);
				_writeHdr(&asi_audio, sizeof asi_audio);
				_writeHdr(asie_audio, sizeof(_avisuperindex_entry)*MAX_SUPERINDEX_ENTRIES);
			}
		}
	}

	if (pSegmentHint) {
		_seekHdr(seghint_pos+8);
		_writeHdr(pSegmentHint, cbSegmentHint);
	}

	_flushHdr();

	_RPT0(0,"AVIOutputFile: closing RIFF and movi chunks...\n");

	for(i=0; i<xblock; i++) {
		DWORD dwLen;
		
		dwLen = (DWORD)avi_riff_len[i];
		_seekDirect(avi_riff_pos[i]+4);
		_writeDirect(&dwLen, 4);

		dwLen = (DWORD)avi_movi_len[i];
		_seekDirect(avi_movi_pos[i]+4);
		_writeDirect(&dwLen, 4);
	}

	if (!FlushFileBuffers(hFile))
		throw MyWin32Error("%s: %%s", GetLastError(), szME);

	// What do you do when a close fails?

	if (!CloseHandle(hFile)) {
		hFile = NULL;
		throw MyWin32Error("%s: %%s", GetLastError(), szME);
	}

	hFile = NULL;

	return TRUE;
}

BOOL AVIOutputFile::isPreview() { return FALSE; }

long AVIOutputFile::bufferStatus(long *lplBufferSize) {
	if (fastIO) {
		return fastIO->getBufferStatus(lplBufferSize);
	} else {
		return 0;
	}
}

////////////////////////////

__int64 AVIOutputFile::_writeHdr(void *data, long len) {
	if (nHeaderLen < (long)i64FilePosition + len)
		nHeaderLen = (long)i64FilePosition + len;

	memcpy(pHeaderBlock + (long)i64FilePosition, data, len);

	i64FilePosition += len;

	return i64FilePosition - len;
}

__int64 AVIOutputFile::_beginList(FOURCC ckid) {
	DWORD dw[3];

	dw[0] = FOURCC_LIST;
	dw[1] = 0;
	dw[2] = ckid;

	return _writeHdr(dw, 12);
}

__int64 AVIOutputFile::_writeHdrChunk(FOURCC ckid, void *data, long len) {
	DWORD dw[2];
	__int64 pos;

	dw[0] = ckid;
	dw[1] = len;

	pos = _writeHdr(dw, 8);

	_writeHdr(data, len);

	if (len & 1) {
		dw[0] = 0;

		_writeHdr(dw, 1);
	}

	return pos;
}

void AVIOutputFile::_closeList(__int64 pos) {
	DWORD dw;
	__int64 i64FPSave = i64FilePosition;
	
	dw =  (long) (i64FilePosition - (pos+8));

	_seekHdr(pos+4);
	_writeHdr(&dw, 4);
	_seekHdr(i64FPSave);
}

void AVIOutputFile::_flushHdr() {
	DWORD dwActual;
	DWORD dwError;

	if (0xFFFFFFFF == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
		if ((dwError = GetLastError()) != NO_ERROR)
			throw MyWin32Error("%s: %%s", dwError, szME);

	i64FilePosition = 0;

	if (!WriteFile(hFile, pHeaderBlock, nHeaderLen, &dwActual, NULL)
		|| dwActual != nHeaderLen)

		throw MyWin32Error("%s: %%s", GetLastError(), szME);

	i64FilePosition = nHeaderLen;

	if (i64FilePosition > i64FarthestWritePoint)
		i64FarthestWritePoint = i64FilePosition;
}

__int64 AVIOutputFile::_getPosition() {
	return i64FilePosition;
}

void AVIOutputFile::_seekHdr(__int64 i64NewPos) {
	i64FilePosition = i64NewPos;
}

void AVIOutputFile::_seekDirect(__int64 i64NewPos) {
	LONG lHi = (LONG)(i64NewPos>>32);
	DWORD dwError;

//	_RPT1(0,"Seeking to %I64d\n", i64NewPos);

	if (0xFFFFFFFF == SetFilePointer(hFile, (LONG)i64NewPos, &lHi, FILE_BEGIN))
		if ((dwError = GetLastError()) != NO_ERROR)
			throw MyWin32Error("%s: %%s", dwError, szME);

	i64FilePosition = i64NewPos;
}

__int64 AVIOutputFile::_writeDirect(void *data, long len) {
	DWORD dwActual;

	if (!WriteFile(hFile, data, len, &dwActual, NULL)
		|| dwActual != len)

		throw MyWin32Error("%s: %%s", GetLastError(), szME);

	i64FilePosition += len;

	if (i64FilePosition > i64FarthestWritePoint)
		i64FarthestWritePoint = i64FilePosition;

	return i64FilePosition - len;
}

bool AVIOutputFile::_extendFile(__int64 i64NewPoint) {
	bool fSuccess;

	// Have we already extended the file past that point?

	if (i64NewPoint < i64EndOfFile)
		return true;

	// Attempt to extend the file.

	__int64 i64Save = i64FilePosition;

	_seekDirect(i64NewPoint);
	fSuccess = !!SetEndOfFile(hFile);
	_seekDirect(i64Save);

	if (fSuccess) {
		i64EndOfFile = i64NewPoint;
//		_RPT1(0,"Successfully extended file to %I64d bytes\n", i64EndOfFile);
	} else {
//		_RPT1(0,"Failed to extend file to %I64d bytes\n", i64NewPoint);
	}

	return fSuccess;
}

void AVIOutputFile::_write(void *data, int len) {

	if (!fPreemptiveExtendFailed && i64FilePosition + len + lIndexSize > i64EndOfFile - 8388608) {
		fPreemptiveExtendFailed = !_extendFile((i64FilePosition + len + lIndexSize + 16777215) & -8388608);
	}

	if (fastIO) {
		fastIO->Put(data,len);
		i64FilePosition += len;
		if (i64FilePosition > i64FarthestWritePoint)
			i64FarthestWritePoint = i64FilePosition;
	} else
//		_writeHdr(data,len);
		_writeDirect(data, len);
}

void AVIOutputFile::writeIndexedChunk(FOURCC ckid, LONG dwIndexFlags, LPVOID lpBuffer, LONG cbBuffer) {
	AVIIndexEntry2 avie;
	long buf[5];
	static char zero = 0;
	long siz;
	bool fOpenNewBlock = false;
	int nStream = 0;

	if ((ckid&0xffff) > (int)'00')
		nStream = 1;

	// Determine if we need to open another RIFF block (xblock).

	siz = cbBuffer + (cbBuffer&1) + 16;

	// The original AVI format can't accommodate RIFF chunks >4Gb due to the
	// use of 32-bit size fields.  Most RIFF parsers don't handle >2Gb because
	// of inappropriate use of signed variables.  And to top it all off,
	// stupid mistakes in the MCI RIFF parser prevent processing beyond the
	// 1Gb mark.
	//
	// To be save, we keep the first RIFF AVI chunk below 1Gb, and subsequent
	// RIFF AVIX chunks below 2Gb.  We have to leave ourselves a little safety
	// margin (16Mb in this case) for index blocks.

	if (fExtendedAVI)
		if (i64XBufferLevel + siz > (xblock ? 0x7F000000 : lAVILimit))
			fOpenNewBlock = true;

	// Check available disk space.
	//
	// Take the largest separation between data blocks,

	__int64 chunkloc;
	int idxblocksize;
	int idxblocks;
	__int64 maxpoint;

	chunkloc = i64FilePosition;
	if (fOpenNewBlock)
		chunkloc += 24;

	if (!i64FirstIndexedChunk[nStream])
		i64FirstIndexedChunk[nStream] = chunkloc;

	if ((long)(chunkloc - i64LastIndexedChunk[nStream]) > lLargestIndexDelta[nStream])
		lLargestIndexDelta[nStream] = (long)(chunkloc - i64LastIndexedChunk[nStream]);

	++lIndexedChunkCount[nStream];

	// compute how much total space we need to close the file

	idxblocks = 0;

	if (lLargestIndexDelta[0]) {
		idxblocksize = (int)(0x100000000i64 / lLargestIndexDelta[0]);
		if (idxblocksize > MAX_INDEX_ENTRIES)
			idxblocksize = MAX_INDEX_ENTRIES;
		idxblocks = (lIndexedChunkCount[0] + idxblocksize - 1) / idxblocksize;
	}
	if (lLargestIndexDelta[1]) {
		idxblocksize = (int)(0x100000000i64 / lLargestIndexDelta[1]);
		if (idxblocksize > MAX_INDEX_ENTRIES)
			idxblocksize = MAX_INDEX_ENTRIES;
		idxblocks += (lIndexedChunkCount[1] + idxblocksize - 1) / idxblocksize;
	}

	lIndexSize = 0;

	if (fExtendedAVI)
		lIndexSize = idxblocks*sizeof(AVISTDINDEX);

	lIndexSize += 8 + 16*(lIndexedChunkCount[0]+lIndexedChunkCount[1]);
	
	// Give ourselves ~4K of headroom...

	maxpoint = (chunkloc + cbBuffer + 1 + 8 + 14 + 2047 + lIndexSize + 4096) & -2048i64;

	if (fLimitTo4Gb && maxpoint >= 0x100000000i64) {
		_RPT1(0,"overflow detected!  maxpoint=%I64d\n", maxpoint);
		_RPT2(0,"lIndexSize = %08lx (%ld index blocks)\n", lIndexSize, idxblocks);
		_RPT2(0,"sample counts = %ld, %ld\n", lIndexedChunkCount[0], lIndexedChunkCount[1]);

		throw MyError("Out of file space: Files cannot exceed 4 gigabytes on a FAT32 partition.");
	}

	if (!_extendFile(maxpoint))
		throw MyError("Not enough disk space to write additional data.");

	i64LastIndexedChunk[nStream] = chunkloc;

	// If we need to open a new Xblock, do so.

	if (fOpenNewBlock) {
		_closeXblock();
		_openXblock();
	}

	// Write the chunk.

	avie.ckid	= ckid;
	avie.pos	= i64FilePosition - (avi_movi_pos[0]+8); //chunkMisc.dwDataOffset - 2064;
	avie.size	= cbBuffer;

	if (dwIndexFlags & AVIIF_KEYFRAME)
		avie.size |= 0x80000000L;

	buf[0] = ckid;
	buf[1] = cbBuffer;

	_write(buf, 8);

	i64XBufferLevel += siz;

	// ActiveMovie/WMP requires a non-zero dwSuggestedBufferSize for
	// hierarchial indexing (piece of sh*t player).  So we continually
	// bump it up to the largest chunk size;


	if ((unsigned short)ckid == '10') {
		if (DWORD(cbBuffer) > audioOut->streamInfo.dwSuggestedBufferSize)
			audioOut->streamInfo.dwSuggestedBufferSize = cbBuffer;

		if (fExtendedAVI)
			if (!index_audio->add(&avie)) throw MyError("%s error: couldn't add audio chunk to index",szME);
	} else {
		if (DWORD(cbBuffer) > videoOut->streamInfo.dwSuggestedBufferSize)
			videoOut->streamInfo.dwSuggestedBufferSize = cbBuffer;

		if (fExtendedAVI)
			if (!index_video->add(&avie)) throw MyError("%s error: couldn't add video chunk to index",szME);
	}

	if (index)
		if (!index->add(&avie)) throw MyError("%s error: couldn't add chunk to index",szME);

	_write(lpBuffer, cbBuffer);

	// Align to 8-byte boundary, not 2-byte, in capture mode.

	if (fCaptureMode) {
		char *pp;
		int offset = (cbBuffer + iPadOffset) & 7;

		// offset=0:	no action
		// offset=1/2:	[00] 'JUNK' 6 <6 bytes>
		// offset=3/4:	[00] 'JUNK' 4 <4 bytes>
		// offset=5/6:	[00] 'JUNK' 2 <2 bytes>
		// offset=7:	00

		if (offset) {
			buf[0]	= 0;
			buf[1]	= 'KNUJ';
			buf[2]	= (-offset) & 6;
			buf[3]	= 0;
			buf[4]	= 0;

			pp = (char *)&buf[1];

			if (offset & 1)
				--pp;

			_write(pp, (offset & 1) + (((offset+1)&7) ? 8+buf[2] : 0));
		}

		iPadOffset = 0;

	} else {

		// Standard AVI: use 2 bytes

		if (cbBuffer & 1)
			_write(&zero, 1);
	}

}

void AVIOutputFile::_closeXblock() {
	avi_movi_len[xblock] = i64FilePosition - (avi_movi_pos[xblock]+8);

	if (!xblock) {
		avihdr.dwTotalFrames = videoOut->lTotalSamplesWritten;
		_writeLegacyIndex(true);
	}

	avi_riff_len[xblock] = i64FilePosition - (avi_riff_pos[xblock]+8);

	++xblock;

	i64XBufferLevel = 0;
}

void AVIOutputFile::_openXblock() {
	DWORD dw[8];

	if (xblock >= MAX_AVIBLOCKS)
		throw MyError("%s: Exceeded maximum RIFF count (%d)", szME, MAX_AVIBLOCKS);

	// If we're in capture mode, keep this stuff aligned to 8-byte boundaries!

	if (xblock != 0) {

		avi_riff_pos[xblock] = i64FilePosition;

		dw[0] = FOURCC_RIFF;
		dw[1] = 0x7F000000;
		dw[2] = xblock ? 'XIVA' : ' IVA';
		dw[3] = FOURCC_LIST;
		dw[4] = 0x7F000000;
		dw[5] = 'ivom';	// movi
		_write(dw,24);

		avi_movi_pos[xblock] = i64FilePosition - 12;
	} else {
		avi_riff_pos[xblock] = 0;

		avi_movi_pos[xblock] = i64FilePosition;

		dw[0] = FOURCC_LIST;
		dw[1] = 0x7FFFFFFF;
		dw[2] = 'ivom';		// movi

		if (fCaptureMode)
			iPadOffset = 4;

		_write(dw, 12);
	}

	// WARNING: For AVIFile to parse the index correctly, it assumes that the
	// first chunk in an index starts right after the movi chunk!

//	dw[0] = ckidAVIPADDING;
//	dw[1] = 4;
//	dw[2] = 0;
//	_write(dw, 12);
}

void AVIOutputFile::_writeLegacyIndex(bool use_fastIO) {
	if (!index)
		return;

	if (!index->makeIndex())
		throw MyMemoryError();

//	if (use_fastIO && fastIO) {
		DWORD dw[2];

		dw[0] = ckidAVINEWINDEX;
		dw[1] = index->indexLen() * sizeof(AVIINDEXENTRY);
		_write(dw, 8);
		_write(index->indexPtr(), index->indexLen() * sizeof(AVIINDEXENTRY));
//	} else {
//		_writeHdrChunk(ckidAVINEWINDEX, index->indexPtr(), index->indexLen() * sizeof(AVIINDEXENTRY));

	delete index;
	index = NULL;
}

void AVIOutputFile::_createNewIndices(AVIIndex *index, AVISUPERINDEX *asi, _avisuperindex_entry *asie, bool is_audio) {
	AVIIndexEntry2 *asie2;
	int size;
	int actual;
	int indexnum=0;
	int blocksize;

	if (!index || !index->size())
		return;

	if (!index->makeIndex2())
		throw MyMemoryError();

	size = index->indexLen();
	asie2 = index->index2Ptr();

	memset(asie, 0, sizeof(_avisuperindex_entry)*MAX_SUPERINDEX_ENTRIES);

	// Now we run into a bit of a problem.  DirectShow's AVI2 parser requires
	// that all index blocks have the same # of entries (except the last),
	// which is a problem since we also have to guarantee that each block
	// has offsets <4Gb.

	// For now, use a O(n^2) algorithm to find the optimal size.  This isn't
	// really a problem because this routine won't ever be called in time
	// critical circumstances, like streaming video capture.

	blocksize = MAX_INDEX_ENTRIES;

	{
		while(blocksize > 1) {
			int i;
			int nextblock = 0;
			__int64 offset;

			for(i=0; i<size; i++) {
				if (i == nextblock) {
					nextblock += blocksize;
					offset = asie2[i].pos;
				}

				if (asie2[i].pos >= offset + 0x100000000i64)
					break;
			}

			if (i >= size)
				break;

			--blocksize;
		}
	}

	// Write out the actual index blocks.

	while(size > 0) {
		if (indexnum >= MAX_SUPERINDEX_ENTRIES)
			throw MyError("Maximum number of extended AVI indices exceeded (%d)\n", MAX_SUPERINDEX_ENTRIES);

		actual = _writeNewIndex(&asie[indexnum], asie2, min(size, blocksize),
			is_audio ? '10xi' : '00xi', is_audio ? 'bw10' : videoOut->id, is_audio ? audioOut->streamInfo.dwSampleSize : videoOut->streamInfo.dwSampleSize);

		asie2 += actual;
		size -= actual;
		++indexnum;
	}

	memset(asi, 0, sizeof(AVISUPERINDEX));
	asi->fcc			= 'xdni';
	asi->cb				= sizeof(AVISUPERINDEX)-8 + sizeof(_avisuperindex_entry)*MAX_SUPERINDEX_ENTRIES;
	asi->wLongsPerEntry	= 4;
	asi->bIndexSubType	= 0;
	asi->bIndexType		= AVI_INDEX_OF_INDEXES;
	asi->nEntriesInUse	= indexnum;
	asi->dwChunkId		= is_audio ? 'bw10' : videoOut->id;
}

int AVIOutputFile::_writeNewIndex(struct _avisuperindex_entry *asie, AVIIndexEntry2 *avie2, int size, FOURCC fcc, DWORD dwChunkId, DWORD dwSampleSize) {
	AVISTDINDEX asi;
	AVIIndexEntry3 asie3[64];
	__int64 offset = avie2->pos;
	int tc;
	int i;
	int size0;

	// Scan, ascertain how many we can handle without exceeding 4Gb offset

	for(i=0; i<size; i++) {
		if (avie2[i].pos - offset >= 0x100000000i64)
			break;
	}

	size0 = size = i;

	// Check to see if we need to open a new AVIX block

	if (i64XBufferLevel + sizeof(AVISTDINDEX) + size*sizeof(_avistdindex_entry) > (xblock ? 0x7F000000 : lAVILimit)) {
		_closeXblock();
		_openXblock();
	}

	// setup superindex entry

	asie->qwOffset	= i64FilePosition;
	asie->dwSize	= sizeof(AVISTDINDEX) + size*sizeof(_avistdindex_entry);

	if (dwSampleSize) {
		__int64 total_bytes = 0;

		for(int i=0; i<size; i++)
			total_bytes += avie2[i].size & 0x7FFFFFFF;

		asie->dwDuration = (DWORD)(total_bytes / dwSampleSize);
	} else
		asie->dwDuration = size;

	asi.fcc				= ((dwChunkId & 0xFFFF)<<16) | 'xi';
	asi.cb				= asie->dwSize - 8;
	asi.wLongsPerEntry	= 2;
	asi.bIndexSubType	= 0;
	asi.bIndexType		= AVI_INDEX_OF_CHUNKS;
	asi.nEntriesInUse	= size;
	asi.dwChunkId		= dwChunkId;
	asi.qwBaseOffset	= offset;
	asi.dwReserved3		= 0;

	_write(&asi, sizeof asi);

	while(size > 0) {
		tc = size;
		if (tc>64) tc=64;

		for(i=0; i<tc; i++) {
			asie3[i].dwOffset	= (DWORD)(avie2->pos - offset + avi_movi_pos[0] + 16);
			asie3[i].dwSizeKeyframe		= avie2->size;
			++avie2;
		}

		_write(asie3, tc*sizeof(AVIIndexEntry3));

		size -= tc;
	}

	return size0;
}
