      SUBROUTINE four_re (nd, x, a, b, wft)
      REAL x(*), wft(*) 
      REAL a(*), b(*), xn
      INTEGER nd, nf, i, j

c   uses NCAR FFTPACK code

C  From FOUR.F BY Jimmy Larsen
C  Version by Ansley Manke that replaces complex array C with A, B real arrays.
C  Uses notes by Ned Cokelet 1/2000 on Swartztrauber FFTPACK code.

c   Calls: RFFTF


C  NF = number of frequencies, half the number of times.
C  The code returns frequencies W(i) for i=0 to ND/2, with ND/2 rounded down.
C  We do not return a(0) = R1/ND
C  We return a(i) and b(i) for i=1,... ND/2  

      nf = nd/ 2

      CALL rfftf (nd, x, wft) 

C  Normalizing factor of 1./N

c      xn = 1.0
      xn = 1.0/ REAL(nd)

c   Save FFT coefficients in arrays a and b.
      
      j = 0
      DO i = 1, nf-1
        j = 2* i
        a(i) =  2.* xn* x(j)
        b(i) = -2.* xn* x(j+1) 
      ENDDO
      a(nf) =  2.* xn* x(2*nf)

C  Set a(nd/2) and b(nd/2) when nd is even

      IF (nf*2 .eq. nd) THEN
         a(nf) = xn* x(nd)
         b(nf) = 0.
      ENDIF
      
      RETURN 
      END

C     SUBROUTINE CFFTB(N,C,WSAVE)                                               
C                                                                               
C     SUBROUTINE CFFTB COMPUTES THE BACKWARD COMPLEX DISCRETE FOURIER           
C     TRANSFORM (THE FOURIER SYNTHESIS). EQUIVALENTLY , CFFTB COMPUTES          
C     A COMPLEX PERIODIC SEQUENCE FROM ITS FOURIER COEFFICIENTS.                
C     THE TRANSFORM IS DEFINED BELOW AT OUTPUT PARAMETER C.                     
C                                                                               
C     A CALL OF CFFTF FOLLOWED BY A CALL OF CFFTB WILL MULTIPLY THE             
C     SEQUENCE BY N.                                                            
C                                                                               
C     THE ARRAY WSAVE WHICH IS USED BY SUBROUTINE CFFTB MUST BE                 
C     INITIALIZED BY CALLING SUBROUTINE CFFTI(N,WSAVE).                         
C                                                                               
C     INPUT PARAMETERS                                                          
C                                                                               
C                                                                               
C     N      THE LENGTH OF THE COMPLEX SEQUENCE C. THE METHOD IS                
C            MORE EFFICIENT WHEN N IS THE PRODUCT OF SMALL PRIMES.              
C                                                                               
C     C      A COMPLEX ARRAY OF LENGTH N WHICH CONTAINS THE SEQUENCE            
C                                                                               
C     WSAVE   A REAL WORK ARRAY WHICH MUST BE DIMENSIONED AT LEAST 4N+15        
C             IN THE PROGRAM THAT CALLS CFFTB. THE WSAVE ARRAY MUST BE          
C             INITIALIZED BY CALLING SUBROUTINE CFFTI(N,WSAVE) AND A            
C             DIFFERENT WSAVE ARRAY MUST BE USED FOR EACH DIFFERENT             
C             VALUE OF N. THIS INITIALIZATION DOES NOT HAVE TO BE               
C             REPEATED SO LONG AS N REMAINS UNCHANGED THUS SUBSEQUENT           
C             TRANSFORMS CAN BE OBTAINED FASTER THAN THE FIRST.                 
C             THE SAME WSAVE ARRAY CAN BE USED BY CFFTF AND CFFTB.              
C                                                                               
C     OUTPUT PARAMETERS                                                         
C                                                                               
C     C      FOR J=1,...,N                                                      
C                                                                               
C                C(J)=THE SUM FROM K=1,...,N OF                                 
C                                                                               
C                      C(K)*EXP(I*(J-1)*(K-1)*2*PI/N)                           
C                                                                               
C                            WHERE I=SQRT(-1)                                   
C                                                                               
C     WSAVE   CONTAINS INITIALIZATION CALCULATIONS WHICH MUST NOT BE            
C             DESTROYED BETWEEN CALLS OF SUBROUTINE CFFTF OR CFFTB              
C
      SUBROUTINE CFFTB (N,C,WSAVE)                                              
      DIMENSION       C(*)       ,WSAVE(*)                                      
C                                                                               
      IF (N .EQ. 1) RETURN                                                      
      IW1 = N+N+1                                                               
      IW2 = IW1+N+N                                                             
      CALL CFFTB1 (N,C,WSAVE,WSAVE(IW1),WSAVE(IW2))                             
      RETURN                                                                    
      END                                                                       
      SUBROUTINE CFFTB1 (N,C,CH,WA,IFAC)                                        
      DIMENSION       CH(*)      ,C(*)       ,WA(*)      ,IFAC(*)               
      NF = IFAC(2)                                                              
      NA = 0                                                                    
      L1 = 1                                                                    
      IW = 1                                                                    
      DO 116 K1=1,NF                                                            
         IP = IFAC(K1+2)                                                        
         L2 = IP*L1                                                             
         IDO = N/L2                                                             
         IDOT = IDO+IDO                                                         
         IDL1 = IDOT*L1                                                         
         IF (IP .NE. 4) GO TO 103                                               
         IX2 = IW+IDOT                                                          
         IX3 = IX2+IDOT                                                         
         IF (NA .NE. 0) GO TO 101                                               
         CALL PASSB4 (IDOT,L1,C,CH,WA(IW),WA(IX2),WA(IX3))                      
         GO TO 102                                                              
  101    CALL PASSB4 (IDOT,L1,CH,C,WA(IW),WA(IX2),WA(IX3))                      
  102    NA = 1-NA                                                              
         GO TO 115                                                              
  103    IF (IP .NE. 2) GO TO 106                                               
         IF (NA .NE. 0) GO TO 104                                               
         CALL PASSB2 (IDOT,L1,C,CH,WA(IW))                                      
         GO TO 105                                                              
  104    CALL PASSB2 (IDOT,L1,CH,C,WA(IW))                                      
  105    NA = 1-NA                                                              
         GO TO 115                                                              
  106    IF (IP .NE. 3) GO TO 109                                               
         IX2 = IW+IDOT                                                          
         IF (NA .NE. 0) GO TO 107                                               
         CALL PASSB3 (IDOT,L1,C,CH,WA(IW),WA(IX2))                              
         GO TO 108                                                              
  107    CALL PASSB3 (IDOT,L1,CH,C,WA(IW),WA(IX2))                              
  108    NA = 1-NA                                                              
         GO TO 115                                                              
  109    IF (IP .NE. 5) GO TO 112                                               
         IX2 = IW+IDOT                                                          
         IX3 = IX2+IDOT                                                         
         IX4 = IX3+IDOT                                                         
         IF (NA .NE. 0) GO TO 110                                               
         CALL PASSB5 (IDOT,L1,C,CH,WA(IW),WA(IX2),WA(IX3),WA(IX4))              
         GO TO 111                                                              
  110    CALL PASSB5 (IDOT,L1,CH,C,WA(IW),WA(IX2),WA(IX3),WA(IX4))              
  111    NA = 1-NA                                                              
         GO TO 115                                                              
  112    IF (NA .NE. 0) GO TO 113                                               
         CALL PASSB (NAC,IDOT,IP,L1,IDL1,C,C,C,CH,CH,WA(IW))                    
         GO TO 114                                                              
  113    CALL PASSB (NAC,IDOT,IP,L1,IDL1,CH,CH,CH,C,C,WA(IW))                   
  114    IF (NAC .NE. 0) NA = 1-NA                                              
  115    L1 = L2                                                                
         IW = IW+(IP-1)*IDOT                                                    
  116 CONTINUE                                                                  
      IF (NA .EQ. 0) RETURN                                                     
      N2 = N+N                                                                  
      DO 117 I=1,N2                                                             
         C(I) = CH(I)                                                           
  117 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE CFFTF(N,C,WSAVE)                                               
C                                                                               
C     SUBROUTINE CFFTF COMPUTES THE FORWARD COMPLEX DISCRETE FOURIER            
C     TRANSFORM (THE FOURIER ANALYSIS). EQUIVALENTLY , CFFTF COMPUTES           
C     THE FOURIER COEFFICIENTS OF A COMPLEX PERIODIC SEQUENCE.                  
C     THE TRANSFORM IS DEFINED BELOW AT OUTPUT PARAMETER C.                     
C                                                                               
C     THE TRANSFORM IS NOT NORMALIZED. TO OBTAIN A NORMALIZED TRANSFORM         
C     THE OUTPUT MUST BE DIVIDED BY N. OTHERWISE A CALL OF CFFTF                
C     FOLLOWED BY A CALL OF CFFTB WILL MULTIPLY THE SEQUENCE BY N.              
C                                                                               
C     THE ARRAY WSAVE WHICH IS USED BY SUBROUTINE CFFTF MUST BE                 
C     INITIALIZED BY CALLING SUBROUTINE CFFTI(N,WSAVE).                         
C                                                                               
C     INPUT PARAMETERS                                                          
C                                                                               
C                                                                               
C     N      THE LENGTH OF THE COMPLEX SEQUENCE C. THE METHOD IS                
C            MORE EFFICIENT WHEN N IS THE PRODUCT OF SMALL PRIMES. N            
C                                                                               
C     C      A COMPLEX ARRAY OF LENGTH N WHICH CONTAINS THE SEQUENCE            
C                                                                               
C     WSAVE   A REAL WORK ARRAY WHICH MUST BE DIMENSIONED AT LEAST 4N+15        
C             IN THE PROGRAM THAT CALLS CFFTF. THE WSAVE ARRAY MUST BE          
C             INITIALIZED BY CALLING SUBROUTINE CFFTI(N,WSAVE) AND A            
C             DIFFERENT WSAVE ARRAY MUST BE USED FOR EACH DIFFERENT             
C             VALUE OF N. THIS INITIALIZATION DOES NOT HAVE TO BE               
C             REPEATED SO LONG AS N REMAINS UNCHANGED THUS SUBSEQUENT           
C             TRANSFORMS CAN BE OBTAINED FASTER THAN THE FIRST.                 
C             THE SAME WSAVE ARRAY CAN BE USED BY CFFTF AND CFFTB.              
C                                                                               
C     OUTPUT PARAMETERS                                                         
C                                                                               
C     C      FOR J=1,...,N                                                      
C                                                                               
C                C(J)=THE SUM FROM K=1,...,N OF                                 
C                                                                               
C                      C(K)*EXP(-I*(J-1)*(K-1)*2*PI/N)                          
C                                                                               
C                            WHERE I=SQRT(-1)                                   
C                                                                               
C     WSAVE   CONTAINS INITIALIZATION CALCULATIONS WHICH MUST NOT BE            
C             DESTROYED BETWEEN CALLS OF SUBROUTINE CFFTF OR CFFTB              
C                                                                               
      SUBROUTINE CFFTF (N,C,WSAVE)                                              
      DIMENSION       C(*)       ,WSAVE(*)                                      
C                                                                               
      IF (N .EQ. 1) RETURN                                                      
      IW1 = N+N+1                                                               
      IW2 = IW1+N+N                                                             
      CALL CFFTF1 (N,C,WSAVE,WSAVE(IW1),WSAVE(IW2))                             
      RETURN                                                                    
      END                                                                       
      SUBROUTINE CFFTF1 (N,C,CH,WA,IFAC)                                        
      DIMENSION       CH(*)      ,C(*)       ,WA(*)      ,IFAC(*)               
      NF = IFAC(2)                                                              
      NA = 0                                                                    
      L1 = 1                                                                    
      IW = 1                                                                    
      DO 116 K1=1,NF                                                            
         IP = IFAC(K1+2)                                                        
         L2 = IP*L1                                                             
         IDO = N/L2                                                             
         IDOT = IDO+IDO                                                         
         IDL1 = IDOT*L1                                                         
         IF (IP .NE. 4) GO TO 103                                               
         IX2 = IW+IDOT                                                          
         IX3 = IX2+IDOT                                                         
         IF (NA .NE. 0) GO TO 101                                               
         CALL PASSF4 (IDOT,L1,C,CH,WA(IW),WA(IX2),WA(IX3))                      
         GO TO 102                                                              
  101    CALL PASSF4 (IDOT,L1,CH,C,WA(IW),WA(IX2),WA(IX3))                      
  102    NA = 1-NA                                                              
         GO TO 115                                                              
  103    IF (IP .NE. 2) GO TO 106                                               
         IF (NA .NE. 0) GO TO 104                                               
         CALL PASSF2 (IDOT,L1,C,CH,WA(IW))                                      
         GO TO 105                                                              
  104    CALL PASSF2 (IDOT,L1,CH,C,WA(IW))                                      
  105    NA = 1-NA                                                              
         GO TO 115                                                              
  106    IF (IP .NE. 3) GO TO 109                                               
         IX2 = IW+IDOT                                                          
         IF (NA .NE. 0) GO TO 107                                               
         CALL PASSF3 (IDOT,L1,C,CH,WA(IW),WA(IX2))                              
         GO TO 108                                                              
  107    CALL PASSF3 (IDOT,L1,CH,C,WA(IW),WA(IX2))                              
  108    NA = 1-NA                                                              
         GO TO 115                                                              
  109    IF (IP .NE. 5) GO TO 112                                               
         IX2 = IW+IDOT                                                          
         IX3 = IX2+IDOT                                                         
         IX4 = IX3+IDOT                                                         
         IF (NA .NE. 0) GO TO 110                                               
         CALL PASSF5 (IDOT,L1,C,CH,WA(IW),WA(IX2),WA(IX3),WA(IX4))              
         GO TO 111                                                              
  110    CALL PASSF5 (IDOT,L1,CH,C,WA(IW),WA(IX2),WA(IX3),WA(IX4))              
  111    NA = 1-NA                                                              
         GO TO 115                                                              
  112    IF (NA .NE. 0) GO TO 113                                               
         CALL PASSF (NAC,IDOT,IP,L1,IDL1,C,C,C,CH,CH,WA(IW))                    
         GO TO 114                                                              
  113    CALL PASSF (NAC,IDOT,IP,L1,IDL1,CH,CH,CH,C,C,WA(IW))                   
  114    IF (NAC .NE. 0) NA = 1-NA                                              
  115    L1 = L2                                                                
         IW = IW+(IP-1)*IDOT                                                    
  116 CONTINUE                                                                  
      IF (NA .EQ. 0) RETURN                                                     
      N2 = N+N                                                                  
      DO 117 I=1,N2                                                             
         C(I) = CH(I)                                                           
  117 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE CFFTI(N,WSAVE)                                                 
C                                                                               
C     SUBROUTINE CFFTI INITIALIZES THE ARRAY WSAVE WHICH IS USED IN             
C     BOTH CFFTF AND CFFTB. THE PRIME FACTORIZATION OF N TOGETHER WITH          
C     A TABULATION OF THE TRIGONOMETRIC FUNCTIONS ARE COMPUTED AND              
C     STORED IN WSAVE.                                                          
C                                                                               
C     INPUT PARAMETER                                                           
C                                                                               
C     N       THE LENGTH OF THE SEQUENCE TO BE TRANSFORMED                      
C                                                                               
C     OUTPUT PARAMETER                                                          
C                                                                               
C     WSAVE   A WORK ARRAY WHICH MUST BE DIMENSIONED AT LEAST 4*N+15            
C             THE SAME WORK ARRAY CAN BE USED FOR BOTH CFFTF AND CFFTB          
C             AS LONG AS N REMAINS UNCHANGED. DIFFERENT WSAVE ARRAYS            
C             ARE REQUIRED FOR DIFFERENT VALUES OF N. THE CONTENTS OF           
C             WSAVE MUST NOT BE CHANGED BETWEEN CALLS OF CFFTF OR CFFTB.        
C                                                                               
      SUBROUTINE CFFTI (N,WSAVE)                                                
      DIMENSION       WSAVE(*)                                                  
C                                                                               
      IF (N .EQ. 1) RETURN                                                      
      IW1 = N+N+1                                                               
      IW2 = IW1+N+N                                                             
      CALL CFFTI1 (N,WSAVE(IW1),WSAVE(IW2))                                     
      RETURN                                                                    
      END                                                                       
      SUBROUTINE CFFTI1 (N,WA,IFAC)                                             
      DIMENSION       WA(*)      ,IFAC(*)    ,NTRYH(4)                          
      DATA NTRYH(1),NTRYH(2),NTRYH(3),NTRYH(4)/3,4,2,5/                         
      NL = N                                                                    
      NF = 0                                                                    
      J = 0                                                                     
  101 J = J+1                                                                   
      IF (J-4) 102,102,103                                                      
  102 NTRY = NTRYH(J)                                                           
      GO TO 104                                                                 
  103 NTRY = NTRY+2                                                             
  104 NQ = NL/NTRY                                                              
      NR = NL-NTRY*NQ                                                           
      IF (NR) 101,105,101                                                       
  105 NF = NF+1                                                                 
      IFAC(NF+2) = NTRY                                                         
      NL = NQ                                                                   
      IF (NTRY .NE. 2) GO TO 107                                                
      IF (NF .EQ. 1) GO TO 107                                                  
      DO 106 I=2,NF                                                             
         IB = NF-I+2                                                            
         IFAC(IB+2) = IFAC(IB+1)                                                
  106 CONTINUE                                                                  
      IFAC(3) = 2                                                               
  107 IF (NL .NE. 1) GO TO 104                                                  
      IFAC(1) = N                                                               
      IFAC(2) = NF                                                              
      TPI = 2.*PIMACH(DUM)                                                      
      ARGH = TPI/FLOAT(N)                                                       
      I = 2                                                                     
      L1 = 1                                                                    
      DO 110 K1=1,NF                                                            
         IP = IFAC(K1+2)                                                        
         LD = 0                                                                 
         L2 = L1*IP                                                             
         IDO = N/L2                                                             
         IDOT = IDO+IDO+2                                                       
         IPM = IP-1                                                             
         DO 109 J=1,IPM                                                         
            I1 = I                                                              
            WA(I-1) = 1.                                                        
            WA(I) = 0.                                                          
            LD = LD+L1                                                          
            FI = 0.                                                             
            ARGLD = FLOAT(LD)*ARGH                                              
            DO 108 II=4,IDOT,2                                                  
               I = I+2                                                          
               FI = FI+1.                                                       
               ARG = FI*ARGLD                                                   
               WA(I-1) = COS(ARG)                                               
               WA(I) = SIN(ARG)                                                 
  108       CONTINUE                                                            
            IF (IP .LE. 5) GO TO 109                                            
            WA(I1-1) = WA(I-1)                                                  
            WA(I1) = WA(I)                                                      
  109    CONTINUE                                                               
         L1 = L2                                                                
  110 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE COSQB(N,X,WSAVE)                                               
C                                                                               
C     SUBROUTINE COSQB COMPUTES THE FAST FOURIER TRANSFORM OF QUARTER           
C     WAVE DATA. THAT IS , COSQB COMPUTES A SEQUENCE FROM ITS                   
C     REPRESENTATION IN TERMS OF A COSINE SERIES WITH ODD WAVE NUMBERS.         
C     THE TRANSFORM IS DEFINED BELOW AT OUTPUT PARAMETER X.                     
C                                                                               
C     COSQB IS THE UNNORMALIZED INVERSE OF COSQF SINCE A CALL OF COSQB          
C     FOLLOWED BY A CALL OF COSQF WILL MULTIPLY THE INPUT SEQUENCE X            
C     BY 4*N.                                                                   
C                                                                               
C     THE ARRAY WSAVE WHICH IS USED BY SUBROUTINE COSQB MUST BE                 
C     INITIALIZED BY CALLING SUBROUTINE COSQI(N,WSAVE).                         
C                                                                               
C                                                                               
C     INPUT PARAMETERS                                                          
C                                                                               
C     N       THE LENGTH OF THE ARRAY X TO BE TRANSFORMED.  THE METHOD          
C             IS MOST EFFICIENT WHEN N IS A PRODUCT OF SMALL PRIMES.            
C                                                                               
C     X       AN ARRAY WHICH CONTAINS THE SEQUENCE TO BE TRANSFORMED            
C                                                                               
C     WSAVE   A WORK ARRAY THAT MUST BE DIMENSIONED AT LEAST 3*N+15             
C             IN THE PROGRAM THAT CALLS COSQB. THE WSAVE ARRAY MUST BE          
C             INITIALIZED BY CALLING SUBROUTINE COSQI(N,WSAVE) AND A            
C             DIFFERENT WSAVE ARRAY MUST BE USED FOR EACH DIFFERENT             
C             VALUE OF N. THIS INITIALIZATION DOES NOT HAVE TO BE               
C             REPEATED SO LONG AS N REMAINS UNCHANGED THUS SUBSEQUENT           
C             TRANSFORMS CAN BE OBTAINED FASTER THAN THE FIRST.                 
C                                                                               
C     OUTPUT PARAMETERS                                                         
C                                                                               
C     X       FOR I=1,...,N                                                     
C                                                                               
C                  X(I)= THE SUM FROM K=1 TO K=N OF                             
C                                                                               
C                    4*X(K)*COS((2*K-1)*(I-1)*PI/(2*N))                         
C                                                                               
C                  A CALL OF COSQB FOLLOWED BY A CALL OF                        
C                  COSQF WILL MULTIPLY THE SEQUENCE X BY 4*N.                   
C                  THEREFORE COSQF IS THE UNNORMALIZED INVERSE                  
C                  OF COSQB.                                                    
C                                                                               
C     WSAVE   CONTAINS INITIALIZATION CALCULATIONS WHICH MUST NOT               
C             BE DESTROYED BETWEEN CALLS OF COSQB OR COSQF.                     
C                                                                               
      SUBROUTINE COSQB (N,X,WSAVE)                                              
      DIMENSION       X(*)       ,WSAVE(*)                                      
      DATA TSQRT2 /2.82842712474619/                                            
