/*
 * Copyright (C) 2001-2012 Ideaworks3D Ltd.
 * All Rights Reserved.
 *
 * This document is protected by copyright, and contains information
 * proprietary to Ideaworks Labs.
 * This file consists of source code released by Ideaworks Labs under
 * the terms of the accompanying End User License Agreement (EULA).
 * Please do not use this program/source code before you have read the
 * EULA and have agreed to be bound by its terms.
 */
#ifndef IW_UTF8_H
#define IW_UTF8_H

/**
 * @addtogroup iwutilgroup
 * @{
 */

/**
IwUTF8
 */

#if defined(_WIN32) && defined(INCLUDE_DEBUGGING)
#pragma message( "      including: " __FILE__ )
#endif

#include "s3eTypes.h"
/**
 * @defgroup iwutf8 IwUTF8
 * The IwUTF8 module contains functionality for converting between UTF-8 and UCS-2
 * text. These are intended as replacements for the Win32 functions
 * <i>WideCharToMultiByte()</i> and <i>MultiByteToWideChar()</i> with CP_UTF8.
 *
 * You can also refer to these functions as UCS2ToUTF8 and UTF8ToUCS2.
 *
 * @note For more information on IwUtil functionality, see the
 * @ref iwutilapidocumentation "IwUtil API Documentation".
 *
 * @{
 */

typedef unsigned short ucs2char;

#define UCS2ToUTF8 IwWideCharToUTF8
#define UTF8ToUCS2 IwUTF8ToWideChar

/**
 * Converts a Windows-style 16bit UCS-2 character string
 * into a UTF-8 encoded string. This assumes that the wide characters
 * are big endian.
 *
 * @return If the function fails, -1 is returned. Otherwise, the number of
 * bytes written into @a utf8Buffer - possibly including a null
 * terminator - is returned.
 *
 * If @a wideStringLength is >= 0, then that is taken to be the
 * number of characters to convert at @a wideString. If
 * @a wideStringLength is set to -1, then the length of the string is
 * calculated as being the length of @a wideString up to and
 * including the first null character.
 *
 * This function will convert null characters it comes across. This
 * means that if the string contains internal null characters, it
 * won't stop there, and it will not add a null terminator. (This
 * makes the function also useful for converting single characters
 * into byte sequences.)
 *
 * If @a utf8Buffer is NULL, then the return value is the length of
 * the string if it were to be converted.
 *
 * The contents of @a utf8Buffer may be altered even if the function
 * fails. (e.g., if the buffer turns out not to be big enough.)
 *
 * This is designed to look like the WideCharToMultiByte function in
 * the Win32 API, the only difference being that it won't destroy your
 * will to live when you discover that CP_UTF8 only exists on certain
 * versions of Windows.
 *
 * @sa UTF8ToWideChar()
 * @sa UTF8ToUCS4
 * @par Required Header Files
 * IwUTF8.h
 */
int IwWideCharToUTF8(   const ucs2char * wideString,
						int wideStringLength,
						char * utf8Buffer,
						unsigned int utf8BufferSize );

/**
 * Converts a UTF-8 encoded string into a Windows-style
 * 16bit UCS-2 character string. This will write big endian wide
 * characters.
 *
 * @return If the function fails, -1 is returned. Otherwise, the number of
 * wide characters written into @a wideBuffer - possibly including a
 * null terminator - is returned.
 *
 * If @a utf8StringSize is >= 0, then that is taken to be the size of
 * @a utf8String in bytes. If @a utf8StringSize is set to -1, then
 * the size of the string is calculated as being the number of bytes
 * at @a utf8String up to and including the first null character.
 *
 * This function will convert null characters it comes across. This
 * means that if the string contains internal null characters, it
 * won't stop there.  Also, it will not add a null terminator if one
 * is not included as the last of the @a utf8StringSize characters.
 * (This makes the function also useful for converting byte sequences
 * into single characters.)
 *
 * If @a wideBuffer is NULL, then the return value is the length of
 * the string if it were to be converted.
 *
 * The contents of @a wideBuffer may be altered even if the function
 * fails. (e.g., if the buffer turns out not to be big enough.)
 *
 * This is designed to look like the MultiByteToWideChar function in
 * the Win32 API. However, it's genuinely cross platform.
 *
 * @sa WideCharToUTF8
 * @sa UCS4ToUTF8
 * @par Required Header Files
 * IwUTF8.h
 */
int IwUTF8ToWideChar(   const char * utf8String,
						int utf8StringSize,
						ucs2char * wideBuffer,
						unsigned int wideBufferSize );


#endif /* !IW_UTF8_H */

/** @} */
/** @} */
