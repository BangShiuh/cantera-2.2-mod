/**
 *  @file CVode.h
 */

/* $Author$
 * $Date$
 * $Revision$
 */

// Copyright 2001  California Institute of Technology


#ifndef CT_CVODE_H
#define CT_CVODE_H

#ifdef WIN32
#pragma warning(disable:4786)
#pragma warning(disable:4503)
#endif

#include "Integrator.h"
#include "FuncEval.h"
#include "ctexceptions.h"
#include "ct_defs.h"

// cvode includes
//#include "cvode/nvector.h"
//#include "cvode/cvode.h"


namespace Cantera {

    /**
     * Exception thrown when a CVODE error is encountered.
     */
    class CVodeErr : public CanteraError {
    public:
        CVodeErr(string msg) : CanteraError("CVodeInt", msg){}
    };


    /**
     *  Wrapper class for 'cvode' integrator from LLNL.
     *  The unmodified cvode code is in directory ext/cvode.
     *
     * @see FuncEval.h. Classes that use CVodeInt:
     * ImplicitChem, ImplicitSurfChem, Reactor
     *
     */
    class CVodeInt : public Integrator {

    public:

        CVodeInt();
        virtual ~CVodeInt();
        virtual void setTolerances(double reltol, int n, double* abstol);
        virtual void setTolerances(double reltol, double abstol);
        virtual void setProblemType(int probtype);
        virtual void initialize(double t0, FuncEval& func);
        virtual void reinitialize(double t0, FuncEval& func);
        virtual void integrate(double tout);
        virtual doublereal step(double tout);
	virtual double& solution(int k);
	virtual double* solution();
	virtual int nEquations() const { return m_neq;}
        virtual int nEvals() const;
        virtual void setMaxOrder(int n) { m_maxord = n; }
        virtual void setMethod(MethodType t);
        virtual void setIterator(IterType t);
        virtual void setMaxStep(double hmax);

    private:

	int m_neq;
        void* m_cvode_mem;
        double m_t0;
        void *m_y, *m_abstol; //N_Vector m_y;
        //N_Vector m_abstol;
        int m_type;
        int m_itol;
        int m_method;
        int m_iter;
        int m_maxord;
        double m_reltol;
        double m_abstols;
        int m_nabs;
        double m_hmax;

        vector_fp m_ropt;
        //vector_int m_iopt;
        long int* m_iopt; //[OPT_SIZE];
        //N_Vector m_yprime;
        void* m_data;
    };

}    // namespace

#endif // CT_CVODE
