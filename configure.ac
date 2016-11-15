#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libapt], [0.0], [ezindy@gmail.com])
AC_CONFIG_SRCDIR([src/APTAPI.h])
AM_INIT_AUTOMAKE
AC_OUTPUT(Makefile src/Makefile)
#AC_CONFIG_HEADERS([config.h])

LT_INIT

PKG_CHECK_MODULES([libftdi1], [libftdi1])

# Configure AM_VARIABLES
m4_pattern_allow(AM_CFLAGS)

# Checks for programs.
AC_PROG_CC

AC_OUTPUT
