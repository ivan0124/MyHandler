/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014 Teunis van Beelen
*
* teuniz@gmail.com
*
***************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2 of the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
***************************************************************************
*
* This version of GPL is at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*
***************************************************************************
*/

/* last revision: October 05, 2014 */

/* For more info and how to use this libray, visit: http://www.teuniz.net/RS-232/ */


#ifndef rs232_INCLUDED
#define rs232_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__linux__) || defined(__FreeBSD__)

#include <stdio.h>
#include <string.h>

#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

#else

#include <windows.h>

#endif
int RS232_OpenComport(unsigned char *comport, int, const char *);
int RS232_PollComport(unsigned char *, int);
int RS232_SendByte(unsigned char);
int RS232_SendBuf(unsigned char *, int);
void RS232_CloseComport(void);
void RS232_cputs(const char *);
int RS232_IsDCDEnabled(void);
int RS232_IsCTSEnabled(void);
int RS232_IsDSREnabled(void);
void RS232_enableDTR(void);
void RS232_disableDTR(void);
void RS232_enableRTS(void);
void RS232_disableRTS(void);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif


