﻿/* zconf.h -- configuration of the zlib compression library
 * Copyright (C) 1995-2013 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#ifndef ZCONF_H
#define ZCONF_H


// 맥, 안드로이드, 마말레이드의 경우 64비트 버전 체크를 하여 'unistd.h' 을 포함한다.
#if defined(__linux__) || defined(__MACH__)
#if defined(__x86_64__) || defined(__LP64__)
#include <unistd.h>
#endif //__x86_64__ || __LP64__
#endif

// snprintf 사용 막고자
#define NO_snprintf

#define Z_PREFIX

/*
 * If you *really* need a unique prefix for all types and library functions,
 * compile with -DZ_PREFIX. The "standard" zlib should be compiled without it.
 * Even better than compiling with -DZ_PREFIX would be to use configure to set
 * this permanently in zconf.h using "./configure --zprefix".
 */
#ifdef Z_PREFIX     /* may be set to #if 1 by ./configure */ // 타 zlib 사용과 겹치지 않게 하기 위해 z_ prefix를 pnz_로 바꾸었음. 자세한 것은 zlib/configure sh를 참고할 것.
#  define Z_PREFIX_SET

/* all linked symbols */
#  define _dist_code            pnz__dist_code
#  define _length_code          pnz__length_code
#  define _tr_align             pnz__tr_align
#  define _tr_flush_bits        pnz__tr_flush_bits
#  define _tr_flush_block       pnz__tr_flush_block
#  define _tr_init              pnz__tr_init
#  define _tr_stored_block      pnz__tr_stored_block
#  define _tr_tally             pnz__tr_tally
#  define adler32               pnz_adler32
#  define adler32_combine       pnz_adler32_combine
#  define adler32_combine64     pnz_adler32_combine64
#  ifndef Z_SOLO
#    define compress              pnz_compress
#    define compress2             pnz_compress2
#    define compressBound         pnz_compressBound
#  endif
#  define crc32                 pnz_crc32
#  define crc32_combine         pnz_crc32_combine
#  define crc32_combine64       pnz_crc32_combine64
#  define deflate               pnz_deflate
#  define deflateBound          pnz_deflateBound
#  define deflateCopy           pnz_deflateCopy
#  define deflateEnd            pnz_deflateEnd
#  define deflateInit2_         pnz_deflateInit2_
#  define deflateInit_          pnz_deflateInit_
#  define deflateParams         pnz_deflateParams
#  define deflatePending        pnz_deflatePending
#  define deflatePrime          pnz_deflatePrime
#  define deflateReset          pnz_deflateReset
#  define deflateResetKeep      pnz_deflateResetKeep
#  define deflateSetDictionary  pnz_deflateSetDictionary
#  define deflateSetHeader      pnz_deflateSetHeader
#  define deflateTune           pnz_deflateTune
#  define deflate_copyright     pnz_deflate_copyright
#  define get_crc_table         pnz_get_crc_table
#  ifndef Z_SOLO
#    define gz_error              pnz_gz_error
#    define gz_intmax             pnz_gz_intmax
#    define gz_strwinerror        pnz_gz_strwinerror
#    define gzbuffer              pnz_gzbuffer
#    define gzclearerr            pnz_gzclearerr
#    define gzclose               pnz_gzclose
#    define gzclose_r             pnz_gzclose_r
#    define gzclose_w             pnz_gzclose_w
#    define gzdirect              pnz_gzdirect
#    define gzdopen               pnz_gzdopen
#    define gzeof                 pnz_gzeof
#    define gzerror               pnz_gzerror
#    define gzflush               pnz_gzflush
#    define gzgetc                pnz_gzgetc
#    define gzgetc_               pnz_gzgetc_
#    define gzgets                pnz_gzgets
#    define gzoffset              pnz_gzoffset
#    define gzoffset64            pnz_gzoffset64
#    define gzopen                pnz_gzopen
#    define gzopen64              pnz_gzopen64
#    ifdef _WIN32
#      define gzopen_w              pnz_gzopen_w
#    endif
#    define gzprintf              pnz_gzprintf
#    define gzvprintf             pnz_gzvprintf
#    define gzputc                pnz_gzputc
#    define gzputs                pnz_gzputs
#    define gzread                pnz_gzread
#    define gzrewind              pnz_gzrewind
#    define gzseek                pnz_gzseek
#    define gzseek64              pnz_gzseek64
#    define gzsetparams           pnz_gzsetparams
#    define gztell                pnz_gztell
#    define gztell64              pnz_gztell64
#    define gzungetc              pnz_gzungetc
#    define gzwrite               pnz_gzwrite
#  endif
#  define inflate               pnz_inflate
#  define inflateBack           pnz_inflateBack
#  define inflateBackEnd        pnz_inflateBackEnd
#  define inflateBackInit_      pnz_inflateBackInit_
#  define inflateCopy           pnz_inflateCopy
#  define inflateEnd            pnz_inflateEnd
#  define inflateGetHeader      pnz_inflateGetHeader
#  define inflateInit2_         pnz_inflateInit2_
#  define inflateInit_          pnz_inflateInit_
#  define inflateMark           pnz_inflateMark
#  define inflatePrime          pnz_inflatePrime
#  define inflateReset          pnz_inflateReset
#  define inflateReset2         pnz_inflateReset2
#  define inflateSetDictionary  pnz_inflateSetDictionary
#  define inflateGetDictionary  pnz_inflateGetDictionary
#  define inflateSync           pnz_inflateSync
#  define inflateSyncPoint      pnz_inflateSyncPoint
#  define inflateUndermine      pnz_inflateUndermine
#  define inflateResetKeep      pnz_inflateResetKeep
#  define inflate_copyright     pnz_inflate_copyright
#  define inflate_fast          pnz_inflate_fast
#  define inflate_table         pnz_inflate_table
#  ifndef Z_SOLO
#    define uncompress            pnz_uncompress
#  endif
#  define zError                pnz_zError
#  ifndef Z_SOLO
#    define zcalloc               pnz_zcalloc
#    define zcfree                pnz_zcfree
#  endif
#  define zlibCompileFlags      pnz_zlibCompileFlags
#  define zlibVersion           pnz_zlibVersion

