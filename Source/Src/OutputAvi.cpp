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

#include "OutputAvi.h"

OutputAvi::OutputAvi(char *path, DWORD length, DWORD rate, DWORD scale, DWORD fourCC, DWORD quality, 
			BITMAPINFOHEADER *bih,AVISTREAMINFO *Audio_streamInfo,WAVEFORMATEXTENSIBLE *wavex)
{
	AVISTREAMINFO streamInfo;

	memset(&streamInfo, 0, sizeof(AVISTREAMINFO));

	streamInfo.fccType               = streamtypeVIDEO;
	streamInfo.fccHandler            = fourCC;
	streamInfo.dwQuality             = quality;
	streamInfo.dwScale               = scale;
	streamInfo.dwRate                = rate;
	streamInfo.dwLength              = length;
	streamInfo.dwSuggestedBufferSize = 0;

	streamInfo.rcFrame.left = 0;
	streamInfo.rcFrame.top = 0;
	streamInfo.rcFrame.right = bih->biWidth;
	streamInfo.rcFrame.bottom = abs(bih->biHeight);
	

	Init(path, &streamInfo, bih, Audio_streamInfo,wavex);
}

OutputAvi::OutputAvi(char *path, AVISTREAMINFO *streamInfo, BITMAPINFOHEADER *bih,AVISTREAMINFO *Audio_streamInfo,WAVEFORMATEXTENSIBLE *wavex) {
	Init(path, streamInfo, bih, Audio_streamInfo,wavex);
}

void OutputAvi::Init(char *path, AVISTREAMINFO *streamInfo, BITMAPINFOHEADER *bih,
			AVISTREAMINFO *Audio_streamInfo,WAVEFORMATEXTENSIBLE *wavex)
{
	aviout = new AVIOutputFile();
	aviout->initOutputStreams();

	AVIStreamHeader_fixed *pOutSI = &aviout->videoOut->streamInfo;
	memset(pOutSI,0,sizeof *pOutSI);

	pOutSI->fccType               = streamInfo->fccType;
	pOutSI->fccHandler            = streamInfo->fccHandler;
	pOutSI->dwFlags               = streamInfo->dwFlags;
	pOutSI->wPriority             = streamInfo->wPriority;
	pOutSI->wLanguage             = streamInfo->wLanguage;
	pOutSI->dwInitialFrames       = streamInfo->dwInitialFrames;
	pOutSI->dwScale               = streamInfo->dwScale;	
	pOutSI->dwRate                = streamInfo->dwRate;
	pOutSI->dwStart               = streamInfo->dwStart;
	pOutSI->dwLength              = streamInfo->dwLength;
	pOutSI->dwSuggestedBufferSize = streamInfo->dwSuggestedBufferSize;
	pOutSI->dwQuality             = streamInfo->dwQuality;
	pOutSI->dwSampleSize          = streamInfo->dwSampleSize;
	pOutSI->rcFrame.left          = (SHORT)streamInfo->rcFrame.left;
	pOutSI->rcFrame.top           = (SHORT)streamInfo->rcFrame.top;
	pOutSI->rcFrame.right         = (SHORT)streamInfo->rcFrame.right;
	pOutSI->rcFrame.bottom        = (SHORT)streamInfo->rcFrame.bottom;

	aviout->videoOut->setCompressed(TRUE);

	aviout->videoOut->allocFormat(bih->biSize);
	memcpy(aviout->videoOut->getFormat(), bih, bih->biSize);

	aviout->disable_os_caching();

	AVIStreamHeader_fixed *pAudSi = &aviout->audioOut->streamInfo;
	memset(pAudSi,0,sizeof *pAudSi);
	if(Audio_streamInfo != NULL) {										// ssS
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

//		aviout->audioOut->setPrimary(TRUE);

		int szOfWavEx = (wavex->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE && wavex->Format.cbSize!=0)
			? sizeof(WAVEFORMATEXTENSIBLE)
			: sizeof(WAVEFORMATEX)-sizeof(wavex->Format.cbSize);// BEWARE, using WAVEFORMATEX size-2 (not incl cbSize, same as VDub)
		aviout->audioOut->allocFormat(szOfWavEx);
		memcpy(aviout->audioOut->getFormat(), wavex, szOfWavEx);
	}

//	BOOL init(const char *szFile, LONG xSize, LONG ySize, BOOL videoIn, BOOL audioIn, LONG bufferSize, BOOL is_interleaved)=0;
	DPRINTF("Calling aviout->init with path='%s'",path)
	aviout->init(path, streamInfo->rcFrame.right, streamInfo->rcFrame.bottom,
		TRUE, (Audio_streamInfo==NULL)?FALSE:TRUE, (512 * 1024),(Audio_streamInfo==NULL)?FALSE:TRUE);
}

OutputAvi::~OutputAvi(void) {
	aviout->finalize();

	delete aviout;
}

void OutputAvi::writeData(void* data, int dataSize, bool keyframe) {
	aviout->videoOut->write(keyframe ? AVIIF_KEYFRAME : 0, (char *)data, dataSize, 1);
}

void OutputAvi::Audio_writeData(void* data, int Samples) {			// ssS
	aviout->audioOut->write(0, data,Samples * aviout->audioOut->streamInfo.dwSampleSize,Samples);
}





