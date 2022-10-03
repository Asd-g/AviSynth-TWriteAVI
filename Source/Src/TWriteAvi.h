/*
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

#ifndef f_TWRITEAVI_H
#define f_TWRITEAVI_H


#include "Compiler.h"

#include <windows.h>
#include <vfw.h>
#include <stdio.h>
#include <shlwapi.h>
#include <KS.h>
#include <Ksmedia.h>
#include "OutputAvi.h"
#include "virtualdub\Error.h"

#ifdef AVISYNTH_PLUGIN_25
	#include "avisynth25.h"
#else
	#include "avisynth.h"
#endif

enum FourCCs {
	FCC_NULL = 0x00000000,
	FCC_DIB  = 0x20424944
};

#define NO_RECOMPRESS (cvar.fccHandler == FCC_NULL)
extern HINSTANCE g_hInst; // vdub stuff

class TWriteAvi : public GenericVideoFilter {
private:
	//	args except for _fourcc
	const char *fname;
	const bool overwrite, showAll;
	int	dwcm;
	//
	const char *myName;
	BITMAPINFOHEADER bih;
	COMPVARS cvar;
	char fullname[MAX_PATH];
	AVISTREAMINFO SRCStreamInfo;
	int KFCount, BFCount;
	bool closed;
	//
	OutputFormat* out;
	unsigned char *lpBuffer;
// ssS
	int NextFrame;
	__int64 NextSample;
	bool vComplete;
	bool aComplete;
	AVISTREAMINFO AUDStreamInfo;
	WAVEFORMATEXTENSIBLE wavex;
	int P_xSubS,P_ySubS;					// Planar only
	int P_hasChroma;
public:
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment *env);
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
	TWriteAvi(PClip _child, const char* _fname, bool _overwrite, bool _showAll, const char* _fourcc,int _dwcm, IScriptEnvironment *env);
	~TWriteAvi();
	void Close(bool force=true,bool vDone=false,bool aDone=false);
};


#endif