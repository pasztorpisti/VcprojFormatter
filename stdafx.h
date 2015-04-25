//
// Copyright (c) Istvan Pasztor
// This source has been published on www.codeproject.com under the CPOL license.
//
#pragma once

#include <stdio.h>
#include <tchar.h>

#include <windows.h>
#include <cassert>
#include <algorithm>
#include <vector>
#include <map>
#include <string>
typedef std::basic_string<wchar_t> wstring;

#define ILINE __forceinline

#include "smart.h"



inline void LogNoNewline(const wchar_t* fmtstr, va_list args)
{
	vwprintf(fmtstr, args);
}

inline void LogNoNewline(const wchar_t* fmtstr, ...)
{
	va_list args;
	va_start(args, fmtstr);
	LogNoNewline(fmtstr, args);
	va_end(args);
}

inline void Log(const wchar_t* fmtstr, va_list args)
{
	vwprintf(fmtstr, args);
	wprintf(L"\n");
}

inline void Log(const wchar_t* fmtstr, ...)
{
	va_list args;
	va_start(args, fmtstr);
	Log(fmtstr, args);
	va_end(args);
}

inline void Error(const wchar_t* fmtstr, va_list args)
{
	vfwprintf(stderr, fmtstr, args);
	fwprintf(stderr, L"\n");
}

inline void Error(const wchar_t* fmtstr, ...)
{
	va_list args;
	va_start(args, fmtstr);
	Error(fmtstr, args);
	va_end(args);
}


inline wstring LastErrorToString(DWORD last_error)
{
	wstring s;
	s.resize(0x100);
	DWORD msg_len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, last_error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &s[0], (DWORD)s.size(), NULL);
	while (msg_len>0 && (s[msg_len-1]=='\n' || s[msg_len-1]=='\r'))
		--msg_len;
	if (msg_len)
	{
		s.resize(msg_len);
		return s;
	}
	return L"Unknown LastError!";
}

inline void WinError(const wchar_t* fmtstr, ...)
{
	DWORD last_error = GetLastError();
	wstring last_error_str = LastErrorToString(last_error);

	va_list args;
	va_start(args, fmtstr);
	vwprintf(fmtstr, args);
	va_end(args);

	wprintf(L" [LastError: %d] %s\n", last_error, last_error_str.c_str());
}

inline wstring DirName(const wchar_t* path)
{
	wstring dir = path;
	wstring::size_type i = dir.rfind('/');
	wstring::size_type j = dir.rfind('\\');
	if (j != wstring::npos)
	{
		if (i != wstring::npos)
			dir.resize(max(i, j)+1);
		else
			dir.resize(j+1);
	}
	else if (i != wstring::npos)
	{
		dir.resize(i+1);
	}
	else
	{
		dir.clear();
	}
	return dir;
}
