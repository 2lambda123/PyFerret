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

/* NCF_Util.c
*
* Ansley Manke
* Ferret V600 April 26, 2005
* V5600 *acm* fix declarations of fillc and my_len as required by solaris compiler
*
* This file contains all the utility functions which Ferret
* needs in order to do attribute handling. Based on code for EF's.
* calls are made to nc_ routines from netcdf library.
*
*
* *acm   9/06 v600 - add stdlib.h wherever there is stdio.h for altix build
* *acm  10/06 v601 - Fix by Remik for bug 1455, altix. For string attributes,
*                    allocate one more than the att.len, presumably for the null
*                    terminator for the string. Also double check the string length
*                    that is returned from the call to nc_inq_att, and make sure
*                    we allocate the correct amount of memory for the string.
*
* *acm  11/06 v601 - ncf_delete_var_att didnt reset the attribute id's.  Fix this.
*
* *acm  11/06 v601 - new routine ncf_add_var_num_att_dp
* *acm  11/06 v601 - new routine ncf_repl_var_att_dp
* *acm  11/06 v601 - in ncf_init_other_dset, set the name of the global attribute
*                    to history, and define its attribute type and outflag.
* *acm  11/06 v601 - new routine ncf_rename_var, for fix of bug 1471
* *acm  11/06 v601 - in ncf_delete_var_att, renumber the attid for the remaining attributes.
* *acm* 12/06 v602 - new attribute assigned to coordinate vars on input, orig_file_axname
* *acm*  2/07 V602 - Fix bug 1492, changing attributes of coordinate variables; use pseudo-dataset
*                       of user-defined axes to keep track of attributes.
* *acm* 10 07      - Patches for memory-leak fixes from Remiz Ziemlinski
* *acm* 10/07      - Further fixes by Remik, initializing att.vals, att.string to NULL,
*                      set var.ndims = 0 in ncf_init_other_dset
* *acm*  3/08      - Fix bug 1534; needed to initialize attribute output flag for
*                    the bounds attribute on coordinate axes.
* *acm*  1/09      - If adding a new global attribute, also increment ngatts.
* *acm*  1/09      - Fix bug 1620; In ncf_add_var, which is used when defining user
*                    variables, and also for reading in EZ datasets, I had the default
*                    attribute type for missing_value attribute set to NC_DOUBLE. There's no
*                    reason for this as these variables are always single precision.
* *acm*  5/09      - Fix bug 1664. For user variables, varid matches the uvar from Ferret.
*                    therefore it may be larger than nc_ptr->nvars
* *acm*  3/11      - Fix bug 1825. Routine ncf_get_var_seq no longer called
* *acm*  1/12      - Ferret 6.8 ifdef double_p for double-precision ferret, see the
*                    definition of macro DFTYPE in ferretmacros.h.
* *acm*  5/12 V6.8 - Additions for creating aggregate datasets
* *acm*  8/13        Fix bug 2089. Mark the scale_factor and add_offset attributes  
*                    to-be-output when writing variables.
* *acm*  8/13        Fix bug 2091. If a string variable has the same name as a dimension,
*                    DO NOT mark it as an axis.
* *acm*  v694 1/15   For ticket 2227: if a dimension from a nc file is not also a 
*                    1-D coordinate var, don't write the axis Ferret creates. Do report
*                    in dimnames outputs the dimension names as used by Ferret e.g. a
*                    renamed axis TIME -> TIME1
* *sh*  12/15        Bug fix: ncf_get_agg_member is called with the sequence number of the
*                    desired dataset. So store that in order to locate the right member 
* *acm*  2/16        Additions for ticket 2352: LET/D variables and attributes. User-variables
*                    defined with LET/D=n are stored with dataset n. A flag in the ncvar 
*                    structure tells that the variable is a user-var. A new subroutine call,
*                    ncf_get_var_uvflag returns this flag, so that SHOW DATA/ATTRIBUTES can 
*                    list these variables. When user variables are canceled, the varids for 
*                    user-variables remaining in the dataset are adjusted.
* *sh*  5/16         added grid management for uvars -- dset/grid paris stored in a LIST
*                    replaced uvflag with uvarid
* *acm* 6/16         Make sure var.nmemb is initialized when adding a new variable.
*/

#include <Python.h> /* make sure Python.h is first */
#include <stddef.h>  /* size_t, ptrdiff_t; gfortran on linux rh5*/
#include <unistd.h>		/* for convenience */
#include <stdlib.h>		/* for convenience */
#include <stdio.h>		/* for convenience */
#include <string.h>		/* for convenience */
#include <fcntl.h>		/* for fcntl() */
#include <assert.h>
#include <sys/types.h>	        /* required for some of our prototypes */
#include <sys/stat.h>
#include <sys/errno.h>
#include "ferretmacros.h"

#include "netcdf.h"
#include "nc.h"
#include "list.h"  /* locally added list library */
#include "NCF_Util.h"

/* ................ Global Variables ................ */

static LIST  *GLOBAL_ncdsetList;
static int list_initialized = FALSE;

/* ............. Function Declarations .............. */
/*
 * Note that all routines called directly from Ferret,
 * ie. directly from Fortran, should be all lower case,
 * be of type 'void', pass by reference and should end with 
 * an underscore.
 */


/* .... Functions called by Ferret .... */
int  FORTRAN(ncf_inq_ds)( int *, int *, int *, int *, int *);
int  FORTRAN(ncf_inq_ds_dims)( int *, int *, char *, int *, int *);
int  FORTRAN(ncf_inq_var) (int *, int *, char *, int *, int *, int *, int *, int *, int *, int * );

int  FORTRAN(ncf_inq_var_att)( int *, int *, int *, char *, int *, int *, int *, int *);

void FORTRAN(ncf_free_attlist)(ncvar*);

int  FORTRAN(ncf_get_dsnum)( char * );
int  FORTRAN(ncf_get_dsname)( int *, char *);
int  FORTRAN(ncf_get_dim_id)( int *, char *);

int  FORTRAN(ncf_get_var_name)( int *, int *, char *);
int  FORTRAN(ncf_get_var_id)( int *, int*, char *); 
int  FORTRAN(ncf_get_var_id_case)( int *, int*, char *); 
int  FORTRAN(ncf_get_var_axflag)( int *, int *, int *, int *); 
int  FORTRAN(ncf_get_var_attr_name) (int *, int *, int *, int *, char*);
int  FORTRAN(ncf_get_var_attr_id) (int *, int *, char* , int*);
int  FORTRAN(ncf_get_var_attr_id_case) (int *, int *, char* , int*);
int  FORTRAN(ncf_get_var_attr) (int *, int *, char* , char* , int *, double *);
int  FORTRAN(ncf_get_var_attr) (int *, int *, char* , char* , int *, double *);
int  FORTRAN(ncf_get_attr_from_id) (int *, int *, int * , int *, double* );

int  FORTRAN(ncf_get_var_outflag) (int *, int *, int *);
int  FORTRAN(ncf_get_var_outtype) (int *, int *,  int *);
int  FORTRAN(ncf_get_var_uvflag) (int *, int *, int *);

int  FORTRAN(ncf_init_uvar_dset)( int *);
int  FORTRAN(ncf_init_uax_dset)( int *);
int  FORTRAN(ncf_add_dset)( int *, int *, char *, char *);
int  FORTRAN(ncf_init_other_dset)( int *, char *, char *);
int  FORTRAN(ncf_delete_dset)( int *);
int  FORTRAN(ncf_delete_var_att)( int *, int *, char *);
int  FORTRAN(ncf_delete_var)( int *, char *);

int  FORTRAN(ncf_add_var)( int *, int *, int *, int *, char *, char *, char *, double *);
int  FORTRAN(ncf_add_coord_var)( int *, int *, int *, int *, char *, char *, double *);

int  FORTRAN(ncf_add_var_num_att)( int *, int *, char *, int *, int *, int *, DFTYPE *);
int  FORTRAN(ncf_add_var_num_att_dp)( int *, int *, char *, int *, int *, int *, double *);
int  FORTRAN(ncf_add_var_str_att)( int *, int *, char *, int *, int *, int *, char *);

int  FORTRAN(ncf_rename_var)( int *, int *, char *);
int  FORTRAN(ncf_rename_dim)( int *, int *, char *);

int  FORTRAN(ncf_repl_var_att)( int *, int *, char *, int *, int *, DFTYPE *, char *);
int  FORTRAN(ncf_repl_var_att_dp)( int *, int *, char *, int *, int *, double *, char *);
int  FORTRAN(ncf_set_att_flag)( int *, int *, char *, int *);
int  FORTRAN(ncf_set_var_out_flag)( int *, int *, int *);
int  FORTRAN(ncf_set_var_outtype)( int *, int *, int *);
int  FORTRAN(ncf_set_axdir)(int *, int *, int *);
int  FORTRAN(ncf_transfer_att)(int *, int *, int *, int *, int *);
 
int  FORTRAN(ncf_init_agg_dset)( int *, char *);
int  FORTRAN(ncf_add_agg_member)( int *, int *, int *);
int  FORTRAN(ncf_get_agg_count)( int *, int *);
int  FORTRAN(ncf_get_agg_member)( int *, int *, int *);
int  FORTRAN(ncf_get_agg_var_info)( int *, int *, int *, int *, int *, int *, int *, int *);
int  FORTRAN(ncf_put_agg_memb_grid)( int *, int *, int *, int *);

/* uvar grid management functions */
int  FORTRAN(ncf_free_uvar_grid_list)( int *, int *);
int  FORTRAN(ncf_set_uvar_grid)( int *, int *, int *, int *, int *);
int  FORTRAN(ncf_get_uvar_grid)( int *, int *, int *, int *);
int  FORTRAN(ncf_set_uvar_aux_info)( int *, int *, int *, int *, int *);
int  FORTRAN(ncf_get_uvar_aux_info)( int *, int *, int *, int *, int *);
int  FORTRAN(ncf_get_uvar_grid_list_len)( int *, int *, int *);
int  FORTRAN(ncf_delete_uvar_grid)( int *, int *, int *);

/* .... Functions called internally .... */

ncdset *ncf_ptr_from_dset(int *);
LIST *ncf_get_ds_varlist( int *);
LIST *ncf_get_ds_agglist( int *);
LIST *ncf_get_ds_var_attlist (int *, int *);
LIST *ncf_get_ds_var_gridlist (int *, int *);

static int initialize_output_flag (char *, int);
int NCF_ListTraverse_FoundDsetName( char *, char * );
int NCF_ListTraverse_FoundDsetID( char *, char * );
int NCF_ListTraverse_FoundVarName( char *, char * );
int NCF_ListTraverse_FoundVarNameCase( char *, char * );
int NCF_ListTraverse_FoundVarID( char *, char * );
int NCF_ListTraverse_FoundUvarID( char *, char * );
int NCF_ListTraverse_FoundVarAttName( char *, char * );
int NCF_ListTraverse_FoundVarAttNameCase( char *, char * );
int NCF_ListTraverse_FoundVarAttID( char *, char * );
int NCF_ListTraverse_FoundVariMemb( char *, char * );
int NCF_ListTraverse_FoundDsMemb( char *, char * );
int NCF_ListTraverse_FoundDsMemb( char *, char * );
int NCF_ListTraverse_FoundGridDset( char *, char * );

/*
 * Find a dataset based on its integer ID and return the scalar information:
 * ndims, nvars, ngatts, recdim.
 */

int FORTRAN(ncf_inq_ds)( int *dset, int *ndims, int *nvars, int *ngatts, int *recdim )
{
  ncdset *nc_ptr=NULL;
  int return_val;

  return_val = ATOM_NOT_FOUND;
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL ) { return return_val; }

  *ndims = nc_ptr->ndims;
  *nvars = nc_ptr->nvars;
  *ngatts = nc_ptr->ngatts;

/* dimension for Fortran, add 1 */
  *recdim = nc_ptr->recdim+1;

  return_val = FERR_OK; 
  return return_val; 
}

/* ----
 * Find a dataset based on its integer ID and return the dimension info for
 * dimension given.
 */
int  FORTRAN(ncf_inq_ds_dims)( int *dset, int *idim, char dname[], int *namelen, int *dimsize)
{
  ncdset *nc_ptr=NULL;
  int return_val;

  return_val = ATOM_NOT_FOUND;
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL ) { return return_val; }
  
  strcpy (dname, nc_ptr->dims[*idim-1].name);
  *namelen = strlen(dname);
  *dimsize = nc_ptr->dims[*idim-1].size;

  return_val = FERR_OK; 
  return return_val; 
}

/* ----
 * Find a variable in a dataset based on the dataset integer ID and 
 * variable id. Return the variable name (in its original upper/lower 
   case form), type, nvdims, vdims, nvatts.
 */

 int FORTRAN(ncf_inq_var) (int *dset, int *varid, char newvarname[], int *len_newvarname, int *vtype, int *nvdims,
     int *nvatts, int* coord_var, int *outflag, int *vdims)

{
  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;
  int i, ivar;
  int ndx;
  int the_dim;
  int outdims;
  int status=LIST_OK;
  int return_val;
  LIST *varlist;
	LIST_ELEMENT *lp;

  return_val = ATOM_NOT_FOUND;  
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL )return return_val;

   /*
   * Get the list of variables.  
   */
  varlist = ncf_get_ds_varlist(dset);
  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
  }

  var_ptr=(ncvar *)list_curr(varlist); 	

  strcpy(newvarname, var_ptr->name);
  *len_newvarname = strlen(newvarname);
  *vtype = var_ptr->type;
  *nvdims = var_ptr->ndims;
  *nvatts = var_ptr->natts;
  *outflag = var_ptr->all_outflag;
  *coord_var = var_ptr->is_axis;

   for (i=0; i <var_ptr->ndims ;i++ )
  {
	  the_dim =  var_ptr->dims[i];
	  vdims[i] = the_dim ;
  }

  return_val = FERR_OK;
  return return_val;
}

/* ----
 * Find a variable in a dataset based on the dataset integer ID and 
 * variable id. Return the variable output type.
 */

 int FORTRAN(ncf_get_var_outtype) (int *dset, int *varid,  int *outtype)

{
  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  LIST *varlist;


  return_val = ATOM_NOT_FOUND;

  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL )return return_val;

   /*
   * Get the list of variables.  
   */
  varlist = ncf_get_ds_varlist(dset);
  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
  }
  
  var_ptr=(ncvar *)list_curr(varlist); 

  *outtype = var_ptr->outtype;
   
  return_val = FERR_OK;
  return return_val;
}
 

/* ----
 * Find a variable attribute based on its variable ID and dataset ID, and attribute name
 * Return the attribute name, type, length, and output flag
 */
int  FORTRAN(ncf_inq_var_att)( int *dset, int *varid, int *attid, char attname[], int *namelen, int *attype, int *attlen, int *attoutflag)

