#include "Compiler.h"

#include "TWriteWav.h"

TWriteWav::TWriteWav(PClip _child, const char* _fname, bool _overwrite, int _dwcm,IScriptEnvironment *env) :
		GenericVideoFilter(_child), fname(_fname), overwrite(_overwrite), 
			dwcm(_dwcm), closed(false),myName("TWriteWav: ") {

	out			= NULL;
	NextSample  = 0;

	if (!vi.HasAudio())											env->ThrowError("%sInput clip does not contain audio!",myName);
	if (!(*fname))												env->ThrowError("%sNo output filename specified!",myName);
	if(vi.AudioChannels()>8)									env->ThrowError("%sMax 8 Channel Audio",myName);

	memset(&AUDStreamInfo, 0, sizeof(AUDStreamInfo));
	memset(&wavex, 0, sizeof(wavex));

	AUDStreamInfo.fccType				= streamtypeAUDIO;
	AUDStreamInfo.dwScale				= vi.BytesPerAudioSample();
	AUDStreamInfo.dwRate				= vi.audio_samples_per_second*vi.BytesPerAudioSample();
	AUDStreamInfo.dwStart				= 0;							// Starting Sample number
	AUDStreamInfo.dwLength				= (DWORD)vi.num_audio_samples;	// Length in multi-channel samples. WARNING 32 bit
	AUDStreamInfo.dwInitialFrames		= 0;		// 0 : We are not writing video, so always 0

	AUDStreamInfo.dwSuggestedBufferSize	= 0;
	AUDStreamInfo.dwQuality				= DWORD(-1);
	AUDStreamInfo.dwSampleSize			= vi.BytesPerAudioSample();
	strcpy(AUDStreamInfo.szName,"Avisynth Audio");

	DPRINTF("TWriteWAV: BYTES: = %d",(int)(AUDStreamInfo.dwLength*AUDStreamInfo.dwSampleSize))

	int SampleType = vi.SampleType();
	int bits=0;
	bool isFloat=false;
	switch (SampleType) {
		case SAMPLE_INT8:  bits = 8; break;
		case SAMPLE_INT16: bits =16; break;
		case SAMPLE_INT24: bits =24; break;
		case SAMPLE_INT32: bits =32; break;
		case SAMPLE_FLOAT: bits =32; isFloat=true; break;
		default:           env->ThrowError("%sUnknown Audio Format",myName);
	}

	wavex.Format.wFormatTag=WAVE_FORMAT_PCM;
	wavex.Format.nChannels=vi.AudioChannels();
	wavex.Format.nSamplesPerSec=vi.audio_samples_per_second;
	wavex.Format.nAvgBytesPerSec=vi.BytesPerAudioSample() * vi.audio_samples_per_second;;
	wavex.Format.nBlockAlign=vi.BytesPerAudioSample();
	wavex.Format.wBitsPerSample=bits;
	// WARNING, Below cbSize member NOT written for WAVE_FORMAT_PCM, Vdub dont so neither shall we.
	wavex.Format.cbSize=0;

	DPRINTF("TWriteWAV: Channels    = %d",wavex.Format.nChannels)
	DPRINTF("TWriteWAV: Samples     = %d bit",bits)
	DPRINTF("TWriteWAV: SampleRate  = %d Hz",wavex.Format.nSamplesPerSec)
	DPRINTF("TWriteWAV: nBlockAlign = %d",wavex.Format.nBlockAlign)
	DPRINTF("TWriteWAV: Type        = %s",isFloat?"Float":"PCM")

	if(!(wavex.Format.nChannels <= 2 && bits<=16 && dwcm == -1)) {
		DPRINTF("TWriteWAV: Setting WAVE_FORMAT_EXTENSIBLE")
		static int dwcm_map[]={
			0x00004, // 1   -- -- Cf
			0x00003, // 2   Lf Rf
			0x00007, // 3   Lf Rf Cf
			0x00033, // 4   Lf Rf -- -- Lr Rr
			0x00037, // 5   Lf Rf Cf -- Lr Rr
			0x0003F, // 5.1 Lf Rf Cf Sw Lr Rr
			0x0013F, // 6.1 Lf Rf Cf Sw Lr Rr -- -- Cr
			0x0063F, // 7.1 Lf Rf Cf Sw Lr Rr -- -- -- Ls Rs 
		};
		if(dwcm == -1) dwcm = dwcm_map[wavex.Format.nChannels - 1];
		if(dwcm & ~0x3FFFF)					env->ThrowError("%sUnknown Speaker location for dwChannelMask",myName);
		if(dwcm != 0) {						// dwcm of 0 means non located, ie separate MONAURAL channels
			int cnt=0,i;
			for(i=32;--i>=0;) {
				if(dwcm&(1<<i))
					++cnt;
			}
			if(cnt != wavex.Format.nChannels)	env->ThrowError("%sBad dwChannelMask for channel count",myName);
		}

		wavex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		wavex.Format.cbSize=(sizeof wavex - sizeof(WAVEFORMATEX));

		wavex.Samples.wValidBitsPerSample=bits;
		wavex.dwChannelMask=dwcm;
		wavex.SubFormat = (isFloat) ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;
		DPRINTF("TWriteWAV: SubFormat = %s",isFloat?"FLOAT":"PCM")
		DPRINTF("TWriteWAV: dwChannelMask = $%08X",dwcm)
		if(dwcm != 0) {
			static const char* spk[18]={
				"FRONT_LEFT",
				"FRONT_RIGHT",
				"FRONT_CENTER",
				"LOW_FREQUENCY",
				"BACK_LEFT",
				"BACK_RIGHT",
				"FRONT_LEFT_OF_CENTER",
				"FRONT_RIGHT_OF_CENTER",
				"BACK_CENTER",
				"SIDE_LEFT",
				"SIDE_RIGHT",
				"TOP_CENTER",
				"TOP_FRONT_LEFT",
				"TOP_FRONT_CENTER",
				"TOP_FRONT_RIGHT",
				"TOP_BACK_LEFT",
				"TOP_BACK_CENTER",
				"TOP_BACK_RIGHT"
			};
			for(int j=0;j<=17;++j) {
				if(dwcm & 1 << j) {
					DPRINTF("TWriteWAV: Channel %d -> SPEAKER %s",j+1,spk[j])
				}
			}
		} else {
			DPRINTF("TWriteWAV: Multiple Monaural Channels, non-Located")
		}
	}

	// check destination path/name
	void *htemp = _fullpath(fullname, fname, MAX_PATH);
	if (htemp == NULL) env->ThrowError("%serror getting full file path!",myName);
	if (PathFileExists(fullname)) {
		if (overwrite) {
			HANDLE hFile = CreateFile(fullname, GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
			if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
			else env->ThrowError("%soutput file already exists and unable to overwrite!",myName);
		}
		else env->ThrowError("%soutput file already exists!",myName);
	}
	try {
		out = new OutputWAV(fullname, &AUDStreamInfo,&wavex);
	} catch(const MyError& e) {
		Close();
		env->ThrowError("%s:: new OutputWav %s",myName,e.gets());
	} catch(...) {
		Close();
		env->ThrowError("%sError, Cannot new OutputWAV",myName);
	}
}

TWriteWav::~TWriteWav() {
	Close();
}

void TWriteWav::Close() {
	if (!closed) {
		DPRINTF("TWriteWav::Close")
		if (out != NULL) {
			try {
				delete out; out=NULL;
			} catch (...) {
				// Ignore errors on close, for some reason VDub issues "Invalid Window Handle", if file not completey played.
				DPRINTF("CATCHALL caught")
			}
		}
		closed = true;
	}
}


void __stdcall TWriteWav::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
//  start and count are in multichannel samples, and 'start' is beginning of requested output samples to store @ buf.
	if(out == NULL || start+count <= NextSample) {
		// closed or already fully written requested audio
		child->GetAudio(buf, start, count, env);
	} else if(start < NextSample){
		// Requested samples already partially written 
		child->GetAudio(buf, start, count, env);
		__int64 offset = NextSample - start;
		count -= offset;
		if(NextSample + count > vi.num_audio_samples) {
			count = vi.num_audio_samples - NextSample;				// limit file audio clip len
		}
		try { 
			char *p = (char*)buf + vi.BytesFromAudioSamples(offset);
			out->writeData(p,(int)count);
		} catch(const MyWin32Error& e) {
			DPRINTF("GetAudio Caught MyWin32Error")
			Close();
			env->ThrowError("%s:: GetAudio %s",myName,e.gets());
		} catch(const MyError& e) {
			DPRINTF("GetAudio Caught MyError")
			Close();
			env->ThrowError("%s:: GetAudio %s",myName,e.gets());
		} catch(...) {
			DPRINTF("GetAudio CatchAll")
			Close();
			env->ThrowError("%s : GetAudio Error Writing Audio",myName);
		}
		NextSample+=count;
		if(NextSample >= vi.num_audio_samples) {
			DPRINTF("GetAudio Complete")				
			Close();
		}
	} else {
		// NextSample < start, ie need write out all intervening audio before get start(count) samples
		while(NextSample <= start) {
			__int64 cnt = (NextSample==start) ? count : min(count,start-NextSample);
			child->GetAudio(buf, NextSample, cnt, env);
			if(NextSample + cnt > vi.num_audio_samples) {
				cnt = vi.num_audio_samples - NextSample;		// limit audio to clip length
			}
			try { 
				out->writeData(buf,(int)cnt);
			} catch(const MyWin32Error& e) {
				DPRINTF("GetAudio Caught MyWin32Error")
				Close();
				env->ThrowError("%s:: GetAudio %s",myName,e.gets());
			} catch(const MyError& e) {
				DPRINTF("GetAudio Caught MyError")
				Close();
				env->ThrowError("%s:: GetAudio %s",myName,e.gets());
			} catch(...) {
				DPRINTF("GetAudio CatchAll")
				Close();
				env->ThrowError("%s : GetAudio Error Writing Audio",myName);
			}
			NextSample+=cnt;
		}
		if(NextSample >= vi.num_audio_samples) {
			DPRINTF("GetAudio Complete")				
			Close();
		}
	}
}