C                                                                               
      IF (N-2) 101,102,103                                                      
  101 X(1) = 4.*X(1)                                                            
      RETURN                                                                    
  102 X1 = 4.*(X(1)+X(2))                                                       
      X(2) = TSQRT2*(X(1)-X(2))                                                 
      X(1) = X1                                                                 
      RETURN                                                                    
  103 CALL COSQB1 (N,X,WSAVE,WSAVE(N+1))                                        
      RETURN                                                                    
      END                                                                       
      SUBROUTINE COSQB1 (N,X,W,XH)                                              
      DIMENSION       X(*)       ,W(*)       ,XH(*)                             
      NS2 = (N+1)/2                                                             
      NP2 = N+2                                                                 
      DO 101 I=3,N,2                                                            
         XIM1 = X(I-1)+X(I)                                                     
         X(I) = X(I)-X(I-1)                                                     
         X(I-1) = XIM1                                                          
  101 CONTINUE                                                                  
      X(1) = X(1)+X(1)                                                          
      MODN = MOD(N,2)                                                           
      IF (MODN .EQ. 0) X(N) = X(N)+X(N)                                         
      CALL RFFTB (N,X,XH)                                                       
      DO 102 K=2,NS2                                                            
         KC = NP2-K                                                             
         XH(K) = W(K-1)*X(KC)+W(KC-1)*X(K)                                      
         XH(KC) = W(K-1)*X(K)-W(KC-1)*X(KC)                                     
  102 CONTINUE                                                                  
      IF (MODN .EQ. 0) X(NS2+1) = W(NS2)*(X(NS2+1)+X(NS2+1))                    
      DO 103 K=2,NS2                                                            
         KC = NP2-K                                                             
         X(K) = XH(K)+XH(KC)                                                    
         X(KC) = XH(K)-XH(KC)                                                   
  103 CONTINUE                                                                  
      X(1) = X(1)+X(1)                                                          
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE COSQF(N,X,WSAVE)                                               
C                                                                               
C     SUBROUTINE COSQF COMPUTES THE FAST FOURIER TRANSFORM OF QUARTER           
C     WAVE DATA. THAT IS , COSQF COMPUTES THE COEFFICIENTS IN A COSINE          
C     SERIES REPRESENTATION WITH ONLY ODD WAVE NUMBERS. THE TRANSFORM           
C     IS DEFINED BELOW AT OUTPUT PARAMETER X                                    
C                                                                               
C     COSQF IS THE UNNORMALIZED INVERSE OF COSQB SINCE A CALL OF COSQF          
C     FOLLOWED BY A CALL OF COSQB WILL MULTIPLY THE INPUT SEQUENCE X            
C     BY 4*N.                                                                   
C                                                                               
C     THE ARRAY WSAVE WHICH IS USED BY SUBROUTINE COSQF MUST BE                 
C     INITIALIZED BY CALLING SUBROUTINE COSQI(N,WSAVE).                         
C                                                                               
C                                                                               
C     INPUT PARAMETERS                                                          
C                                                                               
C     N       THE LENGTH OF THE ARRAY X TO BE TRANSFORMED.  THE METHOD          
C             IS MOST EFFICIENT WHEN N IS A PRODUCT OF SMALL PRIMES.            
C                                                                               
C     X       AN ARRAY WHICH CONTAINS THE SEQUENCE TO BE TRANSFORMED            
C                                                                               
C     WSAVE   A WORK ARRAY WHICH MUST BE DIMENSIONED AT LEAST 3*N+15            
C             IN THE PROGRAM THAT CALLS COSQF. THE WSAVE ARRAY MUST BE          
C             INITIALIZED BY CALLING SUBROUTINE COSQI(N,WSAVE) AND A            
C             DIFFERENT WSAVE ARRAY MUST BE USED FOR EACH DIFFERENT             
C             VALUE OF N. THIS INITIALIZATION DOES NOT HAVE TO BE               
C             REPEATED SO LONG AS N REMAINS UNCHANGED THUS SUBSEQUENT           
C             TRANSFORMS CAN BE OBTAINED FASTER THAN THE FIRST.                 
C                                                                               
C     OUTPUT PARAMETERS                                                         
C                                                                               
C     X       FOR I=1,...,N                                                     
C                                                                               
C                  X(I) = X(1) PLUS THE SUM FROM K=2 TO K=N OF                  
C                                                                               
C                     2*X(K)*COS((2*I-1)*(K-1)*PI/(2*N))                        
C                                                                               
C                  A CALL OF COSQF FOLLOWED BY A CALL OF                        
C                  COSQB WILL MULTIPLY THE SEQUENCE X BY 4*N.                   
C                  THEREFORE COSQB IS THE UNNORMALIZED INVERSE                  
C                  OF COSQF.                                                    
C                                                                               
C     WSAVE   CONTAINS INITIALIZATION CALCULATIONS WHICH MUST NOT               
C             BE DESTROYED BETWEEN CALLS OF COSQF OR COSQB.                     
C                                                                               
      SUBROUTINE COSQF (N,X,WSAVE)                                              
      DIMENSION       X(*)       ,WSAVE(*)                                      
      DATA SQRT2 /1.4142135623731/                                              
C                                                                               
      IF (N-2) 102,101,103                                                      
  101 TSQX = SQRT2*X(2)                                                         
      X(2) = X(1)-TSQX                                                          
      X(1) = X(1)+TSQX                                                          
  102 RETURN                                                                    
  103 CALL COSQF1 (N,X,WSAVE,WSAVE(N+1))                                        
      RETURN                                                                    
      END                                                                       
      SUBROUTINE COSQF1 (N,X,W,XH)                                              
      DIMENSION       X(*)       ,W(*)       ,XH(*)                             
      NS2 = (N+1)/2                                                             
      NP2 = N+2                                                                 
      DO 101 K=2,NS2                                                            
         KC = NP2-K                                                             
         XH(K) = X(K)+X(KC)                                                     
         XH(KC) = X(K)-X(KC)                                                    
  101 CONTINUE                                                                  
      MODN = MOD(N,2)                                                           
      IF (MODN .EQ. 0) XH(NS2+1) = X(NS2+1)+X(NS2+1)                            
      DO 102 K=2,NS2                                                            
         KC = NP2-K                                                             
         X(K) = W(K-1)*XH(KC)+W(KC-1)*XH(K)                                     
         X(KC) = W(K-1)*XH(K)-W(KC-1)*XH(KC)                                    
  102 CONTINUE                                                                  
      IF (MODN .EQ. 0) X(NS2+1) = W(NS2)*XH(NS2+1)                              
      CALL RFFTF (N,X,XH)                                                       
      DO 103 I=3,N,2                                                            
         XIM1 = X(I-1)-X(I)                                                     
         X(I) = X(I-1)+X(I)                                                     
         X(I-1) = XIM1                                                          
  103 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE COSQI(N,WSAVE)                                                 
C                                                                               
C     SUBROUTINE COSQI INITIALIZES THE ARRAY WSAVE WHICH IS USED IN             
C     BOTH COSQF AND COSQB. THE PRIME FACTORIZATION OF N TOGETHER WITH          
C     A TABULATION OF THE TRIGONOMETRIC FUNCTIONS ARE COMPUTED AND              
C     STORED IN WSAVE.                                                          
C                                                                               
C     INPUT PARAMETER                                                           
C                                                                               
C     N       THE LENGTH OF THE ARRAY TO BE TRANSFORMED.  THE METHOD            
C             IS MOST EFFICIENT WHEN N IS A PRODUCT OF SMALL PRIMES.            
C                                                                               
C     OUTPUT PARAMETER                                                          
C                                                                               
C     WSAVE   A WORK ARRAY WHICH MUST BE DIMENSIONED AT LEAST 3*N+15.           
C             THE SAME WORK ARRAY CAN BE USED FOR BOTH COSQF AND COSQB          
C             AS LONG AS N REMAINS UNCHANGED. DIFFERENT WSAVE ARRAYS            
C             ARE REQUIRED FOR DIFFERENT VALUES OF N. THE CONTENTS OF           
C             WSAVE MUST NOT BE CHANGED BETWEEN CALLS OF COSQF OR COSQB.        
C                                                                               
      SUBROUTINE COSQI (N,WSAVE)                                                
      DIMENSION       WSAVE(*)                                                  
C                                                                               
      PIH = 0.5*PIMACH(DUM)                                                     
      DT = PIH/FLOAT(N)                                                         
      FK = 0.                                                                   
      DO 101 K=1,N                                                              
         FK = FK+1.                                                             
         WSAVE(K) = COS(FK*DT)                                                  
  101 CONTINUE                                                                  
      CALL RFFTI (N,WSAVE(N+1))                                                 
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE COST(N,X,WSAVE)                                                
C                                                                               
C     SUBROUTINE COST COMPUTES THE DISCRETE FOURIER COSINE TRANSFORM            
C     OF AN EVEN SEQUENCE X(I). THE TRANSFORM IS DEFINED BELOW AT OUTPUT        
C     PARAMETER X.                                                              
C                                                                               
C     COST IS THE UNNORMALIZED INVERSE OF ITSELF SINCE A CALL OF COST           
C     FOLLOWED BY ANOTHER CALL OF COST WILL MULTIPLY THE INPUT SEQUENCE         
C     X BY 2*(N-1). THE TRANSFORM IS DEFINED BELOW AT OUTPUT PARAMETER X        
C                                                                               
C     THE ARRAY WSAVE WHICH IS USED BY SUBROUTINE COST MUST BE                  
C     INITIALIZED BY CALLING SUBROUTINE COSTI(N,WSAVE).                         
C                                                                               
C     INPUT PARAMETERS                                                          
C                                                                               
C     N       THE LENGTH OF THE SEQUENCE X. N MUST BE GREATER THAN 1.           
C             THE METHOD IS MOST EFFICIENT WHEN N-1 IS A PRODUCT OF             
C             SMALL PRIMES.                                                     
C                                                                               
C     X       AN ARRAY WHICH CONTAINS THE SEQUENCE TO BE TRANSFORMED            
C                                                                               
C     WSAVE   A WORK ARRAY WHICH MUST BE DIMENSIONED AT LEAST 3*N+15            
C             IN THE PROGRAM THAT CALLS COST. THE WSAVE ARRAY MUST BE           
C             INITIALIZED BY CALLING SUBROUTINE COSTI(N,WSAVE) AND A            
C             DIFFERENT WSAVE ARRAY MUST BE USED FOR EACH DIFFERENT             
C             VALUE OF N. THIS INITIALIZATION DOES NOT HAVE TO BE               
C             REPEATED SO LONG AS N REMAINS UNCHANGED THUS SUBSEQUENT           
C             TRANSFORMS CAN BE OBTAINED FASTER THAN THE FIRST.                 
C                                                                               
C     OUTPUT PARAMETERS                                                         
C                                                                               
C     X       FOR I=1,...,N                                                     
C                                                                               
C                 X(I) = X(1)+(-1)**(I-1)*X(N)                                  
C                                                                               
C                  + THE SUM FROM K=2 TO K=N-1                                  
C                                                                               
C                      2*X(K)*COS((K-1)*(I-1)*PI/(N-1))                         
C                                                                               
C                  A CALL OF COST FOLLOWED BY ANOTHER CALL OF                   
C                  COST WILL MULTIPLY THE SEQUENCE X BY 2*(N-1)                 
C                  HENCE COST IS THE UNNORMALIZED INVERSE                       
C                  OF ITSELF.                                                   
C                                                                               
C     WSAVE   CONTAINS INITIALIZATION CALCULATIONS WHICH MUST NOT BE            
C             DESTROYED BETWEEN CALLS OF COST.                                  
C                                                                               
      SUBROUTINE COST (N,X,WSAVE)                                               
      DIMENSION       X(*)       ,WSAVE(*)                                      
C                                                                               
      NM1 = N-1                                                                 
      NP1 = N+1                                                                 
      NS2 = N/2                                                                 
      IF (N-2) 106,101,102                                                      
  101 X1H = X(1)+X(2)                                                           
      X(2) = X(1)-X(2)                                                          
      X(1) = X1H                                                                
      RETURN                                                                    
  102 IF (N .GT. 3) GO TO 103                                                   
      X1P3 = X(1)+X(3)                                                          
      TX2 = X(2)+X(2)                                                           
      X(2) = X(1)-X(3)                                                          
      X(1) = X1P3+TX2                                                           
      X(3) = X1P3-TX2                                                           
      RETURN                                                                    
  103 C1 = X(1)-X(N)                                                            
      X(1) = X(1)+X(N)                                                          
      DO 104 K=2,NS2                                                            
         KC = NP1-K                                                             
         T1 = X(K)+X(KC)                                                        
         T2 = X(K)-X(KC)                                                        
         C1 = C1+WSAVE(KC)*T2                                                   
         T2 = WSAVE(K)*T2                                                       
         X(K) = T1-T2                                                           
         X(KC) = T1+T2                                                          
  104 CONTINUE                                                                  
      MODN = MOD(N,2)                                                           
      IF (MODN .NE. 0) X(NS2+1) = X(NS2+1)+X(NS2+1)                             
      CALL RFFTF (NM1,X,WSAVE(N+1))                                             
      XIM2 = X(2)                                                               
      X(2) = C1                                                                 
      DO 105 I=4,N,2                                                            
         XI = X(I)                                                              
         X(I) = X(I-2)-X(I-1)                                                   
         X(I-1) = XIM2                                                          
         XIM2 = XI                                                              
  105 CONTINUE                                                                  
      IF (MODN .NE. 0) X(N) = XIM2                                              
  106 RETURN                                                                    
      END                                                                       
C     SUBROUTINE COSTI(N,WSAVE)                                                 
C                                                                               
C     SUBROUTINE COSTI INITIALIZES THE ARRAY WSAVE WHICH IS USED IN             
C     SUBROUTINE COST. THE PRIME FACTORIZATION OF N TOGETHER WITH               
C     A TABULATION OF THE TRIGONOMETRIC FUNCTIONS ARE COMPUTED AND              
C     STORED IN WSAVE.                                                          
C                                                                               
C     INPUT PARAMETER                                                           
C                                                                               
C     N       THE LENGTH OF THE SEQUENCE TO BE TRANSFORMED.  THE METHOD         
C             IS MOST EFFICIENT WHEN N-1 IS A PRODUCT OF SMALL PRIMES.          
C                                                                               
C     OUTPUT PARAMETER                                                          
C                                                                               
C     WSAVE   A WORK ARRAY WHICH MUST BE DIMENSIONED AT LEAST 3*N+15.           
C             DIFFERENT WSAVE ARRAYS ARE REQUIRED FOR DIFFERENT VALUES          
C             OF N. THE CONTENTS OF WSAVE MUST NOT BE CHANGED BETWEEN           
C             CALLS OF COST.                                                    
C                                                                               
      SUBROUTINE COSTI (N,WSAVE)                                                
      DIMENSION       WSAVE(*)                                                  
C                                                                               
      PI = PIMACH(DUM)                                                          
      IF (N .LE. 3) RETURN                                                      
      NM1 = N-1                                                                 
      NP1 = N+1                                                                 
      NS2 = N/2                                                                 
      DT = PI/FLOAT(NM1)                                                        
      FK = 0.                                                                   
      DO 101 K=2,NS2                                                            
         KC = NP1-K                                                             
         FK = FK+1.                                                             
         WSAVE(K) = 2.*SIN(FK*DT)                                               
         WSAVE(KC) = 2.*COS(FK*DT)                                              
  101 CONTINUE                                                                  
      CALL RFFTI (NM1,WSAVE(N+1))                                               
      RETURN                                                                    
      END                                                                       
      SUBROUTINE EZFFT1 (N,WA,IFAC)                                             
      DIMENSION       WA(*)      ,IFAC(*)    ,NTRYH(4)                          
      DATA NTRYH(1),NTRYH(2),NTRYH(3),NTRYH(4)/4,2,3,5/                         
      TPI = 2.0*PIMACH(DUM)                                                     
      NL = N                                                                    
      NF = 0                                                                    
      J = 0                                                                     
  101 J = J+1                                                                   
      IF (J-4) 102,102,103                                                      
  102 NTRY = NTRYH(J)                                                           
      GO TO 104                                                                 
  103 NTRY = NTRY+2                                                             
  104 NQ = NL/NTRY                                                              
      NR = NL-NTRY*NQ                                                           
      IF (NR) 101,105,101                                                       
  105 NF = NF+1                                                                 
      IFAC(NF+2) = NTRY                                                         
      NL = NQ                                                                   
      IF (NTRY .NE. 2) GO TO 107                                                
      IF (NF .EQ. 1) GO TO 107                                                  
      DO 106 I=2,NF                                                             
         IB = NF-I+2                                                            
         IFAC(IB+2) = IFAC(IB+1)                                                
  106 CONTINUE                                                                  
      IFAC(3) = 2                                                               
  107 IF (NL .NE. 1) GO TO 104                                                  
      IFAC(1) = N                                                               
      IFAC(2) = NF                                                              
      ARGH = TPI/FLOAT(N)                                                       
      IS = 0                                                                    
      NFM1 = NF-1                                                               
      L1 = 1                                                                    
      IF (NFM1 .EQ. 0) RETURN                                                   
      DO 111 K1=1,NFM1                                                          
         IP = IFAC(K1+2)                                                        
         L2 = L1*IP                                                             
         IDO = N/L2                                                             
         IPM = IP-1                                                             
         ARG1 = FLOAT(L1)*ARGH                                                  
         CH1 = 1.                                                               
         SH1 = 0.                                                               
         DCH1 = COS(ARG1)                                                       
         DSH1 = SIN(ARG1)                                                       
         DO 110 J=1,IPM                                                         
            CH1H = DCH1*CH1-DSH1*SH1                                            
            SH1 = DCH1*SH1+DSH1*CH1                                             
            CH1 = CH1H                                                          
            I = IS+2                                                            
            WA(I-1) = CH1                                                       
            WA(I) = SH1                                                         
            IF (IDO .LT. 5) GO TO 109                                           
            DO 108 II=5,IDO,2                                                   
               I = I+2                                                          
               WA(I-1) = CH1*WA(I-3)-SH1*WA(I-2)                                
               WA(I) = CH1*WA(I-2)+SH1*WA(I-3)                                  
  108       CONTINUE                                                            
  109       IS = IS+IDO                                                         
  110    CONTINUE                                                               
         L1 = L2                                                                
  111 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE EZFFTB(N,R,AZERO,A,B,WSAVE)                                    