{
  ncatt *att_ptr=NULL;
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  int i;
  LIST *varlist;
  LIST *varattlist;
	LIST_ELEMENT* lp;

   /*
   * Get the list of variables.  
   */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 
  if (var_ptr->natts < 1) return ATOM_NOT_FOUND;

   /*
   * Get the list of attributes for the variable in the dataset
   */
  varattlist = ncf_get_ds_var_attlist(dset, varid);

  status = list_traverse(varattlist, (char *) attid, NCF_ListTraverse_FoundVarAttID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
    }

  att_ptr=(ncatt *)list_curr(varattlist); 

  strcpy(attname, att_ptr->name);
  *namelen = strlen(attname);
  *attype = att_ptr->type; 
  *attlen = att_ptr->len;
  *attoutflag = att_ptr->outflag;

  return_val = FERR_OK;
  return return_val;
}

/* ----
 * Find a dataset based on its name and
 * return the ferret dataset number.
 */

int FORTRAN(ncf_get_dsnum)( char name[] )
{
  ncdset *nc_ptr=NULL;
  int status=LIST_OK;

  static int return_val=0; /* static because it needs to exist after the return statement */

  /*
   * Find the dataset.
   */

  status = list_traverse(GLOBAL_ncdsetList, name, NCF_ListTraverse_FoundDsetName, (LIST_FRNT | LIST_FORW | LIST_ALTR));

  /*
   * If the search failed, set the dset to ATOM_NOT_FOUND.
   */
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
  }

  nc_ptr=(ncdset *)list_curr(GLOBAL_ncdsetList); 

  return_val = nc_ptr->fer_dsetnum;
  return return_val;
}

/* ----
 * Find a dataset based on its integer ID and return the name.
 */

int FORTRAN(ncf_get_dsname)( int *dset, char name[] )
{
  ncdset *nc_ptr=NULL;
  int return_val;

  return_val = ATOM_NOT_FOUND;
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL ) { return return_val; }

  strcpy(name, nc_ptr->fername);

  return_val = FERR_OK;
  return return_val;
}

/* ----
 * Find a dataset based on its integer ID and a dimension name. Return the dimension ID.
 */
int FORTRAN(ncf_get_dim_id)( int *dset, char dname[])
{
  ncdset *nc_ptr=NULL;
  int return_val;
  int idim;
  int sz;
  int szdim;

  return_val = ATOM_NOT_FOUND;
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL ) { return return_val; }
  
  for (idim = 0; idim < nc_ptr->ndims; idim++) {
	sz = strlen(dname);
	szdim = strlen(nc_ptr->dims[idim].name);
    if ( (sz == szdim) &&
		 (nc_ptr->dims[idim].size !=0) && 
		 (strncmp(dname, nc_ptr->dims[idim].name, sz) == 0) )
    { return_val = idim + 1;
	  return return_val;
    } 
  }
  return return_val;
}

/* ----
 * Find a variable in a dataset based on the dataset integer ID and 
 * variable id. Return the variable name.
 */
 int FORTRAN(ncf_get_var_name) (int *dset, int* ivar, char* string)

{
  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;
  int i;
  int status=LIST_OK;
  int return_val;
  LIST *varlist;
  LIST *dummy;

  return_val = ATOM_NOT_FOUND;  
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL )return return_val;

   /*
   * Get the list of variables.  
   */
  varlist = ncf_get_ds_varlist(dset);
  dummy = list_mvfront(varlist);
  var_ptr=(ncvar *)list_front(varlist); 

  for (i = 0; i < *ivar; i++) {
     strcpy(string, var_ptr->name); 
     dummy = list_mvnext(varlist);
     var_ptr=(ncvar *)list_curr(varlist);  
  }
  
  free(dummy);
  return_val = FERR_OK;
  return return_val;
}

/* ----
 * Find a variable in a dataset based on the dataset integer ID and 
 * variable name. Return the variable id, or NOT FOUND if it does not exist
 */
 int FORTRAN(ncf_get_var_id) (int *dset, int *varid, char string[])

{
  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  LIST *varlist;

  return_val = ATOM_NOT_FOUND;  
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL )return return_val;

   /*
   * Get the list of variables.  
   */

  varlist = ncf_get_ds_varlist(dset);
  status = list_traverse(varlist, string, NCF_ListTraverse_FoundVarName, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
  }
  
  var_ptr=(ncvar *)list_curr(varlist); 
  *varid = var_ptr->varid;
  return_val = FERR_OK;

  return return_val;
}
/* ----
 * Find a variable in a dataset based on the dataset integer ID and 
 * variable name. Return the variable id, or NOT FOUND if it does not exist
 */
 int FORTRAN(ncf_get_var_id_case) (int *dset, int *varid, char string[])

{
  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  LIST *varlist;

  return_val = ATOM_NOT_FOUND;  
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL )return return_val;

   /*
   * Get the list of variables.  
   */

  varlist = ncf_get_ds_varlist(dset);
  status = list_traverse(varlist, string, NCF_ListTraverse_FoundVarNameCase, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
  }
  
  var_ptr=(ncvar *)list_curr(varlist); 
  *varid = var_ptr->varid;
  return_val = FERR_OK;

  return return_val;
}
/* ----
 * Find a variable in a dataset based on the dataset integer ID and 
 * variable ID. Return the coordinate-axis flag.
 */
 int FORTRAN(ncf_get_var_axflag) (int *dset, int *varid, int* coord_var, int* ax_dir)

{
  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;
  int i;
  int status=LIST_OK;
  int return_val;
  LIST *varlist;

  return_val = FALSE;  
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL )return return_val;

   /*
   * Get the list of variables.  
   */
  varlist = ncf_get_ds_varlist(dset);
  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
  }
  
  var_ptr=(ncvar *)list_curr(varlist); 
  
  *coord_var = var_ptr->is_axis;
  *ax_dir = var_ptr->axis_dir;

  return_val = FERR_OK;
  return return_val;
}

/* ----
 * Find a variable in a dataset based on the dataset integer ID and 
 * variable ID. Return the variable all_outflag for attributes
 */
 int FORTRAN(ncf_get_var_outflag) (int *dset, int *varid, int *iflag)

{
  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;
  int i;
  int status=LIST_OK;
  int return_val;
  LIST *varlist;

  return_val = 0;  
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL )return return_val;

   /*
   * Get the list of variables and the variable based on its id
   */
  varlist = ncf_get_ds_varlist(dset);
  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
  }
  
  var_ptr=(ncvar *)list_curr(varlist); 
  *iflag = var_ptr->all_outflag;

  return FERR_OK;
}

/* ----
 * Find a variable in a dataset based on the dataset integer ID and 
 * variable ID. Return the return the flag indicating file 
*  variable vs user-variable
 */
 int FORTRAN(ncf_get_var_uvflag) (int *dset, int *varid, int *uvflag)

{
  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;
  int i;
  int status=LIST_OK;
  int return_val;
  LIST *varlist;
  int flag;

  return_val = 0;  
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL )return return_val;

   /*
   * Get the list of variables and the variable based on its id
   */
  varlist = ncf_get_ds_varlist(dset);
  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
  }
  
  var_ptr=(ncvar *)list_curr(varlist); 
  if (var_ptr->uvarid == 0)
    flag = 0;
  else
    flag = 1;

  *uvflag = flag;

  return FERR_OK;
}


/* ----
 * Find a variable attribute based on the dataset ID and variable ID and attribute name
 * Return the attribute ID
 */
 int FORTRAN(ncf_get_var_attr_id) (int *dset, int *varid, char* attname, int* attid)

{
  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;
  ncatt *att_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  int i;
  LIST *varlist;
  LIST *varattlist;
  LIST *dummy;

   /*
   * Get the list of variables.  
   */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 
  if (var_ptr->natts < 1) return ATOM_NOT_FOUND;

   /*
   * Get the list of attributes for the variable in the dataset. find attname.
   */
  varattlist = ncf_get_ds_var_attlist(dset, varid);

  status = list_traverse(varattlist, attname, NCF_ListTraverse_FoundVarAttName, (LIST_FRNT | LIST_FORW | LIST_ALTR));

  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
    }

  att_ptr=(ncatt *)list_curr(varattlist); 
  *attid = att_ptr->attid;

  return_val = FERR_OK;
  return return_val;
}

/* ----
 * Find a variable attribute based on the dataset ID and variable ID and attribute name
 * Return the attribute ID
 */
 int FORTRAN(ncf_get_var_attr_id_case) (int *dset, int *varid, char* attname, int* attid)

{
  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;
  ncatt *att_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  int i;
  LIST *varlist;
  LIST *varattlist;
  LIST *dummy;

   /*
   * Get the list of variables.  
   */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 
  if (var_ptr->natts < 1) return ATOM_NOT_FOUND;

   /*
   * Get the list of attributes for the variable in the dataset. find attname.
   */
  varattlist = ncf_get_ds_var_attlist(dset, varid);

  status = list_traverse(varattlist, attname, NCF_ListTraverse_FoundVarAttNameCase, (LIST_FRNT | LIST_FORW | LIST_ALTR));

  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
    }

  att_ptr=(ncatt *)list_curr(varattlist); 
  *attid = att_ptr->attid;

  return_val = FERR_OK;
  return return_val;
}

/* ----
 * Find a variable attribute based on the dataset ID and variable ID and attribute ID.
 * Return the attribute name.
 */
 int FORTRAN(ncf_get_var_attr_name) (int *dset, int *varid, int* attid, int *namelen, char* attname)

{
  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;

  ncatt *att_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  int i;
  LIST *varlist;
  LIST *varattlist;
  LIST *dummy;

   /*
   * Get the list of variables.  
   */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 
  if (var_ptr->natts < 1) return ATOM_NOT_FOUND;

   /*
   * Get the list of attributes for the variable in the dataset
   */
  varattlist = ncf_get_ds_var_attlist(dset, varid);

  dummy = list_mvfront(varattlist);
  att_ptr=(ncatt *)list_front(varattlist); 

  for (i = 0; i < *attid; i++) {
     strcpy(attname, att_ptr->name);
	 dummy = list_mvnext(varattlist);
     att_ptr=(ncatt *)list_curr(varattlist);  
  }
  
  
  *namelen = strlen(attname);
  return_val = FERR_OK;
  return return_val;
}

/*----
 * Find a variable attribute based on the dataset ID and variable ID and attribute name.
 * On input, len is the max len to load.
 * Return the attribute, len, and its string or numeric value.
 */
 int FORTRAN(ncf_get_var_attr) (int *dset, int *varid, char* attname, char* string, int *len, double* val)

{
  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;
  ncatt *att_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  int i;
  LIST *varlist;
  LIST *varattlist;

   /*
   * Get the list of variables.  
   */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 
  if (var_ptr->natts < 1) return ATOM_NOT_FOUND;

   /*
   * Get the list of attributes for the variable in the dataset
   */
  varattlist = ncf_get_ds_var_attlist(dset, varid);

  status = list_traverse(varattlist, attname, NCF_ListTraverse_FoundVarAttName, (LIST_FRNT | LIST_FORW | LIST_ALTR));

  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
    }
  
  strcpy(string, "");
  val[0] = NC_FILL_DOUBLE;

  att_ptr=(ncatt *)list_curr(varattlist); 

  if (att_ptr->type == NC_CHAR)
  { 

	  strncpy(string, att_ptr->string, *len); 
  }
  else 
  { for (i = 0; i < att_ptr->len; i++) {
	  val[i] = att_ptr->vals[i]; }
  }
  *len = att_ptr->len;
  return_val = FERR_OK;
  return return_val;
}


/*----
 * Find a numeric attribute based on the dataset ID and variable ID and attribute id.
 * On input, len is the max len to load.
 * Return the attribute, len, and or numeric value.
 */
 int FORTRAN(ncf_get_attr_from_id) (int *dset, int *varid, int *attid, int *len, double* val)

{
  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;
  ncatt *att_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  int i;
  LIST *varlist;
  LIST *varattlist;

   /*
   * Get the list of variables.  
   */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 
  if (var_ptr->natts < 1) return ATOM_NOT_FOUND;

   /*
   * Get the list of attributes for the variable in the dataset. 
   * Find the attribute from its ID
   */
  varattlist = ncf_get_ds_var_attlist(dset, varid);

  status = list_traverse(varattlist, (char *) attid, NCF_ListTraverse_FoundVarAttID, (LIST_FRNT | LIST_FORW | LIST_ALTR));

  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
    }
  
  att_ptr=(ncatt *)list_curr(varattlist); 

  val[0] = NC_FILL_DOUBLE;
  if (att_ptr->type == NC_CHAR)
	{
	  fprintf(stderr, "ERROR: ncf_get_attr_from_id: Atribute is CHAR. This function only for numberic.\n");
	  return_val = -1;
	  return return_val; 
          }
  else 
  { for (i = 0; i < att_ptr->len; i++) {
	  val[i] = att_ptr->vals[i]; }
  }
  *len = att_ptr->len;
  return_val = FERR_OK;
  return return_val;
}

/* ----
 * Initialize new dataset to contain user variables and 
 * save in GLOBAL_ncdsetList for attribute handling 
 */

int FORTRAN(ncf_init_uvar_dset)(int *setnum)

{
  ncdset nc; 
  static int return_val=FERR_OK; /* static because it needs to exist after the return statement */
  
    int i;				/* loop controls */
	int ia;
	int iv;
    int nc_status;		/* return from netcdf calls */
    ncatt att;			/* attribute */
    ncvar var;			/* variable */
		att.vals = NULL;
		att.string = NULL;
    strcpy(nc.fername, "UserVariables");
    strcpy(nc.fullpath, " ");
    nc.fer_dsetnum = *setnum;

    nc.ngatts = 1;
    nc.nvars = 0;
	nc.recdim = -1;   /* never used, but initialize anyway*/
	nc.ndims = 6;     /* never used, but initialize anyway*/
	nc.its_agg = 0;
	nc.num_agg_members = 0;
    nc.vars_list_initialized = FALSE;

   /* set one global attribute, treat as pseudo-variable . the list of variables */

       strcpy(var.name, ".");

       var.attrs_list_initialized = FALSE;

       var.type = NC_CHAR;
       var.outtype = NC_CHAR;
       var.varid = 0;
	   var.natts = nc.ngatts;
       var.has_fillval = FALSE;
	   var.fillval = NC_FILL_FLOAT;
	   var.all_outflag = 1;
	   var.is_axis = FALSE;
	   var.axis_dir = 0;

	   var.attrs_list_initialized = FALSE; 

		 att.outflag = 1;
		 att.type = NC_CHAR;
		 att.outtype = NC_CHAR;
		 att.len = 21;
		 strcpy(att.name, "FerretUserVariables" );
		 att.string = (char*)malloc(2*sizeof(char));
		 strcpy(att.string, " ");

      /*Save attribute in linked list of attributes for variable .*/	
       if (!var.attrs_list_initialized) {
          if ( (var.varattlist = list_init()) == NULL ) {
            fprintf(stderr, "ERROR: ncf_init_uvar_dset: Unable to initialize GLOBAL attributes list.\n");
            return_val = -1;
            return return_val; 
          }
          var.attrs_list_initialized = TRUE;
	  }

       list_insert_after(var.varattlist, (char *) &att, sizeof(ncatt));

       /* global attributes list complete */

      /*Save variable in linked list of variables for this dataset */	
       if (!nc.vars_list_initialized) {
          if ( (nc.dsetvarlist = list_init()) == NULL ) {
            fprintf(stderr, "ERROR: ncf_init_uvar_dset: Unable to initialize variable list.\n");
            return_val = -1;
            return return_val; 
          }
          nc.vars_list_initialized = TRUE;
        }

       list_insert_after(nc.dsetvarlist, (char *) &var, sizeof(ncvar));

/* Add dataset to global nc dataset linked list*/ 
  if (!list_initialized) {
    if ( (GLOBAL_ncdsetList = list_init()) == NULL ) {
      fprintf(stderr, "ERROR: ncf_init_uvar_dset: Unable to initialize GLOBAL_ncDsetList.\n");
      return_val = -1;
      return return_val; 
	}
    list_initialized = TRUE;
  }

  list_insert_after(GLOBAL_ncdsetList, (char *) &nc, sizeof(ncdset));
  return_val = FERR_OK;
  return return_val;
  }