/* all zlib typedefs in zlib.h and zconf.h */
#  define Byte                  pnz_Byte
#  define Bytef                 pnz_Bytef
#  define alloc_func            pnz_alloc_func
#  define charf                 pnz_charf
#  define free_func             pnz_free_func
#  ifndef Z_SOLO
#    define gzFile                pnz_gzFile
#  endif
#  define gz_header             pnz_gz_header
#  define gz_headerp            pnz_gz_headerp
#  define in_func               pnz_in_func
#  define intf                  pnz_intf
#  define out_func              pnz_out_func
#  define uInt                  pnz_uInt
#  define uIntf                 pnz_uIntf
#  define uLong                 pnz_uLong
#  define uLongf                pnz_uLongf
#  define voidp                 pnz_voidp
#  define voidpc                pnz_voidpc
#  define voidpf                pnz_voidpf

/* all zlib structs in zlib.h and zconf.h */
#  define gz_header_s           pnz_gz_header_s
#  define internal_state        pnz_internal_state

#  define z_errmsg				pnz_errmsg

#endif

#if defined(__MSDOS__) && !defined(MSDOS)
#  define MSDOS
#endif
#if (defined(OS_2) || defined(__OS2__)) && !defined(OS2)
#  define OS2
#endif
#if defined(_WINDOWS) && !defined(WINDOWS)
#  define WINDOWS
#endif
#if defined(_WIN32) || defined(_WIN32_WCE) || defined(__WIN32__)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#if (defined(MSDOS) || defined(OS2) || defined(WINDOWS)) && !defined(WIN32)
#  if !defined(__GNUC__) && !defined(__FLAT__) && !defined(__386__)
#    ifndef SYS16BIT
#      define SYS16BIT
#    endif
#  endif
#endif

/*
 * Compile with -DMAXSEG_64K if the alloc function cannot allocate more
 * than 64k bytes at a time (needed on systems with 16-bit int).
 */
#ifdef SYS16BIT
#  define MAXSEG_64K
#endif
#ifdef MSDOS
#  define UNALIGNED_OK
#endif

#ifdef __STDC_VERSION__
#  ifndef STDC
#    define STDC
#  endif
#  if __STDC_VERSION__ >= 199901L
#    ifndef STDC99
#      define STDC99
#    endif
#  endif
#endif
#if !defined(STDC) && (defined(__STDC__) || defined(__cplusplus))
#  define STDC
#endif
#if !defined(STDC) && (defined(__GNUC__) || defined(__BORLANDC__))
#  define STDC
#endif
#if !defined(STDC) && (defined(MSDOS) || defined(WINDOWS) || defined(WIN32))
#  define STDC
#endif
#if !defined(STDC) && (defined(OS2) || defined(__HOS_AIX__))
#  define STDC
#endif

#if defined(__OS400__) && !defined(STDC)    /* iSeries (formerly AS/400). */
#  define STDC
#endif

#ifndef STDC
#  ifndef const /* cannot use !defined(STDC) && !defined(const) on Mac */
#    define const       /* note: need a more gentle solution here */
#  endif
#endif

#if defined(ZLIB_CONST) && !defined(z_const)
#  define z_const const
#else
#  define z_const
#endif

/* Some Mac compilers merge all .h files incorrectly: */
#if defined(__MWERKS__)||defined(applec)||defined(THINK_C)||defined(__SC__)
#  define NO_DUMMY_DECL
#endif

