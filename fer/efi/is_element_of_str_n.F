*
* is_element_of_str_n.F
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
* Ansley Manke
* August 2005
*
*  For string arguments, 
*  Return logical value (integer 1 or 0); If argument 1 takes on any of 
*  the values of the second argument, return 1, else 0.
* 11/2022 *acm* Code cleanup: Modernize DO-loop syntax. 

*
*
* In this subroutine we provide information about
* the function.  The user configurable information 
* consists of the following:
*
* descr              Text description of the function
*
* num_args           Required number of arguments
*
* axis_inheritance   Type of axis for the result
*                       ( CUSTOM, IMPLIED_BY_ARGS, NORMAL, ABSTRACT )
*                       CUSTOM          - user defined axis
*                       IMPLIED_BY_ARGS - same axis as the incoming argument
*                       NORMAL          - the result is normal to this axis
*                       ABSTRACT        - an axis which only has index values
*
* piecemeal_ok       For memory optimization:
*                       axes where calculation may be performed piecemeal
*                       ( YES, NO )
* 
*
* For each argument we provide the following information:
*
* name               Text name for an argument
*
* unit               Text units for an argument
*
* desc               Text description of an argument
*
* axis_influence     Are this argument's axes the same as the result grid?
*                       ( YES, NO )
*
* axis_extend       How much does Ferret need to extend arg limits relative to result 
*


      SUBROUTINE is_element_of_str_n_init(id)

      IMPLICIT NONE
      INCLUDE 'EF_Util.cmn'

      INTEGER id, arg

***********************************************************************
*                                           USER CONFIGURABLE PORTION |
*                                                                     |
*                                                                     V

      CHARACTER*100 fcn_desc

      WRITE (fcn_desc, 10)
   10 FORMAT ('Returns 1 if 1st arg contains any of the ',
     .        'values in 2nd arg, 2 if case-insensitve match, else 0')
      CALL ef_set_desc(id, fcn_desc)

      CALL ef_set_num_args(id, 2)
      CALL ef_set_has_vari_args(id, NO)
      CALL ef_set_axis_inheritance_6d(id,
     .                                ABSTRACT, NORMAL,
     .                                NORMAL,   NORMAL,
     .                                NORMAL,   NORMAL)
      CALL ef_set_piecemeal_ok_6d(id, NO, NO, NO, NO, NO, NO)

      arg = 1
      CALL ef_set_arg_name(id, arg, 'VAR')
      CALL ef_set_arg_desc(id, arg, 'string')
      CALL ef_set_axis_influence_6d(id, arg,
     .                              NO, NO, NO, NO, NO, NO)
      CALL ef_set_arg_type (id, arg, STRING_ARG)

      arg = 2
      CALL ef_set_arg_name(id, arg, 'VALUES')
      CALL ef_set_arg_desc(id, arg, 'strings to search')
      CALL ef_set_axis_influence_6d(id, arg,
     .                              NO, NO, NO, NO, NO, NO)
      CALL ef_set_arg_type (id, arg, STRING_ARG)

*                                                                     ^
*                                                                     |
*                                           USER CONFIGURABLE PORTION |
***********************************************************************

      RETURN 
      END

*
* In this subroutine we provide information about the lo and hi
* limits associated with each abstract or custom axis.   The user 
* configurable information consists of the following:
*
* lo_ss               lo subscript for an axis
*
* hi_ss               hi subscript for an axis
*

      SUBROUTINE is_element_of_str_n_result_limits(id)

      IMPLICIT NONE
      INCLUDE 'EF_Util.cmn'

      INTEGER id

* **********************************************************************
*                                           USER CONFIGURABLE PORTION |
*                                                                     |
*                                                                     V

      INTEGER ivalue

      ivalue = 1
      CALL ef_set_axis_limits(id, X_AXIS, ivalue, ivalue)

*                                                                     ^
*                                                                     |
*                                           USER CONFIGURABLE PORTION |
* **********************************************************************

      RETURN 
      END


*
* In this subroutine we compute the result
*
      SUBROUTINE is_element_of_str_n_compute(id, arg_1, arg_2, result)

      IMPLICIT NONE
      INCLUDE 'EF_Util.cmn'
      INCLUDE 'EF_mem_subsc.cmn'


      INTEGER id

      REAL arg_1(mem1lox:mem1hix, mem1loy:mem1hiy, mem1loz:mem1hiz, 
     .           mem1lot:mem1hit, mem1loe:mem1hie, mem1lof:mem1hif)
      REAL arg_2(mem2lox:mem2hix, mem2loy:mem2hiy, mem2loz:mem2hiz, 
     .           mem2lot:mem2hit, mem2loe:mem2hie, mem2lof:mem2hif)

      REAL result(memreslox:memreshix, memresloy:memreshiy, 
     .            memresloz:memreshiz, memreslot:memreshit,
     .            memresloe:memreshie, memreslof:memreshif)