/* ----
 * Initialize new dataset to contain user-defined coordinate variables and 
 * save in GLOBAL_ncdsetList for attribute handling 
 */

int FORTRAN(ncf_init_uax_dset)(int *setnum)

{
  ncdset nc; 
  static int return_val=FERR_OK; /* static because it needs to exist after the return statement */
  
    int i;				/* loop controls */
	int ia;
	int iv;
    int nc_status;		/* return from netcdf calls */
    ncatt att;			/* attribute */
    ncvar var;			/* variable */
		att.vals = NULL;
		att.string = NULL;
    strcpy(nc.fername, "UserCoordVariables");
    strcpy(nc.fullpath, " ");
    nc.fer_dsetnum = *setnum;

    nc.ngatts = 1;
    nc.nvars = 0;
	nc.recdim = -1;   /* never used, but initialize anyway*/
	nc.its_agg = 0;
    nc.vars_list_initialized = FALSE;

   /* set one global attribute, treat as pseudo-variable . the list of variables */

       strcpy(var.name, "."); /*is this a legal name?*/

       var.attrs_list_initialized = FALSE;

       var.type = NC_CHAR;
       var.outtype = NC_CHAR;
       var.varid = 0;
	   var.natts = nc.ngatts;
       var.has_fillval = FALSE;
       var.fillval = NC_FILL_FLOAT;
	   var.all_outflag = 1;
	   var.is_axis = FALSE;
	   var.axis_dir = 0;

	   var.attrs_list_initialized = FALSE; 

		 att.outflag = 1;
		 att.type = NC_CHAR;
		 att.outtype = NC_CHAR;
		 att.len = 21;
		 strcpy(att.name, "FerretUserCoordVariables" );
     att.string = (char*)malloc(2*sizeof(char));
		 strcpy(att.string, " ");

      /*Save attribute in linked list of attributes.*/	
       if (!var.attrs_list_initialized) {
          if ( (var.varattlist = list_init()) == NULL ) {
            fprintf(stderr, "ERROR: ncf_init_uax_dset: Unable to initialize GLOBAL attributes list.\n");
            return_val = -1;
            return return_val; 
          }
          var.attrs_list_initialized = TRUE;
	  }

       list_insert_after(var.varattlist, (char *) &att, sizeof(ncatt));

       /* global attributes list complete */

      /*Save variable in linked list of variables for this dataset */	
       if (!nc.vars_list_initialized) {
          if ( (nc.dsetvarlist = list_init()) == NULL ) {
            fprintf(stderr, "ERROR: ncf_init_uax_dset: Unable to initialize variable list.\n");
            return_val = -1;
            return return_val; 
          }
          nc.vars_list_initialized = TRUE;
        }

       list_insert_after(nc.dsetvarlist, (char *) &var, sizeof(ncvar));

/* Add dataset to global nc dataset linked list*/ 
  if (!list_initialized) {
    if ( (GLOBAL_ncdsetList = list_init()) == NULL ) {
      fprintf(stderr, "ERROR: ncf_init_uax_dset: Unable to initialize GLOBAL_ncDsetList.\n");
      return_val = -1;
      return return_val; 
	}
    list_initialized = TRUE;
  }

  list_insert_after(GLOBAL_ncdsetList, (char *) &nc, sizeof(ncdset));
  return_val = FERR_OK;
  return return_val;
  }

/* ----
 * Get file info for a dataset and save in GLOBAL_ncdsetList for attribute handling 
 */

int FORTRAN(ncf_add_dset)(int *ncid, int *setnum, char name[], char path[])

{
  ncdset nc; 
  static int return_val=FERR_OK; /* static because it needs to exist after the return statement */
	
	/* code lifted liberally from ncdump.c Calls in nc library.*/
	
	char fillc;
	int i;				/* loop controls */
	int ia;
	int iv;
	int ilen;
	int nc_status;		/* return from netcdf calls */
	ncdim fdims;		/* name and size of dimension */
	ncatt att;			/* attribute */
	ncatt att0;			/* initialize attribute */
	ncvar var;			/* variable */
	int bad_file_attr = 243; /* matches merr_badfileatt in tmap_errors.parm*/
	size_t len;

	att.vals = NULL;
	att.string = NULL;
	strcpy(nc.fername, name);
	strcpy(nc.fullpath, path);
	nc.fer_dsetnum = *setnum;
	nc.its_agg = 0;
	nc.num_agg_members = 0;
	
	/* Set attribute with initialization values*/
	
	strcpy(att0.name, " ");
	att0.type = NC_CHAR;
	att0.outtype = NC_CHAR;
	att0.attid = 0;
	att0.outflag = 0;
	att0.len = 1;
	att0.string = (char *) malloc((att0.len+1)* sizeof(char*));
	strcpy (att0.string," ");
	att0.vals = (double *) malloc(1 * sizeof(double));
	att0.vals[0] = 0;
	
	/*
	 * get number of dimensions, number of variables, number of global
	 * atts, and dimension id of unlimited dimension, if any
	 */
	nc_status = nc_inq(*ncid, &nc.ndims, &nc.nvars, &nc.ngatts, &nc.recdim) ;
	if (nc_status != NC_NOERR) return nc_status;
	
	/* get dimension info */
	if (nc.ndims > 0) {
		for (i = 0; i < nc.ndims; i++) {
			nc_status = nc_inq_dim(*ncid, i, fdims.name, &fdims.size); 
			if (nc_status != NC_NOERR) return nc_status;
			
			if (nc_status != NC_NOERR) return nc_status;
			strcpy (nc.dims[i].name, fdims.name);
			nc.dims[i].size = fdims.size;
/*			strcpy (nc.dimname[i], fdims.name);
			nc.dimsize = fdims.size; */
		}
	}
	
	nc.vars_list_initialized = FALSE;
	nc_status = NC_NOERR;
	
	/* get info on global attributes, treat as pseudo-variable . list of attributes*/
	
	
	/* get global attributes */
	
	if (nc.ngatts > 0)
		{
			strcpy(var.name, ".");
			
			var.attrs_list_initialized = FALSE;
			
			var.type = NC_CHAR;
			var.outtype = NC_CHAR;
			var.varid = 0;
			var.natts = nc.ngatts;
			var.ndims = 1;
			var.dims[0] = 1;
			var.has_fillval = FALSE;
			var.fillval = NC_FILL_FLOAT;
			var.all_outflag = 1;
			var.is_axis = FALSE;
			var.axis_dir = 0;
			var.uvarid = 0;
			var.nmemb = 0;
			
			var.attrs_list_initialized = FALSE;
			for (i = 0; i < nc.ngatts; i++)
				{
					
					/* initialize 
						 att = att0;*/
					
          nc_status = nc_inq_attname(*ncid, NC_GLOBAL, i, att.name);            
					/*		  if (nc_status != NC_NOERR) fprintf(stderr, " ***NOTE: error reading global attribute id %d from file %s\n", i, nc.fullpath);  */
          if (nc_status == NC_NOERR) {
						
            att.attid = i+1;
            nc_status = nc_inq_att(*ncid, NC_GLOBAL, att.name, &att.type, &len);
						att.len = (int)len;

						/*            if (nc_status != NC_NOERR) fprintf(stderr, " ***NOTE: error reading global attribute %s from file %s\n",att.name, nc.fullpath); */
            if (nc_status == NC_NOERR) {
							
							/* Set output flag. By default only the global history attribute is written.
							 *  For string attributes, allocate one more than the att.len, 
							 *  presumably for the null terminator for the string (?) */
							
							att.outflag = 0;
							if (strcmp(att.name,"history")==0)
								{att.outflag = 1;
								}
							
              if (att.len == 0) {	/* show 0-length attributes as empty strings */
								att.type = NC_CHAR;
								att.outtype = NC_CHAR;
								att.len = 1;
								att.string = (char *) malloc(2* sizeof(char));
								strcpy (att.string," ");
              }
              switch (att.type) {
              case NC_CHAR:
								att.string = (char *) malloc((att.len+1)* sizeof(char));
								
								nc_status = nc_get_att_text(*ncid, NC_GLOBAL, att.name, att.string );
								if (nc_status != NC_NOERR) return nc_status;
								
								break;
              default:
								att.vals = (double *) malloc(att.len * sizeof(double));
								
								nc_status = nc_get_att_double(*ncid, NC_GLOBAL, att.name, att.vals );
								if (nc_status != NC_NOERR) return nc_status;
								
								break;
              }
							
						}  /* end of the  if (nc_status == NC_NOERR) */
					}
					/*Save attribute in linked list of attributes for variable . (global attributes)*/	
					if (!var.attrs_list_initialized) {
						if ( (var.varattlist = list_init()) == NULL ) {
							fprintf(stderr, "ERROR: ncf_add_dset: Unable to initialize GLOBAL attributes list.\n");
							return_val = -1;
							return return_val; 
						}
            var.attrs_list_initialized = TRUE;
  	      }
					
					list_insert_after(var.varattlist, (char *) &att, sizeof(ncatt));
				}    /* global attributes list complete */
			
      /*Save variable in linked list of variables for this dataset */	
			if (!nc.vars_list_initialized) {
				if ( (nc.dsetvarlist = list_init()) == NULL ) {
					fprintf(stderr, "ERROR: ncf_add_dset: Unable to initialize variable list.\n");
					return_val = -1;
					return return_val; 
				}
				nc.vars_list_initialized = TRUE;
			}
			
			list_insert_after(nc.dsetvarlist, (char *) &var, sizeof(ncvar));
			
		}    
	
	/* get info on variables */
	
	if (nc.nvars > 0)
		for (iv = 0; iv < nc.nvars; iv++)
			{
				nc_status = nc_inq_var(*ncid, iv, var.name, &var.type, &var.ndims,
									     var.dims, &var.natts);
				if (nc_status != NC_NOERR) return nc_status;
				
				var.varid = iv+1;  
				var.outtype = NC_FLOAT;
				if (var.type == NC_CHAR) var.outtype = NC_CHAR;
				var.outtype = var.type;  /* ?? */
				var.uvarid = 0;
				
				/* Is this a coordinate variable? If not a string, set the flag.
				/* A multi-dimensional variable that shares a dimension name is not a coord. var.
				 */
				if (nc.ndims > 0) {
					var.is_axis = FALSE;
					var.axis_dir = 0;
					i = 0;
					while (i < nc.ndims && var.is_axis == FALSE) {
						if  (strcasecmp(var.name, nc.dims[i].name) == 0) var.is_axis = TRUE;
						if  (var.type == NC_CHAR) var.is_axis = FALSE;
						if  (var.ndims > 1) var.is_axis = FALSE;
						i = i + 1;
					}
				}
				
				/* get _FillValue attribute */
				
				nc_status =  nc_inq_att(*ncid,iv,"_FillValue",&att.type, &len);
				att.len = (int)len;
				
				if(nc_status == NC_NOERR &&
					 att.type == var.type && att.len == 1) {
					
					att.outflag = 1;
					att.outtype = var.type;
					var.has_fillval = TRUE;
					if(var.type == NC_CHAR) {
						att.outtype = NC_CHAR;
						nc_status = nc_get_att_text(*ncid, iv, "_FillValue",
																				&fillc );
						if (nc_status != NC_NOERR)  /* on error set attr to empty string */
							{ att.type = NC_CHAR;
								att.outtype = NC_CHAR;
								att.len = 1;
								att.string = (char *) malloc((att.len+1)* sizeof(char));
								strcpy (att.string," ");
								att.vals = (double *) malloc(1 * sizeof(double));
								att.vals[0] = 0;
								return_val = bad_file_attr;
							}
					} else {
						nc_status = nc_get_att_double(*ncid, iv, "_FillValue",
																					&var.fillval ); }
					att.string = (char *) malloc(2*sizeof(char));
					strcpy(att.string," ");
		    }
				else  /* set to default NC value*/ 
					{
						var.has_fillval = FALSE;
						switch (var.type) {
						case NC_BYTE:
							/* don't do default fill-values for bytes, too risky */
							var.has_fillval = 0;
							break;
						case NC_CHAR:
							var.fillval = NC_FILL_CHAR;
							break;
						case NC_SHORT:
							var.fillval = NC_FILL_SHORT;
							break;
						case NC_INT:
							var.fillval = NC_FILL_INT;
							break;
						case NC_FLOAT:
							var.fillval = NC_FILL_FLOAT;
							break;
						case NC_DOUBLE:
							var.fillval = NC_FILL_DOUBLE;
							break;
						default:
							break;
							att.string = (char *) malloc(2*sizeof(char));
							strcpy (att.string, " ");
						}
					}
				
				var.all_outflag = 1;
				
				/* get all variable attributes 
				 *  For string attributes, allocate one more than the att.len, 
				 *  presumably for the null terminator for the string. See Netcdf User's Guide, nc_get_att_text.
				 */
				var.attrs_list_initialized = FALSE;
				
				ia = 0;
				if (var.natts > 0)
          {
						for (ia = 0; ia < var.natts; ia++)
							
							{
								
								/* initialize
									 att = att0; */
								
								nc_status = nc_inq_attname(*ncid, iv, ia, att.name);
								/*		  if (nc_status != NC_NOERR) fprintf(stderr, " ***NOTE: error reading attribute id %d for variable %s, file %s\n", ia, var.name, nc.fullpath); */
								if (nc_status == NC_NOERR) {
									att.attid = ia+1;
									
									nc_status = nc_inq_att(*ncid, iv, att.name, &att.type, &len);
									att.len = (int)len;

									/*            if (nc_status != NC_NOERR) fprintf(stderr, " ***NOTE: error reading attribute %s for variable %s, file %s\n",att.name, var.name, nc.fullpath); */
									if (nc_status == NC_NOERR) {
										
										if (att.len == 0) {	/* set 0-length attributes to empty strings */
											att.type = NC_CHAR;
											att.outtype = NC_CHAR;
											att.len = 1;
											att.string = (char *) malloc(2*sizeof(char));
											strcpy (att.string," ");
										}
										switch (att.type) {
										case NC_CHAR:
											/* Plus one for end-of-string delimiter. */
											att.string = (char *) malloc((att.len+1)*sizeof(char));
											strcpy (att.string, " ");
											nc_status = nc_get_att_text(*ncid, iv, att.name, att.string );											
											
											if (nc_status != NC_NOERR) /* on error set attr to empty string*/
												{att.type = NC_CHAR;
													att.outtype = NC_CHAR;
													att.len = 1;
													att.string = (char *) malloc((att.len+1)*sizeof(char));
													strcpy (att.string, " ");
													return_val = bad_file_attr;
												} else {
												/* Ensure end-of-string delimiter because Netcdf API doesn't store automatically; it's up to the file's author. */
												att.string[att.len] = '\0';
												
												/* Check the actual string length (one example file has
													 attribute units="m" but gets att.len = 128 from nc_inq_att above) because user probably used some arbitrarily large string buffer and partially populated the string leaving the remainder filled with '\0'.
												*/
												ilen = strlen(att.string);
												if (ilen < att.len)
													{ 
														att.len = ilen;
													}											
											}
											
											att.vals = (double *) malloc(1 * sizeof(double));
											att.vals[0] = 0;
											
											break;
										default:
#ifdef double_p
											att.outtype = NC_DOUBLE;
#else
											att.outtype = NC_FLOAT;
#endif
											att.vals = (double *) malloc(att.len * sizeof(double));
											
											nc_status = nc_get_att_double(*ncid, iv, att.name, att.vals );
											if (nc_status != NC_NOERR) /* on error set attr to empty string*/
												{att.type = NC_CHAR;
													att.outtype = NC_CHAR;
													att.len = 1;
													att.string = (char *) malloc((att.len+1)* sizeof(char));
													strcpy (att.string, " ");
													return_val = bad_file_attr;
												}
											att.string = (char *) malloc(2*sizeof(char));
											strcpy(att.string, " ");
											break;
										}
										
										/* Initialize output flag. Attributes written by default by Ferret
											 will be set to outflag = 1. 
										*/
										att.outflag = initialize_output_flag (att.name, var.is_axis);
										
									} /* end of the if (nc_status == NC_NOERR)  */
								}
								/*Save attribute in linked list of attributes for this variable */	
								if (!var.attrs_list_initialized) {
									if ( (var.varattlist = list_init()) == NULL ) {
										fprintf(stderr, "ERROR: ncf_add_dset: Unable to initialize variable attributes list.\n");
										return_val = -1;
										return return_val; 
									}
									var.attrs_list_initialized = TRUE;
								}
								
								list_insert_after(var.varattlist, (char *) &att, sizeof(ncatt));
							}    /* variable attributes from file complete */
					}  /* if var.natts > 0*/ 
				
/*                    /* If this is a coordinate variable, add an attribute orig_file_axname which 
/*				          contains the axis name, and is used to preserve the original name if 
/*						  Ferret detects a duplicate axis name and changes the axis name.*/
					if (var.is_axis)
					{


						/* initialize
									 att = att0; */
								
						var.natts = var.natts + 1;
						strcpy (att.name, "orig_file_axname");
						att.attid = ia+1;
						att.type = NC_CHAR;
						att.len = strlen(var.name);
						att.string = (char *) malloc((att.len+1)*sizeof(char));

						strcpy (att.string,var.name);
		                                
						/* Ensure end-of-string delimiter because Netcdf API doesn't store automatically; it's up to the file's author. */
						att.string[att.len] = '\0';
											
						att.vals = (double *) malloc(1 * sizeof(double));
						att.vals[0] = 0;
								
						/* Output flag always false for this attribute */
						att.outflag = -1;
	
						/*Save attribute in linked list of attributes for this variable */	
						if (!var.attrs_list_initialized) 
							{
							if ( (var.varattlist = list_init()) == NULL ) 
								{
								fprintf(stderr, "ERROR: ncf_add_dset: Unable to initialize variable attributes list.\n");
								return_val = -1;
								return return_val; 
								}
							var.attrs_list_initialized = TRUE;
							}
								
							list_insert_after(var.varattlist, (char *) &att, sizeof(ncatt));
					}

				/*Save variable in linked list of variables for this dataset */	
				if (!nc.vars_list_initialized) {
          if ( (nc.dsetvarlist = list_init()) == NULL ) {
            fprintf(stderr, "ERROR: ncf_add_dset: Unable to initialize variable list.\n");
            return_val = -1;
            return return_val; 
          }
          nc.vars_list_initialized = TRUE;
        }
				
				list_insert_after(nc.dsetvarlist, (char *) &var, sizeof(ncvar));
				
			}    /* variables list complete */
	
	/* Add dataset to global nc dataset linked list*/ 
  if (!list_initialized) {
    if ( (GLOBAL_ncdsetList = list_init()) == NULL ) {
      fprintf(stderr, "ERROR: ncf_add_dset: Unable to initialize GLOBAL_ncDsetList.\n");
      return_val = -1;
      return return_val; 
		}
    list_initialized = TRUE;
  }
	
  list_insert_after(GLOBAL_ncdsetList, (char *) &nc, sizeof(ncdset));
  return return_val;
}

