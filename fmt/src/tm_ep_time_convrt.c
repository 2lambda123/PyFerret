/*
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
*/



/* this routine extracted unmodified from the EPS library file fil_time.c
   Replaces the earlier port of this routine under the name tm_ep_time_convrt_
   which was FORTRAN-accessible.  This routine user, instead a FORTRAN jacket
   *sh* 1/94
*/
/* *acm*  1/12      - Ferret 6.8 ifdef double_p for double-precision ferret, see the
*					 definition of macro DFTYPE in ferretmacros.h.
*/

#include <Python.h> /* make sure Python.h is first */
#include "fmtprotos.h"

#define JULGREG   2299161

static void ep_time_to_mdyhms(long *time, int *mon, int *day, int *yr, int *hour, int *min, DFTYPE *sec)
{
/*
 * convert eps time format to mdy hms
 */
  long ja, jalpha, jb, jc, jd, je;

  while(time[1] >= 86400000) { /* increament days if ms larger then one day */
    time[0]++;
    time[1] -= 86400000;
  }

  if(time[0] >= JULGREG) {
    jalpha=((double) (time[0]-1867216)-0.25)/36524.25;
    ja=time[0]+1+jalpha-(long)(0.25*jalpha);
  } else
    ja=time[0];

  jb=ja+1524;
  jc=6680.0+((double)(jb-2439870)-122.1)/365.25;
  jd=365*jc+(0.25*jc);
  je=(jb-jd)/30.6001;
  *day=jb-jd-(int)(30.6001*je);
  *mon=je-1;
  if(*mon > 12) *mon -= 12;
  *yr=jc-4715;
  if(*mon > 2) --(*yr);

  if(*yr <=0) --(*yr);

  ja = time[1]/1000;
  *hour = ja/3600;
  *min = (ja - (*hour)*3600)/60;
  *sec = (DFTYPE)(time[1] - ((*hour)*3600 + (*min)*60)*1000)/1000.0;
}

/* convert from eptime to mdyhms */
/* FORTRAN-callable jacket extracted from file jackets.c
   Note:  The entry point name has been changed from mdyhmstoeptime_
   to tm_ep_time_convrt_ and the calling argument time[2] has been
   changed to the *int args epjday and epmsec.  Also, the entry has 
   re-written in prototyping format

   calling arguments:
      epjday (input) - integer
      epmsec (input) - integer
      mon, day, yr, hr, min (output) - integer
      sec (output) - DFTYPE

   *sh* 1/94
*/

void FORTRAN(tm_ep_time_convrt)(int *epjday, int *epmsec, int *mon, int *day, int *yr, int *hour, int *min, DFTYPE *sec)
{
/*  this block added by *sh* 1/94 */
  long time[2];
  time[0] = (long)*epjday;
  time[1] = (long)*epmsec;

  ep_time_to_mdyhms(time, mon, day, yr, hour, min, sec);
}



