#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([Advantech JSON Library], [0], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE([foreign -Wall])
AC_PREFIX_DEFAULT([/usr/])
AC_CONFIG_FILES([advJSON.pc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_AR
LT_INIT
# Checks for libraries.
#AX_CHECK_COMPILE_FLAG([-std=c++11], [CXXFLAGS="$CXXFLAGS -std=c++11"])
AX_CHECK_COMPILE_FLAG([-std=c++11], [AX_APPEND_FLAG([-std=c++11], [CXXFLAGS])])
# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([localtime_r memset mkdir strchr])

AC_CONFIG_FILES([Makefile src/Makefile sample/Makefile])
AC_OUTPUT
