/* ../config.h.  Generated automatically by configure.  */
//
//  Run the 'configure' script to generate 'config.h' from this input file.
//
#ifndef CT_CONFIG_H
#define CT_CONFIG_H


//------------------------ Fortran settings -------------------//


// define types doublereal, integer, and ftnlen to match the 
// corresponding Fortran data types on your system. The defaults
// are OK for most systems

typedef  double       doublereal;       // Fortran double precision 
typedef  int          integer;          // Fortran integer
typedef  int          ftnlen;           // Fortran hidden string length type


// Fortran compilers pass character strings in argument lists by
// adding a hidden argement with the length of the string. Some
// compilers add the hidden length argument immediately after the
// CHARACTER variable being passed, while others put all of the hidden
// length arguments at the end of the argument list. Define this if 
// the lengths are at the end of the argument list. This is usually the
// case for most unix Fortran compilers, but is (by default) false for
// Visual Fortran under Windows.
#define STRING_LEN_AT_END


// Define this if Fortran adds a trailing underscore to names in object files.
// For linux and most unix systems, this is the case.
#define FTN_TRAILING_UNDERSCORE


//-------- LAPACK / BLAS ---------

// Define if you are using LAPACK and BLAS from the Intel Math Kernel
// Library 
/* #undef HAVE_INTEL_MKL */

#define LAPACK_FTN_STRING_LEN_AT_END 1
#define LAPACK_NAMES_LOWERCASE 1
#define LAPACK_FTN_TRAILING_UNDERSCORE 1


//--------- Cantera -------------- 


//--------- CKReader -------------



//--------- CtLib ----------------



#endif