C                                                                               
C     SUBROUTINE EZFFTB COMPUTES A REAL PERODIC SEQUENCE FROM ITS               
C     FOURIER COEFFICIENTS (FOURIER SYNTHESIS). THE TRANSFORM IS                
C     DEFINED BELOW AT OUTPUT PARAMETER R. EZFFTB IS A SIMPLIFIED               
C     BUT SLOWER VERSION OF RFFTB.                                              
C                                                                               
C     INPUT PARAMETERS                                                          
C                                                                               
C     N       THE LENGTH OF THE OUTPUT ARRAY R.  THE METHOD IS MOST             
C             EFFICIENT WHEN N IS THE PRODUCT OF SMALL PRIMES.                  
C                                                                               
C     AZERO   THE CONSTANT FOURIER COEFFICIENT                                  
C                                                                               
C     A,B     ARRAYS WHICH CONTAIN THE REMAINING FOURIER COEFFICIENTS           
C             THESE ARRAYS ARE NOT DESTROYED.                                   
C                                                                               
C             THE LENGTH OF THESE ARRAYS DEPENDS ON WHETHER N IS EVEN OR        
C             ODD.                                                              
C                                                                               
C             IF N IS EVEN N/2    LOCATIONS ARE REQUIRED                        
C             IF N IS ODD (N-1)/2 LOCATIONS ARE REQUIRED                        
C                                                                               
C     WSAVE   A WORK ARRAY WHICH MUST BE DIMENSIONED AT LEAST 3*N+15.           
C             IN THE PROGRAM THAT CALLS EZFFTB. THE WSAVE ARRAY MUST BE         
C             INITIALIZED BY CALLING SUBROUTINE EZFFTI(N,WSAVE) AND A           
C             DIFFERENT WSAVE ARRAY MUST BE USED FOR EACH DIFFERENT             
C             VALUE OF N. THIS INITIALIZATION DOES NOT HAVE TO BE               
C             REPEATED SO LONG AS N REMAINS UNCHANGED THUS SUBSEQUENT           
C             TRANSFORMS CAN BE OBTAINED FASTER THAN THE FIRST.                 
C             THE SAME WSAVE ARRAY CAN BE USED BY EZFFTF AND EZFFTB.            
C                                                                               
C                                                                               
C     OUTPUT PARAMETERS                                                         
C                                                                               
C     R       IF N IS EVEN DEFINE KMAX=N/2                                      
C             IF N IS ODD  DEFINE KMAX=(N-1)/2                                  
C                                                                               
C             THEN FOR I=1,...,N                                                
C                                                                               
C                  R(I)=AZERO PLUS THE SUM FROM K=1 TO K=KMAX OF                
C                                                                               
C                  A(K)*COS(K*(I-1)*2*PI/N)+B(K)*SIN(K*(I-1)*2*PI/N)            
C                                                                               
C     ********************* COMPLEX NOTATION **************************         
C                                                                               
C             FOR J=1,...,N                                                     
C                                                                               
C             R(J) EQUALS THE SUM FROM K=-KMAX TO K=KMAX OF                     
C                                                                               
C                  C(K)*EXP(I*K*(J-1)*2*PI/N)                                   
C                                                                               
C             WHERE                                                             
C                                                                               
C                  C(K) = .5*CMPLX(A(K),-B(K))   FOR K=1,...,KMAX               
C                                                                               
C                  C(-K) = CONJG(C(K))                                          
C                                                                               
C                  C(0) = AZERO                                                 
C                                                                               
C                       AND I=SQRT(-1)                                          
C                                                                               
C     *************** AMPLITUDE - PHASE NOTATION ***********************        
C                                                                               
C             FOR I=1,...,N                                                     
C                                                                               
C             R(I) EQUALS AZERO PLUS THE SUM FROM K=1 TO K=KMAX OF              
C                                                                               
C                  ALPHA(K)*COS(K*(I-1)*2*PI/N+BETA(K))                         
C                                                                               
C             WHERE                                                             
C                                                                               
C                  ALPHA(K) = SQRT(A(K)*A(K)+B(K)*B(K))                         
C                                                                               
C                  COS(BETA(K))=A(K)/ALPHA(K)                                   
C                                                                               
C                  SIN(BETA(K))=-B(K)/ALPHA(K)                                  
C                                                                               
      SUBROUTINE EZFFTB (N,R,AZERO,A,B,WSAVE)                                   
      DIMENSION       R(*)       ,A(*)       ,B(*)       ,WSAVE(*)              
C                                                                               
      IF (N-2) 101,102,103                                                      
  101 R(1) = AZERO                                                              
      RETURN                                                                    
  102 R(1) = AZERO+A(1)                                                         
      R(2) = AZERO-A(1)                                                         
      RETURN                                                                    
  103 NS2 = (N-1)/2                                                             
      DO 104 I=1,NS2                                                            
         R(2*I) = .5*A(I)                                                       
         R(2*I+1) = -.5*B(I)                                                    
  104 CONTINUE                                                                  
      R(1) = AZERO                                                              
      IF (MOD(N,2) .EQ. 0) R(N) = A(NS2+1)                                      
      CALL RFFTB (N,R,WSAVE(N+1))                                               
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE EZFFTF(N,R,AZERO,A,B,WSAVE)                                    
C                                                                               
C     SUBROUTINE EZFFTF COMPUTES THE FOURIER COEFFICIENTS OF A REAL             
C     PERODIC SEQUENCE (FOURIER ANALYSIS). THE TRANSFORM IS DEFINED             
C     BELOW AT OUTPUT PARAMETERS AZERO,A AND B. EZFFTF IS A SIMPLIFIED          
C     BUT SLOWER VERSION OF RFFTF.                                              
C                                                                               
C     INPUT PARAMETERS                                                          
C                                                                               
C     N       THE LENGTH OF THE ARRAY R TO BE TRANSFORMED.  THE METHOD          
C             IS MUST EFFICIENT WHEN N IS THE PRODUCT OF SMALL PRIMES.          
C                                                                               
C     R       A REAL ARRAY OF LENGTH N WHICH CONTAINS THE SEQUENCE              
C             TO BE TRANSFORMED. R IS NOT DESTROYED.                            
C                                                                               
C                                                                               
C     WSAVE   A WORK ARRAY WHICH MUST BE DIMENSIONED AT LEAST 3*N+15.           
C             IN THE PROGRAM THAT CALLS EZFFTF. THE WSAVE ARRAY MUST BE         
C             INITIALIZED BY CALLING SUBROUTINE EZFFTI(N,WSAVE) AND A           
C             DIFFERENT WSAVE ARRAY MUST BE USED FOR EACH DIFFERENT             
C             VALUE OF N. THIS INITIALIZATION DOES NOT HAVE TO BE               
C             REPEATED SO LONG AS N REMAINS UNCHANGED THUS SUBSEQUENT           
C             TRANSFORMS CAN BE OBTAINED FASTER THAN THE FIRST.                 
C             THE SAME WSAVE ARRAY CAN BE USED BY EZFFTF AND EZFFTB.            
C                                                                               
C     OUTPUT PARAMETERS                                                         
C                                                                               
C     AZERO   THE SUM FROM I=1 TO I=N OF R(I)/N                                 
C                                                                               
C     A,B     FOR N EVEN B(N/2)=0. AND A(N/2) IS THE SUM FROM I=1 TO            
C             I=N OF (-1)**(I-1)*R(I)/N                                         
C                                                                               
C             FOR N EVEN DEFINE KMAX=N/2-1                                      
C             FOR N ODD  DEFINE KMAX=(N-1)/2                                    
C                                                                               
C             THEN FOR  K=1,...,KMAX                                            
C                                                                               
C                  A(K) EQUALS THE SUM FROM I=1 TO I=N OF                       
C                                                                               
C                       2./N*R(I)*COS(K*(I-1)*2*PI/N)                           
C                                                                               
C                  B(K) EQUALS THE SUM FROM I=1 TO I=N OF                       
C                                                                               
C                       2./N*R(I)*SIN(K*(I-1)*2*PI/N)                           
C                                                                               
C                                                                               
      SUBROUTINE EZFFTF (N,R,AZERO,A,B,WSAVE)                                   
      DIMENSION       R(*)       ,A(*)       ,B(*)       ,WSAVE(*)              
C                                                                               
      IF (N-2) 101,102,103                                                      
  101 AZERO = R(1)                                                              
      RETURN                                                                    
  102 AZERO = .5*(R(1)+R(2))                                                    
      A(1) = .5*(R(1)-R(2))                                                     
      RETURN                                                                    
  103 DO 104 I=1,N                                                              
         WSAVE(I) = R(I)                                                        
  104 CONTINUE                                                                  
      CALL RFFTF (N,WSAVE,WSAVE(N+1))                                           
      CF = 2./FLOAT(N)                                                          
      CFM = -CF                                                                 
      AZERO = .5*CF*WSAVE(1)                                                    
      NS2 = (N+1)/2                                                             
      NS2M = NS2-1                                                              
      DO 105 I=1,NS2M                                                           
         A(I) = CF*WSAVE(2*I)                                                   
         B(I) = CFM*WSAVE(2*I+1)                                                
  105 CONTINUE                                                                  
      IF (MOD(N,2) .EQ. 1) RETURN                                               
      A(NS2) = .5*CF*WSAVE(N)                                                   
      B(NS2) = 0.                                                               
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE EZFFTI(N,WSAVE)                                                
C                                                                               
C     SUBROUTINE EZFFTI INITIALIZES THE ARRAY WSAVE WHICH IS USED IN            
C     BOTH EZFFTF AND EZFFTB. THE PRIME FACTORIZATION OF N TOGETHER WITH        
C     A TABULATION OF THE TRIGONOMETRIC FUNCTIONS ARE COMPUTED AND              
C     STORED IN WSAVE.                                                          
C                                                                               
C     INPUT PARAMETER                                                           
C                                                                               
C     N       THE LENGTH OF THE SEQUENCE TO BE TRANSFORMED.                     
C                                                                               
C     OUTPUT PARAMETER                                                          
C                                                                               
C     WSAVE   A WORK ARRAY WHICH MUST BE DIMENSIONED AT LEAST 3*N+15.           
C             THE SAME WORK ARRAY CAN BE USED FOR BOTH EZFFTF AND EZFFTB        
C             AS LONG AS N REMAINS UNCHANGED. DIFFERENT WSAVE ARRAYS            
C             ARE REQUIRED FOR DIFFERENT VALUES OF N.                           
C                                                                               
      SUBROUTINE EZFFTI (N,WSAVE)                                               
      DIMENSION       WSAVE(*)                                                  
C                                                                               
      IF (N .EQ. 1) RETURN                                                      
      CALL EZFFT1 (N,WSAVE(2*N+1),WSAVE(3*N+1))                                 
      RETURN                                                                    
      END                                                                       
      SUBROUTINE PASSB (NAC,IDO,IP,L1,IDL1,CC,C1,C2,CH,CH2,WA)                  
      DIMENSION       CH(IDO,L1,IP)          ,CC(IDO,IP,L1)          ,          
     1                C1(IDO,L1,IP)          ,WA(*)      ,C2(IDL1,IP),          
     2                CH2(IDL1,IP)                                              
      IDOT = IDO/2                                                              
      NT = IP*IDL1                                                              
      IPP2 = IP+2                                                               
      IPPH = (IP+1)/2                                                           
      IDP = IP*IDO                                                              
C                                                                               
      IF (IDO .LT. L1) GO TO 106                                                
      DO 103 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         DO 102 K=1,L1                                                          
            DO 101 I=1,IDO                                                      
               CH(I,K,J) = CC(I,J,K)+CC(I,JC,K)                                 
               CH(I,K,JC) = CC(I,J,K)-CC(I,JC,K)                                
  101       CONTINUE                                                            
  102    CONTINUE                                                               
  103 CONTINUE                                                                  
      DO 105 K=1,L1                                                             
         DO 104 I=1,IDO                                                         
            CH(I,K,1) = CC(I,1,K)                                               
  104    CONTINUE                                                               
  105 CONTINUE                                                                  
      GO TO 112                                                                 
  106 DO 109 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         DO 108 I=1,IDO                                                         
            DO 107 K=1,L1                                                       
               CH(I,K,J) = CC(I,J,K)+CC(I,JC,K)                                 
               CH(I,K,JC) = CC(I,J,K)-CC(I,JC,K)                                
  107       CONTINUE                                                            
  108    CONTINUE                                                               
  109 CONTINUE                                                                  
      DO 111 I=1,IDO                                                            
         DO 110 K=1,L1                                                          
            CH(I,K,1) = CC(I,1,K)                                               
  110    CONTINUE                                                               
  111 CONTINUE                                                                  
  112 IDL = 2-IDO                                                               
      INC = 0                                                                   
      DO 116 L=2,IPPH                                                           
         LC = IPP2-L                                                            
         IDL = IDL+IDO                                                          
         DO 113 IK=1,IDL1                                                       
            C2(IK,L) = CH2(IK,1)+WA(IDL-1)*CH2(IK,2)                            
            C2(IK,LC) = WA(IDL)*CH2(IK,IP)                                      
  113    CONTINUE                                                               
         IDLJ = IDL                                                             
         INC = INC+IDO                                                          
         DO 115 J=3,IPPH                                                        
            JC = IPP2-J                                                         
            IDLJ = IDLJ+INC                                                     
            IF (IDLJ .GT. IDP) IDLJ = IDLJ-IDP                                  
            WAR = WA(IDLJ-1)                                                    
            WAI = WA(IDLJ)                                                      
            DO 114 IK=1,IDL1                                                    
               C2(IK,L) = C2(IK,L)+WAR*CH2(IK,J)                                
               C2(IK,LC) = C2(IK,LC)+WAI*CH2(IK,JC)                             
  114       CONTINUE                                                            
  115    CONTINUE                                                               
  116 CONTINUE                                                                  
      DO 118 J=2,IPPH                                                           
         DO 117 IK=1,IDL1                                                       
            CH2(IK,1) = CH2(IK,1)+CH2(IK,J)                                     
  117    CONTINUE                                                               
  118 CONTINUE                                                                  
      DO 120 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         DO 119 IK=2,IDL1,2                                                     
            CH2(IK-1,J) = C2(IK-1,J)-C2(IK,JC)                                  
            CH2(IK-1,JC) = C2(IK-1,J)+C2(IK,JC)                                 
            CH2(IK,J) = C2(IK,J)+C2(IK-1,JC)                                    
            CH2(IK,JC) = C2(IK,J)-C2(IK-1,JC)                                   
  119    CONTINUE                                                               
  120 CONTINUE                                                                  
      NAC = 1                                                                   
      IF (IDO .EQ. 2) RETURN                                                    
      NAC = 0                                                                   
      DO 121 IK=1,IDL1                                                          
         C2(IK,1) = CH2(IK,1)                                                   
  121 CONTINUE                                                                  
      DO 123 J=2,IP                                                             
         DO 122 K=1,L1                                                          
            C1(1,K,J) = CH(1,K,J)                                               
            C1(2,K,J) = CH(2,K,J)                                               
  122    CONTINUE                                                               
  123 CONTINUE                                                                  
      IF (IDOT .GT. L1) GO TO 127                                               
      IDIJ = 0                                                                  
      DO 126 J=2,IP                                                             
         IDIJ = IDIJ+2                                                          
         DO 125 I=4,IDO,2                                                       
            IDIJ = IDIJ+2                                                       
            DO 124 K=1,L1                                                       
               C1(I-1,K,J) = WA(IDIJ-1)*CH(I-1,K,J)-WA(IDIJ)*CH(I,K,J)          
               C1(I,K,J) = WA(IDIJ-1)*CH(I,K,J)+WA(IDIJ)*CH(I-1,K,J)            
  124       CONTINUE                                                            
  125    CONTINUE                                                               
  126 CONTINUE                                                                  
      RETURN                                                                    
  127 IDJ = 2-IDO                                                               
      DO 130 J=2,IP                                                             
         IDJ = IDJ+IDO                                                          
         DO 129 K=1,L1                                                          
            IDIJ = IDJ                                                          
            DO 128 I=4,IDO,2                                                    
               IDIJ = IDIJ+2                                                    
               C1(I-1,K,J) = WA(IDIJ-1)*CH(I-1,K,J)-WA(IDIJ)*CH(I,K,J)          
               C1(I,K,J) = WA(IDIJ-1)*CH(I,K,J)+WA(IDIJ)*CH(I-1,K,J)            
  128       CONTINUE                                                            
  129    CONTINUE                                                               
  130 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
      SUBROUTINE PASSB2 (IDO,L1,CC,CH,WA1)                                      
      DIMENSION       CC(IDO,2,L1)           ,CH(IDO,L1,2)           ,          
     1                WA1(1)                                                    
      IF (IDO .GT. 2) GO TO 102                                                 
      DO 101 K=1,L1                                                             
         CH(1,K,1) = CC(1,1,K)+CC(1,2,K)                                        
         CH(1,K,2) = CC(1,1,K)-CC(1,2,K)                                        
         CH(2,K,1) = CC(2,1,K)+CC(2,2,K)                                        
         CH(2,K,2) = CC(2,1,K)-CC(2,2,K)                                        
  101 CONTINUE                                                                  
      RETURN                                                                    
  102 DO 104 K=1,L1                                                             
         DO 103 I=2,IDO,2                                                       
            CH(I-1,K,1) = CC(I-1,1,K)+CC(I-1,2,K)                               
            TR2 = CC(I-1,1,K)-CC(I-1,2,K)                                       
            CH(I,K,1) = CC(I,1,K)+CC(I,2,K)                                     
            TI2 = CC(I,1,K)-CC(I,2,K)                                           
            CH(I,K,2) = WA1(I-1)*TI2+WA1(I)*TR2                                 
            CH(I-1,K,2) = WA1(I-1)*TR2-WA1(I)*TI2                               
  103    CONTINUE                                                               
  104 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
      SUBROUTINE PASSB3 (IDO,L1,CC,CH,WA1,WA2)                                  
      DIMENSION       CC(IDO,3,L1)           ,CH(IDO,L1,3)           ,          
     1                WA1(*)     ,WA2(*)                                        
      DATA TAUR,TAUI /-.5,.866025403784439/                                     
      IF (IDO .NE. 2) GO TO 102                                                 
      DO 101 K=1,L1                                                             
         TR2 = CC(1,2,K)+CC(1,3,K)                                              
         CR2 = CC(1,1,K)+TAUR*TR2                                               
         CH(1,K,1) = CC(1,1,K)+TR2                                              
         TI2 = CC(2,2,K)+CC(2,3,K)                                              
         CI2 = CC(2,1,K)+TAUR*TI2                                               
         CH(2,K,1) = CC(2,1,K)+TI2                                              
         CR3 = TAUI*(CC(1,2,K)-CC(1,3,K))                                       
         CI3 = TAUI*(CC(2,2,K)-CC(2,3,K))                                       
         CH(1,K,2) = CR2-CI3                                                    
         CH(1,K,3) = CR2+CI3                                                    
         CH(2,K,2) = CI2+CR3                                                    
         CH(2,K,3) = CI2-CR3                                                    
  101 CONTINUE                                                                  
      RETURN                                                                    
  102 DO 104 K=1,L1                                                             
         DO 103 I=2,IDO,2                                                       
            TR2 = CC(I-1,2,K)+CC(I-1,3,K)                                       
            CR2 = CC(I-1,1,K)+TAUR*TR2                                          
            CH(I-1,K,1) = CC(I-1,1,K)+TR2                                       
            TI2 = CC(I,2,K)+CC(I,3,K)                                           
            CI2 = CC(I,1,K)+TAUR*TI2                                            
            CH(I,K,1) = CC(I,1,K)+TI2                                           
            CR3 = TAUI*(CC(I-1,2,K)-CC(I-1,3,K))                                
            CI3 = TAUI*(CC(I,2,K)-CC(I,3,K))                                    
            DR2 = CR2-CI3                                                       
            DR3 = CR2+CI3                                                       
            DI2 = CI2+CR3                                                       
            DI3 = CI2-CR3                                                       
            CH(I,K,2) = WA1(I-1)*DI2+WA1(I)*DR2                                 
            CH(I-1,K,2) = WA1(I-1)*DR2-WA1(I)*DI2                               
            CH(I,K,3) = WA2(I-1)*DI3+WA2(I)*DR3                                 
            CH(I-1,K,3) = WA2(I-1)*DR3-WA2(I)*DI3                               
  103    CONTINUE                                                               
  104 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
      SUBROUTINE PASSB4 (IDO,L1,CC,CH,WA1,WA2,WA3)                              
      DIMENSION       CC(IDO,4,L1)           ,CH(IDO,L1,4)           ,          
     1                WA1(*)     ,WA2(*)     ,WA3(*)                            
      IF (IDO .NE. 2) GO TO 102                                                 
      DO 101 K=1,L1                                                             
         TI1 = CC(2,1,K)-CC(2,3,K)                                              
         TI2 = CC(2,1,K)+CC(2,3,K)                                              
         TR4 = CC(2,4,K)-CC(2,2,K)                                              
         TI3 = CC(2,2,K)+CC(2,4,K)                                              
         TR1 = CC(1,1,K)-CC(1,3,K)                                              
         TR2 = CC(1,1,K)+CC(1,3,K)                                              
         TI4 = CC(1,2,K)-CC(1,4,K)                                              
         TR3 = CC(1,2,K)+CC(1,4,K)                                              
         CH(1,K,1) = TR2+TR3                                                    
         CH(1,K,3) = TR2-TR3                                                    
         CH(2,K,1) = TI2+TI3                                                    
         CH(2,K,3) = TI2-TI3                                                    
         CH(1,K,2) = TR1+TR4                                                    
         CH(1,K,4) = TR1-TR4                                                    
         CH(2,K,2) = TI1+TI4                                                    
         CH(2,K,4) = TI1-TI4                                                    
  101 CONTINUE                                                                  
      RETURN                                                                    
  102 DO 104 K=1,L1                                                             
         DO 103 I=2,IDO,2                                                       
            TI1 = CC(I,1,K)-CC(I,3,K)                                           
            TI2 = CC(I,1,K)+CC(I,3,K)                                           
            TI3 = CC(I,2,K)+CC(I,4,K)                                           
            TR4 = CC(I,4,K)-CC(I,2,K)                                           
            TR1 = CC(I-1,1,K)-CC(I-1,3,K)                                       
            TR2 = CC(I-1,1,K)+CC(I-1,3,K)                                       
            TI4 = CC(I-1,2,K)-CC(I-1,4,K)                                       
            TR3 = CC(I-1,2,K)+CC(I-1,4,K)                                       
            CH(I-1,K,1) = TR2+TR3                                               
            CR3 = TR2-TR3                                                       
            CH(I,K,1) = TI2+TI3                                                 
            CI3 = TI2-TI3                                                       
            CR2 = TR1+TR4                                                       
            CR4 = TR1-TR4                                                       
            CI2 = TI1+TI4                                                       
            CI4 = TI1-TI4                                                       
            CH(I-1,K,2) = WA1(I-1)*CR2-WA1(I)*CI2                               
            CH(I,K,2) = WA1(I-1)*CI2+WA1(I)*CR2                                 
            CH(I-1,K,3) = WA2(I-1)*CR3-WA2(I)*CI3                               
            CH(I,K,3) = WA2(I-1)*CI3+WA2(I)*CR3                                 
            CH(I-1,K,4) = WA3(I-1)*CR4-WA3(I)*CI4                               
            CH(I,K,4) = WA3(I-1)*CI4+WA3(I)*CR4                                 
  103    CONTINUE                                                               
  104 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
      SUBROUTINE PASSB5 (IDO,L1,CC,CH,WA1,WA2,WA3,WA4)                          
      DIMENSION       CC(IDO,5,L1)           ,CH(IDO,L1,5)           ,          
     1                WA1(*)     ,WA2(*)     ,WA3(*)     ,WA4(*)                
      DATA TR11,TI11,TR12,TI12 /.309016994374947,.951056516295154,              
     1-.809016994374947,.587785252292473/                                       
      IF (IDO .NE. 2) GO TO 102                                                 
      DO 101 K=1,L1                                                             
         TI5 = CC(2,2,K)-CC(2,5,K)                                              
         TI2 = CC(2,2,K)+CC(2,5,K)                                              
         TI4 = CC(2,3,K)-CC(2,4,K)                                              
         TI3 = CC(2,3,K)+CC(2,4,K)                                              
         TR5 = CC(1,2,K)-CC(1,5,K)                                              
         TR2 = CC(1,2,K)+CC(1,5,K)                                              
         TR4 = CC(1,3,K)-CC(1,4,K)                                              
         TR3 = CC(1,3,K)+CC(1,4,K)                                              
         CH(1,K,1) = CC(1,1,K)+TR2+TR3                                          
         CH(2,K,1) = CC(2,1,K)+TI2+TI3                                          
         CR2 = CC(1,1,K)+TR11*TR2+TR12*TR3                                      
         CI2 = CC(2,1,K)+TR11*TI2+TR12*TI3                                      
         CR3 = CC(1,1,K)+TR12*TR2+TR11*TR3                                      
         CI3 = CC(2,1,K)+TR12*TI2+TR11*TI3                                      
         CR5 = TI11*TR5+TI12*TR4                                                
         CI5 = TI11*TI5+TI12*TI4                                                
         CR4 = TI12*TR5-TI11*TR4                                                
         CI4 = TI12*TI5-TI11*TI4                                                
         CH(1,K,2) = CR2-CI5                                                    
         CH(1,K,5) = CR2+CI5                                                    
         CH(2,K,2) = CI2+CR5                                                    
         CH(2,K,3) = CI3+CR4                                                    
         CH(1,K,3) = CR3-CI4                                                    
         CH(1,K,4) = CR3+CI4                                                    
         CH(2,K,4) = CI3-CR4                                                    
         CH(2,K,5) = CI2-CR5                                                    
  101 CONTINUE                                                                  
      RETURN                                                                    
  102 DO 104 K=1,L1                                                             
         DO 103 I=2,IDO,2                                                       
            TI5 = CC(I,2,K)-CC(I,5,K)                                           
            TI2 = CC(I,2,K)+CC(I,5,K)                                           
            TI4 = CC(I,3,K)-CC(I,4,K)                                           
            TI3 = CC(I,3,K)+CC(I,4,K)                                           
            TR5 = CC(I-1,2,K)-CC(I-1,5,K)                                       
            TR2 = CC(I-1,2,K)+CC(I-1,5,K)                                       
            TR4 = CC(I-1,3,K)-CC(I-1,4,K)                                       
            TR3 = CC(I-1,3,K)+CC(I-1,4,K)                                       
            CH(I-1,K,1) = CC(I-1,1,K)+TR2+TR3                                   
            CH(I,K,1) = CC(I,1,K)+TI2+TI3                                       
            CR2 = CC(I-1,1,K)+TR11*TR2+TR12*TR3                                 
            CI2 = CC(I,1,K)+TR11*TI2+TR12*TI3                                   
            CR3 = CC(I-1,1,K)+TR12*TR2+TR11*TR3                                 
            CI3 = CC(I,1,K)+TR12*TI2+TR11*TI3                                   
            CR5 = TI11*TR5+TI12*TR4                                             
            CI5 = TI11*TI5+TI12*TI4                                             
            CR4 = TI12*TR5-TI11*TR4                                             
            CI4 = TI12*TI5-TI11*TI4                                             
            DR3 = CR3-CI4                                                       
            DR4 = CR3+CI4                                                       
            DI3 = CI3+CR4                                                       
            DI4 = CI3-CR4                                                       
            DR5 = CR2+CI5                                                       
            DR2 = CR2-CI5                                                       
            DI5 = CI2-CR5                                                       
            DI2 = CI2+CR5                                                       
            CH(I-1,K,2) = WA1(I-1)*DR2-WA1(I)*DI2                               
            CH(I,K,2) = WA1(I-1)*DI2+WA1(I)*DR2                                 
            CH(I-1,K,3) = WA2(I-1)*DR3-WA2(I)*DI3                               
            CH(I,K,3) = WA2(I-1)*DI3+WA2(I)*DR3                                 
            CH(I-1,K,4) = WA3(I-1)*DR4-WA3(I)*DI4                               
            CH(I,K,4) = WA3(I-1)*DI4+WA3(I)*DR4                                 
            CH(I-1,K,5) = WA4(I-1)*DR5-WA4(I)*DI5                               
            CH(I,K,5) = WA4(I-1)*DI5+WA4(I)*DR5                                 
  103    CONTINUE                                                               
  104 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
      SUBROUTINE PASSF (NAC,IDO,IP,L1,IDL1,CC,C1,C2,CH,CH2,WA)                  
      DIMENSION       CH(IDO,L1,IP)          ,CC(IDO,IP,L1)          ,          
     1                C1(IDO,L1,IP)          ,WA(*)      ,C2(IDL1,IP),          
     2                CH2(IDL1,IP)                                              
      IDOT = IDO/2                                                              
      NT = IP*IDL1                                                              
      IPP2 = IP+2                                                               
      IPPH = (IP+1)/2                                                           
      IDP = IP*IDO                                                              
