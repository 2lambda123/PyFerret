        SUBROUTINE SET_TEXT_SIZES(windid, factor)

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
*  ARE DISCLAIMED. IN NO EVENT SHALL NOAA/PMEL/TMAP BE LIABLE FOR ANY 
*  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
*  RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
*  CONTRACT, NEGLIGENCE OR OTHER TORTUOUS ACTION, ARISING OUT OF OR IN
*  CONNECTION WITH THE ACCESS, USE OR PERFORMANCE OF THIS SOFTWARE. 

*  If factor is positive, reset the wn_...() array text sizes
*  to the defaults scaled by the given factor and by pplscale.
*  Always tell ppl about the text sizes assigned in the wn_...() arrays.

        IMPLICIT NONE

        INCLUDE 'tmap_dims.parm'
        INCLUDE 'ferret.parm'
        INCLUDE 'plot_setup.parm'
        INCLUDE 'xplot_setup.cmn'
        INCLUDE 'xplot_state.cmn'
        INCLUDE 'parampl5_dat.decl'
        INCLUDE 'PARAMPL5.DAT'
        INCLUDE 'cont_inc.decl'
        INCLUDE 'CONT.INC'        ! hgt

* argument declaration
        INTEGER windid
        REAL    factor

* local variables
        CHARACTER*48 buff

        IF ( factor .GT. 0.0 ) THEN
*          Reset this windows text scaling factor
           textscale = factor * pplscale
           wn_textscale(windid) = textscale
           wn_axlsze_x(windid)  = dflt_letsize_axis  * textscale
           wn_axlsze_y(windid)  = dflt_letsize_axis  * textscale
           wn_txlsze(windid)    = dflt_letsize_axis  * textscale
           wn_labset_mn(windid) = dflt_letsize_title * textscale
           wn_labset_x(windid)  = dflt_letsize_label * textscale
           wn_labset_y(windid)  = dflt_letsize_label * textscale
           wn_labset_mv(windid) = dflt_letsize_label * textscale
           wn_con_hgt(windid)   = 0.75 * dflt_letsize_label * textscale
        ELSE
*          Assign this windows text scaling factor
           textscale = wn_textscale(windid)
        ENDIF

*       set the X and Y axis value labels sizes
        buff = ' '
        WRITE(buff, 110) wn_axlsze_x(windid), wn_axlsze_y(windid)
 110    FORMAT('AXLSZE',2(',',F7.4))
        CALL PPLCMD(from, line, 0, buff, 1, 1)

*       set the time axis value labels size
        buff = ' '
        WRITE(buff, 120) wn_txlsze(windid)
 120    FORMAT('TXLSZE,',F7.4)
        CALL PPLCMD(from, line, 0, buff, 1, 1)

*       set the title labels sizes
        buff = ' '
        WRITE(buff, 130) wn_labset_mn(windid), wn_labset_x(windid), 
     .                   wn_labset_y(windid), wn_labset_mv(windid)
 130    FORMAT('LABSET',4(',',F7.4))
        CALL PPLCMD ( from, line, 0, buff, 1, 1 )

*       set the contour label values labels size
*       the following can just be set directly (no extra actions)
        hgt = wn_con_hgt(windid)

        RETURN
        END
