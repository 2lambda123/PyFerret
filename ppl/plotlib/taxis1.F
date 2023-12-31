      SUBROUTINE TAXIS1(TLO,ITIME1,ITIME2,DT,NSMTIC,LABEL,IDDB,IMDB,
     1    TFACT,Y,YFACT,CSIZE,yacross,vgrat,vcolor,vline,color0)

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
* revised for TMAP
* from PPLUS :
*	Directory PMEL:[PLOT5.PPLV11.PLOTLIB]TAXIS1.FOR

* search for "TMAP" to find all revisions
* *sh*  - 1/23/89 added interrupt detection
*		  ... no explicit declarations (too much trouble !) so it must
*		      be compiled with /WARNINGS=NODECLARATIONS

* *jd* Mod for 4 digit years
C *kob* 04/07/96 passed in character strings had to be declared 
C                with "*(*)" for the linux port 

C *acm* 24/01/00 Y2k Bug -- Put century at start when comparing 
C                 ITIME to ITIME2 at loop 10
C *acm* 03/04  new arguments and code for plotting graticule lines across plot. 
C V65  *acm*  1/10 New time-axis style for DAY and MONTH, DMY: adds the
C                  year to DAY,MONTH or to MONTH.
C V695 *acm*  5/15 Restore color after a graticule using defined color 0
* *acm* - 8/16   ticket 1787: restore pen 0 correctly after graticule

