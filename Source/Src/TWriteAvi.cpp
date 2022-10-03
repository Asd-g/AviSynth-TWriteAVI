#include "Compiler.h"

#include "TWriteAvi.h"

TWriteAvi::TWriteAvi(PClip _child, const char* _fname, bool _overwrite, bool _showAll,
		const char* _fourcc, int _dwcm,IScriptEnvironment *env) :
		GenericVideoFilter(_child), fname(_fname), overwrite(_overwrite), 
			showAll(_showAll),dwcm(_dwcm), closed(false),myName("TWriteAVI: ") {

	KFCount		= BFCount = 0;
	NextFrame	= 0;
	NextSample  = 0;
	vComplete	= false;
	aComplete	= false;

	lpBuffer	= NULL;
	out			= NULL;

	child->SetCacheHints(CACHE_NOTHING, 0);
	memset(&bih, 0, sizeof(BITMAPINFOHEADER));
	memset(&cvar, 0, sizeof(COMPVARS));
	memset(&SRCStreamInfo, 0, sizeof(SRCStreamInfo));
	memset(&AUDStreamInfo, 0, sizeof(AUDStreamInfo));
	memset(&wavex, 0, sizeof(wavex));


	// ssS
	if (!vi.HasVideo() || vi.num_frames<=0)						env->ThrowError("%sInput clip does not contain video!",myName);
	if (!(*fname))												env->ThrowError("%sNo output filename specified!",myName);
	if(vi.AudioChannels()>8)									env->ThrowError("%sMax 8 Channel Audio",myName);

	P_xSubS=P_ySubS=1;											// Planar YUV only, 1 = no chroma sub sampling
	P_hasChroma=false;
	if(vi.IsPlanar()) {
		# ifdef AVISYNTH_PLUGIN_25
			//                     YV12
			if(vi.pixel_type != 0xA0000008) {
				// Planar but NOT YV12, v2.5 Plugin Does NOT support v2.6+ ColorSpace
				env->ThrowError("%sUnsupported for colorSpace in v2.5 plugin",myName);			
			}
		# endif
		PVideoFrame	src	= child->GetFrame(0, env);				// get frame 0
		int	rowsizeUV   = src->GetRowSize(PLANAR_U);
		if(rowsizeUV!=0) {										// Not Y8
			P_xSubS = src->GetRowSize(PLANAR_Y) / rowsizeUV;
			P_ySubS = src->GetHeight(PLANAR_Y)  / src->GetHeight(PLANAR_U);
			// We dont support YV411 (or anything else weird)
			if(P_xSubS>2 || P_ySubS>2 || P_ySubS > P_xSubS)		env->ThrowError("%sUnsupported colorspace!",myName);
			P_hasChroma=true;
		}
	} else if(!(vi.IsRGB24() || vi.IsRGB32() || vi.IsYUY2()))	env->ThrowError("%sUnsupported colorspace!",myName);

	// ssS
	if(vi.HasAudio()) {
		AUDStreamInfo.fccType				= streamtypeAUDIO;
		AUDStreamInfo.dwScale				= vi.BytesPerAudioSample();
		AUDStreamInfo.dwRate				= vi.audio_samples_per_second*vi.BytesPerAudioSample();
		AUDStreamInfo.dwStart				= 0;							// Starting Sample number
		AUDStreamInfo.dwLength				= (DWORD)vi.num_audio_samples;	// Length in multi-channel samples. WARNING 32 bit
		AUDStreamInfo.dwInitialFrames		= 1;	// 0 or 1 : Audio skew for Interleaved.

		AUDStreamInfo.dwSuggestedBufferSize	= (vi.audio_samples_per_second/2)*vi.BytesPerAudioSample();		// 0.5 secs worth
		AUDStreamInfo.dwQuality				= DWORD(-1);
		AUDStreamInfo.dwSampleSize			= vi.BytesPerAudioSample();
		strcpy(AUDStreamInfo.szName,"Avisynth Audio");

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

		wavex.Format.wFormatTag			= WAVE_FORMAT_PCM;
		wavex.Format.nChannels			= vi.AudioChannels();
		wavex.Format.nSamplesPerSec		= vi.audio_samples_per_second;
		wavex.Format.nAvgBytesPerSec	= vi.BytesPerAudioSample() * vi.audio_samples_per_second;;
		wavex.Format.nBlockAlign		= vi.BytesPerAudioSample();
		wavex.Format.wBitsPerSample		= bits;
		// WARNING, Below cbSize member NOT written for WAVE_FORMAT_PCM, Vdub dont so neither shall we.
		wavex.Format.cbSize=0;

		DPRINTF("Channels    = %d",wavex.Format.nChannels)
		DPRINTF("Samples     = %d bit",bits)
		DPRINTF("SampleRate  = %d Hz",wavex.Format.nSamplesPerSec)
		DPRINTF("nBlockAlign = %d",wavex.Format.nBlockAlign)
		DPRINTF("Type        = %s",isFloat?"Float":"PCM")

		if(!(wavex.Format.nChannels <= 2 && bits<=16 && dwcm == -1)) {
			DPRINTF("Setting WAVE_FORMAT_EXTENSIBLE")
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
			DPRINTF("SubFormat = %s",isFloat?"FLOAT":"PCM")
			DPRINTF("dwChannelMask = $%08X",dwcm)
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
						DPRINTF("Channel %d -> SPEAKER %s",j+1,spk[j])
					}
				}
			} else {
				DPRINTF("Multiple Monaural Channels, non-Located")
			}
		}
	} else {
		aComplete=true;
	}

	DPRINTF("CALL FULLPATH")
	// check destination path/name
	void *htemp = _fullpath(fullname, fname, MAX_PATH);
	if (htemp == NULL) env->ThrowError("%serror getting full file path!",myName);
	DPRINTF("DONE FULLPATH")
	if (PathFileExists(fullname)) {
		DPRINTF("EXISTS")
		if (overwrite) {
			HANDLE hFile = CreateFile(fullname, GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
			if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
			else env->ThrowError("%soutput file already exists and unable to overwrite!",myName);
		}
		else env->ThrowError("%soutput file already exists!",myName);
	}
	DPRINTF("CHECKED EXIST")
	// fill BITMAPINFOHEADER bih info
	bih.biSize     = sizeof(BITMAPINFOHEADER); 

	bih.biWidth    = vi.width;
	bih.biHeight   = vi.height;

	bih.biPlanes   = 1;
	bih.biBitCount = vi.BitsPerPixel();
	if (vi.IsYUY2())							bih.biCompression = '2YUY';								// YUY2
	else if(vi.IsYUV()) {
		if(vi.IsPlanar()) {
			if (!P_hasChroma)					bih.biCompression = '008Y';								// Y8
			else if (vi.IsYV12())				bih.biCompression = '21VY';								// YV12
			else if (P_ySubS==1&&P_xSubS==2)	bih.biCompression = '61VY';								// YV16
			else if (P_ySubS==1&&P_xSubS==1)	bih.biCompression = '42VY';								// YV24
			else								env->ThrowError("%sColorspace not supported",myName);
		} else 									env->ThrowError("%sColorspace not supported",myName);
	}
	else if(vi.IsRGB24() || vi.IsRGB32())		bih.biCompression = BI_RGB;								// RGB24, RGB32
	else										env->ThrowError("%sColorspace not supported",myName);
	bih.biSizeImage = vi.BMPSize();

	// select compressor and fill COMPVARS cvar info
	cvar.cbSize = sizeof(COMPVARS);
	if (*_fourcc) {
		if (strlen(_fourcc) != 4) env->ThrowError("%sFourCC must be exactly four characters long (%s)",myName,_fourcc);
		DWORD fourcc = *(DWORD*)_fourcc;
		cvar.dwFlags = ICMF_COMPVARS_VALID;
		cvar.hic = ICOpen(ICTYPE_VIDEO, fourcc, ICMODE_COMPRESS);
		if (cvar.hic == 0) env->ThrowError("%sCouldn't open a compressor with FourCC code %s",myName,_fourcc);
		cvar.lQ = ICQUALITY_DEFAULT;
		cvar.fccHandler = fourcc;
		cvar.fccType = ICTYPE_VIDEO;
	} else if (showAll) {
		if(!ICCompressorChoose(NULL, ICMF_CHOOSE_ALLCOMPRESSORS | ICMF_CHOOSE_DATARATE | 
			ICMF_CHOOSE_KEYFRAME | ICMF_CHOOSE_PREVIEW, &bih, NULL, &cvar, (LPSTR)fname)) 
			env->ThrowError("%sICCompressorChoose failed!",myName);
	} else {
		if(!ICCompressorChoose(NULL, ICMF_CHOOSE_DATARATE | ICMF_CHOOSE_KEYFRAME | ICMF_CHOOSE_PREVIEW, 
			&bih, NULL, &cvar, (LPSTR)fname)) env->ThrowError("%sICCompressorChoose failed!",myName);
	}
	if (cvar.fccHandler == FCC_DIB) {
		Close();
		env->ThrowError("%svalid compressor not chosen (FCC_DIB, Full Frames)!",myName);
	}

	// fill AVISTREAMINFO SRCStreamInfo info
	SRCStreamInfo.fccType		= streamtypeVIDEO;
	SRCStreamInfo.fccHandler	= cvar.fccHandler;
	SRCStreamInfo.dwScale		= vi.fps_denominator;
	SRCStreamInfo.dwRate		= vi.fps_numerator;
	SRCStreamInfo.dwLength		= vi.num_frames;
	SRCStreamInfo.dwSuggestedBufferSize = vi.BMPSize();
	SRCStreamInfo.dwQuality		= cvar.lQ;
	SRCStreamInfo.dwSampleSize	= 0;
	SRCStreamInfo.rcFrame.right = vi.width;
	SRCStreamInfo.rcFrame.bottom= vi.height;
	strcpy(SRCStreamInfo.szName,"Avisynth Video");
	// allocate temporary storage buffer - lpBuffer
	lpBuffer = (unsigned char *)malloc(vi.BMPSize());
	if (lpBuffer == NULL) {
		Close();
		env->ThrowError("%smalloc failure!",myName);
	}

	memset(lpBuffer,0,vi.BMPSize());		 // Zero the buffer

	// set up compressor and output object
	if (NO_RECOMPRESS) {
		try {
			out = new OutputAvi(fullname, &SRCStreamInfo, &bih,vi.HasAudio()?&AUDStreamInfo:NULL,&wavex);
		} catch(const MyError& e) {
			Close();
			env->ThrowError("%snew OutputAVI() %s",myName,e.gets());
		} catch(...) {
			Close();
			env->ThrowError("%sError, new OutputAVI  failed on\n%s",myName,fullname);
		}
	} else {
		int formatSize = ICCompressGetFormatSize(cvar.hic, (void*)&bih);
		BITMAPINFOHEADER *bih2 = (BITMAPINFOHEADER*)malloc(formatSize);
		if(bih2==NULL) {
			Close();
			env->ThrowError("%sError allocating bih2",myName);
		}
		memset(bih2, 0, formatSize);
		ICCompressGetFormat(cvar.hic, &bih, bih2);
		if (bih2->biSize == 0) bih2->biSize = formatSize;
		DPRINTF("CALLING new OutputAvi")
		try {
			out = new OutputAvi(fullname, SRCStreamInfo.dwLength, SRCStreamInfo.dwRate,
					SRCStreamInfo.dwScale, cvar.fccHandler, cvar.lQ, bih2,vi.HasAudio()?&AUDStreamInfo:NULL,&wavex);
		} catch(const MyError& e) {
			free(bih2);
			Close();
			env->ThrowError("%snew OutputAVI() %s",myName,e.gets());
		} catch(...) {
			free(bih2);
			Close();
			env->ThrowError("%sError, new OutputAVI  failed on\n%s",myName,fullname);
		}
		DPRINTF("new RETURNED OK")
		free(bih2);
		ICCOMPRESSFRAMES iccf;
		memset(&iccf, 0, sizeof(ICCOMPRESSFRAMES));
		iccf.dwRate		= SRCStreamInfo.dwRate;
		iccf.dwScale	= SRCStreamInfo.dwScale;
		iccf.lQuality	= cvar.lQ;
		iccf.lDataRate	= cvar.lDataRate<<10;
		iccf.lKeyRate	= cvar.lKey;
		ICSendMessage(cvar.hic, ICM_COMPRESS_FRAMES_INFO, (DWORD)&iccf, sizeof(ICCOMPRESSFRAMES));
		if (!ICSeqCompressFrameStart(&cvar, (LPBITMAPINFO)&bih)) {
			Close();
			env->ThrowError("%sICSeqCompressFrameStart failed!",myName);
		}
	}
}

