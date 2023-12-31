*
* samplej_multi.F
*
* Ansley Manke
* May 1998
*
* This function samples data along the J axis using a set of indices
* 4/5/99  Ansley Manke 
*         Indices may be oriented along any axis; 
*         Result is abstract on the sampled axis.
* 4/19  Reverse the order of arguments to be consistent with the
*       SAMPLEL etc functions.
* 4/19  See issue 1924: reworking sample-multi functions
* 4/12/19 See issue 1924: both args must have axis in the sample direction.

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
      SUBROUTINE samplej_multi_init(id)

      IMPLICIT NONE
      INCLUDE 'EF_Util.cmn'

      INTEGER id, arg


      CHARACTER*100 fcn_desc
      WRITE (fcn_desc, 10)
   10 FORMAT ('Returns data sampled according to J indices which ',
     .        'may vary in IKLMN')
      CALL ef_set_desc(id, fcn_desc)

      CALL ef_set_num_args(id, 2) 

      CALL ef_set_axis_inheritance_6d(id,
     .     IMPLIED_BY_ARGS, IMPLIED_BY_ARGS, IMPLIED_BY_ARGS, 
     .     IMPLIED_BY_ARGS, IMPLIED_BY_ARGS, IMPLIED_BY_ARGS)

      CALL ef_set_piecemeal_ok_6d(id, NO, NO, NO, NO, NO, NO)

      arg = 1
      CALL ef_set_arg_name(id, arg, 'DAT_TO_SAMPLE')
      CALL ef_set_arg_desc(id, arg, 'data to sample using J indices')
      CALL ef_set_axis_influence_6d(id, arg, YES, NO, YES, YES, YES, YES)

      arg = 2
      CALL ef_set_arg_name(id, arg, 'J_INDICES')
      CALL ef_set_arg_desc(id, arg, 'ordered indices')
      CALL ef_set_axis_influence_6d(id, arg, YES, YES, YES, YES, YES, YES)

      RETURN 
      END

* Compute the result
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
      SUBROUTINE samplej_multi_compute(id, arg_1, arg_2, result)

      IMPLICIT NONE
      INCLUDE 'EF_Util.cmn'
      INCLUDE 'EF_mem_subsc.cmn'

* Declarations
      INTEGER id

      REAL bad_flag(EF_MAX_ARGS), bad_flag_result
      REAL arg_1(mem1lox:mem1hix, mem1loy:mem1hiy, mem1loz:mem1hiz,
     .      mem1lot:mem1hit, mem1loe:mem1hie, mem1lof:mem1hif)
      REAL arg_2(mem2lox:mem2hix, mem2loy:mem2hiy, mem2loz:mem2hiz, 
     .     mem2lot:mem2hit, mem2loe:mem2hie, mem2lof:mem2hif)
      REAL result(memreslox:memreshix, memresloy:memreshiy, 
     .     memresloz:memreshiz, memreslot:memreshit,
     .      memresloe:memreshie, memreslof:memreshif)

      INTEGER res_lo_ss(6), res_hi_ss(6), res_incr(6)
      INTEGER arg_lo_ss(6,EF_MAX_ARGS), arg_hi_ss(6,EF_MAX_ARGS),
     .     arg_incr(6,EF_MAX_ARGS)

      INTEGER i, j, k, l, m, n
      INTEGER i1, j1, k1, l1, m1, n1
      INTEGER i2, j2, k2, l2, m2, n2
      INTEGER idim, p1, sampl_ax
      REAL aindex
      CHARACTER adir(6)*1, dir_string*12, errstr*100
      DATA adir /'X', 'Y', 'Z', 'T', 'E', 'F'/

      CALL ef_get_res_subscripts_6d(id, res_lo_ss, res_hi_ss, res_incr)
      CALL ef_get_arg_subscripts_6d(id, arg_lo_ss, arg_hi_ss, arg_incr)
      CALL ef_get_bad_flags(id, bad_flag, bad_flag_result)

* Both arguments must have an axis in the sample direction

      sampl_ax = Y_AXIS

      dir_string = adir(sampl_ax)
      IF (arg_hi_ss(sampl_ax, ARG1) .EQ. ef_unspecified_int4) GOTO 999
      IF (arg_hi_ss(sampl_ax, ARG2) .EQ. ef_unspecified_int4) GOTO 999

* Initialize the result
      result = bad_flag_result

* Now do the sampling

      n1 = arg_lo_ss(F_AXIS, ARG1)
      n2 = arg_lo_ss(F_AXIS, ARG2)
      DO 600 n = res_lo_ss(F_AXIS), res_hi_ss(F_AXIS) 
     
      m1 = arg_lo_ss(E_AXIS, ARG1)
      m2 = arg_lo_ss(E_AXIS, ARG2)
      DO 500 m = res_lo_ss(E_AXIS), res_hi_ss(E_AXIS) 

      l1 = arg_lo_ss(T_AXIS, ARG1)
      l2 = arg_lo_ss(T_AXIS, ARG2)
      DO 400 l = res_lo_ss(T_AXIS), res_hi_ss(T_AXIS) 

      k1 = arg_lo_ss(Z_AXIS, ARG1)
      k2 = arg_lo_ss(Z_AXIS, ARG2)
      DO 300 k = res_lo_ss(Z_AXIS), res_hi_ss(Z_AXIS)  

      i1 = arg_lo_ss(X_AXIS,ARG1)
      i2 = arg_lo_ss(X_AXIS,ARG2)
      DO 200 i = res_lo_ss(X_AXIS), res_hi_ss(X_AXIS)

      j2 = arg_lo_ss(Y_AXIS,ARG2)
      DO 100 j = res_lo_ss(Y_AXIS), res_hi_ss(Y_AXIS)

*  Get the index to sample.

         aindex = arg_2(i2,j2,k2,l2,m2,n2)
         p1 = aindex

* Set the result. Check that we do have an index of ARG_1.
* If the indices don't lie in range the result is missing.

	 aindex = arg_2(i2,j2,k2,l2,m2,n2)
         p1 = aindex

	 IF (aindex .EQ. bad_flag(ARG1))  CYCLE

	 IF ( p1.GE.arg_lo_ss(sampl_ax,ARG1) .AND. 
     .        p1.LE.arg_hi_ss(sampl_ax,ARG1) ) THEN
           
            IF (arg_1(i1,p1,k1,l1,m1,n1) .NE. bad_flag(ARG1)) 
     .        result(i,j,k,l,m,n) = arg_1(i1,p1,k1,l1,m1,n1)

	 ENDIF


         j2 = j2 + arg_incr(Y_AXIS,ARG2)
 100  CONTINUE

      i1 = i1 + arg_incr(X_AXIS,ARG1)
      i2 = i2 + arg_incr(X_AXIS,ARG2)
 200  CONTINUE

      k1 = k1 + arg_incr(Z_AXIS,ARG1)
      k2 = k2 + arg_incr(Z_AXIS,ARG2)
 300  CONTINUE

      l1 = l1 + arg_incr(T_AXIS,ARG1)
      l2 = l2 + arg_incr(T_AXIS,ARG2)
 400  CONTINUE

      m1 = m1 + arg_incr(E_AXIS,ARG1)
      m2 = m2 + arg_incr(E_AXIS,ARG2)
 500  CONTINUE

      n1 = n1 + arg_incr(F_AXIS,ARG1)
      n2 = n2 + arg_incr(F_AXIS,ARG2)
 600  CONTINUE

      RETURN

 999  CONTINUE
      errstr = 'Both arguments must have an axis in '//dir_string 
      CALL ef_bail_out(id, errstr)

      RETURN
      END
