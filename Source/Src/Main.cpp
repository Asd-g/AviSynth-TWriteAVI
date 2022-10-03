/*
**   TWriteAvi for AviSynth 2.5.x
**   
**   Copyright (C) 2005 Kevin Stone
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   This program is distributed in the hope that it will be useful,
**   but WITHOUT ANY WARRANTY; without even the implied warranty of
**   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**   GNU General Public License for more details.
**
**   You should have received a copy of the GNU General Public License
**   along with this program; if not, write to the Free Software
**   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*

    Link to libs: winmm.lib vfw32.lib shlwapi.lib

	Original source from here: http://forum.doom9.org/showthread.php?p=1073371#post1073371
	VirtualDub.org here :- http://www.virtualdub.org/

	07 Nov  2015. v2.0 Added v2.6 colour spaces (excluding YV411) + audio, + TwriteWAV + ForceProcess funcs. ssS.
	20 Dec  2015. v2.01, Added Catch all Error Message.
	06 Jan  2016. v2.02, Added Trim(0,0) to TWriteAVI, trim/pad audio to match video. Various other fixes.
	03 June 2016. v2.03, Bugfix in GetFrame 'n' frame range limiting.
	        Changed behaviour for out of order frame request.
	26 Sept 2016. v2.04, Added Debug arg to ForceProcessAVI. 
	       ForceProcessAVI, added Sleep(0) every 256 frames.
	       ForceProcessWAV, added Sleep(0) every 1 seconds worth of audio samples.
		   Fixed Skewed YUV chroma where mod 2 width.
	29 July 2018 2.05, ForceProcessAVI, changed Sleep(0) every 128 frames. Added Version resource.


	requires linking :- winmm.lib vfw32.lib shlwapi.lib (Add to Project/Linker/Input/Additional Dependencies)
*/

/*
	Compiling for both Avisynth v2.58 & v2.6 ProjectName under VS6, where ProjectName is the base name of the plugin.
	Create an additional project ProjectName25 and copy the
	project files into ProjectName folder. Add headers and source files to v2.5 project.
	You should have "avisynth26.h" in the ProjectName Header Files and
	"avisynth25.h" in the ProjectName25 Header Files.
	For the v2.58 project, add preprocessor definition to :-
		Menu/Project/Settings/All Configuration/C/C++, AVISYNTH_PLUGIN_25

	*** NOTE ***
	DPRINTF() messages switched via #define BUG in Complier.h
	DPRINTF messages are configured for VS2008, need args enclosed in additional parenthesis for pre C99 compiler
	  eg VS 6, so eg DPRINTF("Hello World=%d",42) would need to be changed to DPRINTF(("Hello World=%d",42))
*/

#include "Compiler.h"

#include <windows.h>
#include <time.h>
#include <vfw.h>
#include <stdio.h>
#include <shlwapi.h>
#include <KS.h>
#include <Ksmedia.h>
#include "OutputAvi.h"
#include "OutputWav.h"
#include "TWriteAvi.h"
#include "TWriteWav.h"

#ifdef AVISYNTH_PLUGIN_25
	#include "avisynth25.h"
#else
	#include "avisynth.h"
#endif

HINSTANCE g_hInst; // vdub stuff

int dprintf(char* fmt, ...) {
	char printString[2048]="TWriteAVI: ";		// MUST NUL TERM THIS [at some point before vsprintf(p, fmt, argp)]
	char *p=printString;
	while(*p++);		
	--p;                                        // @ nul term
	va_list argp;
	va_start(argp, fmt);
	vsprintf(p, fmt, argp);
	va_end(argp);
	while(*p++);
	--p;										// @ nul term
	if(printString == p || p[-1] != '\n') {
		p[0]='\n';								// append n/l if not there already
		p[1]='\0';
	}
	OutputDebugString(printString);
	return int(p-printString);						// strlen printString	
}

