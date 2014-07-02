/*
*  This software was developed by the Thermal Modeling and Analysis
*  Project(TMAP) of the National Oceanographic and Atmospheric
*  Administration's (NOAA) Pacific Marine Environmental Lab(PMEL),
*  hereafter referred to as NOAA/PMEL/TMAP.
*
*  Access and use of this software shall impose the following
*  obligations and understandings on the user. The user is granteHd the
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


/* EF_InternalUtil.c
 *
 * Jonathan Callahan
 * Sep 4th 1997
 *
 * This file contains all the utility functions which Ferret
 * needs in order to communicate with an external function.
 */

/* Ansley Manke  March 2000
 *  Additions to allow internally linked external functions.
 *  Source code is in the directory FERRET/fer/efi
 *  In that directory, run the perl script int_dlsym.pl
 *  int_dlsym.pl ./ > intlines.c
 *  The result is lines of C code to be put into this file.
 *  Search for the comment string --------------------
 *
 *  1.  Function declaration lines.  Need to edit these to have
 *      the correct number of arguments for the _compute subroutines.
 *  2.  definition of N_INTEF and structure I_EFnames
 *  3.  internal_dlsym lines at the end

* Jonathan Callahan and Ansley Manke  30-May-2000
 * Fix memory leak:  already_have_internals needs to be tested for when 
 * we find the external function in efcn_gather_info  and set TRUE once
 * the internals have been set for the first time, also in efcn_gather_info.

* Ansley Manke  August 2001
 * add EOF_SPACE, EOF_STAT, EOF_TFUNC to the functions that are
 * statically linked 

* V5.4 *acm* 10/01 add compress* to the statically linked fcns
* v6.0 *acm*  5/06 many more functions internally linked.
* V6.0 *acm*  5/06 string results for external functions
* v6.0 *acm*  5/06 internal_dlsym was missing the nco functions.
* V6.03 *acm& 5/07 Add tax_ functions, fill_xy to the statically-linked functions
* V6.07 *acm* 8/07 remove xunits_data from list of I_EFnames; it should never 
*                  have been there.
* V6.12 *acm* 8/07 add functions scat2grid_bin_xy and scat2grid_nobs_xy.F
* V6.2 *acm* 11/08 New functions XCAT_STR, YCAT_STR, ...
* V6.2 *acm* 11/08 New internally-called function efcn_get_alt_type_fcn to
*                  get the name of a function to call if the arguments are of
*                  a different type than defined in the current function. E.g. 
*                  this lets the user reference XCAT with string arguments and  
*                  Ferret will run XCAT_STR
* V6.6 *acm* 4/10 add functions scat2grid_nbin_xy and scat2grid_nbin_xyt.F
* V664 *kms*  9/10 Added python-backed external functions via $FER_DIR/lib/libpyefcn.so
*                  Made external function language check more robust
*                  Check that GLOBAL_ExternalFunctionsList is not NULL in ef_ptr_from_id_ptr
*      *kms* 11/10 Check for libpyefcn.so in $FER_LIBS instead of $FER_DIR/lib
*      *kms* 12/10 Eliminated libpyefcn.so; link to pyefcn static library
*                  This makes libpython2.X a required library.
* *acm*  1/12      - Ferret 6.8 ifdef double_p for double-precision ferret, see the
*					 definition of macro DFTYPE in ferret.h
*      *kms*  3/12 Add E and F dimensions 
*      *acm*  6/14 New separate function for DSG files 
*/


/* .................... Includes .................... */
 
#include <Python.h> /* make sure Python.h is first */

#include <unistd.h>		/* for convenience */
#include <stdlib.h>		/* for convenience */
#include <stdio.h>		/* for convenience */
#include <string.h>		/* for convenience */
#include <fcntl.h>		/* for fcntl() */
#include <dlfcn.h>		/* for dynamic linking */
#include <signal.h>             /* for signal() */
#include <setjmp.h>             /* required for jmp_buf */

#include "ferret.h"
#include "EF_Util.h"
#include "list.h"		/* locally added list library */
#include "pyferret.h"		/* python external funtion interfaces */


/* ................ Global Variables ................ */
/*
 * The memory_ptr, mr_list_ptr and cx_list_ptr are obtained from Ferret
 * and cached whenever they are passed into one of the "efcn_" functions.
 * These pointers can be accessed by the utility functions in efn_ext/.
 * This way the EF writer does not need to see these pointers.
 *
 * This is the instantiation of these values.
 */

DFTYPE *GLOBAL_memory_ptr;
int    *GLOBAL_mr_list_ptr;
int    *GLOBAL_cx_list_ptr;
int    *GLOBAL_mres_ptr;
DFTYPE *GLOBAL_bad_flag_ptr;

static LIST *STATIC_ExternalFunctionList;

/*
 * The jumpbuffer is used by setjmp() and longjmp().
 * setjmp() is called by FORTRAN(efcn_compute)() in EF_InternalUtil.c and
 * saves the stack environment in jumpbuffer for later use by longjmp().
 * This allows one to bail out of external functions and still
 * return control to Ferret.
 * Check "Advanced Progrmming in the UNIX Environment" by Stevens
 * sections 7.10 and 10.14 to understand what's going on with these.
 */
static jmp_buf jumpbuffer;
static sigjmp_buf sigjumpbuffer;
static volatile sig_atomic_t canjump;

static int I_have_scanned_already = FALSE;
static int I_have_warned_already = TRUE; /* Warning turned off Jan '98 */

static void *internal_dlsym(char *name);
static void *ferret_ef_mem_subsc_so_ptr;
static void (*copy_ferret_ef_mem_subsc_ptr)(void);

/* ............. Function Declarations .............. */
/*
 * Note that all routines called directly from Ferret,
 * ie. directly from Fortran, should be all lower case,
 * be of type 'void', pass by reference and should end with 
 * an underscore.
 */


/* .... Functions called by Ferret .... */

int  FORTRAN(efcn_scan)( int * );
int  FORTRAN(efcn_already_have_internals)( int * );

void FORTRAN(create_pyefcn)(char fname[], int *lenfname, char pymod[], int *lenpymod,
                            char errstring[], int *lenerrstring);

int  FORTRAN(efcn_gather_info)( int * );
void FORTRAN(efcn_get_custom_axes)( int *, int *, int * );
void FORTRAN(efcn_get_result_limits)( int *, DFTYPE *, int *, int *, int * );
void FORTRAN(efcn_compute)( int *, int *, int *, int *, int *, DFTYPE *, int *, DFTYPE *, int * );


void FORTRAN(efcn_get_custom_axis_sub)( int *, int *, double *, double *, double *, char *, int * );

int  FORTRAN(efcn_get_id)( char * );
int  FORTRAN(efcn_match_template)( int *, char * );

void FORTRAN(efcn_get_name)( int *, char * );
void FORTRAN(efcn_get_version)( int *, DFTYPE * );
void FORTRAN(efcn_get_descr)( int *, char * );
void FORTRAN(efcn_get_alt_type_fcn)( int *, char * );
int  FORTRAN(efcn_get_num_reqd_args)( int * );
void FORTRAN(efcn_get_has_vari_args)( int *, int * );
void FORTRAN(efcn_get_axis_will_be)( int *, int * );
void FORTRAN(efcn_get_axis_reduction)( int *, int * );
void FORTRAN(efcn_get_piecemeal_ok)( int *, int * );

void FORTRAN(efcn_get_axis_implied_from)( int *, int *, int * );
void FORTRAN(efcn_get_axis_extend_lo)( int *, int *, int * );
void FORTRAN(efcn_get_axis_extend_hi)( int *, int *, int * );
void FORTRAN(efcn_get_axis_limits)( int *, int *, int *, int * );
int  FORTRAN(efcn_get_arg_type)( int *, int *);
void FORTRAN(efcn_get_arg_name)( int *, int *, char * );
void FORTRAN(efcn_get_arg_unit)( int *, int *, char * );
void FORTRAN(efcn_get_arg_desc)( int *, int *, char * );
int  FORTRAN(efcn_get_rtn_type)( int *);


/* .... Functions called internally .... */

/* Fortran routines from the efn/ directory */
void FORTRAN(efcn_copy_array_dims)(void);
void FORTRAN(efcn_set_work_array_dims)(int *, int *, int *, int *, int *, int *, int *,
                                              int *, int *, int *, int *, int *, int *);
void FORTRAN(efcn_get_workspace_addr)(DFTYPE *, int *, DFTYPE *);

static void EF_signal_handler(int signo);
static void (*fpe_handler)(int);      /* function pointers */
static void (*segv_handler)(int);
static void (*int_handler)(int);
static void (*bus_handler)(int);
int EF_Util_setsig();
int EF_Util_ressig();


void FORTRAN(ef_err_bail_out)(int *, char *);

void EF_store_globals(DFTYPE *, int *, int *, int *, DFTYPE *);

ExternalFunction *ef_ptr_from_id_ptr(int *);

int  EF_ListTraverse_fprintf( char *, char * );
int  EF_ListTraverse_FoundName( char *, char * );
int  EF_ListTraverse_MatchTemplate( char *, char * );
int  EF_ListTraverse_FoundID( char *, char * );

int  EF_New( ExternalFunction * );

/*  ------------------------------------
 *  Statically linked external functions 
 *  Declarations generated by the perl script int_dlsym.pl.
 *  Need to fill out the arguments for the _compute subroutines.
 */