C                                                                               
      IF (IDO .LT. L1) GO TO 106                                                
      DO 103 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         DO 102 K=1,L1                                                          
            DO 101 I=1,IDO                                                      
               CH(I,K,J) = CC(I,J,K)+CC(I,JC,K)                                 
               CH(I,K,JC) = CC(I,J,K)-CC(I,JC,K)                                
  101       CONTINUE                                                            
  102    CONTINUE                                                               
  103 CONTINUE                                                                  
      DO 105 K=1,L1                                                             
         DO 104 I=1,IDO                                                         
            CH(I,K,1) = CC(I,1,K)                                               
  104    CONTINUE                                                               
  105 CONTINUE                                                                  
      GO TO 112                                                                 
  106 DO 109 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         DO 108 I=1,IDO                                                         
            DO 107 K=1,L1                                                       
               CH(I,K,J) = CC(I,J,K)+CC(I,JC,K)                                 
               CH(I,K,JC) = CC(I,J,K)-CC(I,JC,K)                                
  107       CONTINUE                                                            
  108    CONTINUE                                                               
  109 CONTINUE                                                                  
      DO 111 I=1,IDO                                                            
         DO 110 K=1,L1                                                          
            CH(I,K,1) = CC(I,1,K)                                               
  110    CONTINUE                                                               
  111 CONTINUE                                                                  
  112 IDL = 2-IDO                                                               
      INC = 0                                                                   
      DO 116 L=2,IPPH                                                           
         LC = IPP2-L                                                            
         IDL = IDL+IDO                                                          
         DO 113 IK=1,IDL1                                                       
            C2(IK,L) = CH2(IK,1)+WA(IDL-1)*CH2(IK,2)                            
            C2(IK,LC) = -WA(IDL)*CH2(IK,IP)                                     
  113    CONTINUE                                                               
         IDLJ = IDL                                                             
         INC = INC+IDO                                                          
         DO 115 J=3,IPPH                                                        
            JC = IPP2-J                                                         
            IDLJ = IDLJ+INC                                                     
            IF (IDLJ .GT. IDP) IDLJ = IDLJ-IDP                                  
            WAR = WA(IDLJ-1)                                                    
            WAI = WA(IDLJ)                                                      
            DO 114 IK=1,IDL1                                                    
               C2(IK,L) = C2(IK,L)+WAR*CH2(IK,J)                                
               C2(IK,LC) = C2(IK,LC)-WAI*CH2(IK,JC)                             
  114       CONTINUE                                                            
  115    CONTINUE                                                               
  116 CONTINUE                                                                  
      DO 118 J=2,IPPH                                                           
         DO 117 IK=1,IDL1                                                       
            CH2(IK,1) = CH2(IK,1)+CH2(IK,J)                                     
  117    CONTINUE                                                               
  118 CONTINUE                                                                  
      DO 120 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         DO 119 IK=2,IDL1,2                                                     
            CH2(IK-1,J) = C2(IK-1,J)-C2(IK,JC)                                  
            CH2(IK-1,JC) = C2(IK-1,J)+C2(IK,JC)                                 
            CH2(IK,J) = C2(IK,J)+C2(IK-1,JC)                                    
            CH2(IK,JC) = C2(IK,J)-C2(IK-1,JC)                                   
  119    CONTINUE                                                               
  120 CONTINUE                                                                  
      NAC = 1                                                                   
      IF (IDO .EQ. 2) RETURN                                                    
      NAC = 0                                                                   
      DO 121 IK=1,IDL1                                                          
         C2(IK,1) = CH2(IK,1)                                                   
  121 CONTINUE                                                                  
      DO 123 J=2,IP                                                             
         DO 122 K=1,L1                                                          
            C1(1,K,J) = CH(1,K,J)                                               
            C1(2,K,J) = CH(2,K,J)                                               
  122    CONTINUE                                                               
  123 CONTINUE                                                                  
      IF (IDOT .GT. L1) GO TO 127                                               
      IDIJ = 0                                                                  
      DO 126 J=2,IP                                                             
         IDIJ = IDIJ+2                                                          
         DO 125 I=4,IDO,2                                                       
            IDIJ = IDIJ+2                                                       
            DO 124 K=1,L1                                                       
               C1(I-1,K,J) = WA(IDIJ-1)*CH(I-1,K,J)+WA(IDIJ)*CH(I,K,J)          
               C1(I,K,J) = WA(IDIJ-1)*CH(I,K,J)-WA(IDIJ)*CH(I-1,K,J)            
  124       CONTINUE                                                            
  125    CONTINUE                                                               
  126 CONTINUE                                                                  
      RETURN                                                                    
  127 IDJ = 2-IDO                                                               
      DO 130 J=2,IP                                                             
         IDJ = IDJ+IDO                                                          
         DO 129 K=1,L1                                                          
            IDIJ = IDJ                                                          
            DO 128 I=4,IDO,2                                                    
               IDIJ = IDIJ+2                                                    
               C1(I-1,K,J) = WA(IDIJ-1)*CH(I-1,K,J)+WA(IDIJ)*CH(I,K,J)          
               C1(I,K,J) = WA(IDIJ-1)*CH(I,K,J)-WA(IDIJ)*CH(I-1,K,J)            
  128       CONTINUE                                                            
  129    CONTINUE                                                               
  130 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
      SUBROUTINE PASSF2 (IDO,L1,CC,CH,WA1)                                      
      DIMENSION       CC(IDO,2,L1)           ,CH(IDO,L1,2)           ,          
     1                WA1(*)                                                    
      IF (IDO .GT. 2) GO TO 102                                                 
      DO 101 K=1,L1                                                             
         CH(1,K,1) = CC(1,1,K)+CC(1,2,K)                                        
         CH(1,K,2) = CC(1,1,K)-CC(1,2,K)                                        
         CH(2,K,1) = CC(2,1,K)+CC(2,2,K)                                        
         CH(2,K,2) = CC(2,1,K)-CC(2,2,K)                                        
  101 CONTINUE                                                                  
      RETURN                                                                    
  102 DO 104 K=1,L1                                                             
         DO 103 I=2,IDO,2                                                       
            CH(I-1,K,1) = CC(I-1,1,K)+CC(I-1,2,K)                               
            TR2 = CC(I-1,1,K)-CC(I-1,2,K)                                       
            CH(I,K,1) = CC(I,1,K)+CC(I,2,K)                                     
            TI2 = CC(I,1,K)-CC(I,2,K)                                           
            CH(I,K,2) = WA1(I-1)*TI2-WA1(I)*TR2                                 
            CH(I-1,K,2) = WA1(I-1)*TR2+WA1(I)*TI2                               
  103    CONTINUE                                                               
  104 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
      SUBROUTINE PASSF3 (IDO,L1,CC,CH,WA1,WA2)                                  
      DIMENSION       CC(IDO,3,L1)           ,CH(IDO,L1,3)           ,          
     1                WA1(*)     ,WA2(*)                                        
      DATA TAUR,TAUI /-.5,-.866025403784439/                                    
      IF (IDO .NE. 2) GO TO 102                                                 
      DO 101 K=1,L1                                                             
         TR2 = CC(1,2,K)+CC(1,3,K)                                              
         CR2 = CC(1,1,K)+TAUR*TR2                                               
         CH(1,K,1) = CC(1,1,K)+TR2                                              
         TI2 = CC(2,2,K)+CC(2,3,K)                                              
         CI2 = CC(2,1,K)+TAUR*TI2                                               
         CH(2,K,1) = CC(2,1,K)+TI2                                              
         CR3 = TAUI*(CC(1,2,K)-CC(1,3,K))                                       
         CI3 = TAUI*(CC(2,2,K)-CC(2,3,K))                                       
         CH(1,K,2) = CR2-CI3                                                    
         CH(1,K,3) = CR2+CI3                                                    
         CH(2,K,2) = CI2+CR3                                                    
         CH(2,K,3) = CI2-CR3                                                    
  101 CONTINUE                                                                  
      RETURN                                                                    
  102 DO 104 K=1,L1                                                             
         DO 103 I=2,IDO,2                                                       
            TR2 = CC(I-1,2,K)+CC(I-1,3,K)                                       
            CR2 = CC(I-1,1,K)+TAUR*TR2                                          
            CH(I-1,K,1) = CC(I-1,1,K)+TR2                                       
            TI2 = CC(I,2,K)+CC(I,3,K)                                           
            CI2 = CC(I,1,K)+TAUR*TI2                                            
            CH(I,K,1) = CC(I,1,K)+TI2                                           
            CR3 = TAUI*(CC(I-1,2,K)-CC(I-1,3,K))                                
            CI3 = TAUI*(CC(I,2,K)-CC(I,3,K))                                    
            DR2 = CR2-CI3                                                       
            DR3 = CR2+CI3                                                       
            DI2 = CI2+CR3                                                       
            DI3 = CI2-CR3                                                       
            CH(I,K,2) = WA1(I-1)*DI2-WA1(I)*DR2                                 
            CH(I-1,K,2) = WA1(I-1)*DR2+WA1(I)*DI2                               
            CH(I,K,3) = WA2(I-1)*DI3-WA2(I)*DR3                                 
            CH(I-1,K,3) = WA2(I-1)*DR3+WA2(I)*DI3                               
  103    CONTINUE                                                               
  104 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
      SUBROUTINE PASSF4 (IDO,L1,CC,CH,WA1,WA2,WA3)                              
      DIMENSION       CC(IDO,4,L1)           ,CH(IDO,L1,4)           ,          
     1                WA1(*)     ,WA2(*)     ,WA3(*)                            
      IF (IDO .NE. 2) GO TO 102                                                 
      DO 101 K=1,L1                                                             
         TI1 = CC(2,1,K)-CC(2,3,K)                                              
         TI2 = CC(2,1,K)+CC(2,3,K)                                              
         TR4 = CC(2,2,K)-CC(2,4,K)                                              
         TI3 = CC(2,2,K)+CC(2,4,K)                                              
         TR1 = CC(1,1,K)-CC(1,3,K)                                              
         TR2 = CC(1,1,K)+CC(1,3,K)                                              
         TI4 = CC(1,4,K)-CC(1,2,K)                                              
         TR3 = CC(1,2,K)+CC(1,4,K)                                              
         CH(1,K,1) = TR2+TR3                                                    
         CH(1,K,3) = TR2-TR3                                                    
         CH(2,K,1) = TI2+TI3                                                    
         CH(2,K,3) = TI2-TI3                                                    
         CH(1,K,2) = TR1+TR4                                                    
         CH(1,K,4) = TR1-TR4                                                    
         CH(2,K,2) = TI1+TI4                                                    
         CH(2,K,4) = TI1-TI4                                                    
  101 CONTINUE                                                                  
      RETURN                                                                    
  102 DO 104 K=1,L1                                                             
         DO 103 I=2,IDO,2                                                       
            TI1 = CC(I,1,K)-CC(I,3,K)                                           
            TI2 = CC(I,1,K)+CC(I,3,K)                                           
            TI3 = CC(I,2,K)+CC(I,4,K)                                           
            TR4 = CC(I,2,K)-CC(I,4,K)                                           
            TR1 = CC(I-1,1,K)-CC(I-1,3,K)                                       
            TR2 = CC(I-1,1,K)+CC(I-1,3,K)                                       
            TI4 = CC(I-1,4,K)-CC(I-1,2,K)                                       
            TR3 = CC(I-1,2,K)+CC(I-1,4,K)                                       
            CH(I-1,K,1) = TR2+TR3                                               
            CR3 = TR2-TR3                                                       
            CH(I,K,1) = TI2+TI3                                                 
            CI3 = TI2-TI3                                                       
            CR2 = TR1+TR4                                                       
            CR4 = TR1-TR4                                                       
            CI2 = TI1+TI4                                                       
            CI4 = TI1-TI4                                                       
            CH(I-1,K,2) = WA1(I-1)*CR2+WA1(I)*CI2                               
            CH(I,K,2) = WA1(I-1)*CI2-WA1(I)*CR2                                 
            CH(I-1,K,3) = WA2(I-1)*CR3+WA2(I)*CI3                               
            CH(I,K,3) = WA2(I-1)*CI3-WA2(I)*CR3                                 
            CH(I-1,K,4) = WA3(I-1)*CR4+WA3(I)*CI4                               
            CH(I,K,4) = WA3(I-1)*CI4-WA3(I)*CR4                                 
  103    CONTINUE                                                               
  104 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
      SUBROUTINE PASSF5 (IDO,L1,CC,CH,WA1,WA2,WA3,WA4)                          
      DIMENSION       CC(IDO,5,L1)           ,CH(IDO,L1,5)           ,          
     1                WA1(*)     ,WA2(*)     ,WA3(*)     ,WA4(*)                
      DATA TR11,TI11,TR12,TI12 /.309016994374947,-.951056516295154,             
     1-.809016994374947,-.587785252292473/                                      
      IF (IDO .NE. 2) GO TO 102                                                 
      DO 101 K=1,L1                                                             
         TI5 = CC(2,2,K)-CC(2,5,K)                                              
         TI2 = CC(2,2,K)+CC(2,5,K)                                              
         TI4 = CC(2,3,K)-CC(2,4,K)                                              
         TI3 = CC(2,3,K)+CC(2,4,K)                                              
         TR5 = CC(1,2,K)-CC(1,5,K)                                              
         TR2 = CC(1,2,K)+CC(1,5,K)                                              
         TR4 = CC(1,3,K)-CC(1,4,K)                                              
         TR3 = CC(1,3,K)+CC(1,4,K)                                              
         CH(1,K,1) = CC(1,1,K)+TR2+TR3                                          
         CH(2,K,1) = CC(2,1,K)+TI2+TI3                                          
         CR2 = CC(1,1,K)+TR11*TR2+TR12*TR3                                      
         CI2 = CC(2,1,K)+TR11*TI2+TR12*TI3                                      
         CR3 = CC(1,1,K)+TR12*TR2+TR11*TR3                                      
         CI3 = CC(2,1,K)+TR12*TI2+TR11*TI3                                      
         CR5 = TI11*TR5+TI12*TR4                                                
         CI5 = TI11*TI5+TI12*TI4                                                
         CR4 = TI12*TR5-TI11*TR4                                                
         CI4 = TI12*TI5-TI11*TI4                                                
         CH(1,K,2) = CR2-CI5                                                    
         CH(1,K,5) = CR2+CI5                                                    
         CH(2,K,2) = CI2+CR5                                                    
         CH(2,K,3) = CI3+CR4                                                    
         CH(1,K,3) = CR3-CI4                                                    
         CH(1,K,4) = CR3+CI4                                                    
         CH(2,K,4) = CI3-CR4                                                    
         CH(2,K,5) = CI2-CR5                                                    
  101 CONTINUE                                                                  
      RETURN                                                                    
  102 DO 104 K=1,L1                                                             
         DO 103 I=2,IDO,2                                                       
            TI5 = CC(I,2,K)-CC(I,5,K)                                           
            TI2 = CC(I,2,K)+CC(I,5,K)                                           
            TI4 = CC(I,3,K)-CC(I,4,K)                                           
            TI3 = CC(I,3,K)+CC(I,4,K)                                           
            TR5 = CC(I-1,2,K)-CC(I-1,5,K)                                       
            TR2 = CC(I-1,2,K)+CC(I-1,5,K)                                       
            TR4 = CC(I-1,3,K)-CC(I-1,4,K)                                       
            TR3 = CC(I-1,3,K)+CC(I-1,4,K)                                       
            CH(I-1,K,1) = CC(I-1,1,K)+TR2+TR3                                   
            CH(I,K,1) = CC(I,1,K)+TI2+TI3                                       
            CR2 = CC(I-1,1,K)+TR11*TR2+TR12*TR3                                 
            CI2 = CC(I,1,K)+TR11*TI2+TR12*TI3                                   
            CR3 = CC(I-1,1,K)+TR12*TR2+TR11*TR3                                 
            CI3 = CC(I,1,K)+TR12*TI2+TR11*TI3                                   
            CR5 = TI11*TR5+TI12*TR4                                             
            CI5 = TI11*TI5+TI12*TI4                                             
            CR4 = TI12*TR5-TI11*TR4                                             
            CI4 = TI12*TI5-TI11*TI4                                             
            DR3 = CR3-CI4                                                       
            DR4 = CR3+CI4                                                       
            DI3 = CI3+CR4                                                       
            DI4 = CI3-CR4                                                       
            DR5 = CR2+CI5                                                       
            DR2 = CR2-CI5                                                       
            DI5 = CI2-CR5                                                       
            DI2 = CI2+CR5                                                       
            CH(I-1,K,2) = WA1(I-1)*DR2+WA1(I)*DI2                               
            CH(I,K,2) = WA1(I-1)*DI2-WA1(I)*DR2                                 
            CH(I-1,K,3) = WA2(I-1)*DR3+WA2(I)*DI3                               
            CH(I,K,3) = WA2(I-1)*DI3-WA2(I)*DR3                                 
            CH(I-1,K,4) = WA3(I-1)*DR4+WA3(I)*DI4                               
            CH(I,K,4) = WA3(I-1)*DI4-WA3(I)*DR4                                 
            CH(I-1,K,5) = WA4(I-1)*DR5+WA4(I)*DI5                               
            CH(I,K,5) = WA4(I-1)*DI5-WA4(I)*DR5                                 
  103    CONTINUE                                                               
  104 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
      FUNCTION PIMACH (DUM)                                                     
