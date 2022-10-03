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

#include <ctype.h>

#include <wtypes.h>
#include <mmsystem.h>
#include <stdio.h>

#include "misc.h"
//#include "cpuaccel.h"

#if 0

	long __declspec(naked) MulDivTrunc(long a, long b, long c) {
		__asm {
			mov eax,[esp+4]
			imul dword ptr [esp+8]
			idiv dword ptr [esp+12]
			ret
		}
	}

	unsigned __declspec(naked) __stdcall MulDivUnsigned(unsigned a, unsigned b, unsigned c) {
		__asm {
			mov		eax,[esp+4]
			mov		ecx,[esp+12]
			mul		dword ptr [esp+8]
			shr		ecx,1
			add		eax,ecx
			adc		edx,0
			div		dword ptr [esp+12]
			ret		12
		}
	}
# else
	long MulDivTrunc(long a, long b, long c) {
		return (long)(((signed __int64)a * b) / c);
	}

	unsigned __stdcall MulDivUnsigned(unsigned a, unsigned b, unsigned c) {
		return (unsigned)(((unsigned __int64)a * b + 0x80000000) / c);
}
# endif

int NearestLongValue(long v, const long *array, int array_size) {
	int i;

	for(i=1; i<array_size; i++)
		if (v*2 < array[i-1]+array[i])
			break;

	return i-1;
}

bool isEqualFOURCC(FOURCC fccA, FOURCC fccB) {
	int i;

	for(i=0; i<4; i++) {
		if (tolower((unsigned char)fccA) != tolower((unsigned char)fccB))
			return false;

		fccA>>=8;
		fccB>>=8;
	}

	return true;
}

bool isValidFOURCC(FOURCC fcc) {
	return isprint((unsigned char)(fcc>>24))
		&& isprint((unsigned char)(fcc>>16))
		&& isprint((unsigned char)(fcc>> 8))
		&& isprint((unsigned char)(fcc    ));
}

FOURCC toupperFOURCC(FOURCC fcc) {
	return(toupper((unsigned char)(fcc>>24)) << 24)
		| (toupper((unsigned char)(fcc>>16)) << 16)
		| (toupper((unsigned char)(fcc>> 8)) <<  8)
		| (toupper((unsigned char)(fcc    ))      );
}

#if 0

	bool IsMMXState() {
		char	buf[28];
		unsigned short tagword;

		__asm fnstenv buf

		tagword = *(unsigned short *)(buf + 8);

		return (tagword != 0xffff);
	}

	void ClearMMXState() {
		if (MMX_enabled)
			__asm emms
		else {
			__asm {
				ffree st(0)
				ffree st(1)
				ffree st(2)
				ffree st(3)
				ffree st(4)
				ffree st(5)
				ffree st(6)
				ffree st(7)
			}
		}
	}

#else

	bool IsMMXState() {
		return false;
	}

	void ClearMMXState() {
	}

#endif

char *strCify(const char *s) {
	static char buf[2048];
	char c,*t = buf;

	while(c=*s++) {
		if (!isprint((unsigned char)c))
			t += sprintf(t, "\\x%02x", (int)c & 0xff);
		else {
			if (c=='"' || c=='\\')
				*t++ = '\\';
			*t++ = c;
		}
	}
	*t=0;

	return buf;
}
