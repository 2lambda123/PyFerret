	SUBROUTINE LOAD_DSG_MASKVAR (dset, fmask_var, reload, status)

c set data/fmask=

* If a feaure-mask was given, store the values of the mask in line storage. 
* Save the memory location dsg_msk_lm(dset), in common. Mask data used in creating 
* feature-masks when computing feature-level masking which also may include 
* context limits in the E direftion. Will deallocate line mem dsg_msk_lm on 
* defining a new mask, closing the dataset, opr CANCEL DATA/FMASK
* 
* On a "LET mask= " find dataset(s) where the mask being defined has 
* the same name as the dataset's current mask. Here also test the 
* length of the new mask. If it's the right length for this dataset, 
* will replace its FMASK, otherwise the FMASK is canceled. 
*
* 4/20/2020 *acm* update to error message
* 6/ 3/2020 *acm* Allowing for LET command redefining an existing feature-mask variable.

	include 'tmap_dims.parm'
	include	'ferret.parm'
	include	'errmsg.parm'
	include	'slash.parm'
	include 'command.parm'
	include 'xvariables.cmn'
	include 'xcontext.cmn'
	include 'xprog_state.cmn'
#include "tmap_dset.parm"
	include 'xdset_info.cmn_text'
	include 'xtm_grid.cmn_text'
	include 'netcdf.inc'

	LOGICAL reload
	INTEGER dset, status
	CHARACTER*(*) fmask_var
	
	LOGICAL TM_LEGAL_NAME, TM_ITSA_DSG
	INTEGER TM_DSG_NFEATURES, 
     .		TM_LENSTR1, CX_DIM_LEN,
     .		i, ivar, grid, loc,  ndim, idir, npts, cx, mr1, 
     .		nfeatures, idim, dim(nferdims), attlen, varid, 
     .		attid, attype, attoutflag, slen, orient
	CHARACTER FULL_VAR_TITLE*128, TM_FMT*48,
     .		buff*128, mask_title*128
	REAL	dummy

c Make sure the name is ok
	
	IF (fmask_var .EQ. ' ') GOTO 5100
	IF ( .NOT.TM_LEGAL_NAME(fmask_var) ) GOTO 5200

* ... A feature-mask was already applied to the dataset. Wipe that one out.

	IF (dsg_msk_lm(dset) .NE. unspecified_int4 .OR. 
     .       dsg_ftrsetmsk_lm(dset) .NE. unspecified_int4)  THEN
	   
	   IF (dsg_ftrsetmsk_lm(dset) .NE. unspecified_int4) THEN
	      CALL FREE_LINE_DYNMEM( dsg_ftrsetmsk_lm(dset) )
	      CALL TM_DEALLO_DYN_LINE( dsg_ftrsetmsk_lm(dset) )
	      dsg_ftrsetmsk_lm(dset) = unspecified_int4
	   ENDIF
	   IF (dsg_msk_lm(dset) .NE. unspecified_int4)  THEN
	      CALL FREE_LINE_DYNMEM( dsg_msk_lm(dset) )
	      CALL TM_DEALLO_DYN_LINE( dsg_msk_lm(dset) )
	      dsg_msk_lm(dset) = unspecified_int4
	   ENDIF

* ... wipe memory clear of stored variables - this could change all definitions
	   DO i = 1,max_mr_avail
	      IF ( mr_protected( i ) .NE. mr_deleted ) 
     .               CALL DELETE_VARIABLE( i )
	   ENDDO

	ENDIF

	orient = dsg_orientation(dset)
	IF (orient.LT.pfeatureType_Trajectory .OR. 
     .      orient.GT.pfeatureType_TimeseriesProfile) GOTO 5300

       DO ivar = 1, maxvars
	   IF ( ds_var_setnum(ivar) .EQ. dset ) THEN
	      grid = ds_grid_number(ivar)
              EXIT
          ENDIF
       ENDDO
       
* Get an dsg grid with the e-axis from the dataset. We know
* its a dsg dataset because dsg_orientation was set.

       IF (.NOT. TM_ITSA_DSG( grid ) ) THEN
          i = ivar+1
          DO ivar = i, maxvars
	  IF ( ds_var_setnum(ivar) .EQ. dset ) THEN
	      IF (ds_grid_number(ivar).NE.grid) THEN
	         grid = ds_grid_number(ivar)
                 EXIT
	      ENDIF
          ENDIF
          ENDDO
       ENDIF

       nfeatures = TM_DSG_NFEATURES( grid )	

* Set the line-memory to store the mask data

* ... A mask was already defined. Wipe that one out and we'll redefine it.

	IF (dsg_msk_lm(dset) .NE. unspecified_int4)  THEN
	   CALL FREE_LINE_DYNMEM( dsg_msk_lm(dset) )
	   CALL TM_DEALLO_DYN_LINE( dsg_msk_lm(dset) )
	   dsg_msk_lm(dset) = unspecified_int4

* ... wipe memory clear of stored variables - this could change all definitions
	   DO i = 1,max_mr_avail
	      IF ( mr_protected( i ) .NE. mr_deleted ) 
     .               CALL DELETE_VARIABLE( i )
	   ENDDO

	ENDIF

