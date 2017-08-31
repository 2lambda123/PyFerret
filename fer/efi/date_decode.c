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
*  ARE DISCLAIMED. IN NO EVENT SHALL NOAA/PMEL/TMAP BE LIABLE FOR ANY SPECIAL
,
*  INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
*  RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
*  CONTRACT, NEGLIGENCE OR OTHER TORTUOUS ACTION, ARISING OUT OF OR IN
*  CONNECTION WITH THE ACCESS, USE OR PERFORMANCE OF THIS SOFTWARE.  
*
*/


/*
  Code to perform decoding of formatted dates and times
  called by date1900.F
  8/2006 *acm* add dummy 5th argument to days_from_day0,
               needed by ez_delimited_read for 64-big build.
  5/2007 *acm* Fixing bug 1510.  Return the result as an argument rather
               than a return from function; under 64-bit the return
			   always yielded 0.
* *acm*  1/12      - Ferret 6.8 ifdef double_p for double-precision ferret, see the
*					 definition of macro DFTYPE in ferret.h.
*/

#include <Python.h> /* make sure Python.h is first */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "fmtprotos.h"
#include "ferret.h"

void FORTRAN(date_decode)(char *strdate, DFTYPE *res)
{
  int id,im,iy, ok, status;
  char str3[4],str1[2];
  char months[13][4] = {"jan","feb","mar","apr","may","jun","jul","aug","sep","oct","nov","dec"};
  DFTYPE rdum;
  double days_1900 = 59958230400.0 / (60.*60.*24.);

  if (sscanf(strdate,"%d/%d/%d%1s",&im,&id,&iy,str1) == 3)
    /* date as mm/dd/yy */
    {
      ok = 1;
    }
  else if (sscanf(strdate,"%d-%d-%d%1s",&iy,&im,&id,str1) == 3)
    /* date as yyyy-mm-dd */
    {
      ok = 1;
    }
  else if (sscanf(strdate,"%d-%3s-%d%1s",&id,str3,&iy,str1) == 3)
    /* date as dd-MMM-yy or dd-MMM-yyyy*/
    {
      /* 2 digit year */
      if (iy < 30)   /* will break after 2029 or before 1930 */
	iy += 2000;
      else if (iy<100)
	iy += 1900;
      
      /* translate month name */
      ok = 0;
      for (im=0; im<12; im++)
	{
	  if (strcasecmp(str3,months[im])==0)
	    {
	      im++;
	      ok = 1;
	      break;
	    }
	}
    }
  else
    {
      ok = 0;
    }    

  if (ok) {
    FORTRAN(days_from_day0)(&days_1900,&iy,&im,&id,&rdum,&status); 
    *res = rdum;
  }
  else
    *res = -1.e34;

}