void FORTRAN(ffta_init)(int *);
void FORTRAN(ffta_custom_axes)(int *);
void FORTRAN(ffta_result_limits)(int *);
void FORTRAN(ffta_work_size)(int *);
void FORTRAN(ffta_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(fftp_init)(int *);
void FORTRAN(fftp_custom_axes)(int *);
void FORTRAN(fftp_result_limits)(int *);
void FORTRAN(fftp_work_size)(int *);
void FORTRAN(fftp_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(fft_im_init)(int *);
void FORTRAN(fft_im_custom_axes)(int *);
void FORTRAN(fft_im_result_limits)(int *);
void FORTRAN(fft_im_work_size)(int *);
void FORTRAN(fft_im_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(fft_inverse_init)(int *);
void FORTRAN(fft_inverse_result_limits)(int *);
void FORTRAN(fft_inverse_work_size)(int *);
void FORTRAN(fft_inverse_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(fft_re_init)(int *);
void FORTRAN(fft_re_custom_axes)(int *);
void FORTRAN(fft_re_result_limits)(int *);
void FORTRAN(fft_re_work_size)(int *);
void FORTRAN(fft_re_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(sampleij_init)(int *);
void FORTRAN(sampleij_result_limits)(int *);
void FORTRAN(sampleij_work_size)(int *);
void FORTRAN(sampleij_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
       DFTYPE *, DFTYPE *, DFTYPE *);


void FORTRAN(samplei_multi_init)(int *);
void FORTRAN(samplei_multi_result_limits)(int *);
void FORTRAN(samplei_multi_compute)(int *, DFTYPE *, DFTYPE *);


void FORTRAN(samplej_multi_init)(int *);
void FORTRAN(samplej_multi_result_limits)(int *);
void FORTRAN(samplej_multi_compute)(int *, DFTYPE *, DFTYPE *);


void FORTRAN(samplek_multi_init)(int *);
void FORTRAN(samplek_multi_result_limits)(int *);
void FORTRAN(samplek_multi_compute)(int *, DFTYPE *, DFTYPE *);


void FORTRAN(samplel_multi_init)(int *);
void FORTRAN(samplel_multi_result_limits)(int *);
void FORTRAN(samplel_multi_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(samplet_date_init)(int *);
void FORTRAN(samplet_date_result_limits)(int *);
void FORTRAN(samplet_date_work_size)(int *);
void FORTRAN(samplet_date_compute)(int *, DFTYPE *, DFTYPE *,
      DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
      DFTYPE *, DFTYPE *);

void FORTRAN(samplexy_init)(int *);
void FORTRAN(samplexy_result_limits)(int *);
void FORTRAN(samplexy_work_size)(int *);
void FORTRAN(samplexy_compute)(int *, DFTYPE *, DFTYPE *,
      DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(samplexyt_init)(int *);
void FORTRAN(samplexyt_result_limits)(int *);
void FORTRAN(samplexyt_work_size)(int *);
void FORTRAN(samplexyt_compute)(int *, DFTYPE *, DFTYPE *,
      DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(samplexyt_nrst_init)(int *);
void FORTRAN(samplexyt_nrst_result_limits)(int *);
void FORTRAN(samplexyt_nrst_work_size)(int *);
void FORTRAN(samplexyt_nrst_compute)(int *, DFTYPE *, DFTYPE *,
      DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridgauss_xy_init)(int *);
void FORTRAN(scat2gridgauss_xy_work_size)(int *);
void FORTRAN(scat2gridgauss_xy_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridgauss_xz_init)(int *);
void FORTRAN(scat2gridgauss_xz_work_size)(int *);
void FORTRAN(scat2gridgauss_xz_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridgauss_yz_init)(int *);
void FORTRAN(scat2gridgauss_yz_work_size)(int *);
void FORTRAN(scat2gridgauss_yz_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridgauss_xt_init)(int *);
void FORTRAN(scat2gridgauss_xt_work_size)(int *);
void FORTRAN(scat2gridgauss_xt_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridgauss_yt_init)(int *);
void FORTRAN(scat2gridgauss_yt_work_size)(int *);
void FORTRAN(scat2gridgauss_yt_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridgauss_zt_init)(int *);
void FORTRAN(scat2gridgauss_zt_work_size)(int *);
void FORTRAN(scat2gridgauss_zt_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridgauss_xy_v0_init)(int *);
void FORTRAN(scat2gridgauss_xy_v0_work_size)(int *);
void FORTRAN(scat2gridgauss_xy_v0_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridgauss_xz_v0_init)(int *);
void FORTRAN(scat2gridgauss_xz_v0_work_size)(int *);
void FORTRAN(scat2gridgauss_xz_v0_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridgauss_yz_v0_init)(int *);
void FORTRAN(scat2gridgauss_yz_v0_work_size)(int *);
void FORTRAN(scat2gridgauss_yz_v0_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridgauss_xt_v0_init)(int *);
void FORTRAN(scat2gridgauss_xt_v0_work_size)(int *);
void FORTRAN(scat2gridgauss_xt_v0_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridgauss_yt_v0_init)(int *);
void FORTRAN(scat2gridgauss_yt_v0_work_size)(int *);
void FORTRAN(scat2gridgauss_yt_v0_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridgauss_zt_v0_init)(int *);
void FORTRAN(scat2gridgauss_zt_v0_work_size)(int *);
void FORTRAN(scat2gridgauss_zt_v0_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridlaplace_xy_init)(int *);
void FORTRAN(scat2gridlaplace_xy_work_size)(int *);
void FORTRAN(scat2gridlaplace_xy_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridlaplace_xz_init)(int *);
void FORTRAN(scat2gridlaplace_xz_work_size)(int *);
void FORTRAN(scat2gridlaplace_xz_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridlaplace_yz_init)(int *);
void FORTRAN(scat2gridlaplace_yz_work_size)(int *);
void FORTRAN(scat2gridlaplace_yz_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *);


void FORTRAN(scat2gridlaplace_xt_init)(int *);
void FORTRAN(scat2gridlaplace_xt_work_size)(int *);
void FORTRAN(scat2gridlaplace_xt_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridlaplace_yt_init)(int *);
void FORTRAN(scat2gridlaplace_yt_work_size)(int *);
void FORTRAN(scat2gridlaplace_yt_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *);

void FORTRAN(scat2gridlaplace_zt_init)(int *);
void FORTRAN(scat2gridlaplace_zt_work_size)(int *);
void FORTRAN(scat2gridlaplace_zt_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *);

void FORTRAN(sorti_init)(int *);
void FORTRAN(sorti_result_limits)(int *);
void FORTRAN(sorti_work_size)(int *);
void FORTRAN(sorti_compute)(int *, DFTYPE *, DFTYPE *, 
      DFTYPE *, DFTYPE *);

void FORTRAN(sorti_str_init)(int *);
void FORTRAN(sorti_str_result_limits)(int *);
void FORTRAN(sorti_str_work_size)(int *);
void FORTRAN(sorti_str_compute)(int *, char *, DFTYPE *, 
      char *, DFTYPE *);
                   
void FORTRAN(sortj_init)(int *);
void FORTRAN(sortj_result_limits)(int *);
void FORTRAN(sortj_work_size)(int *);
void FORTRAN(sortj_compute)(int *, DFTYPE *, DFTYPE *, 
      DFTYPE *, DFTYPE *);

void FORTRAN(sortj_str_init)(int *);
void FORTRAN(sortj_str_result_limits)(int *);
void FORTRAN(sortj_str_work_size)(int *);
void FORTRAN(sortj_str_compute)(int *, char *, DFTYPE *, 
      char *, DFTYPE *);

void FORTRAN(sortk_init)(int *);
void FORTRAN(sortk_result_limits)(int *);
void FORTRAN(sortk_work_size)(int *);
void FORTRAN(sortk_compute)(int *, DFTYPE *, DFTYPE *, 
      DFTYPE *, DFTYPE *);

void FORTRAN(sortk_str_init)(int *);
void FORTRAN(sortk_str_result_limits)(int *);
void FORTRAN(sortk_str_work_size)(int *);
void FORTRAN(sortk_str_compute)(int *, char *, DFTYPE *, 
      char *, DFTYPE *);

void FORTRAN(sortl_init)(int *);
void FORTRAN(sortl_result_limits)(int *);
void FORTRAN(sortl_work_size)(int *);
void FORTRAN(sortl_compute)(int *, DFTYPE *, DFTYPE *, 
      DFTYPE *, DFTYPE *);

void FORTRAN(sortl_str_init)(int *);
void FORTRAN(sortl_str_result_limits)(int *);
void FORTRAN(sortl_str_work_size)(int *);
void FORTRAN(sortl_str_compute)(int *, char *, DFTYPE *, 
      char *, DFTYPE *);


void FORTRAN(sortm_init)(int *);
void FORTRAN(sortm_result_limits)(int *);
void FORTRAN(sortm_work_size)(int *);
void FORTRAN(sortm_compute)(int *, DFTYPE *, DFTYPE *, 
      DFTYPE *, DFTYPE *);

void FORTRAN(sortm_str_init)(int *);
void FORTRAN(sortm_str_result_limits)(int *);
void FORTRAN(sortm_str_work_size)(int *);
void FORTRAN(sortm_str_compute)(int *, char *, DFTYPE *, 
      char *, DFTYPE *);


void FORTRAN(sortn_init)(int *);
void FORTRAN(sortn_result_limits)(int *);
void FORTRAN(sortn_work_size)(int *);
void FORTRAN(sortn_compute)(int *, DFTYPE *, DFTYPE *, 
      DFTYPE *, DFTYPE *);

void FORTRAN(sortn_str_init)(int *);
void FORTRAN(sortn_str_result_limits)(int *);
void FORTRAN(sortn_str_work_size)(int *);
void FORTRAN(sortn_str_compute)(int *, char *, DFTYPE *, 
      char *, DFTYPE *);

void FORTRAN(tauto_cor_init)(int *);
void FORTRAN(tauto_cor_result_limits)(int *);
void FORTRAN(tauto_cor_work_size)(int *);
void FORTRAN(tauto_cor_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *);

void FORTRAN(xauto_cor_init)(int *);
void FORTRAN(xauto_cor_result_limits)(int *);
void FORTRAN(xauto_cor_work_size)(int *);
void FORTRAN(xauto_cor_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *);
						   
void FORTRAN(eof_space_init)(int *);
void FORTRAN(eof_space_result_limits)(int *);
void FORTRAN(eof_space_work_size)(int *);
void FORTRAN(eof_space_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);
						   
void FORTRAN(eof_stat_init)(int *);
void FORTRAN(eof_stat_result_limits)(int *);
void FORTRAN(eof_stat_work_size)(int *);
void FORTRAN(eof_stat_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);
						   
void FORTRAN(eof_tfunc_init)(int *);
void FORTRAN(eof_tfunc_result_limits)(int *);
void FORTRAN(eof_tfunc_work_size)(int *);
void FORTRAN(eof_tfunc_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);
						   
						   
void FORTRAN(eofsvd_space_init)(int *);
void FORTRAN(eofsvd_space_result_limits)(int *);
void FORTRAN(eofsvd_space_work_size)(int *);
void FORTRAN(eofsvd_space_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *);
						   
void FORTRAN(eofsvd_stat_init)(int *);
void FORTRAN(eofsvd_stat_result_limits)(int *);
void FORTRAN(eofsvd_stat_work_size)(int *);
void FORTRAN(eofsvd_stat_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *);
						   
void FORTRAN(eofsvd_tfunc_init)(int *);
void FORTRAN(eofsvd_tfunc_result_limits)(int *);
void FORTRAN(eofsvd_tfunc_work_size)(int *);
void FORTRAN(eofsvd_tfunc_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                           DFTYPE *);
 
void FORTRAN(compressi_init)(int *);
void FORTRAN(compressi_result_limits)(int *);
void FORTRAN(compressi_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(compressj_init)(int *);
void FORTRAN(compressj_result_limits)(int *);
void FORTRAN(compressj_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(compressk_init)(int *);
void FORTRAN(compressk_result_limits)(int *);
void FORTRAN(compressk_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(compressl_init)(int *);
void FORTRAN(compressl_result_limits)(int *);
void FORTRAN(compressl_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(compressm_init)(int *);
void FORTRAN(compressm_result_limits)(int *);
void FORTRAN(compressm_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(compressn_init)(int *);
void FORTRAN(compressn_result_limits)(int *);
void FORTRAN(compressn_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(compressi_by_init)(int *);
void FORTRAN(compressi_by_result_limits)(int *);
void FORTRAN(compressi_by_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(compressj_by_init)(int *);
void FORTRAN(compressj_by_result_limits)(int *);
void FORTRAN(compressj_by_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(compressk_by_init)(int *);
void FORTRAN(compressk_by_result_limits)(int *);
void FORTRAN(compressk_by_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(compressl_by_init)(int *);
void FORTRAN(compressl_by_result_limits)(int *);
void FORTRAN(compressl_by_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(compressm_by_init)(int *);
void FORTRAN(compressm_by_result_limits)(int *);
void FORTRAN(compressm_by_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(compressn_by_init)(int *);
void FORTRAN(compressn_by_result_limits)(int *);
void FORTRAN(compressn_by_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(labwid_init)(int *);
void FORTRAN(labwid_result_limits)(int *);
void FORTRAN(labwid_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(convolvei_init)(int *);
void FORTRAN(convolvei_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(convolvej_init)(int *);
void FORTRAN(convolvej_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(convolvek_init)(int *);
void FORTRAN(convolvek_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(convolvel_init)(int *);
void FORTRAN(convolvel_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(convolvem_init)(int *);
void FORTRAN(convolvem_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(convolven_init)(int *);
void FORTRAN(convolven_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(curv_range_init)(int *);
void FORTRAN(curv_range_result_limits)(int *);
void FORTRAN(curv_range_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(curv_to_rect_map_init)(int *);
void FORTRAN(curv_to_rect_map_result_limits)(int *);
void FORTRAN(curv_to_rect_map_work_size)(int *);
void FORTRAN(curv_to_rect_map_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                                       DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);
void FORTRAN(curv_to_rect_init)(int *);
void FORTRAN(curv_to_rect_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(rect_to_curv_init)(int *);
void FORTRAN(rect_to_curv_work_size)(int *);
void FORTRAN(rect_to_curv_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                                       DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(date1900_init)(int *);
void FORTRAN(date1900_result_limits)(int *);
void FORTRAN(date1900_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(days1900toymdhms_init)(int *);
void FORTRAN(days1900toymdhms_result_limits)(int *);
void FORTRAN(days1900toymdhms_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(minutes24_init)(int *);
void FORTRAN(minutes24_result_limits)(int *);
void FORTRAN(minutes24_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(element_index_init)(int *);
void FORTRAN(element_index_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(element_index_str_init)(int *);
void FORTRAN(element_index_str_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(element_index_str_n_init)(int *);
void FORTRAN(element_index_str_n_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(expnd_by_len_init)(int *);
void FORTRAN(expnd_by_len_result_limits)(int *);
void FORTRAN(expnd_by_len_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(expnd_by_len_str_init)(int *);
void FORTRAN(expnd_by_len_str_result_limits)(int *);
void FORTRAN(expnd_by_len_str_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(expndi_by_init)(int *);
void FORTRAN(expndi_by_result_limits)(int *);
void FORTRAN(expndi_by_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(expndi_by_t_init)(int *);
void FORTRAN(expndi_by_t_result_limits)(int *);
void FORTRAN(expndi_by_t_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(expndi_by_z_init)(int *);
void FORTRAN(expndi_by_z_result_limits)(int *);
void FORTRAN(expndi_by_z_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(expndi_by_z_counts_init)(int *);
void FORTRAN(expndi_by_z_counts_result_limits)(int *);
void FORTRAN(expndi_by_z_counts_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(expndi_id_by_z_counts_init)(int *);
void FORTRAN(expndi_id_by_z_counts_result_limits)(int *);
void FORTRAN(expndi_id_by_z_counts_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(expndi_by_m_counts_init)(int *);
void FORTRAN(expndi_by_m_counts_custom_axes)(int *);
void FORTRAN(expndi_by_m_counts_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(fc_isubset_init)(int *);
void FORTRAN(fc_isubset_result_limits)(int *);
void FORTRAN(fc_isubset_custom_axes)(int *);
void FORTRAN(fc_isubset_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(findhi_init)(int *);
void FORTRAN(findhi_result_limits)(int *);
void FORTRAN(findhi_work_size)(int *);
void FORTRAN(findhi_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                            DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(findlo_init)(int *);
void FORTRAN(findlo_result_limits)(int *);
void FORTRAN(findlo_work_size)(int *);
void FORTRAN(findlo_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                            DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(is_element_of_init)(int *);
void FORTRAN(is_element_of_result_limits)(int *);
void FORTRAN(is_element_of_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(is_element_of_str_init)(int *);
void FORTRAN(is_element_of_str_result_limits)(int *);
void FORTRAN(is_element_of_str_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);


void FORTRAN(is_element_of_str_n_init)(int *);
void FORTRAN(is_element_of_str_n_result_limits)(int *);
void FORTRAN(is_element_of_str_n_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(lanczos_init)(int *);
void FORTRAN(lanczos_work_size)(int *);
void FORTRAN(lanczos_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                            DFTYPE *, DFTYPE *);

void FORTRAN(lsl_lowpass_init)(int *);
void FORTRAN(lsl_lowpass_work_size)(int *);
void FORTRAN(lsl_lowpass_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
                            DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);
							

void FORTRAN(samplexy_curv_init)(int *);
void FORTRAN(samplexy_curv_result_limits)(int *);
void FORTRAN(samplexy_curv_work_size)(int *);
void FORTRAN(samplexy_curv_compute)(int *, DFTYPE *, DFTYPE *,
      DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(samplexy_curv_avg_init)(int *);
void FORTRAN(samplexy_curv_avg_result_limits)(int *);
void FORTRAN(samplexy_curv_avg_work_size)(int *);
void FORTRAN(samplexy_curv_avg_compute)(int *, DFTYPE *, DFTYPE *,
      DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(samplexy_curv_nrst_init)(int *);
void FORTRAN(samplexy_curv_nrst_result_limits)(int *);
void FORTRAN(samplexy_curv_nrst_work_size)(int *);
void FORTRAN(samplexy_curv_nrst_compute)(int *, DFTYPE *, DFTYPE *,
      DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(samplexy_closest_init)(int *);
void FORTRAN(samplexy_closest_result_limits)(int *);
void FORTRAN(samplexy_closest_work_size)(int *);
void FORTRAN(samplexy_closest_compute)(int *, DFTYPE *, DFTYPE *,
      DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(samplexz_init)(int *);
void FORTRAN(samplexz_result_limits)(int *);
void FORTRAN(samplexz_work_size)(int *);
void FORTRAN(samplexz_compute)(int *, DFTYPE *, DFTYPE *,
      DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(sampleyz_init)(int *);
void FORTRAN(sampleyz_result_limits)(int *);
void FORTRAN(sampleyz_work_size)(int *);
void FORTRAN(sampleyz_compute)(int *, DFTYPE *, DFTYPE *,
      DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2ddups_init)(int *);
void FORTRAN(scat2ddups_result_limits)(int *);
void FORTRAN(scat2ddups_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(ave_scat2grid_t_init)(int *);
void FORTRAN(ave_scat2grid_t_work_size)(int *);
void FORTRAN(ave_scat2grid_t_compute)(int *, DFTYPE *, DFTYPE *,
      DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2grid_t_init)(int *);
void FORTRAN(scat2grid_t_work_size)(int *);
void FORTRAN(scat2grid_t_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(transpose_xt_init)(int *);
void FORTRAN(transpose_xt_result_limits)(int *);
void FORTRAN(transpose_xt_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(transpose_xy_init)(int *);
void FORTRAN(transpose_xy_result_limits)(int *);
void FORTRAN(transpose_xy_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(transpose_xz_init)(int *);
void FORTRAN(transpose_xz_result_limits)(int *);
void FORTRAN(transpose_xz_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(transpose_yt_init)(int *);
void FORTRAN(transpose_yt_result_limits)(int *);
void FORTRAN(transpose_yt_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(transpose_yz_init)(int *);
void FORTRAN(transpose_yz_result_limits)(int *);
void FORTRAN(transpose_yz_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(transpose_zt_init)(int *);
void FORTRAN(transpose_zt_result_limits)(int *);
void FORTRAN(transpose_zt_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(xcat_init)(int *);
void FORTRAN(xcat_result_limits)(int *);
void FORTRAN(xcat_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(xcat_str_init)(int *);
void FORTRAN(xcat_str_result_limits)(int *);
void FORTRAN(xcat_str_compute)(int *, char *, char *, char *);

void FORTRAN(ycat_init)(int *);
void FORTRAN(ycat_result_limits)(int *);
void FORTRAN(ycat_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(ycat_str_init)(int *);
void FORTRAN(ycat_str_result_limits)(int *);
void FORTRAN(ycat_str_compute)(int *, char *, char *, char *);

void FORTRAN(zcat_init)(int *);
void FORTRAN(zcat_result_limits)(int *);
void FORTRAN(zcat_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(zcat_str_init)(int *);
void FORTRAN(zcat_str_result_limits)(int *);
void FORTRAN(zcat_str_compute)(int *, char *, char *, char *);

void FORTRAN(tcat_init)(int *);
void FORTRAN(tcat_result_limits)(int *);
void FORTRAN(tcat_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(tcat_str_init)(int *);
void FORTRAN(tcat_str_result_limits)(int *);
void FORTRAN(tcat_str_compute)(int *, char *, char *, char *);

void FORTRAN(ecat_init)(int *);
void FORTRAN(ecat_result_limits)(int *);
void FORTRAN(ecat_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(ecat_str_init)(int *);
void FORTRAN(ecat_str_result_limits)(int *);
void FORTRAN(ecat_str_compute)(int *, char *, char *, char *);

void FORTRAN(fcat_init)(int *);
void FORTRAN(fcat_result_limits)(int *);
void FORTRAN(fcat_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(fcat_str_init)(int *);
void FORTRAN(fcat_str_result_limits)(int *);
void FORTRAN(fcat_str_compute)(int *, char *, char *, char *);

void FORTRAN(xreverse_init)(int *);
void FORTRAN(xreverse_result_limits)(int *);
void FORTRAN(xreverse_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(yreverse_init)(int *);
void FORTRAN(yreverse_result_limits)(int *);
void FORTRAN(yreverse_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(zreverse_init)(int *);
void FORTRAN(zreverse_result_limits)(int *);
void FORTRAN(zreverse_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(treverse_init)(int *);
void FORTRAN(treverse_result_limits)(int *);
void FORTRAN(treverse_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(ereverse_init)(int *);
void FORTRAN(ereverse_result_limits)(int *);
void FORTRAN(ereverse_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(freverse_init)(int *);
void FORTRAN(freverse_result_limits)(int *);
void FORTRAN(freverse_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(zaxreplace_avg_init)(int *);
void FORTRAN(zaxreplace_avg_work_size)(int *);
void FORTRAN(zaxreplace_avg_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(zaxreplace_bin_init)(int *);
void FORTRAN(zaxreplace_bin_work_size)(int *);
void FORTRAN(zaxreplace_bin_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(zaxreplace_rev_init)(int *);
void FORTRAN(zaxreplace_rev_work_size)(int *);
void FORTRAN(zaxreplace_rev_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(zaxreplace_zlev_init)(int *);
void FORTRAN(zaxreplace_zlev_work_size)(int *);
void FORTRAN(zaxreplace_zlev_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(nco_attr_init)(int *);
void FORTRAN(nco_attr_result_limits)(int *);
void FORTRAN(nco_attr_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(nco_init)(int *);
void FORTRAN(nco_result_limits)(int *);
void FORTRAN(nco_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(tax_datestring_init)(int *);
void FORTRAN(tax_datestring_work_size)(int *);
void FORTRAN(tax_datestring_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(tax_day_init)(int *);
void FORTRAN(tax_day_work_size)(int *);
void FORTRAN(tax_day_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(tax_dayfrac_init)(int *);
void FORTRAN(tax_dayfrac_work_size)(int *);
void FORTRAN(tax_dayfrac_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(tax_jday1900_init)(int *);
void FORTRAN(tax_jday1900_work_size)(int *);
void FORTRAN(tax_jday1900_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(tax_jday_init)(int *);
void FORTRAN(tax_jday_work_size)(int *);
void FORTRAN(tax_jday_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(tax_month_init)(int *);
void FORTRAN(tax_month_work_size)(int *);
void FORTRAN(tax_month_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(tax_times_init)(int *);
void FORTRAN(tax_times_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(tax_tstep_init)(int *);
void FORTRAN(tax_tstep_work_size)(int *);
void FORTRAN(tax_tstep_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(tax_units_init)(int *);
void FORTRAN(tax_units_compute)(int *, DFTYPE *, DFTYPE*);

void FORTRAN(tax_year_init)(int *);
void FORTRAN(tax_year_work_size)(int *);
void FORTRAN(tax_year_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(tax_yearfrac_init)(int *);
void FORTRAN(tax_yearfrac_work_size)(int *);
void FORTRAN(tax_yearfrac_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(fill_xy_init)(int *);
void FORTRAN(fill_xy_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(test_opendap_init)(int *);
void FORTRAN(test_opendap_result_limits)(int *);
void FORTRAN(test_opendap_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2grid_bin_xy_init)(int *);
void FORTRAN(scat2grid_bin_xy_work_size)(int *);
void FORTRAN(scat2grid_bin_xy_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2grid_bin_xyt_init)(int *);
void FORTRAN(scat2grid_bin_xyt_work_size)(int *);
void FORTRAN(scat2grid_bin_xyt_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
  DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
  DFTYPE *, DFTYPE *);

void FORTRAN(scat2grid_nbin_xy_init)(int *);
void FORTRAN(scat2grid_nbin_xy_work_size)(int *);
void FORTRAN(scat2grid_nbin_xy_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2grid_nbin_xyt_init)(int *);
void FORTRAN(scat2grid_nbin_xyt_work_size)(int *);
void FORTRAN(scat2grid_nbin_xyt_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
  DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
  DFTYPE *, DFTYPE *);

void FORTRAN(scat2grid_nobs_xyt_init)(int *);
void FORTRAN(scat2grid_nobs_xyt_work_size)(int *);
void FORTRAN(scat2grid_nobs_xyt_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
  DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
  DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(scat2grid_nobs_xy_init)(int *);
void FORTRAN(scat2grid_nobs_xy_work_size)(int *);
void FORTRAN(scat2grid_nobs_xy_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(unique_str2int_init)(int *);
void FORTRAN(unique_str2int_compute)(char *, int *);

void FORTRAN(bin_index_wt_init)(int *);
void FORTRAN(bin_index_wt_result_limits)(int *);
void FORTRAN(bin_index_wt_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(minmax_init)(int *);
void FORTRAN(minmax_result_limits)(int *);
void FORTRAN(minmax_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(floatstr_init)(int *);
void FORTRAN(floatstr_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(pt_in_poly_init)(int *);
void FORTRAN(pt_in_poly_work_size)(int *);
void FORTRAN(pt_in_poly_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, 
  DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(list_value_xml_init)(int *);
void FORTRAN(list_value_xml_result_limits)(int *);
void FORTRAN(list_value_xml_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(write_webrow_init)(int *);
void FORTRAN(write_webrow_result_limits)(int *);
void FORTRAN(write_webrow_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

void FORTRAN(str_mask_init)(int *);
void FORTRAN(str_mask_compute)(int *, DFTYPE *, DFTYPE *);

void FORTRAN(separate_init)(int *);
void FORTRAN(separate_result_limits)(int *);
void FORTRAN(separate_compute)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

/*
 *  End of declarations for internally linked external functions
 *  ------------------------------------ */


/* .............. Function Definitions .............. */


/* .... Functions for use by Ferret (to be called from Fortran) .... */

/*
 * Note that all routines called directly from Ferret,
 * ie. directly from Fortran, should be all lower case,
 * should pass by reference and should end with an underscore.
 */

/*
 * Find all of the ~.so files in directories listed in the
 * FER_EXTERNAL_FUNCTIONS environment variable and add all 
 * the names and associated directory information to the 
 * STATIC_ExternalFunctionList.
 */
int FORTRAN(efcn_scan)( int *gfcn_num_internal )
{
  FILE *file_ptr=NULL;
  ExternalFunction ef; 
 
  char file[EF_MAX_NAME_LENGTH]="";
  char *path_ptr=NULL, path[8192]="";
  char paths[8192]="", cmd[EF_MAX_DESCRIPTION_LENGTH]="";
  int count=0, status=LIST_OK;
  int i_intEF;

  static int return_val=0; /* static because it needs to exist after the return statement */
    
/*  ------------------------------------
 *  Count and list the names of internally linked EF's
 *  Lines with names are generated by the perl script 
 *  int_dlsym.pl.  Check that N_INTEF is correctly defined below.
 */

#define N_INTEF 164

struct {
  char funcname[EF_MAX_NAME_LENGTH];
} I_EFnames[N_INTEF];

   strcpy(I_EFnames[0].funcname, "ave_scat2grid_t");
   strcpy(I_EFnames[1].funcname, "bin_index_wt");
   strcpy(I_EFnames[2].funcname, "compressi");
   strcpy(I_EFnames[3].funcname, "compressi_by");
   strcpy(I_EFnames[4].funcname, "compressj");
   strcpy(I_EFnames[5].funcname, "compressj_by");
   strcpy(I_EFnames[6].funcname, "compressk");
   strcpy(I_EFnames[7].funcname, "compressk_by");
   strcpy(I_EFnames[8].funcname, "compressl");
   strcpy(I_EFnames[9].funcname, "compressl_by");
   strcpy(I_EFnames[10].funcname, "compressm");
   strcpy(I_EFnames[11].funcname, "compressm_by");
   strcpy(I_EFnames[12].funcname, "compressn");
   strcpy(I_EFnames[13].funcname, "compressn_by");
   strcpy(I_EFnames[14].funcname, "convolvei");
   strcpy(I_EFnames[15].funcname, "convolvej");
   strcpy(I_EFnames[16].funcname, "convolvek");
   strcpy(I_EFnames[17].funcname, "convolvel");
   strcpy(I_EFnames[18].funcname, "convolvem");
   strcpy(I_EFnames[19].funcname, "convolven");
   strcpy(I_EFnames[20].funcname, "curv_range");
   strcpy(I_EFnames[21].funcname, "curv_to_rect");
   strcpy(I_EFnames[22].funcname, "curv_to_rect_map");
   strcpy(I_EFnames[23].funcname, "date1900");
   strcpy(I_EFnames[24].funcname, "days1900toymdhms");
   strcpy(I_EFnames[25].funcname, "ecat");
   strcpy(I_EFnames[26].funcname, "ecat_str");
   strcpy(I_EFnames[27].funcname, "element_index");
   strcpy(I_EFnames[28].funcname, "element_index_str");
   strcpy(I_EFnames[29].funcname, "element_index_str_n");
   strcpy(I_EFnames[30].funcname, "eof_space");
   strcpy(I_EFnames[31].funcname, "eof_stat");
   strcpy(I_EFnames[32].funcname, "eof_tfunc");
   strcpy(I_EFnames[33].funcname, "ereverse");
   strcpy(I_EFnames[34].funcname, "expndi_by");
   strcpy(I_EFnames[35].funcname, "expndi_by_t");
   strcpy(I_EFnames[36].funcname, "expndi_by_z");
   strcpy(I_EFnames[37].funcname, "fcat");
   strcpy(I_EFnames[38].funcname, "fcat_str");
   strcpy(I_EFnames[39].funcname, "ffta");
   strcpy(I_EFnames[40].funcname, "fft_im");
   strcpy(I_EFnames[41].funcname, "fft_inverse");
   strcpy(I_EFnames[42].funcname, "fftp");
   strcpy(I_EFnames[43].funcname, "fft_re");
   strcpy(I_EFnames[44].funcname, "fill_xy");
   strcpy(I_EFnames[45].funcname, "findhi");
   strcpy(I_EFnames[46].funcname, "findlo");
   strcpy(I_EFnames[47].funcname, "floatstr");
   strcpy(I_EFnames[48].funcname, "freverse");
   strcpy(I_EFnames[49].funcname, "is_element_of");
   strcpy(I_EFnames[50].funcname, "is_element_of_str");
   strcpy(I_EFnames[51].funcname, "is_element_of_str_n");
   strcpy(I_EFnames[52].funcname, "labwid");
   strcpy(I_EFnames[53].funcname, "lanczos");
   strcpy(I_EFnames[54].funcname, "list_value_xml");
   strcpy(I_EFnames[55].funcname, "lsl_lowpass");
   strcpy(I_EFnames[56].funcname, "minmax");
   strcpy(I_EFnames[57].funcname, "minutes24");
   strcpy(I_EFnames[58].funcname, "nco");
   strcpy(I_EFnames[59].funcname, "nco_attr");
   strcpy(I_EFnames[60].funcname, "pt_in_poly");
   strcpy(I_EFnames[61].funcname, "rect_to_curv");
   strcpy(I_EFnames[62].funcname, "sampleij");
   strcpy(I_EFnames[63].funcname, "samplei_multi");
   strcpy(I_EFnames[64].funcname, "samplej_multi");
   strcpy(I_EFnames[65].funcname, "samplek_multi");
   strcpy(I_EFnames[66].funcname, "samplel_multi");
   strcpy(I_EFnames[67].funcname, "samplet_date");
   strcpy(I_EFnames[68].funcname, "samplexy");
   strcpy(I_EFnames[69].funcname, "samplexy_closest");
   strcpy(I_EFnames[70].funcname, "samplexy_curv");
   strcpy(I_EFnames[71].funcname, "samplexy_curv_avg");
   strcpy(I_EFnames[72].funcname, "samplexy_curv_nrst");
   strcpy(I_EFnames[73].funcname, "samplexyt");
   strcpy(I_EFnames[74].funcname, "samplexz");
   strcpy(I_EFnames[75].funcname, "sampleyz");
   strcpy(I_EFnames[76].funcname, "scat2ddups");
   strcpy(I_EFnames[77].funcname, "scat2grid_bin_xy");
   strcpy(I_EFnames[78].funcname, "scat2grid_bin_xyt");
   strcpy(I_EFnames[79].funcname, "scat2gridgauss_xt");
   strcpy(I_EFnames[80].funcname, "scat2gridgauss_xt_v0");
   strcpy(I_EFnames[81].funcname, "scat2gridgauss_xy");
   strcpy(I_EFnames[82].funcname, "scat2gridgauss_xy_v0");
   strcpy(I_EFnames[83].funcname, "scat2gridgauss_xz");
   strcpy(I_EFnames[84].funcname, "scat2gridgauss_xz_v0");
   strcpy(I_EFnames[85].funcname, "scat2gridgauss_yt");
   strcpy(I_EFnames[86].funcname, "scat2gridgauss_yt_v0");
   strcpy(I_EFnames[87].funcname, "scat2gridgauss_yz");
   strcpy(I_EFnames[88].funcname, "scat2gridgauss_yz_v0");
   strcpy(I_EFnames[89].funcname, "scat2gridgauss_zt");
   strcpy(I_EFnames[90].funcname, "scat2gridgauss_zt_v0");
   strcpy(I_EFnames[91].funcname, "scat2gridlaplace_xt");
   strcpy(I_EFnames[92].funcname, "scat2gridlaplace_xy");
   strcpy(I_EFnames[93].funcname, "scat2gridlaplace_xz");
   strcpy(I_EFnames[94].funcname, "scat2gridlaplace_yt");
   strcpy(I_EFnames[95].funcname, "scat2gridlaplace_yz");
   strcpy(I_EFnames[96].funcname, "scat2gridlaplace_zt");
   strcpy(I_EFnames[97].funcname, "scat2grid_nbin_xy");
   strcpy(I_EFnames[98].funcname, "scat2grid_nbin_xyt");
   strcpy(I_EFnames[99].funcname, "scat2grid_nobs_xy");
   strcpy(I_EFnames[100].funcname, "scat2grid_nobs_xyt");
   strcpy(I_EFnames[101].funcname, "scat2grid_t");
   strcpy(I_EFnames[102].funcname, "sorti");
   strcpy(I_EFnames[103].funcname, "sorti_str");
   strcpy(I_EFnames[104].funcname, "sortj");
   strcpy(I_EFnames[105].funcname, "sortj_str");
   strcpy(I_EFnames[106].funcname, "sortk");
   strcpy(I_EFnames[107].funcname, "sortk_str");
   strcpy(I_EFnames[108].funcname, "sortl");
   strcpy(I_EFnames[109].funcname, "sortl_str");
   strcpy(I_EFnames[110].funcname, "sortm");
   strcpy(I_EFnames[111].funcname, "sortm_str");
   strcpy(I_EFnames[112].funcname, "sortn");
   strcpy(I_EFnames[113].funcname, "sortn_str");
   strcpy(I_EFnames[114].funcname, "tauto_cor");
   strcpy(I_EFnames[115].funcname, "tax_datestring");
   strcpy(I_EFnames[116].funcname, "tax_day");
   strcpy(I_EFnames[117].funcname, "tax_dayfrac");
   strcpy(I_EFnames[118].funcname, "tax_jday");
   strcpy(I_EFnames[119].funcname, "tax_jday1900");
   strcpy(I_EFnames[120].funcname, "tax_month");
   strcpy(I_EFnames[121].funcname, "tax_times");
   strcpy(I_EFnames[122].funcname, "tax_tstep");
   strcpy(I_EFnames[123].funcname, "tax_units");
   strcpy(I_EFnames[124].funcname, "tax_year");
   strcpy(I_EFnames[125].funcname, "tax_yearfrac");
   strcpy(I_EFnames[126].funcname, "tcat");
   strcpy(I_EFnames[127].funcname, "tcat_str");
   strcpy(I_EFnames[128].funcname, "test_opendap");
   strcpy(I_EFnames[129].funcname, "transpose_xt");
   strcpy(I_EFnames[130].funcname, "transpose_xy");
   strcpy(I_EFnames[131].funcname, "transpose_xz");
   strcpy(I_EFnames[132].funcname, "transpose_yt");
   strcpy(I_EFnames[133].funcname, "transpose_yz");
   strcpy(I_EFnames[134].funcname, "transpose_zt");
   strcpy(I_EFnames[135].funcname, "treverse");
   strcpy(I_EFnames[136].funcname, "unique_str2int");
   strcpy(I_EFnames[137].funcname, "write_webrow");
   strcpy(I_EFnames[138].funcname, "xauto_cor");
   strcpy(I_EFnames[139].funcname, "xcat");
   strcpy(I_EFnames[140].funcname, "xcat_str");
   strcpy(I_EFnames[141].funcname, "xreverse");
   strcpy(I_EFnames[142].funcname, "ycat");
   strcpy(I_EFnames[143].funcname, "ycat_str");
   strcpy(I_EFnames[144].funcname, "yreverse");
   strcpy(I_EFnames[145].funcname, "zaxreplace_avg");
   strcpy(I_EFnames[146].funcname, "zaxreplace_bin");
   strcpy(I_EFnames[147].funcname, "zaxreplace_rev");
   strcpy(I_EFnames[148].funcname, "zaxreplace_zlev");
   strcpy(I_EFnames[149].funcname, "zcat");
   strcpy(I_EFnames[150].funcname, "zcat_str");
   strcpy(I_EFnames[151].funcname, "zreverse");
   strcpy(I_EFnames[152].funcname, "eofsvd_space");
   strcpy(I_EFnames[153].funcname, "eofsvd_stat");
   strcpy(I_EFnames[154].funcname, "eofsvd_tfunc");
   strcpy(I_EFnames[155].funcname, "expnd_by_len");
   strcpy(I_EFnames[156].funcname, "expnd_by_len_str");
   strcpy(I_EFnames[157].funcname, "fc_isubset");
   strcpy(I_EFnames[158].funcname, "expndi_by_z_counts");
   strcpy(I_EFnames[159].funcname, "expndi_id_by_z_counts");
   strcpy(I_EFnames[160].funcname, "expndi_by_m_counts");
   strcpy(I_EFnames[161].funcname, "str_mask");
   strcpy(I_EFnames[162].funcname, "samplexyt_nrst");
   strcpy(I_EFnames[163].funcname, "separate");

/*    
 *  ------------------------------------ 
 */



  if ( I_have_scanned_already ) {
    return_val = list_size(STATIC_ExternalFunctionList);
    return return_val;
  }

  if ( (STATIC_ExternalFunctionList = list_init()) == NULL ) {
    fputs("**ERROR: efcn_scan: Unable to initialize STATIC_ExternalFunctionList.\n", stderr);
    return_val = -1;
    return return_val;
  }

  /*
   * Open $FER_LIBS/ferret_ef_mem_subsc.so with RTLD_GLOBAL flag to create
   * the external copy of the FERRET_EF_MEM_SUBSC common block.
   */
  path_ptr = getenv("FER_LIBS");
  if ( path_ptr == NULL ) {
     fputs("**ERROR: efcn_scan: FER_LIBS is not defined\n", stderr);
     return_val = -1;
     return return_val;
  }
  sprintf(path, "%s/ferret_ef_mem_subsc.so", path_ptr);
  ferret_ef_mem_subsc_so_ptr = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
  if ( ferret_ef_mem_subsc_so_ptr == NULL ) {
     /*
      * fprintf(stderr, "**ERROR: efcn_scan: dlopen of %s\n"
      *                 "  failed -- %s\n", path, dlerror());
      * return_val = -1;
      * return return_val;
      */
     copy_ferret_ef_mem_subsc_ptr = NULL;
  }
  else {
     copy_ferret_ef_mem_subsc_ptr = 
             (void (*)(void)) dlsym(ferret_ef_mem_subsc_so_ptr,
                                    "copy_ferret_ef_mem_subsc_");
     if ( copy_ferret_ef_mem_subsc_ptr == NULL ) {
        fprintf(stderr, "**ERROR: efcn_scan: copy_ferret_ef_mem_subsc_\n"
                        "  not found in $FER_LIBS/ferret_ef_mem_subsc.so\n"
                        "  -- %s\n", dlerror());
        return_val = -1;
        return return_val;
     }
  }

  /*
   * Get internally linked external functions;  and add all 
   * the names and associated directory information to the 
   * STATIC_ExternalFunctionList.
   */


  /*
   * Read a name at a time.
   */

      for (i_intEF = 0; i_intEF < N_INTEF;   i_intEF = i_intEF + 1 ) {
	      strcpy(ef.path, "internally_linked");
	      strcpy(ef.name, I_EFnames[i_intEF].funcname);
	      ef.id = *gfcn_num_internal + ++count; /* pre-increment because F arrays start at 1 */
	      ef.already_have_internals = NO;
	      ef.internals_ptr = NULL;
	      list_insert_after(STATIC_ExternalFunctionList, (char *) &ef, sizeof(ExternalFunction));

      }

  /*
   * - Get all the paths from the "FER_EXTERNAL_FUNCTIONS" environment variable.
   *
   * - While there is another path:
   *    - get the path;
   *    - create a pipe for the "ls -1" command;
   *    - read stdout and use each file name to create another external function entry;
   *
   */

  if ( !getenv("FER_EXTERNAL_FUNCTIONS") ) {
    if ( !I_have_warned_already ) {
      fprintf(stderr, "\n"
                      "WARNING: environment variable FER_EXTERNAL_FUNCTIONS not defined.\n\n");
      I_have_warned_already = TRUE;
    }
    /* *kob* v5.32 - the return val was set to 0 below but that was wrong. 
       That didn't take into account that on any system, the 
       FER_EXTERNAL_FUNCTIONS env variable might not be set.  If that were the
       case, a core dump occurred on all systems.  Set return_val to count, 
       which was generated above - also have to  note that the ef's 
       have been scanned*/
    return_val = count; 
    I_have_scanned_already = TRUE;
    return return_val;
  }

  sprintf(paths, "%s", getenv("FER_EXTERNAL_FUNCTIONS"));
    
  path_ptr = strtok(paths, " \t");

  if ( path_ptr == NULL ) {
 
    fprintf(stderr, "\n"
                    "WARNING:No paths were found in the environment variable FER_EXTERNAL_FUNCTIONS.\n\n");

    return_val = 0;
    return return_val;
 
  } else {
    
    do {

	  strcpy(path, path_ptr);

      if (path[strlen(path)-1] != '/')
        strcat(path, "/"); 

      sprintf(cmd, "ls -1 %s", path);

      /* Open a pipe to the "ls" command */
      if ( (file_ptr = popen(cmd, "r")) == (FILE *) NULL ) {
	    fputs("**ERROR: Cannot open pipe.\n", stderr);
	    return_val = -1;
	    return return_val;
      }
 
      /*
       * Read a line at a time.
       * Any ~.so files are assumed to be external functions.
       */
      while ( fgets(file, EF_MAX_NAME_LENGTH, file_ptr) != NULL ) {

        char *extension;

	    file[strlen(file)-1] = '\0';   /* chop off the carriage return */
	    extension = &file[strlen(file)-3];
	    if ( strcmp(extension, ".so") == 0 ) {
          file[strlen(file)-3] = '\0'; /* chop off the ".so" */
	      strcpy(ef.path, path);
	      strcpy(ef.name, file);
	      ef.id = *gfcn_num_internal + ++count; /* pre-increment because F arrays start at 1 */
	      ef.already_have_internals = NO;
	      ef.internals_ptr = NULL;
	      list_insert_after(STATIC_ExternalFunctionList, (char *) &ef, sizeof(ExternalFunction));
	    }

      }
 
      pclose(file_ptr);
 
      path_ptr = strtok(NULL, " \t"); /* get the next directory */
 
    } while ( path_ptr != NULL );

    I_have_scanned_already = TRUE;
  }

  return_val = count;
  return return_val;

}


/*
 * Determine whether an external function has already 
 * had its internals read.
 */
int FORTRAN(efcn_already_have_internals)( int *id_ptr )
{
  ExternalFunction *ef_ptr;
  int return_val;

  ef_ptr = ef_ptr_from_id_ptr(id_ptr);
  if ( ef_ptr == NULL ) {
     return 0;
  }

  return_val = ef_ptr->already_have_internals;
  return return_val;
}



/*
 * Create a new python-backed external function.  The initialization of
 * this function is done at this time to ensure that the python module is
 * valid and contains suitable functions.  Initialization is accomplished
 * using generic wrapper functions.
 * Input arguments:
 *    fname - name for the function
 *    lenfname - actual length of the name in fname
 *    pymod - name of the python module suitable for a python import statement
 *            (eg, "package.subpackage.module")
 *    lenpymod - actual length of the name in pymod
 * Output arguments:
 *    errstring - error message if something went wrong
 *    lenerrstring - actual length of the string returned in errstring
 * The value of lenerrstring will be zero if and only if there were no errors
 *
 * Note: this function assume Hollerith strings are passed as character arrays
 *       (and max lengths appended as ints to the end of the argument list -
 *        they are not listed here since unused; also permits saying the strings 
 *        are simple arrays in Fortran)
 */
void FORTRAN(create_pyefcn)(char fname[], int *lenfname, char pymod[], int *lenpymod,
                            char errstring[], int *lenerrstring)
{
    ExternalFunction ef; 
    ExternalFunction *ef_ptr; 
    char libname[1024];

    /* Check string lengths since these values might possibly be exceeded */
    if ( *lenpymod >= EF_MAX_DESCRIPTION_LENGTH ) {
        sprintf(errstring, "Module name too long (must be less than %d characters)", EF_MAX_DESCRIPTION_LENGTH);
        *lenerrstring = strlen(errstring);
        return;
    }
    if ( *lenfname >= EF_MAX_NAME_LENGTH ) {
        sprintf(errstring, "Function name too long (must be less than %d characters)", EF_MAX_NAME_LENGTH);
        *lenerrstring = strlen(errstring);
        return;
    }

    /* 
     * Assign the local ExternalFunction structure, assigning the module name to the path element
     * Get the ID for this new function by adding one to the ID of the last element in the list.
     * (The IDs do not match the size of the list.)
     */
    ef.handle = NULL;
    ef_ptr = (ExternalFunction *) list_rear(STATIC_ExternalFunctionList);
    ef.id = ef_ptr->id + 1;
    strncpy(ef.name, fname, *lenfname);
    ef.name[*lenfname] = '\0';
    strncpy(ef.path, pymod, *lenpymod);
    ef.path[*lenpymod] = '\0';
    ef.already_have_internals = FALSE;
    ef.internals_ptr = NULL;

    /* Add a copy of this ExternalFunction to the end of the global list of external functions */
    list_mvrear(STATIC_ExternalFunctionList);
    ef_ptr = (ExternalFunction *)list_insert_after(STATIC_ExternalFunctionList, (char *) &ef, sizeof(ExternalFunction));

    /* Allocate and initialize the internals data for this ExternalFunction in the list */
    if ( EF_New(ef_ptr) != 0 ) {
        strcpy(errstring, "Unable to allocate memory for the internals data in create_pyefcn");
        *lenerrstring = strlen(errstring);
        return;
    }
    ef_ptr->internals_ptr->language = EF_PYTHON;
    ef_ptr->already_have_internals = TRUE;

    /*
     * Prepare for bailout possibilities by setting a signal handler for
     * SIGFPE, SIGSEGV, SIGINT and SIGBUS and then by cacheing the stack 
     * environment with sigsetjmp (for the signal handler) and setjmp 
     * (for the "bail out" utility function).
     */   
    if ( EF_Util_setsig("create_pyefcn")) {
        list_remove_rear(STATIC_ExternalFunctionList);
        free(ef_ptr->internals_ptr);
        free(ef_ptr);
        strcpy(errstring, "Unable to set signal handlers in create_pyefcn");
        *lenerrstring = strlen(errstring);
        return;
    }
    if (sigsetjmp(sigjumpbuffer, 1) != 0) {
        list_remove_rear(STATIC_ExternalFunctionList);
        free(ef_ptr->internals_ptr);
        free(ef_ptr);
        strcpy(errstring, "Signal caught in create_pyefcn");
        *lenerrstring = strlen(errstring);
        return;
    }
    if (setjmp(jumpbuffer) != 0) {
        list_remove_rear(STATIC_ExternalFunctionList);
        free(ef_ptr->internals_ptr);
        free(ef_ptr);
        strcpy(errstring, "ef_bail_out called in create_pyefcn");
        *lenerrstring = strlen(errstring);
        return;
    }
    canjump = 1;

    pyefcn_init(ef_ptr->id, ef_ptr->path, errstring);

    /* Restore the old signal handlers. */
    EF_Util_ressig("create_pyefcn");

    *lenerrstring = strlen(errstring);
    if ( *lenerrstring > 0 ) {
        list_remove_rear(STATIC_ExternalFunctionList);
        free(ef_ptr->internals_ptr);
        free(ef_ptr);
    }
    return;
}



/*
 * Find an external function based on its integer ID and
 * gather information describing the function. 
 *
 * Return values:
 *     -1: error occurred, dynamic linking was unsuccessful
 *      0: success
 */
int FORTRAN(efcn_gather_info)( int *id_ptr )
{
  ExternalFunction *ef_ptr;
  int internally_linked;
  char tempText[1024];
  ExternalFunctionInternals *i_ptr;
  void (*f_init_ptr)(int *);

   /*
    * Find the external function.
    */
   ef_ptr = ef_ptr_from_id_ptr(id_ptr);
   if ( ef_ptr == NULL ) {
      fprintf(stderr, "**ERROR: No external function of id %d was found.\n", *id_ptr);
      return -1;
   }
   /* Check if this has already been done */
   if (ef_ptr->already_have_internals)  {
      return 0;
   }
   /* Check if this is an internal function */
   if ( strcmp(ef_ptr->path,"internally_linked") == 0 )
      internally_linked = TRUE;
   else
      internally_linked = FALSE;

   /* Get a handle for the shared object if not internally linked */
   if ( ! internally_linked ) {
      strcat(tempText, ef_ptr->path);
      strcat(tempText, ef_ptr->name);
      strcat(tempText, ".so");

      if ( (ef_ptr->handle = dlopen(tempText, RTLD_LAZY)) == NULL ) {
         fprintf(stderr, "**ERROR in External Function %s:\n"
                         "  Dynamic linking call dlopen() returns --\n"
                         "  \"%s\".\n", ef_ptr->name, dlerror());
         return -1;
      }
   }

   /* Allocate and default initialize the internal information. */
   if ( EF_New(ef_ptr) != 0 )
      return -1;

   /* Call the external function initialization routine */
   i_ptr = ef_ptr->internals_ptr;

   if ( i_ptr->language == EF_F ) {

      /*
       * Prepare for bailout possibilities by setting a signal handler for
       * SIGFPE, SIGSEGV, SIGINT and SIGBUS and then by cacheing the stack 
       * environment with sigsetjmp (for the signal handler) and setjmp 
       * (for the "bail out" utility function).
       */   
      if ( EF_Util_setsig("efcn_gather_info")) {
         return -1;
      }

      /* Set the signal return location and process jumps */
      if ( sigsetjmp(sigjumpbuffer, 1) != 0 ) {
         /* Must have come from bailing out */
         return -1;
      }
      /* Set the bail out return location and process jumps */
      if ( setjmp(jumpbuffer) != 0 ) {
         /* Must have come from bailing out */
         return -1;
      }
      canjump = 1;

      /* Get the pointer to external function initialization routine */
      sprintf(tempText, "%s_init_", ef_ptr->name);
      if ( ! internally_linked ) {
         f_init_ptr = (void (*)(int *))dlsym(ef_ptr->handle, tempText);
      } else {
         f_init_ptr = (void (*)(int *))internal_dlsym(tempText);
      }
      if ( f_init_ptr == NULL ) {
         fprintf(stderr, "**ERROR in efcn_gather_info(): %s is not found.\n", tempText);
         if ( ! internally_linked )
            fprintf(stderr, "  dlerror: \"%s\"\n", dlerror());
         EF_Util_ressig("efcn_gather_info");
         return -1;
      }

      /*
       * Call the initialization routine.  If it bails out,
       * this will jump back to one of the setjmp methods, returning non-zero.
       */
      (*f_init_ptr)(id_ptr);
      ef_ptr->already_have_internals = TRUE;

      /* Restore the old signal handlers. */
      if ( EF_Util_ressig("efcn_gather_info") ) {
         return -1;
      }

   }
   else {
      /* Note: Python-backed external functions get initialized when added, so no support here for them */
      fprintf(stderr, "**ERROR: unsupported language (%d) for efcn_gather_info.\n", i_ptr->language);
      return -1;
   }

   return 0;
}


/*
 * Find an external function based on its integer ID, 
 * Query the function about custom axes. Store the context
 * list information for use by utility functions.
 */
void FORTRAN(efcn_get_custom_axes)( int *id_ptr, int *cx_list_ptr, int *status )
{
  ExternalFunction *ef_ptr=NULL;
  char tempText[EF_MAX_NAME_LENGTH]="";
  int internally_linked = FALSE;

  void (*fptr)(int *);

  /*
   * Initialize the status
   */
  *status = FERR_OK;

  /*
   * Store the context list globally.
   */
  EF_store_globals(NULL, NULL, cx_list_ptr, NULL, NULL);

  /*
   * Find the external function.
   */
  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }
  if ( (!strcmp(ef_ptr->path,"internally_linked")) ) {internally_linked = TRUE; }

  if ( ef_ptr->internals_ptr->language == EF_F ) {

    /*
     * Prepare for bailout possibilities by setting a signal handler for
     * SIGFPE, SIGSEGV, SIGINT and SIGBUS and then by cacheing the stack 
     * environment with sigsetjmp (for the signal handler) and setjmp 
     * (for the "bail out" utility function).
     */   

    if (EF_Util_setsig("efcn_get_custom_axes")) {
      *status = FERR_EF_ERROR;
       return;
    }

    /*
     * Set the signal return location and process jumps
     */
    if (sigsetjmp(sigjumpbuffer, 1) != 0) {
      *status = FERR_EF_ERROR;
      return;
    }

    /*
     * Set the bail out return location and process jumps
     */
    if (setjmp(jumpbuffer) != 0) {
      *status = FERR_EF_ERROR;
      return;
    } 
   
    canjump = 1;

    sprintf(tempText, "");
    strcat(tempText, ef_ptr->name);
    strcat(tempText, "_custom_axes_");

    if (!internally_linked) {
       fptr  = (void (*)(int *))dlsym(ef_ptr->handle, tempText);
    } else {
      fptr  = (void (*)(int *))internal_dlsym(tempText);
    } 
    (*fptr)( id_ptr );


    /*
     * Restore the old signal handlers.
     */
    if ( EF_Util_ressig("efcn_get_custom_axes")) {
       return;
    }

    /* end of EF_F */
  }
  else if ( ef_ptr->internals_ptr->language == EF_PYTHON ) {
      char errstring[2048];

      /*
       * Prepare for bailout possibilities by setting a signal handler for
       * SIGFPE, SIGSEGV, SIGINT and SIGBUS and then by cacheing the stack 
       * environment with sigsetjmp (for the signal handler) and setjmp 
       * (for the "bail out" utility function).
       */   
      if ( EF_Util_setsig("efcn_get_custom_axes")) {
          *status = FERR_EF_ERROR;
           return;
      }
      if (sigsetjmp(sigjumpbuffer, 1) != 0) {
          *status = FERR_EF_ERROR;
          return;
      }
      if (setjmp(jumpbuffer) != 0) {
          *status = FERR_EF_ERROR;
          return;
      }
      canjump = 1;

      /* Call pyefcn_custom_axes which in turn calls the ferret_custom_axes method in the python module */
      pyefcn_custom_axes(*id_ptr, ef_ptr->path, errstring);
      if ( strlen(errstring) > 0 ) {
          /* (In effect) call ef_bail_out_ to process the error in a standard way */
          ef_err_bail_out_(id_ptr, errstring);
          /* Should never return - instead jumps to setjmp() returning 1 */
      }

      /* Restore the old signal handlers. */
      EF_Util_ressig("efcn_get_custom_axes");

      /* end of EF_PYTHON */
  }
  else {
    *status = FERR_EF_ERROR;
    fprintf(stderr, "**ERROR: unsupported language (%d) for efcn_get_custom_axes.\n", ef_ptr->internals_ptr->language);
  }

  return;
}


/*
 * Find an external function based on its integer ID, 
 * Query the function about abstract axes. Pass memory,
 * mr_list and cx_list info into the external function.
 */
void FORTRAN(efcn_get_result_limits)( int *id_ptr, DFTYPE *memory, int *mr_list_ptr, int *cx_list_ptr, int *status )
{
  ExternalFunction *ef_ptr=NULL;
  char tempText[EF_MAX_NAME_LENGTH]="";
  int internally_linked = FALSE;

  void (*fptr)(int *);

  /*
   * Initialize the status
   */
  *status = FERR_OK;

  /*
   * Store the memory pointer and various lists globally.
   */
  EF_store_globals(memory, mr_list_ptr, cx_list_ptr, NULL, NULL);

  /*
   * Find the external function.
   */

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }

  if ( (!strcmp(ef_ptr->path,"internally_linked")) ) {internally_linked = TRUE; }

  if ( ef_ptr->internals_ptr->language == EF_F ) {

    /*
     * Prepare for bailout possibilities by setting a signal handler for
     * SIGFPE, SIGSEGV, SIGINT and SIGBUS and then by cacheing the stack 
     * environment with sigsetjmp (for the signal handler) and setjmp 
     * (for the "bail out" utility function).
     */   


    if ( EF_Util_setsig("efcn_get_result_limits")) {
      *status = FERR_EF_ERROR;
       return;
    }

    /*
     * Set the signal return location and process jumps
     */
    if (sigsetjmp(sigjumpbuffer, 1) != 0) {
      *status = FERR_EF_ERROR;
      return;
    }

    /*
     * Set the bail out return location and process jumps
     */
    if (setjmp(jumpbuffer) != 0) {
      *status = FERR_EF_ERROR;
      return;
    }

    canjump = 1;


    sprintf(tempText, "");
    strcat(tempText, ef_ptr->name);
    strcat(tempText, "_result_limits_");

    if (!internally_linked) {
      fptr  = (void (*)(int *))dlsym(ef_ptr->handle, tempText);
    } else {
      fptr  = (void (*)(int *))internal_dlsym(tempText);
    }

    (*fptr)( id_ptr);

    /*
     * Restore the old signal handlers.
     */
    if ( EF_Util_ressig("efcn_get_result_limits")) {
       return;
    }

    /* end of EF_F */
  }
  else if ( ef_ptr->internals_ptr->language == EF_PYTHON ) {
      char errstring[2048];

      /*
       * Prepare for bailout possibilities by setting a signal handler for
       * SIGFPE, SIGSEGV, SIGINT and SIGBUS and then by cacheing the stack 
       * environment with sigsetjmp (for the signal handler) and setjmp 
       * (for the "bail out" utility function).
       */   
      if ( EF_Util_setsig("efcn_get_result_limits")) {
          *status = FERR_EF_ERROR;
           return;
      }
      if (sigsetjmp(sigjumpbuffer, 1) != 0) {
          *status = FERR_EF_ERROR;
          return;
      }
      if (setjmp(jumpbuffer) != 0) {
          *status = FERR_EF_ERROR;
          return;
      }
      canjump = 1;

      /* Call pyefcn_result_limits which in turn calls the ferret_result_limits method in the python module */
      pyefcn_result_limits(*id_ptr, ef_ptr->path, errstring);
      if ( strlen(errstring) > 0 ) {
          /* (In effect) call ef_bail_out_ to process the error in a standard way */
          ef_err_bail_out_(id_ptr, errstring);
          /* Should never return - instead jumps to setjmp() returning 1 */
      }

      /* Restore the old signal handlers. */
      EF_Util_ressig("efcn_get_result_limits");

      /* end of EF_PYTHON */
  }
  else {
    *status = FERR_EF_ERROR;
    fprintf(stderr, "**ERROR: unsupported language (%d) for efcn_get_result_limits.\n", ef_ptr->internals_ptr->language);
  }

  return;
}


/*
 * Find an external function based on its integer ID, 
 * pass the necessary information and the data and tell
 * the function to calculate the result.
 */
void FORTRAN(efcn_compute)( int *id_ptr, int *narg_ptr, int *cx_list_ptr, int *mr_list_ptr, int *mres_ptr,
	DFTYPE *bad_flag_ptr, int *mr_arg_offset_ptr, DFTYPE *memory, int *status )
{
  ExternalFunction *ef_ptr=NULL;
  ExternalFunctionInternals *i_ptr=NULL;
  DFTYPE *arg_ptr[EF_MAX_COMPUTE_ARGS];
  int xyzt=0, i=0, j=0;
  int size=0;
  char tempText[EF_MAX_NAME_LENGTH]="";
  int internally_linked = FALSE;

  /*
   * Prototype all the functions needed for varying numbers of
   * arguments and work arrays.
   */

  void (*fptr)(int *);
  void (*f1arg)(int *, DFTYPE *, DFTYPE *);
  void (*f2arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *);
  void (*f3arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);
  void (*f4arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);
  void (*f5arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);
  void (*f6arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
		DFTYPE *);
  void (*f7arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
		DFTYPE *, DFTYPE *);
  void (*f8arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
		DFTYPE *, DFTYPE *, DFTYPE *);
  void (*f9arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
		DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);
  void (*f10arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
		DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);
  void (*f11arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
		DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);
  void (*f12arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
		DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);
  void (*f13arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
		DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);
  void (*f14arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
		DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
        DFTYPE *);
  void (*f15arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
		DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
        DFTYPE *, DFTYPE *);
  void (*f16arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
		DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
        DFTYPE *, DFTYPE *, DFTYPE *);
  void (*f17arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
		DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
        DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);
  void (*f18arg)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
		DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
        DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *);

  /*
   * Initialize the status
   */
  *status = FERR_OK;

  /*
   * Store the array dimensions for memory resident variables and for working storage.
   * Store the memory pointer and various lists globally.
   */
  FORTRAN(efcn_copy_array_dims)();
  EF_store_globals(memory, mr_list_ptr, cx_list_ptr, mres_ptr, bad_flag_ptr);

  /*
   * Find the external function.
   */
  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) {
    fprintf(stderr, "**ERROR in efcn_compute() finding external function: id = [%d]\n", *id_ptr);
    *status = FERR_EF_ERROR;
    return;
  }
  if ( (!strcmp(ef_ptr->path,"internally_linked")) ) {internally_linked = TRUE; }

  i_ptr = ef_ptr->internals_ptr;

  if ( i_ptr->language == EF_F ) {
    /*
     * Begin assigning the arg_ptrs.
     */

    /* First come the arguments to the function. */

     for (i=0; i<i_ptr->num_reqd_args; i++) {
       arg_ptr[i] = memory + mr_arg_offset_ptr[i];
     }

    /* Now for the result */

     arg_ptr[i++] = memory + mr_arg_offset_ptr[EF_MAX_ARGS];

    /* Now for the work arrays */

    /*
     * If this program has requested working storage we need to 
     * ask the function to specify the amount of space needed
     * and then create the memory here.  Memory will be released
     * after the external function returns.
     */
    if (i_ptr->num_work_arrays > EF_MAX_WORK_ARRAYS) {

	  fprintf(stderr, "**ERROR specifying number of work arrays in ~_init subroutine of external function %s\n"
                          "\tnum_work_arrays[=%d] exceeds maximum[=%d].\n\n",
                          ef_ptr->name, i_ptr->num_work_arrays, EF_MAX_WORK_ARRAYS);
	  *status = FERR_EF_ERROR;
	  return;

    } else if (i_ptr->num_work_arrays < 0) {

	  fprintf(stderr, "**ERROR specifying number of work arrays in ~_init subroutine of external function %s\n"
                          "\tnum_work_arrays[=%d] must be a positive number.\n\n",
                          ef_ptr->name, i_ptr->num_work_arrays);
	  *status = FERR_EF_ERROR;
	  return;

    } else if (i_ptr->num_work_arrays > 0)  {

      sprintf(tempText, "");
      strcat(tempText, ef_ptr->name);
      strcat(tempText, "_work_size_");

      if (!internally_linked) {
         fptr = (void (*)(int *))dlsym(ef_ptr->handle, tempText);
      } else {
         fptr  = (void (*)(int *))internal_dlsym(tempText);
      }

      if (fptr == NULL) {
	fprintf(stderr, "**ERROR in efcn_compute() accessing %s\n", tempText);
	*status = FERR_EF_ERROR;
        return;
      }
      (*fptr)( id_ptr );


	  /* Allocate memory for each individual work array */

      for (j=0; j<i_ptr->num_work_arrays; i++, j++) {

        int iarray, xlo, ylo, zlo, tlo, elo, flo,
                    xhi, yhi, zhi, thi, ehi, fhi;
        iarray = j+1;
        xlo = i_ptr->work_array_lo[j][0];
        ylo = i_ptr->work_array_lo[j][1];
        zlo = i_ptr->work_array_lo[j][2];
        tlo = i_ptr->work_array_lo[j][3];
        elo = i_ptr->work_array_lo[j][4];
        flo = i_ptr->work_array_lo[j][5];
        xhi = i_ptr->work_array_hi[j][0];
        yhi = i_ptr->work_array_hi[j][1];
        zhi = i_ptr->work_array_hi[j][2];
        thi = i_ptr->work_array_hi[j][3];
        ehi = i_ptr->work_array_hi[j][4];
        fhi = i_ptr->work_array_hi[j][5];

        FORTRAN(efcn_set_work_array_dims)(&iarray, &xlo, &ylo, &zlo, &tlo, &elo, &flo,
                                                   &xhi, &yhi, &zhi, &thi, &ehi, &fhi);

        size = sizeof(DFTYPE) * (xhi-xlo+1) * (yhi-ylo+1) * (zhi-zlo+1) 
                              * (thi-tlo+1) * (ehi-elo+1) * (fhi-flo+1);

        arg_ptr[i] = (DFTYPE *)malloc(size);
        if ( arg_ptr[i] == NULL ) { 
          fprintf(stderr, "**ERROR in efcn_compute() allocating %d bytes of memory\n"
                          "\twork array %d:  X=%d:%d, Y=%d:%d, Z=%d:%d, T=%d:%d, E=%d:%d, F=%d:%d\n", 
                          size, iarray, xlo, xhi, ylo, yhi, zlo, zhi, tlo, thi, elo, ehi, flo, fhi);
	  *status = FERR_EF_ERROR;
	  return;
        }
      }

    }

    /*
     * Copy the contents of Ferret's internal copy of the common block
     * FERRET_EF_MEM_SUBSC to the external copy of this same common
     * block using load_ferret_ef_mem_subsc_ in libferret_ef_mem_subsc.so
     * Because libferret_ef_mem_subsc.so was loaded with RTLD_GLOBAL,
     * this external copy of the common block will be seen by other
     * Ferret Fotran external functions in shared-object libraries.
     */
    (*copy_ferret_ef_mem_subsc_ptr)();

    /*
     * Prepare for bailout possibilities by setting a signal handler for
     * SIGFPE, SIGSEGV, SIGINT and SIGBUS and then by cacheing the stack 
     * environment with sigsetjmp (for the signal handler) and setjmp 
     * (for the "bail out" utility function).
     */   

    if ( EF_Util_setsig("efcn_compute")) {
      *status = FERR_EF_ERROR;
      return;
    }

    /*
     * Set the signal return location and process jumps
     */
    if (sigsetjmp(sigjumpbuffer, 1) != 0) {
      *status = FERR_EF_ERROR;
      return;
    }

    /*
     * Set the bail out return location and process jumps
     */
    if (setjmp(jumpbuffer) != 0) {
      *status = FERR_EF_ERROR;
      return;
    }

    canjump = 1;


    /*
     * Now go ahead and call the external function's "_compute_" function,
     * prototyping it for the number of arguments expected.
     */
    sprintf(tempText, "");
    strcat(tempText, ef_ptr->name);
    strcat(tempText, "_compute_");

    switch ( i_ptr->num_reqd_args + i_ptr->num_work_arrays ) {

    case 1:
	  if (!internally_linked) {
            f1arg  = (void (*)(int *, DFTYPE *, DFTYPE *))
             dlsym(ef_ptr->handle, tempText);
          } else {
	    f1arg  = (void (*)(int *, DFTYPE *, DFTYPE *))
             internal_dlsym(tempText);
          }
	  (*f1arg)( id_ptr, arg_ptr[0], arg_ptr[1] );
	break;


    case 2:
	  if (!internally_linked) {
            f2arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *))
             dlsym(ef_ptr->handle, tempText);
          } else {
            f2arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *))
             internal_dlsym(tempText);
          }
	  (*f2arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2] );
	break;


    case 3:
	  if (!internally_linked) {
	     f3arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))
              dlsym(ef_ptr->handle, tempText);
          } else {
	     f3arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))
              internal_dlsym(tempText);
          }
	  (*f3arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2], arg_ptr[3] );
	break;


    case 4:
	  if (!internally_linked) {
            f4arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))
             dlsym(ef_ptr->handle, tempText);
          } else {
            f4arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))
             internal_dlsym(tempText);
          }
	  (*f4arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2], arg_ptr[3], arg_ptr[4] );
	break;


    case 5:
	  if (!internally_linked) {
	    f5arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
             DFTYPE *))
             dlsym(ef_ptr->handle, tempText);
          } else {
	    f5arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, 
             DFTYPE *))
             internal_dlsym(tempText);
          }
	  (*f5arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2], arg_ptr[3], arg_ptr[4],
        arg_ptr[5] );
	break;


    case 6:
	  if (!internally_linked) {
	    f6arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *))dlsym(ef_ptr->handle, tempText);
          } else {
	    f6arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *))internal_dlsym(tempText);
          }
	  (*f6arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2], arg_ptr[3], arg_ptr[4],
        arg_ptr[5], arg_ptr[6] );
	break;


    case 7:
	  if (!internally_linked) {
	    f7arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *))dlsym(ef_ptr->handle, tempText);
          } else {
	    f7arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *))internal_dlsym(tempText);
          }
	  (*f7arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2], arg_ptr[3], arg_ptr[4],
        arg_ptr[5], arg_ptr[6], arg_ptr[7] );
	break;


    case 8:
	  if (!internally_linked) {
	    f8arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *))dlsym(ef_ptr->handle, tempText);
          } else {
	    f8arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *))internal_dlsym(tempText);
          }
	  (*f8arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2], arg_ptr[3], arg_ptr[4],
        arg_ptr[5], arg_ptr[6], arg_ptr[7], arg_ptr[8] );
	break;


    case 9:
	  if (!internally_linked) {
            f9arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))dlsym(ef_ptr->handle, tempText);
          } else {
            f9arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))internal_dlsym(tempText);
          }
	  (*f9arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2], arg_ptr[3], arg_ptr[4],
        arg_ptr[5], arg_ptr[6], arg_ptr[7], arg_ptr[8], arg_ptr[9] );
	break;


    case 10:
	  if (!internally_linked) {
	    f10arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))dlsym(ef_ptr->handle, tempText);
          } else {
	    f10arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))internal_dlsym(tempText);
          }
	  (*f10arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2], arg_ptr[3], arg_ptr[4],
        arg_ptr[5], arg_ptr[6], arg_ptr[7], arg_ptr[8], arg_ptr[9], arg_ptr[10] );
	break;


    case 11:
	  if (!internally_linked) {
            f11arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))
             dlsym(ef_ptr->handle, tempText);
          } else {
            f11arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))
             internal_dlsym(tempText);
          }
	  (*f11arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2], arg_ptr[3], arg_ptr[4],
        arg_ptr[5], arg_ptr[6], arg_ptr[7], arg_ptr[8], arg_ptr[9], arg_ptr[10],
        arg_ptr[11] );
	break;


    case 12:
	  if (!internally_linked) {
	    f12arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))
             dlsym(ef_ptr->handle, tempText);
          } else {
	    f12arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))
             internal_dlsym(tempText);
          }
	  (*f12arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2], arg_ptr[3], arg_ptr[4],
        arg_ptr[5], arg_ptr[6], arg_ptr[7], arg_ptr[8], arg_ptr[9], arg_ptr[10],
        arg_ptr[11], arg_ptr[12] );
	break;


    case 13:
	  if (!internally_linked) {
	    f13arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))
             dlsym(ef_ptr->handle, tempText);
          } else {
	    f13arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))
             internal_dlsym(tempText);
          }
	  (*f13arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2], arg_ptr[3], arg_ptr[4],
        arg_ptr[5], arg_ptr[6], arg_ptr[7], arg_ptr[8], arg_ptr[9], arg_ptr[10],
        arg_ptr[11], arg_ptr[12], arg_ptr[13] );
	break;


    case 14:
	  if (!internally_linked) {
	    f14arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *))dlsym(ef_ptr->handle, tempText);
          } else {
	    f14arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *))internal_dlsym(tempText);
          }
	  (*f14arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2], arg_ptr[3], arg_ptr[4],
        arg_ptr[5], arg_ptr[6], arg_ptr[7], arg_ptr[8], arg_ptr[9], arg_ptr[10],
        arg_ptr[11], arg_ptr[12], arg_ptr[13], arg_ptr[14] );
	break;


    case 15:
	  if (!internally_linked) {
	   f15arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
            DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
            DFTYPE *, DFTYPE *))dlsym(ef_ptr->handle, tempText);
          } else {
	   f15arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
            DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
            DFTYPE *, DFTYPE *))internal_dlsym(tempText);
          }
	  (*f15arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2], arg_ptr[3], arg_ptr[4],
        arg_ptr[5], arg_ptr[6], arg_ptr[7], arg_ptr[8], arg_ptr[9], arg_ptr[10],
        arg_ptr[11], arg_ptr[12], arg_ptr[13], arg_ptr[14], arg_ptr[15] );
	break;


    case 16:
	  if (!internally_linked) {
	    f16arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *))dlsym(ef_ptr->handle, tempText);
          } else {
	    f16arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *))internal_dlsym(tempText);
          }
	  (*f16arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2], arg_ptr[3], arg_ptr[4],
        arg_ptr[5], arg_ptr[6], arg_ptr[7], arg_ptr[8], arg_ptr[9], arg_ptr[10],
        arg_ptr[11], arg_ptr[12], arg_ptr[13], arg_ptr[14], arg_ptr[15], arg_ptr[16] );
	break;


    case 17:
	  if (!internally_linked) {
            f17arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))dlsym(ef_ptr->handle, tempText);
          } else {
            f17arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))internal_dlsym(tempText);
          }
	  (*f17arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2], arg_ptr[3], arg_ptr[4],
        arg_ptr[5], arg_ptr[6], arg_ptr[7], arg_ptr[8], arg_ptr[9], arg_ptr[10],
        arg_ptr[11], arg_ptr[12], arg_ptr[13], arg_ptr[14], arg_ptr[15], arg_ptr[16],
        arg_ptr[17] );
	break;


    case 18:
	  if (!internally_linked) {
	    f18arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))dlsym(ef_ptr->handle, tempText);
          } else {
	    f18arg  = (void (*)(int *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *,
             DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *, DFTYPE *))internal_dlsym(tempText);
          }
	  (*f18arg)( id_ptr, arg_ptr[0], arg_ptr[1], arg_ptr[2], arg_ptr[3], arg_ptr[4],
        arg_ptr[5], arg_ptr[6], arg_ptr[7], arg_ptr[8], arg_ptr[9], arg_ptr[10],
        arg_ptr[11], arg_ptr[12], arg_ptr[13], arg_ptr[14], arg_ptr[15], arg_ptr[16],
        arg_ptr[17], arg_ptr[18] );
	break;


    default:
      fprintf(stderr, "**ERROR: External functions with more than %d arguments are not implemented.\n",
                      EF_MAX_ARGS);
      *status = FERR_EF_ERROR;
      return;
      break;

    }

    /*
     * Restore the old signal handlers.
     */
    if ( EF_Util_ressig("efcn_compute")) {
       *status = FERR_EF_ERROR;
       return;
    }

    /*
     * Now it's time to release the work space.
     * With arg_ptr[0] for argument #1, and remembering one slot for the result,
     * we should begin freeing up memory at arg_ptr[num_reqd_args+1].
     */
    for (i=i_ptr->num_reqd_args+1; i<i_ptr->num_reqd_args+1+i_ptr->num_work_arrays; i++) {
      free(arg_ptr[i]);
    }

    /* Success for EF_F */
  }
  else if ( i_ptr->language == EF_PYTHON ) {
      int   memlo[EF_MAX_COMPUTE_ARGS][NFERDIMS], memhi[EF_MAX_COMPUTE_ARGS][NFERDIMS],
            steplo[EF_MAX_COMPUTE_ARGS][NFERDIMS], stephi[EF_MAX_COMPUTE_ARGS][NFERDIMS],
            incr[EF_MAX_COMPUTE_ARGS][NFERDIMS];
      DFTYPE badflags[EF_MAX_COMPUTE_ARGS];
      char  errstring[2048];

      /* First the results grid array, then the argument grid arrays */
      arg_ptr[0] = memory + mr_arg_offset_ptr[EF_MAX_ARGS];
      for (i = 0; i < i_ptr->num_reqd_args; i++) {
          arg_ptr[i+1] = memory + mr_arg_offset_ptr[i];
      }

      /* Assign the memory limits, step values, and bad-data-flag values - first result, then arguments */
      ef_get_res_mem_subscripts_6d_(id_ptr, memlo[0], memhi[0]);
      ef_get_arg_mem_subscripts_6d_(id_ptr, &(memlo[1]), &(memhi[1]));
      ef_get_res_subscripts_6d_(id_ptr, steplo[0], stephi[0], incr[0]);
      ef_get_arg_subscripts_6d_(id_ptr, &(steplo[1]), &(stephi[1]), &(incr[1]));
      ef_get_bad_flags_(id_ptr, &(badflags[1]), &(badflags[0]));

      /* Reset zero increments to +1 or -1 for pyefcn_compute */
      for (i = 0; i <= i_ptr->num_reqd_args; i++) {
          for (j = 0; j < NFERDIMS; j++) {
              if ( incr[i][j] == 0 ) {
                  if ( steplo[i][j] <= stephi[i][j] )
                      incr[i][j] = 1;
                  else
                      incr[i][j] = -1;
              }
          }
      }

      /*
       * Prepare for bailout possibilities by setting a signal handler for
       * SIGFPE, SIGSEGV, SIGINT and SIGBUS and then by cacheing the stack 
       * environment with sigsetjmp (for the signal handler) and setjmp 
       * (for the "bail out" utility function).
       */   
      if ( EF_Util_setsig("efcn_compute")) {
          *status = FERR_EF_ERROR;
          return;
      }
      if (sigsetjmp(sigjumpbuffer, 1) != 0) {
          *status = FERR_EF_ERROR;
          return;
      }
      if (setjmp(jumpbuffer) != 0) {
          *status = FERR_EF_ERROR;
          return;
      }
      canjump = 1;

      /* Call pyefcn_compute which in turn calls the ferret_compute method in the python module */
      pyefcn_compute(*id_ptr, ef_ptr->path, arg_ptr, (i_ptr->num_reqd_args)+1, memlo, memhi, steplo, stephi, incr, badflags, errstring);
      if ( strlen(errstring) > 0 ) {
          /* (In effect) call ef_bail_out_ to process the error in a standard way */
          ef_err_bail_out_(id_ptr, errstring);
          /* Should never return - instead jumps to setjmp() returning 1 */
      }

      /* Restore the original signal handlers */
      EF_Util_ressig("efcn_compute");

      /* Success for EF_PYTHON */
  }
  else {
    fprintf(stderr, "**ERROR: unsupported language (%d) for efcn_compute.\n", i_ptr->language);
    *status = FERR_EF_ERROR;
  }

  return;
}


/*
 * A signal handler for SIGFPE, SIGSEGV, SIGINT and SIGBUS signals generated
 * while executing an external function.  See "Advanced Programming
 * in the UNIX Environment" p. 299 ff for details.
 *
 * This routine should never return since a signal was raised indicating a
 * problem.  The siglongjump rewinds back to where sigsetjmp was called with
 * the current sigjumpbuffer.
 */
static void EF_signal_handler(int signo)
{
   if ( canjump == 0 ) {
      fprintf(stderr, "EF_signal_handler invoked with signal %d but canjump = 0", signo);
      fflush(stderr);
      abort();
   }

   /*
    * Restore the old signal handlers.
    */
   if ( EF_Util_ressig("efcn_compute")) {
      /* error message already printed */
      fflush(stderr);
      abort();
   }

   if (signo == SIGFPE) {
      fprintf(stderr, "**ERROR in external function: Floating Point Error\n");
      canjump = 0;
      siglongjmp(sigjumpbuffer, 1);
   } else if (signo == SIGSEGV) {
      fprintf(stderr, "**ERROR in external function: Segmentation Violation\n");
      canjump = 0;
      siglongjmp(sigjumpbuffer, 1);
   } else if (signo == SIGINT) {
      fprintf(stderr, "**External function halted with Control-C\n");
      canjump = 0;
      siglongjmp(sigjumpbuffer, 1);
   } else if (signo == SIGBUS) {
      fprintf(stderr, "**ERROR in external function: Hardware Fault\n");
      canjump = 0;
      siglongjmp(sigjumpbuffer, 1);
   } else {
      fprintf(stderr, "**ERROR in external function: signo = %d\n", signo);
      canjump = 0;
      siglongjmp(sigjumpbuffer, 1);
   }

}


/*
 * Find an external function based on its name and
 * return the integer ID associated with that funciton.
 */
int FORTRAN(efcn_get_id)( char name[] )
{
  ExternalFunction *ef_ptr=NULL;
  int status=LIST_OK;

  static int return_val=0; /* static because it needs to exist after the return statement */

  /*
   * Find the external function.
   */

  status = list_traverse(STATIC_ExternalFunctionList, name, EF_ListTraverse_FoundName,
                         (LIST_FRNT | LIST_FORW | LIST_ALTR));

  /*
   * If the search failed, set the id_ptr to ATOM_NOT_FOUND.
   */
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
  }

  ef_ptr=(ExternalFunction *)list_curr(STATIC_ExternalFunctionList); 

  return_val = ef_ptr->id;

  return return_val;
}


/*
 * Determine whether a function name matches a template.
 * Return 1 if the name matchs.
 */
int FORTRAN(efcn_match_template)( int *id_ptr, char template[] )
{
  ExternalFunction *ef_ptr=NULL;
  int status=LIST_OK;
  int EF_LT_MT_return;

  static int return_val=0; /* static because it needs to exist after the return statement */

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return return_val; }

  EF_LT_MT_return = EF_ListTraverse_MatchTemplate((char *)template, (char *)ef_ptr);
  
  /* The list package forces 'list traversal' functions to return
   * 0 whenever a match is found.  We want to return a more reasonable
   * 1 (=true) if we find a match.
   */
  if ( EF_LT_MT_return == FALSE ) {
	return_val = 1;
  } else {
    return_val = 0;
  }

  return return_val;
}


/*
 */
void FORTRAN(efcn_get_custom_axis_sub)( int *id_ptr, int *axis_ptr, double *lo_ptr, double *hi_ptr, 
			       double *del_ptr, char *unit, int *modulo_ptr )
{
  ExternalFunction *ef_ptr=NULL;

  /*
   * Find the external function.
   */

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }

  strcpy(unit, ef_ptr->internals_ptr->axis[*axis_ptr-1].unit);
  *lo_ptr = ef_ptr->internals_ptr->axis[*axis_ptr-1].ww_lo;
  *hi_ptr = ef_ptr->internals_ptr->axis[*axis_ptr-1].ww_hi;
  *del_ptr = ef_ptr->internals_ptr->axis[*axis_ptr-1].ww_del;
  *modulo_ptr = ef_ptr->internals_ptr->axis[*axis_ptr-1].modulo;

  return;
}


/*
 * Find an external function based on its integer ID and
 * return the name.
 */
void FORTRAN(efcn_get_name)( int *id_ptr, char *name )
{
  ExternalFunction *ef_ptr=NULL;

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }

  strcpy(name, ef_ptr->name);

  return;
}


/*
 * Find an external function based on its integer ID and
 * return the version number.
 */
void FORTRAN(efcn_get_version)( int *id_ptr, DFTYPE *version )
{
  ExternalFunction *ef_ptr=NULL;

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }

  *version = ef_ptr->internals_ptr->version;

  return;
}


/*
 * Find an external function based on its integer ID and
 * return the description.
 */
void FORTRAN(efcn_get_descr)( int *id_ptr, char *descr )
{
  ExternalFunction *ef_ptr=NULL;

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }

  strcpy(descr, ef_ptr->internals_ptr->description);

  return;
}

/*
 * Find an external function based on its integer ID and
 * return the name of an alternate function that operates 
 * with string arguments.
 *
 * *kms* 2/11 - assign blank-terminated (not null-terminated)
 * string since code using this name expects this style.
 * Assumes alt_str_name has been intialized to all-blank.
 */
void FORTRAN(efcn_get_alt_type_fcn)( int *id_ptr, char *alt_str_name )
{
  ExternalFunction *ef_ptr=NULL;

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }

  strcpy(alt_str_name, ef_ptr->internals_ptr->alt_fcn_name);
  alt_str_name[strlen(alt_str_name)] = ' ';

  return;
}


/*
 * Find an external function based on its integer ID and
 * return the number of arguments.
 */
int FORTRAN(efcn_get_num_reqd_args)( int *id_ptr )
{
  ExternalFunction *ef_ptr=NULL;

  static int return_val=0; /* static because it needs to exist after the return statement */

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return return_val; }

  return_val = ef_ptr->internals_ptr->num_reqd_args;

  return return_val;
}


/*
 * Find an external function based on its integer ID and
 * return the flag stating whether the function has
 * a variable number of arguments.
 */
void FORTRAN(efcn_get_has_vari_args)( int *id_ptr, int *has_vari_args_ptr )
{
  ExternalFunction *ef_ptr=NULL;

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }

  *has_vari_args_ptr = ef_ptr->internals_ptr->has_vari_args;

  return;
}


/*
 * Find an external function based on its integer ID and
 * return the axis sources (merged, normal, abstract, custom).
 */
void FORTRAN(efcn_get_axis_will_be)( int *id_ptr, int *array_ptr )
{
  ExternalFunction *ef_ptr=NULL;

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }

  array_ptr[X_AXIS] = ef_ptr->internals_ptr->axis_will_be[X_AXIS];
  array_ptr[Y_AXIS] = ef_ptr->internals_ptr->axis_will_be[Y_AXIS];
  array_ptr[Z_AXIS] = ef_ptr->internals_ptr->axis_will_be[Z_AXIS];
  array_ptr[T_AXIS] = ef_ptr->internals_ptr->axis_will_be[T_AXIS];
  array_ptr[E_AXIS] = ef_ptr->internals_ptr->axis_will_be[E_AXIS];
  array_ptr[F_AXIS] = ef_ptr->internals_ptr->axis_will_be[F_AXIS];

  return;
}


/*
 * Find an external function based on its integer ID and
 * return the axis_reduction (retained, reduced) information.
 */
void FORTRAN(efcn_get_axis_reduction)( int *id_ptr, int *array_ptr )
{
  ExternalFunction *ef_ptr=NULL;

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }

  array_ptr[X_AXIS] = ef_ptr->internals_ptr->axis_reduction[X_AXIS];
  array_ptr[Y_AXIS] = ef_ptr->internals_ptr->axis_reduction[Y_AXIS];
  array_ptr[Z_AXIS] = ef_ptr->internals_ptr->axis_reduction[Z_AXIS];
  array_ptr[T_AXIS] = ef_ptr->internals_ptr->axis_reduction[T_AXIS];
  array_ptr[E_AXIS] = ef_ptr->internals_ptr->axis_reduction[E_AXIS];
  array_ptr[F_AXIS] = ef_ptr->internals_ptr->axis_reduction[F_AXIS];

  return;
}


/*
 * Find an external function based on its integer ID and
 * return the piecemeal_ok information.  This lets Ferret
 * know if it's ok to break up a calculation along an axis
 * for memory management reasons.
 */
void FORTRAN(efcn_get_piecemeal_ok)( int *id_ptr, int *array_ptr )
{
  ExternalFunction *ef_ptr=NULL;

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }

  array_ptr[X_AXIS] = ef_ptr->internals_ptr->piecemeal_ok[X_AXIS];
  array_ptr[Y_AXIS] = ef_ptr->internals_ptr->piecemeal_ok[Y_AXIS];
  array_ptr[Z_AXIS] = ef_ptr->internals_ptr->piecemeal_ok[Z_AXIS];
  array_ptr[T_AXIS] = ef_ptr->internals_ptr->piecemeal_ok[T_AXIS];
  array_ptr[E_AXIS] = ef_ptr->internals_ptr->piecemeal_ok[E_AXIS];
  array_ptr[F_AXIS] = ef_ptr->internals_ptr->piecemeal_ok[F_AXIS];

  return;
}


/*
 * Find an external function based on its integer ID and
 * return the (boolean) 'axis_implied_from' information for
 * a particular argument to find out if its axes should
 * be merged in to the result grid.
 */
void FORTRAN(efcn_get_axis_implied_from)( int *id_ptr, int *iarg_ptr, int *array_ptr )
{
  ExternalFunction *ef_ptr=NULL;
  int index = *iarg_ptr - 1; /* C indices are 1 less than Fortran */ 

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }

  array_ptr[X_AXIS] = ef_ptr->internals_ptr->axis_implied_from[index][X_AXIS];
  array_ptr[Y_AXIS] = ef_ptr->internals_ptr->axis_implied_from[index][Y_AXIS];
  array_ptr[Z_AXIS] = ef_ptr->internals_ptr->axis_implied_from[index][Z_AXIS];
  array_ptr[T_AXIS] = ef_ptr->internals_ptr->axis_implied_from[index][T_AXIS];
  array_ptr[E_AXIS] = ef_ptr->internals_ptr->axis_implied_from[index][E_AXIS];
  array_ptr[F_AXIS] = ef_ptr->internals_ptr->axis_implied_from[index][F_AXIS];

  return;
}


