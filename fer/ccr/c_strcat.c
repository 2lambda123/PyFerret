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
 * Concatenate two strings, returning the output pointer to the new string
 *
 * v5.41 *kob*  3/02
 *
 */

#include <Python.h> /* make sure Python.h is first */
#include <stdlib.h>
#include <string.h>
#include "ferret.h"

void FORTRAN(c_strcat)(char **in_ptr1, char **in_ptr2, char **out_ptr)  
{
   char* tmp1; 
   char* tmp2;
   int len1;
   int len2;

   if ( *out_ptr != NULL )
      PyMem_Free(*out_ptr);

   /* this treats an undefined string the same as an empty string */
   if ( *in_ptr1 == NULL )
      len1 = 0;
   else
      len1 = strlen(*in_ptr1);
   if ( *in_ptr2 == NULL )
      len2 = 0;
   else
      len2 = strlen(*in_ptr2);

   *out_ptr = (char *) PyMem_Malloc(sizeof(char) * (len1 + len2 + 1));
   if ( *out_ptr == NULL )
      abort();

   tmp2 = *out_ptr;
   if ( *in_ptr1 != NULL ) {
      tmp1 = *in_ptr1;
      while ( *tmp1 != '\0' ) {
         *tmp2 = *tmp1;
         tmp1++;
         tmp2++;
      } 
   } 
   if ( *in_ptr2 != NULL ) {
      tmp1 = *in_ptr2;
      while ( *tmp1 != '\0' ) {
         *tmp2 = *tmp1;
         tmp1++;
         tmp2++;
      }
   }
   *tmp2 = '\0';
}