TWriteAvi::~TWriteAvi() {
	Close(true,true,true);
}


PVideoFrame __stdcall TWriteAvi::GetFrame(int n, IScriptEnvironment *env) {
	n = (n<0) ? 0 : (n >= vi.num_frames) ? vi.num_frames-1 : n;		// Range limit n to valid frames
	PVideoFrame frame;
	const bool IsRGB24 = vi.IsRGB24();
	if(out != NULL && lpBuffer != NULL && n >= NextFrame) {
		// Render NextFrame >-> n, ie render all missing non sequential frames up to requested n.
		for( ; NextFrame <= n ; ++NextFrame) {
			frame = child->GetFrame(NextFrame, env);
			const int pitch     = frame->GetPitch();
			const int rowsize   = frame->GetRowSize();			
			const int height    = frame->GetHeight();
			// Align RGB24 start of line mod 4
			const int out_pitch = (IsRGB24) ? (rowsize + 3) & (~0x03) : rowsize;
			env->BitBlt(lpBuffer, out_pitch, frame->GetReadPtr(), pitch, rowsize, height);	// RGB, YUY2, Planar-Y
			if(vi.IsPlanar() && P_hasChroma) {
				const int out_offsetUV = out_pitch*height;
				const int heightUV = frame->GetHeight( PLANAR_V);
				const int rowsizeUV = frame->GetRowSize(PLANAR_V);
				const int out_pitchUV = out_pitch/P_xSubS;
				env->BitBlt(lpBuffer + out_offsetUV, out_pitchUV, 
					frame->GetReadPtr(PLANAR_V), frame->GetPitch(PLANAR_V), rowsizeUV, heightUV);
				env->BitBlt(lpBuffer + out_offsetUV + heightUV*out_pitchUV, out_pitchUV, 
					frame->GetReadPtr(PLANAR_U), frame->GetPitch(PLANAR_U), rowsizeUV, heightUV);
			}
			if (NO_RECOMPRESS) {
				try {
					out->writeData(lpBuffer, vi.BMPSize(), true);
					KFCount++;
				} catch(const MyError& e) {
					Close();
					env->ThrowError("%s::GetFrame(%d) %s",myName,NextFrame,e.gets());
				} catch(...) {
					Close();
					env->ThrowError("%sError, GetFrame(%d) failed",myName,NextFrame);
				}
			} else {
				BOOL KF_Flag;
				LONG DSTDataSize;
				void *DSTData = ICSeqCompressFrame(&cvar, 0, lpBuffer, &KF_Flag, &DSTDataSize);
				if (!DSTData) {
					Close();
					env->ThrowError("%sICSeqCompressFrame failed!",myName);
				}
				if (KF_Flag) KFCount++;
				if (DSTDataSize == 1 && (*(byte*)DSTData == 0x7F)) BFCount++;
				else {
					try {
						out->writeData(DSTData, DSTDataSize, KF_Flag != 0);
					} catch(const MyError& e) {
						Close();
						env->ThrowError("%s::GetFrame(%d) %s",myName,NextFrame,e.gets());
					} catch(...) {
						Close();
						env->ThrowError("%sError, GetFrame(%d) failed",myName,NextFrame);
					}
				}
			}
		}
		// Here, frame=child->GetFrame(n, env) AND  NextFrame = n + 1
		if(NextFrame>=vi.num_frames) {
			DPRINTF("GetFrame Complete")
			Close(false,true,false);			// Last frame, Write no more frames, allow re-open of written file for read.
		}
	} else {
		frame   = child->GetFrame(n, env);
	}
	return frame;
}

