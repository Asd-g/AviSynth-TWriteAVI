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

#ifndef f_TWRITEWAV_H
#define f_TWRITEWAV_H


#include "Compiler.h"

#include <windows.h>
#include <vfw.h>
#include <stdio.h>
#include <shlwapi.h>
#include <KS.h>
#include <Ksmedia.h>
#include "OutputWav.h"
#include "virtualdub\Error.h"

#ifdef AVISYNTH_PLUGIN_25
	#include "avisynth25.h"
#else
	#include "avisynth.h"
#endif

extern HINSTANCE g_hInst; // vdub stuff

class TWriteWav : public GenericVideoFilter {
private:
	// args
	const char *fname;
	const bool overwrite;
	int	dwcm;
	//
	const char *myName;
	char fullname[MAX_PATH];
	__int64 NextSample;
	AVISTREAMINFO AUDStreamInfo;
	WAVEFORMATEXTENSIBLE wavex;
	bool closed;
	//
	OutputFormatWAV* out;
	//
	void Close();
public:
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
	TWriteWav(PClip _child, const char* _fname, bool _overwrite, int _dwcm, IScriptEnvironment *env);
	~TWriteWav();
};


#endif