* After initialization, the 'res_' arrays contain indexing information 
* for the result axes.  The 'arg_' arrays will contain the indexing 
* information for each variable''s axes. 

      INTEGER res_lo_ss(6),
     .        res_hi_ss(6),
     .        res_incr (6)
      INTEGER arg_lo_ss(6,EF_MAX_ARGS),
     .        arg_hi_ss(6,EF_MAX_ARGS),
     .        arg_incr (6,EF_MAX_ARGS)

      REAL bad_flag(EF_MAX_ARGS), bad_flag_result

***********************************************************************
*                                           USER CONFIGURABLE PORTION |
*                                                                     |
*                                                                     V

      INTEGER arg
      INTEGER i, j, k, l, m, n
      INTEGER i1, j1, k1, l1, m1, n1
      INTEGER i2, j2, k2, l2, m2, n2
      INTEGER slen1, slen2
      INTEGER STR_UPCASE, status
      CHARACTER text1*128, text2*128
      CHARACTER text1up*128, text2up*128

      CALL ef_get_res_subscripts_6d(id, res_lo_ss, res_hi_ss, res_incr)
      CALL ef_get_arg_subscripts_6d(id, arg_lo_ss, arg_hi_ss, arg_incr)
      CALL ef_get_bad_flags(id, bad_flag, bad_flag_result)

C  Check for strings matching arg2

C  Returns a single value, so no incrementing result indices
      i = res_lo_ss(X_AXIS)
      j = res_lo_ss(Y_AXIS)
      k = res_lo_ss(Z_AXIS)
      l = res_lo_ss(T_AXIS)
      m = res_lo_ss(E_AXIS)
      n = res_lo_ss(F_AXIS)

C Initialize to no match since case-insensitive match does not immediately return
      result(i,j,k,l,m,n) = 0.

      DO n1=arg_lo_ss(F_AXIS,ARG1), arg_hi_ss(F_AXIS, ARG1)
      DO m1=arg_lo_ss(E_AXIS,ARG1), arg_hi_ss(E_AXIS, ARG1)
      DO l1=arg_lo_ss(T_AXIS,ARG1), arg_hi_ss(T_AXIS, ARG1)
      DO k1=arg_lo_ss(Z_AXIS,ARG1), arg_hi_ss(Z_AXIS, ARG1)
      DO j1=arg_lo_ss(Y_AXIS,ARG1), arg_hi_ss(Y_AXIS, ARG1)
      DO i1=arg_lo_ss(X_AXIS,ARG1), arg_hi_ss(X_AXIS, ARG1)

         arg = 1
         CALL ef_get_string_arg_element_6d(id, arg, arg_1, 
     .                      i1, j1, k1, l1, m1, n1, slen1, text1)

         IF ( slen1 .GT. 0 ) THEN

            status = STR_UPCASE(text1up,text1)

            DO n2=arg_lo_ss(F_AXIS,ARG2), arg_hi_ss(F_AXIS, ARG2)
            DO m2=arg_lo_ss(E_AXIS,ARG2), arg_hi_ss(E_AXIS, ARG2)
            DO l2=arg_lo_ss(T_AXIS,ARG2), arg_hi_ss(T_AXIS, ARG2)
            DO k2=arg_lo_ss(Z_AXIS,ARG2), arg_hi_ss(Z_AXIS, ARG2)
            DO j2=arg_lo_ss(Y_AXIS,ARG2), arg_hi_ss(Y_AXIS, ARG2)
            DO i2=arg_lo_ss(X_AXIS,ARG2), arg_hi_ss(X_AXIS, ARG2)

               arg = 2
               CALL ef_get_string_arg_element_6d(id, arg, arg_2, 
     .                            i2, j2, k2, l2, m2, n2, slen2, text2)

               IF ( slen1 .EQ. slen2 ) THEN

                  IF ( text1(:slen1) .EQ. text2(:slen2) ) THEN
C Found an exact match; done
                     result(i,j,k,l,m,n) = 1.
                     RETURN
                  ENDIF
                  status = STR_UPCASE(text2up,text2)
                  IF ( text1up(:slen1) .EQ. text2up(:slen2) ) THEN
C Case-insensitive match found; mark it but continue searching for an exact match
                     result(i,j,k,l,m,n) = 2.
                  ENDIF

               ENDIF

            ENDDO
            ENDDO
            ENDDO
            ENDDO
            ENDDO
            ENDDO

         ENDIF

      ENDDO
      ENDDO
      ENDDO
      ENDDO
      ENDDO
      ENDDO

C Either no match or case-insensitive match found; already marked appropriately
      RETURN

      END

*                                                                     ^
*                                                                     |
*                                           USER CONFIGURABLE PORTION |
***********************************************************************