C     PI=3.1415926535897932384626433832795028841971693993751058209749446        
C                                                                               
      PIMACH = 4.*ATAN(1.0)                                                     
      RETURN                                                                    
      END                                                                       
      SUBROUTINE RADB2 (IDO,L1,CC,CH,WA1)                                       
      DIMENSION       CC(IDO,2,L1)           ,CH(IDO,L1,2)           ,          
     1                WA1(*)                                                    
      DO 101 K=1,L1                                                             
         CH(1,K,1) = CC(1,1,K)+CC(IDO,2,K)                                      
         CH(1,K,2) = CC(1,1,K)-CC(IDO,2,K)                                      
  101 CONTINUE                                                                  
      IF (IDO-2) 107,105,102                                                    
  102 IDP2 = IDO+2                                                              
      DO 104 K=1,L1                                                             
         DO 103 I=3,IDO,2                                                       
            IC = IDP2-I                                                         
            CH(I-1,K,1) = CC(I-1,1,K)+CC(IC-1,2,K)                              
            TR2 = CC(I-1,1,K)-CC(IC-1,2,K)                                      
            CH(I,K,1) = CC(I,1,K)-CC(IC,2,K)                                    
            TI2 = CC(I,1,K)+CC(IC,2,K)                                          
            CH(I-1,K,2) = WA1(I-2)*TR2-WA1(I-1)*TI2                             
            CH(I,K,2) = WA1(I-2)*TI2+WA1(I-1)*TR2                               
  103    CONTINUE                                                               
  104 CONTINUE                                                                  
      IF (MOD(IDO,2) .EQ. 1) RETURN                                             
  105 DO 106 K=1,L1                                                             
         CH(IDO,K,1) = CC(IDO,1,K)+CC(IDO,1,K)                                  
         CH(IDO,K,2) = -(CC(1,2,K)+CC(1,2,K))                                   
  106 CONTINUE                                                                  
  107 RETURN                                                                    
      END                                                                       
      SUBROUTINE RADB3 (IDO,L1,CC,CH,WA1,WA2)                                   
      DIMENSION       CC(IDO,3,L1)           ,CH(IDO,L1,3)           ,          
     1                WA1(*)     ,WA2(*)                                        
      DATA TAUR,TAUI /-.5,.866025403784439/                                     
      DO 101 K=1,L1                                                             
         TR2 = CC(IDO,2,K)+CC(IDO,2,K)                                          
         CR2 = CC(1,1,K)+TAUR*TR2                                               
         CH(1,K,1) = CC(1,1,K)+TR2                                              
         CI3 = TAUI*(CC(1,3,K)+CC(1,3,K))                                       
         CH(1,K,2) = CR2-CI3                                                    
         CH(1,K,3) = CR2+CI3                                                    
  101 CONTINUE                                                                  
      IF (IDO .EQ. 1) RETURN                                                    
      IDP2 = IDO+2                                                              
      DO 103 K=1,L1                                                             
         DO 102 I=3,IDO,2                                                       
            IC = IDP2-I                                                         
            TR2 = CC(I-1,3,K)+CC(IC-1,2,K)                                      
            CR2 = CC(I-1,1,K)+TAUR*TR2                                          
            CH(I-1,K,1) = CC(I-1,1,K)+TR2                                       
            TI2 = CC(I,3,K)-CC(IC,2,K)                                          
            CI2 = CC(I,1,K)+TAUR*TI2                                            
            CH(I,K,1) = CC(I,1,K)+TI2                                           
            CR3 = TAUI*(CC(I-1,3,K)-CC(IC-1,2,K))                               
            CI3 = TAUI*(CC(I,3,K)+CC(IC,2,K))                                   
            DR2 = CR2-CI3                                                       
            DR3 = CR2+CI3                                                       
            DI2 = CI2+CR3                                                       
            DI3 = CI2-CR3                                                       
            CH(I-1,K,2) = WA1(I-2)*DR2-WA1(I-1)*DI2                             
            CH(I,K,2) = WA1(I-2)*DI2+WA1(I-1)*DR2                               
            CH(I-1,K,3) = WA2(I-2)*DR3-WA2(I-1)*DI3                             
            CH(I,K,3) = WA2(I-2)*DI3+WA2(I-1)*DR3                               
  102    CONTINUE                                                               
  103 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
      SUBROUTINE RADB4 (IDO,L1,CC,CH,WA1,WA2,WA3)                               
      DIMENSION       CC(IDO,4,L1)           ,CH(IDO,L1,4)           ,          
     1                WA1(*)     ,WA2(*)     ,WA3(*)                            
      DATA SQRT2 /1.414213562373095/                                            
      DO 101 K=1,L1                                                             
         TR1 = CC(1,1,K)-CC(IDO,4,K)                                            
         TR2 = CC(1,1,K)+CC(IDO,4,K)                                            
         TR3 = CC(IDO,2,K)+CC(IDO,2,K)                                          
         TR4 = CC(1,3,K)+CC(1,3,K)                                              
         CH(1,K,1) = TR2+TR3                                                    
         CH(1,K,2) = TR1-TR4                                                    
         CH(1,K,3) = TR2-TR3                                                    
         CH(1,K,4) = TR1+TR4                                                    
  101 CONTINUE                                                                  
      IF (IDO-2) 107,105,102                                                    
  102 IDP2 = IDO+2                                                              
      DO 104 K=1,L1                                                             
         DO 103 I=3,IDO,2                                                       
            IC = IDP2-I                                                         
            TI1 = CC(I,1,K)+CC(IC,4,K)                                          
            TI2 = CC(I,1,K)-CC(IC,4,K)                                          
            TI3 = CC(I,3,K)-CC(IC,2,K)                                          
            TR4 = CC(I,3,K)+CC(IC,2,K)                                          
            TR1 = CC(I-1,1,K)-CC(IC-1,4,K)                                      
            TR2 = CC(I-1,1,K)+CC(IC-1,4,K)                                      
            TI4 = CC(I-1,3,K)-CC(IC-1,2,K)                                      
            TR3 = CC(I-1,3,K)+CC(IC-1,2,K)                                      
            CH(I-1,K,1) = TR2+TR3                                               
            CR3 = TR2-TR3                                                       
            CH(I,K,1) = TI2+TI3                                                 
            CI3 = TI2-TI3                                                       
            CR2 = TR1-TR4                                                       
            CR4 = TR1+TR4                                                       
            CI2 = TI1+TI4                                                       
            CI4 = TI1-TI4                                                       
            CH(I-1,K,2) = WA1(I-2)*CR2-WA1(I-1)*CI2                             
            CH(I,K,2) = WA1(I-2)*CI2+WA1(I-1)*CR2                               
            CH(I-1,K,3) = WA2(I-2)*CR3-WA2(I-1)*CI3                             
            CH(I,K,3) = WA2(I-2)*CI3+WA2(I-1)*CR3                               
            CH(I-1,K,4) = WA3(I-2)*CR4-WA3(I-1)*CI4                             
            CH(I,K,4) = WA3(I-2)*CI4+WA3(I-1)*CR4                               
  103    CONTINUE                                                               
  104 CONTINUE                                                                  
      IF (MOD(IDO,2) .EQ. 1) RETURN                                             
  105 CONTINUE                                                                  
      DO 106 K=1,L1                                                             
         TI1 = CC(1,2,K)+CC(1,4,K)                                              
         TI2 = CC(1,4,K)-CC(1,2,K)                                              
         TR1 = CC(IDO,1,K)-CC(IDO,3,K)                                          
         TR2 = CC(IDO,1,K)+CC(IDO,3,K)                                          
         CH(IDO,K,1) = TR2+TR2                                                  
         CH(IDO,K,2) = SQRT2*(TR1-TI1)                                          
         CH(IDO,K,3) = TI2+TI2                                                  
         CH(IDO,K,4) = -SQRT2*(TR1+TI1)                                         
  106 CONTINUE                                                                  
  107 RETURN                                                                    
      END                                                                       
      SUBROUTINE RADB5 (IDO,L1,CC,CH,WA1,WA2,WA3,WA4)                           
      DIMENSION       CC(IDO,5,L1)           ,CH(IDO,L1,5)           ,          
     1                WA1(*)     ,WA2(*)     ,WA3(*)     ,WA4(*)                
      DATA TR11,TI11,TR12,TI12 /.309016994374947,.951056516295154,              
     1-.809016994374947,.587785252292473/                                       
      DO 101 K=1,L1                                                             
         TI5 = CC(1,3,K)+CC(1,3,K)                                              
         TI4 = CC(1,5,K)+CC(1,5,K)                                              
         TR2 = CC(IDO,2,K)+CC(IDO,2,K)                                          
         TR3 = CC(IDO,4,K)+CC(IDO,4,K)                                          
         CH(1,K,1) = CC(1,1,K)+TR2+TR3                                          
         CR2 = CC(1,1,K)+TR11*TR2+TR12*TR3                                      
         CR3 = CC(1,1,K)+TR12*TR2+TR11*TR3                                      
         CI5 = TI11*TI5+TI12*TI4                                                
         CI4 = TI12*TI5-TI11*TI4                                                
         CH(1,K,2) = CR2-CI5                                                    
         CH(1,K,3) = CR3-CI4                                                    
         CH(1,K,4) = CR3+CI4                                                    
         CH(1,K,5) = CR2+CI5                                                    
  101 CONTINUE                                                                  
      IF (IDO .EQ. 1) RETURN                                                    
      IDP2 = IDO+2                                                              
      DO 103 K=1,L1                                                             
         DO 102 I=3,IDO,2                                                       
            IC = IDP2-I                                                         
            TI5 = CC(I,3,K)+CC(IC,2,K)                                          
            TI2 = CC(I,3,K)-CC(IC,2,K)                                          
            TI4 = CC(I,5,K)+CC(IC,4,K)                                          
            TI3 = CC(I,5,K)-CC(IC,4,K)                                          
            TR5 = CC(I-1,3,K)-CC(IC-1,2,K)                                      
            TR2 = CC(I-1,3,K)+CC(IC-1,2,K)                                      
            TR4 = CC(I-1,5,K)-CC(IC-1,4,K)                                      
            TR3 = CC(I-1,5,K)+CC(IC-1,4,K)                                      
            CH(I-1,K,1) = CC(I-1,1,K)+TR2+TR3                                   
            CH(I,K,1) = CC(I,1,K)+TI2+TI3                                       
            CR2 = CC(I-1,1,K)+TR11*TR2+TR12*TR3                                 
            CI2 = CC(I,1,K)+TR11*TI2+TR12*TI3                                   
            CR3 = CC(I-1,1,K)+TR12*TR2+TR11*TR3                                 
            CI3 = CC(I,1,K)+TR12*TI2+TR11*TI3                                   
            CR5 = TI11*TR5+TI12*TR4                                             
            CI5 = TI11*TI5+TI12*TI4                                             
            CR4 = TI12*TR5-TI11*TR4                                             
            CI4 = TI12*TI5-TI11*TI4                                             
            DR3 = CR3-CI4                                                       
            DR4 = CR3+CI4                                                       
            DI3 = CI3+CR4                                                       
            DI4 = CI3-CR4                                                       
            DR5 = CR2+CI5                                                       
            DR2 = CR2-CI5                                                       
            DI5 = CI2-CR5                                                       
            DI2 = CI2+CR5                                                       
            CH(I-1,K,2) = WA1(I-2)*DR2-WA1(I-1)*DI2                             
            CH(I,K,2) = WA1(I-2)*DI2+WA1(I-1)*DR2                               
            CH(I-1,K,3) = WA2(I-2)*DR3-WA2(I-1)*DI3                             
            CH(I,K,3) = WA2(I-2)*DI3+WA2(I-1)*DR3                               
            CH(I-1,K,4) = WA3(I-2)*DR4-WA3(I-1)*DI4                             
            CH(I,K,4) = WA3(I-2)*DI4+WA3(I-1)*DR4                               
            CH(I-1,K,5) = WA4(I-2)*DR5-WA4(I-1)*DI5                             
            CH(I,K,5) = WA4(I-2)*DI5+WA4(I-1)*DR5                               
  102    CONTINUE                                                               
  103 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
      SUBROUTINE RADBG (IDO,IP,L1,IDL1,CC,C1,C2,CH,CH2,WA)                      
      DIMENSION       CH(IDO,L1,IP)          ,CC(IDO,IP,L1)          ,          
     1                C1(IDO,L1,IP)          ,C2(IDL1,IP),                      
     2                CH2(IDL1,IP)           ,WA(*)                             
      TPI = 2.0*PIMACH(DUM)                                                     
      ARG = TPI/FLOAT(IP)                                                       
      DCP = COS(ARG)                                                            
      DSP = SIN(ARG)                                                            
      IDP2 = IDO+2                                                              
      NBD = (IDO-1)/2                                                           
      IPP2 = IP+2                                                               
      IPPH = (IP+1)/2                                                           
      IF (IDO .LT. L1) GO TO 103                                                
      DO 102 K=1,L1                                                             
         DO 101 I=1,IDO                                                         
            CH(I,K,1) = CC(I,1,K)                                               
  101    CONTINUE                                                               
  102 CONTINUE                                                                  
      GO TO 106                                                                 
  103 DO 105 I=1,IDO                                                            
         DO 104 K=1,L1                                                          
            CH(I,K,1) = CC(I,1,K)                                               
  104    CONTINUE                                                               
  105 CONTINUE                                                                  
  106 DO 108 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         J2 = J+J                                                               
         DO 107 K=1,L1                                                          
            CH(1,K,J) = CC(IDO,J2-2,K)+CC(IDO,J2-2,K)                           
            CH(1,K,JC) = CC(1,J2-1,K)+CC(1,J2-1,K)                              
  107    CONTINUE                                                               
  108 CONTINUE                                                                  
      IF (IDO .EQ. 1) GO TO 116                                                 
      IF (NBD .LT. L1) GO TO 112                                                
      DO 111 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         DO 110 K=1,L1                                                          
            DO 109 I=3,IDO,2                                                    
               IC = IDP2-I                                                      
               CH(I-1,K,J) = CC(I-1,2*J-1,K)+CC(IC-1,2*J-2,K)                   
               CH(I-1,K,JC) = CC(I-1,2*J-1,K)-CC(IC-1,2*J-2,K)                  
               CH(I,K,J) = CC(I,2*J-1,K)-CC(IC,2*J-2,K)                         
               CH(I,K,JC) = CC(I,2*J-1,K)+CC(IC,2*J-2,K)                        
  109       CONTINUE                                                            
  110    CONTINUE                                                               
  111 CONTINUE                                                                  
      GO TO 116                                                                 
  112 DO 115 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         DO 114 I=3,IDO,2                                                       
            IC = IDP2-I                                                         
            DO 113 K=1,L1                                                       
               CH(I-1,K,J) = CC(I-1,2*J-1,K)+CC(IC-1,2*J-2,K)                   
               CH(I-1,K,JC) = CC(I-1,2*J-1,K)-CC(IC-1,2*J-2,K)                  
               CH(I,K,J) = CC(I,2*J-1,K)-CC(IC,2*J-2,K)                         
               CH(I,K,JC) = CC(I,2*J-1,K)+CC(IC,2*J-2,K)                        
  113       CONTINUE                                                            
  114    CONTINUE                                                               
  115 CONTINUE                                                                  
  116 AR1 = 1.                                                                  
      AI1 = 0.                                                                  
      DO 120 L=2,IPPH                                                           
         LC = IPP2-L                                                            
         AR1H = DCP*AR1-DSP*AI1                                                 
         AI1 = DCP*AI1+DSP*AR1                                                  
         AR1 = AR1H                                                             
         DO 117 IK=1,IDL1                                                       
            C2(IK,L) = CH2(IK,1)+AR1*CH2(IK,2)                                  
            C2(IK,LC) = AI1*CH2(IK,IP)                                          
  117    CONTINUE                                                               
         DC2 = AR1                                                              
         DS2 = AI1                                                              
         AR2 = AR1                                                              
         AI2 = AI1                                                              
         DO 119 J=3,IPPH                                                        
            JC = IPP2-J                                                         
            AR2H = DC2*AR2-DS2*AI2                                              
            AI2 = DC2*AI2+DS2*AR2                                               
            AR2 = AR2H                                                          
            DO 118 IK=1,IDL1                                                    
               C2(IK,L) = C2(IK,L)+AR2*CH2(IK,J)                                
               C2(IK,LC) = C2(IK,LC)+AI2*CH2(IK,JC)                             
  118       CONTINUE                                                            
  119    CONTINUE                                                               
  120 CONTINUE                                                                  
      DO 122 J=2,IPPH                                                           
         DO 121 IK=1,IDL1                                                       
            CH2(IK,1) = CH2(IK,1)+CH2(IK,J)                                     
  121    CONTINUE                                                               
  122 CONTINUE                                                                  
      DO 124 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         DO 123 K=1,L1                                                          
            CH(1,K,J) = C1(1,K,J)-C1(1,K,JC)                                    
            CH(1,K,JC) = C1(1,K,J)+C1(1,K,JC)                                   
  123    CONTINUE                                                               
  124 CONTINUE                                                                  
      IF (IDO .EQ. 1) GO TO 132                                                 
      IF (NBD .LT. L1) GO TO 128                                                
      DO 127 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         DO 126 K=1,L1                                                          
            DO 125 I=3,IDO,2                                                    
               CH(I-1,K,J) = C1(I-1,K,J)-C1(I,K,JC)                             
               CH(I-1,K,JC) = C1(I-1,K,J)+C1(I,K,JC)                            
               CH(I,K,J) = C1(I,K,J)+C1(I-1,K,JC)                               
               CH(I,K,JC) = C1(I,K,J)-C1(I-1,K,JC)                              
  125       CONTINUE                                                            
  126    CONTINUE                                                               
  127 CONTINUE                                                                  
      GO TO 132                                                                 
  128 DO 131 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         DO 130 I=3,IDO,2                                                       
            DO 129 K=1,L1                                                       
               CH(I-1,K,J) = C1(I-1,K,J)-C1(I,K,JC)                             
               CH(I-1,K,JC) = C1(I-1,K,J)+C1(I,K,JC)                            
               CH(I,K,J) = C1(I,K,J)+C1(I-1,K,JC)                               
               CH(I,K,JC) = C1(I,K,J)-C1(I-1,K,JC)                              
  129       CONTINUE                                                            
  130    CONTINUE                                                               
  131 CONTINUE                                                                  
  132 CONTINUE                                                                  
      IF (IDO .EQ. 1) RETURN                                                    
      DO 133 IK=1,IDL1                                                          
         C2(IK,1) = CH2(IK,1)                                                   
  133 CONTINUE                                                                  
      DO 135 J=2,IP                                                             
         DO 134 K=1,L1                                                          
            C1(1,K,J) = CH(1,K,J)                                               
  134    CONTINUE                                                               
  135 CONTINUE                                                                  
      IF (NBD .GT. L1) GO TO 139                                                
      IS = -IDO                                                                 
      DO 138 J=2,IP                                                             
         IS = IS+IDO                                                            
         IDIJ = IS                                                              
         DO 137 I=3,IDO,2                                                       
            IDIJ = IDIJ+2                                                       
            DO 136 K=1,L1                                                       
               C1(I-1,K,J) = WA(IDIJ-1)*CH(I-1,K,J)-WA(IDIJ)*CH(I,K,J)          
               C1(I,K,J) = WA(IDIJ-1)*CH(I,K,J)+WA(IDIJ)*CH(I-1,K,J)            
  136       CONTINUE                                                            
  137    CONTINUE                                                               
  138 CONTINUE                                                                  
      GO TO 143                                                                 
  139 IS = -IDO                                                                 
      DO 142 J=2,IP                                                             
         IS = IS+IDO                                                            
         DO 141 K=1,L1                                                          
            IDIJ = IS                                                           
            DO 140 I=3,IDO,2                                                    
               IDIJ = IDIJ+2                                                    
               C1(I-1,K,J) = WA(IDIJ-1)*CH(I-1,K,J)-WA(IDIJ)*CH(I,K,J)          
               C1(I,K,J) = WA(IDIJ-1)*CH(I,K,J)+WA(IDIJ)*CH(I-1,K,J)            
  140       CONTINUE                                                            
  141    CONTINUE                                                               
  142 CONTINUE                                                                  
  143 RETURN                                                                    
      END                                                                       
      SUBROUTINE RADF2 (IDO,L1,CC,CH,WA1)                                       
      DIMENSION       CH(IDO,2,L1)           ,CC(IDO,L1,2)           ,          
     1                WA1(*)                                                    
      DO 101 K=1,L1                                                             
         CH(1,1,K) = CC(1,K,1)+CC(1,K,2)                                        
         CH(IDO,2,K) = CC(1,K,1)-CC(1,K,2)                                      
  101 CONTINUE                                                                  
      IF (IDO-2) 107,105,102                                                    
  102 IDP2 = IDO+2                                                              
      DO 104 K=1,L1                                                             
         DO 103 I=3,IDO,2                                                       
            IC = IDP2-I                                                         
            TR2 = WA1(I-2)*CC(I-1,K,2)+WA1(I-1)*CC(I,K,2)                       
            TI2 = WA1(I-2)*CC(I,K,2)-WA1(I-1)*CC(I-1,K,2)                       
            CH(I,1,K) = CC(I,K,1)+TI2                                           
            CH(IC,2,K) = TI2-CC(I,K,1)                                          
            CH(I-1,1,K) = CC(I-1,K,1)+TR2                                       
            CH(IC-1,2,K) = CC(I-1,K,1)-TR2                                      
  103    CONTINUE                                                               
  104 CONTINUE                                                                  
      IF (MOD(IDO,2) .EQ. 1) RETURN                                             
  105 DO 106 K=1,L1                                                             
         CH(1,2,K) = -CC(IDO,K,2)                                               
         CH(IDO,1,K) = CC(IDO,K,1)                                              
  106 CONTINUE                                                                  
  107 RETURN                                                                    
      END                                                                       
      SUBROUTINE RADF3 (IDO,L1,CC,CH,WA1,WA2)                                   
      DIMENSION       CH(IDO,3,L1)           ,CC(IDO,L1,3)           ,          
     1                WA1(*)     ,WA2(*)                                        
      DATA TAUR,TAUI /-.5,.866025403784439/                                     
      DO 101 K=1,L1                                                             
         CR2 = CC(1,K,2)+CC(1,K,3)                                              
         CH(1,1,K) = CC(1,K,1)+CR2                                              
         CH(1,3,K) = TAUI*(CC(1,K,3)-CC(1,K,2))                                 
         CH(IDO,2,K) = CC(1,K,1)+TAUR*CR2                                       
  101 CONTINUE                                                                  
      IF (IDO .EQ. 1) RETURN                                                    
      IDP2 = IDO+2                                                              
      DO 103 K=1,L1                                                             
         DO 102 I=3,IDO,2                                                       
            IC = IDP2-I                                                         
            DR2 = WA1(I-2)*CC(I-1,K,2)+WA1(I-1)*CC(I,K,2)                       
            DI2 = WA1(I-2)*CC(I,K,2)-WA1(I-1)*CC(I-1,K,2)                       
            DR3 = WA2(I-2)*CC(I-1,K,3)+WA2(I-1)*CC(I,K,3)                       
            DI3 = WA2(I-2)*CC(I,K,3)-WA2(I-1)*CC(I-1,K,3)                       
            CR2 = DR2+DR3                                                       
            CI2 = DI2+DI3                                                       
            CH(I-1,1,K) = CC(I-1,K,1)+CR2                                       
            CH(I,1,K) = CC(I,K,1)+CI2                                           
            TR2 = CC(I-1,K,1)+TAUR*CR2                                          
            TI2 = CC(I,K,1)+TAUR*CI2                                            
            TR3 = TAUI*(DI2-DI3)                                                
            TI3 = TAUI*(DR3-DR2)                                                
            CH(I-1,3,K) = TR2+TR3                                               
            CH(IC-1,2,K) = TR2-TR3                                              
            CH(I,3,K) = TI2+TI3                                                 
            CH(IC,2,K) = TI3-TI2                                                
  102    CONTINUE                                                               
  103 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
      SUBROUTINE RADF4 (IDO,L1,CC,CH,WA1,WA2,WA3)                               
      DIMENSION       CC(IDO,L1,4)           ,CH(IDO,4,L1)           ,          
     1                WA1(*)     ,WA2(*)     ,WA3(*)                            
      DATA HSQT2 /.7071067811865475/                                            
      DO 101 K=1,L1                                                             
         TR1 = CC(1,K,2)+CC(1,K,4)                                              
         TR2 = CC(1,K,1)+CC(1,K,3)                                              
         CH(1,1,K) = TR1+TR2                                                    
         CH(IDO,4,K) = TR2-TR1                                                  
         CH(IDO,2,K) = CC(1,K,1)-CC(1,K,3)                                      
         CH(1,3,K) = CC(1,K,4)-CC(1,K,2)                                        
  101 CONTINUE                                                                  
      IF (IDO-2) 107,105,102                                                    
  102 IDP2 = IDO+2                                                              
      DO 104 K=1,L1                                                             
         DO 103 I=3,IDO,2                                                       
            IC = IDP2-I                                                         
            CR2 = WA1(I-2)*CC(I-1,K,2)+WA1(I-1)*CC(I,K,2)                       
            CI2 = WA1(I-2)*CC(I,K,2)-WA1(I-1)*CC(I-1,K,2)                       
            CR3 = WA2(I-2)*CC(I-1,K,3)+WA2(I-1)*CC(I,K,3)                       
            CI3 = WA2(I-2)*CC(I,K,3)-WA2(I-1)*CC(I-1,K,3)                       
            CR4 = WA3(I-2)*CC(I-1,K,4)+WA3(I-1)*CC(I,K,4)                       
            CI4 = WA3(I-2)*CC(I,K,4)-WA3(I-1)*CC(I-1,K,4)                       
            TR1 = CR2+CR4                                                       
            TR4 = CR4-CR2                                                       
            TI1 = CI2+CI4                                                       
            TI4 = CI2-CI4                                                       
            TI2 = CC(I,K,1)+CI3                                                 
            TI3 = CC(I,K,1)-CI3                                                 
            TR2 = CC(I-1,K,1)+CR3                                               
            TR3 = CC(I-1,K,1)-CR3                                               
            CH(I-1,1,K) = TR1+TR2                                               
            CH(IC-1,4,K) = TR2-TR1                                              
            CH(I,1,K) = TI1+TI2                                                 
            CH(IC,4,K) = TI1-TI2                                                
            CH(I-1,3,K) = TI4+TR3                                               
            CH(IC-1,2,K) = TR3-TI4                                              
            CH(I,3,K) = TR4+TI3                                                 
            CH(IC,2,K) = TR4-TI3                                                
  103    CONTINUE                                                               
  104 CONTINUE                                                                  
      IF (MOD(IDO,2) .EQ. 1) RETURN                                             
  105 CONTINUE                                                                  
      DO 106 K=1,L1                                                             
         TI1 = -HSQT2*(CC(IDO,K,2)+CC(IDO,K,4))                                 
         TR1 = HSQT2*(CC(IDO,K,2)-CC(IDO,K,4))                                  
         CH(IDO,1,K) = TR1+CC(IDO,K,1)                                          
         CH(IDO,3,K) = CC(IDO,K,1)-TR1                                          
         CH(1,2,K) = TI1-CC(IDO,K,3)                                            
         CH(1,4,K) = TI1+CC(IDO,K,3)                                            
  106 CONTINUE                                                                  
  107 RETURN                                                                    
      END                                                                       
      SUBROUTINE RADF5 (IDO,L1,CC,CH,WA1,WA2,WA3,WA4)                           
      DIMENSION       CC(IDO,L1,5)           ,CH(IDO,5,L1)           ,          
     1                WA1(*)     ,WA2(*)     ,WA3(*)     ,WA4(*)                
      DATA TR11,TI11,TR12,TI12 /.309016994374947,.951056516295154,              
     1-.809016994374947,.587785252292473/                                       
      DO 101 K=1,L1                                                             
         CR2 = CC(1,K,5)+CC(1,K,2)                                              
         CI5 = CC(1,K,5)-CC(1,K,2)                                              
         CR3 = CC(1,K,4)+CC(1,K,3)                                              
         CI4 = CC(1,K,4)-CC(1,K,3)                                              
         CH(1,1,K) = CC(1,K,1)+CR2+CR3                                          
         CH(IDO,2,K) = CC(1,K,1)+TR11*CR2+TR12*CR3                              
         CH(1,3,K) = TI11*CI5+TI12*CI4                                          
         CH(IDO,4,K) = CC(1,K,1)+TR12*CR2+TR11*CR3                              
         CH(1,5,K) = TI12*CI5-TI11*CI4                                          
  101 CONTINUE                                                                  
      IF (IDO .EQ. 1) RETURN                                                    
      IDP2 = IDO+2                                                              
      DO 103 K=1,L1                                                             
         DO 102 I=3,IDO,2                                                       
            IC = IDP2-I                                                         
            DR2 = WA1(I-2)*CC(I-1,K,2)+WA1(I-1)*CC(I,K,2)                       
            DI2 = WA1(I-2)*CC(I,K,2)-WA1(I-1)*CC(I-1,K,2)                       
            DR3 = WA2(I-2)*CC(I-1,K,3)+WA2(I-1)*CC(I,K,3)                       
            DI3 = WA2(I-2)*CC(I,K,3)-WA2(I-1)*CC(I-1,K,3)                       
            DR4 = WA3(I-2)*CC(I-1,K,4)+WA3(I-1)*CC(I,K,4)                       
            DI4 = WA3(I-2)*CC(I,K,4)-WA3(I-1)*CC(I-1,K,4)                       
            DR5 = WA4(I-2)*CC(I-1,K,5)+WA4(I-1)*CC(I,K,5)                       
            DI5 = WA4(I-2)*CC(I,K,5)-WA4(I-1)*CC(I-1,K,5)                       
            CR2 = DR2+DR5                                                       
            CI5 = DR5-DR2                                                       
            CR5 = DI2-DI5                                                       
            CI2 = DI2+DI5                                                       
            CR3 = DR3+DR4                                                       
            CI4 = DR4-DR3                                                       
            CR4 = DI3-DI4                                                       
            CI3 = DI3+DI4                                                       
            CH(I-1,1,K) = CC(I-1,K,1)+CR2+CR3                                   
            CH(I,1,K) = CC(I,K,1)+CI2+CI3                                       
            TR2 = CC(I-1,K,1)+TR11*CR2+TR12*CR3                                 
            TI2 = CC(I,K,1)+TR11*CI2+TR12*CI3                                   
            TR3 = CC(I-1,K,1)+TR12*CR2+TR11*CR3                                 
            TI3 = CC(I,K,1)+TR12*CI2+TR11*CI3                                   
            TR5 = TI11*CR5+TI12*CR4                                             
            TI5 = TI11*CI5+TI12*CI4                                             
            TR4 = TI12*CR5-TI11*CR4                                             
            TI4 = TI12*CI5-TI11*CI4                                             
            CH(I-1,3,K) = TR2+TR5                                               
            CH(IC-1,2,K) = TR2-TR5                                              
            CH(I,3,K) = TI2+TI5                                                 
            CH(IC,2,K) = TI5-TI2                                                
            CH(I-1,5,K) = TR3+TR4                                               
            CH(IC-1,4,K) = TR3-TR4                                              
            CH(I,5,K) = TI3+TI4                                                 
            CH(IC,4,K) = TI4-TI3                                                
  102    CONTINUE                                                               
  103 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
      SUBROUTINE RADFG (IDO,IP,L1,IDL1,CC,C1,C2,CH,CH2,WA)                      
      DIMENSION       CH(IDO,L1,IP)          ,CC(IDO,IP,L1)          ,          
     1                C1(IDO,L1,IP)          ,C2(IDL1,IP),                      
     2                CH2(IDL1,IP)           ,WA(*)                             
      TPI = 2.0*PIMACH(DUM)                                                     
      ARG = TPI/FLOAT(IP)                                                       
      DCP = COS(ARG)                                                            
      DSP = SIN(ARG)                                                            
      IPPH = (IP+1)/2                                                           
      IPP2 = IP+2                                                               
      IDP2 = IDO+2                                                              
      NBD = (IDO-1)/2                                                           
      IF (IDO .EQ. 1) GO TO 119                                                 
      DO 101 IK=1,IDL1                                                          
         CH2(IK,1) = C2(IK,1)                                                   
  101 CONTINUE                                                                  
      DO 103 J=2,IP                                                             
         DO 102 K=1,L1                                                          
            CH(1,K,J) = C1(1,K,J)                                               
  102    CONTINUE                                                               
  103 CONTINUE                                                                  
      IF (NBD .GT. L1) GO TO 107                                                
      IS = -IDO                                                                 
      DO 106 J=2,IP                                                             
         IS = IS+IDO                                                            
         IDIJ = IS                                                              
         DO 105 I=3,IDO,2                                                       
            IDIJ = IDIJ+2                                                       
            DO 104 K=1,L1                                                       
               CH(I-1,K,J) = WA(IDIJ-1)*C1(I-1,K,J)+WA(IDIJ)*C1(I,K,J)          
               CH(I,K,J) = WA(IDIJ-1)*C1(I,K,J)-WA(IDIJ)*C1(I-1,K,J)            
  104       CONTINUE                                                            
  105    CONTINUE                                                               
  106 CONTINUE                                                                  
      GO TO 111                                                                 
  107 IS = -IDO                                                                 
      DO 110 J=2,IP                                                             
         IS = IS+IDO                                                            
         DO 109 K=1,L1                                                          
            IDIJ = IS                                                           
            DO 108 I=3,IDO,2                                                    
               IDIJ = IDIJ+2                                                    
               CH(I-1,K,J) = WA(IDIJ-1)*C1(I-1,K,J)+WA(IDIJ)*C1(I,K,J)          
               CH(I,K,J) = WA(IDIJ-1)*C1(I,K,J)-WA(IDIJ)*C1(I-1,K,J)            
  108       CONTINUE                                                            
  109    CONTINUE                                                               
  110 CONTINUE                                                                  
  111 IF (NBD .LT. L1) GO TO 115                                                
      DO 114 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         DO 113 K=1,L1                                                          
            DO 112 I=3,IDO,2                                                    
               C1(I-1,K,J) = CH(I-1,K,J)+CH(I-1,K,JC)                           
               C1(I-1,K,JC) = CH(I,K,J)-CH(I,K,JC)                              
               C1(I,K,J) = CH(I,K,J)+CH(I,K,JC)                                 
               C1(I,K,JC) = CH(I-1,K,JC)-CH(I-1,K,J)                            
  112       CONTINUE                                                            
  113    CONTINUE                                                               
  114 CONTINUE                                                                  
      GO TO 121                                                                 
  115 DO 118 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         DO 117 I=3,IDO,2                                                       
            DO 116 K=1,L1                                                       
               C1(I-1,K,J) = CH(I-1,K,J)+CH(I-1,K,JC)                           
               C1(I-1,K,JC) = CH(I,K,J)-CH(I,K,JC)                              
               C1(I,K,J) = CH(I,K,J)+CH(I,K,JC)                                 
               C1(I,K,JC) = CH(I-1,K,JC)-CH(I-1,K,J)                            
  116       CONTINUE                                                            
  117    CONTINUE                                                               
  118 CONTINUE                                                                  
      GO TO 121                                                                 
  119 DO 120 IK=1,IDL1                                                          
         C2(IK,1) = CH2(IK,1)                                                   
  120 CONTINUE                                                                  
  121 DO 123 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         DO 122 K=1,L1                                                          
            C1(1,K,J) = CH(1,K,J)+CH(1,K,JC)                                    
            C1(1,K,JC) = CH(1,K,JC)-CH(1,K,J)                                   
  122    CONTINUE                                                               
  123 CONTINUE                                                                  
