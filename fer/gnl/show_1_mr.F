	SUBROUTINE SHOW_1_MR( mr, status )

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
* display a description of variable MR

* programmer - steve hankin
* NOAA/PMEL, Seattle, WA - Tropical Modeling and Analysis Program
* written for VAX computer under VMS operating system
*
* V200:  6/12/89 - extracted from SHOW_MEM_VARS
*	 2/27/90 - changed VAR_CODE to *8
*	 3/30/90 - allow 6 digit subscripts
* Risc/UNIX port 4/91 *sh* cannot use functions in WRITE list
* V230:  11/4/92 - expanded variable name length
* V420: 11/95 - indicate un-cached variables
* V500 *kob* 3/99- up VAR_CODE and vname decl's to 64 chars
* V552 *acm* 4/03- up VAR_CODE and vname decl's from 64 to 128 chars
* V63  *acm* 10/09 Changes for gfortran build
* V650  Write to show_lun which may be set to a file unit number rather than std out
*       *acm* 3/12 cleanup ifdefs and unnecessary include files
*       *acm* 3/12 6D Ferret (common uses nferdims in tmap_dims.parm)
* V6.8  acm  6/12 Implement mode_6d_lab, set by default. If canceled, the 
*                 various SHOW commnands will not list info for E,F dimensions.
* V698  1/16 *sh* bug fix: the initial 6D trap was removing all possibility of
*		  showing 5D or 6D variables
* V7    acm  6/16 Ticket 2437: Deprecating mode_6d_lab. 
*		  Adjust output to 6-D or 4-D grids
* V702 *sh* 2/17  Reveal dynamic memory instead of block memory
*		  Increase field widths for long axes
* V720 *sh* 4/17  customize output to show a DSG mr
* V751 *acm* 6/19 Issue 1903: memory set and reported in terms of bytes

        IMPLICIT NONE
	include	'tmap_dims.parm'
	include	'xtm_grid.cmn_text'
	external xgt_grid_data
	include	'xdset_info.cmn_text'
	external xdset_info_data
	include	'ferret.parm'
	include 'xprog_state.cmn'
	include 'xvariables.cmn'
	include 'xtext_info.cmn'
	include 'xalgebra.cmn'
	include 'xrisc.cmn'          ! 12/94 SPLIT_LIST buffer
	include 'errmsg.parm'

* calling argument declarations:
	INTEGER	mr, status

* V500 *kob* 3/99- up VAR_CODE and vname decl's to 64 chars
* V552 *acm* 4/03- up VAR_CODE and vname decl's from 64 to 128 chars
* local variable declarations:

	LOGICAL	ITS_UN_CACHED, FOUR_D_GRID, TM_ITSA_DSG
	INTEGER TM_LENSTR1, MR_DIM_LEN, TM_DSG_DSET_FROM_GRID,
     .		idim, ndim, mrlen, mrsizelen, dset, listdims, grid
	INTEGER*8 mem
	CHARACTER VAR_CODE*128, ALG_TRANS_CODE*3, ARG_STRING*12, TM_FMT*48,
     .		  protection*4, shape*4, dname*14, gname*12, vname*128,
     .		  ww_lo(nferdims)*20, ww_hi(nferdims)*20, 
     .		  ss_lo(nferdims)*9, ss_hi(nferdims)*9,
     .		  mrbuf*4, mrsizebuf*16,
     .		  cache_stat*8, byte_unit*2

* DSGTBD Use dsg_xlate_grid wherever TRANSLATE_TO_WORLD is called in Ferret
* Get the grid to be use for formatting output
	grid = mr_grid(mr)
	IF (TM_ITSA_DSG(grid)) THEN
	   dset = TM_DSG_DSET_FROM_GRID(grid)
	   grid = dsg_xlate_grid(dset)
	ENDIF

* If mode 6d_lab has been canceled, we will list only the first 4 dimensions.
	listdims = nferdims
	IF ( .NOT. mode_6d_lab) listdims = 4
	status = ferr_ok
	IF ( FOUR_D_GRID(mr_grid(mr)) ) listdims = 4

* get shape
	shape = ' '
	ndim = 0

* If mode 6d_lab has been canceled, but the variable has dimension 5 or 6,
* that is an error
	DO 150 idim = listdims+1, nferdims
	   IF ( MR_DIM_LEN( idim, mr ) .GT. 1 ) GOTO 5100
 150	CONTINUE

	DO 170 idim = 1, listdims
	   IF ( MR_DIM_LEN( idim, mr ) .GT. 1 ) THEN
	      ndim = ndim + 1
	      shape(ndim:ndim) = ww_dim_name(idim)
	   ENDIF
 170	CONTINUE