void __stdcall TWriteAvi::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
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
			count = vi.num_audio_samples - NextSample;				// limit file audio to video length
		}
		try { 
			char *p = (char*)buf + vi.BytesFromAudioSamples(offset);
			out->Audio_writeData(p,(int)count);
		} catch(const MyError& e) {
			Close();
			env->ThrowError("%s::GetAudio %s",myName,e.gets());
		} catch(...) {
			Close();
			env->ThrowError("%sError Writing Audio",myName);
		}
		NextSample+=count;
		if(NextSample >= vi.num_audio_samples) {
			DPRINTF("GetAudio Complete")				
			Close(false,false,true);
		}
	} else {
		// NextSample < start, ie need write out all intervening audio before get start(count) samples
		while(NextSample <= start) {
			__int64 cnt = (NextSample==start) ? count : min(count,start-NextSample);
			child->GetAudio(buf, NextSample, cnt, env);
			if(NextSample + cnt > vi.num_audio_samples) {
				cnt = vi.num_audio_samples - NextSample;		// limit audio to video length
			}
			try { 
				out->Audio_writeData(buf,(int)cnt);
			} catch(const MyError& e) {
				Close();
				env->ThrowError("%s::GetAudio %s",myName,e.gets());
			} catch(...) {
				Close();
				env->ThrowError("%sError Writing Audio",myName);
			}
			NextSample+=cnt;
		}
		if(NextSample >= vi.num_audio_samples) {
			DPRINTF("GetAudio Complete")				
			Close(false,false,true);
		}
	}
}




