#pragma once

#ifndef f_OUTPUTAVI_H
#define f_OUTPUTAVI_H

#include "Compiler.h"

#include "outputformat.h"
#include "virtualdub\AVIOutput.h"

class OutputAvi :
	public OutputFormat
{
private:
	AVIOutputFile* aviout;
	void Init(char *path, AVISTREAMINFO *streamInfo, BITMAPINFOHEADER *bih, AVISTREAMINFO *Audio_streamInfo,WAVEFORMATEXTENSIBLE *wavex);	// ssS
public:
	OutputAvi(char *path, DWORD length, DWORD rate, DWORD scale, DWORD fourCC, DWORD quality, 
			BITMAPINFOHEADER *bih,AVISTREAMINFO *Audio_streamInfo,WAVEFORMATEXTENSIBLE *wavex);												// ssS
	OutputAvi(char *path, AVISTREAMINFO *streamInfo, BITMAPINFOHEADER *bih, AVISTREAMINFO *Audio_streamInfo,WAVEFORMATEXTENSIBLE *wavex);	// ssS
	void writeData(void* data, int dataSize, bool keyframe);
	void Audio_writeData(void* data, int Samples);					// ssS
	~OutputAvi(void);
};


#endif