/*
 * Find an external function based on its integer ID and
 * return the 'arg_extend_lo' information for a particular
 * argument which tells Ferret how much to extend axis limits
 * when providing input data (e.g. to compute a derivative).
 */
void FORTRAN(efcn_get_axis_extend_lo)( int *id_ptr, int *iarg_ptr, int *array_ptr )
{
  ExternalFunction *ef_ptr=NULL;
  int index = *iarg_ptr - 1; /* C indices are 1 less than Fortran */ 

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }

  array_ptr[X_AXIS] = ef_ptr->internals_ptr->axis_extend_lo[index][X_AXIS];
  array_ptr[Y_AXIS] = ef_ptr->internals_ptr->axis_extend_lo[index][Y_AXIS];
  array_ptr[Z_AXIS] = ef_ptr->internals_ptr->axis_extend_lo[index][Z_AXIS];
  array_ptr[T_AXIS] = ef_ptr->internals_ptr->axis_extend_lo[index][T_AXIS];
  array_ptr[E_AXIS] = ef_ptr->internals_ptr->axis_extend_lo[index][E_AXIS];
  array_ptr[F_AXIS] = ef_ptr->internals_ptr->axis_extend_lo[index][F_AXIS];

  return;
}


/*
 * Find an external function based on its integer ID and
 * return the 'arg_extend_hi' information for a particular
 * argument which tells Ferret how much to extend axis limits
 * when providing input data (e.g. to compute a derivative).
 */
