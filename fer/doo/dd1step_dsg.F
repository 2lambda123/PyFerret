	SUBROUTINE DD1STEP_DSG (idim, shf, dset, nfeatures, maxobs,
     .			com, com_mr, com_cx,
     .			res, res_mr, res_cx,
     .			box	)

*
*
*
*  This software was developed by the Thermal Modeling and Analysis
*  Project(TMAP) of the National Oceanographic and Atmospheric
*  Administration''s (NOAA) Pacific Marine Environmental Lab(PMEL),
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
* compute a 1 step derivative of the component along axis idim
* for data on Distributed Sampling Geometries grids
* note: for a forward  derivative shf=0
*       for a backward derivative shf=1
*
* programmer - Ansley Manke
* NOAA/PMEL, Seattle, WA - Science Data Integration Group
* 2/2019
* V76   1/20 *acm* Orientation of Point-type data set to e_dim
* 10/2022 *acm* Code cleanup: remove comment after # include statement

        IMPLICIT NONE
        include 'tmap_dims.parm'
#	include	"tmap_dset.parm"
	include	'ferret.parm'
	include	'xcontext.cmn'
	include	'xvariables.cmn'
	include	'xmem_subsc.cmn'
	include 'xunits.cmn_text'
	external xunits_data
	include 'xtm_grid.cmn_text'
	external xgt_grid_data
	include 'xdyn_linemem.cmn_text'
	include 'xdset_info.cmn_text'
	include 'xdsg_context.cmn'


* calling argument declarations:
	INTEGER	idim, shf, dset, nfeatures, maxobs, com_mr, com_cx, res_mr, res_cx
	REAL	box( * )
	REAL	com   (*),
     .          res   (*)

* internal variable declarations:
	LOGICAL	fmask(nfeatures), omask(maxobs)
	INTEGER i, ii, ifeature, iobs, flen, base, orientation, row_size_lm, 
     .          dsg_fmt_grid, dsg_coord_lm, iaxis
	REAL	bad_com, bad_res, lo_com, hi_com, unit, 
     .          dummy, boxdel, boxmean, boxstd


* initialize
	bad_com = mr_bad_data( com_mr )
	bad_res = mr_bad_data( res_mr )

* get feature mask and dsg grid and coordinate information
        CALL MAKE_DSG_FEATURE_MASK(dset, res_cx, fmask, nfeatures)

	row_size_lm  = dsg_loaded_lm(dsg_row_size_var(dset))
	dsg_fmt_grid = dsg_xlate_grid(dset)
	orientation  = dsg_orientation(dset)
	dsg_coord_lm = cxdsg_coord_lm(orientation)

* perform integrations on standard units if possible
	iaxis = grid_line(orientation, dsg_fmt_grid)
	unit = un_convert( line_unit_code(iaxis) )
	unit = 1. ! just testing

* Handle z transforms for feature-type of trajectory-profile or timeseries-profile - within this routine
* treat the data type as profile.

	IF (orientation.EQ.pfeatureType_TrajectoryProfile .OR. 
     .	      orientation.EQ.pfeatureType_TimeseriesProfile) orientation = pfeatureType_Profile

* ****** PERFORM THE COMPUTATION *****
	IF (idim .EQ. orientation) THEN

* *** true ragged array processing:

	   base = 0    ! obs index at end of preceding feature

* ... loop over the features
	   ftrloop: DO ifeature = 1, nfeatures 

	      flen = dsg_linemem(row_size_lm)%ptr(ifeature)  ! feature length
              iobs = base

* ... have the user's WW constraints removed this feature?
	      IF (.NOT.fmask(ifeature)) THEN
                 base = base + flen
                 CYCLE  ftrloop
              ENDIF

* ... get observation-level mask for this feature
	      CALL MAKE_DSG_OBS_MASK(dset, com_cx, ifeature, base,
     .				     omask, flen)

* ... loop over the observations within each feature

	      res(iobs+flen) = bad_res
	      

* Get and save box sizes for all feature coordinates. Feature-mask applied later.
* Weighted computation:
*   For trajectories the box size is the  great-circle-distance along the line between 
*   lon-lat points, For integrals, convert to meters; routine returns distance in Km.
*   In Z and T use distance between, construct boxes based on coordinate data.

              IF (orientation .EQ. x_dim) THEN
	         CALL BOXES_DSG (orientation, flen, unit, 
     .		   dsg_linemem(dsg_coord_lm)%ptr(base+1), 
     .		   dsg_linemem(cxdsg_coord_lm(y_dim))%ptr(base+1), box)

              ELSE
	         CALL BOXES_DSG (orientation, flen, unit, 
     .		   dsg_linemem(dsg_coord_lm)%ptr(base+1), 
     .		   dummy, box)

              ENDIF

              IF (unit .NE. 1.) THEN
	        DO i = 1, flen
		    box(i) = unit*box(i)
                 ENDDO
              ENDIF

* note: for a forward  derivative shf=0
*       for a backward derivative shf=1

	      oloop: DO i = 1, flen
	         iobs = iobs + 1   ! index in the contig ragged array
	         res(iobs) = bad_res
		 IF (i-shf .LT. 1) CYCLE oloop
		 IF (i+1-shf .GT. flen) cycle oloop

	         IF (.NOT.omask(i-shf)) CYCLE oloop
	         IF (.NOT.omask(i+1-shf)) CYCLE oloop

	         ii = iobs-shf
		 lo_com = com(iobs-shf)
	         ii = iobs+1-shf
	         hi_com = com(iobs+1-shf)

*    box size. For trajectory datasets, look for extra-large boxes
*    and instead use nearby delta 

		 boxdel = box(i)
		 
		 IF ( orientation.EQ.x_dim .AND. boxstd .GT. 0.) THEN
		    IF ( boxdel .GT. 2.* boxstd) THEN
		       IF (i.GT.1) THEN
		          boxdel = MIN( boxstd, box(i-1) )
		       ELSEIF (i.LT.flen) THEN
		          boxdel = MIN( boxstd, box(i+1) )
		       ELSE
		          boxdel = boxmean
		       ENDIF
		    ENDIF
	         ENDIF

	         IF ( lo_com.EQ.bad_res .OR. hi_com.EQ.bad_res .OR. boxdel.EQ.0.) THEN
	            res(iobs) = bad_res
	         ELSE
	            res(iobs) = (hi_com-lo_com) / boxdel
	         ENDIF

              ENDDO  oloop

              base = base + flen

	   ENDDO  ftrloop

	ELSE

* *** a simple list of instances:


	   DO ifeature = 1, nfeatures
	      box(ifeature) = ifeature
	   ENDDO

	   res(1) = bad_res
	   res(ifeature) = bad_res
	   DO ifeature = 1+shf, nfeatures-shf

* ... have the user's WW constraints removed this feature?
	      IF (.NOT.fmask(ifeature-shf)) CYCLE
	      IF (.NOT.fmask(ifeature+1-shf)  ) CYCLE

* ... the input data values
	      lo_com = com(ifeature-shf)
	      hi_com = com(ifeature+1-shf)

	      IF ( lo_com.EQ.bad_com .OR. hi_com.EQ.bad_com ) THEN
	         res(ifeature) = bad_res
	      ELSE
	         res(ifeature) = (hi_com-lo_com) / box(i)
	      ENDIF

	   ENDDO

	ENDIF


	RETURN
	END
