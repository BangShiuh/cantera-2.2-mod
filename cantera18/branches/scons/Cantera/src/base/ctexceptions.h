/**
 * @file ctexceptions.h
 *   Definitions for the classes that are
 *   thrown when %Cantera experiences an error condition 
 *   (also contains errorhandling module text - see \ref errorhandling).
 */
// Copyright 2001  California Institute of Technology

#ifndef CT_CTEXCEPTIONS_H
#define CT_CTEXCEPTIONS_H

#include <string>
#include <exception>

// See file misc.cpp for implementations of methods/functions declared
// here.

namespace Cantera {

    /*!
     * @defgroup errorhandling Error Handling
     *
     * \brief These classes and related functions are used to handle errors and 
     *        unknown events within Cantera.
     * 
     *  The general idea is that exceptions are thrown using the common
     *  base class called CanteraError. Derived types of CanteraError
     *  characterize what type of error is thrown. A list of all
     *  of the thrown errors is kept in the Application class. 
     *
     *  Any exceptions which are not caught cause a fatal error exit
     *  from the program.
     *
     *  Below is an example of how to catch errors that throw the CanteraError class.
     *  In general, all Cantera C++ programs will have this basic structure.
     * 
     *  \include edemo.cpp
     *
     *  The function showErrors() will print out the fatal error
     *  condition to standard output.
     *
     *  A group of defines may be used during debugging to assert
     *  conditions which should be true. These are named AssertTrace(),
     *  AssertThrow(), and AssertThrowMsg().  Examples of their usage is
     *  given below.
     *
     * @code
     *       AssertTrace(p == OneAtm);
     *       AssertThrow(p == OneAtm, "Kinetics::update");
     *       AssertThrowMsg(p == OneAtm, "Kinetics::update", 
     *                    "Algorithm limited to atmospheric pressure");
     * @endcode
     *
     *  Their first argument is a boolean. If the boolean is not true, a
     *  CanteraError is thrown, with descriptive information indicating
     *  where the error occured. These functions may be eliminated from
     *  the source code, if the -DNDEBUG option is specified to the
     *  compiler.
     *
     */
  
 
    //! Base class for exceptions thrown by Cantera classes.
    /*!
     * This class is the base class for exceptions thrown by Cantera.
     * It inherits from std::exception so that normal error handling
     * operations from applications may automatically handle the
     * errors in their own way.
     *
     * @ingroup errorhandling
     */
    class CanteraError : public std::exception {
    public:
        //! Normal Constructor for the CanteraError base class
        /*!
         * This class doesn't have any storage associated with it. In its
         * constructor, a call to the Application class is made to store
         * the strings associated with the generated error condition.
         *
         * @param procedure String name for the function within which the error was
         *             generated.
         * @param msg  Descriptive string describing the type of error message.
         */
        CanteraError(std::string procedure, std::string msg);

        //! Destructor for base class does nothing
        virtual ~CanteraError() throw() {}
    protected:
        //! Empty base constructor is made protected so that it may be used only by 
        //! inherited classes.
        /*!
         *  We want to discourage throwing an error containing no information.
         */
        CanteraError() {}
    };

    //! Array size error.
    /*!
     *  This error is thrown if a supplied length to a vector supplied
     *  to Cantera is too small.
     *
     * @ingroup errorhandling
     */
    class ArraySizeError : public CanteraError {
    public:
        //! Constructor
        /*!
         * The length needed is supplied by the argument, reqd, and the
         * length supplied is given by the argument sz.
         *
         * @param procedure String name for the function within which the error was
         *             generated.
         * @param sz   This is the length supplied to Cantera.
         * @param reqd This is the required length needed by Cantera
         */
        ArraySizeError(std::string procedure, size_t sz, size_t reqd);
    };

    //! An element index is out of range.
    /*!
     *
     * @ingroup errorhandling
     */
    class ElementRangeError : public CanteraError {
    public:
        //! Constructor
        /*!
         * This class indicates an out-of-bounds index.
         *
         * @param func String name for the function within which the error was
         *             generated.
         * @param m   This is the value of the out-of-bounds index.
         * @param mmax This is the maximum allowed value of the index. The
         *             minimum allowed value is assumed to be 0.
         */
        ElementRangeError(std::string func, size_t m, size_t mmax);
    };

    //! Print a warning when a deprecated method is called.
    /*!
     * These methods are slated to go away in future releases of Cantera.
     * The developer should work towards eliminating the use of these
     * methods in the near future.
     *
     * @param classnm Class the method belongs to
     * @param oldnm Name of the deprecated method
     * @param newnm Name of the method users should use instead
     *
     * @ingroup errorhandling
     */
    void deprecatedMethod(std::string classnm, std::string oldnm, std::string newnm);

    //! Throw an error condition for a procedure that has been removed.
    /*!
     *  
     * @param func String name for the function within which the error was
     *             generated.
     * @param version Version of Cantera that first removed this function.
     *
     * @ingroup errorhandling 
     */
    void removeAtVersion(std::string func, std::string version);

    //! Provides a line number
#define XSTR_TRACE_LINE(s) STR_TRACE_LINE(s)

    //! Provides a line number
#define STR_TRACE_LINE(s) #s  

    //! Provides a std::string variable containing the file and line number
    /*!
     *   This is a std:string containing the file name and the line number
     */
#define STR_TRACE   (std::string(__FILE__) +  ":" + XSTR_TRACE_LINE(__LINE__))

#ifdef NDEBUG
#ifndef AssertTrace
#  define AssertTrace(expr)                        ((void) (0))
#endif
#ifndef AssertThrow
#  define AssertThrow(expr, procedure)             ((void) (0))
#endif
#ifndef AssertThrowMsg
#  define AssertThrowMsg(expr,procedure, message)  ((void) (0))
#endif
#else

    //! Assertion must be true or an error is thrown
    /*!
     * Assertion must be true or else a CanteraError is thrown. A diagnostic string containing the
     * file and line number,  indicating where the error 
     * occured is added to the thrown object.
     *
     * @param expr  Boolean expression that must be true
     *
     * @ingroup errorhandling
     */
#ifndef AssertTrace
#  define AssertTrace(expr)  ((expr) ? (void) 0 : throw Cantera::CanteraError(STR_TRACE, std::string("failed assert: ") + #expr))
#endif

    //!  Assertion must be true or an error is thrown
    /*!
     * Assertion must be true or else a CanteraError is thrown. A diagnostic string indicating where the error 
     * occured is added to the thrown object.
     *
     * @param expr  Boolean expression that must be true
     * @param procedure  Character string or std:string expression indicating the procedure where the assertion failed
     * @ingroup errorhandling
     */
#ifndef AssertThrow
#  define AssertThrow(expr, procedure)   ((expr) ? (void) 0 : throw Cantera::CanteraError(procedure, std::string("failed assert: ") + #expr))
#endif

    //!  Assertion must be true or an error is thrown
    /*!
     * Assertion must be true or else a CanteraError is thrown. A
     * diagnostic string indicating where the error occured is added
     * to the thrown object.
     *
     * @param expr  Boolean expression that must be true
     * @param procedure  Character string or std:string expression indicating 
     *                   the procedure where the assertion failed
     * @param message  Character string or std:string expression contaiing 
     *    a descriptive message is added to the thrown error condition.
     *
     * @ingroup errorhandling
     */
#ifndef AssertThrowMsg
#  define AssertThrowMsg(expr, procedure, message)  ((expr) ? (void) 0 : throw Cantera::CanteraError(procedure + std::string(": at failed assert: \"") + std::string(#expr) + std::string("\""), message))
#endif



#endif
 
}

#endif