void FORTRAN(efcn_get_axis_extend_hi)( int *id_ptr, int *iarg_ptr, int *array_ptr )
{
  ExternalFunction *ef_ptr=NULL;
  int index = *iarg_ptr - 1; /* C indices are 1 less than Fortran */ 

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }
  
  array_ptr[X_AXIS] = ef_ptr->internals_ptr->axis_extend_hi[index][X_AXIS];
  array_ptr[Y_AXIS] = ef_ptr->internals_ptr->axis_extend_hi[index][Y_AXIS];
  array_ptr[Z_AXIS] = ef_ptr->internals_ptr->axis_extend_hi[index][Z_AXIS];
  array_ptr[T_AXIS] = ef_ptr->internals_ptr->axis_extend_hi[index][T_AXIS];
  array_ptr[E_AXIS] = ef_ptr->internals_ptr->axis_extend_hi[index][E_AXIS];
  array_ptr[F_AXIS] = ef_ptr->internals_ptr->axis_extend_hi[index][F_AXIS];

  return;
}


/*
 * Find an external function based on its integer ID and
 * return the 'axis_limits' information for a particular
 * argument.
 */
void FORTRAN(efcn_get_axis_limits)( int *id_ptr, int *axis_ptr, int *lo_ptr, int *hi_ptr )
{
  ExternalFunction *ef_ptr=NULL;
  int index = *axis_ptr - 1; /* C indices are 1 less than Fortran */ 

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }
  
  *lo_ptr = ef_ptr->internals_ptr->axis[index].ss_lo;
  *hi_ptr = ef_ptr->internals_ptr->axis[index].ss_hi;
  
  return;
}