C                                                                               
      AR1 = 1.                                                                  
      AI1 = 0.                                                                  
      DO 127 L=2,IPPH                                                           
         LC = IPP2-L                                                            
         AR1H = DCP*AR1-DSP*AI1                                                 
         AI1 = DCP*AI1+DSP*AR1                                                  
         AR1 = AR1H                                                             
         DO 124 IK=1,IDL1                                                       
            CH2(IK,L) = C2(IK,1)+AR1*C2(IK,2)                                   
            CH2(IK,LC) = AI1*C2(IK,IP)                                          
  124    CONTINUE                                                               
         DC2 = AR1                                                              
         DS2 = AI1                                                              
         AR2 = AR1                                                              
         AI2 = AI1                                                              
         DO 126 J=3,IPPH                                                        
            JC = IPP2-J                                                         
            AR2H = DC2*AR2-DS2*AI2                                              
            AI2 = DC2*AI2+DS2*AR2                                               
            AR2 = AR2H                                                          
            DO 125 IK=1,IDL1                                                    
               CH2(IK,L) = CH2(IK,L)+AR2*C2(IK,J)                               
               CH2(IK,LC) = CH2(IK,LC)+AI2*C2(IK,JC)                            
  125       CONTINUE                                                            
  126    CONTINUE                                                               
  127 CONTINUE                                                                  
      DO 129 J=2,IPPH                                                           
         DO 128 IK=1,IDL1                                                       
            CH2(IK,1) = CH2(IK,1)+C2(IK,J)                                      
  128    CONTINUE                                                               
  129 CONTINUE                                                                  
C                                                                               
      IF (IDO .LT. L1) GO TO 132                                                
      DO 131 K=1,L1                                                             
         DO 130 I=1,IDO                                                         
            CC(I,1,K) = CH(I,K,1)                                               
  130    CONTINUE                                                               
  131 CONTINUE                                                                  
      GO TO 135                                                                 
  132 DO 134 I=1,IDO                                                            
         DO 133 K=1,L1                                                          
            CC(I,1,K) = CH(I,K,1)                                               
  133    CONTINUE                                                               
  134 CONTINUE                                                                  
  135 DO 137 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         J2 = J+J                                                               
         DO 136 K=1,L1                                                          
            CC(IDO,J2-2,K) = CH(1,K,J)                                          
            CC(1,J2-1,K) = CH(1,K,JC)                                           
  136    CONTINUE                                                               
  137 CONTINUE                                                                  
      IF (IDO .EQ. 1) RETURN                                                    
      IF (NBD .LT. L1) GO TO 141                                                
      DO 140 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         J2 = J+J                                                               
         DO 139 K=1,L1                                                          
            DO 138 I=3,IDO,2                                                    
               IC = IDP2-I                                                      
               CC(I-1,J2-1,K) = CH(I-1,K,J)+CH(I-1,K,JC)                        
               CC(IC-1,J2-2,K) = CH(I-1,K,J)-CH(I-1,K,JC)                       
               CC(I,J2-1,K) = CH(I,K,J)+CH(I,K,JC)                              
               CC(IC,J2-2,K) = CH(I,K,JC)-CH(I,K,J)                             
  138       CONTINUE                                                            
  139    CONTINUE                                                               
  140 CONTINUE                                                                  
      RETURN                                                                    
  141 DO 144 J=2,IPPH                                                           
         JC = IPP2-J                                                            
         J2 = J+J                                                               
         DO 143 I=3,IDO,2                                                       
            IC = IDP2-I                                                         
            DO 142 K=1,L1                                                       
               CC(I-1,J2-1,K) = CH(I-1,K,J)+CH(I-1,K,JC)                        
               CC(IC-1,J2-2,K) = CH(I-1,K,J)-CH(I-1,K,JC)                       
               CC(I,J2-1,K) = CH(I,K,J)+CH(I,K,JC)                              
               CC(IC,J2-2,K) = CH(I,K,JC)-CH(I,K,J)                             
  142       CONTINUE                                                            
  143    CONTINUE                                                               
  144 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE RFFTB(N,R,WSAVE)                                               
