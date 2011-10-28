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
*/

/* 
   Return a float value from a character string

   v5.41 *kob*  3/02

*/

/* *kob* 10/03 v553 - gcc v3.x needs wchar.h included */
/* *acm*  3/05 v581 - return bad_value if input cannot be converted to numeric */
#include <Python.h> /* make sure Python.h is first */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float c_strfloat_(in_ptr, out_ptr, bad_ptr)
     char** in_ptr;
     float* out_ptr;
	 float* bad_ptr;
{
  double dval;
  float fval;
  int slen;
  

  dval = atof(*in_ptr);
  fval = dval;

  *out_ptr = fval;

/*  The atof function returns 0 if there is an error.  We want to return a 
    bad-value. Concatenate the original string with a 1. If the input argument 
	is a numeric value (zero), we will get a non-zero result from atof. If we 
	get 0, then the original argument was not numeric.
*/

  if (fval == 0.0)
  {
	  dval = atof(strcat(*in_ptr,"1"));
	  if (dval == 0.0)
	  {
		  *out_ptr = *bad_ptr;
	  }
  }
  
  return;
}