AVSValue __cdecl ForceProcessAVI(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="ForceProcessAVI: ";
	if(!args[0].IsClip())						env->ThrowError("%sMust have a clip",myName);
	PClip child  = args[0].AsClip();
	const bool debug=args[1].AsBool(true);
    const VideoInfo &vi = child->GetVideoInfo();
	const int frames=vi.num_frames;
	if(!vi.HasVideo() || frames <= 0)			env->ThrowError("%sClip must have video",myName);
	int bfsz = 0;
	BYTE *bf = NULL;
	const bool HasAudio = vi.HasAudio();
	if(HasAudio) {
		bfsz = int(vi.BytesFromAudioSamples(vi.AudioSamplesFromFrames(1))+(8*4*2));
		bf = new BYTE[bfsz];
		if(bf==NULL)		env->ThrowError("%sCannot allocate audio buffer",myName);
	}
    double start_T =  double(clock()) / double(CLOCKS_PER_SEC);
    double sT = start_T;
    int parts = min(20,frames);
    int part=1;
    int sf=0;                       // this time start frameno
    int stopf = frames * part / parts;
    if(debug)	dprintf("%sCommencing Forced process",myName);
	for(int n=0;n<frames;++n) {
		if(HasAudio) {
			__int64 s = vi.AudioSamplesFromFrames(n);
			__int64 e = vi.AudioSamplesFromFrames(n+1);
			child->GetAudio((void*)bf, s, e-s, env);	// Force 1 video frame worth of audio
		}
	    PVideoFrame	src=child->GetFrame(n,env);		// MUST assign to PVideoFrame (else maybe not ALL prior filters forced).
        if(debug && n+1 >= stopf) {
            double now = double(clock()) / double(CLOCKS_PER_SEC);
            double Tim   = max(now - sT,0.0001); // time taken for this part
            double FPS = (n+1-sf)/Tim;
            double SPF = 1.0/FPS;
            dprintf("%s%6d] %6.2f%%  nFrms=%d  T=%.3fsec  :  %.2fFpS  %.6fSpF",
				myName,n,(n+1)*100.0/frames,n-sf+1,Tim,FPS,SPF);
            sf=n+1;                    // next time start frame
            sT=now;                    // next time start time
            ++part;                    // next time part number
            stopf = frames * part / parts; // next time stop frame
        }
		if((n & 0x7F)==0) {
			Sleep(0);
		}
	}
	if(debug) {
		double Tim = max(double(clock()) / double(CLOCKS_PER_SEC) - start_T,0.0001);
		double FPS = frames/Tim;
		double SPF = 1.0/FPS;
		dprintf("%sTime=%.3fsecs (%.3fmins) : Avg %.3fFpS %.6fSpF",myName,Tim,Tim/60.0,FPS,SPF);
	}
	if(bf!=NULL)	{delete [] bf;}
    return 0;
}

AVSValue __cdecl ForceProcessWAV(AVSValue args, void* user_data, IScriptEnvironment* env) {
	char *myName="ForceProcessWAV: ";
	if(!args[0].IsClip())							env->ThrowError("%sMust have a clip",myName);
	PClip child  = args[0].AsClip();
    const VideoInfo &vi = child->GetVideoInfo();
	if(!vi.HasAudio())								env->ThrowError("%sMust have audio",myName);
	int SamplesPerSecond = vi.SamplesPerSecond();
	int bfsz = SamplesPerSecond * vi.BytesPerAudioSample();
	BYTE *bf = new BYTE[bfsz];
	if(bf==NULL)									env->ThrowError("%sCannot allocate audio buffer",myName);
	__int64 num_audio_samples = vi.num_audio_samples;
	__int64 s,e=0;
	do {
		s = e;
		e = e + SamplesPerSecond;
		if(e > num_audio_samples)
			e = num_audio_samples;
		child->GetAudio((void*)bf, s, e-s, env);	// Force 1 second worth of audio
		Sleep(0);
	}while(e < num_audio_samples);
	if(bf!=NULL)	{delete [] bf;}
    return 0;
}

AVSValue __cdecl Create_TWriteAvi(AVSValue args, void* user_data, IScriptEnvironment* env) {
	PClip child=args[0].AsClip();
	const VideoInfo &vi=child->GetVideoInfo();
	if(vi.HasAudio()) {
		AVSValue avsv[3]={child,0,0};								// Trim(0,0), trim/pad audio to video length
		child = env->Invoke("Trim", AVSValue(avsv, 3)).AsClip();
	}
    return new TWriteAvi(child, args[1].AsString(), args[2].AsBool(false),
			args[3].AsBool(false), args[4].AsString(""),args[5].AsInt(-1), env);
}

AVSValue __cdecl Create_TWriteWav(AVSValue args, void* user_data, IScriptEnvironment* env) {
    return new TWriteWav(args[0].AsClip(), args[1].AsString(), args[2].AsBool(false),args[3].AsInt(-1), env);
}

#ifdef AVISYNTH_PLUGIN_25
	extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env) {
#else
	// New 2.6 requirement!!! //
	// Declare and initialise server pointers static storage.
	const AVS_Linkage *AVS_linkage = 0;

	// New 2.6 requirement!!! //
	// DLL entry point called from LoadPlugin() to setup a user plugin.
	extern "C" __declspec(dllexport) const char* __stdcall
			AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors) {

	// New 2.6 requirment!!! //
	// Save the server pointers.
	AVS_linkage = vectors;
#endif
    env->AddFunction("TWriteAvi", "cs[overwrite]b[showAll]b[fourcc]s[dwChannelMask]i", Create_TWriteAvi, 0);
    env->AddFunction("TWriteWav", "cs[overwrite]b[dwChannelMask]i", Create_TWriteWav, 0);
    env->AddFunction("ForceProcessAVI", "c[Debug]b", ForceProcessAVI, 0);
    env->AddFunction("ForceProcessWAV", "c", ForceProcessWAV, 0);
	return "TWriteAVI by tritical, squid_80 and mikeytown2";
}
