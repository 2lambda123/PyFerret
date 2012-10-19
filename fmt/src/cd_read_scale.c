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

/*   
    cd_read_scale.c -- from cd_read_sub.c to read data as double
    precision, if that is how they are in the file, and apply the
    scale and offset values before converting to single precision.
    
  v6.01 9/06 *acm* use a malloc rather than a fixed buffer -> the data
              does not need to be 1-D
*/ 
/* *acm   2/11 v67  - Call nc_get_varm only if strides and permuted.
                      Call nc_get_vars if strided, and nc_get_vara if neither permuted
					  nor strided. */
/* *acm*  1/12      - Ferret 6.8 ifdef double_p for double-precision ferret, see the
 *					 definition of macro DFTYPE in ferretmacros.h. */
/*  V674 2/12 *acm* 6D Ferret: use NFERDIMS rather than 4 for dimension indexing */

#include <Python.h> /* make sure Python.h is first */
#include <stddef.h>  /* size_t, ptrdiff_t; gfortran on linux rh5*/
#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>
#include <signal.h>
#include <setjmp.h>

#include "ferretmacros.h"
#include "list.h"
#include "NCF_Util.h"


/* jump buffer for returning to the point prior to calling nc_* functions */
static jmp_buf cd_read_scale_sigint_jmp_buf;

/* pointer to previous function called when Ctrl-C is given */
static void (*orig_sigint_handler)(int signum);

/* function called when Ctrl-C is given */
static void cd_read_scale_sigint_handler(int signum) {
    /* Return to the setjmp call but return a value of 1 */
    longjmp(cd_read_scale_sigint_jmp_buf, 1);
}

/* prototype */
void tm_scale_buffer(DFTYPE *dat, double *dbuff,
			   DFTYPE *offset, DFTYPE *scale, DFTYPE *bad,
			   int ntotal);

void FORTRAN(cd_read_scale) (int *cdfid, int *varid, int *dims, 
			   DFTYPE *offset, DFTYPE *scale, DFTYPE* bad,
			   int *tmp_start, int *tmp_count, 
			   int *tmp_stride, int *tmp_imap,
			   void *dat, int *permuted, int *strided, int *already_scaled,
			   int *cdfstat, int *status)

