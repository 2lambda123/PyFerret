	SUBROUTINE SETUP_KEYS( nvar )

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
* determine parameters needed to layout titles and keys on the plot based on
* how many variables are in the plot

* programmer - steve hankin
* NOAA/PMEL, Seattle, WA - Tropical Modeling and Analysis Program
* written for VAX computer under VMS operating system

* V200: 12/21/89
* 	  5/6/90 - fixed bug when xlen is very small using MAX
* V420:  4/23/96 - number of columns was computed without respect to nvar
*	  	thus if the plot could fit 3 keys across but only 2 were needed
*	  	they didn't end up centered
* V540: *sh* 9/01 - do not adjust Y axis origin of PLOT command if user
*			specifies not to
*       *acm* 3/12 Add E and F dimensions (use nferdims in tmap_dims.parm)
* PyFr *kms*  7/13 Moved key_xspace0, key_yspace0, key_labht0, key_xorigin, 
*                  key_yorigin, key_xwhite_space, key_ywhite_space, 
*                  key_line_frac, ovkey_space, ovkey_yspace, ovkey_labht,
*                  ovkey_xorigin, ovkey_yorigin, ul_labht0, ul_yspace0,
*                  ul_xorigin, and ul_yorigin to plot_setup.parm,
*                  since they are used as parameters, and prefixed with 
*                  dflt_ those that need to be scaled by pplscale or textscale.
* V698 *sh* 3/16 Limit the maximum number of keys.
*                /NOKEY and /TITLE suppresses yadjust

        IMPLICIT NONE
	include 'tmap_dims.parm'
	include	'ferret.parm'
	include	'slash.parm'
	include	'plot_setup.parm'
	include	'command.parm'
	include	'xplot_setup.cmn'
	include	'xcontext.cmn'
	include	'xprog_state.cmn'
	include 'axis_inc.decl'		! axis lengths
	include 'AXIS.INC'		! axis lengths

* calling argument declarations
	INTEGER	nvar

* local variable declarations:
	LOGICAL	change_yorg, change_ysize, yadjust
	INTEGER	nrow, idim, nkey
	REAL	ysqueeze, yspace_needed, yspace_avail

* limit the number of labels
	nkey = MIN(nvar, max_key_entries)

* how many columns of keys can fit on the plot ?
	key_ncolmn  = MAX( 1, INT( (xlen - dflt_key_xorigin*pplscale)
     .                            /(dflt_key_xspace0*pplscale) ) )
	IF (key_ncolmn .GT. nkey) key_ncolmn = nkey	! 4/96
	key_xspace  = MAX( xlen/5., (xlen - dflt_key_xorigin*pplscale)/key_ncolmn )
	key_xsize   = MAX( xlen/6., key_xspace - dflt_key_xwhite_space*pplscale )

* and how many rows do we need ?
	nrow = ((nkey-1) / key_ncolmn) + 1
	yspace_needed = nrow*dflt_key_yspace0*pplscale

* where was the y origin before we tampered with it ?
	IF ( saved_ppl_yax ) THEN
	   yspace_avail = ppl_saved_yorg
	ELSE
	   yspace_avail = yorg
	ENDIF
	yspace_avail = yspace_avail + dflt_key_yorigin*pplscale

* compute how much we will allow the key labels to shrink
	IF ( nrow .LE. 2 ) THEN
	   ysqueeze = 0.9
	ELSE
	   ysqueeze = MAX( 0.75, 0.92**(nrow-1) )
	ENDIF

* determine if
*  1) the keys will fit using the default y origin and key label size
*  2) the keys will fit but only by shrinking the label size
*  3) the keys will fit only by shifting the y origin (y location of x axis)
	yadjust = cmnd_num .EQ. cmnd_plot
	IF (yadjust) yadjust = qual_given(slash_plot_noyadjust).EQ.0
	IF (yadjust) yadjust = qual_given(slash_plot_nokey).EQ.0
	IF (yadjust) yadjust = qual_given(slash_plot_title).EQ.0
	IF ( .NOT.yadjust ) THEN
	   change_ysize = .FALSE.
	   change_yorg  = .FALSE.
	ELSE
	   change_ysize = yspace_avail .LT. yspace_needed
	   change_yorg  = yspace_avail .LT. yspace_needed*ysqueeze
	ENDIF

	IF ( .NOT. change_yorg ) THEN
* ... restore the default vertical layout if we tampered with it
           IF ( saved_ppl_yax ) THEN
	      ylen = ppl_saved_yaxlen
	      yorg = ppl_saved_yorg
	      saved_ppl_yax = .FALSE.
	   ENDIF
	ENDIF

	IF ( change_yorg ) THEN
* ... shrunken keys AND increased yorigin
* ... save the default vertical layout so we can restore it later
	   IF ( .NOT. saved_ppl_yax ) THEN
	      ppl_saved_yorg   = yorg
	      ppl_saved_yaxlen = ylen
	      saved_ppl_yax = .TRUE.
	   ENDIF 
	   yorg = yspace_needed * ysqueeze - dflt_key_yorigin*pplscale
	   ylen = (ppl_saved_yaxlen + ppl_saved_yorg) - yorg
	   key_yspace = dflt_key_yspace0*pplscale * ysqueeze
	   key_ysize  = (dflt_key_yspace0*pplscale - 
     .                   dflt_key_ywhite_space*pplscale) * ysqueeze
	   key_labht  = dflt_key_labht0*textscale * ysqueeze
	ELSEIF ( change_ysize ) THEN
* ... shrunken keys but don't tamper with y origin
	   ysqueeze = yspace_avail / yspace_needed
	   key_yspace = dflt_key_yspace0*pplscale * ysqueeze
	   key_ysize  = (dflt_key_yspace0*pplscale - 
     .                   dflt_key_ywhite_space*pplscale) * ysqueeze
	   key_labht  = dflt_key_labht0*textscale * ysqueeze
	ELSE
* ... full-sized keys, don't tamper with y origin
	   key_yspace = dflt_key_yspace0*pplscale
 	   key_ysize  = dflt_key_yspace0*pplscale - 
     .                  dflt_key_ywhite_space*pplscale
	   key_labht  = dflt_key_labht0*textscale
	ENDIF

* how many labels in the upper left corner?
	ul_nlabs = 0
	DO 100 idim = 1, nferdims
	   ul_dolab(idim) = cx_lo_ww(idim,cx_plot).NE.unspecified_val8
     . 		      .AND. cx_lo_ww(idim,cx_plot).NE.unspec_val8_2
	   IF ( ul_dolab(idim) ) ul_nlabs = ul_nlabs + 1	
 100	CONTINUE	

	RETURN
	END
