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



/* batch_graphics.c

* contains entries
*     void set_batch_graphics()    ! sets program state
* and
*     int its_batch_graphics       ! queries program state

* programmer - steve hankin
* NOAA/PMEL, Seattle, WA - Tropical Modeling and Analysis Program

* revision 0.0 - 3/5/97
* v552 *acm* 6/5/03 check for the new flag its_gif
* v602 *acm*  12/07 additions for metafile batch mode; new flag its_meta
*                   and routine its_meta_graphics to check for it
*
* compile with
*    cc -g -c batch_graphics.c
*  or
*    cc    -c batch_graphics.c

*/

/* *kob* 10/03 v553 - gcc v3.x needs wchar.h included */
#include <Python.h> /* make sure Python.h is first */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef NO_ENTRY_NAME_UNDERSCORES
#define FORTRAN(a) a
#else
#define FORTRAN(a) a##_
#endif

/* local static variable to contain the state */
static int its_batch=0;
static int its_gif=0;
static int its_ps=0;
static int its_meta=0;

/* set_batch_graphics */
void FORTRAN(set_batch_graphics)(char *outfile)
{
  int length;
  char * result;
  int modestate;


  assert(outfile);
  length = strlen(outfile);
  modestate = 1;
  FORTRAN(save_metafile_name)(outfile, &length, &modestate);
  its_batch = -1;

  result = strstr(outfile,".gif"); 
  if (result)  {
      its_gif = -1;
   }
  result = strstr(outfile,".ps"); 
  if (result)  {
      its_ps = -1;
   }
  result = strstr(outfile,".plt"); 
  if (result)  {
      its_meta = -1;
   }
  return;
}

/* its_batch_graphics */
int FORTRAN(its_batch_graphics)()
{
   return (its_batch);
}

      /* its_gif_graphics */
int FORTRAN(its_gif_graphics)()
{
   return (its_gif);
}

      /* its_ps_graphics */
int FORTRAN(its_ps_graphics)()
{
   return (its_ps);
}

      /* its_meta_graphics */
int FORTRAN(its_meta_graphics)()
{
   return (its_meta);
}


      


