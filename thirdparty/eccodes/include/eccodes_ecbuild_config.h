/*
 * (C) Copyright 2011- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#ifndef eccodes_ecbuild_config_h
#define eccodes_ecbuild_config_h

/* ecbuild info */

#ifndef ECBUILD_VERSION_STR
#define ECBUILD_VERSION_STR "2.8.1"
#endif

/* cpu arch info */

/* #undef EC_BIG_ENDIAN */
#define EC_LITTLE_ENDIAN    1

/* compiler support */

/* #undef EC_HAVE_FUNCTION_DEF */

/* os capability checks */

/* --- symbols --- */

#define EC_HAVE_FSEEK
#define EC_HAVE_FSEEKO
#define EC_HAVE_FTELLO
/* #undef EC_HAVE_LSEEK */
/* #undef EC_HAVE_FTRUNCATE */
/* #undef EC_HAVE_OPEN */
#define EC_HAVE_FOPEN
#define EC_HAVE_FMEMOPEN
#define EC_HAVE_FUNOPEN
#define EC_HAVE_FLOCK
#define EC_HAVE_MMAP
/* #undef EC_HAVE_FOPENCOOKIE */

#define EC_HAVE_POSIX_MEMALIGN

#define EC_HAVE_F_GETLK
#define EC_HAVE_F_SETLKW
#define EC_HAVE_F_SETLK

/* #undef EC_HAVE_F_GETLK64 */
/* #undef EC_HAVE_F_SETLKW64 */
/* #undef EC_HAVE_F_SETLK64 */

#define EC_HAVE_MAP_ANONYMOUS
#define EC_HAVE_MAP_ANON

/* --- include files --- */

#define EC_HAVE_ASSERT_H
#define EC_HAVE_STDLIB_H
#define EC_HAVE_UNISTD_H
#define EC_HAVE_STRING_H
#define EC_HAVE_STRINGS_H
#define EC_HAVE_SYS_STAT_H
#define EC_HAVE_SYS_TIME_H
#define EC_HAVE_SYS_TYPES_H

/* #undef EC_HAVE_MALLOC_H */
#define EC_HAVE_SYS_MALLOC_H

#define EC_HAVE_SYS_PARAM_H
#define EC_HAVE_SYS_MOUNT_H
/* #undef EC_HAVE_SYS_VFS_H */

/* --- capabilities --- */

#define EC_HAVE_OFFT
/* #undef EC_HAVE_OFF64T */

#define EC_HAVE_STRUCT_STAT
#define EC_HAVE_STRUCT_STAT64
#define EC_HAVE_STAT
#define EC_HAVE_STAT64
#define EC_HAVE_FSTAT
#define EC_HAVE_FSTAT64

/* #undef EC_HAVE_FSEEKO64 */
/* #undef EC_HAVE_FTELLO64 */
/* #undef EC_HAVE_LSEEK64 */
/* #undef EC_HAVE_OPEN64 */
/* #undef EC_HAVE_FOPEN64 */
/* #undef EC_HAVE_FTRUNCATE64 */
/* #undef EC_HAVE_FLOCK64 */
/* #undef EC_HAVE_MMAP64 */

#define EC_HAVE_STRUCT_STATVFS
/* #undef EC_HAVE_STRUCT_STATVFS64 */
/* #undef EC_HAVE_STATVFS */
/* #undef EC_HAVE_STATVFS64 */

#define EC_HAVE_FSYNC
/* #undef EC_HAVE_FDATASYNC */
#define EC_HAVE_DIRFD
#define EC_HAVE_SYSPROC
/* #undef EC_HAVE_SYSPROCFS */

#define EC_HAVE_EXECINFO_BACKTRACE

/* --- reentrant funtions support --- */

#define EC_HAVE_GMTIME_R
#define EC_HAVE_GETPWUID_R
#define EC_HAVE_GETPWNAM_R
#define EC_HAVE_READDIR_R
#define EC_HAVE_DIRENT_D_TYPE
/* #undef EC_HAVE_GETHOSTBYNAME_R */

/* --- compiler __attribute__ support --- */

#define EC_HAVE_ATTRIBUTE_CONSTRUCTOR
/* #undef EC_ATTRIBUTE_CONSTRUCTOR_INITS_ARGV */
/* #undef EC_HAVE_PROCFS */


/* --- dl library support --- */

#define EC_HAVE_DLFCN_H
#define EC_HAVE_DLADDR

/* --- c compiler support --- */

#define EC_HAVE_C_INLINE

/* --- c++ compiler support --- */

/* #undef EC_HAVE_FUNCTION_DEF */

/* #undef EC_HAVE_CXXABI_H */
/* #undef EC_HAVE_CXX_BOOL */
/* #undef EC_HAVE_CXX_INT_128 */

/* #undef EC_HAVE_CXX_SSTREAM */

/* config info */

#define ECCODES_OS_NAME          "Darwin-17.5.0"
#define ECCODES_OS_BITS          64
#define ECCODES_OS_BITS_STR      "64"
#define ECCODES_OS_STR           "macosx.64"
#define ECCODES_OS_VERSION       "17.5.0"
#define ECCODES_SYS_PROCESSOR    "x86_64"

#define ECCODES_BUILD_TIMESTAMP  "20180522122950"
#define ECCODES_BUILD_TYPE       "RelWithDebInfo"

#define ECCODES_C_COMPILER_ID      "Clang"
#define ECCODES_C_COMPILER_VERSION "9.1.0.9020039"

#define ECCODES_CXX_COMPILER_ID      ""
#define ECCODES_CXX_COMPILER_VERSION ""

#define ECCODES_C_COMPILER       "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc"
#define ECCODES_C_FLAGS          " -O2 -g"

#define ECCODES_CXX_COMPILER     ""
#define ECCODES_CXX_FLAGS        ""

/* Needed for finding per package config files */

/* #undef EC_HAVE_FORTRAN */

#ifdef EC_HAVE_FORTRAN

#define ECCODES_Fortran_COMPILER_ID      ""
#define ECCODES_Fortran_COMPILER_VERSION ""

#define ECCODES_Fortran_COMPILER ""
#define ECCODES_Fortran_FLAGS    ""

#endif

/* #undef BOOST_UNIT_TEST_FRAMEWORK_HEADER_ONLY */
/* #undef BOOST_UNIT_TEST_FRAMEWORK_LINKED */

#endif /* eccodes_ecbuild_config_h */
