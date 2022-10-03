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

#ifndef f_VIRTUALDUB_H
#define f_VIRTUALDUB_H

#include "..\Compiler.h"

#include <crtdbg.h>

#ifdef _DEBUG

	#define new_track new(_NORMAL_BLOCK, __FILE__, __LINE__)

	#define CHECK_FPU_STACK checkfpustack(__FILE__, __LINE__);
	extern void checkfpustack(const char *, const int) throw();


#else

	#define new_track new
	#define CHECK_FPU_STACK

#endif

#if 0
	#define DEFINE_SP(var) void *var=0
	#define CHECK_STACK(var) stackcheck(var)
#else
	#define DEFINE_SP(var)
	#define CHECK_STACK(var)
#endif

	extern void stackcheck(void *&);

	void *allocmem(size_t);
	void freemem(void *);
	void *reallocmem(void *, size_t);
	void *callocmem(size_t, size_t);

#define ENABLE_DIRECTDRAW_SUPPORT

#endif
