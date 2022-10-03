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

#ifndef f_ERROR_H
#define f_ERROR_H

#include "..\Compiler.h"

#include <windows.h>
#include <vfw.h>

class MyError {
private:
	const MyError& operator=(const MyError&);		// protect against accidents

protected:
	char *buf;

public:
	MyError() throw();
	MyError(const MyError& err) throw();
	MyError(const char *f, ...) throw();
	~MyError() throw();
	void assign(const MyError& e) throw();
	void setf(const char *f, ...) throw();
	void vsetf(const char *f, va_list val) throw();
//	void post(HWND hWndParent, const char *title) const throw();		// ssS
	char *gets() const throw() {
		return buf;
	}
	void discard() throw();
	void TransferFrom(MyError& err) throw();
};

class MyICError : public MyError {
public:
	MyICError(const char *s, DWORD icErr) throw();
	MyICError(DWORD icErr, const char *format, ...) throw();
};

class MyMMIOError : public MyError {
public:
	MyMMIOError(const char *s, DWORD icErr) throw();
};

class MyAVIError : public MyError {
public:
	MyAVIError(const char *s, DWORD aviErr) throw();
};

class MyMemoryError : public MyError {
public:
	MyMemoryError() throw();
};

class MyWin32Error : public MyError {
public:
	MyWin32Error(const char *format, DWORD err, ...) throw();
};

class MyCrashError : public MyError {
public:
	MyCrashError(const char *format, DWORD dwExceptionCode) throw();
};

class MyUserAbortError : public MyError {
public:
	MyUserAbortError() throw();
};

class MyInternalError : public MyError {
public:
	MyInternalError(const char *format, ...) throw();
};

#endif