/* ----
 * Initialize new dataset to contain a non-netcdf dataset 
 * save in GLOBAL_ncdsetList for attribute handling 
 */

int FORTRAN(ncf_init_other_dset)(int *setnum, char name[], char path[])

{
  ncdset nc; 
  static int return_val=FERR_OK; /* static because it needs to exist after the return statement */
  
    int i;				/* loop controls */
	int ia;
	int iv;
    int nc_status;		/* return from netcdf calls */
    ncatt att;			/* attribute */
    ncvar var;			/* variable */
		att.vals = NULL;
		att.string = NULL;
    strcpy(nc.fername, name);
    strcpy(nc.fullpath, path);
    nc.fer_dsetnum = *setnum;

    nc.ngatts = 1;
    nc.nvars = 0;
	nc.recdim = -1;   /* not used, but initialize anyway*/
	nc.ndims = 6;
	nc.its_agg = 0;
	nc.num_agg_members = 0;
    nc.vars_list_initialized = FALSE;

   /* set up pseudo-variable . the list of variables */

       strcpy(var.name, ".");

       var.attrs_list_initialized = FALSE;

       var.type = NC_CHAR;
       var.outtype = NC_CHAR;
       var.varid = 0;
	   var.natts = nc.ngatts;
       var.has_fillval = FALSE;
#ifdef double_p
	   var.fillval = NC_FILL_DOUBLE;
#else
	   var.fillval = NC_FILL_FLOAT;
#endif
	   var.all_outflag = 1;
	   var.is_axis = FALSE;
	   var.axis_dir = 0;		
		 var.ndims = 0;
	   var.attrs_list_initialized = FALSE; 

   /* set one global attribute, history */

		  att.outflag = 1;
          att.type = NC_CHAR;
          att.outtype = NC_CHAR;
          att.outflag = 0;
          att.attid = 1;
		  att.len = strlen(name);
          strcpy(att.name, "history" );

	      att.string = (char *) malloc((att.len+1)* sizeof(char));
		  strcpy(att.string, name );

      /*Save attribute in linked list of attributes for variable .*/	
       if (!var.attrs_list_initialized) {
          if ( (var.varattlist = list_init()) == NULL ) {
            fprintf(stderr, "ERROR: ncf_init_other_dset: Unable to initialize GLOBAL attributes list.\n");
            return_val = -1;
            return return_val; 
          }
          var.attrs_list_initialized = TRUE;
	  }

       list_insert_after(var.varattlist, (char *) &att, sizeof(ncatt));

       /* global attributes list complete */

      /*Save variable in linked list of variables for this dataset */	
       if (!nc.vars_list_initialized) {
          if ( (nc.dsetvarlist = list_init()) == NULL ) {
            fprintf(stderr, "ERROR: ncf_init_uvar_dset: Unable to initialize variable list.\n");
            return_val = -1;
            return return_val; 
          }
          nc.vars_list_initialized = TRUE;
        }

       list_insert_after(nc.dsetvarlist, (char *) &var, sizeof(ncvar));

/* Add dataset to global nc dataset linked list*/ 
  if (!list_initialized) {
    if ( (GLOBAL_ncdsetList = list_init()) == NULL ) {
      fprintf(stderr, "ERROR: ncf_init_uvar_dset: Unable to initialize GLOBAL_ncDsetList.\n");
      return_val = -1;
      return return_val; 
	}
    list_initialized = TRUE;
  }

  list_insert_after(GLOBAL_ncdsetList, (char *) &nc, sizeof(ncdset));
  return_val = FERR_OK;
  return return_val;
  }

/* ----
 * Find a dataset based on an integer id and return the nc_ptr.
 */
ncdset *ncf_ptr_from_dset(int *dset)
{
  static ncdset *nc_ptr=NULL;
  int status=LIST_OK;

  status = list_traverse(GLOBAL_ncdsetList, (char *) dset, NCF_ListTraverse_FoundDsetID, (LIST_FRNT | LIST_FORW | LIST_ALTR));

  /*
   * If the search failed, print a warning message and return.
   */
  if ( status != LIST_OK ) {
    /* fprintf(stderr, "\nERROR: in ncf_ptr_from_dset: No dataset of id %d was found.\n\n", *dset); */
    return NULL;
  }

  nc_ptr=(ncdset *)list_curr(GLOBAL_ncdsetList); 
  
  return nc_ptr;
}

/* ----
 * Find a dataset based on its integer ID and return a pointer to its variable list
 */

LIST *ncf_get_ds_varlist( int *dset)
{
  ncdset *nc_ptr=NULL;
  static LIST *list_ptr=NULL;

  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL ) { return NULL; }

  list_ptr=nc_ptr->dsetvarlist; 
  return list_ptr;
}

/* ----
 * Find a variable based on its dataset and variable IDs 
 * and return a pointer to its attribute list
 */

LIST *ncf_get_ds_var_attlist( int *dset, int *varid)
{
  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;
  static LIST *varlist=NULL;
  static LIST *att_ptr=NULL;
  int status;

   /*
   * Get the list of variables.  
   */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return NULL;

  var_ptr=(ncvar *)list_curr(varlist); 

  att_ptr=var_ptr->varattlist; 
  return att_ptr;
}

/* ----
 * Find a dataset based on its integer ID and return a pointer to its aggregate member list
 */

LIST *ncf_get_ds_agglist( int *dset)
{
  ncdset *nc_ptr=NULL;
  static LIST *list_ptr=NULL;

  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL ) { return NULL; }

  list_ptr=nc_ptr->agg_dsetlist; 
  return list_ptr;
}

/* ----
 * Find a variable based on its dataset and variable IDs 
 * and return a pointer to its aggregate-grid list
 */

LIST *ncf_get_ds_var_gridlist( int *dset, int *varid)
{
  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;
  static LIST *varlist=NULL;
  static LIST *grids_ptr=NULL;
  int status;

   /*
   * Get the list of variables.  
   */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return NULL;

  var_ptr=(ncvar *)list_curr(varlist); 

  grids_ptr=var_ptr->varagglist; 
  return grids_ptr;
}

/* ----
/*
 * Deallocates ncatts for a ncvar.
 */
void ncf_free_attlist(ncvar* varptr)
{
	LIST_ELEMENT *lp, *prev;
	int status = TRUE;
	ncatt* att;

	if (varptr == NULL) 
		return;

	/* Traverse list */
	lp = varptr->varattlist->front;

	if (lp == NULL) 
		return;

	while(status) {		
		att = (ncatt*) lp->data;

		if (att->string != NULL) {
		 	free(att->string);
	 	}

 		if (att->vals != NULL) {
	 		free(att->vals);
		}

		if (lp->next  == NULL) {
			status = FALSE;
		} else {
			lp = lp->next;
		}
	}
}

/* ----
 * Remove a dataset from the global dataset list
 */

int FORTRAN(ncf_delete_dset)(int *dset)
{

  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;
  ncatt *att_ptr=NULL;
  static int return_val;
  LIST *varlist;
  LIST *dummy;
  int ivar;
	LIST_ELEMENT *lp;

/* Find the dataset
 */ 

  return_val = ATOM_NOT_FOUND;
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL ) { return return_val; }

  /* For each variable, deallocate the list of attributes, 
   * and remove the variable from the dataset list
   */

  varlist = ncf_get_ds_varlist(dset);
  var_ptr=(ncvar *)list_front(varlist); 
  for (ivar = 0; ivar< nc_ptr->nvars ;ivar ++ )
  {  
/*	 list_free(var_ptr->varattlist, LIST_DEALLOC); */ /* removed here just for testing...*/
     list_remove_curr(varlist);

     /* Point to next variable */
     dummy = list_mvnext(varlist);
     var_ptr=(ncvar *)list_curr(varlist); 
  }

	/* Remove dataset from dataset list */
  list_remove_curr(GLOBAL_ncdsetList);

  return_val = FERR_OK;
  return return_val;
  }

/* ----
 * Add a new variable to a dataset.
 * If varid is < 0, set it to nvars+1 for this dataset, return varid
 * and store -1*varid as the user variable ID (uvarid)
 */
int  FORTRAN(ncf_add_var)( int *dset, int *varid, int *type, int *coordvar, char *varname, char title[], char units[], double *bad)

