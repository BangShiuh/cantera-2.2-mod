/**
 * @file ShomateThermo.h
 * 
 * This parameterization requires 7 coefficients A - G:
 *
 *  Cp� = A + B*t + C*t2 + D*t3 + E/t^2
 *
 *  H� - H�298.15= A*t + B*t^2/2 + C*t^3/3 + D*t^4/4 - E/t + F 
 *                 - \Delta_f H�f,298
 *
 *  S� = A*ln(t) + B*t + C*t^2/2 + D*t^3/3 - E/(2*t^2) + G
 *
 *     Cp = heat capacity (J/mol*K)
 *     H� = standard enthalpy (kJ/mol)
 *     \Delta_f H�298.15 = enthalpy of formation at 298.15 K (kJ/mol)
 *     S� = standard entropy (J/mol*K)
 *     t = temperature (K) / 1000.
 *
 */

#ifndef CT_SHOMATETHERMO_H
#define CT_SHOMATETHERMO_H

#include "SpeciesThermoMgr.h"
#include "ShomatePoly.h"
#include "speciesThermoTypes.h"

namespace Cantera {

    /**
     * A species thermodynamic property manager for the Shomate
     * polynomial parameterization. This is the parameterization used
     * in the NIST Chemistry WebBook (http://webbook.nist.gov/chemistry)
     */
    class ShomateThermo : public SpeciesThermo {
    
    public:

        const int ID;
 
        ShomateThermo() :
            ID(SHOMATE),
            m_tlow_max(0.0), 
            m_thigh_min(1.e30),
            m_ngroups(0) { m_t.resize(7); }

        virtual ~ShomateThermo() {}

        /**
         * Install values for a new species.
         * @param index  Species index
         * @param type   ignored, since only Shomate type is supported
         * @param c      coefficients. These are parameters A through G
         * in the same units as used in the NIST Chemistry WebBook.
         *
         */
        virtual void install(int index, int type, const doublereal* c,
            doublereal minTemp, doublereal maxTemp, doublereal refPressure) {
            int imid = int(c[0]);       // midpoint temp converted to integer
            int igrp = m_index[imid];   // has this value been seen before?
            if (igrp == 0) {            // if not, prepare new group
                vector<ShomatePoly> v;
                m_high.push_back(v);
                m_low.push_back(v);
                m_tmid.push_back(c[0]);
                m_index[imid] = igrp = m_high.size();
                m_ngroups++;
            }
            doublereal tlow  = minTemp;
            doublereal tmid  = c[0];
            doublereal thigh = maxTemp;
            doublereal pref  = refPressure;
            const doublereal* clow = c + 1;
            const doublereal* chigh = c + 8;
            m_high[igrp-1].push_back(ShomatePoly(index, tmid, thigh, 
                                         pref, chigh));
            m_low[igrp-1].push_back(ShomatePoly(index, tlow, tmid, 
                                        pref, clow));
            if (tlow > m_tlow_max)    m_tlow_max = tlow;
            if (thigh < m_thigh_min)  m_thigh_min = thigh;
            m_tlow.push_back(tlow);
            m_thigh.push_back(thigh);
            m_p0 = pref;
        }


        virtual void update(doublereal t, doublereal* cp_R, 
            doublereal* h_RT, doublereal* s_R) const {
            int i;

            doublereal tt = 1.e-3*t;
            m_t[0] = tt;
            m_t[1] = tt*tt;
            m_t[2] = m_t[1]*tt;
            m_t[3] = 1.0/m_t[1];
            m_t[4] = log(tt);
            m_t[5] = 1.0/GasConstant;
            m_t[6] = 1.0/(GasConstant * t);

            vector<ShomatePoly>::const_iterator _begin, _end;
            for (i = 0; i != m_ngroups; i++) {
                if (t > m_tmid[i]) {
                    _begin  = m_high[i].begin();
                    _end    = m_high[i].end();
                }
                else {
                    _begin  = m_low[i].begin();
                    _end    = m_low[i].end();
                }
                for (; _begin != _end; ++_begin) {
                    _begin->updateProperties(m_t.begin(), cp_R, h_RT, s_R);
                }
            }
        }

        virtual doublereal minTemp(int k=-1) const {
            if (k < 0)
                return m_tlow_max;
            else
                return m_tlow[k];
        }

        virtual doublereal maxTemp(int k=-1) const {
            if (k < 0)
                return m_thigh_min;
            else
                return m_thigh[k];
        }

        virtual doublereal refPressure() const {return m_p0;}

 protected:

        vector<vector<ShomatePoly> > m_high;
        vector<vector<ShomatePoly> > m_low;
        map<int, int>              m_index;
        vector_fp                  m_tmid;
        doublereal                 m_tlow_max;
        doublereal                 m_thigh_min;
        vector_fp                  m_tlow;
        vector_fp                  m_thigh;
        doublereal                 m_p0;
        int                        m_ngroups;
        mutable vector_fp          m_t;
    };

}

#endif