/* Maximum value for memLevel in deflateInit2 */
#ifndef MAX_MEM_LEVEL
#  ifdef MAXSEG_64K
#    define MAX_MEM_LEVEL 8
#  else
#    define MAX_MEM_LEVEL 9
#  endif
#endif

/* Maximum value for windowBits in deflateInit2 and inflateInit2.
 * WARNING: reducing MAX_WBITS makes minigzip unable to extract .gz files
 * created by gzip. (Files created by minigzip can still be extracted by
 * gzip.)
 */
#ifndef MAX_WBITS
#  define MAX_WBITS   15 /* 32K LZ77 window */
#endif

/* The memory requirements for deflate are (in bytes):
            (1 << (windowBits+2)) +  (1 << (memLevel+9))
 that is: 128K for windowBits=15  +  128K for memLevel = 8  (default values)
 plus a few kilobytes for small objects. For example, if you want to reduce
 the default memory requirements from 256K to 128K, compile with
     make CFLAGS="-O -DMAX_WBITS=14 -DMAX_MEM_LEVEL=7"
 Of course this will generally degrade compression (there's no free lunch).

   The memory requirements for inflate are (in bytes) 1 << windowBits
 that is, 32K for windowBits=15 (default value) plus a few kilobytes
 for small objects.
*/

                        /* Type declarations */

#ifndef OF /* function prototypes */
#  ifdef STDC
#    define OF(args)  args
#  else
#    define OF(args)  ()
#  endif
#endif

#ifndef Z_ARG /* function prototypes for stdarg */
#  if defined(STDC) || defined(Z_HAVE_STDARG_H)
#    define Z_ARG(args)  args
#  else
#    define Z_ARG(args)  ()
#  endif
#endif

/* The following definitions for FAR are needed only for MSDOS mixed
 * model programming (small or medium model with some far allocations).
 * This was tested only with MSC; for other MSDOS compilers you may have
 * to define NO_MEMCPY in zutil.h.  If you don't need the mixed model,
 * just define FAR to be empty.
 */
#ifdef SYS16BIT
#  if defined(M_I86SM) || defined(M_I86MM)
     /* MSC small or medium model */
#    define SMALL_MEDIUM
#    ifdef _MSC_VER
#      define FAR _far
#    else
#      define FAR far
#    endif
#  endif
#  if (defined(__SMALL__) || defined(__MEDIUM__))
     /* Turbo C small or medium model */
#    define SMALL_MEDIUM
#    ifdef __BORLANDC__
#      define FAR _far
#    else
#      define FAR far
#    endif
#  endif
#endif

#if defined(WINDOWS) || defined(WIN32)
   /* If building or using zlib as a DLL, define ZLIB_DLL.
    * This is not mandatory, but it offers a little performance increase.
    */
#  ifdef ZLIB_DLL
#    if defined(WIN32) && (!defined(__BORLANDC__) || (__BORLANDC__ >= 0x500))
#      ifdef ZLIB_INTERNAL
#        define ZEXTERN extern __declspec(dllexport)
#      else
#        define ZEXTERN extern __declspec(dllimport)
#      endif
#    endif
#  endif  /* ZLIB_DLL */
   /* If building or using zlib with the WINAPI/WINAPIV calling convention,
    * define ZLIB_WINAPI.
    * Caution: the standard ZLIB1.DLL is NOT compiled using ZLIB_WINAPI.
    */
#  ifdef ZLIB_WINAPI
#    ifdef FAR
#      undef FAR
#    endif
#    include <windows.h>
     /* No need for _export, use ZLIB.DEF instead. */
     /* For complete Windows compatibility, use WINAPI, not __stdcall. */
#    define ZEXPORT WINAPI
#    ifdef WIN32
#      define ZEXPORTVA WINAPIV
#    else
#      define ZEXPORTVA FAR CDECL
#    endif
#  endif
#endif

#if defined (__BEOS__)
#  ifdef ZLIB_DLL
#    ifdef ZLIB_INTERNAL
#      define ZEXPORT   __declspec(dllexport)
#      define ZEXPORTVA __declspec(dllexport)
#    else
#      define ZEXPORT   __declspec(dllimport)
#      define ZEXPORTVA __declspec(dllimport)
#    endif
#  endif
#endif

#ifndef ZEXTERN
#  define ZEXTERN extern
#endif
#ifndef ZEXPORT
#  define ZEXPORT
#endif
#ifndef ZEXPORTVA
#  define ZEXPORTVA
#endif

#ifndef FAR
#  define FAR
#endif

#if !defined(__MACTYPES__)
typedef unsigned char  Byte;  /* 8 bits */
#endif
typedef unsigned int   uInt;  /* 16 bits or more */
typedef unsigned long  uLong; /* 32 bits or more */

#ifdef SMALL_MEDIUM
   /* Borland C/C++ and some old MSC versions ignore FAR inside typedef */
