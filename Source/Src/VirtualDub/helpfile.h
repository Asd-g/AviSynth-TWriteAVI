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

#include "..\Compiler.h"

#ifndef f_HELPFILE_H
#define f_HELPFILE_H

#define IDH_CAPTURE_PREFS_DEFAULTDRIVER		50000
#define IDH_CAPTURE_PREFS_PERDRIVER			50001
#define IDH_CAPTURE_PREFS_INITIALDISPLAY	50002
#define IDH_CAPTURE_PREFS_SLOW				50003

#define IDH_CAPTURE_SETTINGS_FRAMERATE		50100
#define IDH_CAPTURE_SETTINGS_LIMITLENGTH	50101
#define IDH_CAPTURE_SETTINGS_DROPLIMIT		50102
#define IDH_CAPTURE_SETTINGS_MAXINDEX		50103
#define IDH_CAPTURE_SETTINGS_VIDBUFLIMIT	50104
#define IDH_CAPTURE_SETTINGS_AUDIOBUFFERS	50105
#define IDH_CAPTURE_SETTINGS_LOCKDURATION	50106

#define IDH_CAPTURE_HISTOGRAM				50200
#define IDH_CAPTURE_VUMETER					50201

#define	IDH_DLG_VFR_FRAMERATE				50300
#define	IDH_DLG_VFR_DECIMATION				50301

#define IDH_DLG_VDEPTH_INPUT				50400
#define	IDH_DLG_VDEPTH_OUTPUT				50401

#define IDH_DLG_VCLIP_RANGES				50500
#define	IDH_DLG_VCLIP_OFFSETAUDIO			50501
#define IDH_DLG_VCLIP_CLIPAUDIO				50502

#define IDH_VIDCOMP_DELTAFRAMES				50600
#define IDH_VIDCOMP_FOURCCCODE				50601
#define IDH_VIDCOMP_DRIVERNAME				50602
#define IDH_VIDCOMP_FORMATRESTRICTIONS		50603
#define IDH_VIDCOMP_QUALITYSETTING			50604
#define IDH_VIDCOMP_TARGETDATARATE			50605
#define IDH_VIDCOMP_KEYFRAMEINTERVAL		50606

#define IDH_AUDCONV_SAMPLINGRATE			50700
#define IDH_AUDCONV_INTEGRALCONVERSION		50701
#define IDH_AUDCONV_PRECISION				50702
#define IDH_AUDCONV_CHANNELS				50703
#define IDH_AUDCONV_BANDWIDTHREQUIRED		50704
#define IDH_AUDCONV_HIGHQUALITY				50705

#define IDH_SETUP_BENCHMARK					55000
#define	IDH_SETUP_INSTALL					55001
#define	IDH_SETUP_UNINSTALL					55002
#define	IDH_SETUP_REMOVE					55003
#define	IDH_BENCHMARK_FRAME_SIZE			55100
#define	IDH_BENCHMARK_FRAME_COUNT			55101
#define	IDH_BENCHMARK_FRAME_BUFFERS			55102
#define	IDH_BENCHMARK_FRAME_RATE			55103
#define	IDH_BENCHMARK_DISK_BUFFER			55104
#define	IDH_BENCHMARK_DATA_RATE				55105
#define	IDH_BENCHMARK_BUFFERING				55106

#define IDH_PREFS_MAIN_OUTPUTCOLORDEPTH		56000
#define IDH_PREFS_MAIN_PROCESSPRIORITY		56001
#define IDH_PREFS_MAIN_ADDEXTENSION			56002
#define IDH_PREFS_DISPLAY_16BITDITHER		56100
#define IDH_PREFS_SCENE_INTERFRAME			56200
#define IDH_PREFS_SCENE_INTRAFRAME			56201
#define IDH_PREFS_CPU_OPTIMIZATIONS			56300
#define	IDH_PREFS_AVI_RESTRICT_1GB			56400
#define	IDH_PREFS_AVI_AUTOCORRECT_L3		56401

#define IDH_COACH							59500

#define IDH_CAPWARN_PINNACLE				59600
#define IDH_CAPWARN_ZORAN					59601
#define IDH_CAPWARN_BROOKTREE				59602
#define IDH_WARN_MPEG4						59603

#define IDH_CRASH							60000

#define IDH_TROUBLE_CAP						61000

#endif