* Load the variable. Reset the arg pointers to get the mask-var
* Use arg_start, arg_end to point to the name of the variable.
* We may be coming from a SET DATA/FMASK command or a LET command
* redefining a variable whose name is already associated with a 
* feature-mask for this dataset.

	loc = qual_given(slash_set_data_fmask)
	IF (loc.GT.0) THEN
	   num_args = 1	 
	   arg_end(1) = qual_end(loc)
	   arg_start(1) = qual_end(loc) - TM_LENSTR1(fmask_var) + 1
	ELSE
	   arg_end(1) = arg_start(1) + TM_LENSTR1(fmask_var) - 1
	ENDIF

	CALL GET_CMND_DATA ( cx_last, ptype_float, status )
	IF (status .NE. ferr_ok .OR. num_uvars_in_cmnd .GT. 1) GOTO 5400

* ... make sure it's a line (not a point,plane,etc.) and that the M extent
*     is the feature-length

	cx = is_cx(isp)
	CALL GET_CX_DIMS( cx, ndim, dim )
	IF ( ndim .GT. 1 ) GOTO 5400
	idim = dim(1)
	IF (nfeatures .NE. CX_DIM_LEN( idim, cx ) ) THEN
*  the mask doesn't match mask on this dset. Issue a note
	   IF (reload) GOTO 5500  
	   GOTO 5400
	ENDIF

* store it here,

	CALL TM_ALLO_TMP_LINE(dsg_msk_lm(dset), status)
	CALL GET_LINE_DYNMEM (nfeatures, dsg_msk_lm(dset), status)
	CALL TM_USE_LINE(dsg_msk_lm(dset))

	CALL TM_NEW_LINE_NAME ('FEATURE_MASK', buff ) 
	line_name(dsg_msk_lm(dset)) = buff
	line_direction( dsg_msk_lm(dset) ) = 'EE'

	mr1 = is_mr( num_uvars_in_cmnd )
	cx = is_cx( num_uvars_in_cmnd )

	CALL EXTRACT_LINE (cx,
     .			memry(mr1)%ptr,
     .			mr1,
     .			linemem(dsg_msk_lm(dset))%ptr,
     .			idir,
     .			ndim,
     .			npts,
     .			status	)

	IF (npts.NE.nfeatures .OR. status.NE.ferr_ok) GOTO 5000

* Put the default bad-flag into the mask data
	CALL TM_SWITCH_BAD ( cx_bad_data (cx), bad_val4, linemem(dsg_msk_lm(dset))%ptr, npts )

* Set a global attribute with the mask title

	buff = '__feature_mask_'
	attlen = TM_LENSTR1(buff)
	mask_title = FULL_VAR_TITLE( cx, .FALSE., attlen )

	varid = 0
	CALL CD_GET_VAR_ATT_ID (dset, varid, buff, attid, status) 
	IF (attid .GT. 0) THEN
	   CALL CD_GET_VAR_ATT_INFO (dset, varid, attid, 
     .                buff, attype, attlen, attoutflag, status )
	   attoutflag = 0
	   CALL CD_REPLACE_ATTR (dset, varid, buff, attype, attlen, 
     .              mask_title, dummy, status)
	ELSE
	   attype = NCCHAR
	   attoutflag = 0
	   CALL CD_PUT_NEW_ATTR (dset, varid, buff, attype, 
     .                 attlen, attoutflag, mask_title, dummy, status )
	ENDIF
	IF (status .NE. ferr_ok) GOTO 5000

* And another with the feature-mask name	

	buff = '__feature_mask_var'
	attlen = TM_LENSTR1(buff)

	varid = 0
	CALL CD_GET_VAR_ATT_ID (dset, varid, buff, attid, status) 
	IF (attid .GT. 0) THEN
	   CALL CD_GET_VAR_ATT_INFO (dset, varid, attid, 
     .                buff, attype, attlen, attoutflag, status )
	   attoutflag = 0
	   CALL CD_REPLACE_ATTR (dset, varid, buff, attype, attlen, 
     .              fmask_var, dummy, status)
	ELSE
	   attype = NCCHAR
	   attoutflag = 0
	   CALL CD_PUT_NEW_ATTR (dset, varid, buff, attype, 
     .                 attlen, attoutflag, fmask_var, dummy, status )
	ENDIF
	IF (status .NE. ferr_ok) GOTO 5000

	status = ferr_ok

 5000	RETURN

 5100	CALL ERRMSG( ferr_unknown_arg, status, 'argument required /FMASK=?', *5000)
 5200	CALL ERRMSG( ferr_invalid_command, status,
     .			'/FMASK=name is not an acceptable name', *5000)
 5300	CALL ERRMSG (ferr_invalid_command, status,
     .'/FMASK is set only for Discrete Sampling Geometries datasets',
     .               						*5000 )
 5400	buff = TM_FMT(DBLE(nfeatures), 0, 12, i) 
	CALL ERRMSG (ferr_invalid_command, status,
     .'Mask variable must be 1-dimensional, with length num-features = '//
     .   buff(:i),						*5000 )
     
 5500	buff = TM_FMT(DBLE(dset), 0, 12, i)
	slen = TM_LENSTR1(fmask_var)
	CALL WARN ('Feature-mask on dataset '//buff(:i)//
     .   ' canceled. New definition of '//fmask_var(:slen)//
     .   ' does not match nfeatures of set '//buff(:i) )
	GOTO 5000

	END
