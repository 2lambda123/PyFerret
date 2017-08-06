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
 *  Save the address of Ferrets "memory(1,mr_blk1(mr))" as the contents
 *  of fer_ptr
 */

#include <Python.h> /* make sure Python.h is first */
#include "ferret.h"
#include "FerMem.h"

void FORTRAN(init_c_string_array)(int *length, char **mr_blk1, char ***fer_ptr)
{
   int i;
   char** ptr;

#ifdef MEMORYDEBUG
   char msg[1024];
   sprintf(msg, "init_c_string_array assigned as an array of %d 64-bit pointers (%d bytes) initialize to NULL", 
                mr_blk1, *length, (*length)*8);
   FerMem_WriteDebugMessage(mr_blk1, mr_blk1 + (*length), msg);
#endif

   /* save the pointer to the array of pointers */
   *fer_ptr = mr_blk1;

   /* initialize the pointers - room for 64-bit pointers, so double if 32-bit pointers */
   ptr = mr_blk1;
   for (i = 0; i < (*length)*(8/sizeof(char *)); i++) {
      *ptr = NULL;
      ptr++;
   }

}
