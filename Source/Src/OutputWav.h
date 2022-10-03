#pragma once

#ifndef f_OUTPUTWAV_H
#define f_OUTPUTWAV_H

#include "OutputFormatWav.h"
#include "virtualdub\AVIOutputWav.h"

class OutputWAV :	
	public OutputFormatWAV
{
private:
	AVIOutputWAV * wavout;
	void Init(char *path, AVISTREAMINFO *Audio_streamInfo,WAVEFORMATEXTENSIBLE *wavex);
public:
	OutputWAV(char *path, AVISTREAMINFO *Audio_streamInfo,WAVEFORMATEXTENSIBLE *wavex);
	void writeData(void* data, int Samples);
	~OutputWAV(void);
};

#endif
