/**
 * @file ctexceptions.h
 */

// $Author$
// $Revision$
// $Date$

// Copyright 2001  California Institute of Technology

#ifndef CT_CTEXCEPTIONS_H
#define CT_CTEXCEPTIONS_H

#include "global.h"
#include "stringUtils.h"

namespace Cantera {

    /**
     * Base class for exceptions thrown by Cantera classes
     */
    class CanteraError {
    public:
        CanteraError() {}
        CanteraError(string proc, string msg) {
            setError(proc, msg);
            m_msg = msg;
        }
        virtual ~CanteraError(){}
        string errorMessage() { return m_msg; }
        void append(string msg) { m_msg += msg; }
        void saveError(string procedure) {
            setError(procedure, m_msg);
            m_msg = "";
        }
    protected:
        string m_msg;
    };

    class ArraySizeError : public CanteraError {
    public:
        ArraySizeError(string proc, int sz, int reqd) :
            CanteraError(proc, "Array size ("+int2str(sz)+") too small. Must be at least "+int2str(reqd)) {}
    };
}

#endif
