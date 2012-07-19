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
/*  V674 2/12 *acm* 6D Ferret: use NFERDIMS rather than 4 for dimension indexing */



/* Read a DOUBLE hyperslab from a netCDF file into a FLOAT array.
   Since the array to be read is bigger than the amount of memory
   we can count on being available in the calling program buffer
   we will allocate temporary storage and use it to buffer the read.
*/

/* compilation may require "-I/usr/local/include" (to find netcdf.h) */

/* Call from FORTRAN using

     CALL CD_RD_R8_AS_R4( cdfid, varid, start, count, ndim, array, cdfstat )
*/

#include <Python.h> /* make sure Python.h is first */
#include <stddef.h>  /* size_t, ptrdiff_t  gfortran on linux rh5*/
#include <netcdf.h>      /* usually in /usr/local/include */
/* Suns need to include stdio.h to get definition for NULL */
/* *acm   9/06 v600 - add stdlib.h wherever there is stdio.h for altix build */
#include <stdlib.h> 
#include <stdio.h>
#include "ferretmacros.h"  /* with NFERDIMS */

#ifdef NO_ENTRY_NAME_UNDERSCORES
void cd_rd_r8_as_r4(cdfid,
#else
void cd_rd_r8_as_r4_(cdfid,
#endif
		    varid,
		    start,
		    count,
		    ndim,
		    values,
		    cdfstat)

int *cdfid;
int *varid;
int start[NFERDIMS];
int count[NFERDIMS];
int *ndim;
float *values;
int *cdfstat;/*returns one of: ncnoerr, netCDF status, or -1 (malloc failed)*/ 

{

  int  idim, i, npts, rcode;
  long cstart[NFERDIMS], ccount[NFERDIMS];
  double *dvals;

/* change the start/count values to C ordering and the start to zero offset */
  for (idim=0; idim<*ndim; idim++){
    cstart[idim] = (long) (start[(*ndim)-1-idim] - 1);
    ccount[idim] = (long) (count[(*ndim)-1-idim]);
  }
/* the total number of data points */
  for (npts=1,idim=0; idim<*ndim; idim++) npts *= count[idim];

/* allocate memory for the double precision hyperslab */
  dvals = (double *) malloc(8*npts);
  if ( dvals == NULL ) {
    *cdfstat = -1;
    return;
  }

/* read the data */
  rcode = ncvarget(*cdfid,*varid-1,cstart,ccount,dvals);
  if ( rcode == -1 ) {
    free(dvals);
    *cdfstat = ncerr;     /* global var from netcdf.h */
    return;
  }
  
/* convert to single precision */
  for (i=0; i<npts; i++) values[i] = (float) dvals[i];

/* successful completion */
  free(dvals);
  *cdfstat = NC_NOERR;
  return;
}