#  define Bytef Byte FAR
#else
   typedef Byte  FAR Bytef;
#endif
typedef char  FAR charf;
typedef int   FAR intf;
typedef uInt  FAR uIntf;
typedef uLong FAR uLongf;

#ifdef STDC
   typedef void const *voidpc;
   typedef void FAR   *voidpf;
   typedef void       *voidp;
#else
   typedef Byte const *voidpc;
   typedef Byte FAR   *voidpf;
   typedef Byte       *voidp;
#endif

#if !defined(Z_U4) && !defined(Z_SOLO) && defined(STDC)
#  include <limits.h>
#  if (UINT_MAX == 0xffffffffUL)
#    define Z_U4 unsigned
#  elif (ULONG_MAX == 0xffffffffUL)
#    define Z_U4 unsigned long
#  elif (USHRT_MAX == 0xffffffffUL)
#    define Z_U4 unsigned short
#  endif
#endif

#ifdef Z_U4
   typedef Z_U4 z_crc_t;
#else
   typedef unsigned long z_crc_t;
#endif

#ifdef HAVE_UNISTD_H    /* may be set to #if 1 by ./configure */
#  define Z_HAVE_UNISTD_H
#endif

#ifdef HAVE_STDARG_H    /* may be set to #if 1 by ./configure */
#  define Z_HAVE_STDARG_H
#endif

#ifdef STDC
#  ifndef Z_SOLO
#    include <sys/types.h>      /* for off_t */
#  endif
#endif

#if defined(STDC) || defined(Z_HAVE_STDARG_H)
#  ifndef Z_SOLO
#    include <stdarg.h>         /* for va_list */
#  endif
#endif

#ifdef _WIN32
#  ifndef Z_SOLO
#    include <stddef.h>         /* for wchar_t */
#  endif
#endif

/* a little trick to accommodate both "#define _LARGEFILE64_SOURCE" and
 * "#define _LARGEFILE64_SOURCE 1" as requesting 64-bit operations, (even
 * though the former does not conform to the LFS document), but considering
 * both "#undef _LARGEFILE64_SOURCE" and "#define _LARGEFILE64_SOURCE 0" as
 * equivalently requesting no 64-bit operations
 */
#if defined(_LARGEFILE64_SOURCE) && -_LARGEFILE64_SOURCE - -1 == 1
#  undef _LARGEFILE64_SOURCE
#endif

#if defined(__WATCOMC__) && !defined(Z_HAVE_UNISTD_H)
#  define Z_HAVE_UNISTD_H
#endif
#ifndef Z_SOLO
#  if defined(Z_HAVE_UNISTD_H) || defined(_LARGEFILE64_SOURCE)
#    include <unistd.h>         /* for SEEK_*, off_t, and _LFS64_LARGEFILE */
#    ifdef VMS
#      include <unixio.h>       /* for off_t */
#    endif
#    ifndef z_off_t
#      define z_off_t off_t
#    endif
#  endif
#endif

#if defined(_LFS64_LARGEFILE) && _LFS64_LARGEFILE-0
#  define Z_LFS64
#endif

#if defined(_LARGEFILE64_SOURCE) && defined(Z_LFS64)
#  define Z_LARGE64
#endif

#if defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS-0 == 64 && defined(Z_LFS64)
#  define Z_WANT64
#endif

#if !defined(SEEK_SET) && !defined(Z_SOLO)
#  define SEEK_SET        0       /* Seek from beginning of file.  */
#  define SEEK_CUR        1       /* Seek from current position.  */
#  define SEEK_END        2       /* Set file pointer to EOF plus "offset" */
#endif

#ifndef z_off_t
#  define z_off_t long
#endif

#if !defined(_WIN32) && defined(Z_LARGE64)
#  define z_off64_t off64_t
#else
#  if defined(_WIN32) && !defined(__GNUC__) && !defined(Z_SOLO)
#    define z_off64_t __int64
#  else
#    define z_off64_t z_off_t
#  endif
#endif

/* MVS linker does not support external names larger than 8 bytes */
#if defined(__MVS__)
  #pragma map(deflateInit_,"DEIN")
  #pragma map(deflateInit2_,"DEIN2")
  #pragma map(deflateEnd,"DEEND")
  #pragma map(deflateBound,"DEBND")
  #pragma map(inflateInit_,"ININ")
  #pragma map(inflateInit2_,"ININ2")
  #pragma map(inflateEnd,"INEND")
  #pragma map(inflateSync,"INSY")
  #pragma map(inflateSetDictionary,"INSEDI")
  #pragma map(compressBound,"CMBND")
  #pragma map(inflate_table,"INTABL")
  #pragma map(inflate_fast,"INFA")
  #pragma map(inflate_copyright,"INCOPY")
#endif

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif


#endif /* ZCONF_H */
