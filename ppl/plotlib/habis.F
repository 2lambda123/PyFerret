      SUBROUTINE HABIS(IPLOT,IPAUSE,IHDCPY,IN)
*
*
*  This software was developed by the Thermal Modeling and Analysis
*  Project(TMAP) of the National Oceanographic and Atmospheric
*  Administration's (NOAA) Pacific Marine Environmental Lab(PMEL),
*  hereafter referred to as NOAA/PMEL/TMAP.
*
*  Access and use of this software shall impose the following
*  obligations and understandings on the user. The user is granted the
*  right, without any fee or cost, to use, copy, modify, alter, enhance
*  and distribute this software, and any derivative works thereof, and
*  its supporting documentation for any purpose whatsoever, provided
*  that this entire notice appears in all copies of the software,
*  derivative works and supporting documentation.  Further, the user
*  agrees to credit NOAA/PMEL/TMAP in any publications that result from
*  the use of this software or in any product that includes this
*  software. The names TMAP, NOAA and/or PMEL, however, may not be used
*  in any advertising or publicity to endorse or promote any products
*  or commercial entity unless specific written permission is obtained
*  from NOAA/PMEL/TMAP. The user also understands that NOAA/PMEL/TMAP
*  is not obligated to provide the user with any support, consulting,
*  training or assistance of any kind with regard to the use, operation
*  and performance of this software nor to provide the user with any
*  updates, revisions, new versions or "bug fixes".
*
*  THIS SOFTWARE IS PROVIDED BY NOAA/PMEL/TMAP "AS IS" AND ANY EXPRESS
*  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL NOAA/PMEL/TMAP BE LIABLE FOR ANY SPECIAL,
*  INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
*  RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
*  CONTRACT, NEGLIGENCE OR OTHER TORTUOUS ACTION, ARISING OUT OF OR IN
*  CONNECTION WITH THE ACCESS, USE OR PERFORMANCE OF THIS SOFTWARE. 
*
*
C**    @(#)habis.F	1.1    3/10/88
* 11/2022 *acm* Code cleanup: Remove ref to pause, bell, etc. Not used
*               by modern code.
C**
C**
C***********************************************************************
C**
C**                 PLOT+ Scientific Graphics System
C**
C***********************************************************************
C**
C**
	INCLUDE 'PLTCOM.DAT'
	INCLUDE 'SYSTEM.INC'
	CHARACTER INS*1
C
C     THIS ROUTINE IS OUR ALTERNATIVE TO PLOTEND.
C         IPLOT=XXXX0YYYY0Z
C             XXXX=BAUD
C             YYYY=TEK MODEL
C             Z=ITYPE (PLOT TYPE)
C         IPAUSE = 0 FOR NO PAUSE
C                = 1 FOR A  PAUSE
C         IHDCPY= 0 FOR NO HARDCOPY
C                = 1 FOR A HARD COPY
C
C
c ( call alphas, call page, which boil down to these calls)

C     SET THE PLOTTING MODE.
C
      PMODE = ALPHA

C     IF THE LAST POINT WAS NOT PLOTTED BECAUSE THE PEN WAS UP,
C     PLOT THE POINT NOW.

      IF(SAVEF)CALL XYZPLT
	call clsdev(termf,sover)
	RETURN
	END