/*
 * Find an external function based on its integer ID and
 * return the 'arg_type' information for a particular
 * argument which tells Ferret whether an argument is a 
 * DFTYPE or a string.
 */
int FORTRAN(efcn_get_arg_type)( int *id_ptr, int *iarg_ptr )
{
  ExternalFunction *ef_ptr=NULL;
  int return_val=0;
  int index = *iarg_ptr - 1; /* C indices are 1 less than Fortran */ 

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return return_val; }
  
  return_val = ef_ptr->internals_ptr->arg_type[index];
  
  return return_val;
}


/*
 * Find an external function based on its integer ID and
 * return the 'rtn_type' information for the result which
 * tells Ferret whether an argument is a DFTYPE or a string.
 */
int FORTRAN(efcn_get_rtn_type)( int *id_ptr )
{
  ExternalFunction *ef_ptr=NULL;
  static int return_val=0; /* static because it needs to exist after the return statement */

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return return_val; }
  
  return_val = ef_ptr->internals_ptr->return_type;
  
  return return_val;
}


/*
 * Find an external function based on its integer ID and
 * return the name of a particular argument.
 */
void FORTRAN(efcn_get_arg_name)( int *id_ptr, int *iarg_ptr, char *string )
{
  ExternalFunction *ef_ptr=NULL;
  int index = *iarg_ptr - 1; /* C indices are 1 less than Fortran */ 
  int i=0, printable=FALSE;

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }
  
  /*
   * JC_NOTE: if the argument has no name then memory gets overwritten, corrupting
   * the address of iarg_ptr and causing a core dump.  I need to catch that case
   * here.
   */

  for (i=0;i<strlen(ef_ptr->internals_ptr->arg_name[index]);i++) {
    if (isgraph(ef_ptr->internals_ptr->arg_name[index][i])) {
      printable = TRUE;
      break;
    }
  }

  if ( printable ) {
    strcpy(string, ef_ptr->internals_ptr->arg_name[index]);
  } else {
    strcpy(string, "--");
  }

  return;
}


/*
 * Find an external function based on its integer ID and
 * return the units for a particular argument.
 */
void FORTRAN(efcn_get_arg_unit)( int *id_ptr, int *iarg_ptr, char *string )
{
  ExternalFunction *ef_ptr=NULL;
  int index = *iarg_ptr - 1; /* C indices are 1 less than Fortran */ 

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }
  
  ef_ptr=(ExternalFunction *)list_curr(STATIC_ExternalFunctionList); 
  
  strcpy(string, ef_ptr->internals_ptr->arg_unit[index]);

  return;
}


/*
 * Find an external function based on its integer ID and
 * return the description of a particular argument.
 */
void FORTRAN(efcn_get_arg_desc)( int *id_ptr, int *iarg_ptr, char *string )
{
  ExternalFunction *ef_ptr=NULL;
  int index = *iarg_ptr - 1; /* C indices are 1 less than Fortran */ 

  if ( (ef_ptr = ef_ptr_from_id_ptr(id_ptr)) == NULL ) { return; }
  
  strcpy(string, ef_ptr->internals_ptr->arg_desc[index]);

  return;
}



/*
 * This function should never return since there was a user-detected
 * error of some sort.  The call to longjump rewinds back to where
 * setjmp was called with the current jumpbuffer.
 */
void FORTRAN(ef_err_bail_out)(int *id_ptr, char *text)
{
   ExternalFunction *ef_ptr=NULL;

   ef_ptr = ef_ptr_from_id_ptr(id_ptr);
   if ( ef_ptr == NULL ) {
      fprintf(stderr, "Unknown external function ID of %d in ef_err_bail_out", *id_ptr);
      fflush(stderr);
      abort();
   }
   if ( canjump == 0 ) {
      fputs("ef_err_bail_out called with canjump = 0", stderr);
      fflush(stderr);
      abort();
   }
   /*
    * Restore the old signal handlers.
    */
   if ( EF_Util_ressig("efcn_compute")) {
      /* error message already printed */
      fflush(stderr);
      abort();
   }

   fprintf(stderr, "\n"
                   "Bailing out of external function \"%s\":\n"
                   "\t%s\n", ef_ptr->name, text);

   longjmp(jumpbuffer, 1);
}



/* .... Object Oriented Utility Functions .... */


/*
 * Allocate space for and initialize the internal
 * information for an EF.
 *
 * Return values:
 *     -1: error allocating space
 *      0: success
 */
int EF_New( ExternalFunction *this )
{
  ExternalFunctionInternals *i_ptr=NULL;
  int status=LIST_OK, i=0, j=0;

  static int return_val=0; /* static because it needs to exist after the return statement */


  /*
   * Allocate space for the internals.
   * If the allocation failed, print a warning message and return.
   */

  this->internals_ptr = malloc(sizeof(ExternalFunctionInternals));
  i_ptr = this->internals_ptr;

  if ( i_ptr == NULL ) {
    fprintf(stderr, "**ERROR in EF_New(): cannot allocate ExternalFunctionInternals.\n");
    return_val = -1;
    return return_val;
  }


  /*
   * Initialize the internals.
   */

  /* Information about the overall function */

  i_ptr->version = EF_VERSION;
  strcpy(i_ptr->description, "");
  i_ptr->language = EF_F;
  i_ptr->num_reqd_args = 1;
  i_ptr->has_vari_args = NO;
  i_ptr->num_work_arrays = 0;
  i_ptr->return_type = FLOAT_RETURN;
  for (i=0; i<NFERDIMS; i++) {
    for (j=0; j<EF_MAX_WORK_ARRAYS; j++) {
      i_ptr->work_array_lo[j][i] = 1;
      i_ptr->work_array_hi[j][i] = 1;
    }
    i_ptr->axis_will_be[i] = IMPLIED_BY_ARGS;
    i_ptr->axis_reduction[i] = RETAINED;
    i_ptr->piecemeal_ok[i] = NO;
  }

  /* Information specific to each argument of the function */

  for (i=0; i<EF_MAX_ARGS; i++) {
    for (j=0; j<NFERDIMS; j++) {
      i_ptr->axis_implied_from[i][j] = YES;
      i_ptr->axis_extend_lo[i][j] = 0;
      i_ptr->axis_extend_hi[i][j] = 0;
    }
    i_ptr->arg_type[i] = FLOAT_ARG;
    strcpy(i_ptr->arg_name[i], "");
    strcpy(i_ptr->arg_unit[i], "");
    strcpy(i_ptr->arg_desc[i], "");
  }

  return return_val;

}


/* .... UtilityFunctions for dealing with STATIC_ExternalFunctionList .... */

/*
 * Store the global values which will be needed by utility routines
 * in EF_ExternalUtil.c
 */
void EF_store_globals(DFTYPE *memory_ptr, int *mr_list_ptr, int *cx_list_ptr, 
	int *mres_ptr, DFTYPE *bad_flag_ptr)
{
  int i=0;

  GLOBAL_memory_ptr = memory_ptr;
  GLOBAL_mr_list_ptr = mr_list_ptr;
  GLOBAL_cx_list_ptr = cx_list_ptr;
  GLOBAL_mres_ptr = mres_ptr;
  GLOBAL_bad_flag_ptr = bad_flag_ptr;

}


/*
 * Find an external function based on an integer id
 * and return the pointer to the function.
 * Returns NULL if it fails.
 */
ExternalFunction *ef_ptr_from_id_ptr(int *id_ptr)
{
   ExternalFunction *ef_ptr;
   int status;

   /* Check if the list has been created to avoid a seg fault if called indiscriminately */
   if ( STATIC_ExternalFunctionList == NULL ) {
      return NULL;
   }

   /* Search the list for the function ID */
   status = list_traverse(STATIC_ExternalFunctionList, (char *) id_ptr, EF_ListTraverse_FoundID,
                          (LIST_FRNT | LIST_FORW | LIST_ALTR));
   if ( status != LIST_OK ) {
      return NULL;
   }

   /* Get the pointer to the function from the list */
   ef_ptr = (ExternalFunction *) list_curr(STATIC_ExternalFunctionList); 
   return ef_ptr;
}


int EF_ListTraverse_fprintf( char *data, char *curr )
{
   FILE *File_ptr=(FILE *)data;
   ExternalFunction *ef_ptr=(ExternalFunction *)curr; 

   fprintf(stderr, "path = \"%s\", name = \"%s\", id = %d, internals_ptr = %ld\n",
	   ef_ptr->path, ef_ptr->name, ef_ptr->id, (long) (ef_ptr->internals_ptr));

   return TRUE;
}
 

/*
 * Ferret always capitalizes everything so we'd better
 * be case INsensitive.
 */
int EF_ListTraverse_FoundName( char *data, char *curr )
{
  ExternalFunction *ef_ptr=(ExternalFunction *)curr; 

  if ( !strcasecmp(data, ef_ptr->name) ) {
    return FALSE; /* found match */
  } else
    return TRUE;
}


int EF_ListTraverse_MatchTemplate( char data[], char *curr )
{
  ExternalFunction *ef_ptr=(ExternalFunction *)curr; 

  int i=0, star_skip=FALSE;
  char upname[EF_MAX_DESCRIPTION_LENGTH];
  char *t, *n;

  for (i=0; i<strlen(ef_ptr->name); i++) {
    upname[i] = toupper(ef_ptr->name[i]);
  }
  upname[i] = '\0';

  n = upname;

  for (i=0, t=data; i<strlen(data); i++, t++) {

    if ( *t == '*' ) {

      star_skip = TRUE;
      continue;

    } else if ( *t == '?' ) {

      if ( star_skip ) {
	continue;
      } else {
	if ( ++n == '\0' ) /* end of name */
	  return TRUE; /* no match */
	else
	  continue;
      }

    } else if ( star_skip ) {

      if ( (n = strchr(n, *t)) == NULL ) { /* character not found in rest of name */
	return TRUE; /* no match */
      } else {
	star_skip = FALSE;
      }

    } else if ( *n == '\0' ) /* end of name */
      return TRUE; /* no match */

    else if ( *t == *n ) {
      n++;
      continue;
    }

    else
      return TRUE; /* no match */

  } 

  /* *sh* if any non-wildcard characters remain in the "curr" name, then reject
     probably a bug remains for a regexp ending in "?" */
  if ( *n == '\0' || star_skip )
    return FALSE; /* got all the way through: a match */
  else
    return TRUE; /* characters remain--e.g. "xx5" does not math regexp "xx" */

}


int EF_ListTraverse_FoundID( char *data, char *curr )
{
  ExternalFunction *ef_ptr=(ExternalFunction *)curr; 
  int ID=*((int *)data);

  if ( ID == ef_ptr->id ) {
    return FALSE; /* found match */
  } else
    return TRUE;
}


int EF_Util_setsig(char fcn_name[])
{
    /*
     * Prepare for bailout possibilities by setting a signal handler for
     * SIGFPE, SIGSEGV, SIGINT and SIGBUS and then by cacheing the stack 
     * environment with sigsetjmp (for the signal handler) and setjmp 
     * (for the "bail out" utility function).
     */   

    if ( (fpe_handler = signal(SIGFPE, EF_signal_handler)) == SIG_ERR ) {
      fprintf(stderr, "**ERROR in %s() catching SIGFPE.\n", fcn_name);
      return 1;
    }
    if ( (segv_handler = signal(SIGSEGV, EF_signal_handler)) == SIG_ERR ) {
      fprintf(stderr, "**ERROR in %s() catching SIGSEGV.\n", fcn_name);
      return 1;
    }
    if ( (int_handler = signal(SIGINT, EF_signal_handler)) == SIG_ERR ) {
      fprintf(stderr, "**ERROR in %s() catching SIGINT.\n", fcn_name);
      return 1;
    }
    if ( (bus_handler = signal(SIGBUS, EF_signal_handler)) == SIG_ERR ) {
      fprintf(stderr, "**ERROR in %s() catching SIGBUS.\n", fcn_name);
      return 1;
    }

    /* the setjmp and sigsetjmp code moved to in-line 10/00 --
     * longjump returns cannot be made reliably into a subroutine that may
     *no longer be active on the stack
     */

    return 0;
}


