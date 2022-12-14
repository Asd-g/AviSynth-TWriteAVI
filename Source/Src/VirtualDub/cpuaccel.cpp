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

#include <wtypes.h>
#include <winnt.h>
#include "cpuaccel.h"

static long g_lCPUExtensionsEnabled;
static long g_lCPUExtensionsAvailable;

extern "C" {
	bool FPU_enabled, MMX_enabled, ISSE_enabled;
};

// This is ridiculous.

static long CPUCheckForSSESupport() {
	__try {
//		__asm andps xmm0,xmm0

		__asm _emit 0x0f
		__asm _emit 0x54
		__asm _emit 0xc0

	} __except(EXCEPTION_EXECUTE_HANDLER) {
		if (_exception_code() == STATUS_ILLEGAL_INSTRUCTION)
			g_lCPUExtensionsAvailable &= ~(CPUF_SUPPORTS_SSE|CPUF_SUPPORTS_SSE2);
	}

	return g_lCPUExtensionsAvailable;
}

long __declspec(naked) CPUCheckForExtensions() {
	__asm {
		push	ebp
		push	edi
		push	esi
		push	ebx

		xor		ebp,ebp			;cpu flags - if we don't have CPUID, we probably
								;won't want to try FPU optimizations.

		;check for CPUID.

		pushfd					;flags -> EAX
		pop		eax
		or		eax,00200000h	;set the ID bit
		push	eax				;EAX -> flags
		popfd
		pushfd					;flags -> EAX
		pop		eax
		and		eax,00200000h	;ID bit set?
		jz		done			;nope...

		;CPUID exists, check for features register.

		mov		ebp,00000003h
		xor		eax,eax
		cpuid
		or		eax,eax
		jz		done			;no features register?!?

		;features register exists, look for MMX, SSE, SSE2.

		mov		eax,1
		cpuid
		mov		ebx,edx
		and		ebx,00800000h	;MMX is bit 23
		shr		ebx,21
		or		ebp,ebx			;set bit 2 if MMX exists

		mov		ebx,edx
		and		edx,02000000h	;SSE is bit 25
		shr		edx,25
		neg		edx
		and		edx,00000018h	;set bits 3 and 4 if SSE exists
		or		ebp,edx

		and		ebx,04000000h	;SSE2 is bit 26
		shr		ebx,21
		and		ebx,00000020h	;set bit 5
		or		ebp,ebx

		;check for vendor feature register (K6/Athlon).

		mov		eax,80000000h
		cpuid
		mov		ecx,80000001h
		cmp		eax,ecx
		jb		done

		;vendor feature register exists, look for 3DNow! and Athlon extensions

		mov		eax,ecx
		cpuid

		mov		eax,edx
		and		edx,80000000h	;3DNow! is bit 31
		shr		edx,25
		or		ebp,edx			;set bit 6

		mov		edx,eax
		and		eax,40000000h	;3DNow!2 is bit 30
		shr		eax,23
		or		ebp,eax			;set bit 7

		and		edx,00400000h	;AMD MMX extensions (integer SSE) is bit 22
		shr		edx,19
		or		ebp,edx

done:
		mov		eax,ebp
		mov		g_lCPUExtensionsAvailable, ebp

		;Full SSE and SSE-2 require OS support for the xmm* registers.

		test	eax,00000030h
		jz		nocheck
		call	CPUCheckForSSESupport
nocheck:
		pop		ebx
		pop		esi
		pop		edi
		pop		ebp
		ret
	}
}

long CPUEnableExtensions(long lEnableFlags) {
	g_lCPUExtensionsEnabled = lEnableFlags;

	MMX_enabled = !!(g_lCPUExtensionsEnabled & CPUF_SUPPORTS_MMX);
	FPU_enabled = !!(g_lCPUExtensionsEnabled & CPUF_SUPPORTS_FPU);
	ISSE_enabled = !!(g_lCPUExtensionsEnabled & CPUF_SUPPORTS_INTEGER_SSE);

	return g_lCPUExtensionsEnabled;
}

long CPUGetAvailableExtensions() {
	return g_lCPUExtensionsAvailable;
}

long CPUGetEnabledExtensions() {
	return g_lCPUExtensionsEnabled;
}
