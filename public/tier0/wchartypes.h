//========= Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:	All of our code is completely Unicode.  Instead of char, you should
//			use wchar, uint8, or char8, as explained below.
//
// $NoKeywords: $
//=============================================================================//


#ifndef WCHARTYPES_H
#define WCHARTYPES_H
#ifdef _WIN32
#pragma once
#endif

#ifdef _INC_TCHAR
#error ("Must include tier0 type headers before tchar.h")
#endif

// Temporarily turn off Valve defines
#include "tier0/valve_off.h"

#if !defined(_WCHAR_T_DEFINED)  && !defined( __WCHAR_TYPE__ ) && !defined(GNUC)
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

// char8
// char8 is equivalent to char, and should be used when you really need a char
// (for example, when calling an external function that's declared to take
// chars).
typedef char char8;

// uint8
// uint8 is equivalent to byte (but is preferred over byte for clarity).  Use this
// whenever you mean a byte (for example, one byte of a network packet).
typedef unsigned char uint8;
typedef unsigned char BYTE;
typedef unsigned char byte;
// uchar
// uchar is equivalent to unsigned char, typically used for clarity
// when you mean an unsigned character or a raw byte.
typedef unsigned char uchar;

// ushort
// ushort is equivalent to unsigned short. Use this when you want a clear,
// concise alias for unsigned short integers (typically 16 bits).
typedef unsigned short ushort;

// uint
// uint is equivalent to unsigned int. Use this when you want a short and clear
// type name for unsigned integer values (typically 32 bits).
typedef unsigned int uint;

// ulong
// ulong is equivalent to unsigned long. Use this when you need an unsigned long
// integer type (typically 32 or 64 bits depending on platform).
typedef unsigned long ulong;

// uint8
// uint8 is equivalent to byte (but is preferred over byte for clarity).  Use this
// whenever you mean a byte (for example, one byte of a network packet).
typedef unsigned char uint8;
typedef unsigned char BYTE;
typedef unsigned char byte;

// wchar
// wchar is a single character of text (currently 16 bits, as all of our text is
// Unicode).  Use this whenever you mean a piece of text (for example, in a string).
typedef wchar_t wchar;
//typedef char wchar;

// __WFILE__
// This is a Unicode version of __FILE__
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)

#ifdef STEAM
#ifndef _UNICODE
#define FORCED_UNICODE
#endif
#define _UNICODE
#endif

#if defined( POSIX )
#define _tcsstr strstr
#define _tcsicmp stricmp
#define _tcscmp strcmp
#define _tcscpy strcpy
#define _tcsncpy strncpy
#define _tcsrchr strrchr
#define _tcslen strlen
#define _tfopen fopen
#define _stprintf sprintf 
#define _ftprintf fprintf
#define _vsntprintf _vsnprintf
#define _tprintf printf
#define _sntprintf _snprintf
#define _T(s) s
#else
#include <tchar.h>
#endif

#if defined(_UNICODE)
typedef wchar tchar;
#define tstring wstring
#define __TFILE__ __WFILE__
#define TCHAR_IS_WCHAR
#else
typedef char tchar;
#define tstring string
#define __TFILE__ __FILE__
#define TCHAR_IS_CHAR
#endif

#if defined( _MSC_VER ) || defined( WIN32 )
typedef wchar_t uchar16;
typedef unsigned int uchar32;
#else
typedef unsigned short uchar16;
typedef wchar_t uchar32;
#endif

#ifdef FORCED_UNICODE
#undef _UNICODE
#endif

#if defined( _MSC_VER ) || defined( WIN32 )
typedef wchar_t	uchar16;
typedef unsigned int uchar32;
#else
typedef unsigned short uchar16;
typedef wchar_t uchar32;
#endif

#ifdef GNUC
typedef unsigned short ucs2; // wchar_t is 4 bytes on sane os's, specially define a ucs2 type so we can read out localization files and the list saved as 2 byte wchar (or ucs16 Matt tells me)
#elif defined(_MSC_VER)
typedef wchar_t ucs2; // under windows wchar_t is ucs2
#endif

// Turn valve defines back on
#include "tier0/valve_on.h"


#endif // WCHARTYPES