int EF_Util_ressig(char fcn_name[])
{
    /*
     * Restore the old signal handlers.
     */
    if (signal(SIGFPE, (*fpe_handler)) == SIG_ERR) {
      fprintf(stderr, "**ERROR in %s() restoring default SIGFPE handler.\n", fcn_name);
      return 1;
    }
    if (signal(SIGSEGV, (*segv_handler)) == SIG_ERR) {
      fprintf(stderr, "**ERROR in %s() restoring default SIGSEGV handler.\n", fcn_name);
      return 1;
    }
    if (signal(SIGINT, (*int_handler)) == SIG_ERR) {
      fprintf(stderr, "**ERROR in %s() restoring default SIGINT handler.\n", fcn_name);
      return 1;
    }
    if (signal(SIGBUS, (*bus_handler)) == SIG_ERR) {
      fprintf(stderr, "**ERROR in %s() restoring default SIGBUS handler.\n", fcn_name);
      return 1;
    }
    return 0;
}


/* 
 *  ------------------------------------

 *  internal_dlsym
 *  Accept a string and return the function pointer 
 *
 *  The names of all subroutines of internally linked EF's
 *  generated by the perl script int_dlsym.pl.  Check the
 *  first if statement - change else if to if.
 *
 *   ACM 2-25-00 Solaris and OSF both have the trailing
 *   underscore for statically-linked routines. */