C**
C**    @(#)taxis1.f	1.1    3/10/88
C**
C**
C***********************************************************************
C**
C**                 PLOT+ Scientific Graphics System
C**
C***********************************************************************
C**
C**
C
C     THIS ROUTINE PLOTS A TIME AXIS AND LABELS IT WITH THE DAY OF THE M
C     AND THE MONTH.
C     VARIBLES:
C         TLO=X VALUE CORRESPONDING TO THE START TIME.
C         ITIME1 = START TIME (IE.  7708190000)
C                  WE RESTRICT THE PERMISSIBLE START TIMES TO BE
C                  EVEN DAYS.  (THAT IS THE LAST 6 DIGITS OF ITIME1
C                  MUST BE 0)
C         ITIME2 = END TIME (IE   7709070000)
C                  THIS IS THE FIRST DAY NOT TO BE PLOTTED, AND IT
C                  MUST BE AN EVEN DAY.
C         DT = SAMPLING RATE IN MINUTES.
C         NSMTIC = NUMBER OF SMALL TICS TO MAKE EACH DAY.
C                = 23 FOR HOURLY TICS.
C                = 11 FOR BIHOURLY TICS.
C                   ETC.
C         LABEL > 0 IF LABELS ARE TO BE DRAWN ABOVE THE AXIS.
C               = 0 IF NO LABELS ARE TO BE DRAWN.
C               < 0 IF LABEL ARE TO BE DRAWN BELOW THE AXIS.
C         IABS(LABEL) = 1 FOR LABELS OF THE STYLE
C                  1..........1..........1..........1..........1
C                       3           4          5        6
C                                      JUN
C         IABS(LABEL) = 2 FOR LABELS OF THE STYLE
C                  1..........1..........1..........1..........1
C                     JUN  3     JUN  4      JUN  5    JUN  6
C         IDDB = PRINT LABELS OF DAYS DIVIBLE BY THIS NUMBER ONLY.
C                IF(IDDB = 0) PRINT NO DAY LABELS.
C         IMDB = PRINT LABELS OF MONTHS DIVISIBLE BY THIS NUMBER ONLY.
C                IF(IMDB = 0) PRINT NO MONTH LABELS.
C                THIS VARIBLE ONLY APPLYS WHEN IABS(LABEL) = 1
C         TFACT = (LENGTH OF T AXIS IN INCHES)/(LENGTH OF T AXIS IN USER
C         Y = Y COORDINATE OF TIME AXIS.
C         YFACT = (LENGTH OF Y AXIS IN INCHES)/(LENGTH OF Y AXIS IN USER
C         CSIZE = SIZE TO DRAW DAY LABELS.
C                 MONTH LABELS WILL BE 1.5 TIMES AS LARGE.
C
C     THERE WAS A MODIFICATION TO THIE IN JUNE 1980.  THE
C     SOLE PURPOSE FOR THIS MODIFICATION WAS TO IMPROVE 'S
C     EFFICIENCY.
C
	CHARACTER ITIME3*14
	CHARACTER SYM*21,ITIME1*(*),ITIME*14,ITIME2*(*),MON*3,DAY*2,
     .		  YEAR*4, BUFF*8
	COMMON/TICS/TSMX,TLX,TSMY,TLY,IX,IY
	COMMON/AXLWID/XWID,YWID

* Parameters for drawing tics across plot

        REAL yacross
        REAL dash1_1, dash1_2, dash1_3, dash1_4
        REAL dash2_1, dash2_2, dash2_3, dash2_4
        REAL vgrat(4,2)        ! line type: 0=tics only, <0=line, >0=dash lengths 
        INTEGER vcolor(*)      ! pen number for graticule lines
	INTEGER color0	       ! pen number to restore 
        LOGICAL vline, do_line   ! If true, do the lines across.
        LOGICAL solid1, solid2   ! solid lines across at tics
        LOGICAL dashed1, dashed2 ! dashed lines across at tics

        INTEGER ltic, color1, color2

	include 'xinterrupt.cmn'
        include 'century.cmn'  ! with MONYRLAB flag
        include 'parampl5_dat.decl'
        include 'PARAMPL5.DAT'
        include 'lines_inc.decl'
        include 'LINES.INC'
        INCLUDE 'pen_inc.decl'  ! with IPEN(1)
        INCLUDE 'PEN.INC'

C     CHECK THE PARAMETERS PASSED TO THE SUBROUTINE.
	IF(ITIME1(7:10).NE.'0000')STOP ',1'
	IF(ITIME2(7:10).NE.'0000')STOP  ',1'

C set up for plotting graticules across at tic marks
        
        solid1 = (vgrat(1,1).LT.0 .AND. vgrat(2,1).LT.0 .AND.  
     .            vgrat(3,1).LT.0 .AND.  vgrat(4,1).LT.0 )
        solid2 = (vgrat(1,2).LT.0 .AND. vgrat(2,2).LT.0 .AND.  
     .            vgrat(3,2).LT.0 .AND.  vgrat(4,2).LT.0 )
        dashed1 = (vgrat(1,1).GT.0 .OR. vgrat(2,1).GT.0 .OR.  
     .            vgrat(3,1).GT.0 .OR.  vgrat(4,1).GT.0 )
        dashed2 = (vgrat(1,2).GT.0 .OR. vgrat(2,2).GT.0 .OR.  
     .            vgrat(3,2).GT.0 .OR.  vgrat(4,2).GT.0 )

	do_line = vline

        color1 = vcolor(1)
        color2 = vcolor(2)

        IF (dashed1) THEN
           dash1_1 = vgrat(1,1)
           dash1_2 = vgrat(2,1)
           dash1_3 = vgrat(3,1)
           dash1_4 = vgrat(4,1)
        ENDIF
        IF (dashed2) THEN
           dash2_1 = vgrat(1,2)
           dash2_2 = vgrat(2,2)
           dash2_3 = vgrat(3,2)
           dash2_4 = vgrat(4,2)
        ENDIF

C
C     SET PARAMETERS.

*     *jd Number of arguments of TIME3 calls reduced to 6.  TIME3 uses only
*     the first 6, 'seconds' are not handled.  Done for MAC 7.92

 
      TIC = 1440./DT
      NTIC = NSMTIC + 1
	IF(NSMTIC.LT.0)THEN
		IHVY=-8
		INML=-7
	ELSE
		IHVY=-9
		INML=-8
		TDIST = TIC/NTIC
	ENDIF
	IF(LABEL+IX.EQ.0)THEN
		YTEMP=0.0
	ELSE
		YTEMP=TLX*0.5
	ENDIF
	IF(IDDB.NE.0)THEN
	FIDDB=0.0
	ELSE
	FIDDB=-1.4
	ENDIF
      IF(LABEL.LT.0) THEN
      YTEMP1=Y-(1.4*CSIZE+YTEMP)/YFACT
      YTEMP2=Y-((4.0+FIDDB)*CSIZE+YTEMP)/YFACT
      ELSE
      YTEMP1=Y+(0.4*CSIZE+YTEMP)/YFACT
      YTEMP2=Y+((2.5+FIDDB)*CSIZE+YTEMP)/YFACT
      ENDIF
	IF(LABEL.NE.0)XWID=(4.0+FIDDB)*CSIZE+YTEMP

      CALL PLOT(TLO,Y,0,0)
	IF(IDAY.EQ.1)THEN
	   CALL PLOTT7(TLO,Y,TFACT,YFACT,1,IHVY)
	ELSE
	   CALL PLOTT7(TLO,Y,TFACT,YFACT,1,INML)
	ENDIF

      T=TLO
      ITIME=ITIME1
      CALL TIME3(ITIME,IYR,IMON,IDAY,IHR,IMIN,ISEC,ICEN)
      TMON=T

c ----  count number of large tics. If no large tics will be drawn
c ----  then will want to draw graticule lines at the small ones.

C      LOOP like loop 10 below
      ltic = 0
  110 CONTINUE
      IF ( interrupted ) RETURN		! TMAP interrupt
      
      CALL TIME1(1,ITIME,2,' ',I3,ITIME3,1440.)
      CALL TIME3(ITIME3,IYR3,IMON3,IDAY3,IHR3,IMIN3,ISEC3,ICEN3)

      ITIME=ITIME3
      IF(ITIME(13:14)//ITIME(1:12) .LT. 
     .       ITIME2(13:14)//ITIME2(1:12) ) THEN
         IF (IDAY3.EQ.1) ltic = ltic + 1  !It is a large tic.
         GO TO 110
      ENDIF

! IF no large tics, and if graticules at just the large tics were
! requested, then plot graticules at the small tics

      IF (ltic .EQ. 0) THEN   
           IF ( (.NOT.solid2) .AND. (solid1) ) THEN
              solid2 = solid1
              color2 = color1
           ENDIF
           IF ( (.NOT.dashed2) .AND. (dashed1) ) THEN 
              dashed2 = dashed1
              dash2_1 = dash1_1
              dash2_2 = dash1_2
              dash2_3 = dash1_3
              dash2_4 = dash1_4
              color2 = color1
           ENDIF
      ENDIF

      ITIME=ITIME1
      CALL TIME3(ITIME,IYR,IMON,IDAY,IHR,IMIN,ISEC,ICEN)

c ----

C      LOOP:
   10     CALL PLOT(T,Y,0,0)
	  IF ( interrupted ) RETURN		! TMAP interrupt

*    do not draw a graticule line along the upper axis
	  do_line = ( ITIME3(13:14)// ITIME3(1:10).LT.
     .	              ITIME2(13:14)//ITIME2(1:10)) 

          DO 20 I=1,NSMTIC
              CALL PLOTT7(T+FLOAT(I)*TDIST,Y,TFACT,YFACT,1,-7)
              

                 IF ((solid2.OR.dashed2) .AND. do_line) THEN ! make line across
                    CALL COLOR(color2)
                    IF (dashed2) THEN
                       CALL DASHSZ(dash2_1,dash2_2,dash2_3,dash2_4)
                       CALL DASHES
                    ENDIF
                    CALL PLOT(T+FLOAT(I)*TDIST,y,0,0) 
                    CALL PLOT(T+FLOAT(I)*TDIST,yacross,1,0)
                    CALL PLOT(T+FLOAT(I)*TDIST,Y,0,0) 
                    CALL COLOR(color0)
                    CALL VECTRS
                 ENDIF

   20         CONTINUE
          CALL TIME1(1,ITIME,2,' ',I3,ITIME3,1440.)
          CALL TIME3(ITIME3,IYR3,IMON3,IDAY3,IHR3,IMIN3,ISEC3,ICEN3)	
	  
*    do not draw a graticule line along the upper axis
	  do_line = ( ITIME3(13:14)// ITIME3(1:10).LT.
     .	              ITIME2(13:14)//ITIME2(1:10)) 

          IF(IDAY3.EQ.1)THEN
	      CALL PLOTT7(T+TIC,Y,TFACT,YFACT,1,IHVY)
               
                 IF ((solid1.OR.dashed1) .AND. do_line) THEN ! make line across
                    CALL COLOR(color1)
                    IF (dashed1) THEN
                       CALL DASHSZ(dash1_1,dash1_2,dash1_3,dash1_4)
                       CALL DASHES
                    ENDIF
                    CALL PLOT(t+tic,y,0,0) 
                    CALL PLOT(t+tic,yacross,1,0)
                    CALL PLOT(t+tic,Y,0,0) 
                    CALL COLOR(color0)
                    CALL VECTRS
                 ENDIF

	ELSE

              IF ((solid2.OR.dashed2) .AND. do_line) THEN ! make line across
                 CALL COLOR(color2)
                 IF (dashed2) THEN
                    CALL DASHSZ(dash2_1,dash2_2,dash2_3,dash2_4)
                    CALL DASHES
                 ENDIF
                 CALL PLOT(t+tic,y,0,0) 
                 CALL PLOT(t+tic,yacross,1,0)
                 CALL PLOT(t+tic,Y,0,0) 
                 CALL COLOR(color0)
                 CALL VECTRS
              ENDIF
	      CALL PLOTT7(T+TIC,Y,TFACT,YFACT,1,INML)

	ENDIF
C         THE LABELS
         CALL TIME3(ITIME,IYR,IMON,IDAY,IHR,IMIN,ISEC,ICEN)
         IF(IDDB.EQ.0.OR.LABEL.EQ.0) GO TO 50
         IF(IDAY.NE.IDDB*(IDAY/IDDB)) GO TO 50
             CALL TIME2(ITIME,SYM)
	READ(SYM,30) MON,DAY
30	FORMAT(6X,A3,1X,A2,1X,A4)
	IF(DAY(1:1).EQ.' ')THEN
		NC=1
		DAY=DAY(2:2)
	ELSE
		NC=2
	ENDIF
	IF(IABS(LABEL).EQ.1)THEN
        TSYM=T+(TIC-SYMWID(CSIZE,NC,DAY)/TFACT)*.5
        CALL SYMBEL(TSYM,YTEMP1,0.,CSIZE,NC,DAY)
	ELSE
	WRITE(SYM,40)MON,DAY
40	FORMAT(A3,' ',A2)
        TSYM=T+(TIC-SYMWID(CSIZE,6,SYM)/TFACT)*.5
        CALL SYMBEL(TSYM,YTEMP1,0.,CSIZE,6,SYM)
	ENDIF
   50   CONTINUE
          IF(IMDB.EQ.0.OR.IABS(LABEL).NE.1) GO TO 70
              IF(IMON.EQ.IMON3) GO TO 70
              IF(IMON.NE.IMDB*(IMON/IMDB)) GO TO 60
C             ELSE:
	IF(T-TMON.GT.4320./DT)THEN
           CALL TIME2(ITIME,SYM)
           READ(SYM,30)MON,DAY,YEAR
           IF (monyrlab) THEN
	      WRITE (buff, 100) mon, year
 100	      FORMAT (A3, ' ', A4)
              TSYM=TMON+(T+TIC-TMON-SYMWID(1.5*CSIZE,8,BUFF)/TFACT)*.5
	      CALL SYMBEL(TSYM,YTEMP2,0.,1.5*CSIZE,8,buff)
	   ELSE
              TSYM=TMON+(T+TIC-TMON-SYMWID(1.5*CSIZE,3,MON)/TFACT)*.5
	      CALL SYMBEL(TSYM,YTEMP2,0.,1.5*CSIZE,3,MON)
	   ENDIF
	ENDIF
   60         TMON=T+TIC
   70     CONTINUE
          ITIME=ITIME3
          T=T+TIC
c          IF(ITIME.LT.ITIME2) GO TO 10

          IF(ITIME(13:14)//ITIME(1:12) .LT. 
     .       ITIME2(13:14)//ITIME2(1:12) ) GO TO 10
	     IF(IMDB.EQ.0) RETURN

*     Draw complete time axis
          CALL PLOT(TLO,Y,0,0)
          CALL PLOT(T,Y,1,0)

	     
	IF(IABS(LABEL).EQ.1.AND.IMON.EQ.IMDB*(IMON/IMDB)
     .	.AND.IDAY3.GE.4)THEN
           CALL TIME2(ITIME,SYM)
	   READ(SYM,30)MON,DAY, year
           IF (monyrlab) THEN
	      WRITE (buff, 100) mon, year
              TSYM=TMON+(T+TIC-TMON-SYMWID(1.5*CSIZE,8,BUFF)/TFACT)*.5
	      CALL SYMBEL(TSYM,YTEMP2,0.,1.5*CSIZE,8,buff)
	   ELSE
              TSYM=TMON+(T+TIC-TMON-SYMWID(1.5*CSIZE,3,MON)/TFACT)*.5
	      CALL SYMBEL(TSYM,YTEMP2,0.,1.5*CSIZE,3,MON)
	   ENDIF
     	ENDIF
      RETURN
      END