C                                                                               
C     SUBROUTINE RFFTB COMPUTES THE REAL PERODIC SEQUENCE FROM ITS              
C     FOURIER COEFFICIENTS (FOURIER SYNTHESIS). THE TRANSFORM IS DEFINED        
C     BELOW AT OUTPUT PARAMETER R.                                              
C                                                                               
C     INPUT PARAMETERS                                                          
C                                                                               
C     N       THE LENGTH OF THE ARRAY R TO BE TRANSFORMED.  THE METHOD          
C             IS MOST EFFICIENT WHEN N IS A PRODUCT OF SMALL PRIMES.            
C             N MAY CHANGE SO LONG AS DIFFERENT WORK ARRAYS ARE PROVIDED        
C                                                                               
C     R       A REAL ARRAY OF LENGTH N WHICH CONTAINS THE SEQUENCE              
C             TO BE TRANSFORMED                                                 
C                                                                               
C     WSAVE   A WORK ARRAY WHICH MUST BE DIMENSIONED AT LEAST 2*N+15.           
C             IN THE PROGRAM THAT CALLS RFFTB. THE WSAVE ARRAY MUST BE          
C             INITIALIZED BY CALLING SUBROUTINE RFFTI(N,WSAVE) AND A            
C             DIFFERENT WSAVE ARRAY MUST BE USED FOR EACH DIFFERENT             
C             VALUE OF N. THIS INITIALIZATION DOES NOT HAVE TO BE               
C             REPEATED SO LONG AS N REMAINS UNCHANGED THUS SUBSEQUENT           
C             TRANSFORMS CAN BE OBTAINED FASTER THAN THE FIRST.                 
C             THE SAME WSAVE ARRAY CAN BE USED BY RFFTF AND RFFTB.              
C                                                                               
C                                                                               
C     OUTPUT PARAMETERS                                                         
C                                                                               
C     R       FOR N EVEN AND FOR I = 1,...,N                                    
C                                                                               
C                  R(I) = R(1)+(-1)**(I-1)*R(N)                                 
C                                                                               
C                       PLUS THE SUM FROM K=2 TO K=N/2 OF                       
C                                                                               
C                        2.*R(2*K-2)*COS((K-1)*(I-1)*2*PI/N)                    
C                                                                               
C                       -2.*R(2*K-1)*SIN((K-1)*(I-1)*2*PI/N)                    
C                                                                               
C             FOR N ODD AND FOR I = 1,...,N                                     
C                                                                               
C                  R(I) = R(1) PLUS THE SUM FROM K=2 TO K=(N+1)/2 OF            
C                                                                               
C                       2.*R(2*K-2)*COS((K-1)*(I-1)*2*PI/N)                     
C                                                                               
C                      -2.*R(2*K-1)*SIN((K-1)*(I-1)*2*PI/N)                     
C                                                                               
C      *****  NOTE                                                              
C                  THIS TRANSFORM IS UNNORMALIZED SINCE A CALL OF RFFTF         
C                  FOLLOWED BY A CALL OF RFFTB WILL MULTIPLY THE INPUT          
C                  SEQUENCE BY N.                                               
C                                                                               
C     WSAVE   CONTAINS RESULTS WHICH MUST NOT BE DESTROYED BETWEEN              
C             CALLS OF RFFTB OR RFFTF.                                          
C                                                                               
C                                                                               
      SUBROUTINE RFFTB (N,R,WSAVE)                                              
      DIMENSION       R(*)       ,WSAVE(*)                                      
C                                                                               
      IF (N .EQ. 1) RETURN                                                      
      CALL RFFTB1 (N,R,WSAVE,WSAVE(N+1),WSAVE(2*N+1))                           
      RETURN                                                                    
      END                                                                       
      SUBROUTINE RFFTB1 (N,C,CH,WA,IFAC)                                        
      DIMENSION       CH(*)      ,C(*)       ,WA(*)      ,IFAC(*)               
      NF = IFAC(2)                                                              
      NA = 0                                                                    
      L1 = 1                                                                    
      IW = 1                                                                    
      DO 116 K1=1,NF                                                            
         IP = IFAC(K1+2)                                                        
         L2 = IP*L1                                                             
         IDO = N/L2                                                             
         IDL1 = IDO*L1                                                          
         IF (IP .NE. 4) GO TO 103                                               
         IX2 = IW+IDO                                                           
         IX3 = IX2+IDO                                                          
         IF (NA .NE. 0) GO TO 101                                               
         CALL RADB4 (IDO,L1,C,CH,WA(IW),WA(IX2),WA(IX3))                        
         GO TO 102                                                              
  101    CALL RADB4 (IDO,L1,CH,C,WA(IW),WA(IX2),WA(IX3))                        
  102    NA = 1-NA                                                              
         GO TO 115                                                              
  103    IF (IP .NE. 2) GO TO 106                                               
         IF (NA .NE. 0) GO TO 104                                               
         CALL RADB2 (IDO,L1,C,CH,WA(IW))                                        
         GO TO 105                                                              
  104    CALL RADB2 (IDO,L1,CH,C,WA(IW))                                        
  105    NA = 1-NA                                                              
         GO TO 115                                                              
  106    IF (IP .NE. 3) GO TO 109                                               
         IX2 = IW+IDO                                                           
         IF (NA .NE. 0) GO TO 107                                               
         CALL RADB3 (IDO,L1,C,CH,WA(IW),WA(IX2))                                
         GO TO 108                                                              
  107    CALL RADB3 (IDO,L1,CH,C,WA(IW),WA(IX2))                                
  108    NA = 1-NA                                                              
         GO TO 115                                                              
  109    IF (IP .NE. 5) GO TO 112                                               
         IX2 = IW+IDO                                                           
         IX3 = IX2+IDO                                                          
         IX4 = IX3+IDO                                                          
         IF (NA .NE. 0) GO TO 110                                               
         CALL RADB5 (IDO,L1,C,CH,WA(IW),WA(IX2),WA(IX3),WA(IX4))                
         GO TO 111                                                              
  110    CALL RADB5 (IDO,L1,CH,C,WA(IW),WA(IX2),WA(IX3),WA(IX4))                
  111    NA = 1-NA                                                              
         GO TO 115                                                              
  112    IF (NA .NE. 0) GO TO 113                                               
         CALL RADBG (IDO,IP,L1,IDL1,C,C,C,CH,CH,WA(IW))                         
         GO TO 114                                                              
  113    CALL RADBG (IDO,IP,L1,IDL1,CH,CH,CH,C,C,WA(IW))                        
  114    IF (IDO .EQ. 1) NA = 1-NA                                              
  115    L1 = L2                                                                
         IW = IW+(IP-1)*IDO                                                     
  116 CONTINUE                                                                  
      IF (NA .EQ. 0) RETURN                                                     
      DO 117 I=1,N                                                              
         C(I) = CH(I)                                                           
  117 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE RFFTF(N,R,WSAVE)                                               
C                                                                               
C     SUBROUTINE RFFTF COMPUTES THE FOURIER COEFFICIENTS OF A REAL              
C     PERODIC SEQUENCE (FOURIER ANALYSIS). THE TRANSFORM IS DEFINED             
C     BELOW AT OUTPUT PARAMETER R.                                              
C                                                                               
C     INPUT PARAMETERS                                                          
C                                                                               
C     N       THE LENGTH OF THE ARRAY R TO BE TRANSFORMED.  THE METHOD          
C             IS MOST EFFICIENT WHEN N IS A PRODUCT OF SMALL PRIMES.            
C             N MAY CHANGE SO LONG AS DIFFERENT WORK ARRAYS ARE PROVIDED        
C                                                                               
C     R       A REAL ARRAY OF LENGTH N WHICH CONTAINS THE SEQUENCE              
C             TO BE TRANSFORMED                                                 
C                                                                               
C     WSAVE   A WORK ARRAY WHICH MUST BE DIMENSIONED AT LEAST 2*N+15.           
C             IN THE PROGRAM THAT CALLS RFFTF. THE WSAVE ARRAY MUST BE          
C             INITIALIZED BY CALLING SUBROUTINE RFFTI(N,WSAVE) AND A            
C             DIFFERENT WSAVE ARRAY MUST BE USED FOR EACH DIFFERENT             
C             VALUE OF N. THIS INITIALIZATION DOES NOT HAVE TO BE               
C             REPEATED SO LONG AS N REMAINS UNCHANGED THUS SUBSEQUENT           
C             TRANSFORMS CAN BE OBTAINED FASTER THAN THE FIRST.                 
C             THE SAME WSAVE ARRAY CAN BE USED BY RFFTF AND RFFTB.              
C                                                                               
C                                                                               
C     OUTPUT PARAMETERS                                                         
C                                                                               
C     R       R(1) = THE SUM FROM I=1 TO I=N OF R(I)                            
C                                                                               
C             IF N IS EVEN SET L =N/2   , IF N IS ODD SET L = (N+1)/2           
C                                                                               
C               THEN FOR K = 2,...,L                                            
C                                                                               
C                  R(2*K-2) = THE SUM FROM I = 1 TO I = N OF                    
C                                                                               
C                       R(I)*COS((K-1)*(I-1)*2*PI/N)                            
C                                                                               
C                  R(2*K-1) = THE SUM FROM I = 1 TO I = N OF                    
C                                                                               
C                      -R(I)*SIN((K-1)*(I-1)*2*PI/N)                            
C                                                                               
C             IF N IS EVEN                                                      
C                                                                               
C                  R(N) = THE SUM FROM I = 1 TO I = N OF                        
C                                                                               
C                       (-1)**(I-1)*R(I)                                        
C                                                                               
C      *****  NOTE                                                              
C                  THIS TRANSFORM IS UNNORMALIZED SINCE A CALL OF RFFTF         
C                  FOLLOWED BY A CALL OF RFFTB WILL MULTIPLY THE INPUT          
C                  SEQUENCE BY N.                                               
C                                                                               
C     WSAVE   CONTAINS RESULTS WHICH MUST NOT BE DESTROYED BETWEEN              
C             CALLS OF RFFTF OR RFFTB.                                          
C                                                                               
      SUBROUTINE RFFTF (N,R,WSAVE)                                              
      DIMENSION       R(*)       ,WSAVE(*)                                      
C                                                                               
      IF (N .EQ. 1) RETURN                                                      
      CALL RFFTF1 (N,R,WSAVE,WSAVE(N+1),WSAVE(2*N+1))                           
      RETURN                                                                    
      END                                                                       
      SUBROUTINE RFFTF1 (N,C,CH,WA,IFAC)                                        
      DIMENSION       CH(*)      ,C(*)       ,WA(*)      ,IFAC(*)               
      NF = IFAC(2)                                                              
      NA = 1                                                                    
      L2 = N                                                                    
      IW = N                                                                    
      DO 111 K1=1,NF                                                            
         KH = NF-K1                                                             
         IP = IFAC(KH+3)                                                        
         L1 = L2/IP                                                             
         IDO = N/L2                                                             
         IDL1 = IDO*L1                                                          
         IW = IW-(IP-1)*IDO                                                     
         NA = 1-NA                                                              
         IF (IP .NE. 4) GO TO 102                                               
         IX2 = IW+IDO                                                           
         IX3 = IX2+IDO                                                          
         IF (NA .NE. 0) GO TO 101                                               
         CALL RADF4 (IDO,L1,C,CH,WA(IW),WA(IX2),WA(IX3))                        
         GO TO 110                                                              
  101    CALL RADF4 (IDO,L1,CH,C,WA(IW),WA(IX2),WA(IX3))                        
         GO TO 110                                                              
  102    IF (IP .NE. 2) GO TO 104                                               
         IF (NA .NE. 0) GO TO 103                                               
         CALL RADF2 (IDO,L1,C,CH,WA(IW))                                        
         GO TO 110                                                              
  103    CALL RADF2 (IDO,L1,CH,C,WA(IW))                                        
         GO TO 110                                                              
  104    IF (IP .NE. 3) GO TO 106                                               
         IX2 = IW+IDO                                                           
         IF (NA .NE. 0) GO TO 105                                               
         CALL RADF3 (IDO,L1,C,CH,WA(IW),WA(IX2))                                
         GO TO 110                                                              
  105    CALL RADF3 (IDO,L1,CH,C,WA(IW),WA(IX2))                                
         GO TO 110                                                              
  106    IF (IP .NE. 5) GO TO 108                                               
         IX2 = IW+IDO                                                           
         IX3 = IX2+IDO                                                          
         IX4 = IX3+IDO                                                          
         IF (NA .NE. 0) GO TO 107                                               
         CALL RADF5 (IDO,L1,C,CH,WA(IW),WA(IX2),WA(IX3),WA(IX4))                
         GO TO 110                                                              
  107    CALL RADF5 (IDO,L1,CH,C,WA(IW),WA(IX2),WA(IX3),WA(IX4))                
         GO TO 110                                                              
  108    IF (IDO .EQ. 1) NA = 1-NA                                              
         IF (NA .NE. 0) GO TO 109                                               
         CALL RADFG (IDO,IP,L1,IDL1,C,C,C,CH,CH,WA(IW))                         
         NA = 1                                                                 
         GO TO 110                                                              
  109    CALL RADFG (IDO,IP,L1,IDL1,CH,CH,CH,C,C,WA(IW))                        
         NA = 0                                                                 
  110    L2 = L1                                                                
  111 CONTINUE                                                                  
      IF (NA .EQ. 1) RETURN                                                     
      DO 112 I=1,N                                                              
         C(I) = CH(I)                                                           
  112 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE RFFTI(N,WSAVE)                                                 
C                                                                               
C     SUBROUTINE RFFTI INITIALIZES THE ARRAY WSAVE WHICH IS USED IN             
C     BOTH RFFTF AND RFFTB. THE PRIME FACTORIZATION OF N TOGETHER WITH          
C     A TABULATION OF THE TRIGONOMETRIC FUNCTIONS ARE COMPUTED AND              
C     STORED IN WSAVE.                                                          
C                                                                               
C     INPUT PARAMETER                                                           
C                                                                               
C     N       THE LENGTH OF THE SEQUENCE TO BE TRANSFORMED.                     
C                                                                               
C     OUTPUT PARAMETER                                                          
C                                                                               
C     WSAVE   A WORK ARRAY WHICH MUST BE DIMENSIONED AT LEAST 2*N+15.           
C             THE SAME WORK ARRAY CAN BE USED FOR BOTH RFFTF AND RFFTB          
C             AS LONG AS N REMAINS UNCHANGED. DIFFERENT WSAVE ARRAYS            
C             ARE REQUIRED FOR DIFFERENT VALUES OF N. THE CONTENTS OF           
C             WSAVE MUST NOT BE CHANGED BETWEEN CALLS OF RFFTF OR RFFTB.        
C                                                                               
      SUBROUTINE RFFTI (N,WSAVE)                                                
      DIMENSION       WSAVE(*)                                                  
C                                                                               
      IF (N .EQ. 1) RETURN                                                      
      CALL RFFTI1 (N,WSAVE(N+1),WSAVE(2*N+1))                                   
      RETURN                                                                    
      END                                                                       
      SUBROUTINE RFFTI1 (N,WA,IFAC)                                             
      DIMENSION       WA(*)      ,IFAC(*)    ,NTRYH(4)                          
      DATA NTRYH(1),NTRYH(2),NTRYH(3),NTRYH(4)/4,2,3,5/                         
      NL = N                                                                    
      NF = 0                                                                    
      J = 0                                                                     
  101 J = J+1                                                                   
      IF (J-4) 102,102,103                                                      
  102 NTRY = NTRYH(J)                                                           
      GO TO 104                                                                 
  103 NTRY = NTRY+2                                                             
  104 NQ = NL/NTRY                                                              
      NR = NL-NTRY*NQ                                                           
      IF (NR) 101,105,101                                                       
  105 NF = NF+1                                                                 
      IFAC(NF+2) = NTRY                                                         
      NL = NQ                                                                   
      IF (NTRY .NE. 2) GO TO 107                                                
      IF (NF .EQ. 1) GO TO 107                                                  
      DO 106 I=2,NF                                                             
         IB = NF-I+2                                                            
         IFAC(IB+2) = IFAC(IB+1)                                                
  106 CONTINUE                                                                  
      IFAC(3) = 2                                                               
  107 IF (NL .NE. 1) GO TO 104                                                  
      IFAC(1) = N                                                               
      IFAC(2) = NF                                                              
      TPI = 2.0*PIMACH(DUM)                                                     
      ARGH = TPI/FLOAT(N)                                                       
      IS = 0                                                                    
      NFM1 = NF-1                                                               
      L1 = 1                                                                    
      IF (NFM1 .EQ. 0) RETURN                                                   
      DO 110 K1=1,NFM1                                                          
         IP = IFAC(K1+2)                                                        
         LD = 0                                                                 
         L2 = L1*IP                                                             
         IDO = N/L2                                                             
         IPM = IP-1                                                             
         DO 109 J=1,IPM                                                         
            LD = LD+L1                                                          
            I = IS                                                              
            ARGLD = FLOAT(LD)*ARGH                                              
            FI = 0.                                                             
            DO 108 II=3,IDO,2                                                   
               I = I+2                                                          
               FI = FI+1.                                                       
               ARG = FI*ARGLD                                                   
               WA(I-1) = COS(ARG)                                               
               WA(I) = SIN(ARG)                                                 
  108       CONTINUE                                                            
            IS = IS+IDO                                                         
  109    CONTINUE                                                               
         L1 = L2                                                                
  110 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE SINQB(N,X,WSAVE)                                               
C                                                                               
C     SUBROUTINE SINQB COMPUTES THE FAST FOURIER TRANSFORM OF QUARTER           
C     WAVE DATA. THAT IS , SINQB COMPUTES A SEQUENCE FROM ITS                   
C     REPRESENTATION IN TERMS OF A SINE SERIES WITH ODD WAVE NUMBERS.           
C     THE TRANSFORM IS DEFINED BELOW AT OUTPUT PARAMETER X.                     
C                                                                               
C     SINQF IS THE UNNORMALIZED INVERSE OF SINQB SINCE A CALL OF SINQB          
C     FOLLOWED BY A CALL OF SINQF WILL MULTIPLY THE INPUT SEQUENCE X            
C     BY 4*N.                                                                   
C                                                                               
C     THE ARRAY WSAVE WHICH IS USED BY SUBROUTINE SINQB MUST BE                 
C     INITIALIZED BY CALLING SUBROUTINE SINQI(N,WSAVE).                         
C                                                                               
C                                                                               
C     INPUT PARAMETERS                                                          
C                                                                               
C     N       THE LENGTH OF THE ARRAY X TO BE TRANSFORMED.  THE METHOD          
C             IS MOST EFFICIENT WHEN N IS A PRODUCT OF SMALL PRIMES.            
C                                                                               
C     X       AN ARRAY WHICH CONTAINS THE SEQUENCE TO BE TRANSFORMED            
C                                                                               
C     WSAVE   A WORK ARRAY WHICH MUST BE DIMENSIONED AT LEAST 3*N+15.           
C             IN THE PROGRAM THAT CALLS SINQB. THE WSAVE ARRAY MUST BE          
C             INITIALIZED BY CALLING SUBROUTINE SINQI(N,WSAVE) AND A            
C             DIFFERENT WSAVE ARRAY MUST BE USED FOR EACH DIFFERENT             
C             VALUE OF N. THIS INITIALIZATION DOES NOT HAVE TO BE               
C             REPEATED SO LONG AS N REMAINS UNCHANGED THUS SUBSEQUENT           
C             TRANSFORMS CAN BE OBTAINED FASTER THAN THE FIRST.                 
C                                                                               
C     OUTPUT PARAMETERS                                                         
C                                                                               
C     X       FOR I=1,...,N                                                     
C                                                                               
C                  X(I)= THE SUM FROM K=1 TO K=N OF                             
C                                                                               
C                    4*X(K)*SIN((2K-1)*I*PI/(2*N))                              
C                                                                               
C                  A CALL OF SINQB FOLLOWED BY A CALL OF                        
C                  SINQF WILL MULTIPLY THE SEQUENCE X BY 4*N.                   
C                  THEREFORE SINQF IS THE UNNORMALIZED INVERSE                  
C                  OF SINQB.                                                    
C                                                                               
C     WSAVE   CONTAINS INITIALIZATION CALCULATIONS WHICH MUST NOT               
C             BE DESTROYED BETWEEN CALLS OF SINQB OR SINQF.                     
C                                                                               
      SUBROUTINE SINQB (N,X,WSAVE)                                              
      DIMENSION       X(*)       ,WSAVE(*)                                      
