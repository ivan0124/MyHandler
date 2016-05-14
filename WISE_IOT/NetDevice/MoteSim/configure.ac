#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
m4_define([ver_current], [1])
m4_define([ver_revision], [0])
m4_define([ver_age], [0])
m4_define([lib_version],[ver_current.ver_revision.ver_age])


AC_PREREQ([2.68])
AC_INIT([MoteSim], [lib_version], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE([foreign -Wall])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_SUBST([LIB_VERSION], [ver_current:ver_revision:ver_age])
# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_AR
LT_INIT
# Checks for libraries.
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([rt], [clock_gettime])
AC_CHECK_LIB([AdvLog], [AdvLog_Init])

AC_CHECK_HEADERS([stdlib.h string.h unistd.h])


# Checks for typedefs, structures, and compiler characteristics.
#AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset strchr strdup strpbrk])

AC_CONFIG_FILES([Makefile
                 libs/Makefile
                 src/Makefile
		 data/Makefile
		 include/Makefile
		 ])
AC_OUTPUT