{
  ncdset *nc_ptr=NULL;
  ncatt att;
  ncvar var;
  ncagg_var_descr vdescr;
  int status=LIST_OK;
  static int return_val;
  int *i;
  int newvar;
  int my_len;
  LIST *vlist=NULL;
	ncvar* var_ptr;
	LIST_ELEMENT *lp;
	ncatt* att_ptr;
	att.vals = NULL;
	att.string = NULL;

   /*
   * Get the dataset pointer.  
   */
  return_val = ATOM_NOT_FOUND;  
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL )return return_val;

   /*
   * Get the list of variables.  See if this variable already exists.
   */
  newvar = FALSE;
  vlist = ncf_get_ds_varlist(dset);
  status = list_traverse(vlist, varname, NCF_ListTraverse_FoundVarName, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    newvar = TRUE;
  }

  if (newvar != TRUE) {
		/* If this variable is not new, remove the old definition of it		 */
		FORTRAN(ncf_delete_var)(dset, varname);
  }
	
	nc_ptr->nvars = nc_ptr->nvars + 1;

   /*
    * Set variable structure and insert the new variable at the end of the 
	* variable list. The type is not known at this time.
    */

  strcpy(var.name,varname);
  var.type = *type;
  var.outtype = *type;
  var.ndims = 6;
  var.natts = 0;
  var.nmemb = 0;
     
  if (*varid < 0)
    {
      /* user variable (aka "LET") */
      var.uvarid = -1* *varid;  /* value of uvar as found in Ferret */
      if (*dset == PDSET_UVARS)
        /* for global uvars,  varid always matches uvarid */
        /* which means that gaps may occur in the varid sequence */
         var.varid = var.uvarid;
      else
        /* for LET/D uvars, varid is the var count */
        /* ==> gaps in varid must be compacted when LET/D vars are deleted */
        var.varid = nc_ptr->nvars;
    }
  else
    /* file variable */
    {
      var.varid = nc_ptr->nvars;
      var.uvarid = 0;   /* 0 signals a file var */ 
    }

  var.is_axis = *coordvar;
  var.axis_dir = 0;
  var.has_fillval = FALSE;
  var.all_outflag = 1;
  var.fillval = 0;  /* initialize this */
  var.attrs_list_initialized = FALSE;

  if ( (var.varattlist = list_init()) == NULL ) {
      fprintf(stderr, "ERROR: ncf_add_var: Unable to initialize attributes list.\n");
      return_val = -1;
      return return_val; 
      }
  var.attrs_list_initialized = TRUE;

   /* Set up initial set of attributes*/

/*  Save the long_name, all variables
 *  For string attributes, allocate one more than the att.len, 
 *  presumably for the null terminator for the string (?) */

    var.natts = var.natts+1;
    strcpy(att.name, "long_name");    
	att.type = NC_CHAR; 
	att.outtype = NC_CHAR;
    att.attid = var.natts;
    att.outflag = 1;
    att.len = strlen(title);
    att.string = (char *) malloc((att.len+1)* sizeof(char));
    strcpy(att.string, title);
	
    att.vals = (double *) malloc(1 * sizeof(double));
    att.vals[0] = 0; 

    /*Save attribute in linked list of attributes for variable .*/
    list_insert_after(var.varattlist, (char *) &att, sizeof(ncatt));

/*  Now the units, if given
 *  For the units string, allocate one more than the att.len, 
 *  presumably for the null terminator for the string (?)*/

    if (strlen(units) > 0 )
		{
		var.natts = var.natts+1;

		att.attid = var.natts;
		strcpy(att.name, "units");
		att.len = strlen(units);
		att.outflag = 1;
		att.type = NC_CHAR;
		att.outtype = NC_CHAR;
		att.string = (char *) malloc((att.len+1)* sizeof(char));
		strcpy(att.string, units);

        my_len = 1;
	    att.vals = (double *) malloc(my_len * sizeof(double)); 
        att.vals[0] = 0;


/*Save attribute in linked list of attributes for this variable */	
	  list_insert_after(var.varattlist, (char *) &att, sizeof(ncatt));
		}


/* Now the missing_value, for numeric variables*/
/*    if (*type != NC_CHAR)
    { */
		var.natts = var.natts+1;
                var.fillval = *bad;

		att.attid = var.natts;
		strcpy(att.name,"missing_value");
		att.len = 1;
		att.string = NULL;
#ifdef double_p
		att.type = NC_FLOAT;
		att.outtype = NC_FLOAT;
#else
		att.type = NC_DOUBLE;
		att.outtype = NC_DOUBLE;
#endif
		att.vals = (double *) malloc(att.len * sizeof(double));
		att.vals[0] = *bad;

    /* Initialize output flag. Attributes written by default by Ferret
	   will be set to outflag = 1. 
	*/
          att.outflag = initialize_output_flag (att.name, var.is_axis);

      /*Save attribute in linked list of attributes for this variable */	

       list_insert_after(var.varattlist, (char *) &att, sizeof(ncatt));
 /* 

    /* If this is an aggregate dataset, initialize the list of member-info
	    for the variable. The values will be filled in later. */

       var.agg_list_initialized = FALSE;

         if ( (var.varagglist = list_init()) == NULL ) {
             fprintf(stderr, "ERROR: ncf_add_var: Unable to initialize aggregate info list.\n");
             return_val = -1;
             return return_val; 
             }
         var.agg_list_initialized = TRUE;

         vdescr.imemb = 0;
         vdescr.gnum = 0;
         list_insert_after(var.varagglist, (char *) &vdescr, sizeof(ncatt));

/* if it's a uvar, then initialize a grid LIST for it */ 
  if (var.uvarid == 0) 
    var.uvarGridList = 0;
  else
    {
      if ( (var.uvarGridList = list_init()) == NULL ) {
	fprintf(stderr, "ERROR: ncf_add_var: Unable to initialize uvar grid list.\n");
	return_val = -1;
	return return_val; 
      }
    }
/*Save variable in linked list of variables for this dataset */

  list_insert_after(nc_ptr->dsetvarlist, (char *) &var, sizeof(ncvar));

  return_val = FERR_OK;
  return return_val;
}

/* ----
 * Add a new variable to the pseudo user-defined coordinate variable dataset.
 */
int  FORTRAN(ncf_add_coord_var)( int *dset, int *varid, int *type, int *coordvar, char varname[], char units[], double *bad)

{
  ncdset *nc_ptr=NULL;
  ncatt att;
  ncvar var;
  int status=LIST_OK;
  static int return_val;
  int *i;
  int newvar;
  int my_len;
  LIST *vlist=NULL;
	att.vals = NULL;
	att.string = NULL;
   /*
   * Get the dataset pointer.  
   */
  return_val = ATOM_NOT_FOUND;  
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL )return return_val;

   /*
   * Get the list of variables.  See if this variable already exists.
   */
  newvar = FALSE;
  vlist = ncf_get_ds_varlist(dset);
  status = list_traverse(vlist, varname, NCF_ListTraverse_FoundVarName, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    newvar = TRUE;
  }
  nc_ptr->nvars = nc_ptr->nvars + 1;

  if (newvar == TRUE)
  {
  }
  else
   /* If this variable is not new, remove the old definition of it
   */
  {
  list_remove_curr(vlist);

  }
  
/*
   * initialize the variable definition.
   */
       strcpy(var.name, " ");

       var.attrs_list_initialized = FALSE;

       var.type = NC_CHAR;
       var.outtype = NC_CHAR;
       var.varid = 0;
	   var.natts = 0;
	   var.nmemb = 0;
       var.has_fillval = FALSE;
       var.fillval = NC_FILL_FLOAT;
	   var.all_outflag = 1;
	   var.is_axis = FALSE;
	   var.axis_dir = 0;

   /*
    * Set variable structure and insert the new variable at the end of the 
	* variable list.
    */

  strcpy(var.name,varname);
  var.type = *type;
  var.outtype = *type;
  var.ndims = 6;
  var.natts = 0;  
  var.varid = nc_ptr->nvars;
  *varid = nc_ptr->nvars;
  var.is_axis = *coordvar;
  var.axis_dir = 0;
  var.has_fillval = FALSE;
  var.all_outflag = 1;
  var.fillval = *bad;	   
  var.attrs_list_initialized = FALSE;

  if ( (var.varattlist = list_init()) == NULL ) {
      fprintf(stderr, "ERROR: ncf_add_coord_var: Unable to initialize attributes list.\n");
      return_val = -1;
      return return_val; 
      }
  var.attrs_list_initialized = TRUE;

   /* Set up initial set of attributes*/

/*  Units, if given
 *  For the units string, allocate one more than the att.len, 
 *  presumably for the null terminator for the string (?)*/

    if (strlen(units) > 0 )
		{
		var.natts = var.natts+1;

		att.attid = var.natts;
		strcpy(att.name, "units");
		att.len = strlen(units);
		att.outflag = 1;
		att.type = NC_CHAR;
		att.outtype = NC_CHAR;
		att.string = (char *) malloc((att.len+1)* sizeof(char));
		strcpy(att.string, units);

        my_len = 1;
	    att.vals = (double *) malloc(my_len * sizeof(double)); 
        att.vals[0] = 0;

      /*Save attribute in linked list of attributes for this variable */	

          var.attrs_list_initialized = TRUE;
          list_insert_after(var.varattlist, (char *) &att, sizeof(ncatt));
        }

 /*   } */

/*Save variable in linked list of variables for this dataset */

    list_insert_after(nc_ptr->dsetvarlist, (char *) &var, sizeof(ncvar));
  
  return_val = FERR_OK;
  return return_val;
}

/* ----
 * Find a variable based on its variable ID and dataset ID
 * Add a new numeric attribute.
 */
int  FORTRAN(ncf_add_var_num_att)( int *dset, int *varid, char attname[], 
int *attype, int *attlen, int *outflag, DFTYPE *vals)

{
  ncatt *att_ptr=NULL;
  ncvar *var_ptr=NULL;
  ncatt att;
  int status=LIST_OK;
  static int return_val;
  int i;
  LIST *varlist;
  LIST *varattlist;
	att.vals = NULL;
	att.string = NULL;
   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 
  if (var_ptr->natts < 1) return ATOM_NOT_FOUND;

    /*
    * Get the list of attributes for the variable in the dataset
    * If the attribute is already defined, return -1* attid
    */
  varattlist = ncf_get_ds_var_attlist(dset, varid);

  status = list_traverse(varattlist, attname, NCF_ListTraverse_FoundVarAttName, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status == LIST_OK ) {
    att_ptr=(ncatt *)list_curr(varattlist); 
    return_val = -1* att_ptr->attid; 
    return return_val;
    }

   /* Increment number of attributes.  
   */

  var_ptr->natts = var_ptr->natts + 1;

   /*
    * Set attribute structure and insert the new attribute at 
	* the end of the attribute list. 
    */

  strcpy(att.name,attname);
  att.attid = var_ptr->natts;
  att.type = *attype;
#ifdef double_p
  att.outtype = NC_FLOAT;
#else
  att.outtype = NC_DOUBLE;
#endif
  att.len = *attlen;
  att.outflag = *outflag;
	att.string = NULL;
  att.vals = (double *) malloc(*attlen * sizeof(double));

  for (i = 0; i<*attlen;i++ )
  {att.vals[i] = vals[i];
  }

   /*Save attribute in linked list of attributes for this variable */	

  list_insert_after(var_ptr->varattlist, (char *) &att, sizeof(ncatt));

  return_val = FERR_OK;
  return return_val;
}

/* ----
 * Find a variable based on its variable ID and dataset ID
 * Add a new numeric attribute.
 */
int  FORTRAN(ncf_add_var_num_att_dp)( int *dset, int *varid, char attname[], int *attype, int *attlen, int *outflag, double *vals)

{
  ncatt *att_ptr=NULL;
  ncvar *var_ptr=NULL;
  ncatt att;
  int status=LIST_OK;
  static int return_val;
  int i;
  LIST *varlist;
  LIST *varattlist;
	att.vals = NULL;
	att.string = NULL;
   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 
  if (var_ptr->natts < 1) return ATOM_NOT_FOUND;

    /*
    * Get the list of attributes for the variable in the dataset
    * If the attribute is already defined, return -1* attid
    */
  varattlist = ncf_get_ds_var_attlist(dset, varid);

  status = list_traverse(varattlist, attname, NCF_ListTraverse_FoundVarAttName, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status == LIST_OK ) {
    att_ptr=(ncatt *)list_curr(varattlist); 
    return_val = -1* att_ptr->attid; 
    return return_val;
    }

   /* Increment number of attributes.  
   */

  var_ptr->natts = var_ptr->natts + 1;

   /*
    * Set attribute structure and insert the new attribute at 
	* the end of the attribute list. 
    */

  strcpy(att.name,attname);
  att.attid = var_ptr->natts;
  att.type = *attype;
  att.outtype = NC_DOUBLE;
  att.len = *attlen;
  att.outflag = *outflag;
	att.string = NULL;
  att.vals = (double *) malloc(*attlen * sizeof(double));

  for (i = 0; i<*attlen;i++ )
  {att.vals[i] = vals[i];
  }

   /*Save attribute in linked list of attributes for this variable */	

  list_insert_after(var_ptr->varattlist, (char *) &att, sizeof(ncatt));

  return_val = FERR_OK;
  return return_val;
}

/* ----
 * Find a variable  based on its variable ID and dataset ID
 * Add a new string attribute.
 */
int  FORTRAN(ncf_add_var_str_att)( int *dset, int *varid, char attname[], int *attype, int *attlen, int *outflag, char attstring[])

{
  ncdset *nc_ptr=NULL;
  ncatt *att_ptr=NULL;
  ncvar *var_ptr=NULL;
  ncatt att;
  int status=LIST_OK;
  int return_val;
  int i;
  LIST *varlist;
  LIST *varattlist;
	att.vals = NULL;
	att.string = NULL;

   /*
   * Get the dataset pointer.  
   */
  return_val = ATOM_NOT_FOUND;  
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL )return return_val;

   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 
  
    /*
    * here if natts < 1 we should initialize the list!
    */
      /*Save attribute in linked list of attributes for variable */	
  if (!var_ptr->attrs_list_initialized) {
    if ( (var_ptr->varattlist = list_init()) == NULL ) {
      fprintf(stderr, "ERROR: add_var_str_att: Unable to initialize attributes list.\n");
      return_val = -1;
      return return_val; 
     }
    var_ptr->attrs_list_initialized = TRUE;
  }

    /*
    * Get the list of attributes for the variable in the dataset
    * If the attribute is already defined, return -1* attid
    */
  varattlist = ncf_get_ds_var_attlist(dset, varid);

  status = list_traverse(varattlist, attname, NCF_ListTraverse_FoundVarAttName, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status == LIST_OK ) {
    att_ptr=(ncatt *)list_curr(varattlist); 
    return_val = -1* att_ptr->attid; 

    return return_val;
    }

   /* Increment number of attributes.  
   */
  if (*varid == 0)
  { nc_ptr->ngatts = nc_ptr->ngatts + 1; 
  }

  var_ptr->natts = var_ptr->natts + 1;

   /*
    * Set attribute structure and insert the new attribute at 
	* the end of the attribute list. 
	
    *  For string attributes, allocate one more than the att.len, 
    *  presumably for the null terminator for the string (?)
    */

  strcpy(att.name,attname);
  att.attid = var_ptr->natts;
  att.type = *attype;
  att.outtype = NC_CHAR;
  att.len = *attlen;
  att.outflag = *outflag;
  att.string = (char *) malloc((att.len+1)* sizeof(char));
  strcpy(att.string, attstring);

      /*Save attribute in linked list of attributes for this variable */	


 list_insert_after(var_ptr->varattlist, (char *) &att, sizeof(ncatt));

  return_val = FERR_OK;
  return return_val;
}

/* ----
 * Find a variable based on its variable ID and dataset ID
 * Replace the variable name with the new one passed in.
 */