{

  /* convert FORTRAN-index-ordered, FORTRAN-1-referenced ids, count,
     and start to C equivalent

     *kob* need start,count,stride and imap variables of the same type
           as is predfined for each O.S.
  */

  size_t start[NFERDIMSP1], count[NFERDIMSP1];
  ptrdiff_t stride[NFERDIMSP1], imap[NFERDIMSP1];

  int tmp, i, maxstrlen, ndimsp, *dimids;
  size_t bufsiz;
  int ndim = *dims - 1; /* C referenced to zero */
  int vid = *varid;
  nc_type vtyp;
  int n_sections;
  int ntotal;
  int scale_it;
  double *data_double;

  /* cast passed in int values (from fortran) to proper types, which can
     be different depending on o.s       *kob* 11/01 */
  ntotal = 1;
  for (i=0; i<=ndim; i++) {
    start[i] = (size_t)tmp_start[i];
    count[i] = (size_t)tmp_count[i];
	ntotal = ntotal * count[i];
    stride[i] = (ptrdiff_t)tmp_stride[i];
    imap[i] = (ptrdiff_t)tmp_imap[i];
  }


  /* change FORTRAN indexing and offsets to C */
  vid--;
  for (i=0; i<=ndim; i++)
    start[i]--;
  for (i=0; i<=ndim/2; i++) {
    tmp = count[i];
    count[i] = count[ndim-i];
    count[ndim-i] = tmp;
    tmp = start[i];
    start[i] = start[ndim-i];
    start[ndim-i] = tmp;
    tmp = stride[i];
    stride[i] = stride[ndim-i];
    stride[ndim-i] = tmp;
    tmp = imap[i];
    imap[i] = imap[ndim-i];
    imap[ndim-i] = tmp;
  }

  /* 
   * Capture the program state at this moment (zero is returned)
   * or returning via longjmp after a Ctrl-C (non-zero is returned)
   */
  if ( setjmp(cd_read_scale_sigint_jmp_buf) != 0 ) {
      /* restore the original Ctrl-C handler */
      signal(SIGINT, orig_sigint_handler);
      /* call CTRLC_AST (in fer/gnl/ctrl_c.F) to set the interrupted flag */
      ctrlc_ast_();
      /* return NC_INTERRUPT in cdfstat to indicate the read interrupt */
      *cdfstat = NC_INTERRUPT;
      return;
  }

  /* Put in our own Ctrl-C handler */
  orig_sigint_handler = signal(SIGINT, cd_read_scale_sigint_handler);
  if ( orig_sigint_handler == SIG_ERR )
      abort();

  /* get the type of the variable on disk */
  *cdfstat = nc_inq_vartype(*cdfid, vid, &vtyp);
  if (*cdfstat != NC_NOERR) {
      /* restore the original Ctrl-C handler */
      signal(SIGINT, orig_sigint_handler);
      return;
  }

  *status = 3;  /* merr_ok*/


  if (vtyp == NC_CHAR) 
	{
          /* restore the original Ctrl-C handler */
          signal(SIGINT, orig_sigint_handler);
	  *status = 111;
	  return;
	}

  scale_it = 0;
  if (*offset != 0 || *scale != 1)
  { scale_it = 1;
  }
  if (vtyp == NC_DOUBLE && scale_it) 
  {

    /* Read into a buffer area as double precision,
	   and apply the scaling before converting to single precision 
	   in variable dat
    */

      data_double = (double *) malloc(ntotal * sizeof(double));
      if ( data_double == NULL )
          abort();

      if (*permuted > 0)
      {
      *cdfstat = nc_get_varm_double (*cdfid, vid, start,
				  count, stride, imap, data_double);
      }
	  else if (*strided > 0)
	  {
      *cdfstat = nc_get_vars_double (*cdfid, vid, start,
				  count, stride, data_double);
      }
	  else
	  {
      *cdfstat = nc_get_vara_double (*cdfid, vid, start,
				  count, data_double);
	  }


      tm_scale_buffer ((DFTYPE*) dat, data_double, offset,
         scale, bad, ntotal);
	  *already_scaled = 1;

	  free(data_double);
                  
  }
   
  /* read float data */
  else
  {
      if (*permuted > 0)
	  {
#ifdef double_p
    *cdfstat = nc_get_varm_double (*cdfid, vid, start,
     count, stride, imap, (double*) dat); 
	  }
	  else if (*strided > 0)
	  {
    *cdfstat = nc_get_vars_double (*cdfid, vid, start,
     count, stride, (double*) dat);
      }
	  else
	  { 
    *cdfstat = nc_get_vara_double (*cdfid, vid, start,
     count, (double*) dat);
#else
    *cdfstat = nc_get_varm_float (*cdfid, vid, start,
     count, stride, imap, (float*) dat); 
	  }
	  else if (*strided > 0)
	  {
    *cdfstat = nc_get_vars_float (*cdfid, vid, start,
     count, stride, (float*) dat);
      }
	  else
	  { 
    *cdfstat = nc_get_vara_float (*cdfid, vid, start,
     count, (float*) dat);
#endif
	  }
  }

  /* restore the original Ctrl-C handler */
  signal(SIGINT, orig_sigint_handler);

  return;
}

/*  */
void tm_scale_buffer(DFTYPE *dat, double *dbuff,
                     DFTYPE *offset, DFTYPE *scale, DFTYPE *bad,
                     int ntotal)

{
        int j;
        double dbad;

        dbad = (double)*bad;
        for (j=0; j<ntotal; j++ )
        {
                if (dbuff[j] == dbad)
                        { dat[j] = *bad;
                        }
                else
                        {dat[j] = dbuff[j] * *scale + *offset;
                }
        }
    return;
}
