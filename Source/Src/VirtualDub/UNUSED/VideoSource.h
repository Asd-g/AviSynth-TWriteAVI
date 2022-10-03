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

#ifndef f_VIDEOSOURCE_H
#define f_VIDEOSOURCE_H

#include <windows.h>
#include <vfw.h>

#include "DubSource.h"

class AVIStripeSystem;
class AVIStripeIndexLookup;
class IMJPEGDecoder;
class IAVIReadHandler;
class IAVIReadStream;

class VideoSource : public DubSource {
protected:
	HANDLE		hBufferObject;
	LONG		lBufferOffset;
	void		*lpvBuffer;
	BITMAPINFOHEADER *bmihDecompressedFormat;
	long		stream_desired_frame;
	long		stream_current_frame;

	void *AllocFrameBuffer(long size);
	void FreeFrameBuffer();

	VideoSource();

public:
	enum {
		IFMODE_NORMAL		=0,
		IFMODE_SWAP			=1,
		IFMODE_SPLIT1		=2,
		IFMODE_SPLIT2		=3,
		IFMODE_DISCARD1		=4,
		IFMODE_DISCARD2		=5,
	};

	virtual ~VideoSource();

	BITMAPINFOHEADER *getImageFormat() const {
		return (BITMAPINFOHEADER *)getFormat();
	}

	void *getFrameBuffer() {
		return lpvBuffer;
	}

	HANDLE getFrameBufferObject() {
		return hBufferObject;
	}

	LONG getFrameBufferOffset() {
		return lBufferOffset;
	}

	virtual bool setDecompressedFormat(int depth);
	virtual bool setDecompressedFormat(BITMAPINFOHEADER *pbih);

	BITMAPINFOHEADER *getDecompressedFormat() {
		return bmihDecompressedFormat;
	}

	virtual void streamSetDesiredFrame(long frame_num);
	virtual long streamGetNextRequiredFrame(BOOL *is_preroll);
	virtual int	streamGetRequiredCount(long *pSize);
	virtual void *streamGetFrame(void *inputBuffer, long data_len, BOOL is_key, BOOL is_preroll, long frame_num) = NULL;

	virtual void streamBegin(bool fRealTime);

	virtual void invalidateFrameBuffer();
	virtual	BOOL isFrameBufferValid() = NULL;

	virtual void *getFrame(LONG frameNum) = NULL;

	virtual char getFrameTypeChar(long lFrameNum) = 0;

	enum eDropType {
		kDroppable		= 0,
		kDependant,
		kIndependent,
	};

	virtual eDropType getDropType(long lFrameNum)=0;

	virtual bool isKeyframeOnly();
	virtual bool isType1();

	virtual long	streamToDisplayOrder(long sample_num) { return sample_num; }
	virtual long	displayToStreamOrder(long display_num) { return display_num; }

	virtual bool isDecodable(long sample_num) = 0;
};

class VideoSourceAVI : public VideoSource {
private:
	IAVIReadHandler *pAVIFile;
	IAVIReadStream *pAVIStream;
	HIC			hicDecomp, hicDecomp2;
	LONG		lLastFrame;
	BITMAPINFOHEADER *bmihTemp;
	BOOL		use_ICDecompressEx;

	AVIStripeSystem			*stripesys;
	IAVIReadHandler			**stripe_files;
	IAVIReadStream			**stripe_streams;
	AVIStripeIndexLookup	*stripe_index;
	int						stripe_count;

	IMJPEGDecoder *mdec;
	HBITMAP		hbmLame;
	bool		fUseGDI;
	bool		fAllKeyFrames;
	bool		bIsType1;
	bool		bDirectDecompress;
	bool		bInvertFrames;

	IAVIReadStream *format_stream;

	char		*key_flags;
	bool		use_internal;
	int			mjpeg_mode;
	void		*mjpeg_reorder_buffer;
	int			mjpeg_reorder_buffer_size;
	long		*mjpeg_splits;
	long		mjpeg_last;
	long		mjpeg_last_size;
	FOURCC		fccForceVideo;
	FOURCC		fccForceVideoHandler;

	void _construct();
	void _destruct();

	bool AttemptCodecNegotiation(BITMAPINFOHEADER *, bool);

public:
	VideoSourceAVI(IAVIReadHandler *pAVI, AVIStripeSystem *stripesys=NULL, IAVIReadHandler **stripe_files=NULL, bool use_internal=false, int mjpeg_mode=0, FOURCC fccForceVideo=0, FOURCC fccForceVideoHandler=0);
	~VideoSourceAVI();

	void Reinit();
	void redoKeyFlags();

	int _read(LONG lStart, LONG lCount, LPVOID lpBuffer, LONG cbBuffer, LONG *lBytesRead, LONG *lSamplesRead);
	BOOL _isKey(LONG samp);
	LONG nearestKey(LONG lSample);
	LONG prevKey(LONG lSample);
	LONG nextKey(LONG lSample);

	bool setDecompressedFormat(int depth);
	bool setDecompressedFormat(BITMAPINFOHEADER *pbih);
	void invalidateFrameBuffer();
	BOOL isFrameBufferValid();
	bool isStreaming();

	void streamBegin(bool fRealTime);
	void *streamGetFrame(void *inputBuffer, long data_len, BOOL is_key, BOOL is_preroll, long frame_num);
	void streamEnd();

	void *getFrame(LONG frameNum);

	HIC	getDecompressorHandle() const { return hicDecomp; }
	bool isUsingInternalMJPEG() const { return !!mdec; }

	char getFrameTypeChar(long lFrameNum);
	eDropType getDropType(long lFrameNum);
	bool isKeyframeOnly();
	bool isType1();
	bool isDecodable(long sample_num);
};

#endif
