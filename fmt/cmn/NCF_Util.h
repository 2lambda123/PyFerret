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

/* NC_Util.h
 *
 * Ansley Manke
 * Ferret V600 April 2005
 * *acm*  5/12 Additions for creating aggregate datasets
 *
 * This is the header file to be included by routines which
 * are part of the Ferret NetCDF attribute handling library.
 * V683 10/10*acm* New NC_INTERRUPT for user-interrupt reading netCDF/OPeNDAP data
 * V698 12/15 *sh* added aggSeqNo, the sequence number (FORTRAN index) of each dset w/in the agg 
 * V698  2/16 *acm Additions for ticket 2352: LET/D variables and attributes. User-variables
 *                 defined with LET/D=n are stored with dataset n. A flag in the ncvar 
 *                 structure tells that the variable is a user-var. 
 * V699 5/16 *sh* added grid management for uvars: gridList and uvarGrid
 * V701 8/16 *kms* enclose in #ifndef _NCF_UTIL_H_ ... #endif so this is included only once per file;
 *                 include netcdf.h to make sure the NC_... values are defined;
 *                 remove the ...list_initialized... values - just check the list against NULL
 */

#ifndef _NCF_UTIL_H_
#define _NCF_UTIL_H_

#include <Python.h>
#include <netcdf.h>   /* for many NC_... values */
#include "ferret.h"   /* for NFERDIMS */

/* .................... Defines ..................... */

#define TRUE  1
#define FALSE 0
#define YES   1
#define NO    0

#define LO    0
#define HI    1

#define ATOM_NOT_FOUND 0  /* This should match the atom_not_found parameter in ferret.parm. */
#define FERR_OK 3  /* This should match the ferr_ok parameter in errmsg.parm. */
#define PDSET_UVARS -1  /* This should match pdset_uvars ferret.parm */

/* Ferret-defined "netcdf error status" when a read was interrupted by Crtl-C */
/* match nc_interrupt with nf_interrupt in tmap_errors.parm */
#define NC_INTERRUPT 900

#define MAX_PATH_NAME	2048	 /* max length of a path */
#define MAX_FER_SETNAME	256	 /* max length of a path */
#define MAX_FER_SETNAME	256	 /* max length of a path */

/* .................... Typedefs .................... */


typedef struct  {			/* dimension */
    char name[NC_MAX_NAME];
    size_t size;
} ncdim;

typedef struct  {
	char fullpath[MAX_PATH_NAME];
	char fername[MAX_FER_SETNAME];
	LIST *dsetvarlist;
	ncdim dims[NC_MAX_DIMS];
	int ndims;
	int ngatts;
	int recdim;
	int nvars;
	int fer_dsetnum;
	int fer_current;
	int its_epic;
	int its_agg;
	int num_agg_members;
	LIST *agg_dsetlist;
} ncdset;

typedef struct  {          /* variable */
	char name[NC_MAX_NAME];
	LIST *varattlist;
	nc_type type;
	int outtype;
	int ndims;
	int dims[MAX_VAR_DIMS];
	int natts;
	int varid;
        int uvarid;            /* the value of uvar as Ferret knows it, 0 if fvar */
                               /* note that uvflag no longer needed with this */
	int is_axis;           /* coordinate variable */
	int axis_dir;          /* coordinate direction 1,2,3,4, for X,Y,Z,T, etc*/
	int has_fillval;
	int all_outflag;       /* 0 write no attrs, 
	                          1 check individual attr flags
	                          2 write all attrs,
	                          3 reset attr flags to Ferret defaults */
	double fillval;
	LIST *varagglist;      /* if an aggregate dataset, for each var,
	                          list the members of the aggregate components. */
        LIST *uvarGridList;    /* if a uvar, keep track of its grid(s) */
	int nmemb;
} ncvar;

typedef struct {			/* attribute */
	char name[NC_MAX_NAME];
	nc_type type;
	int outtype;
	int attid;
	int outflag;        /* 1 to write this attr, 0 to not write */
	int len;
	void *inval;        /* for inputting attributes of all types*/
	char *string;       /* for text attributes (type = NC_CHAR) */
	double *vals;       /* for numeric attributes of all types */
} ncatt;

typedef struct {	    /* aggregate member-dataset */
	int dsetnum;	    /* Ferret dataset number */
	int aggSeqNo;	    /* sequence number of dset within agg */
} ncagg;

typedef struct {	    /* for var in aggregate member-dataset: */
	int imemb;	    /* for members of the aggregate, member sequence number */
	int vtype;	    /* for members of the aggregate, type: user-var or file-var */
	int datid;	    /* for members of the aggregate, Ferret dataset id */
	int gnum;           /* Ferret grid numbers */
	int iline;          /* Ferret line number for the aggregate dimension */
	int nv;             /* Ferret sequence # in ds_var_code or uvar_name_code */
} ncagg_var_descr;

typedef struct {     /* for uvars: grid/dataset pairs*/
	int grid;
	int dset;
	int dtype;
        int auxCat[NFERDIMS];
        int auxVar[NFERDIMS];
} uvarGrid;

#endif   /* _NCF_UTIL_H_ */