C                                                                               
      IF (N .GT. 1) GO TO 101                                                   
      X(1) = 4.*X(1)                                                            
      RETURN                                                                    
  101 NS2 = N/2                                                                 
      DO 102 K=2,N,2                                                            
         X(K) = -X(K)                                                           
  102 CONTINUE                                                                  
      CALL COSQB (N,X,WSAVE)                                                    
      DO 103 K=1,NS2                                                            
         KC = N-K                                                               
         XHOLD = X(K)                                                           
         X(K) = X(KC+1)                                                         
         X(KC+1) = XHOLD                                                        
  103 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE SINQF(N,X,WSAVE)                                               
C                                                                               
C     SUBROUTINE SINQF COMPUTES THE FAST FOURIER TRANSFORM OF QUARTER           
C     WAVE DATA. THAT IS , SINQF COMPUTES THE COEFFICIENTS IN A SINE            
C     SERIES REPRESENTATION WITH ONLY ODD WAVE NUMBERS. THE TRANSFORM           
C     IS DEFINED BELOW AT OUTPUT PARAMETER X.                                   
C                                                                               
C     SINQB IS THE UNNORMALIZED INVERSE OF SINQF SINCE A CALL OF SINQF          
C     FOLLOWED BY A CALL OF SINQB WILL MULTIPLY THE INPUT SEQUENCE X            
C     BY 4*N.                                                                   
C                                                                               
C     THE ARRAY WSAVE WHICH IS USED BY SUBROUTINE SINQF MUST BE                 
C     INITIALIZED BY CALLING SUBROUTINE SINQI(N,WSAVE).                         
C                                                                               
C                                                                               
C     INPUT PARAMETERS                                                          
C                                                                               
C     N       THE LENGTH OF THE ARRAY X TO BE TRANSFORMED.  THE METHOD          
C             IS MOST EFFICIENT WHEN N IS A PRODUCT OF SMALL PRIMES.            
C                                                                               
C     X       AN ARRAY WHICH CONTAINS THE SEQUENCE TO BE TRANSFORMED            
C                                                                               
C     WSAVE   A WORK ARRAY WHICH MUST BE DIMENSIONED AT LEAST 3*N+15.           
C             IN THE PROGRAM THAT CALLS SINQF. THE WSAVE ARRAY MUST BE          
C             INITIALIZED BY CALLING SUBROUTINE SINQI(N,WSAVE) AND A            
C             DIFFERENT WSAVE ARRAY MUST BE USED FOR EACH DIFFERENT             
C             VALUE OF N. THIS INITIALIZATION DOES NOT HAVE TO BE               
C             REPEATED SO LONG AS N REMAINS UNCHANGED THUS SUBSEQUENT           
C             TRANSFORMS CAN BE OBTAINED FASTER THAN THE FIRST.                 
C                                                                               
C     OUTPUT PARAMETERS                                                         
C                                                                               
C     X       FOR I=1,...,N                                                     
C                                                                               
C                  X(I) = (-1)**(I-1)*X(N)                                      
C                                                                               
C                     + THE SUM FROM K=1 TO K=N-1 OF                            
C                                                                               
C                     2*X(K)*SIN((2*I-1)*K*PI/(2*N))                            
C                                                                               
C                  A CALL OF SINQF FOLLOWED BY A CALL OF                        
C                  SINQB WILL MULTIPLY THE SEQUENCE X BY 4*N.                   
C                  THEREFORE SINQB IS THE UNNORMALIZED INVERSE                  
C                  OF SINQF.                                                    
C                                                                               
C     WSAVE   CONTAINS INITIALIZATION CALCULATIONS WHICH MUST NOT               
C             BE DESTROYED BETWEEN CALLS OF SINQF OR SINQB.                     
C                                                                               
      SUBROUTINE SINQF (N,X,WSAVE)                                              
      DIMENSION       X(*)       ,WSAVE(*)                                      
C                                                                               
      IF (N .EQ. 1) RETURN                                                      
      NS2 = N/2                                                                 
      DO 101 K=1,NS2                                                            
         KC = N-K                                                               
         XHOLD = X(K)                                                           
         X(K) = X(KC+1)                                                         
         X(KC+1) = XHOLD                                                        
  101 CONTINUE                                                                  
      CALL COSQF (N,X,WSAVE)                                                    
      DO 102 K=2,N,2                                                            
         X(K) = -X(K)                                                           
  102 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE SINQI(N,WSAVE)                                                 
C                                                                               
C     SUBROUTINE SINQI INITIALIZES THE ARRAY WSAVE WHICH IS USED IN             
C     BOTH SINQF AND SINQB. THE PRIME FACTORIZATION OF N TOGETHER WITH          
C     A TABULATION OF THE TRIGONOMETRIC FUNCTIONS ARE COMPUTED AND              
C     STORED IN WSAVE.                                                          
C                                                                               
C     INPUT PARAMETER                                                           
C                                                                               
C     N       THE LENGTH OF THE SEQUENCE TO BE TRANSFORMED. THE METHOD          
C             IS MOST EFFICIENT WHEN N IS A PRODUCT OF SMALL PRIMES.            
C                                                                               
C     OUTPUT PARAMETER                                                          
C                                                                               
C     WSAVE   A WORK ARRAY WHICH MUST BE DIMENSIONED AT LEAST 3*N+15.           
C             THE SAME WORK ARRAY CAN BE USED FOR BOTH SINQF AND SINQB          
C             AS LONG AS N REMAINS UNCHANGED. DIFFERENT WSAVE ARRAYS            
C             ARE REQUIRED FOR DIFFERENT VALUES OF N. THE CONTENTS OF           
C             WSAVE MUST NOT BE CHANGED BETWEEN CALLS OF SINQF OR SINQB.        
C                                                                               
      SUBROUTINE SINQI (N,WSAVE)                                                
      DIMENSION       WSAVE(*)                                                  
C                                                                               
      CALL COSQI (N,WSAVE)                                                      
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE SINT(N,X,WSAVE)                                                
C                                                                               
C     SUBROUTINE SINT COMPUTES THE DISCRETE FOURIER SINE TRANSFORM              
C     OF AN ODD SEQUENCE X(I). THE TRANSFORM IS DEFINED BELOW AT                
C     OUTPUT PARAMETER X.                                                       
C                                                                               
C     SINT IS THE UNNORMALIZED INVERSE OF ITSELF SINCE A CALL OF SINT           
C     FOLLOWED BY ANOTHER CALL OF SINT WILL MULTIPLY THE INPUT SEQUENCE         
C     X BY 2*(N+1).                                                             
C                                                                               
C     THE ARRAY WSAVE WHICH IS USED BY SUBROUTINE SINT MUST BE                  
C     INITIALIZED BY CALLING SUBROUTINE SINTI(N,WSAVE).                         
C                                                                               
C     INPUT PARAMETERS                                                          
C                                                                               
C     N       THE LENGTH OF THE SEQUENCE TO BE TRANSFORMED.  THE METHOD         
C             IS MOST EFFICIENT WHEN N+1 IS THE PRODUCT OF SMALL PRIMES.        
C                                                                               
C     X       AN ARRAY WHICH CONTAINS THE SEQUENCE TO BE TRANSFORMED            
C                                                                               
C                                                                               
C     WSAVE   A WORK ARRAY WITH DIMENSION AT LEAST INT(2.5*N+15)                
C             IN THE PROGRAM THAT CALLS SINT. THE WSAVE ARRAY MUST BE           
C             INITIALIZED BY CALLING SUBROUTINE SINTI(N,WSAVE) AND A            
C             DIFFERENT WSAVE ARRAY MUST BE USED FOR EACH DIFFERENT             
C             VALUE OF N. THIS INITIALIZATION DOES NOT HAVE TO BE               
C             REPEATED SO LONG AS N REMAINS UNCHANGED THUS SUBSEQUENT           
C             TRANSFORMS CAN BE OBTAINED FASTER THAN THE FIRST.                 
C                                                                               
C     OUTPUT PARAMETERS                                                         
C                                                                               
C     X       FOR I=1,...,N                                                     
C                                                                               
C                  X(I)= THE SUM FROM K=1 TO K=N                                
C                                                                               
C                       2*X(K)*SIN(K*I*PI/(N+1))                                
C                                                                               
C                  A CALL OF SINT FOLLOWED BY ANOTHER CALL OF                   
C                  SINT WILL MULTIPLY THE SEQUENCE X BY 2*(N+1).                
C                  HENCE SINT IS THE UNNORMALIZED INVERSE                       
C                  OF ITSELF.                                                   
C                                                                               
C     WSAVE   CONTAINS INITIALIZATION CALCULATIONS WHICH MUST NOT BE            
C             DESTROYED BETWEEN CALLS OF SINT.                                  
C                                                                               
      SUBROUTINE SINT (N,X,WSAVE)                                               
      DIMENSION       X(*)       ,WSAVE(*)                                      
C                                                                               
      NP1 = N+1                                                                 
      IW1 = N/2+1                                                               
      IW2 = IW1+NP1                                                             
      IW3 = IW2+NP1                                                             
      CALL SINT1(N,X,WSAVE,WSAVE(IW1),WSAVE(IW2),WSAVE(IW3))                    
      RETURN                                                                    
      END                                                                       
      SUBROUTINE SINT1(N,WAR,WAS,XH,X,IFAC)                                     
      DIMENSION WAR(*),WAS(*),X(*),XH(*),IFAC(*)                                
      DATA SQRT3 /1.73205080756888/                                             
      DO 100 I=1,N                                                              
      XH(I) = WAR(I)                                                            
      WAR(I) = X(I)                                                             
  100 CONTINUE                                                                  
      IF (N-2) 101,102,103                                                      
  101 XH(1) = XH(1)+XH(1)                                                       
      GO TO 106                                                                 
  102 XHOLD = SQRT3*(XH(1)+XH(2))                                               
      XH(2) = SQRT3*(XH(1)-XH(2))                                               
      XH(1) = XHOLD                                                             
      GO TO 106                                                                 
  103 NP1 = N+1                                                                 
      NS2 = N/2                                                                 
      X(1) = 0.                                                                 
      DO 104 K=1,NS2                                                            
         KC = NP1-K                                                             
         T1 = XH(K)-XH(KC)                                                      
         T2 = WAS(K)*(XH(K)+XH(KC))                                             
         X(K+1) = T1+T2                                                         
         X(KC+1) = T2-T1                                                        
  104 CONTINUE                                                                  
      MODN = MOD(N,2)                                                           
      IF (MODN .NE. 0) X(NS2+2) = 4.*XH(NS2+1)                                  
      CALL RFFTF1 (NP1,X,XH,WAR,IFAC)                                           
      XH(1) = .5*X(1)                                                           
      DO 105 I=3,N,2                                                            
         XH(I-1) = -X(I)                                                        
         XH(I) = XH(I-2)+X(I-1)                                                 
  105 CONTINUE                                                                  
      IF (MODN .NE. 0) GO TO 106                                                
      XH(N) = -X(N+1)                                                           
  106 DO 107 I=1,N                                                              
      X(I) = WAR(I)                                                             
      WAR(I) = XH(I)                                                            
  107 CONTINUE                                                                  
      RETURN                                                                    
      END                                                                       
C     SUBROUTINE SINTI(N,WSAVE)                                                 
C                                                                               
C     SUBROUTINE SINTI INITIALIZES THE ARRAY WSAVE WHICH IS USED IN             
C     SUBROUTINE SINT. THE PRIME FACTORIZATION OF N TOGETHER WITH               
C     A TABULATION OF THE TRIGONOMETRIC FUNCTIONS ARE COMPUTED AND              
C     STORED IN WSAVE.                                                          
C                                                                               
C     INPUT PARAMETER                                                           
C                                                                               
C     N       THE LENGTH OF THE SEQUENCE TO BE TRANSFORMED.  THE METHOD         
C             IS MOST EFFICIENT WHEN N+1 IS A PRODUCT OF SMALL PRIMES.          
C                                                                               
C     OUTPUT PARAMETER                                                          
C                                                                               
C     WSAVE   A WORK ARRAY WITH AT LEAST INT(2.5*N+15) LOCATIONS.               
C             DIFFERENT WSAVE ARRAYS ARE REQUIRED FOR DIFFERENT VALUES          
C             OF N. THE CONTENTS OF WSAVE MUST NOT BE CHANGED BETWEEN           
C             CALLS OF SINT.                                                    
C                                                                               
      SUBROUTINE SINTI (N,WSAVE)                                                
      DIMENSION       WSAVE(*)                                                  
C                                                                               
      PI = PIMACH(DUM)                                                          
      IF (N .LE. 1) RETURN                                                      
      NS2 = N/2                                                                 
      NP1 = N+1                                                                 
      DT = PI/FLOAT(NP1)                                                        
      DO 101 K=1,NS2                                                            
         WSAVE(K) = 2.*SIN(K*DT)                                                
  101 CONTINUE                                                                  
      CALL RFFTI (NP1,WSAVE(NS2+1))                                             
      RETURN                                                                    
      END                                                                       
      SUBROUTINE TFFTPK (IERROR)
C
C
C PURPOSE                TO DEMONSTRATE THE USE OF FFTPACK, AND TO
C                        TEST THE PERFORMANCE OF FFTPACK ON ONE
C                        WELL-CONDITIONED PROBLEM. 
C
C USAGE                  CALL TFFTPK (IERROR)
C
C ARGUMENTS              
C 
C ON OUTPUT              IERROR
C                          INTEGER VARIABLE SET TO ZERO IF FFTPACK
C                          CORRECTLY SOLVED THE TEST PROBLEM, AND 
C                          ONE IF FFTPACK FAILED.
C
C I/O                    IF THE TEST SUCCEEDS(FAILS), THE MESSAGE
C
C                           FFTPACK TEST SUCCESSFUL (UNSUCCESSFUL)
C
C                        IS WRITTEN ON UNIT 6. IN THE CASE OF FAILURE,
C                        ADDITIONAL MESSAGES ARE WRITTEN IDENTIFYING THE
C                        FAILURE MORE EXPLICITLY.
C 
C PRECISION              SINGLE
C
C REQUIRED LIBRARY       NONE
C FILES
C
C LANGUAGE               FORTRAN
C
C HISTORY                WRITTEN BY MEMBERS OF THE SCIENTIFIC 
C                        COMPUTING DIVISION OF NCAR,
C                        BOULDER COLORADO.
C
C ALGORITHM              FOR EACH OF THE ROUTINES, RFFTF, RFFTB, EZFFTF,
C                        AND EZFFTB IN THE FFTPACK PACKAGE A SIMILAR 
C                        TEST IS RUN. AN APPROPIATE VECTOR, FOR WHICH
C                        THE EXACT TRANSFORM IS KNOWN IS USED AS THE
C                        INPUT VECTOR. THE ROUTINE IS CALLED TO PERFORM
C                        THE TRANSFORM. THE CALCULATED TRANSFORM VECTOR
C                        IS COMPARED WITH THE EXACT TRANSFORM TO SEE
C                        WHETHER THE PERFORMANCE CRITERION IS MET WITHIN
C                        THE SPECIFED TOLERANCE.
C
C                        FOR RFFTF AND EZFFTF, A REAL VECTOR, THE ELEMENTS
C                        WHICH ARE EQUAL TO ONE, IS USED AS INPUT. THE
C                        TRANSFORMED VECTOR HAS THE FIRST ELEMENT EQUAL
C                        TO THE LENGTH OF THE INPUT VECTOR. ALL OTHER
C                        ELEMENTS ARE EQUAL TO ZERO.
C
C                        FOR RFFTB AND EZFFTB, THE INPUT VECTOR HAS FIRST
C                        ELEMENT EQUAL TO ONE AND ALL THE OTHER ELEMENTS
C                        EQUAL TO ZERO. THE TRANSFORMED VECTOR HAS ALL
C                        COMPONENTS EQUAL TO ONE.
C
C PORTABILITY            ANSI STANDARD
C
C
      PARAMETER( N=36 )
      INTEGER DIM1, DIM2
      PARAMETER( DIM1=2*N+15, DIM2=3*N+15, ND2=N/2 )
      REAL RLDAT(N), WRFFT(DIM1), WEZFFT(DIM2), A(ND2), B(ND2)
      DATA TOL/0.01/
C
C STATEMENT FUNCTION SMALL(EX) IS FOR TESTING WHETHER X IS CLOSE TO ZERO,
C INDEPENDENTLY OF MACHINE WORD SIZE. SMALL(EX) IS EXACTLY ZERO ONLY IF
C ABS(X) .LT. EPS/TOL, WHERE EPS IS THE MACHINE PRECESION AND TOL IS A
C TOLERANCE FACTOR USED TO CONTROL THE STRICTNESS OF THE TEST.
C
      SMALL(EX) = TRUNC(1.0+TOL*ABS(EX))-1.0
C
C CALL INITIALIZATION ROUTINE FOR RFFTF AND RFFTB.
C
      CALL RFFTI( N, WRFFT )
C  
C TEST OF RFFTF.
C
      DO 10 I = 1,N
 10   RLDAT(I) = 1.0
C
      CALL RFFTF( N, RLDAT, WRFFT )
C
C TEST RESULTS OF RFFTF
C
      ERROR = ABS( FLOAT(N) - RLDAT(1) )
      DO 15 I = 2,N
 15   ERROR = AMAX1( ERROR, ABS(RLDAT(I)) )
      IF( SMALL(ERROR) .EQ. 0 ) THEN
          IER1 = 0
      ELSE
          IER1 = 1
          WRITE(6,1001)
      END IF
C
C TEST OF RFFTB.
C
      RLDAT(1) = 1.0
      DO 20 I = 2,N
 20   RLDAT(I) = 0.0
C
      CALL RFFTB( N, RLDAT, WRFFT )
C
C TEST RESULTS OF RFFTB
C
      ERROR = 0.0
      DO 25 I = 1,N
 25   ERROR = AMAX1( ERROR, ABS(1.0 - RLDAT(I)) )
      IF( SMALL(ERROR) .EQ. 0 ) THEN
          IER2 = 0
      ELSE
          IER2 = 1
          WRITE(6,1002)
      END IF
C
C CALL INITIALIZATION ROUTINE EZFFTI FOR EZFFTF AND EZFFTB
C
      CALL EZFFTI( N, WEZFFT )
C
C TEST OF EZFFTF.
C
      DO 30 I = 1,N
 30   RLDAT(I) = 1.0
C
      CALL EZFFTF( N, RLDAT, AZERO, A, B, WEZFFT )
C
C TEST RESULTS OF EZFFTF
C
      ERROR = ABS( AZERO - 1.0 )
      DO 35 I = 1,ND2
 35   ERROR = AMAX1( ABS(A(I))+ABS(B(I)), ERROR )
      IF( SMALL(ERROR) .EQ. 0 ) THEN
          IER3 = 0
      ELSE
          IER3 = 1
          WRITE(6,1003)
      END IF 
C     
C TEST OF EZFFTB.
C
      AZERO = 1.0
      DO 40 I = 1,ND2
      A(I) = 0.0
 40   B(I) = 0.0
C
      CALL EZFFTB( N, RLDAT, AZERO, A, B, WEZFFT )
C
C TEST RESULTS OF EZFFTB
C
      ERROR = 0.0
      DO 45 I = 1,N
 45   ERROR = AMAX1( ABS(1.0 - RLDAT(I)), ERROR)
      IF( SMALL(ERROR) .EQ. 0 ) THEN
          IER4 = 0
      ELSE
          IER4 = 1
          WRITE(6,1004)
      END IF
C
C
      IERROR = IER1 + IER2 + IER3 + IER4
      IF(IERROR .EQ. 0 ) THEN
         WRITE(6,998)
      ELSE
         IERROR = 1
         WRITE(6,999)
      END IF
  998 FORMAT(' FFTPACK TEST SUCCESSFUL')
  999 FORMAT(' FFTPACK TEST UNSUCCESSFUL')
 1001 FORMAT(' IN FFTPACK, ENTRY RFFTF RESULTS IN ERROR')
 1002 FORMAT(' IN FFTPACK, ENTRY RFFTB RESULTS IN ERROR')
 1003 FORMAT(' IN FFTPACK, ENTRY EZFFTF RESULTS IN ERROR')
 1004 FORMAT(' IN FFTPACK, ENTRY EZFFTB RESULTS IN ERROR')
      RETURN
      END
      FUNCTION TRUNC(X)
C
C TRUNC IS A PORTABLE FORTRAN FUNCTION WHICH TRUNCATES A VALUE TO THE
C MACHINE SINGLE PRECISION WORD SIZE, REGARDLESS OF WHETHER LONGER
C PRECISION INTERNAL REGISTERS ARE USED FOR FLOATING POINT ARITHMETIC IN
C COMPUTING THE VALUE INPUT TO TRUNC.  THE METHOD USED IS TO FORCE A
C STORE INTO MEMORY BY USING A COMMON BLOCK IN ANOTHER SUBROUTINE.
C
      COMMON /VALUE/ V
      CALL STORES(X)
      TRUNC=V
      RETURN
      END
      SUBROUTINE STORES(X)
C
C FORCES THE ARGUMENT VALUE X TO BE STORED IN MEMORY LOCATION V.
C
      COMMON /VALUE/ V
      V=X
      RETURN
      END
