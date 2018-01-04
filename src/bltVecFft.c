/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltVecFft.c --
 *
 * This module implements vector data objects.
 *
 * Copyright 2015 George A. Howlett. All rights reserved.  
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *   1) Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2) Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the
 *      distribution.
 *   3) Neither the name of the authors nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *   4) Products derived from this software may not be called "BLT" nor may
 *      "BLT" appear in their names without specific prior written
 *      permission from the author.
 *
 *   THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED
 *   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* spinellia@acm.org START */

#include "bltVecInt.h"

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef TIME_WITH_SYS_TIME
  #include <sys/time.h>
  #include <time.h>
#else
  #ifdef HAVE_SYS_TIME_H
    #include <sys/time.h>
  #else
    #include <time.h>
  #endif /* HAVE_SYS_TIME_H */
#endif /* TIME_WITH_SYS_TIME */

#include "bltAlloc.h"
#include <bltMath.h>
#include "bltNsUtil.h"
#include "bltSwitch.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr

/* routine by Brenner
 * data is the array of complex data points, perversely
 * starting at 1
 * nn is the number of complex points, i.e. half the length of data
 * isign is 1 for forward, -1 for inverse
 */
static void 
four1(double *data, unsigned long nn, int isign)
{
    unsigned long n, mmax, m, j, i;
    double tempr, tempi;
    
    n=nn << 1;
    j=1;
    for (i = 1; i < n; i += 2) {
        int m;
        
        if (j > i) {
            SWAP(data[j],data[i]);
            SWAP(data[j+1],data[i+1]);
        }
        m = n >> 1;
        while ((m >= 2) && (j > m)) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
    mmax = 2;
    while (n > mmax) {
        double wr, wi, wpr, wpi;
        int istep;
        double theta, wtemp;
        
        istep=mmax << 1;
        theta = isign * (6.28318530717959 / mmax);
        wtemp = sin(0.5*theta);
        wpr = -2.0 * wtemp*wtemp;
        wpi = sin(theta);
        wr = 1.0;
        wi = 0.0;
        for (m = 1; m < mmax; m += 2) {
            for (i=m;i<=n;i+=istep) {
                int j;
                
                j=i+mmax;
                tempr=wr*data[j]-wi*data[j+1];
                tempi=wr*data[j+1]+wi*data[j];
                data[j]=data[i]-tempr;
                data[j+1]=data[i+1]-tempi;
                data[i] += tempr;
                data[i+1] += tempi;
            }
            wr=(wtemp=wr)*wpr-wi*wpi+wr;
            wi=wi*wpr+wtemp*wpi+wi;
        }
        mmax=istep;
    }
}
#undef SWAP

static long 
smallest_power_of_2_not_less_than(long x)
{
    long pow2 = 1;

    while (pow2 < x){
        pow2 <<= 1;
    }
    return pow2;
}


int
Blt_Vec_FFT(
    Tcl_Interp *interp,                 /* Interpreter to report errors
                                         * to */
    Vector *realVecPtr,                 /* If non-NULL, indicates to
                                         * compute and store the real
                                         * values in this vector.  */
    Vector *phasesVecPtr,                  /* If non-NULL, indicates to
                                         * compute and store the imaginary
                                         * values in this vector. */
    Vector *freqVecPtr,                 /* If non-NULL, indicates to
                                         * compute and store the frequency
                                         * values in this vector.  */
    double delta,                       /*  */
    int flags,                          /* Bit mask representing various
                                         * flags: FFT_NO_CONSTANT,
                                         * FFT_SPECTRUM, and
                                         * FFT_BARTLETT. */
    Vector *srcPtr) 
{
    long length;
    long pow2len;
    double *pad;
    long i;
    double Wss = 0.0;
    /* TENTATIVE */
    long middle = 1;
    int noconstant;

    noconstant = (flags & FFT_NO_CONSTANT) ? 1 : 0;

    /* Length of the original vector. */
    length = srcPtr->last - srcPtr->first;

    /* New length */
    pow2len = smallest_power_of_2_not_less_than(length);

    /* We do not do in-place FFTs */
    if (realVecPtr == srcPtr) {
        Tcl_AppendResult(interp, "real vector \"", realVecPtr->name, 
                 "\" can't be the same as the source", (char *)NULL);
        return TCL_ERROR;
    }
    if (phasesVecPtr != NULL) {
        if (phasesVecPtr == srcPtr) {
            Tcl_AppendResult(interp, "imaginary vector \"", phasesVecPtr->name, 
                        "\" can't be the same as the source", (char *)NULL);
            return TCL_ERROR;
        }
        if (Blt_Vec_ChangeLength(interp, phasesVecPtr, 
                pow2len/2-noconstant+middle) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (freqVecPtr != NULL) {
        if (freqVecPtr == srcPtr) {
            Tcl_AppendResult(interp, "frequency vector \"", freqVecPtr->name, 
                     "\" can't be the same as the source", (char *)NULL);
            return TCL_ERROR;
        }
        if (Blt_Vec_ChangeLength(interp, freqVecPtr, 
                           pow2len/2-noconstant+middle) != TCL_OK) {
            return TCL_ERROR;
        }
    }

    /* Allocate memory zero-filled array. */
    pad = Blt_Calloc(pow2len * 2, sizeof(double));
    if (pad == NULL) {
        Tcl_AppendResult(interp, "can't allocate memory for padded data",
                 (char *)NULL);
        return TCL_ERROR;
    }
    
    /*
     * Since we just do real transforms, only even locations will be
     * filled with data.
     */
    if (flags & FFT_BARTLETT) {
        /* Bartlett window 1 - ( (x - N/2) / (N/2) ) */
        double Nhalf = pow2len*0.5;
        double Nhalf_1 = 1.0 / Nhalf;
        double w;

        for (i = 0; i < length; i++) {
            w = 1.0 - fabs( (i-Nhalf) * Nhalf_1 );
            Wss += w;
            pad[2*i] = w * srcPtr->valueArr[i];
        }
        for(/*empty*/; i < pow2len; i++) {
            w = 1.0 - fabs((i-Nhalf) * Nhalf_1);
            Wss += w;
        }
    } else {
        /* Squared window, i.e. no data windowing. */
        for (i = 0; i < length; i++) { 
            pad[2*i] = srcPtr->valueArr[i]; 
        }
        Wss = pow2len;
    }
    
    /* Fourier */
    four1(pad-1, pow2len, 1);
    
    /*
      for(i=0;i<pow2len;i++){
      printf( "(%f %f) ", pad[2*i], pad[2*i+1] );
      }
    */
    
    /* the spectrum is the modulus of the transforms, scaled by 1/N^2 */
    /* or 1/(N * Wss) for windowed data */
    if (flags & FFT_SPECTRUM) {
        double factor = 1.0 / (pow2len*Wss);
        double *v = realVecPtr->valueArr;
        
        for (i = 0 + noconstant; i < pow2len / 2; i++) {
            double re, im, reS, imS;

            re = pad[2*i];
            im = pad[2*i+1];
            reS = pad[2*pow2len-2*i-2];
            imS = pad[2*pow2len-2*i-1];
# if 0
            v[i - noconstant] = factor *
                (hypot(pad[2*i], pad[2*i+1]) +
                 hypot(pad[pow2len*2-2*i-2], pad[pow2len*2-2*i-1]));
# else
            v[i - noconstant] = factor * (sqrt(re*re + im*im) + sqrt(reS*reS + imS*imS));
# endif
        }
    } else {
        for(i = 0 + noconstant; i < pow2len / 2 + middle; i++) {
            realVecPtr->valueArr[i - noconstant] = pad[2*i];
        }
    }
    if (phasesVecPtr != NULL) {
        for (i = 0 + noconstant; i < pow2len / 2 + middle; i++) {
            phasesVecPtr->valueArr[i-noconstant] = pad[2*i+1];
        }
    }
    
    /* Compute frequencies */
    if (freqVecPtr != NULL) {
        double N = pow2len;
        double denom = 1.0 / N / delta;
        for( i=0+noconstant; i<pow2len/2+middle; i++ ){
            freqVecPtr->valueArr[i-noconstant] = ((double) i) * denom;
        }
    }
    /* Memory is necessarily dynamic, because nobody touched it ! */
    Blt_Free(pad);
    
    realVecPtr->offset = 0;
    return TCL_OK;
}


int
Blt_Vec_InverseFFT(Tcl_Interp *interp, Vector *srcImagPtr, Vector *destRealPtr, 
                   Vector *destImagPtr, Vector *srcPtr)
{
    double *pad;
    double oneOverN;
    long i, length, pow2len;

    if ((destRealPtr == srcPtr) || (destImagPtr == srcPtr )){
        Tcl_AppendResult(interp,
                "real or imaginary vectors can't be same as source",
                (char *)NULL);
        return TCL_ERROR;               /* We do not do in-place FFTs */
    }
    length = srcPtr->last - srcPtr->first;

    /* Minus one because of the magical middle element! */
    pow2len = smallest_power_of_2_not_less_than( (length-1)*2 );
    oneOverN = 1.0 / pow2len;

    if (Blt_Vec_ChangeLength(interp, destRealPtr, pow2len) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_Vec_ChangeLength(interp, destImagPtr, pow2len) != TCL_OK) {
        return TCL_ERROR;
    }

    if( length != (srcImagPtr->last - srcImagPtr->first) ){
        Tcl_AppendResult(srcPtr->interp,
                "the length of the imagPart vector must ",
                "be the same as the real one", (char *)NULL);
        return TCL_ERROR;
    }

    pad = Blt_AssertMalloc( pow2len*2*sizeof(double) );
    if( pad == NULL ){
        if (interp != NULL) {
            Tcl_AppendResult(interp, "memory allocation failed", (char *)NULL);
        }
        return TCL_ERROR;
    }
    for(i=0;i<pow2len*2;i++) { pad[i] = 0.0; }
    for(i=0;i<length-1;i++){
        pad[2*i] = srcPtr->valueArr[i];
        pad[2*i+1] = srcImagPtr->valueArr[i];
        pad[pow2len*2 - 2*i - 2 ] = srcPtr->valueArr[i+1];
        pad[pow2len*2 - 2*i - 1 ] = - srcImagPtr->valueArr[i+1];
    }

    /* Mythical middle element */
    pad[(length-1)*2] = srcPtr->valueArr[length-1];
    pad[(length-1)*2+1] = srcImagPtr->valueArr[length-1];

    /*
      for(i=0;i<pow2len;i++){
      printf( "(%f %f) ", pad[2*i], pad[2*i+1] );
      }
    */
    
    /* Fourier */
    four1( pad-1, pow2len, -1 );

    /* Put values in their places, normalising by 1/N */
    for(i=0;i<pow2len;i++){
        destRealPtr->valueArr[i] = pad[2*i] * oneOverN;
        destImagPtr->valueArr[i] = pad[2*i+1] * oneOverN;
    }

    /* Memory is necessarily dynamic, because nobody touched it ! */
    Blt_Free(pad);
    return TCL_OK;
}

/* spinellia@acm.org STOP */