int  FORTRAN(ncf_rename_var)( int *dset, int *varid, char newvarname[])

{
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  int i;
  LIST *varlist;

   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 

  /* Insert the new name. */
  strcpy( var_ptr->name, newvarname); 

  return_val = FERR_OK;
  return return_val;
}


/* ----
 * Find a dimension in the datset using dataset ID
 * Replace the dimension name with the new one passed in.
 */

int  FORTRAN(ncf_rename_dim)( int *dset, int *dimid, char newdimname[])

{
  ncdset *nc_ptr=NULL;
  int status=LIST_OK;
  int return_val;
	
   /*
   * Get the dataset pointer.  
   */
  return_val = ATOM_NOT_FOUND;  
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL ) return return_val;

  /* Insert the new name. */
  
  strcpy (nc_ptr->dims[*dimid-1].name, newdimname);

/* just return for now. */
  return_val = FERR_OK;
  return return_val;
}



/* ----
 * Find an attribute based on its variable ID and dataset ID
 * Replace the type, length, and/or value(s).
 */
int  FORTRAN(ncf_repl_var_att)( int *dset, int *varid, char attname[], int *attype, int *attlen, DFTYPE *vals, char attstring[])

{
  ncatt *att_ptr=NULL;
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  int i;
  LIST *varlist;
  LIST *varattlist;

   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 
  if (var_ptr->natts < 1) return ATOM_NOT_FOUND;

   /*
    * Get the list of attributes for the variable in the dataset
    * If the attribute is not defined, return
    */
  varattlist = ncf_get_ds_var_attlist(dset, varid);

  status = list_traverse(varattlist, attname, NCF_ListTraverse_FoundVarAttName, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
    }

   /*
    * Get the attribute.
    */
  att_ptr=(ncatt *)list_curr(varattlist); 

   /*
    * Free the memory used by the string or values 
    */
  if (att_ptr->type == NC_CHAR)
  {
	  /*free(att_ptr->string);
  }
  else
  {
	  free(att_ptr->vals);*/
  }
  

   /*
    * Keep the name and ID. Reset type, length, and values
    *  For string attributes, allocate one more than the att.len, 
    *  presumably for the null terminator for the string (?)
    */

  att_ptr->type = *attype;
  att_ptr->outtype = NC_FLOAT;
  att_ptr->len = *attlen;

  if (*attlen == 0) /* set 0-length attributes to empty strings */
	  {
		  att_ptr->type = NC_CHAR;
		  att_ptr->outtype = NC_CHAR;
		  att_ptr->len = 1;
			att_ptr->string = (char *) malloc(2*sizeof(char));
		  strcpy(att_ptr->string," ");
	  }
   else
	  {
	   switch (*attype) 
		   {
		   case NC_CHAR:
			   i = (*attlen+1);   /* this line for debugging*/
	        att_ptr->string = (char *) malloc((*attlen+1)* sizeof(char));
            strcpy(att_ptr->string,attstring);
            break;
			
		   default:
            att_ptr->vals = (double *) malloc(*attlen * sizeof(double));
	        for (i = 0; i<*attlen;i++ )
            {
				att_ptr->vals[i] = vals[i];
            }
            break;
         }
	  }

  return_val = FERR_OK;
  return return_val;
}

/* ----
 * Find an attribute based on its variable ID and dataset ID
 * Replace the type, length, and/or value(s).
 */
int  FORTRAN(ncf_repl_var_att_dp)( int *dset, int *varid, char attname[], int *attype, int *attlen, double *vals, char attstring[])

{
  ncatt *att_ptr=NULL;
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  int i;
  LIST *varlist;
  LIST *varattlist;

   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 
  if (var_ptr->natts < 1) return ATOM_NOT_FOUND;

   /*
    * Get the list of attributes for the variable in the dataset
    * If the attribute is not defined, return
    */
  varattlist = ncf_get_ds_var_attlist(dset, varid);

  status = list_traverse(varattlist, attname, NCF_ListTraverse_FoundVarAttName, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
    }

   /*
    * Get the attribute.
    */
  att_ptr=(ncatt *)list_curr(varattlist); 

   /*
    * Free the memory used by the string or values 
    */
  /*
  if (att_ptr->type == NC_CHAR)
  {
	  free(att_ptr->string);
  }
  else
  {
	  free(att_ptr->vals);
  }
  */
  

   /*
    * Keep the name and ID. Reset type, length, and values
    *  For string attributes, allocate one more than the att.len, 
    *  presumably for the null terminator for the string (?)
    */

  att_ptr->type = *attype;
#ifdef double_p
  att_ptr->outtype = NC_DOUBLE;
#else
  att_ptr->outtype = NC_FLOAT;
#endif
  att_ptr->len = *attlen;

  if (*attlen == 0) /* set 0-length attributes to empty strings */
	  {
		  att_ptr->type = NC_CHAR;
		  att_ptr->outtype = NC_CHAR;
		  att_ptr->len = 1;
			att_ptr->string = (char *) malloc(2* sizeof(char));
		  strcpy(att_ptr->string," ");
	  }
   else
	  {
	   switch (*attype) 
		   {
		   case NC_CHAR:
			   i = (*attlen+1);   /* this line for debugging*/
	        att_ptr->string = (char *) malloc((*attlen+1)* sizeof(char));
            strcpy(att_ptr->string,attstring);
            break;
			
		   default:
            att_ptr->vals = (double *) malloc(*attlen * sizeof(double));
	        for (i = 0; i<*attlen;i++ )
            {
				att_ptr->vals[i] = vals[i];
            }
            break;
         }
	  }

  return_val = FERR_OK;
  return return_val;
}

/* ---- 
 * Find an attribute based on its variable ID and dataset ID
 * Delete it.
 */
int  FORTRAN(ncf_delete_var_att)( int *dset, int *varid, char attname[])