static void *internal_dlsym(char *name) {

/* ffta.F */
if ( !strcmp(name,"ffta_init_") ) return (void *)FORTRAN(ffta_init);
else if ( !strcmp(name,"ffta_custom_axes_") ) return (void *)FORTRAN(ffta_custom_axes);
else if ( !strcmp(name,"ffta_result_limits_") ) return (void *)FORTRAN(ffta_result_limits);
else if ( !strcmp(name,"ffta_work_size_") ) return (void *)FORTRAN(ffta_work_size);
else if ( !strcmp(name,"ffta_compute_") ) return (void *)FORTRAN(ffta_compute);

/* fftp.F */
else if ( !strcmp(name,"fftp_init_") ) return (void *)FORTRAN(fftp_init);
else if ( !strcmp(name,"fftp_custom_axes_") ) return (void *)FORTRAN(fftp_custom_axes);
else if ( !strcmp(name,"fftp_result_limits_") ) return (void *)FORTRAN(fftp_result_limits);
else if ( !strcmp(name,"fftp_work_size_") ) return (void *)FORTRAN(fftp_work_size);
else if ( !strcmp(name,"fftp_compute_") ) return (void *)FORTRAN(fftp_compute);

/* fft_im.F */
else if ( !strcmp(name,"fft_im_init_") ) return (void *)FORTRAN(fft_im_init);
else if ( !strcmp(name,"fft_im_custom_axes_") ) return (void *)FORTRAN(fft_im_custom_axes);
else if ( !strcmp(name,"fft_im_result_limits_") ) return (void *)FORTRAN(fft_im_result_limits);
else if ( !strcmp(name,"fft_im_work_size_") ) return (void *)FORTRAN(fft_im_work_size);
else if ( !strcmp(name,"fft_im_compute_") ) return (void *)FORTRAN(fft_im_compute);

/* fft_inverse.F */
else if ( !strcmp(name,"fft_inverse_init_") ) return (void *)FORTRAN(fft_inverse_init);
else if ( !strcmp(name,"fft_inverse_result_limits_") ) return (void *)FORTRAN(fft_inverse_result_limits);
else if ( !strcmp(name,"fft_inverse_work_size_") ) return (void *)FORTRAN(fft_inverse_work_size);
else if ( !strcmp(name,"fft_inverse_compute_") ) return (void *)FORTRAN(fft_inverse_compute);

/* fft_re.F */
else if ( !strcmp(name,"fft_re_init_") ) return (void *)FORTRAN(fft_re_init);
else if ( !strcmp(name,"fft_re_custom_axes_") ) return (void *)FORTRAN(fft_re_custom_axes);
else if ( !strcmp(name,"fft_re_result_limits_") ) return (void *)FORTRAN(fft_re_result_limits);
else if ( !strcmp(name,"fft_re_work_size_") ) return (void *)FORTRAN(fft_re_work_size);
else if ( !strcmp(name,"fft_re_compute_") ) return (void *)FORTRAN(fft_re_compute);

/* sampleij.F */
else if ( !strcmp(name,"sampleij_init_") ) return (void *)FORTRAN(sampleij_init);
else if ( !strcmp(name,"sampleij_result_limits_") ) return (void *)FORTRAN(sampleij_result_limits);
else if ( !strcmp(name,"sampleij_work_size_") ) return (void *)FORTRAN(sampleij_work_size);
else if ( !strcmp(name,"sampleij_compute_") ) return (void *)FORTRAN(sampleij_compute);


/* samplei_multi.F */
else if ( !strcmp(name,"samplei_multi_init_") ) return (void *)FORTRAN(samplei_multi_init);
else if ( !strcmp(name,"samplei_multi_result_limits_") ) return (void *)FORTRAN(samplei_multi_result_limits);
else if ( !strcmp(name,"samplei_multi_compute_") ) return (void *)FORTRAN(samplei_multi_compute);

/* samplej_multi.F */
else if ( !strcmp(name,"samplej_multi_init_") ) return (void *)FORTRAN(samplej_multi_init);
else if ( !strcmp(name,"samplej_multi_result_limits_") ) return (void *)FORTRAN(samplej_multi_result_limits);
else if ( !strcmp(name,"samplej_multi_compute_") ) return (void *)FORTRAN(samplej_multi_compute);

/* samplek_multi.F */
else if ( !strcmp(name,"samplek_multi_init_") ) return (void *)FORTRAN(samplek_multi_init);
else if ( !strcmp(name,"samplek_multi_result_limits_") ) return (void *)FORTRAN(samplek_multi_result_limits);
else if ( !strcmp(name,"samplek_multi_compute_") ) return (void *)FORTRAN(samplek_multi_compute);

/* samplel_multi.F */
else if ( !strcmp(name,"samplel_multi_init_") ) return (void *)FORTRAN(samplel_multi_init);
else if ( !strcmp(name,"samplel_multi_result_limits_") ) return (void *)FORTRAN(samplel_multi_result_limits);
else if ( !strcmp(name,"samplel_multi_compute_") ) return (void *)FORTRAN(samplel_multi_compute);

/* samplet_date.F */
else if ( !strcmp(name,"samplet_date_init_") ) return (void *)FORTRAN(samplet_date_init);
else if ( !strcmp(name,"samplet_date_result_limits_") ) return (void *)FORTRAN(samplet_date_result_limits);
else if ( !strcmp(name,"samplet_date_work_size_") ) return (void *)FORTRAN(samplet_date_work_size);
else if ( !strcmp(name,"samplet_date_compute_") ) return (void *)FORTRAN(samplet_date_compute);

/* samplexy.F */
else if ( !strcmp(name,"samplexy_init_") ) return (void *)FORTRAN(samplexy_init);
else if ( !strcmp(name,"samplexy_result_limits_") ) return (void *)FORTRAN(samplexy_result_limits);
else if ( !strcmp(name,"samplexy_work_size_") ) return (void *)FORTRAN(samplexy_work_size);
else if ( !strcmp(name,"samplexy_compute_") ) return (void *)FORTRAN(samplexy_compute);

/* samplexyt.F */
else if ( !strcmp(name,"samplexyt_init_") ) return (void *)FORTRAN(samplexyt_init);
else if ( !strcmp(name,"samplexyt_result_limits_") ) return (void *)FORTRAN(samplexyt_result_limits);
else if ( !strcmp(name,"samplexyt_work_size_") ) return (void *)FORTRAN(samplexyt_work_size);
else if ( !strcmp(name,"samplexyt_compute_") ) return (void *)FORTRAN(samplexyt_compute);

/* samplexyt_nrst.F */
else if ( !strcmp(name,"samplexyt_nrst_init_") ) return (void *)FORTRAN(samplexyt_nrst_init);
else if ( !strcmp(name,"samplexyt_nrst_result_limits_") ) return (void *)FORTRAN(samplexyt_nrst_result_limits);
else if ( !strcmp(name,"samplexyt_nrst_work_size_") ) return (void *)FORTRAN(samplexyt_nrst_work_size);
else if ( !strcmp(name,"samplexyt_nrst_compute_") ) return (void *)FORTRAN(samplexyt_nrst_compute);

/* samplexy_curv.F */
else if ( !strcmp(name,"samplexy_curv_init_") ) return (void *)FORTRAN(samplexy_curv_init);
else if ( !strcmp(name,"samplexy_curv_result_limits_") ) return (void *)FORTRAN(samplexy_curv_result_limits);
else if ( !strcmp(name,"samplexy_curv_work_size_") ) return (void *)FORTRAN(samplexy_curv_work_size);
else if ( !strcmp(name,"samplexy_curv_compute_") ) return (void *)FORTRAN(samplexy_curv_compute);

/* samplexy_curv_avg.F */
else if ( !strcmp(name,"samplexy_curv_avg_init_") ) return (void *)FORTRAN(samplexy_curv_avg_init);
else if ( !strcmp(name,"samplexy_curv_avg_result_limits_") ) return (void *)FORTRAN(samplexy_curv_avg_result_limits);
else if ( !strcmp(name,"samplexy_curv_avg_work_size_") ) return (void *)FORTRAN(samplexy_curv_avg_work_size);
else if ( !strcmp(name,"samplexy_curv_avg_compute_") ) return (void *)FORTRAN(samplexy_curv_avg_compute);

/* samplexy_curv_nrst.F */
else if ( !strcmp(name,"samplexy_curv_nrst_init_") ) return (void *)FORTRAN(samplexy_curv_nrst_init);
else if ( !strcmp(name,"samplexy_curv_nrst_result_limits_") ) return (void *)FORTRAN(samplexy_curv_nrst_result_limits);
else if ( !strcmp(name,"samplexy_curv_nrst_work_size_") ) return (void *)FORTRAN(samplexy_curv_nrst_work_size);
else if ( !strcmp(name,"samplexy_curv_nrst_compute_") ) return (void *)FORTRAN(samplexy_curv_nrst_compute);

/* samplexy_closest.F */
else if ( !strcmp(name,"samplexy_closest_init_") ) return (void *)FORTRAN(samplexy_closest_init);
else if ( !strcmp(name,"samplexy_closest_result_limits_") ) return (void *)FORTRAN(samplexy_closest_result_limits);
else if ( !strcmp(name,"samplexy_closest_work_size_") ) return (void *)FORTRAN(samplexy_closest_work_size);
else if ( !strcmp(name,"samplexy_closest_compute_") ) return (void *)FORTRAN(samplexy_closest_compute);

/* samplexz.F */
else if ( !strcmp(name,"samplexz_init_") ) return (void *)FORTRAN(samplexz_init);
else if ( !strcmp(name,"samplexz_result_limits_") ) return (void *)FORTRAN(samplexz_result_limits);
else if ( !strcmp(name,"samplexz_work_size_") ) return (void *)FORTRAN(samplexz_work_size);
else if ( !strcmp(name,"samplexz_compute_") ) return (void *)FORTRAN(samplexz_compute);

/* sampleyz.F */
else if ( !strcmp(name,"sampleyz_init_") ) return (void *)FORTRAN(sampleyz_init);
else if ( !strcmp(name,"sampleyz_result_limits_") ) return (void *)FORTRAN(sampleyz_result_limits);
else if ( !strcmp(name,"sampleyz_work_size_") ) return (void *)FORTRAN(sampleyz_work_size);
else if ( !strcmp(name,"sampleyz_compute_") ) return (void *)FORTRAN(sampleyz_compute);

/* scat2grid_bin_xy.F */
else if ( !strcmp(name,"scat2grid_bin_xy_init_") ) return (void *)FORTRAN(scat2grid_bin_xy_init);
else if ( !strcmp(name,"scat2grid_bin_xy_work_size_") ) return (void *)FORTRAN(scat2grid_bin_xy_work_size);
else if ( !strcmp(name,"scat2grid_bin_xy_compute_") ) return (void *)FORTRAN(scat2grid_bin_xy_compute);

/* scat2grid_bin_xyt.F */
else if ( !strcmp(name,"scat2grid_bin_xyt_init_") ) return (void *)FORTRAN(scat2grid_bin_xyt_init);
else if ( !strcmp(name,"scat2grid_bin_xyt_work_size_") ) return (void *)FORTRAN(scat2grid_bin_xyt_work_size);
else if ( !strcmp(name,"scat2grid_bin_xyt_compute_") ) return (void *)FORTRAN(scat2grid_bin_xyt_compute);

/* scat2grid_nbin_xy.F */
else if ( !strcmp(name,"scat2grid_nbin_xy_init_") ) return (void *)FORTRAN(scat2grid_nbin_xy_init);
else if ( !strcmp(name,"scat2grid_nbin_xy_work_size_") ) return (void *)FORTRAN(scat2grid_nbin_xy_work_size);
else if ( !strcmp(name,"scat2grid_nbin_xy_compute_") ) return (void *)FORTRAN(scat2grid_nbin_xy_compute);

/* scat2grid_nbin_xyt.F */
else if ( !strcmp(name,"scat2grid_nbin_xyt_init_") ) return (void *)FORTRAN(scat2grid_nbin_xyt_init);
else if ( !strcmp(name,"scat2grid_nbin_xyt_work_size_") ) return (void *)FORTRAN(scat2grid_nbin_xyt_work_size);
else if ( !strcmp(name,"scat2grid_nbin_xyt_compute_") ) return (void *)FORTRAN(scat2grid_nbin_xyt_compute);

/* scat2gridgauss_xy.F */
else if ( !strcmp(name,"scat2gridgauss_xy_init_") ) return (void *)FORTRAN(scat2gridgauss_xy_init);
else if ( !strcmp(name,"scat2gridgauss_xy_work_size_") ) return (void *)FORTRAN(scat2gridgauss_xy_work_size);
else if ( !strcmp(name,"scat2gridgauss_xy_compute_") ) return (void *)FORTRAN(scat2gridgauss_xy_compute);

/* scat2gridgauss_xz.F */
else if ( !strcmp(name,"scat2gridgauss_xz_init_") ) return (void *)FORTRAN(scat2gridgauss_xz_init);
else if ( !strcmp(name,"scat2gridgauss_xz_work_size_") ) return (void *)FORTRAN(scat2gridgauss_xz_work_size);
else if ( !strcmp(name,"scat2gridgauss_xz_compute_") ) return (void *)FORTRAN(scat2gridgauss_xz_compute);

/* scat2gridgauss_yz.F */
else if ( !strcmp(name,"scat2gridgauss_yz_init_") ) return (void *)FORTRAN(scat2gridgauss_yz_init);
else if ( !strcmp(name,"scat2gridgauss_yz_work_size_") ) return (void *)FORTRAN(scat2gridgauss_yz_work_size);
else if ( !strcmp(name,"scat2gridgauss_yz_compute_") ) return (void *)FORTRAN(scat2gridgauss_yz_compute);

/* scat2gridgauss_xt.F */
else if ( !strcmp(name,"scat2gridgauss_xt_init_") ) return (void *)FORTRAN(scat2gridgauss_xt_init);
else if ( !strcmp(name,"scat2gridgauss_xt_work_size_") ) return (void *)FORTRAN(scat2gridgauss_xt_work_size);
else if ( !strcmp(name,"scat2gridgauss_xt_compute_") ) return (void *)FORTRAN(scat2gridgauss_xt_compute);

/* scat2gridgauss_yt.F */
else if ( !strcmp(name,"scat2gridgauss_yt_init_") ) return (void *)FORTRAN(scat2gridgauss_yt_init);
else if ( !strcmp(name,"scat2gridgauss_yt_work_size_") ) return (void *)FORTRAN(scat2gridgauss_yt_work_size);
else if ( !strcmp(name,"scat2gridgauss_yt_compute_") ) return (void *)FORTRAN(scat2gridgauss_yt_compute);

/* scat2gridgauss_zt.F */
else if ( !strcmp(name,"scat2gridgauss_zt_init_") ) return (void *)FORTRAN(scat2gridgauss_zt_init);
else if ( !strcmp(name,"scat2gridgauss_zt_work_size_") ) return (void *)FORTRAN(scat2gridgauss_zt_work_size);
else if ( !strcmp(name,"scat2gridgauss_zt_compute_") ) return (void *)FORTRAN(scat2gridgauss_zt_compute);

/* scat2gridgauss_xy_v0.F */
else if ( !strcmp(name,"scat2gridgauss_xy_v0_init_") ) return (void *)FORTRAN(scat2gridgauss_xy_v0_init);
else if ( !strcmp(name,"scat2gridgauss_xy_v0_work_size_") ) return (void *)FORTRAN(scat2gridgauss_xy_v0_work_size);
else if ( !strcmp(name,"scat2gridgauss_xy_v0_compute_") ) return (void *)FORTRAN(scat2gridgauss_xy_v0_compute);

/* scat2gridgauss_xz.F */
else if ( !strcmp(name,"scat2gridgauss_xz_v0_init_") ) return (void *)FORTRAN(scat2gridgauss_xz_v0_init);
else if ( !strcmp(name,"scat2gridgauss_xz_v0_work_size_") ) return (void *)FORTRAN(scat2gridgauss_xz_v0_work_size);
else if ( !strcmp(name,"scat2gridgauss_xz_v0_compute_") ) return (void *)FORTRAN(scat2gridgauss_xz_v0_compute);

/* scat2gridgauss_yz.F */
else if ( !strcmp(name,"scat2gridgauss_yz_v0_init_") ) return (void *)FORTRAN(scat2gridgauss_yz_v0_init);
else if ( !strcmp(name,"scat2gridgauss_yz_v0_work_size_") ) return (void *)FORTRAN(scat2gridgauss_yz_v0_work_size);
else if ( !strcmp(name,"scat2gridgauss_yz_v0_compute_") ) return (void *)FORTRAN(scat2gridgauss_yz_v0_compute);

/* scat2gridgauss_xt.F */
else if ( !strcmp(name,"scat2gridgauss_xt_v0_init_") ) return (void *)FORTRAN(scat2gridgauss_xt_v0_init);
else if ( !strcmp(name,"scat2gridgauss_xt_v0_work_size_") ) return (void *)FORTRAN(scat2gridgauss_xt_v0_work_size);
else if ( !strcmp(name,"scat2gridgauss_xt_v0_compute_") ) return (void *)FORTRAN(scat2gridgauss_xt_v0_compute);

/* scat2gridgauss_yt.F */
else if ( !strcmp(name,"scat2gridgauss_yt_v0_init_") ) return (void *)FORTRAN(scat2gridgauss_yt_v0_init);
else if ( !strcmp(name,"scat2gridgauss_yt_v0_work_size_") ) return (void *)FORTRAN(scat2gridgauss_yt_v0_work_size);
else if ( !strcmp(name,"scat2gridgauss_yt_v0_compute_") ) return (void *)FORTRAN(scat2gridgauss_yt_v0_compute);

/* scat2gridgauss_zt.F */
else if ( !strcmp(name,"scat2gridgauss_zt_v0_init_") ) return (void *)FORTRAN(scat2gridgauss_zt_v0_init);
else if ( !strcmp(name,"scat2gridgauss_zt_v0_work_size_") ) return (void *)FORTRAN(scat2gridgauss_zt_v0_work_size);
else if ( !strcmp(name,"scat2gridgauss_zt_v0_compute_") ) return (void *)FORTRAN(scat2gridgauss_zt_v0_compute);

/* scat2gridlaplace_xy.F */
else if ( !strcmp(name,"scat2gridlaplace_xy_init_") ) return (void *)FORTRAN(scat2gridlaplace_xy_init);
else if ( !strcmp(name,"scat2gridlaplace_xy_work_size_") ) return (void *)FORTRAN(scat2gridlaplace_xy_work_size);
else if ( !strcmp(name,"scat2gridlaplace_xy_compute_") ) return (void *)FORTRAN(scat2gridlaplace_xy_compute);

/* scat2gridlaplace_xz.F */
else if ( !strcmp(name,"scat2gridlaplace_xz_init_") ) return (void *)FORTRAN(scat2gridlaplace_xz_init);
else if ( !strcmp(name,"scat2gridlaplace_xz_work_size_") ) return (void *)FORTRAN(scat2gridlaplace_xz_work_size);
else if ( !strcmp(name,"scat2gridlaplace_xz_compute_") ) return (void *)FORTRAN(scat2gridlaplace_xz_compute);

/* scat2gridlaplace_yz.F */
else if ( !strcmp(name,"scat2gridlaplace_yz_init_") ) return (void *)FORTRAN(scat2gridlaplace_yz_init);
else if ( !strcmp(name,"scat2gridlaplace_yz_work_size_") ) return (void *)FORTRAN(scat2gridlaplace_yz_work_size);
else if ( !strcmp(name,"scat2gridlaplace_yz_compute_") ) return (void *)FORTRAN(scat2gridlaplace_yz_compute);

/* scat2gridlaplace_xt.F */
else if ( !strcmp(name,"scat2gridlaplace_xt_init_") ) return (void *)FORTRAN(scat2gridlaplace_xt_init);
else if ( !strcmp(name,"scat2gridlaplace_xt_work_size_") ) return (void *)FORTRAN(scat2gridlaplace_xt_work_size);
else if ( !strcmp(name,"scat2gridlaplace_xt_compute_") ) return (void *)FORTRAN(scat2gridlaplace_xt_compute);

/* scat2gridlaplace_yt.F */
else if ( !strcmp(name,"scat2gridlaplace_yt_init_") ) return (void *)FORTRAN(scat2gridlaplace_yt_init);
else if ( !strcmp(name,"scat2gridlaplace_yt_work_size_") ) return (void *)FORTRAN(scat2gridlaplace_yt_work_size);
else if ( !strcmp(name,"scat2gridlaplace_yt_compute_") ) return (void *)FORTRAN(scat2gridlaplace_yt_compute);

/* scat2gridlaplace_zt.F */
else if ( !strcmp(name,"scat2gridlaplace_zt_init_") ) return (void *)FORTRAN(scat2gridlaplace_zt_init);
else if ( !strcmp(name,"scat2gridlaplace_zt_work_size_") ) return (void *)FORTRAN(scat2gridlaplace_zt_work_size);
else if ( !strcmp(name,"scat2gridlaplace_zt_compute_") ) return (void *)FORTRAN(scat2gridlaplace_zt_compute);

/* scat2grid_nobs_xy.F */
else if ( !strcmp(name,"scat2grid_nobs_xy_init_") ) return (void *)FORTRAN(scat2grid_nobs_xy_init);
else if ( !strcmp(name,"scat2grid_nobs_xy_work_size_") ) return (void *)FORTRAN(scat2grid_nobs_xy_work_size);
else if ( !strcmp(name,"scat2grid_nobs_xy_compute_") ) return (void *)FORTRAN(scat2grid_nobs_xy_compute);

else if ( !strcmp(name,"scat2grid_nobs_xyt_init_") ) return (void *)FORTRAN(scat2grid_nobs_xyt_init);
else if ( !strcmp(name,"scat2grid_nobs_xyt_work_size_") ) return (void *)FORTRAN(scat2grid_nobs_xyt_work_size);
else if ( !strcmp(name,"scat2grid_nobs_xyt_compute_") ) return (void *)FORTRAN(scat2grid_nobs_xyt_compute);

/* sorti.F */
else if ( !strcmp(name,"sorti_init_") ) return (void *)FORTRAN(sorti_init);
else if ( !strcmp(name,"sorti_result_limits_") ) return (void *)FORTRAN(sorti_result_limits);
else if ( !strcmp(name,"sorti_work_size_") ) return (void *)FORTRAN(sorti_work_size);
else if ( !strcmp(name,"sorti_compute_") ) return (void *)FORTRAN(sorti_compute);

/* sorti_str.F */
else if ( !strcmp(name,"sorti_str_init_") ) return (void *)FORTRAN(sorti_str_init);
else if ( !strcmp(name,"sorti_str_result_limits_") ) return (void *)FORTRAN(sorti_str_result_limits);
else if ( !strcmp(name,"sorti_str_work_size_") ) return (void *)FORTRAN(sorti_str_work_size);
else if ( !strcmp(name,"sorti_str_compute_") ) return (void *)FORTRAN(sorti_str_compute);

/* sortj.F */
else if ( !strcmp(name,"sortj_init_") ) return (void *)FORTRAN(sortj_init);
else if ( !strcmp(name,"sortj_result_limits_") ) return (void *)FORTRAN(sortj_result_limits);
else if ( !strcmp(name,"sortj_work_size_") ) return (void *)FORTRAN(sortj_work_size);
else if ( !strcmp(name,"sortj_compute_") ) return (void *)FORTRAN(sortj_compute);

/* sortj_str.F */
else if ( !strcmp(name,"sortj_str_init_") ) return (void *)FORTRAN(sortj_str_init);
else if ( !strcmp(name,"sortj_str_result_limits_") ) return (void *)FORTRAN(sortj_str_result_limits);
else if ( !strcmp(name,"sortj_str_work_size_") ) return (void *)FORTRAN(sortj_str_work_size);
else if ( !strcmp(name,"sortj_str_compute_") ) return (void *)FORTRAN(sortj_str_compute);

/* sortk.F */
else if ( !strcmp(name,"sortk_init_") ) return (void *)FORTRAN(sortk_init);
else if ( !strcmp(name,"sortk_result_limits_") ) return (void *)FORTRAN(sortk_result_limits);
else if ( !strcmp(name,"sortk_work_size_") ) return (void *)FORTRAN(sortk_work_size);
else if ( !strcmp(name,"sortk_compute_") ) return (void *)FORTRAN(sortk_compute);

/* sortk_str.F */
else if ( !strcmp(name,"sortk_str_init_") ) return (void *)FORTRAN(sortk_str_init);
else if ( !strcmp(name,"sortk_str_result_limits_") ) return (void *)FORTRAN(sortk_str_result_limits);
else if ( !strcmp(name,"sortk_str_work_size_") ) return (void *)FORTRAN(sortk_str_work_size);
else if ( !strcmp(name,"sortk_str_compute_") ) return (void *)FORTRAN(sortk_str_compute);

/* sortl.F */
else if ( !strcmp(name,"sortl_init_") ) return (void *)FORTRAN(sortl_init);
else if ( !strcmp(name,"sortl_result_limits_") ) return (void *)FORTRAN(sortl_result_limits);
else if ( !strcmp(name,"sortl_work_size_") ) return (void *)FORTRAN(sortl_work_size);
else if ( !strcmp(name,"sortl_compute_") ) return (void *)FORTRAN(sortl_compute);

/* sortl_str.F */
else if ( !strcmp(name,"sortl_str_init_") ) return (void *)FORTRAN(sortl_str_init);
else if ( !strcmp(name,"sortl_str_result_limits_") ) return (void *)FORTRAN(sortl_str_result_limits);
else if ( !strcmp(name,"sortl_str_work_size_") ) return (void *)FORTRAN(sortl_str_work_size);
else if ( !strcmp(name,"sortl_str_compute_") ) return (void *)FORTRAN(sortl_str_compute);

/* sortm.F */
else if ( !strcmp(name,"sortm_init_") ) return (void *)FORTRAN(sortm_init);
else if ( !strcmp(name,"sortm_result_limits_") ) return (void *)FORTRAN(sortm_result_limits);
else if ( !strcmp(name,"sortm_work_size_") ) return (void *)FORTRAN(sortm_work_size);
else if ( !strcmp(name,"sortm_compute_") ) return (void *)FORTRAN(sortm_compute);

/* sortm_str.F */
else if ( !strcmp(name,"sortm_str_init_") ) return (void *)FORTRAN(sortm_str_init);
else if ( !strcmp(name,"sortm_str_result_limits_") ) return (void *)FORTRAN(sortm_str_result_limits);
else if ( !strcmp(name,"sortm_str_work_size_") ) return (void *)FORTRAN(sortm_str_work_size);
else if ( !strcmp(name,"sortm_str_compute_") ) return (void *)FORTRAN(sortm_str_compute);

/* sortn.F */
else if ( !strcmp(name,"sortn_init_") ) return (void *)FORTRAN(sortn_init);
else if ( !strcmp(name,"sortn_result_limits_") ) return (void *)FORTRAN(sortn_result_limits);
else if ( !strcmp(name,"sortn_work_size_") ) return (void *)FORTRAN(sortn_work_size);
else if ( !strcmp(name,"sortn_compute_") ) return (void *)FORTRAN(sortn_compute);

/* sortn_str.F */
else if ( !strcmp(name,"sortn_str_init_") ) return (void *)FORTRAN(sortn_str_init);
else if ( !strcmp(name,"sortn_str_result_limits_") ) return (void *)FORTRAN(sortn_str_result_limits);
else if ( !strcmp(name,"sortn_str_work_size_") ) return (void *)FORTRAN(sortn_str_work_size);
else if ( !strcmp(name,"sortn_str_compute_") ) return (void *)FORTRAN(sortn_str_compute);

/* tauto_cor.F */
else if ( !strcmp(name,"tauto_cor_init_") ) return (void *)FORTRAN(tauto_cor_init);
else if ( !strcmp(name,"tauto_cor_result_limits_") ) return (void *)FORTRAN(tauto_cor_result_limits);
else if ( !strcmp(name,"tauto_cor_work_size_") ) return (void *)FORTRAN(tauto_cor_work_size);
else if ( !strcmp(name,"tauto_cor_compute_") ) return (void *)FORTRAN(tauto_cor_compute);

/* xauto_cor.F */
else if ( !strcmp(name,"xauto_cor_init_") ) return (void *)FORTRAN(xauto_cor_init);
else if ( !strcmp(name,"xauto_cor_result_limits_") ) return (void *)FORTRAN(xauto_cor_result_limits);
else if ( !strcmp(name,"xauto_cor_work_size_") ) return (void *)FORTRAN(xauto_cor_work_size);
else if ( !strcmp(name,"xauto_cor_compute_") ) return (void *)FORTRAN(xauto_cor_compute);

/* eof_space.F */
else if ( !strcmp(name,"eof_space_init_") ) return (void *)FORTRAN(eof_space_init);
else if ( !strcmp(name,"eof_space_result_limits_") ) return (void *)FORTRAN(eof_space_result_limits);
else if ( !strcmp(name,"eof_space_work_size_") ) return (void *)FORTRAN(eof_space_work_size);
else if ( !strcmp(name,"eof_space_compute_") ) return (void *)FORTRAN(eof_space_compute);

/* eof_stat.F */
else if ( !strcmp(name,"eof_stat_init_") ) return (void *)FORTRAN(eof_stat_init);
else if ( !strcmp(name,"eof_stat_result_limits_") ) return (void *)FORTRAN(eof_stat_result_limits);
else if ( !strcmp(name,"eof_stat_work_size_") ) return (void *)FORTRAN(eof_stat_work_size);
else if ( !strcmp(name,"eof_stat_compute_") ) return (void *)FORTRAN(eof_stat_compute);

/* eof_tfunc.F */
else if ( !strcmp(name,"eof_tfunc_init_") ) return (void *)FORTRAN(eof_tfunc_init);
else if ( !strcmp(name,"eof_tfunc_result_limits_") ) return (void *)FORTRAN(eof_tfunc_result_limits);
else if ( !strcmp(name,"eof_tfunc_work_size_") ) return (void *)FORTRAN(eof_tfunc_work_size);
else if ( !strcmp(name,"eof_tfunc_compute_") ) return (void *)FORTRAN(eof_tfunc_compute);

/* eofsvd_space.F */
else if ( !strcmp(name,"eofsvd_space_init_") ) return (void *)FORTRAN(eofsvd_space_init);
else if ( !strcmp(name,"eofsvd_space_result_limits_") ) return (void *)FORTRAN(eofsvd_space_result_limits);
else if ( !strcmp(name,"eofsvd_space_work_size_") ) return (void *)FORTRAN(eofsvd_space_work_size);
else if ( !strcmp(name,"eofsvd_space_compute_") ) return (void *)FORTRAN(eofsvd_space_compute);

/* eofsvd_stat.F */
else if ( !strcmp(name,"eofsvd_stat_init_") ) return (void *)FORTRAN(eofsvd_stat_init);
else if ( !strcmp(name,"eofsvd_stat_result_limits_") ) return (void *)FORTRAN(eofsvd_stat_result_limits);
else if ( !strcmp(name,"eofsvd_stat_work_size_") ) return (void *)FORTRAN(eofsvd_stat_work_size);
else if ( !strcmp(name,"eofsvd_stat_compute_") ) return (void *)FORTRAN(eofsvd_stat_compute);

/* eofsvd_tfunc.F */
else if ( !strcmp(name,"eofsvd_tfunc_init_") ) return (void *)FORTRAN(eofsvd_tfunc_init);
else if ( !strcmp(name,"eofsvd_tfunc_result_limits_") ) return (void *)FORTRAN(eofsvd_tfunc_result_limits);
else if ( !strcmp(name,"eofsvd_tfunc_work_size_") ) return (void *)FORTRAN(eofsvd_tfunc_work_size);
else if ( !strcmp(name,"eofsvd_tfunc_compute_") ) return (void *)FORTRAN(eofsvd_tfunc_compute);

/* compressi.F */
else if ( !strcmp(name,"compressi_init_") ) return (void *)FORTRAN(compressi_init);
else if ( !strcmp(name,"compressi_result_limits_") ) return (void *)FORTRAN(compressi_result_limits);
else if ( !strcmp(name,"compressi_compute_") ) return (void *)FORTRAN(compressi_compute);

/* compressj.F */
else if ( !strcmp(name,"compressj_init_") ) return (void *)FORTRAN(compressj_init);
else if ( !strcmp(name,"compressj_result_limits_") ) return (void *)FORTRAN(compressj_result_limits);
else if ( !strcmp(name,"compressj_compute_") ) return (void *)FORTRAN(compressj_compute);

/* compressk.F */
else if ( !strcmp(name,"compressk_init_") ) return (void *)FORTRAN(compressk_init);
else if ( !strcmp(name,"compressk_result_limits_") ) return (void *)FORTRAN(compressk_result_limits);
else if ( !strcmp(name,"compressk_compute_") ) return (void *)FORTRAN(compressk_compute);

/* compressl.F */
else if ( !strcmp(name,"compressl_init_") ) return (void *)FORTRAN(compressl_init);
else if ( !strcmp(name,"compressl_result_limits_") ) return (void *)FORTRAN(compressl_result_limits);
else if ( !strcmp(name,"compressl_compute_") ) return (void *)FORTRAN(compressl_compute);

/* compressm.F */
else if ( !strcmp(name,"compressm_init_") ) return (void *)FORTRAN(compressm_init);
else if ( !strcmp(name,"compressm_result_limits_") ) return (void *)FORTRAN(compressm_result_limits);
else if ( !strcmp(name,"compressm_compute_") ) return (void *)FORTRAN(compressm_compute);

/* compressn.F */
else if ( !strcmp(name,"compressn_init_") ) return (void *)FORTRAN(compressn_init);
else if ( !strcmp(name,"compressn_result_limits_") ) return (void *)FORTRAN(compressn_result_limits);
else if ( !strcmp(name,"compressn_compute_") ) return (void *)FORTRAN(compressn_compute);

/* compressi_by.F */
else if ( !strcmp(name,"compressi_by_init_") ) return (void *)FORTRAN(compressi_by_init);
else if ( !strcmp(name,"compressi_by_result_limits_") ) return (void *)FORTRAN(compressi_by_result_limits);
else if ( !strcmp(name,"compressi_by_compute_") ) return (void *)FORTRAN(compressi_by_compute);

/* compressj_by.F */
else if ( !strcmp(name,"compressj_by_init_") ) return (void *)FORTRAN(compressj_by_init);
else if ( !strcmp(name,"compressj_by_result_limits_") ) return (void *)FORTRAN(compressj_by_result_limits);
else if ( !strcmp(name,"compressj_by_compute_") ) return (void *)FORTRAN(compressj_by_compute);

/* compressk_by.F */
else if ( !strcmp(name,"compressk_by_init_") ) return (void *)FORTRAN(compressk_by_init);
else if ( !strcmp(name,"compressk_by_result_limits_") ) return (void *)FORTRAN(compressk_by_result_limits);
else if ( !strcmp(name,"compressk_by_compute_") ) return (void *)FORTRAN(compressk_by_compute);

/* compressl_by.F */
else if ( !strcmp(name,"compressl_by_init_") ) return (void *)FORTRAN(compressl_by_init);
else if ( !strcmp(name,"compressl_by_result_limits_") ) return (void *)FORTRAN(compressl_by_result_limits);
else if ( !strcmp(name,"compressl_by_compute_") ) return (void *)FORTRAN(compressl_by_compute);

/* compressm_by.F */
else if ( !strcmp(name,"compressm_by_init_") ) return (void *)FORTRAN(compressm_by_init);
else if ( !strcmp(name,"compressm_by_result_limits_") ) return (void *)FORTRAN(compressm_by_result_limits);
else if ( !strcmp(name,"compressm_by_compute_") ) return (void *)FORTRAN(compressm_by_compute);

/* compressn_by.F */
else if ( !strcmp(name,"compressn_by_init_") ) return (void *)FORTRAN(compressn_by_init);
else if ( !strcmp(name,"compressn_by_result_limits_") ) return (void *)FORTRAN(compressn_by_result_limits);
else if ( !strcmp(name,"compressn_by_compute_") ) return (void *)FORTRAN(compressn_by_compute);

/* labwid.F */
else if ( !strcmp(name,"labwid_init_") ) return (void *)FORTRAN(labwid_init);
else if ( !strcmp(name,"labwid_result_limits_") ) return (void *)FORTRAN(labwid_result_limits);
else if ( !strcmp(name,"labwid_compute_") ) return (void *)FORTRAN(labwid_compute);

/* convolvei.F */
else if ( !strcmp(name,"convolvei_init_") ) return (void *)FORTRAN(convolvei_init);
else if ( !strcmp(name,"convolvei_compute_") ) return (void *)FORTRAN(convolvei_compute);

/* convolvej.F */
else if ( !strcmp(name,"convolvej_init_") ) return (void *)FORTRAN(convolvej_init);
else if ( !strcmp(name,"convolvej_compute_") ) return (void *)FORTRAN(convolvej_compute);

/* convolvek.F */
else if ( !strcmp(name,"convolvek_init_") ) return (void *)FORTRAN(convolvek_init);
else if ( !strcmp(name,"convolvek_compute_") ) return (void *)FORTRAN(convolvek_compute);

/* convolvel.F */
else if ( !strcmp(name,"convolvel_init_") ) return (void *)FORTRAN(convolvel_init);
else if ( !strcmp(name,"convolvel_compute_") ) return (void *)FORTRAN(convolvel_compute);

/* convolvem.F */
else if ( !strcmp(name,"convolvem_init_") ) return (void *)FORTRAN(convolvem_init);
else if ( !strcmp(name,"convolvem_compute_") ) return (void *)FORTRAN(convolvem_compute);

/* convolven.F */
else if ( !strcmp(name,"convolven_init_") ) return (void *)FORTRAN(convolven_init);
else if ( !strcmp(name,"convolven_compute_") ) return (void *)FORTRAN(convolven_compute);

/* curv_range.F */
else if ( !strcmp(name,"curv_range_init_") ) return (void *)FORTRAN(curv_range_init);
else if ( !strcmp(name,"curv_range_result_limits_") ) return (void *)FORTRAN(curv_range_result_limits);
else if ( !strcmp(name,"curv_range_compute_") ) return (void *)FORTRAN(curv_range_compute);

/* curv_to_rect_map.F */
else if ( !strcmp(name,"curv_to_rect_map_init_") ) return (void *)FORTRAN(curv_to_rect_map_init);
else if ( !strcmp(name,"curv_to_rect_map_result_limits_") ) return (void *)FORTRAN(curv_to_rect_map_result_limits);
else if ( !strcmp(name,"curv_to_rect_map_work_size_") ) return (void *)FORTRAN(curv_to_rect_map_work_size);
else if ( !strcmp(name,"curv_to_rect_map_compute_") ) return (void *)FORTRAN(curv_to_rect_map_compute);

/* curv_to_rect.F */
else if ( !strcmp(name,"curv_to_rect_init_") ) return (void *)FORTRAN(curv_to_rect_init);
else if ( !strcmp(name,"curv_to_rect_compute_") ) return (void *)FORTRAN(curv_to_rect_compute);

/* rect_to_curv.F */
else if ( !strcmp(name,"rect_to_curv_init_") ) return (void *)FORTRAN(rect_to_curv_init);
else if ( !strcmp(name,"rect_to_curv_work_size_") ) return (void *)FORTRAN(rect_to_curv_work_size);
else if ( !strcmp(name,"rect_to_curv_compute_") ) return (void *)FORTRAN(rect_to_curv_compute);

/* date1900.F */
else if ( !strcmp(name,"date1900_init_") ) return (void *)FORTRAN(date1900_init);
else if ( !strcmp(name,"date1900_result_limits_") ) return (void *)FORTRAN(date1900_result_limits);
else if ( !strcmp(name,"date1900_compute_") ) return (void *)FORTRAN(date1900_compute);

/* days1900toymdhms.F */
else if ( !strcmp(name,"days1900toymdhms_init_") ) return (void *)FORTRAN(days1900toymdhms_init);
else if ( !strcmp(name,"days1900toymdhms_result_limits_") ) return (void *)FORTRAN(days1900toymdhms_result_limits);
else if ( !strcmp(name,"days1900toymdhms_compute_") ) return (void *)FORTRAN(days1900toymdhms_compute);

/* minutes24.F */
else if ( !strcmp(name,"minutes24_init_") ) return (void *)FORTRAN(minutes24_init);
else if ( !strcmp(name,"minutes24_result_limits_") ) return (void *)FORTRAN(minutes24_result_limits);
else if ( !strcmp(name,"minutes24_compute_") ) return (void *)FORTRAN(minutes24_compute);

/* element_index.F */
else if ( !strcmp(name,"element_index_init_") ) return (void *)FORTRAN(element_index_init);
else if ( !strcmp(name,"element_index_compute_") ) return (void *)FORTRAN(element_index_compute);

/* element_index_str.F */
else if ( !strcmp(name,"element_index_str_init_") ) return (void *)FORTRAN(element_index_str_init);
else if ( !strcmp(name,"element_index_str_compute_") ) return (void *)FORTRAN(element_index_str_compute);

/* element_index_str_n.F */
else if ( !strcmp(name,"element_index_str_n_init_") ) return (void *)FORTRAN(element_index_str_n_init);
else if ( !strcmp(name,"element_index_str_n_compute_") ) return (void *)FORTRAN(element_index_str_n_compute);

/* expnd_by_len.F */
else if ( !strcmp(name,"expnd_by_len_init_") ) return (void *)FORTRAN(expnd_by_len_init);
else if ( !strcmp(name,"expnd_by_len_result_limits_") ) return (void *)FORTRAN(expnd_by_len_result_limits);
else if ( !strcmp(name,"expnd_by_len_compute_") ) return (void *)FORTRAN(expnd_by_len_compute);

/* expnd_by_len_str.F */
else if ( !strcmp(name,"expnd_by_len_str_init_") ) return (void *)FORTRAN(expnd_by_len_str_init);
else if ( !strcmp(name,"expnd_by_len_str_result_limits_") ) return (void *)FORTRAN(expnd_by_len_str_result_limits);
else if ( !strcmp(name,"expnd_by_len_str_compute_") ) return (void *)FORTRAN(expnd_by_len_str_compute);

/* expndi_by.F */
else if ( !strcmp(name,"expndi_by_init_") ) return (void *)FORTRAN(expndi_by_init);
else if ( !strcmp(name,"expndi_by_result_limits_") ) return (void *)FORTRAN(expndi_by_result_limits);
else if ( !strcmp(name,"expndi_by_compute_") ) return (void *)FORTRAN(expndi_by_compute);

/* expndi_by_t.F */
else if ( !strcmp(name,"expndi_by_t_init_") ) return (void *)FORTRAN(expndi_by_t_init);
else if ( !strcmp(name,"expndi_by_t_result_limits_") ) return (void *)FORTRAN(expndi_by_t_result_limits);
else if ( !strcmp(name,"expndi_by_t_compute_") ) return (void *)FORTRAN(expndi_by_t_compute);

/* expndi_by_z.F */
else if ( !strcmp(name,"expndi_by_z_init_") ) return (void *)FORTRAN(expndi_by_z_init);
else if ( !strcmp(name,"expndi_by_z_result_limits_") ) return (void *)FORTRAN(expndi_by_z_result_limits);
else if ( !strcmp(name,"expndi_by_z_compute_") ) return (void *)FORTRAN(expndi_by_z_compute);

/* expndi_by_z_counts.F */
else if ( !strcmp(name,"expndi_by_z_counts_init_") ) return (void *)FORTRAN(expndi_by_z_counts_init);
else if ( !strcmp(name,"expndi_by_z_counts_result_limits_") ) return (void *)FORTRAN(expndi_by_z_counts_result_limits);
else if ( !strcmp(name,"expndi_by_z_counts_compute_") ) return (void *)FORTRAN(expndi_by_z_counts_compute);

/* expndi_id_by_z_counts.F */
else if ( !strcmp(name,"expndi_id_by_z_counts_init_") ) return (void *)FORTRAN(expndi_id_by_z_counts_init);
else if ( !strcmp(name,"expndi_id_by_z_counts_result_limits_") ) return (void *)FORTRAN(expndi_id_by_z_counts_result_limits);
else if ( !strcmp(name,"expndi_id_by_z_counts_compute_") ) return (void *)FORTRAN(expndi_id_by_z_counts_compute);

/* expndi_by_m_counts.F */
else if ( !strcmp(name,"expndi_by_m_counts_init_") ) return (void *)FORTRAN(expndi_by_m_counts_init);
else if ( !strcmp(name,"expndi_by_m_counts_custom_axes_") ) return (void *)FORTRAN(expndi_by_m_counts_custom_axes);
else if ( !strcmp(name,"expndi_by_m_counts_compute_") ) return (void *)FORTRAN(expndi_by_m_counts_compute);

/* fc_isubset.F */
else if ( !strcmp(name,"fc_isubset_init_") ) return (void *)FORTRAN(fc_isubset_init);
else if ( !strcmp(name,"fc_isubset_result_limits_") ) return (void *)FORTRAN(fc_isubset_result_limits);
else if ( !strcmp(name,"fc_isubset_custom_axes_") ) return (void *)FORTRAN(fc_isubset_custom_axes);
else if ( !strcmp(name,"fc_isubset_compute_") ) return (void *)FORTRAN(fc_isubset_compute);

/* findhi.F */
else if ( !strcmp(name,"findhi_init_") ) return (void *)FORTRAN(findhi_init);
else if ( !strcmp(name,"findhi_result_limits_") ) return (void *)FORTRAN(findhi_result_limits);
else if ( !strcmp(name,"findhi_work_size_") ) return (void *)FORTRAN(findhi_work_size);
else if ( !strcmp(name,"findhi_compute_") ) return (void *)FORTRAN(findhi_compute);

/* findlo.F */
else if ( !strcmp(name,"findlo_init_") ) return (void *)FORTRAN(findlo_init);
else if ( !strcmp(name,"findlo_result_limits_") ) return (void *)FORTRAN(findlo_result_limits);
else if ( !strcmp(name,"findlo_work_size_") ) return (void *)FORTRAN(findlo_work_size);
else if ( !strcmp(name,"findlo_compute_") ) return (void *)FORTRAN(findlo_compute);

/* is_element_of.F */
else if ( !strcmp(name,"is_element_of_init_") ) return (void *)FORTRAN(is_element_of_init);
else if ( !strcmp(name,"is_element_of_result_limits_") ) return (void *)FORTRAN(is_element_of_result_limits);
else if ( !strcmp(name,"is_element_of_compute_") ) return (void *)FORTRAN(is_element_of_compute);

/* is_element_of_str.F */
else if ( !strcmp(name,"is_element_of_str_init_") ) return (void *)FORTRAN(is_element_of_str_init);
else if ( !strcmp(name,"is_element_of_str_result_limits_") ) return (void *)FORTRAN(is_element_of_str_result_limits);
else if ( !strcmp(name,"is_element_of_str_compute_") ) return (void *)FORTRAN(is_element_of_str_compute);

/* is_element_of_str_n.F */
else if ( !strcmp(name,"is_element_of_str_n_init_") ) return (void *)FORTRAN(is_element_of_str_n_init);
else if ( !strcmp(name,"is_element_of_str_n_result_limits_") ) return (void *)FORTRAN(is_element_of_str_n_result_limits);
else if ( !strcmp(name,"is_element_of_str_n_compute_") ) return (void *)FORTRAN(is_element_of_str_n_compute);

/* lanczos.F */
else if ( !strcmp(name,"lanczos_init_") ) return (void *)FORTRAN(lanczos_init);
else if ( !strcmp(name,"lanczos_work_size_") ) return (void *)FORTRAN(lanczos_work_size);
else if ( !strcmp(name,"lanczos_compute_") ) return (void *)FORTRAN(lanczos_compute);

/* lsl_lowpass.F */
else if ( !strcmp(name,"lsl_lowpass_init_") ) return (void *)FORTRAN(lsl_lowpass_init);
else if ( !strcmp(name,"lsl_lowpass_work_size_") ) return (void *)FORTRAN(lsl_lowpass_work_size);
else if ( !strcmp(name,"lsl_lowpass_compute_") ) return (void *)FORTRAN(lsl_lowpass_compute);

/* scat2grid_t.F */
else if ( !strcmp(name,"scat2grid_t_init_") ) return (void *)FORTRAN(scat2grid_t_init);
else if ( !strcmp(name,"scat2grid_t_work_size_") ) return (void *)FORTRAN(scat2grid_t_work_size);
else if ( !strcmp(name,"scat2grid_t_compute_") ) return (void *)FORTRAN(scat2grid_t_compute);

/* ave_scat2grid_t.F */
else if ( !strcmp(name,"ave_scat2grid_t_init_") ) return (void *)FORTRAN(ave_scat2grid_t_init);
else if ( !strcmp(name,"ave_scat2grid_t_work_size_") ) return (void *)FORTRAN(ave_scat2grid_t_work_size);
else if ( !strcmp(name,"ave_scat2grid_t_compute_") ) return (void *)FORTRAN(ave_scat2grid_t_compute);

/* scat2ddups.F */
else if ( !strcmp(name,"scat2ddups_init_") ) return (void *)FORTRAN(scat2ddups_init);
else if ( !strcmp(name,"scat2ddups_result_limits_") ) return (void *)FORTRAN(scat2ddups_result_limits);
else if ( !strcmp(name,"scat2ddups_compute_") ) return (void *)FORTRAN(scat2ddups_compute);

/* transpose_xt.F */
else if ( !strcmp(name,"transpose_xt_init_") ) return (void *)FORTRAN(transpose_xt_init);
else if ( !strcmp(name,"transpose_xt_result_limits_") ) return (void *)FORTRAN(transpose_xt_result_limits);
else if ( !strcmp(name,"transpose_xt_compute_") ) return (void *)FORTRAN(transpose_xt_compute);

/* transpose_xy.F */
else if ( !strcmp(name,"transpose_xy_init_") ) return (void *)FORTRAN(transpose_xy_init);
else if ( !strcmp(name,"transpose_xy_result_limits_") ) return (void *)FORTRAN(transpose_xy_result_limits);
else if ( !strcmp(name,"transpose_xy_compute_") ) return (void *)FORTRAN(transpose_xy_compute);

/* transpose_xz.F */
else if ( !strcmp(name,"transpose_xz_init_") ) return (void *)FORTRAN(transpose_xz_init);
else if ( !strcmp(name,"transpose_xz_result_limits_") ) return (void *)FORTRAN(transpose_xz_result_limits);
else if ( !strcmp(name,"transpose_xz_compute_") ) return (void *)FORTRAN(transpose_xz_compute);

/* transpose_yt.F */
else if ( !strcmp(name,"transpose_yt_init_") ) return (void *)FORTRAN(transpose_yt_init);
else if ( !strcmp(name,"transpose_yt_result_limits_") ) return (void *)FORTRAN(transpose_yt_result_limits);
else if ( !strcmp(name,"transpose_yt_compute_") ) return (void *)FORTRAN(transpose_yt_compute);

/* transpose_yz.F */
else if ( !strcmp(name,"transpose_yz_init_") ) return (void *)FORTRAN(transpose_yz_init);
else if ( !strcmp(name,"transpose_yz_result_limits_") ) return (void *)FORTRAN(transpose_yz_result_limits);
else if ( !strcmp(name,"transpose_yz_compute_") ) return (void *)FORTRAN(transpose_yz_compute);

/* transpose_zt.F */
else if ( !strcmp(name,"transpose_zt_init_") ) return (void *)FORTRAN(transpose_zt_init);
else if ( !strcmp(name,"transpose_zt_result_limits_") ) return (void *)FORTRAN(transpose_zt_result_limits);
else if ( !strcmp(name,"transpose_zt_compute_") ) return (void *)FORTRAN(transpose_zt_compute);

/* xcat.F */
else if ( !strcmp(name,"xcat_init_") ) return (void *)FORTRAN(xcat_init);
else if ( !strcmp(name,"xcat_result_limits_") ) return (void *)FORTRAN(xcat_result_limits);
else if ( !strcmp(name,"xcat_compute_") ) return (void *)FORTRAN(xcat_compute);

/* xcat_str.F */
else if ( !strcmp(name,"xcat_str_init_") ) return (void *)FORTRAN(xcat_str_init);
else if ( !strcmp(name,"xcat_str_result_limits_") ) return (void *)FORTRAN(xcat_str_result_limits);
else if ( !strcmp(name,"xcat_str_compute_") ) return (void *)FORTRAN(xcat_str_compute);

/* ycat.F */
else if ( !strcmp(name,"ycat_init_") ) return (void *)FORTRAN(ycat_init);
else if ( !strcmp(name,"ycat_result_limits_") ) return (void *)FORTRAN(ycat_result_limits);
else if ( !strcmp(name,"ycat_compute_") ) return (void *)FORTRAN(ycat_compute);

/* ycat_str.F */
else if ( !strcmp(name,"ycat_str_init_") ) return (void *)FORTRAN(ycat_str_init);
else if ( !strcmp(name,"ycat_str_result_limits_") ) return (void *)FORTRAN(ycat_str_result_limits);
else if ( !strcmp(name,"ycat_str_compute_") ) return (void *)FORTRAN(ycat_str_compute);

/* zcat.F */
else if ( !strcmp(name,"zcat_init_") ) return (void *)FORTRAN(zcat_init);
else if ( !strcmp(name,"zcat_result_limits_") ) return (void *)FORTRAN(zcat_result_limits);
else if ( !strcmp(name,"zcat_compute_") ) return (void *)FORTRAN(zcat_compute);

/* zcat_str.F */
else if ( !strcmp(name,"zcat_str_init_") ) return (void *)FORTRAN(zcat_str_init);
else if ( !strcmp(name,"zcat_str_result_limits_") ) return (void *)FORTRAN(zcat_str_result_limits);
else if ( !strcmp(name,"zcat_str_compute_") ) return (void *)FORTRAN(zcat_str_compute);

/* tcat.F */
else if ( !strcmp(name,"tcat_init_") ) return (void *)FORTRAN(tcat_init);
else if ( !strcmp(name,"tcat_result_limits_") ) return (void *)FORTRAN(tcat_result_limits);
else if ( !strcmp(name,"tcat_compute_") ) return (void *)FORTRAN(tcat_compute);

/* tcat_str.F */
else if ( !strcmp(name,"tcat_str_init_") ) return (void *)FORTRAN(tcat_str_init);
else if ( !strcmp(name,"tcat_str_result_limits_") ) return (void *)FORTRAN(tcat_str_result_limits);
else if ( !strcmp(name,"tcat_str_compute_") ) return (void *)FORTRAN(tcat_str_compute);

/* ecat.F */
else if ( !strcmp(name,"ecat_init_") ) return (void *)FORTRAN(ecat_init);
else if ( !strcmp(name,"ecat_result_limits_") ) return (void *)FORTRAN(ecat_result_limits);
else if ( !strcmp(name,"ecat_compute_") ) return (void *)FORTRAN(ecat_compute);

/* ecat_str.F */
else if ( !strcmp(name,"ecat_str_init_") ) return (void *)FORTRAN(ecat_str_init);
else if ( !strcmp(name,"ecat_str_result_limits_") ) return (void *)FORTRAN(ecat_str_result_limits);
else if ( !strcmp(name,"ecat_str_compute_") ) return (void *)FORTRAN(ecat_str_compute);

/* fcat.F */
else if ( !strcmp(name,"fcat_init_") ) return (void *)FORTRAN(fcat_init);
else if ( !strcmp(name,"fcat_result_limits_") ) return (void *)FORTRAN(fcat_result_limits);
else if ( !strcmp(name,"fcat_compute_") ) return (void *)FORTRAN(fcat_compute);

/* fcat_str.F */
else if ( !strcmp(name,"fcat_str_init_") ) return (void *)FORTRAN(fcat_str_init);
else if ( !strcmp(name,"fcat_str_result_limits_") ) return (void *)FORTRAN(fcat_str_result_limits);
else if ( !strcmp(name,"fcat_str_compute_") ) return (void *)FORTRAN(fcat_str_compute);

/* xreverse.F */
else if ( !strcmp(name,"xreverse_init_") ) return (void *)FORTRAN(xreverse_init);
else if ( !strcmp(name,"xreverse_result_limits_") ) return (void *)FORTRAN(xreverse_result_limits);
else if ( !strcmp(name,"xreverse_compute_") ) return (void *)FORTRAN(xreverse_compute);

/* yreverse.F */
else if ( !strcmp(name,"yreverse_init_") ) return (void *)FORTRAN(yreverse_init);
else if ( !strcmp(name,"yreverse_result_limits_") ) return (void *)FORTRAN(yreverse_result_limits);
else if ( !strcmp(name,"yreverse_compute_") ) return (void *)FORTRAN(yreverse_compute);

/* zreverse.F */
else if ( !strcmp(name,"zreverse_init_") ) return (void *)FORTRAN(zreverse_init);
else if ( !strcmp(name,"zreverse_result_limits_") ) return (void *)FORTRAN(zreverse_result_limits);
else if ( !strcmp(name,"zreverse_compute_") ) return (void *)FORTRAN(zreverse_compute);

/* treverse.F */
else if ( !strcmp(name,"treverse_init_") ) return (void *)FORTRAN(treverse_init);
else if ( !strcmp(name,"treverse_result_limits_") ) return (void *)FORTRAN(treverse_result_limits);
else if ( !strcmp(name,"treverse_compute_") ) return (void *)FORTRAN(treverse_compute);

/* ereverse.F */
else if ( !strcmp(name,"ereverse_init_") ) return (void *)FORTRAN(ereverse_init);
else if ( !strcmp(name,"ereverse_result_limits_") ) return (void *)FORTRAN(ereverse_result_limits);
else if ( !strcmp(name,"ereverse_compute_") ) return (void *)FORTRAN(ereverse_compute);

/* freverse.F */
else if ( !strcmp(name,"freverse_init_") ) return (void *)FORTRAN(freverse_init);
else if ( !strcmp(name,"freverse_result_limits_") ) return (void *)FORTRAN(freverse_result_limits);
else if ( !strcmp(name,"freverse_compute_") ) return (void *)FORTRAN(freverse_compute);

/* zaxreplace_avg.F */
else if ( !strcmp(name,"zaxreplace_avg_init_") ) return (void *)FORTRAN(zaxreplace_avg_init);
else if ( !strcmp(name,"zaxreplace_avg_work_size_") ) return (void *)FORTRAN(zaxreplace_avg_work_size);
else if ( !strcmp(name,"zaxreplace_avg_compute_") ) return (void *)FORTRAN(zaxreplace_avg_compute);

/* zaxreplace_bin.F */
else if ( !strcmp(name,"zaxreplace_bin_init_") ) return (void *)FORTRAN(zaxreplace_bin_init);
else if ( !strcmp(name,"zaxreplace_bin_work_size_") ) return (void *)FORTRAN(zaxreplace_bin_work_size);
else if ( !strcmp(name,"zaxreplace_bin_compute_") ) return (void *)FORTRAN(zaxreplace_bin_compute);

/* zaxreplace_rev.F */
else if ( !strcmp(name,"zaxreplace_rev_init_") ) return (void *)FORTRAN(zaxreplace_rev_init);
else if ( !strcmp(name,"zaxreplace_rev_compute_") ) return (void *)FORTRAN(zaxreplace_rev_compute);

/* zaxreplace_zlev.F */
else if ( !strcmp(name,"zaxreplace_zlev_init_") ) return (void *)FORTRAN(zaxreplace_zlev_init);
else if ( !strcmp(name,"zaxreplace_zlev_work_size_") ) return (void *)FORTRAN(zaxreplace_zlev_work_size);
else if ( !strcmp(name,"zaxreplace_zlev_compute_") ) return (void *)FORTRAN(zaxreplace_zlev_compute);

/* nco.F */
else if ( !strcmp(name,"nco_init_") ) return (void *)FORTRAN(nco_init);
else if ( !strcmp(name,"nco_result_limits_") ) return (void *)FORTRAN(nco_result_limits);
else if ( !strcmp(name,"nco_compute_") ) return (void *)FORTRAN(nco_compute);

/* nco_attr.F */
else if ( !strcmp(name,"nco_attr_init_") ) return (void *)FORTRAN(nco_attr_init);
else if ( !strcmp(name,"nco_attr_result_limits_") ) return (void *)FORTRAN(nco_attr_result_limits);
else if ( !strcmp(name,"nco_attr_compute_") ) return (void *)FORTRAN(nco_attr_compute);


else if ( !strcmp(name,"tax_datestring_init_") ) return (void *)FORTRAN(tax_datestring_init);
else if ( !strcmp(name,"tax_datestring_work_size_") ) return (void *)FORTRAN(tax_datestring_work_size);
else if ( !strcmp(name,"tax_datestring_compute_") ) return (void *)FORTRAN(tax_datestring_compute);

else if ( !strcmp(name,"tax_day_init_") ) return (void *)FORTRAN(tax_day_init);
else if ( !strcmp(name,"tax_day_work_size_") ) return (void *)FORTRAN(tax_day_work_size);
else if ( !strcmp(name,"tax_day_compute_") ) return (void *)FORTRAN(tax_day_compute);

else if ( !strcmp(name,"tax_dayfrac_init_") ) return (void *)FORTRAN(tax_dayfrac_init);
else if ( !strcmp(name,"tax_dayfrac_work_size_") ) return (void *)FORTRAN(tax_dayfrac_work_size);
else if ( !strcmp(name,"tax_dayfrac_compute_") ) return (void *)FORTRAN(tax_dayfrac_compute);

else if ( !strcmp(name,"tax_jday1900_init_") ) return (void *)FORTRAN(tax_jday1900_init);
else if ( !strcmp(name,"tax_jday1900_work_size_") ) return (void *)FORTRAN(tax_jday1900_work_size);
else if ( !strcmp(name,"tax_jday1900_compute_") ) return (void *)FORTRAN(tax_jday1900_compute);

else if ( !strcmp(name,"tax_jday_init_") ) return (void *)FORTRAN(tax_jday_init);
else if ( !strcmp(name,"tax_jday_work_size_") ) return (void *)FORTRAN(tax_jday_work_size);
else if ( !strcmp(name,"tax_jday_compute_") ) return (void *)FORTRAN(tax_jday_compute);

else if ( !strcmp(name,"tax_month_init_") ) return (void *)FORTRAN(tax_month_init);
else if ( !strcmp(name,"tax_month_work_size_") ) return (void *)FORTRAN(tax_month_work_size);
else if ( !strcmp(name,"tax_month_compute_") ) return (void *)FORTRAN(tax_month_compute);

else if ( !strcmp(name,"tax_times_init_") ) return (void *)FORTRAN(tax_times_init);
else if ( !strcmp(name,"tax_times_compute_") ) return (void *)FORTRAN(tax_times_compute);

else if ( !strcmp(name,"tax_tstep_init_") ) return (void *)FORTRAN(tax_tstep_init);
else if ( !strcmp(name,"tax_tstep_work_size_") ) return (void *)FORTRAN(tax_tstep_work_size);
else if ( !strcmp(name,"tax_tstep_compute_") ) return (void *)FORTRAN(tax_tstep_compute);

else if ( !strcmp(name,"tax_units_init_") ) return (void *)FORTRAN(tax_units_init);
else if ( !strcmp(name,"tax_units_compute_") ) return (void *)FORTRAN(tax_units_compute);

else if ( !strcmp(name,"tax_year_init_") ) return (void *)FORTRAN(tax_year_init);
else if ( !strcmp(name,"tax_year_work_size_") ) return (void *)FORTRAN(tax_year_work_size);
else if ( !strcmp(name,"tax_year_compute_") ) return (void *)FORTRAN(tax_year_compute);

else if ( !strcmp(name,"tax_yearfrac_init_") ) return (void *)FORTRAN(tax_yearfrac_init);
else if ( !strcmp(name,"tax_yearfrac_work_size_") ) return (void *)FORTRAN(tax_yearfrac_work_size);
else if ( !strcmp(name,"tax_yearfrac_compute_") ) return (void *)FORTRAN(tax_yearfrac_compute);

else if ( !strcmp(name,"fill_xy_init_") ) return (void *)FORTRAN(fill_xy_init);
else if ( !strcmp(name,"fill_xy_compute_") ) return (void *)FORTRAN(fill_xy_compute);

else if ( !strcmp(name,"test_opendap_init_") ) return (void *)FORTRAN(test_opendap_init);
else if ( !strcmp(name,"test_opendap_result_limits_") ) return (void *)FORTRAN(test_opendap_result_limits);
else if ( !strcmp(name,"test_opendap_compute_") ) return (void *)FORTRAN(test_opendap_compute);

else if ( !strcmp(name,"unique_str2int_init_") ) return (void *)FORTRAN(unique_str2int_init);
else if ( !strcmp(name,"unique_str2int_compute_") ) return (void *)FORTRAN(unique_str2int_compute);

else if ( !strcmp(name,"bin_index_wt_init_") ) return (void *)FORTRAN(bin_index_wt_init);
else if ( !strcmp(name,"bin_index_wt_result_limits_") ) return (void *)FORTRAN(bin_index_wt_result_limits);
else if ( !strcmp(name,"bin_index_wt_compute_") ) return (void *)FORTRAN(bin_index_wt_compute);

else if ( !strcmp(name,"minmax_init_") ) return (void *)FORTRAN(minmax_init);
else if ( !strcmp(name,"minmax_result_limits_") ) return (void *)FORTRAN(minmax_result_limits);
else if ( !strcmp(name,"minmax_compute_") ) return (void *)FORTRAN(minmax_compute);

else if ( !strcmp(name,"floatstr_init_") ) return (void *)FORTRAN(floatstr_init);
else if ( !strcmp(name,"floatstr_compute_") ) return (void *)FORTRAN(floatstr_compute);

else if ( !strcmp(name,"pt_in_poly_init_") ) return (void *)FORTRAN(pt_in_poly_init);
else if ( !strcmp(name,"pt_in_poly_work_size_") ) return (void *)FORTRAN(pt_in_poly_work_size);
else if ( !strcmp(name,"pt_in_poly_compute_") ) return (void *)FORTRAN(pt_in_poly_compute);

else if ( !strcmp(name,"list_value_xml_init_") ) return (void *)FORTRAN(list_value_xml_init);
else if ( !strcmp(name,"list_value_xml_result_limits_") ) return (void *)FORTRAN(list_value_xml_result_limits);
else if ( !strcmp(name,"list_value_xml_compute_") ) return (void *)FORTRAN(list_value_xml_compute);

else if ( !strcmp(name,"write_webrow_init_") ) return (void *)FORTRAN(write_webrow_init);
else if ( !strcmp(name,"write_webrow_result_limits_") ) return (void *)FORTRAN(write_webrow_result_limits);
else if ( !strcmp(name,"write_webrow_compute_") ) return (void *)FORTRAN(write_webrow_compute);

else if ( !strcmp(name,"str_mask_init_") ) return (void *)FORTRAN(str_mask_init);
else if ( !strcmp(name,"str_mask_compute_") ) return (void *)FORTRAN(str_mask_compute);

else if ( !strcmp(name,"separate_init_") ) return (void *)FORTRAN(separate_init);
else if ( !strcmp(name,"separate_result_limits_") ) return (void *)FORTRAN(separate_result_limits);
else if ( !strcmp(name,"separate_compute_") ) return (void *)FORTRAN(separate_compute);



return NULL;
 }
/*  End of function pointer list for internally-linked External Functions
 *  ------------------------------------ */
