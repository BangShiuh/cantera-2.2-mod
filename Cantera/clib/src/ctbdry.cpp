
// Build as a DLL under Windows
#ifdef WIN32
#define DLL_EXPORT __declspec(dllexport)
#pragma warning(disable:4786)
#pragma warning(disable:4503)
#else
#define DLL_EXPORT
#endif


// Cantera includes
#include "oneD/OneDim.h"
#include "oneD/Inlet1D.h"

#include "Cabinet.h"
#include "Storage.h"

// Values returned for error conditions
#define ERR -999
#define DERR -999.999

Cabinet<Bdry1D>*  Cabinet<Bdry1D>::__storage = 0;

inline Bdry1D* _bndry(int i) {
    return Cabinet<Bdry1D>::cabinet()->item(i);
}

//inline Phase* _phase(int n) {
//    return Storage::__storage->__phasetable[n];
//}

inline ThermoPhase* _thermo(int n) {
    return Storage::__storage->__thtable[n];
}

extern "C" {  

    int DLL_EXPORT bndry_new(int itype) {
        Bdry1D* s;
        switch (itype) {
        case 1:
            s = new Inlet1D(); break;
        case 2: 
            s = new Symm1D(); break;
        case 3:
            s = new Surf1D(); break;
        default:
            return -2;
        }
        int i = Cabinet<Bdry1D>::cabinet()->add(s);
        return i;
    }

    int DLL_EXPORT bndry_del(int i) {
        Cabinet<Bdry1D>::cabinet()->del(i);
        return 0;
    }

    double DLL_EXPORT bndry_temperature(int i) {
        return _bndry(i)->temperature();
    }

    int DLL_EXPORT bndry_settemperature(int i, double t) {
        _bndry(i)->setTemperature(t);
        return 0;
    }

    int DLL_EXPORT bndry_setmdot(int i, double mdot) {
        try {
            _bndry(i)->setMdot(mdot);
        }
        catch (CanteraError) {return -1;}
        return 0;
    }


    double DLL_EXPORT bndry_mdot(int i) {
        return _bndry(i)->mdot();
        return 0;
    }

    int DLL_EXPORT bndry_setxin(int i, double* xin) {
        _bndry(i)->setMoleFractions(xin);
        return 0;
    }

    int DLL_EXPORT bndry_setxinbyname(int i, char* xin) {
        _bndry(i)->setMoleFractions(string(xin));
        return 0;
    }
}