{
  ncatt *att_ptr=NULL;
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  int i;
  int att_to_remove;
  LIST *varlist;
  LIST *varattlist;

   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 
  if (var_ptr->natts < 1) return ATOM_NOT_FOUND;

   /*
    * Get the list of attributes for the variable in the dataset
    * If the attribute is not defined, return
    */
  varattlist = ncf_get_ds_var_attlist(dset, varid);

   /*
    * Get the attribute.
    */

  status = list_traverse(varattlist, attname, NCF_ListTraverse_FoundVarAttName, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
    }

	  att_ptr=(ncatt *)list_curr(varattlist); 
      att_to_remove = att_ptr->attid;

  /* 
   * reset the attribute id for remaining attributes
   */

  for (i = 1; i <= var_ptr->natts; i++ )
	  {

	  status = list_traverse(varattlist, (char *) &i, NCF_ListTraverse_FoundVarAttID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
	  if ( status != LIST_OK ) 
		  {
		  return_val = ATOM_NOT_FOUND;
		  return return_val;
		  }

	  att_ptr=(ncatt *)list_curr(varattlist); 

	  /*
	  * Reset the attribute id?
	  */

	  if (i > att_to_remove) att_ptr->attid = att_ptr->attid -1;

	  }  /* end of iatt loop*/   

   /*
    * Now get and remove the attribute.
    */

  status = list_traverse(varattlist, attname, NCF_ListTraverse_FoundVarAttName, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
    }
  list_remove_curr(varattlist);
  
   /* Decrement number of attributes for the variable.  
   */

  var_ptr->natts = var_ptr->natts - 1;


  return_val = FERR_OK;
  return return_val;
  }

/* ---- 
 * Find an attribute based on its variable ID and dataset ID
 * Change its output flag: 1=output it, 0=dont.
 */
int  FORTRAN(ncf_set_att_flag)( int *dset, int *varid, char attname[], int *attoutflag)

{
  ncatt *att_ptr=NULL;
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  int i;
  LIST *varlist;
  LIST *varattlist;

   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 
  if (var_ptr->natts < 1) return ATOM_NOT_FOUND;

   /*
    * Get the list of attributes for the variable in the dataset
    * If the attribute is not defined, return
    */
  varattlist = ncf_get_ds_var_attlist(dset, varid);

  status = list_traverse(varattlist, attname, NCF_ListTraverse_FoundVarAttName, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
    }

   /*
    * Get the attribute.
    */
  att_ptr=(ncatt *)list_curr(varattlist); 

   /*
    * Keep the attribute as is, but reset its output flag.
    */

  att_ptr->outflag = *attoutflag;


  return_val = FERR_OK;
  return return_val;
}

/* ---- 
 * Find variable based on its variable ID and dataset ID
 * Change the variable flag: 
 * 1=output no attributes, 
   0=check individual attribute output flags,
   2=write all attributes, except any internal Ferret
     attributes, marked with outflag=-1.
*  3=reset attr flags to defaults
 */
int  FORTRAN(ncf_set_var_out_flag)( int *dset, int *varid, int *all_outflag)

{
  ncvar *var_ptr=NULL;
  ncatt *att_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  int i;
  int *iatt;
  LIST *varlist;
  LIST *varattlist;

   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 

   /*
    * Keep the default if there are no attributes
    */

  if (var_ptr->natts < 1)
  {
	  var_ptr->all_outflag = 1;
	  return FERR_OK;
  }

   /*
    * Reset the variable output flag.
    */
  var_ptr->all_outflag = *all_outflag;
  if (*all_outflag == 0)

  {
   /*
    * Get the list of attributes for the variable varid
    */
  varattlist = ncf_get_ds_var_attlist(dset, varid);

  /* 
   * reset the output flag for each attribute
   */
  for (i = 1; i <= var_ptr->natts; i++ )
	  {
	  /* *iatt = i; */

	  status = list_traverse(varattlist, (char *) &i, NCF_ListTraverse_FoundVarAttID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
	  if ( status != LIST_OK ) 
		  {
		  return_val = ATOM_NOT_FOUND;
		  return return_val;
		  }

   /*
    * Get the attribute.
    */
	  att_ptr=(ncatt *)list_curr(varattlist); 

	  /*
	  * Reset the attribute output flag.
	  */

	  att_ptr->outflag = 0;

	  }  /* end of iatt loop*/   

  }


  else if (*all_outflag == 2)

  {
   /*
    * Get the list of attributes for the variable varid
    */
  varattlist = ncf_get_ds_var_attlist(dset, varid);

  /* 
   * reset the output flag for each attribute
   */

  for (i = 1; i <= var_ptr->natts; i++ )
 {
	 /* *iatt = i; */
	 status = list_traverse(varattlist, (char *) &i, NCF_ListTraverse_FoundVarAttID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
	  if ( status != LIST_OK ) 
		  {
		  return_val = ATOM_NOT_FOUND;
		  return return_val;
		  }

   /*
    * Get the attribute.
    */

	  att_ptr=(ncatt *)list_curr(varattlist); 

	  /*
	  * Reset the attribute output flag.
	  */

	  if (att_ptr->outflag != -1) att_ptr->outflag = 1;


	  }  /* end of iatt loop*/   

 }

  else if  (*all_outflag == 3)

  {

   /*
    * Get the list of attributes for the variable varid
    */
  varattlist = ncf_get_ds_var_attlist(dset, varid);

  /* 
   * reset the output flag for each attribute to the default Ferret value 
   */
  for (i = 1; i <= var_ptr->natts; i++ )
	  {
	  /* *iatt = i; */
	  status = list_traverse(varattlist, (char *) &i, NCF_ListTraverse_FoundVarAttID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
	  if ( status != LIST_OK ) {
	    return_val = ATOM_NOT_FOUND;
	    return return_val;
	  }

   /*
    * Get the attribute.
    */
	  att_ptr=(ncatt *)list_curr(varattlist); 

	  /*
	  * Reset the attribute output flag to the Ferret default value
	    (output missing flag, etc, but not nonstd attributes from
		the input file or user definitions.)
	  */

	  att_ptr->outflag = initialize_output_flag(att_ptr->name, var_ptr->is_axis);

	  }  /* end of iatt loop*/   
  }

  return_val = FERR_OK;
  return return_val;
}

/* ---- 
 * Find variable based on its variable ID and dataset ID
 * Change the variable output type.
 */
int  FORTRAN(ncf_set_var_outtype)( int *dset, int *varid, int *outtype)

{
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  LIST *varlist;

   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 

   /*
    * Reset the variable output type.
    */
  var_ptr->outtype = *outtype;

  return_val = FERR_OK;
  return return_val;
}


/* ---- 
 * Find variable based on its variable ID and dataset ID
 * Check that its a coordinate variable and set the axis direction.
 */

int  FORTRAN(ncf_set_axdir)( int *dset, int *varid, int *axdir)

{
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  LIST *varlist;

   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(dset);

  return_val = ATOM_NOT_FOUND;
  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 

   /*
    * Reset the variable output type.
    */
  return_val = ATOM_NOT_FOUND;
  if (var_ptr->is_axis)
  {
	 var_ptr->axis_dir = *axdir;
	 return_val = FERR_OK;
  }

  return return_val;
}

/* ---- 
 * Find an attribute based on its dataset ID, variable ID and attribute ID
 * Add the attribute to variable 2 in dataset 2
 */
int  FORTRAN(ncf_transfer_att)(int *dset1, int *varid1, int *iatt, int *dset2, int *varid2)

{
  ncatt *att_ptr1=NULL;
  ncatt att;
  ncvar *var_ptr1=NULL;
  ncvar *var_ptr2=NULL;
  int status=LIST_OK;
  int return_val;
  int i;
  LIST *varlist1;
  LIST *varlist2;
  LIST *varattlist1;
  LIST *varattlist2;
	att.vals = NULL;
	att.string = NULL;
   /*
    * Get the list of variables in dset1, find pointer to variable varid1.
    */
  varlist1 = ncf_get_ds_varlist(dset1);

  status = list_traverse(varlist1, (char *) varid1, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr1=(ncvar *)list_curr(varlist1); 
  if (var_ptr1->natts < 1) return ATOM_NOT_FOUND;

   /*
    * Get the list of attributes for the variable varid1
    * If the attribute is not defined, return
    */
  varattlist1 = ncf_get_ds_var_attlist(dset1, varid1);

  status = list_traverse(varattlist1, (char *) iatt, NCF_ListTraverse_FoundVarAttID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
    }

   /*
    * Get the attribute.
    */
  att_ptr1=(ncatt *)list_curr(varattlist1); 

   /*
    * Get the list of variables in dset2, find pointer to variable varid2
    */
  varlist2 = ncf_get_ds_varlist(dset2);

  status = list_traverse(varlist2, (char *) varid2, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr2=(ncvar *)list_curr(varlist2); 
  
   /*
    * Get the list of attributes for the variable varid2
    */
  varattlist2 = ncf_get_ds_var_attlist(dset2, varid2);

   /* Increment number of attributes for varid2
   */

  var_ptr2->natts = var_ptr2->natts + 1;

   /*
    * Set attribute structure and insert the new attribute at 
	* the end of the attribute list. 
    *  For string attributes, allocate one more than the att.len, 
    *  presumably for the null terminator for the string (?)
    */
  strcpy(att.name, att_ptr1->name);
  att.attid = var_ptr2->natts;
  att.type = att_ptr1->type;
  att.outtype = att_ptr1->type;
  att.len = att_ptr1->len;
  att.outflag = att_ptr1->outflag;
  
  if (att_ptr1->type == NC_CHAR)
  {
	  att.string = (char *) malloc((att_ptr1->len+1)* sizeof(char)); 
	  strcpy(att.string, att_ptr1->string);
  }
  else
  {
	  att.vals = (double *) malloc(att_ptr1->len * sizeof(double));
	  for (i = 0; i<att_ptr1->len;i++ )
		  {att.vals[i] = att_ptr1->vals[i];
		  }
  }

  /*Save attribute in linked list of attributes for this variable */	

  list_insert_after(var_ptr2->varattlist, (char *) &att, sizeof(ncatt));

  return_val = FERR_OK;
  return return_val;
}

/* ---- 
 * Find variable based on the dataset ID and variable name
 * Delete it.
 */
int  FORTRAN(ncf_delete_var)( int *dset, char *varname)

{
  ncdset *nc_ptr=NULL;
  ncvar *var_ptr=NULL;
  ncatt *att_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  int ivar;
  int i;
  LIST *varlist;
  LIST *uvgridList;
  LIST *dummy;
  LIST_ELEMENT *lp;

 /* Find the dataset based on its integer ID 
 */
  return_val = ATOM_NOT_FOUND;  
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL )return return_val;

   /*
   * Get the list of variables. Find varname in the dataset.
   */
  varlist = ncf_get_ds_varlist(dset);
  status = list_traverse(varlist, varname, NCF_ListTraverse_FoundVarName, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
  }
  
  /* Deallocate the list of attributes */
	lp = varlist->curr;
	var_ptr = (ncvar*)lp->data;
	ivar = var_ptr->varid;
	/* Free the attributes for this var (list data). */
	ncf_free_attlist(var_ptr);
	/* Free the attributes list. */
	list_free(var_ptr->varattlist, LIST_DEALLOC);

  /* Free the list of uvarGrids */
	if (var_ptr->uvarid != 0) 
	  {
	    uvgridList = var_ptr->uvarGridList;
	    while (!list_empty(uvgridList))
	      {
		list_remove_front(uvgridList);
	      }
	    list_free(uvgridList, LIST_DEALLOC);
	  }
  /* remove the variable from the dataset list */
	list_remove_curr(varlist);
	free(var_ptr);

   /* Reset the varids for variables added to external datasets with LET/D
    * For the virtual user-variable datset, leave varids alone.
    */

   if (*dset > PDSET_UVARS)
   {
	   for (i=ivar; i <nc_ptr->nvars ;i++ )
	   {
		   var_ptr=(ncvar *)list_curr(varlist);  /* Point to next variable */
		   var_ptr->varid = var_ptr->varid - 1;
		   dummy = list_mvnext(varlist);
	   }
   }

   /* Decrement number of variables in the dataset. 
    */
  nc_ptr->nvars = nc_ptr->nvars - 1;

  return_val = FERR_OK;
  return return_val;
  }

/* ----
 * Initialize new dataset to contain an aggregate dataset 
 * save in GLOBAL_ncdsetList for attribute handling 
 */

int FORTRAN(ncf_init_agg_dset)(int *setnum, char name[])

{
  ncdset nc; 
  static int return_val=FERR_OK; /* static because it needs to exist after the return statement */
  
    int i;				/* loop controls */
	int ia;
	int iv;
    ncatt att;			/* attribute */
    ncvar var;			/* variable */
	ncagg agg;			/* list of aggregate datset members */
	att.vals = NULL;
	att.string = NULL;
    strcpy(nc.fername, name);
    nc.fer_dsetnum = *setnum;

    nc.ngatts = 1;
    nc.nvars = 0;
	nc.recdim = -1;
	nc.ndims = 6;
	nc.its_agg = 1;
	nc.num_agg_members = 0;
    nc.vars_list_initialized = FALSE;
    nc.agg_list_initialized = FALSE;

   /* set up pseudo-variable . the list of variables */

       strcpy(var.name, ".");

       var.attrs_list_initialized = FALSE;

       var.type = NC_CHAR;
       var.outtype = NC_CHAR;
       var.varid = 0;
	   var.natts = nc.ngatts;
       var.has_fillval = FALSE;
#ifdef double_p
	   var.fillval = NC_FILL_DOUBLE;
#else
	   var.fillval = NC_FILL_FLOAT;
#endif
	   var.all_outflag = 1;
	   var.is_axis = FALSE;
	   var.axis_dir = 0;		
	   var.ndims = 0;
	   var.attrs_list_initialized = FALSE; 

   /* set global attribute, aggregate name */

		  att.outflag = 1;
          att.type = NC_CHAR;
          att.outtype = NC_CHAR;
          att.outflag = 0;
          att.attid = 1;
		  att.len = strlen(name);
          strcpy(att.name, "aggregate name" );

	      att.string = (char *) malloc((att.len+1)* sizeof(char));
		  strcpy(att.string, name );

      /*Save attribute in linked list of attributes for variable .*/	
       if (!var.attrs_list_initialized) {
          if ( (var.varattlist = list_init()) == NULL ) {
            fprintf(stderr, "ERROR: ncf_init_agg_dset: Unable to initialize GLOBAL attributes list.\n");
            return_val = -1;
            return return_val; 
          }
          var.attrs_list_initialized = TRUE;
       }

		  list_insert_after(var.varattlist, (char *) &att, sizeof(ncatt));

       /* global attributes list complete */

   /* Initialize linked list of variables for this dataset */	
       if (!nc.vars_list_initialized) {
          if ( (nc.dsetvarlist = list_init()) == NULL ) {
            fprintf(stderr, "ERROR: ncf_init_agg_dset: Unable to initialize variable list.\n");
            return_val = -1;
            return return_val; 
          }
          nc.vars_list_initialized = TRUE;
        }

       list_insert_after(nc.dsetvarlist, (char *) &var, sizeof(ncvar));

  /*Initialize list of aggregate members for this dataset */	
       if (!nc.agg_list_initialized) {
          if ( (nc.agg_dsetlist = list_init()) == NULL ) {
            fprintf(stderr, "ERROR: ncf_init_agg_dset: Unable to initialize aggregate list.\n");
            return_val = -1;
            return return_val; 
          }
          nc.agg_list_initialized = TRUE;
        }
       

/* Add dataset to global nc dataset linked list*/ 
  if (!list_initialized) {
    if ( (GLOBAL_ncdsetList = list_init()) == NULL ) {
      fprintf(stderr, "ERROR: ncf_init_uvar_dset: Unable to initialize GLOBAL_ncDsetList.\n");
      return_val = -1;
      return return_val; 
	}
    list_initialized = TRUE;
  }

  list_insert_after(GLOBAL_ncdsetList, (char *) &nc, sizeof(ncdset));
  return_val = FERR_OK;
  return return_val;
  }

/* ----
 * Add a new aggregate member to an aggregate dataset.
 */
int  FORTRAN(ncf_add_agg_member)( int *dset, int *sequence_number, int *member_dset)

{
  ncdset *nc_ptr=NULL;
  int status=LIST_OK;
  static int return_val;
  LIST *elist=NULL;
  ncagg agg_ptr;
  int i;

   /*
   * Get the dataset pointer.  
   */
  return_val = ATOM_NOT_FOUND;  
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL )return return_val;

  /*Save aggregate member number in linked list of aggregate members for this dataset */	

   /*
   * Get the list of aggregate members. Put the new info at the end.
   */
  elist = ncf_get_ds_agglist(dset);
  agg_ptr.dsetnum = *member_dset;
  agg_ptr.aggSeqNo = *sequence_number;  // added 12/15 //

  list_insert_after(nc_ptr->agg_dsetlist, (char *) &agg_ptr, sizeof(agg_ptr));

  nc_ptr->num_agg_members = nc_ptr->num_agg_members + 1;

  return_val = FERR_OK;
  return return_val;
}

/* ----
 * Find a dataset based on its integer ID and return the 
 * number of aggregate member datasets
 */

int  FORTRAN(ncf_get_agg_count)( int *dset, int *num_agg_dsets)
{
  ncdset *nc_ptr=NULL;
  int return_val;

  return_val = ATOM_NOT_FOUND;
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL ) { return return_val; }

  *num_agg_dsets = nc_ptr->num_agg_members;

  return_val = FERR_OK; 
  return return_val; 
}


/* ----
 * Find a dataset based on its integer ID and for a given member number
 * return the Ferret dataset number
 * 
 */

int  FORTRAN(ncf_get_agg_member)( int *dset, int *imemb, int *membset)
{
  ncdset *nc_ptr=NULL;
  ncagg *agg_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  int num_agg_dsets;
  
  LIST *agglist;

  return_val = ATOM_NOT_FOUND;
  if ( (nc_ptr = ncf_ptr_from_dset(dset)) == NULL ) { return return_val; }

   /*
   * Get the list of aggregation members.  
   */

  agglist = ncf_get_ds_agglist(dset);

  status = list_traverse(agglist, (char *) imemb, NCF_ListTraverse_FoundDsMemb, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) {
    return_val = ATOM_NOT_FOUND;
    return return_val;
  }
  
  agg_ptr=(ncagg *)list_curr(agglist); 

  *membset = agg_ptr->dsetnum;

  return_val = FERR_OK; 
  return return_val; 
}


/* ----
 * Add description for variable in aggregate dataset.
 * Given the aggregate dataset, and the varid of the variable, and 
 * the aggregate sequence-number, save the variable type (1=file-variable, 
 * 3=user-var), the Ferret datset id, the grid, the Ferret line number
 * for the aggregate dimension, and the sequence number in ds_var_code 
 * or uvar_name_code.
 */
int  FORTRAN(ncf_add_agg_var_info)( int *dset, int *varid, int *imemb,
int *vtype, int *datid, int *igrid, int *iline, int *nv)

{
  ncvar *var_ptr=NULL;
  ncagg_var_descr vdescr;
  int status=LIST_OK;
  static int return_val;
  int i;
  LIST *varlist;
  LIST *varagglist;

   /*
    * Get the list of variables, find pointer to variable varid.
    */

  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 

    /*
    * Get the list of members for the variable in the dataset
    */
  varagglist = ncf_get_ds_var_gridlist(dset, varid);

  vdescr.imemb = *imemb;
  vdescr.vtype = *vtype;
  vdescr.datid = *datid;
  vdescr.gnum  = *igrid;
  vdescr.iline  = *iline;
  vdescr.nv    = *nv;

   /* Increment number of grid values saved.  
   */

  var_ptr->nmemb = var_ptr->nmemb + 1;

   /*Save grid number in linked list of grid for this variable */	

  list_insert_after(var_ptr->varagglist, (char *) &vdescr, sizeof(ncagg_var_descr));


  return_val = FERR_OK;
  return return_val;
}
	  
/* ----
 * For a variable in aggregate aggregate dataset, store its grid.
 * Given the aggregate dataset, the varid of the variable, and 
 * the aggregate sequence-number, save the grid of the variable.
 */
int  FORTRAN(ncf_put_agg_memb_grid)( int *dset, int *varid, int *imemb, int *igrid)

{
  ncvar *var_ptr=NULL;
  ncagg_var_descr vdescr;
  int status=LIST_OK;
  static int return_val;
  int i;
  LIST *varlist;
  LIST *varagglist;

   /*
    * Get the list of variables, find pointer to variable varid.
    */

  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 

    /*
    * Get the list of members for the variable in the dataset. Reset grid number.
    */
  varagglist = ncf_get_ds_var_gridlist(dset, varid);
  vdescr.gnum  = *igrid;

  return_val = FERR_OK;
  return return_val;
}

/* ----
 *
 * Given the aggregate dataset, varid, and member number, return the 
 * variable type (1=file-variable, 3=user-var), the Ferret datset id, 
 * the grid and the sequence number in ds_var_code or uvar_name_code.
 */
int  FORTRAN(ncf_get_agg_var_info)( int *dset, int *varid, int *imemb, int* vtype, 
 int* datid, int *igrid, int *iline, int *nv)

{
	
  ncvar *var_ptr=NULL;
  ncagg_var_descr *vdescr_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  LIST *varlist;
  LIST *varagglist;

   /*
   * Get the list of variables, find variable varid.
   */
  varlist = ncf_get_ds_varlist(dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundVarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 

   /*
   * Get the list of aggregate-grids for the variable in the dataset
   */
  varagglist = ncf_get_ds_var_gridlist(dset, varid);

  status = list_traverse(varagglist, (char *) imemb, NCF_ListTraverse_FoundVariMemb, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  vdescr_ptr=(ncagg_var_descr *)list_curr(varagglist); 

  *vtype = vdescr_ptr->vtype;
  *datid = vdescr_ptr->datid;
  *igrid = vdescr_ptr->gnum;
  *iline = vdescr_ptr->iline;
  *nv    = vdescr_ptr->nv;
  return_val = FERR_OK;
  return return_val;
}

/* ---- 
 * For attributes that Ferret always writes, set the output flag to 1
   All others are not written by default. The flag can be set to 1 by the user.
   The modulo flag is set to 0. This will be overriden ni the Ferret code
   depending on the value of the modulo attribute.
  */

static int initialize_output_flag (char *attname, int is_axis)
{
	int return_val;
    return_val = 0;

    /* attributes on coordinate variables */
	if (strcmp(attname,"axis")==0)
	{return_val = 1;
	}
	if (strcmp(attname,"units")==0)
	{return_val = 1;
	}
	if (strcmp(attname,"calendar")==0)
	{return_val = 1;
	}
	if (strcmp(attname,"positive")==0)
	{return_val = 1;
	}
	if (strcmp(attname,"point_spacing")==0)
	{return_val = 1;
	}
	if (strcmp(attname,"modulo")==0)
	{return_val = 1;
	}
	if (strcmp(attname,"time_origin")==0)
	{return_val = 1;
	}
    /* attributes on variables */
	if (strcmp(attname,"missing_value")==0)
	{return_val = 1;
	}
	if (strcmp(attname,"_FillValue")==0)
	{return_val = 1;
	}
	if (strcmp(attname,"long_name")==0)
	{return_val = 1;
	}
	if (strcmp(attname,"title")==0)
	{return_val = 1;
	}
	if (strcmp(attname,"history")==0)
	{return_val = 1;
	}
	if (strcmp(attname,"bounds")==0)
	{return_val = 1;
	}
    /* write scale attributes on non-coordinate variables */
	if (is_axis==0)
	{
	   if (strcmp(attname,"scale_factor")==0)
	   {return_val = 1;
	   }
	   if (strcmp(attname,"add_offset")==0)
	   {return_val = 1;
	   }
	}
	return return_val;

}


/* *******************************
 *  uvar grid management routines
 * *******************************
 */


/* ---- 
 * Find variable based on its variable ID and LIST_dset ID
 * Free ("purge" in Ferret-speak) the entire list of uvar grids
 */
int  FORTRAN(ncf_free_uvar_grid_list)( int *LIST_dset, int *uvarid)

{
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  LIST *varlist;
  LIST *uvgridList;

   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(LIST_dset);

    /* find the relevant LET var (i.e. uvar) */
  status = list_traverse(varlist, (char *) uvarid, NCF_ListTraverse_FoundUvarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 

  /* remove all elements from the uvar grid list */
  uvgridList = var_ptr->uvarGridList;
  while (!list_empty(uvgridList))
    {
      list_remove_front(uvgridList);
    }

  return FERR_OK;
}


/* ---- 
 * Find variable based on its variable ID and LIST_dset ID
 * Store a grid/context_dset pair for the variable 

 * The dual dataset arguments arise because Ferret's global uvars are managed
 * in the c LIST structures as a special dataset -- PDSET_UVARS
 * By contrast LET/D uvars are managed in the c LIST structure of the parent dataset
 * So we refer to the dataset that owns (parents) the uvar as LIST_dset 
 * and we refer to the dataset in which Ferret is evaluating the uvar is as context_dset
 */
int  FORTRAN(ncf_set_uvar_grid)( int *LIST_dset, int *varid, int *grid, int *datatype, int *context_dset)

{
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  LIST *varlist;
  LIST *uvgridlist;
  uvarGrid uvgrid;
  int i;

   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(LIST_dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundUvarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 

   /*  
    * if a grid already exists for this context dataset
    * remove it before continuing
    */
   /* I dunno why other routines are calling internal routines such
       as ncf_get_ds_var_attlist to get the LIST pointers they need.
       am I missing something? *sh* */
  uvgridlist = var_ptr->uvarGridList;
  status = list_traverse(uvgridlist, (char *) context_dset, NCF_ListTraverse_FoundGridDset, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status == LIST_OK)
    {
      list_remove_curr(uvgridlist);
    }

   /*
    * Fill the uvarGrid structure
    */
  /*  not needed: uvgrid = (uvarGrid *) malloc(sizeof(uvarGrid)); */
  uvgrid.grid  = *grid;
  uvgrid.dset  = *context_dset;
  uvgrid.dtype = *datatype;

   /*
    * Set the auxiliary variables as unspecified at this point
    */
  for (i=0; i<NFERDIMS ;i++ )
    {
      uvgrid.auxCat[i] = 0;
      uvgrid.auxVar[i] = 0;
    }

   /*
    * Save it in the grid list of this uvar
    */
  list_insert_after(uvgridlist, (char *) &uvgrid, sizeof(uvarGrid));

  return_val = FERR_OK;
  return return_val;
}

/* ---- 
 * Find variable based on its variable ID and LIST_dset ID
 * Return the grid that corresponds to the context_dset pair 

 * The dual dataset arguments arise because Ferret's global uvars are managed
 * in the c LIST structures as a special dataset -- PDSET_UVARS
 * By contrast LET/D uvars are managed in the c LIST structure of the parent dataset
 * So we refer to the dataset that owns (parents) the uvar as LIST_dset 
 * and we refer to the dataset in which Ferret is evaluating the uvar is as context_dset
 */
int  FORTRAN(ncf_get_uvar_grid)( int *LIST_dset, int *uvarid, int *context_dset, int *uvgrid)

{
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  LIST *varlist;
  LIST *uvgridlist;
  uvarGrid *uvgrid_ptr=NULL;

   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(LIST_dset);

    /* find the relevant LET var (i.e. uvar) */
  status = list_traverse(varlist, (char *) uvarid, NCF_ListTraverse_FoundUvarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 

    /* find the relevant grid/dataset pair owned by this uvar */
    /* I dunno why other routines are calling internal routines such
     * as ncf_get_ds_var_attlist to get the LIST pointers they need.
     * am I missing something? *sh*
     */
  uvgridlist = var_ptr->uvarGridList;
  if (list_empty(uvgridlist)) return ATOM_NOT_FOUND;

  status = list_traverse(uvgridlist, (char *) context_dset, NCF_ListTraverse_FoundGridDset, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  uvgrid_ptr=(uvarGrid *)list_curr(uvgridlist); 

  *uvgrid = uvgrid_ptr->grid;
  
  return FERR_OK;
}

/* ---- 
 * Find variable based on its variable ID and LIST_dset ID
 * Store a grid/context_dset pair for the variable 

 * The dual dataset arguments arise because Ferret's global uvars are managed
 * in the c LIST structures as a special dataset -- PDSET_UVARS
 * By contrast LET/D uvars are managed in the c LIST structure of the parent dataset
 * So we refer to the dataset that owns (parents) the uvar as LIST_dset 
 * and we refer to the dataset in which Ferret is evaluating the uvar is as context_dset
 */
int  FORTRAN(ncf_set_uvar_aux_info)( int *LIST_dset, int *varid, int aux_cat[], int aux_var[], int *context_dset)

{
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  LIST *varlist;
  LIST *uvgridlist;
  uvarGrid *uvgrid;
  int i;

   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(LIST_dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundUvarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 

   /*  
    * a grid must already exists for this context dataset
    */
  uvgridlist = var_ptr->uvarGridList;
  status = list_traverse(uvgridlist, (char *) context_dset, NCF_ListTraverse_FoundGridDset, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK)
    {
      return_val = ATOM_NOT_FOUND;
      return return_val;
    }

  uvgrid=(uvarGrid *)list_curr(uvgridlist); 

   /*
    * Fill the uvar aux arrays
    */
  for (i=0; i<NFERDIMS ;i++ )
    {
      uvgrid->auxCat[i] = aux_cat[i];
      uvgrid->auxVar[i] = aux_var[i];
    }

  return_val = FERR_OK;
  return return_val;
}

/* ---- 
 * Find variable based on its variable ID and LIST_dset ID
 * Store a grid/context_dset pair for the variable 

 * The dual dataset arguments arise because Ferret's global uvars are managed
 * in the c LIST structures as a special dataset -- PDSET_UVARS
 * By contrast LET/D uvars are managed in the c LIST structure of the parent dataset
 * So we refer to the dataset that owns (parents) the uvar as LIST_dset 
 * and we refer to the dataset in which Ferret is evaluating the uvar is as context_dset
 */
int  FORTRAN(ncf_get_uvar_aux_info)( int *LIST_dset, int *varid, int *context_dset,
                                     int aux_cat[], int aux_var[])

{
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  int return_val;
  LIST *varlist;
  LIST *uvgridlist;
  uvarGrid *uvgrid;
  int i;

   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(LIST_dset);

  status = list_traverse(varlist, (char *) varid, NCF_ListTraverse_FoundUvarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 

   /*  
    * a grid must already exists for this context dataset
    */
  uvgridlist = var_ptr->uvarGridList;
  status = list_traverse(uvgridlist, (char *) context_dset, NCF_ListTraverse_FoundGridDset, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK)
    {
      return_val = ATOM_NOT_FOUND;
      return return_val;
    }

  uvgrid=(uvarGrid *)list_curr(uvgridlist); 

   /*
    * Return the uvar aux arrays
    */
  for (i=0; i<NFERDIMS ;i++ )
    {
      aux_cat[i] = uvgrid->auxCat[i];
      aux_var[i] = uvgrid->auxVar[i];
    }

  return_val = FERR_OK;
  return return_val;
}

/* ---- 
 * Find variable based on its variable ID and LIST_dset ID
 * Return the length of the LIST of saved grids

 * The dual dataset arguments arise because Ferret's global uvars are managed
 * in the c LIST structures as a special dataset -- PDSET_UVARS
 * By contrast LET/D uvars are managed in the c LIST structure of the parent dataset
 * So we refer to the dataset that owns (parents) the uvar as LIST_dset 
 * and we refer to the dataset in which Ferret is evaluating the uvar is as context_dset
 */
int  FORTRAN(ncf_get_uvar_grid_list_len)( int *LIST_dset, int *uvarid, int *uvgrid_list_len)

{
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  LIST *varlist;
  LIST *uvgridlist;
  uvarGrid *uvgrid_ptr=NULL;

   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(LIST_dset);

    /* find the relevant LET var (i.e. uvar) */
  status = list_traverse(varlist, (char *) uvarid, NCF_ListTraverse_FoundUvarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 

  uvgridlist = var_ptr->uvarGridList;
  *uvgrid_list_len = (int)list_size(uvgridlist);
  
  return FERR_OK;
}


/* ---- 
 * Find variable based on its variable ID and LIST_dset ID
 * Delete the grid that corresponds to the context_dset
 * from the uvarGridList
 */
int  FORTRAN(ncf_delete_uvar_grid)( int *LIST_dset, int *uvarid, int *context_dset)

{
  ncvar *var_ptr=NULL;
  int status=LIST_OK;
  LIST *varlist;
  LIST *uvgridlist;
  int return_val;

   /*
    * Get the list of variables, find pointer to variable varid.
    */
  varlist = ncf_get_ds_varlist(LIST_dset);

    /* find the relevant LET var (i.e. uvar) */
  status = list_traverse(varlist, (char *) uvarid, NCF_ListTraverse_FoundUvarID, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

  var_ptr=(ncvar *)list_curr(varlist); 

    /* find the relevant grid/dataset pair owned by this uvar */
    /* I dunno why other routines are calling internal routines such
     * as ncf_get_ds_var_attlist to get the LIST pointers they need.
     * am I missing something? *sh*
     */
  uvgridlist = var_ptr->uvarGridList;
  if (list_empty(uvgridlist)) return ATOM_NOT_FOUND;

  status = list_traverse(uvgridlist, (char *) context_dset, NCF_ListTraverse_FoundGridDset, (LIST_FRNT | LIST_FORW | LIST_ALTR));
  if ( status != LIST_OK ) return ATOM_NOT_FOUND;

	/* Remove this grid from uvaGridList list */
  list_remove_curr(uvgridlist);

  return_val = FERR_OK;
  return return_val;
}


/* ***********************************
 *  search routines for LIST traversal
 * ***********************************
 */

/* ---- 
 * See if the name in data matches the ferret dset name in 
 * curr. Ferret always capitalizes everything so be case INsensitive.
 */
int NCF_ListTraverse_FoundDsetName( char *data, char *curr )
{
  ncdset *nc_ptr=(ncdset *)curr; 

  if ( !strcasecmp(data, nc_ptr->fername) ) {
    return FALSE; /* found match */
  } else
    return TRUE;
}

/* ---- 
 * See if the dataset id in data matches the ferret dset id in curr.
 */
int NCF_ListTraverse_FoundDsetID( char *data, char *curr )
{
  ncdset *nc_ptr=(ncdset *)curr; 
  int ID=*((int *)data);

  if ( ID == nc_ptr->fer_dsetnum ) {
    return FALSE; /* found match */
  } else
    return TRUE;
}

/* ---- 
 * See if the name in data matches the variable name in 
 * curr. Ferret always capitalizes everything so be case INsensitive,
 * unless the string has been passed in inside single quotes.
 */
int NCF_ListTraverse_FoundVarName( char *data, char *curr )
{
  ncvar *var_ptr=(ncvar*)curr;

  if ( !strcasecmp(data, var_ptr->name) ) {
    return FALSE; /* found match */
  } else
    return TRUE;
}

/* ---- 
 * See if the name in data matches the variable name in 
 * curr. Make the string comparison case-sensive.
 */
int NCF_ListTraverse_FoundVarNameCase( char *data, char *curr )
{
  ncvar *var_ptr=(ncvar*)curr;

  if ( !strcmp(data, var_ptr->name) ) {
    return FALSE; /* found match */
  } else
    return TRUE;
}

/* ---- 
 * See if the ID in data matches the variable ID in curr. 
 */
int NCF_ListTraverse_FoundVarID( char *data, char *curr )
{
  ncvar *var_ptr=(ncvar*)curr; 
  int ID=*((int *)data);

   if ( ID == var_ptr->varid)  {
    return FALSE; /* found match */
  } else
    return TRUE;
}

/* ---- 
 * See if the ID in data matches the uvar ID in curr. 
 */
int NCF_ListTraverse_FoundUvarID( char *data, char *curr )
{
  ncvar *var_ptr=(ncvar*)curr; 
  int ID=*((int *)data);

   if ( ID == var_ptr->uvarid)  {
    return FALSE; /* found match */
  } else
    return TRUE;
}

/* ---- 
 * See if the name in data matches the attribute name in curr.
 */
int NCF_ListTraverse_FoundVarAttName( char *data, char *curr )
{
  ncatt *att_ptr=(ncatt *)curr;

  if ( !strcasecmp(data, att_ptr->name) ) {
    return FALSE; /* found match */
  } else
    return TRUE;
}

/* ---- 
 * See if the name in data matches the attribute name in curr. 
 * Make the string comparison case-sensive.
 */
int NCF_ListTraverse_FoundVarAttNameCase( char *data, char *curr )
{
  ncatt *att_ptr=(ncatt *)curr;

  if ( !strcmp(data, att_ptr->name) ) {
    return FALSE; /* found match */
  } else
    return TRUE;
}
/* ---- 
 * See if there is an ID in data matches the attribute id in curr.
 */
int NCF_ListTraverse_FoundVarAttID( char *data, char *curr )
{
  ncatt *att_ptr=(ncatt *)curr;
  int ID=*((int *)data);

  if ( ID== att_ptr->attid)  {
    return FALSE; /* found match */
  } else
    return TRUE;
}

/* ---- 
 * See if there is an ID in data matches the dset-member id.
 */
int NCF_ListTraverse_FoundVariMemb( char *data, char *curr )
{
  ncagg_var_descr *vdescr_ptr=(ncagg_var_descr *)curr;
  int ID=*((int *)data);

  if ( ID== vdescr_ptr->imemb)  {
    return FALSE; /* found match */
  } else
    return TRUE;
}


/* ---- 
 * See if there is a match on the dset sequence number.
 */
int NCF_ListTraverse_FoundDsMemb( char *data, char *curr )
{

  ncagg *agg_ptr=(ncagg *)curr;
  int ID=*((int *)data);

  /* 12/15 -- search is successful if sequence number (FORTRAN index) matches */
  if ( ID== agg_ptr->aggSeqNo)  {
    return FALSE;
  } else
    return TRUE; /* found match */
}

/* ---- 
 * See if there is a match on the context dset
 */
int NCF_ListTraverse_FoundGridDset( char *data, char *curr )
{

  uvarGrid *uvgrid_ptr=(uvarGrid *)curr;
  int ID=*((int *)data);

  if ( ID== uvgrid_ptr->dset)  {
    return FALSE;
  } else
    return TRUE;  /* found match  (are the other comments like this wrong?)*/
}