void TWriteAvi::Close(bool force,bool vDone,bool aDone) {
	if (!closed) {
		DPRINTF("CLOSING %s %s %s",force?"T":"F",vDone?"T":"F",aDone?"T":"F")
		if(!vComplete && (vDone || force)) {
			if (!NO_RECOMPRESS) {
				if (out != NULL && lpBuffer != NULL) {
					if (BFCount > 0 && NextFrame >= 1) {  // b-frame delay handling
						BOOL KF_Flag;
						LONG DSTDataSize;
						void *DSTData;
						try {
							for (int i=0; i<BFCount; ++i) {
								DSTData = ICSeqCompressFrame(&cvar, 0, lpBuffer, &KF_Flag, &DSTDataSize);
								if (!DSTData) { i = BFCount+1; continue; }
								if (KF_Flag) KFCount++;
								if (DSTDataSize == 1 && (*(byte*)DSTData == 0x7F)) BFCount++;
								else out->writeData(DSTData, DSTDataSize, KF_Flag != 0);
							}
						} catch (...) {
							DPRINTF("BFrames, CatchAll caught")
						}
					}
				}
				ICSeqCompressFrameEnd(&cvar);
			}
			ICCompressorFree(&cvar);
			vComplete=true;
		}
		if(!aComplete && (aDone || force)) {
			aComplete=true;
		}
		closed = (vComplete && aComplete);
		if(closed) {
			DPRINTF("Closing File")
			if(out != NULL) {
				try{
					delete out;
				} catch (...) {
					DPRINTF("Delete[] CatchAll Caught")
				}
				out=NULL;
			}
			if (lpBuffer != NULL)			{free(lpBuffer);	lpBuffer=NULL;}
		}
	}
}

