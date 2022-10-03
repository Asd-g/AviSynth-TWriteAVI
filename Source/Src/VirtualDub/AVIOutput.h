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

#ifndef f_AVIOUTPUT_H
#define f_AVIOUTPUT_H

#include "..\Compiler.h"

#include <windows.h>
#include <vfw.h>

#include "Fixes.h"

class AVIIndex;
//class AudioSource;
//class VideoSource;
class FastWriteStream;
class AVIIndexEntry2;
typedef struct _avisuperindex_chunk AVISUPERINDEX;
struct _avisuperindex_entry;

//////////////////////////

class AVIOutputStream {
private:
	LONG				formatLen;
	void				*format;

protected:
	class AVIOutput		*output;
	BOOL _write(FOURCC ckid, LONG dwIndexFlags, LPVOID lpBuffer, LONG cbBuffer);

public:
	AVIStreamHeader_fixed		streamInfo;

	AVIOutputStream(class AVIOutput *output);
	virtual ~AVIOutputStream();

	void *allocFormat(LONG len) {
		if (format && formatLen == len) return format;

		delete format;
		return format = new char[formatLen = len];
	}
	void *getFormat() { return format; }
	LONG getFormatLen() { return formatLen; }

	virtual BOOL write(LONG dwIndexFlags, LPVOID lpBuffer, LONG cbBuffer, LONG lSamples) = 0;
	virtual BOOL finalize();

	LONG msToSamples(LONG lMs) {
		return (LONG)(((__int64)lMs * streamInfo.dwRate) / ((__int64)1000 * streamInfo.dwScale));
	}
	LONG samplesToMs(LONG lSamples) {
		return (LONG)((((__int64)lSamples * streamInfo.dwScale) * 1000) / streamInfo.dwScale);
	}
};

class AVIAudioOutputStream : public AVIOutputStream {
public:
	LONG lTotalSamplesWritten;

	AVIAudioOutputStream(class AVIOutput *out);

	WAVEFORMATEX *getWaveFormat() { return (WAVEFORMATEX *)getFormat(); }

	BOOL write(LONG dwIndexFlags, LPVOID lpBuffer, LONG cbBuffer, LONG lSamples);
	virtual BOOL finalize();
	virtual BOOL flush();
};

class AVIVideoOutputStream : public AVIOutputStream {
public:
	LONG lTotalSamplesWritten;
	FOURCC id;

	AVIVideoOutputStream(class AVIOutput *out);

	BITMAPINFOHEADER *getImageFormat() { return (BITMAPINFOHEADER *)getFormat(); }

	void setCompressed(BOOL x) { id = x ? mmioFOURCC('0','0','d','c') : mmioFOURCC('0','0','d','b'); }
	BOOL isCompressed() { return id == mmioFOURCC('0','0','d','c'); }

	BOOL write(LONG dwIndexFlags, LPVOID lpBuffer, LONG cbBuffer, LONG lSamples);
	virtual BOOL finalize();
};

class AVIOutput {
protected:
	static char szME[];
public:
	AVIAudioOutputStream	*audioOut;
	AVIVideoOutputStream	*videoOut;

	AVIOutput();
	virtual ~AVIOutput();

	virtual BOOL initOutputStreams()=0;
	virtual BOOL init(const char *szFile, LONG xSize, LONG ySize, BOOL videoIn, BOOL audioIn, LONG bufferSize, BOOL is_interleaved)=0;
	virtual BOOL finalize()=0;
	virtual BOOL isPreview()=0;

	virtual void writeIndexedChunk(FOURCC ckid, LONG dwIndexFlags, LPVOID lpBuffer, LONG cbBuffer)=0;
};

class AVIOutputFile : public AVIOutput {
private:
	enum { MAX_AVIBLOCKS = 64, MAX_SUPERINDEX_ENTRIES=256, MAX_INDEX_ENTRIES=3072 };

	FastWriteStream *fastIO;
	HANDLE		hFile;
	__int64		i64FilePosition;
	__int64		i64XBufferLevel;

	__int64		avi_movi_pos[64];
	__int64		avi_movi_len[64];
	__int64		avi_riff_pos[64];
	__int64		avi_riff_len[64];
	int			xblock;

	long		strl_pos;
	long		misc_pos;
	long		main_hdr_pos;
	long		audio_hdr_pos;
	long		audio_format_pos;
	long		video_hdr_pos;
	long		audio_indx_pos;
	long		video_indx_pos;
	long		dmlh_pos;
	long		seghint_pos;

	int			chunkFlags;

	AVIIndex	*index, *index_audio, *index_video;
	char *		pHeaderBlock;
	long		nHeaderLen;

	MainAVIHeader		avihdr;

	long		lChunkSize;
	long		lAVILimit;
	int			iPadOffset;

	bool		fCaching;
	bool		fExtendedAVI;
	bool		fCaptureMode;
	bool		fInitComplete;

	char *		pSegmentHint;
	int			cbSegmentHint;

	__int64		i64EndOfFile;
	__int64		i64FarthestWritePoint;
	long		lLargestIndexDelta[2];
	__int64		i64FirstIndexedChunk[2];
	__int64		i64LastIndexedChunk[2];
	bool		fLimitTo4Gb;
	long		lIndexedChunkCount[2];
	long		lIndexSize;
	bool		fPreemptiveExtendFailed;

	BOOL		_init(const char *szFile, LONG xSize, LONG ySize, BOOL videoIn, BOOL audioIn, LONG bufferSize, BOOL is_interleaved, bool fThreaded);

	__int64		_writeHdr(void *data, long len);
	__int64		_beginList(FOURCC ckid);
	__int64		_writeHdrChunk(FOURCC ckid, void *data, long len);
	void		_closeList(__int64 pos);
	void		_flushHdr();
	__int64		_getPosition();
	void		_seekHdr(__int64 i64NewPos);
	bool		_extendFile(__int64 i64NewPoint);
	void		_seekDirect(__int64 i64NewPos);
	__int64		_writeDirect(void *data, long len);

	void		_write(void *data, int len);
	void		_closeXblock();
	void		_openXblock();
	void		_writeLegacyIndex(bool use_fastIO);

	void		_createNewIndices(AVIIndex *index, AVISUPERINDEX *asi, _avisuperindex_entry *asie, bool is_audio);
	int			_writeNewIndex(struct _avisuperindex_entry *asie, AVIIndexEntry2 *avie2, int size, FOURCC fcc, DWORD dwChunkId, DWORD dwSampleSize);
public:
	AVIOutputFile();
	virtual ~AVIOutputFile();

	void disable_os_caching();
	void disable_extended_avi();
	void set_1Gb_limit();
	void set_chunk_size(long cs);
	void set_capture_mode(bool b);
	void setSegmentHintBlock(bool fIsFinal, const char *pszNextPath, int cbBlock);

	BOOL initOutputStreams();
	BOOL init(const char *szFile, LONG xSize, LONG ySize, BOOL videoIn, BOOL audioIn, LONG bufferSize, BOOL is_interleaved);
	FastWriteStream *initCapture(const char *szFile, LONG xSize, LONG ySize, BOOL videoIn, BOOL audioIn, LONG bufferSize, BOOL is_interleaved);

	BOOL finalize();
	BOOL isPreview();

	void writeIndexedChunk(FOURCC ckid, LONG dwIndexFlags, LPVOID lpBuffer, LONG cbBuffer);

	LONG bufferStatus(LONG *lplBufferSize);
};

#endif
