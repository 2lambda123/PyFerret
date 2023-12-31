   	SUBROUTINE DO_QUERY( chout, fmt, query, tag, status ) 

*
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
*
* respond to the specific QUERY command

* programmer - steve hankin
* NOAA/PMEL, Seattle, WA - Tropical Modeling and Analysis Program

* for FERRET V300: 6/16/93
* V314: 8/19/94 *kob* IBM port - Modified format statements, added 
*				 ifdef MANDATORY_FORMAT_WIDTHS for I format
*			         descriptor 
* V320: 12/29/94 - use SPLIT_LIST to redirect tty output for GUI
* V420: 11/95 - bug in QUERY GRID if grid unknown:  LET A = I; QUERY GRID 1 A
* Linux Port 1/97 *kob* - Added a preprocessor include for tmap_dset.parm
*			  because it needed a preprocessor.
* kob*   9/97 - replaced call to ISUBSCRIPT w/ call to ISUBSCR_CX in order
*               to have access to context information - needed for negative
*               time step processing
* V5.3  1/01 *acm* cal_id for alternative calendar types: call to TM_DATE_OK
* v540 *acm* 10/01 increase length of ds_name
* v540 *acm* 10/01  remove VMS include statements
* v604 *acm* 7/07 increase length of ds_name
*      *acm* 3/12 Add E and F dimensions (use nferdims in tmap_dims.parm)
* V761 *acm* 7/20 Issue 1981:in ISUBSCR_CX distinguish rounding for single 
*                 point vs range of points
* 10/2022 *acm* Code cleanup: remove references to obsolete gui interface
* 11/2022 *acm* Code cleanup: Modernize DO-loop syntax. 
* 12/2022 *acm* Code cleanup: Replace the computed GOTO statement with IF blocks
*                 GOTO ( 100, 200, 300, ...

        IMPLICIT NONE
        include 'tmap_dims.parm'
#        include "tmap_dset.parm"
        include 'xdset_info.cmn_text'
        external xdset_info_data
        include 'xtm_grid.cmn_text'
        external xgt_grid_data
	include	'ferret.parm'
	include 'errmsg.parm'
	include 'gfdl_vms.parm'
	include 'xprog_state.cmn'
	include 'xcommand.cmn'
	include 'xtext_info.cmn'
	include 'xcontext.cmn'
	include 'xalgebra.cmn'
	include 'xrisc.cmn'          ! 12/94 SPLIT_LIST buffer

* calling argument declarations
	INTEGER chout, query, status
	CHARACTER*(*) fmt, tag

* local variable declarations:
	LOGICAL TM_DATE_OK, done, formatted, ww_given
	INTEGER	TM_LENSTR1, FIND_DSET_NUMBER, IGRID, TM_GET_LINENUM,
     .		STR_CASE_BLIND_COMPARE, TM_GET_GRIDNUM, ISUBSCR_CX,
     .		i, n, dim, dset, cat, var, axis, grid, reg, modulo,
     .		calendar, dummy, istat, slen, ds_var_len
	INTEGER TM_GET_CALENDAR_ID, cal_id


	REAL*8	TWORLD, TM_WORLD, lo, hi, delta
	CHARACTER TM_FMT*48, buff1*32, buff2*32

* initialize
	status = ferr_ok
	risc_buff = ' '
	IF ( query .GT. max_queries ) THEN
	   status = ferr_invalid_command
	   RETURN
	ENDIF

* respond to the appropriate query

* STATUS
* error status of last command

      IF (queries(query) .EQ. 'STATUS') THEN

	WRITE (risc_buff, fmt ) 0, queries(query), tag
	CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	GOTO 4999

      ENDIF

* MESSAGE
* messages from last command

      IF (queries(query) .EQ. 'MESSAGE') THEN

	GOTO 4999

      ENDIF

* DSET
* list of open data sets
      IF (queries(query) .EQ. 'DSET') THEN

*   ... count data sets
 	   n = 0
	   DO i = 1,  maxdsets
	      IF ( ds_name(i) .NE. char_init2048) n = n + 1
 	   ENDDO
*   ... list 'em
	   WRITE (risc_buff, fmt ) n, queries(query), tag
	   CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	   DO i = 1,  maxdsets
	      IF (ds_name(i).NE.char_init2048) THEN
	         WRITE (risc_buff,'(A)')
     .			ds_name(i)(1:TM_LENSTR1(ds_name(i)))
	         CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	         risc_buff = ' '
	      ENDIF
 	   ENDDO
	GOTO 4999

      ENDIF

* VARIABLE dset
* list variables in data set

      IF (queries(query) .EQ. 'VARIABLE') THEN

	IF ( num_args .NE. 2 ) GOTO 5040
* ... identify the data set
	dset = FIND_DSET_NUMBER( cmnd_buff(arg_start(2):arg_end(2)) )
	IF ( dset .EQ. unspecified_int4 ) GOTO 5910
* ... pass1 to count variables in data set
	n = 0
	DO var = 1, maxvars
	   IF ( ds_var_setnum(var) .NE. dset ) CYCLE
	   IF ( ds_var_code(var) .EQ. '-' ) CYCLE
	   n = n + 1
 	ENDDO
	WRITE (risc_buff, fmt ) n, queries(query), tag
	CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
* ... pass 2 to output variable names
	DO var = 1, maxvars
           IF ( ds_var_setnum(var) .NE. dset ) CYCLE
           IF ( ds_var_code(var) .EQ. '-' ) CYCLE
*           WRITE (risc_buff,'(A)')
*     .			ds_var_code(var)(1:TM_LENSTR1(ds_var_code(var)))
           CALL string_array_get_strlen1(ds_var_code_head, var, ds_var_len)
           WRITE (risc_buff,'(A)')
     .			ds_var_code(var)(1:ds_var_len)

	   CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	   risc_buff = ' '
 	ENDDO
	GOTO 4999

      ENDIF

* GRID dset variable
* n-axis description of grid for given variable
* 5 records back: grid name and axis names with flags

      IF (queries(query) .EQ. 'GRID') THEN

	IF ( num_args .NE. 3 ) GOTO 5050
* ... identify the data set, then variable, then its grid
	dset = FIND_DSET_NUMBER( cmnd_buff(arg_start(2):arg_end(2)) )
	IF ( dset .EQ. unspecified_int4 ) GOTO 5910
	CALL FIND_VAR_NAME( dset, cmnd_buff(arg_start(3):arg_end(3)),
     .	                    cat, var )
	IF ( var .EQ. munknown_var_name ) GOTO 5920
	grid = IGRID ( dset, cat, var, status )
	IF ( status .NE. ferr_ok ) GOTO 5000
	IF ( grid .EQ. unspecified_int4 ) GOTO 5925
	WRITE (risc_buff, fmt ) 5, queries(query), tag
	CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	risc_buff = ' '
	WRITE (risc_buff, '(A)')
     .			grid_name(grid)(1:TM_LENSTR1(grid_name(grid)))
	CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
* ... return the names of the axes and an inner/outer flag
	DO dim = 1, nferdims
	   axis = grid_line( dim, grid )
	   IF ( axis .EQ. munknown ) THEN
	      CALL SPLIT_LIST(pttmode_explct, chout, 'UNKNOWN O', 9)
	   ELSEIF( axis .EQ. mnormal ) THEN
	      CALL SPLIT_LIST(pttmode_explct, chout, 'NORMAL O', 8)
	   ELSE
	      CALL SPLIT_LIST(pttmode_explct, chout,
     .					line_name(axis)//' O', 0)
	   ENDIF
 	ENDDO
	GOTO 4999

      ENDIF

* AXIS  axname
* axis description: "npts reg modulo calendar lo_value delta unit_code units"

      IF (queries(query) .EQ. 'AXIS') THEN
	IF ( num_args .NE. 2 ) GOTO 5060
	axis = TM_GET_LINENUM( cmnd_buff(arg_start(2):arg_end(2)) )
 610	IF ( axis .EQ. unspecified_int4 ) GOTO 5930
	GOTO 5000

      ENDIF  ! not available yet

* COORDS axis
* coordinates of (irregular) axis

      IF (queries(query) .EQ. 'AXIS') THEN

	GOTO 5000

      ENDIF

* TRANS  *************
* list of transforms

      IF (queries(query) .EQ. 'TRANS') THEN

* ... count the number of transforms
	n = 0
	DO i = 4, num_transforms
	   IF ( i .EQ. 11 .OR. i .EQ. 12 
     .	   .OR. alg_trans_num( i ) .EQ. unspecified_int4 ) CYCLE
	   n = n + 1
	ENDDO
	WRITE (risc_buff, fmt ) n, queries(query), tag
	CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
* ... each transform: XXX dflt_arg_or_* Compress_or_Span  "title"
	DO i = 4, num_transforms
	   IF ( i .EQ. 11 .OR. i .EQ. 12 
     .	   .OR. alg_trans_num( i ) .EQ. unspecified_int4 ) CYCLE
	   risc_buff = ' '
	   IF ( alg_trans_has_arg(i) ) THEN
	      buff1 = TM_FMT( alg_trans_dflt_arg(i), 3, 5, slen ) 
	      IF ( .NOT.alg_trans_its_int(i)
     .	     .AND. INDEX(buff1,'.') .EQ. 0  ) THEN
	         buff1 = buff1(:slen)//".0"
	         slen = slen + 2
	      ENDIF	
	   ELSE
	      buff1 = '*'
	   ENDIF
	   IF ( alg_trans_num(i) .GT. trans_compress_code ) THEN
	      buff2 = 'C'   ! compresses range of data
	   ELSE
	      buff2 = 'S'   ! span of data preserved
	   ENDIF
	   WRITE (risc_buff, 3800 )
     .			alg_trans_text(i) ,
     .			buff1(:TM_LENSTR1(buff1)),
     .			buff2,
     .			alg_trans_title(i)(:TM_LENSTR1(alg_trans_title(i)))
	   CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	ENDDO
 3800	FORMAT(A3,1X,A,1X,A1,' "',A,'"')
	GOTO 4999

      ENDIF

* TRNARG (trans) ****will not be used!!*************
* argument description for given transform

      IF (queries(query) .EQ. 'TRNARG') THEN

	GOTO 5000

      ENDIF

* LVARS  *************
* list of LET-defined variables

      IF (queries(query) .EQ. 'LVARS') THEN

	GOTO 5000

      ENDIF

* DVARS (dset) *************
* list of LET-defined variables defined for data set

      IF (queries(query) .EQ. 'DVARS') THEN

	GOTO 5000

      ENDIF

* VBACKGROUND  (dset variable) *************
* background info about this variable

      IF (queries(query) .EQ. 'VBACKGROUND') THEN

	GOTO 5000

      ENDIF

* DBACKGROUND (dset) *************
* background info about this data set

      IF (queries(query) .EQ. 'DBACKGROUND') THEN

	GOTO 5000

      ENDIF
* WINDOWS  *************
* list of open windows

      IF (queries(query) .EQ. 'WINDOWS') THEN

	GOTO 5000

      ENDIF
* WCURRENT  *************
* name of current window

      IF (queries(query) .EQ. 'WCURRENT') THEN

	GOTO 5000

      ENDIF
* VPORTS  *************
* list of defined viewports

      IF (queries(query) .EQ. 'VPORTS') THEN

	GOTO 5000

      ENDIF
* VCURRENT  *************
* name of current viewport

      IF (queries(query) .EQ. 'VCURRENT') THEN

	GOTO 5000

      ENDIF
* VIEW  (vport) *************
* definition of indicated viewport

      IF (queries(query) .EQ. 'VIEW') THEN

	GOTO 5000

      ENDIF

* DCURRENT
* name of current data set

      IF (queries(query) .EQ. 'DCURRENT') THEN

	done = .FALSE.
	DO dset = 1, maxdsets
           IF ( ds_name(dset) .EQ. char_init2048) CYCLE
	   IF ( dset .EQ. cx_data_set(cx_last) ) THEN
	      WRITE (risc_buff, fmt ) 1, queries(query), tag
	      CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	      risc_buff = ' '
	      WRITE (risc_buff, '(A)' )
     .			ds_name(dset)(1:TM_LENSTR1(ds_name(dset)))
	      CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	      risc_buff = ' '
	      done = .TRUE.
	   ENDIF
	ENDDO
	IF ( .NOT.done ) THEN
	   WRITE (risc_buff, fmt ) 0, queries(query), tag
	   CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	ENDIF
	GOTO 4999

      ENDIF

* SPAN  dset variable
* return nferdims+1 records:
* 1 - the grid name for this variable
* 2-n the subssript, world coords, and world text limits along each axis

      IF (queries(query) .EQ. 'SPAN') THEN

	IF ( num_args .NE. 3 ) GOTO 5050
* ... identify the data set, then variable, then its grid
	dset = FIND_DSET_NUMBER( cmnd_buff(arg_start(2):arg_end(2)) )
	IF ( dset .EQ. unspecified_int4 ) GOTO 5910
	CALL FIND_VAR_NAME( dset, cmnd_buff(arg_start(3):arg_end(3)),
     .	                    cat, var )
	IF ( var .EQ. munknown_var_name ) GOTO 5920
	IF ( cat .NE. cat_file_var ) GOTO 5000
	grid = IGRID ( dset, cat, var, status )
        IF ( status .NE. ferr_ok ) GOTO 5000
	WRITE (risc_buff, fmt ) 5, queries(query), tag
	CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	risc_buff = ' '
* ... return the grid name
	WRITE (risc_buff, '(A)' ) grid_name(grid)
     .	                          (1:TM_LENSTR1(grid_name(grid)))
	CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	risc_buff = ' '
* ... return the subscript span along each axis
	DO dim = 1, nferdims
	   axis = grid_line( dim, grid )
	   IF ( axis .EQ. munknown
     .     .OR. axis .EQ. mnormal ) THEN
	      WRITE (risc_buff,3200) -111, -111, 0.D0, 0.D0, '*', '*'
	      CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	   ELSE
	      lo = TM_WORLD(ds_grid_start(dim,var), grid, dim, box_middle)
	      hi = TM_WORLD(ds_grid_end  (dim,var), grid, dim, box_middle)
	      CALL TRANSLATE_TO_WORLD( lo,dim,grid,ax_dec_pt(dim),buff1 )
	      CALL TRANSLATE_TO_WORLD( hi,dim,grid,ax_dec_pt(dim),buff2 )
	      CALL CLEAN_COORD_STRING(dim, buff1(1:TM_LENSTR1(buff1)))
	      CALL CLEAN_COORD_STRING(dim, buff2(1:TM_LENSTR1(buff2)))
              WRITE (risc_buff,3200) ds_grid_start(dim,var),
     .				 ds_grid_end  (dim,var),
     .				 lo, hi,
     .				 buff1(1:TM_LENSTR1(buff1)),
     .				 buff2(1:TM_LENSTR1(buff2))
	      CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	   ENDIF
	   risc_buff = ' '
	ENDDO
 3200	FORMAT(2I6,2(1PG14.5),2(1x,A))
	GOTO 4999

      ENDIF

* GAXIS grid orientation    (like AXIS but different args)

      IF (queries(query) .EQ. 'GAXIS') THEN

	IF ( num_args .NE. 3 ) GOTO 5210
	grid = TM_GET_GRIDNUM( cmnd_buff(arg_start(2):arg_end(2)) )
	IF ( grid .EQ. unspecified_int4 ) GOTO 5930
	DO dim = 1, nferdims
	  istat = STR_CASE_BLIND_COMPARE( ww_dim_name(dim),
     .				cmnd_buff(arg_start(3):arg_end(3)) )
	  IF ( istat .EQ. vms_str_success ) GOTO 2120
	ENDDO
	GOTO 5210
 2120	axis = grid_line( dim, grid )
	WRITE (risc_buff, fmt ) 1, queries(query), tag
	CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	risc_buff = ' '
	IF ( axis .EQ. munknown
     .  .OR. axis .EQ. mnormal ) THEN
	   n = 0
	   lo = 0.D0
	   hi = 0.D0
	   delta = 0.D0
	   reg = 0
	   modulo = 0
	   calendar = 0
	ELSE
	   n = line_dim(axis)
	   lo = TWORLD(1, grid, dim, box_middle)
	   hi = TWORLD(n, grid, dim, box_middle)
	   delta = (hi-lo)/(n-1)
	   IF ( line_regular(axis) ) THEN
	      reg = 1
	   ELSE
	      reg = 0
	   ENDIF
	   IF ( line_modulo( axis ) ) THEN	
	      modulo = 1
	   ELSE
	      modulo = 0
	   ENDIF
           cal_id = TM_GET_CALENDAR_ID (line_cal_name(axis) )
	   IF ( TM_DATE_OK( line_t0(axis), cal_id)  ) THEN
	      calendar = 1
	   ELSE
	      calendar = 0
	   ENDIF
	ENDIF
	WRITE (risc_buff, 3060 ) n, reg, modulo, calendar,
     .			      lo, delta,
     .			      line_unit_code(axis),
     .			      line_units(axis)(1:TM_LENSTR1(line_units(axis)))
	CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
 3060	FORMAT(I6,3I2,2(1PG14.5),I4,2X,A)
	GOTO 4999

      ENDIF

* GCOORDS grid orientation    (like COORDS but different args)

      IF (queries(query) .EQ. 'GCOORDS') THEN

	IF ( num_args .NE. 3 ) GOTO 5220
	grid = TM_GET_GRIDNUM( cmnd_buff(arg_start(2):arg_end(2)) )
	IF ( grid .EQ. unspecified_int4 ) GOTO 5930
	DO dim = 1, nferdims
	  istat = STR_CASE_BLIND_COMPARE( ww_dim_name(dim),
     .					 cmnd_buff(arg_start(3):arg_end(3)) )
	  IF ( istat .EQ. vms_str_success ) GOTO 2220
	ENDDO
	GOTO 5220
 2220	CALL GRID_SUBSCRIPT_EXTREMES( dummy, n, grid, dim )
	WRITE (risc_buff, fmt ) n+1, queries(query), tag
	CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	risc_buff = ' '
* for each record write the coordinate and the lower box bound
	DO i = 1, n
	   WRITE (risc_buff, '(2(1PG14.5))' ) TWORLD(i, grid, dim, box_middle),
     .				   TWORLD(i, grid, dim, box_lo_lim)
	   CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	   risc_buff = ' '
	ENDDO
	WRITE (risc_buff, '(2(1PG14.5))' ) -999.D0, TWORLD(n, grid, 
     .                             dim, box_hi_lim)
	CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	risc_buff = ' '
	GOTO 4999

      ENDIF

* SSPOINT grid orientation delta_index index
* given an index or a world coordinate return an index value
* that is offset by an index change delta_index
* The "orientation" parameter should match the "coordinate" wrt index or ww

      IF (queries(query) .EQ. 'SSPOINT') THEN

	IF ( num_args .NE. 5 ) GOTO 5230
	grid = TM_GET_GRIDNUM( cmnd_buff(arg_start(2):arg_end(2)) )
	IF ( grid .EQ. unspecified_int4 ) GOTO 5930
	DO dim = 1, nferdims ! match orientation against X,Y,Z,T,E,F
	  istat = STR_CASE_BLIND_COMPARE( ww_dim_name(dim),
     .				cmnd_buff(arg_start(3):arg_end(3)) )
	  IF ( istat .EQ. vms_str_success ) THEN
	     ww_given = .TRUE.
	     GOTO 2320
	  ENDIF
	ENDDO
	DO dim = 1, nferdims ! match orientation against I,J,K,L,M,N
	  istat = STR_CASE_BLIND_COMPARE( ss_dim_name(dim),
     .				cmnd_buff(arg_start(3):arg_end(3)) )
	  IF ( istat .EQ. vms_str_success ) THEN
	     ww_given = .FALSE.
             GOTO 2320
          ENDIF
	ENDDO
	GOTO 5230
* no fixed field width works but "*" may cause problems too
* such as "Y" being an unacceptable integer
 2320	READ (cmnd_buff(arg_start(4):arg_end(4)), *,ERR=5304) n ! delta
	IF ( ww_given ) THEN
	   axis = grid_line(t_dim, grid)
	   cal_id = TM_GET_CALENDAR_ID ( line_cal_name(axis) )
 	   CALL TRANSLATE_LIMIT( cmnd_buff(arg_start(5):arg_end(5)),
     .		           dim, .FALSE., lo, formatted, cal_id, status )
	   IF ( status .NE. ferr_ok ) GOTO 5315
	   i = ISUBSCR_CX( lo, grid, dim, cx_last, round_sngl )
	ELSE
* no fixed field width works but "*" may cause problems too
* such as "Y" being an unacceptable integer
	   READ (cmnd_buff(arg_start(5):arg_end(5)), *,ERR=5305) i
	ENDIF
	i = MAX( i+n, 1 )
	WRITE (risc_buff, fmt ) 1, queries(query), tag
	CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	risc_buff = ' '
	WRITE (risc_buff, '(I6)' ) i
	CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	GOTO 4999

      ENDIF

* WWPOINT grid orientation delta_index coordinate
* given a index or a world coordinate return both a formatted string
* for a world coordinate and a numerical value that are offset by an
* index change delta_index
* The "orientation" parameter should match the "coordinate" wrt index or ww

      IF (queries(query) .EQ. 'WWPOINT') THEN

	IF ( num_args .NE. 5 ) GOTO 5240
	grid = TM_GET_GRIDNUM( cmnd_buff(arg_start(2):arg_end(2)) )
	IF ( grid .EQ. unspecified_int4 ) GOTO 5930
	DO dim = 1, nferdims   ! match orientation against X,Y,Z,T
	  istat = STR_CASE_BLIND_COMPARE( ww_dim_name(dim),
     .				cmnd_buff(arg_start(3):arg_end(3)) )
	  IF ( istat .EQ. vms_str_success ) THEN
	      ww_given = .TRUE.
	      GOTO 2420
	  ENDIF
	ENDDO
	DO dim = 1, nferdims     ! match orientation against I,J,K,L,M,N
	  istat = STR_CASE_BLIND_COMPARE( ss_dim_name(dim),
     .				cmnd_buff(arg_start(3):arg_end(3)) )
	  IF ( istat .EQ. vms_str_success ) THEN
	      ww_given = .FALSE.
	      GOTO 2420
	  ENDIF
	ENDDO
	GOTO 5240

 2420	READ (cmnd_buff(arg_start(4):arg_end(4)), *,ERR=5304) n ! delta
	IF ( ww_given ) THEN
	   axis = grid_line(t_dim, grid)
	   cal_id = TM_GET_CALENDAR_ID ( line_cal_name(axis) )
 	   CALL TRANSLATE_LIMIT( cmnd_buff(arg_start(5):arg_end(5)),
     .		           dim, .FALSE., lo, formatted, cal_id, status )
	   IF ( status .NE. ferr_ok ) GOTO 5315
	   i = ISUBSCR_CX( lo, grid, dim, cx_last, round_sngl )
	ELSE
	   READ (cmnd_buff(arg_start(5):arg_end(5)), *,ERR=5305) i
	ENDIF
	i = MAX(i+n, 1)
	lo = TM_WORLD( i, grid, dim, box_middle )
	CALL TRANSLATE_TO_WORLD( lo,dim,grid,ax_dec_pt(dim),buff1 )
	CALL CLEAN_COORD_STRING( dim, buff1(1:TM_LENSTR1(buff1)) )
	WRITE (risc_buff, fmt ) 1, queries(query), tag
	CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	risc_buff = ' '
	WRITE (risc_buff, '(A,1X,1PG14.5)' ) buff1( 1:TM_LENSTR1(buff1) ), lo
	CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
	GOTO 4999

      ENDIF

* successful completion
 4999	RETURN

* error exits
 5000	WRITE (risc_buff, fmt ) 0, "*ERR", tag
	CALL SPLIT_LIST(pttmode_explct, chout, risc_buff, 0)
 	RETURN
 5040	CALL ERRMSG( ferr_invalid_command, status,
     .		     "QUERY VARIABLES needs data set as argument", *5000)
 5050	CALL ERRMSG( ferr_invalid_command, status,
     .		     "this QUERY needs data set and variable as argument",
     .		     *5000)
 5060	CALL ERRMSG( ferr_invalid_command, status,
     .		     'QUERY AXIS needs axis name as argument', *5000)
 5210	CALL ERRMSG( ferr_invalid_command, status,
     .		     'usage: QUERY GAXIS grid_name X_Y_Z_T_EorF', *5000)
 5220	CALL ERRMSG( ferr_invalid_command, status,
     .		     'usage: QUERY GCOORDS grid_name X_Y_Z_T_EorF', *5000)
 5230	CALL ERRMSG( ferr_invalid_command, status,
     .		     'usage: QUERY SSPOINT grid_name X_Y_Z_T_EorF delta index'
     .	             , *5000)
 5240	CALL ERRMSG( ferr_invalid_command, status,
     .		     'usage: QUERY WWPOINT grid_name X_Y_Z_T_EorF delta coord'
     .	             , *5000)
 5304	CALL ERRMSG( ferr_erreq, status,
     .		     cmnd_buff(arg_start(4):arg_end(4)), *5000 )
 5305	CALL ERRMSG( ferr_erreq, status,
     .		     cmnd_buff(arg_start(5):arg_end(5)), *5000 )
 5315	CALL ERRMSG( ferr_syntax, status,
     .		     cmnd_buff(arg_start(5):arg_end(5)), *5000 )
 5910	CALL ERRMSG( ferr_unknown_data_set, status,
     .		     cmnd_buff(arg_start(2):arg_end(2)), *5000)
 5920	CALL ERRMSG( ferr_unknown_variable, status,
     .		     cmnd_buff(arg_start(3):arg_end(3)), *5000)
 5925	CALL ERRMSG( ferr_unknown_grid, status,
     .		     cmnd_buff(arg_start(3):arg_end(3)), *5000)
 5930	CALL ERRMSG( ferr_unknown_arg, status,
     .		     cmnd_buff(arg_start(2):arg_end(2)), *5000)
	END