* get formatted world positions
	DO 180 idim = 1, listdims
	   IF ( mr_lo_ww(idim,mr) .EQ. unspecified_val8 ) THEN
	      ww_lo(idim) = ' ...'
	      ww_hi(idim) = ' ...'
	   ELSE
	      CALL TRANSLATE_TO_WORLD
     .	           ( mr_lo_ww(idim,mr),idim,grid,
     .		     ax_dec_pt(idim), ww_lo(idim) )
	      CALL TRANSLATE_TO_WORLD
     .	           ( mr_hi_ww(idim,mr),idim,grid,
     .		     ax_dec_pt(idim), ww_hi(idim) )
	   ENDIF
	   IF ( mr_lo_ss(mr,idim) .EQ. unspecified_int4 ) THEN
	      ss_lo(idim) = '     ...'
	      ss_hi(idim) = '     ...'
	   ELSEIF ( mr_hi_ss(mr,idim) .GT. 9999 ) THEN
! not needed any longer ...
	      WRITE ( ss_lo(idim), '(I9)' ) mr_lo_ss(mr,idim)
	      WRITE ( ss_hi(idim), '(I9)' ) mr_hi_ss(mr,idim)
	   ELSE
	      WRITE ( ss_lo(idim), '(I9)' ) mr_lo_ss(mr,idim)
	      WRITE ( ss_hi(idim), '(I9)' ) mr_hi_ss(mr,idim)
	   ENDIF
 180	CONTINUE

* write a line summary of this memory variable
	IF ( mr_protected(mr) .EQ. mr_not_protected ) THEN
	   protection = ' '
	ELSEIF ( mr_protected(mr) .EQ. mr_perm_protected ) THEN
	   protection = 'PERM'
	ELSE
	   protection = '*ERR'
	ENDIF
	IF ( ITS_UN_CACHED(mr) ) THEN
	   cache_stat = 'UNCACHED'
	ELSE
	   cache_stat = ' '
	ENDIF
	vname = VAR_CODE( mr_category(mr),mr_variable(mr) )
	dset = mr_data_set(mr)
	IF ( dset .EQ. pdset_irrelevant ) THEN
	   dname = ' '
	ELSE
	   dname = 'D='//ds_name(dset)
	ENDIF
	IF ( mr_unstand_grid(mr) ) THEN
	   gname = grid_name(mr_grid(mr))
	   gname = ',G='//gname(:TM_LENSTR1(gname))//']'
	ELSE
	   gname = ']'
	ENDIF
	mrbuf      = TM_FMT( FLOAT(mr), 0, 4, mrlen )
	mrsizebuf  = TM_FMT( FLOAT(mr_size(mr)), 0, 8, mrsizelen )

	mem = mr_size(mr)

	CALL MEM_TO_BYTES (mem, mrsizebuf, mrsizelen, byte_unit)
	IF (mem .EQ. 0) byte_unit = ' '

	risc_buff  = ' '
	WRITE ( risc_buff, 3010 )	vname(:TM_LENSTR1(vname)),
     .					dname(:TM_LENSTR1(dname)),
     .					gname(:TM_LENSTR1(gname)),
     .					shape,
     .					mrbuf(:mrlen),
     .					mrsizebuf(:mrsizelen),
     .					byte_unit,
     .					protection,
     .					cache_stat

	CALL SPLIT_LIST(pttmode_explct, show_lun, risc_buff, 0)
	risc_buff = ' '
	WRITE ( risc_buff, 3020 )
     .	  ( ss_lo(idim), ww_lo(idim), idim=1,listdims )
	CALL SPLIT_LIST(pttmode_explct, show_lun, risc_buff, 0)
	risc_buff = ' '
	WRITE ( risc_buff, 3020 )
     .	  ( ss_hi(idim), ww_hi(idim), idim=1,listdims )
	CALL SPLIT_LIST(pttmode_explct, show_lun, risc_buff, 0)
	risc_buff = ' '

* RISC compiler bug: cannot use character functions in WRITE statement
* ... use ww_lo and ss_lo as temporary buffers
        DO 200 idim = 1, listdims
           ss_lo(idim) = ALG_TRANS_CODE( mr_trans(idim,mr) )
           ww_lo(idim) = ARG_STRING( mr_trans_arg(idim,mr) )
 200    CONTINUE
	WRITE ( risc_buff, 3030 ) (ss_lo(idim), ww_lo(idim), idim = 1,listdims)

	CALL SPLIT_LIST(pttmode_explct, show_lun, risc_buff, 0)

 3010	FORMAT(T2,A,'[',2A,T36,A4,'  mr:',A, ' size:', A, 1X,A2,1X,A4,1x,A8)
 3020	FORMAT(T2,5(A9,'/',A12),A9,'/',A12)
 3030	FORMAT(T2,6(4X,A3,A12))

 5000	RETURN
 5100	CALL ERRMSG( ferr_invalid_command, status,
     .		     'variable has more than 4 dimensions. Set mode 6D_LAB'
     .		     //pCR//'Output will automatically adjust to 4D or 6D grid'
     .		     //pCR//cmnd_buff(:len_cmnd),
     .			*5000 )
	END
