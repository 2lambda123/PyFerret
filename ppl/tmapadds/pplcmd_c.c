/*  pplcmd_c.c  
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

   This intermediate "envelope" routine provides the global pointer to the
   PLOT+ memory buffer "X". The buffer size has been set if needed in 
   pplldc when called by pplldc. Call the PLOT+ pplcmd routine -- 
   the original arguments are simply passed through, with a new argument, 
   the PPLUS memory buffer

*		SUBROUTINE pplcmd(K,Z,MX,MY,IMN,IMX,JMN,JMX,pplmem)

*/
/*******************/

#include <Python.h> /* make sure Python.h is first */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "pplmem.h"

/* Called by FORTRAN subroutine pplcmd.  PPLCMD has its original
   input arguments, but all it does is put string variables in
   common and call pplcmd_c.
   
   Get ppl_memory and pass it to pplcmd_f, which has all the original code.
 */

void FORTRAN (pplcmd_c)(int*isi, int *icmdim, int *icmsze)

/*******************/

{
/* The global pointer to PLOT+ memory is declared as extern here
   (Defined in fermain_c.c)
*/
extern float *ppl_memory;

#ifdef NO_ENTRY_NAME_UNDERSCORES
  pplcmd_f(isi, icmdim, icmsze, ppl_memory);
#else
  pplcmd_f_(isi, icmdim, icmsze, ppl_memory);
#endif

  return;
}
