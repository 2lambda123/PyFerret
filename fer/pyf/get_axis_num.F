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



*     Assign axisnum, axisstart, and axisend with the number, start
*     index, and end index of a line, either existing or created by
*     this routine, that matches the rest of the axis information
*     arguments.  An empty (blank) axisname will match all names of
*     existing axes.  If an error occurs, errmsg will be assigned
*     with the error message and lenerrmsg will be the length of that
*     message (a positive value).  If successful, lenerrmsg will be
*     zero on return.
*         axiscoords is a array of floating-point axis coordinates

      SUBROUTINE GET_AXIS_NUM(axisnum, axisstart, axisend,
     .                        axisname, axisunit, axiscoords,
     .                        numcoords, axistype, errmsg, lenerrmsg)
      IMPLICIT NONE

#include "gt_lib.parm"
#include "tmap_dset.parm"
      INCLUDE 'implicit.parm'
      INCLUDE 'tmap_dims.parm'
      INCLUDE 'tmap_errors.parm'
      INCLUDE 'xtm_grid.cmn_text'
      INCLUDE 'pyferret.parm'

*     Passed arguments
      CHARACTER*(*) axisname, axisunit, errmsg
      INTEGER       axisnum, axisstart, axisend,
     .              numcoords, axistype, lenerrmsg
      REAL*8        axiscoords
      DIMENSION     axiscoords(numcoords)

*     Function declarations
      INTEGER TM_LENSTR, TM_UNIT_ID, STR_UPCASE
      LOGICAL TM_LEGAL_NAME, CHECK_LINE_SUBSET

*     Local arguments
      CHARACTER*(64) capaxname, uniqaxname
      INTEGER        axucode, k, sts, first_pt, j
      REAL*8         delta, eps, val, modulen
      LOGICAL        reg, modu

*     Sanity check
      IF ( numcoords .LT. 1 ) THEN
          errmsg = 'Non-positive numcoords passed to GET_AXIS_NUM'
          lenerrmsg = TM_LENSTR(errmsg)
          RETURN
      ENDIF

*     Uppercase the name for comparisons
      IF ( axisname .NE. '' ) THEN
          sts = STR_UPCASE(capaxname, axisname)
          IF ( .NOT. TM_LEGAL_NAME(capaxname) ) THEN
              errmsg = 'Invalid axis name given'
              lenerrmsg = TM_LENSTR(errmsg)
              RETURN
          ENDIF
      ELSE
          capaxname = ''
      ENDIF

*     Get the code number of known units; returns zero for unknown units
      axucode = TM_UNIT_ID(axisunit)

      IF ( numcoords .GT. 1 ) THEN
          delta = (axiscoords(numcoords) - axiscoords(1)) /
     .                       (numcoords - 1)
      ELSE
          delta = 0.0
      ENDIF
      eps = abs(delta) * 1.0E-7
      IF ( eps .LT. 1.0E-14 ) THEN
          eps = 1.0E-14
      ENDIF
*     Check for regular coordinates
      reg = .TRUE.
      DO 10 k = 2, numcoords-1
         val = axiscoords(1) + (k-1) * delta
         IF ( abs(val - axiscoords(k)) .GT. eps ) THEN
             reg = .FALSE.
             GOTO 20
         ENDIF
   10 CONTINUE

*     Crude assumption:
*        longitude are modular with a modulo of 360 deg
*        and nothing else is modular.
   20 IF ( axistype .EQ. AXISTYPE_LONGITUDE ) THEN
          modu = .TRUE.
          modulen = 360.0
      ELSE
          modu = .FALSE.
          modulen = 0.0
      ENDIF

*     See if a dynamic line already exists containing the provided info
      k = max_lines
   50 k = line_flink(k)
      IF ( k .LE. max_lines ) THEN
          GOTO 100
      ENDIF
      IF ( line_class(k) .EQ. pline_class_basic ) THEN
          IF ( CHECK_LINE_SUBSET(k, axisstart, axisend, capaxname,
     .                    axisunit, axiscoords, numcoords, delta, eps,
     .                    reg, modu, modulen, axucode) ) THEN
              axisnum = k
              GOTO 500
          ENDIF
      ENDIF
      GOTO 50

*     See if a permanent line already exists containing the provided info
 100  CONTINUE
      DO 110 k = 1, max_lines
          IF ( (line_class(k) .EQ. pline_class_basic) .AND.
     .         (line_name(k) .NE. char_init16) ) THEN
              IF ( CHECK_LINE_SUBSET(k, axisstart, axisend, capaxname,
     .                   axisunit, axiscoords, numcoords, delta, eps,
     .                   reg, modu, modulen, axucode) ) THEN
                  axisnum = k
                  GOTO 500
              ENDIF
          ENDIF
  110 CONTINUE

*     If we got here, there is no line containing the provided info.
      IF ( capaxname .NE. '' ) THEN
*         If a name is given, make sure it is unique, or make it unique.
*         Do this first so the newly created dynamic line will not interfere.
          CALL TM_NEW_LINE_NAME(capaxname, uniqaxname)
      ENDIF
*     Now create the dynamic line
      CALL TM_ALLO_DYN_LINE(k, sts)
      IF ( sts .NE. merr_ok ) THEN
          errmsg = 'No space left for a new dynamic line'
          lenerrmsg = TM_LENSTR(errmsg)
          RETURN
      ENDIF
*     Assign it a unique name.
      IF ( capaxname .EQ. '' ) THEN
          WRITE(line_name(k), 160) (k - max_lines)
  160     FORMAT('(AX',I3.3,')')
          line_name_orig(k) = line_name(k)
      ELSE
          line_name(k)      = uniqaxname
          line_name_orig(k) = axisname
      ENDIF
*     Assign the rest of the line info
      line_units(k)         = axisunit
      line_unit_code(k)     = axucode
      line_t0(k)            = char_init20
      line_shift_origin(k)  = .FALSE.
      line_tunit(k)         = 0.0
      line_parent(k)        = 0
      line_class(k)         = pline_class_basic
      line_modulo(k)        = modu
      line_modulo_len(k)    = modulen
      IF ( axistype .EQ. AXISTYPE_LONGITUDE ) THEN
          line_direction(k) = axis_orients(1)
      ELSE IF ( axistype .EQ. AXISTYPE_LATITUDE ) THEN
          line_direction(k) = axis_orients(2)
      ELSE IF ( axistype .EQ. AXISTYPE_LEVEL ) THEN
          line_direction(k) = axis_orients(3)
      ELSE
          line_direction(k) = 'NA'
      ENDIF
      line_dim(k)           = numcoords
      line_regular(k)       = reg
      IF ( reg ) THEN
          line_start(k)     = axiscoords(1)
          line_delta(k)     = delta
      ELSE
          CALL GET_LINE_DYNMEM(numcoords, k, sts)
          IF ( sts .NE. merr_ok ) THEN
              errmsg = 'No memory left for a new irregular line'
              lenerrmsg = TM_LENSTR(errmsg)
*             No memory allocated so set to regular for cleanup
              line_regular(k) = .TRUE.
              CALL TM_RM_TMP_LINE(k, sts)
              RETURN
          ENDIF
          CALL COPY_LINE_COORDS(axiscoords, linemem(k)%ptr, numcoords)
          CALL TM_IRREGAX_MID(axiscoords, lineedg(k)%ptr, numcoords)
          line_start(k)     = axiscoords(1)
          line_delta(k)     = unspecified_val8
      ENDIF
      line_use_cnt(k)       = 0

      axisnum = k
      axisstart = 1
      axisend = numcoords

  500 lenerrmsg = 0
      RETURN

      END

