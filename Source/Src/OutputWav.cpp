#include "Compiler.h"

//#include <stdio.h>
//#include <stdlib.h>
//#include <signal.h>
//#include <windows.h>
//#include <windowsx.h>
//#include <vfw.h>
//#include <io.h>
//#include <fcntl.h>
//#include <sys\stat.h>

#include "OutputWav.h"
#include "VirtualDub\AVIOutputWav.h"

OutputWAV::OutputWAV(char *path, AVISTREAMINFO *Audio_streamInfo,WAVEFORMATEXTENSIBLE *wavex) {
	Init(path, Audio_streamInfo,wavex);
}

void OutputWAV::Init(char *path, AVISTREAMINFO *Audio_streamInfo,WAVEFORMATEXTENSIBLE *wavex) {
	wavout = new AVIOutputWAV();
	wavout->initOutputStreams();
	AVIStreamHeader_fixed *pAudSi = &wavout->audioOut->streamInfo;
	memset(pAudSi,0,sizeof *pAudSi);
	pAudSi->fccType               = Audio_streamInfo->fccType;
	pAudSi->fccHandler            = Audio_streamInfo->fccHandler;
	pAudSi->dwFlags               = Audio_streamInfo->dwFlags;
	pAudSi->wPriority             = Audio_streamInfo->wPriority;
	pAudSi->wLanguage             = Audio_streamInfo->wLanguage;
	pAudSi->dwInitialFrames       = Audio_streamInfo->dwInitialFrames;
	pAudSi->dwScale               = Audio_streamInfo->dwScale;	
	pAudSi->dwRate                = Audio_streamInfo->dwRate;
	pAudSi->dwStart               = Audio_streamInfo->dwStart;
	pAudSi->dwLength              = Audio_streamInfo->dwLength;
	pAudSi->dwSuggestedBufferSize = Audio_streamInfo->dwSuggestedBufferSize;
	pAudSi->dwQuality             = Audio_streamInfo->dwQuality;
	pAudSi->dwSampleSize          = Audio_streamInfo->dwSampleSize;

	int szOfWavEx = (wavex->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE && wavex->Format.cbSize!=0)
		? sizeof(WAVEFORMATEXTENSIBLE)
		: sizeof(WAVEFORMATEX)-sizeof(wavex->Format.cbSize);// BEWARE, using WAVEFORMATEX size-2 (not incl cbSize, same as VDub)
	wavout->audioOut->allocFormat(szOfWavEx);
	memcpy(wavout->audioOut->getFormat(), wavex, szOfWavEx);

	//BOOL init(const char *szFile, LONG xSize, LONG ySize, BOOL videoIn, BOOL audioIn, LONG bufferSize, BOOL is_interleaved)=0;
	DPRINTF(("OutputWAV::Init->init"))
	wavout->init(path, 0, 0,FALSE, TRUE, (512 * 1024),FALSE);
}

OutputWAV::~OutputWAV(void) {
	delete wavout;
}


void OutputWAV::writeData(void* data, int Samples) {
	wavout->audioOut->write(0, data,Samples * wavout->audioOut->streamInfo.dwSampleSize,Samples);
}





