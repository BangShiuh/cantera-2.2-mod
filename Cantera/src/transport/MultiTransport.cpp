/**
 *
 *  @file MultiTransport.cpp
 *  Implementation file for class MultiTransport
 *
 * @ingroup transportProps
 *
 *  $Author$
 *  $Date$
 *  $Revision$
 *
 *  Copyright 2001 California Institute of Technology
 *  See file License.txt for licensing information
 *
 */


// turn off warnings under Windows
#ifdef WIN32
#pragma warning(disable:4786)
#pragma warning(disable:4503)
#endif

#include "MultiTransport.h"
#include "ctlapack.h"
#include "../../ext/math/gmres.h"

#include "DenseMatrix.h"
#include "polyfit.h"
#include "utilities.h"
#include "L_matrix.h"
#include "TransportParams.h"
#include "IdealGasPhase.h"

#include <iostream>

/** 
 * Mole fractions below MIN_X will be set to MIN_X when computing
 * transport properties.
 */

#define MIN_X 1.e-20


namespace Cantera {


    template<class S>
    struct UpdateSpeciesVisc : public Updater {
        UpdateSpeciesVisc(S& s) : Updater(), m_s(s) {}
        void update() { m_s._update_species_visc_T(); }
        S& m_s;
    };

    template<class S>
    struct UpdateVisc_T : public Updater {
        UpdateVisc_T(S& s) : Updater(), m_s(s) {}
        void update() { m_s._update_visc_T(); }
        S& m_s;
    };

    template<class S>
    struct UpdateDiff_T : public Updater {
        UpdateDiff_T(S& s) : Updater(), m_s(s) {}
        void update() { m_s._update_diff_T(); }
        S& m_s;
    };

    template<class S>
    struct UpdateThermal_T : public Updater {
        UpdateThermal_T(S& s) : Updater(), m_s(s) {}
        void update() { m_s._update_thermal_T(); }
        S& m_s;
    };


    /////////////////////////// constants //////////////////////////

    const doublereal ThreeSixteenths = 3.0/16.0;



    ///////////////////// helper functions /////////////////////////


    /**
     *  @internal
     *
     *  The Parker temperature correction to the rotational collision
     *  number. 
     *
     *  @param tr Reduced temperature \f$ \epsilon/kT \f$
     *  @param sqtr square root of tr.
     */
    inline doublereal Frot(doublereal tr, doublereal sqtr) {
        const doublereal c1 = 0.5*SqrtPi*Pi;
        const doublereal c2 = 0.25*Pi*Pi + 2.0;
        const doublereal c3 = SqrtPi*Pi;
        return 1.0 + c1*sqtr + c2*tr + c3*sqtr*tr;
    }


        /**
         * This method is used by GMRES to multiply the L matrix by a
         * vector b.  The L matrix has a 3x3 block structure, where each
         * block is a K x K matrix.  The elements of the upper-right and
         * lower-left blocks are all zero.  This method is defined so
         * that the multiplication only involves the seven non-zero
         * blocks.
         */
        void L_Matrix::mult(const doublereal* b, doublereal* prod) const {
            integer n = nRows()/3;
            integer n2 = 2*n;
            integer n3 = 3*n;
            ct_dgemv(ctlapack::ColMajor, ctlapack::NoTranspose, n, n2, 1.0, 
                data().begin(), nRows(), b, 1, 0.0, prod, 1);
            ct_dgemv(ctlapack::ColMajor, ctlapack::NoTranspose, n, n3, 1.0, 
                data().begin() + n, nRows(), b, 1, 0.0, prod+n, 1);
            ct_dgemv(ctlapack::ColMajor, ctlapack::NoTranspose, n, n, 1.0, 
                data().begin() + n*n3 + n2, nRows(), b + n, 1, 0.0, prod+n2, 1);
            for (int i = 0; i < n; i++)
                prod[i + n2] += b[i + n2] * value(i + n2, i + n2);
        }

    //////////////////// class MultiTransport methods //////////////


    MultiTransport::MultiTransport(thermo_t* thermo) 
        : Transport(thermo) {}

    bool MultiTransport::init(TransportParams& tr) {

        // constant mixture attributes
        //m_phase = tr.mix;
        m_thermo = tr.thermo;
        m_nsp   = m_thermo->nSpecies();
        m_tmin  = m_thermo->minTemp();
        m_tmax  = m_thermo->maxTemp();

        // make a local copy of the molecular weights
        m_mw.resize(m_nsp);
        copy(m_thermo->molecularWeights().begin(), 
            m_thermo->molecularWeights().end(), m_mw.begin());

        // copy polynomials and parameters into local storage
        m_poly       = tr.poly;
        m_visccoeffs = tr.visccoeffs;
        m_diffcoeffs = tr.diffcoeffs;
        m_astar_poly = tr.astar_poly;
        m_bstar_poly = tr.bstar_poly;
        m_cstar_poly = tr.cstar_poly;
        m_om22_poly  = tr.omega22_poly;
        m_zrot       = tr.zrot;
        m_crot       = tr.crot;
        m_epsilon    = tr.epsilon;
        m_mode       = tr.mode;
        m_diam       = tr.diam;
        m_eps        = tr.eps;

        // the L matrix
        m_Lmatrix.resize(3*m_nsp, 3*m_nsp);
        m_a.resize(3*m_nsp, 1.0);
        m_b.resize(3*m_nsp, 0.0);
        m_aa.resize(m_nsp, m_nsp, 0.0);

        m_frot_298.resize(m_nsp);
        m_rotrelax.resize(m_nsp);

        m_phi.resize(m_nsp, m_nsp, 0.0);
        m_cinternal.resize(m_nsp);

        m_polytempvec.resize(5);
        m_visc.resize(m_nsp);
        m_bdiff.resize(m_nsp, m_nsp);

        //m_poly.resize(m_nsp);
        m_om22.resize(m_nsp, m_nsp);
        m_astar.resize(m_nsp, m_nsp);
        m_bstar.resize(m_nsp, m_nsp);
        m_cstar.resize(m_nsp, m_nsp);

        m_molefracs.resize(m_nsp);

        // set flags all false
        m_visc_ok = false;
        m_spvisc_ok = false;
        m_diff_ok = false;
        m_abc_ok = false;
        m_l0000_ok = false;
        m_lmatrix_soln_ok = false;

        // use LU decomposition by default
        m_gmres = false;
 
        // default GMRES parameters
        m_mgmres = 100;
        m_eps_gmres = 1.e-4;

        // some work space
        m_spwork.resize(m_nsp);


        // precompute and store log(epsilon_ij/k_B)
        m_log_eps_k.resize(m_nsp, m_nsp);
        int i, j;
        for (i = 0; i < m_nsp; i++) {
            for (j = i; j < m_nsp; j++) {
                m_log_eps_k(i,j) = log(tr.epsilon(i,j)/Boltzmann);
                m_log_eps_k(j,i) = m_log_eps_k(i,j);
            }
        }


        // precompute and store constant parts of the Parker rotational
        // collision number temperature correction
        const doublereal sq298 = sqrt(298.0);
        const doublereal kb298 = Boltzmann * 298.0;
        m_sqrt_eps_k.resize(m_nsp);
        int k;
        for (k = 0; k < m_nsp; k++) {
            m_sqrt_eps_k[k] = sqrt(tr.eps[k]/Boltzmann); 
            m_frot_298[k] = Frot( tr.eps[k]/kb298, 
                m_sqrt_eps_k[k]/sq298);
        }

        // install updaters
        m_update_transport_T = m_thermo->installUpdater_T(
            new UpdateTransport_T<MultiTransport>(*this));
        m_update_transport_C = m_thermo->installUpdater_C(
            new UpdateTransport_C<MultiTransport>(*this));
        m_update_spvisc_T = m_thermo->installUpdater_T(
            new UpdateSpeciesVisc<MultiTransport>(*this));
        m_update_visc_T = m_thermo->installUpdater_T(
            new UpdateVisc_T<MultiTransport>(*this));
        m_update_diff_T = m_thermo->installUpdater_T(
            new UpdateDiff_T<MultiTransport>(*this));
        m_update_thermal_T = m_thermo->installUpdater_T(
            new UpdateThermal_T<MultiTransport>(*this));

        return true;
    }


    /******************  viscosity ******************************/

    doublereal MultiTransport::viscosity() {
        doublereal vismix = 0.0, denom;
        int k, j;

        // update m_visc if necessary
        updateViscosity_T();

        // update the mole fractions
        updateTransport_C();

        for (k = 0; k < m_nsp; k++) {
            denom = 0.0;
            for (j = 0; j < m_nsp; j++) {
                denom += m_phi(k,j) * m_molefracs[j];
            }
            vismix += m_molefracs[k] * m_visc[k]/denom;
        }
        return vismix;
    }



    /******************* binary diffusion coefficients **************/

    void MultiTransport::getBinaryDiffCoeffs(int ld, doublereal* d) {
        int i,j;

        // if necessary, evaluate the binary diffusion coefficents
        // from the polynomial fits
        updateDiff_T();

        doublereal p = pressure_ig();
        doublereal rp = 1.0/p;    
        for (i = 0; i < m_nsp; i++) 
            for (j = 0; j < m_nsp; j++) {
                d[ld*j + i] = rp * m_bdiff(i,j);
            }
    }



    /****************** thermal conductivity **********************/

    /**
     * @internal
     */
    doublereal MultiTransport::thermalConductivity() {
        
        solveLMatrixEquation();
        doublereal sum = 0.0;
        int k;
        for (k = 0; k  < 2*m_nsp; k++) {
            sum += m_b[k + m_nsp] * m_a[k + m_nsp];
        }
        return -4.0*sum;
    }


    /****************** thermal diffusion coefficients ************/

    /**
     * @internal
     */
    void MultiTransport::getThermalDiffCoeffs(doublereal* dt) {

        solveLMatrixEquation();
        const doublereal c = 1.6/GasConstant;
        int k;
        for (k = 0; k < m_nsp; k++) {
            dt[k] = c * m_mw[k] * m_molefracs[k] * m_a[k];
        }
    }


    /**
     * @internal
     */
    void MultiTransport::solveLMatrixEquation() {

        // if T has changed, update the temperature-dependent
        // properties.
        
        updateThermal_T();
        updateTransport_C();

        // Copy the mole fractions twice into the last two blocks of
        // the right-hand-side vector m_b. The first block of m_b was
        // set to zero when it was created, and is not modified so
        // doesn't need to be reset to zero.
        int k;
        for (k = 0; k < m_nsp; k++) {
            m_b[k] = 0.0;
            m_b[k + m_nsp] = m_molefracs[k];
            m_b[k + 2*m_nsp] = m_molefracs[k];
        }

        // Set the right-hand side vector to zero in the 3rd block for
        // all species with no internal energy modes.  The
        // corresponding third-block rows and columns will be set to
        // zero, except on the diagonal of L01,01, where they are set
        // to 1.0. This has the effect of eliminating these equations
        // from the system, since the equation becomes: m_a[2*m_nsp +
        // k] = 0.0.

        // Note that this differs from the Chemkin procedure, where
        // all *monatomic* species are excluded. Since monatomic
        // radicals can have non-zero internal heat capacities due to
        // electronic excitation, they should be retained.
        //
        // But if CHEMKIN_COMPATIBILITY_MODE is defined, then all
        // monatomic species are excluded.

        for (k = 0; k < m_nsp; k++) {
            if (!hasInternalModes(k)) m_b[2*m_nsp + k] = 0.0;
        }

        // evaluate the submatrices of the L matrix
        
        m_Lmatrix.resize(3*m_nsp, 3*m_nsp, 0.0);

        eval_L0000(m_molefracs.begin());
        eval_L0010(m_molefracs.begin());
        eval_L0001();
        eval_L1000();
        eval_L1010(m_molefracs.begin());
        eval_L1001(m_molefracs.begin());
        eval_L0100();
        eval_L0110();
        eval_L0101(m_molefracs.begin());


        // Solve it using GMRES or LU decomposition. The last solution
        // in m_a should provide a good starting guess, so convergence
        // should be fast.

        if (m_gmres) {
            gmres(m_mgmres, 3*m_nsp, m_Lmatrix, m_b.begin(), 
                m_a.begin(), m_eps_gmres);
            m_lmatrix_soln_ok = true;
            m_l0000_ok = true;            // L matrix not modified by GMRES
        }
        else {
            copy(m_b.begin(), m_b.end(), m_a.begin());
            int info = solve(m_Lmatrix, m_a.begin());
            if (info != 0) {
                throw CanteraError("MultiTransport::solveLMatrixEquation",
                    "error in solving L matrix.");
            }
            m_lmatrix_soln_ok = true;
            m_l0000_ok = false;          
            // L matrix is overwritten with LU decomposition
        }
        m_lmatrix_soln_ok = true;
    }


    /**
     * 
     */
    void MultiTransport::getSpeciesFluxes(int ndim, 
        doublereal* grad_T, int ldx, const doublereal* grad_X, 
        int ldf, doublereal* fluxes) {

        // update the binary diffusion coefficients if necessary
        updateDiff_T();

        doublereal sum;
        int i, j;

        // If any component of grad_T is non-zero, then get the
        // thermal diffusion coefficients

        bool addThermalDiffusion = false;
        for (i = 0; i < ndim; i++) {
            if (grad_T[i] != 0.0) addThermalDiffusion = true;
        }
        if (addThermalDiffusion) getThermalDiffCoeffs(m_spwork.begin());

        const doublereal* y = m_thermo->massFractions();
        doublereal rho = m_thermo->density();

        for (i = 0; i < m_nsp; i++) {
            sum = 0.0;
            for (j = 0; j < m_nsp; j++) {
                m_aa(i,j) = m_molefracs[j]*m_molefracs[i]/m_bdiff(i,j);
                sum += m_aa(i,j);
            }
            m_aa(i,i) -= sum;
        }

        // enforce the condition \sum Y_k V_k = 0. This is done by replacing 
        // the flux equation with the largest gradx component in the first 
        // coordinate direction with the flux balance condition.
        int jmax = 0;
        doublereal gradmax = -1.0;
        for (j = 0; j < m_nsp; j++) {
            if (fabs(grad_X[j]) > gradmax) {
                gradmax = fabs(grad_X[j]);
                jmax = j;
            }
        }

        // set the matrix elements in this row to the mass fractions,
        // and set the entry in gradx to zero

        for (j = 0; j < m_nsp; j++) {
            m_aa(jmax,j) = y[j];
        }
        vector_fp gsave(ndim), grx(ldx*m_nsp);
        int n;
        for (n = 0; n < ldx*ndim; n++) {
            grx[n] = grad_X[n];
        }
        //for (n = 0; n < ndim; n++) {
        //    gsave[n] = grad_X[jmax + n*ldx];   // save the input mole frac gradient
            //grad_X[jmax + n*ldx] = 0.0;
        //    grx[jmax + n*ldx] = 0.0;
        // }

        // copy grad_X to fluxes
        const doublereal* gx;
        for (n = 0; n < ndim; n++) {
            gx = grad_X + ldx*n;
            copy(gx, gx + m_nsp, fluxes + ldf*n);
            fluxes[jmax + n*ldf] = 0.0;
        }

        // use LAPACK to solve the equations
        int info=0;
        ct_dgetrf(m_aa.nRows(), m_aa.nColumns(), m_aa.begin(), m_aa.nRows(), 
            m_aa.ipiv().begin(), info);
        if (info == 0) { 
            ct_dgetrs(ctlapack::NoTranspose, m_aa.nRows(), ndim, 
                m_aa.begin(), m_aa.nRows(), 
                m_aa.ipiv().begin(), fluxes, ldf, info);
            if (info != 0) info += 100;
        }
        else 
            throw CanteraError("MultiTransport::getSpeciesFluxes",
                "Error in DGETRF");
        if (info > 50)
            throw CanteraError("MultiTransport::getSpeciesFluxes",
                "Error in DGETRS");

        
        int offset;
        doublereal pp = pressure_ig();

        // multiply diffusion velocities by rho * V to create
        // mass fluxes, and restore the gradx elements that were
        // modified        
        for (n = 0; n < ndim; n++) {
            offset = n*ldf;
            for (i = 0; i < m_nsp; i++) {
                fluxes[i + offset] *= rho * y[i] / pp;
            }
            //grad_X[jmax + n*ldx] = gsave[n];
        }

        // thermal diffusion
        if (addThermalDiffusion) {
            for (n = 0; n < ndim; n++) {
                offset = n*ldf;
                doublereal grad_logt = grad_T[n]/m_temp;
                for (i = 0; i < m_nsp; i++) 
                    fluxes[i + offset] -= m_spwork[i]*grad_logt;
            }
        }
    }


    void MultiTransport::getMultiDiffCoeffs(int ld, doublereal* d) {
        int i,j;

        doublereal p = pressure_ig();

        // update the mole fractions
        updateTransport_C();

        // update the binary diffusion coefficients
        updateDiff_T();

        // evaluate L0000 if the temperature or concentrations have
        // changed since it was last evaluated.
        if (!m_l0000_ok) eval_L0000(m_molefracs.begin());
        
        // invert L00,00
        int ierr = invert(m_Lmatrix, m_nsp);
        if (ierr != 0) {
            cout << " invert returned ierr = " << ierr << endl;
            exit(1);
        }
        m_l0000_ok = false;           // matrix is overwritten by inverse

        //doublereal pres = m_thermo->pressure();
        doublereal prefactor = 16.0 * m_temp 
                               * m_thermo->meanMolecularWeight()/(25.0 * p);
        doublereal c;

        for (i = 0; i < m_nsp; i++) {
            for (j = 0; j < m_nsp; j++) {            
                c = prefactor/m_mw[j];
                d[ld*j + i] = c*m_molefracs[i]*
                              (m_Lmatrix(i,j) - m_Lmatrix(i,i));
            }
        }
    }

                 
    /**
     *  Update temperature-dependent quantities. This method is called
     *  by the temperature property updater.
     */ 
    void MultiTransport::_update_transport_T() 
    {
        //if (m_temp == m_thermo->temperature()) return;

        m_temp = m_thermo->temperature();
        m_logt = log(m_temp);
        m_kbt = Boltzmann * m_temp;
        m_sqrt_t = sqrt(m_temp);
        m_t32 = m_temp * m_sqrt_t;
        m_sqrt_kbt = sqrt(Boltzmann*m_temp);

        // compute powers of log(T)
        m_polytempvec[0] = 1.0;
        m_polytempvec[1] = m_logt;
        m_polytempvec[2] = m_logt*m_logt;
        m_polytempvec[3] = m_logt*m_logt*m_logt;
        m_polytempvec[4] = m_logt*m_logt*m_logt*m_logt;

        // temperature has changed, so polynomial fits will need to be
        // redone, and the L matrix reevaluated.
        m_visc_ok = false;
        m_spvisc_ok = false;
        m_diff_ok = false;
        m_abc_ok  = false;
        m_lmatrix_soln_ok = false;
        m_l0000_ok = false;
    }                 

    /**
     *  This is called the first time any transport property
     *  is requested from ThermoSubstance after the concentrations
     *  have changed.
     */ 
    void MultiTransport::_update_transport_C()  
    {
        // signal that concentration-dependent quantities will need to
        // be recomputed before use, and update the local mole
        // fraction array.
        m_l0000_ok = false;
        m_lmatrix_soln_ok = false;
        m_thermo->getMoleFractions(m_molefracs.begin());


        // add an offset to avoid a pure species condition
        // (check - this may be unnecessary)
        int k;
        for (k = 0; k < m_nsp; k++) {
            m_molefracs[k] = fmaxx(MIN_X, m_molefracs[k]);
        }
    }


    /*************************************************************************
     *
     *    methods to update temperature-dependent properties
     *
     *************************************************************************/

    /**
     * @internal
     * Update the binary diffusion coefficients. These are evaluated
     * from the polynomial fits at unit pressure (1 Pa).
     */
    void MultiTransport::updateDiff_T() {
        m_thermo->update_T(m_update_diff_T);
    }

    void MultiTransport::_update_diff_T() {

        updateTransport_T();

        // evaluate binary diffusion coefficients at unit pressure
        int i,j;
        int ic = 0;
        if (m_mode == CK_Mode) {
            for (i = 0; i < m_nsp; i++) {
                for (j = i; j < m_nsp; j++) {
                    m_bdiff(i,j) = exp(dot4(m_polytempvec, m_diffcoeffs[ic]));
                    m_bdiff(j,i) = m_bdiff(i,j);
                    ic++;
                }
            }
        }
        else {
            for (i = 0; i < m_nsp; i++) {
                for (j = i; j < m_nsp; j++) {
                    m_bdiff(i,j) = m_temp * m_sqrt_t*dot5(m_polytempvec, 
                        m_diffcoeffs[ic]);
                    m_bdiff(j,i) = m_bdiff(i,j);
                    ic++;
                }
            }
        } 
        m_diff_ok = true;
    }


    /**
     * @internal
     * Update the temperature-dependent viscosity terms.
     * Updates the array of pure species viscosities, and the 
     * weighting functions in the viscosity mixture rule.
     * The flag m_visc_ok is set to true.
     */
    void MultiTransport::updateSpeciesViscosities_T() {
        m_thermo->update_T(m_update_spvisc_T);
    }


    void MultiTransport::_update_species_visc_T() {

        updateTransport_T();

        int k;
        if (m_mode == CK_Mode) {
            for (k = 0; k < m_nsp; k++) {
                m_visc[k] = exp(dot4(m_polytempvec, m_visccoeffs[k]));
            }
        }
        else {
            for (k = 0; k < m_nsp; k++) {
                m_visc[k] = m_sqrt_t*dot5(m_polytempvec, m_visccoeffs[k]);
            }
        }
        m_spvisc_ok = true;
    }

    /**
     * @internal
     */
    void MultiTransport::updateViscosity_T() {
        m_thermo->update_T(m_update_visc_T);
    }

    void MultiTransport::_update_visc_T() {
        doublereal vratiokj, wratiojk, rootwjk, factor1;

        updateSpeciesViscosities_T();

        // see Eq. (9-5.15) of Reid, Prausnitz, and Poling
        int j, k;
        for (j = 0; j < m_nsp; j++) {
            for (k = j; k < m_nsp; k++) {
                vratiokj = m_visc[k]/m_visc[j];
                wratiojk = m_mw[j]/m_mw[k];
                rootwjk = sqrt(wratiojk);
                factor1 = 1.0 + sqrt(vratiokj * rootwjk);
                m_phi(k,j) = factor1*factor1 /
                             (SqrtEight * sqrt(1.0 + m_mw[k]/m_mw[j]));
                m_phi(j,k) = m_phi(k,j)/(vratiokj * wratiojk);
            }
        }
        m_visc_ok = true;
    }


    /**
     * @internal
     * Update the temperature-dependent terms needed to compute the
     * thermal conductivity and thermal diffusion coefficients.
     */
    void MultiTransport::updateThermal_T() {
        m_thermo->update_T(m_update_thermal_T);
    }

    void MultiTransport::_update_thermal_T() {
        // we need species viscosities and binary diffusion
        // coefficients
        updateSpeciesViscosities_T();
        updateDiff_T();

        // evaluate polynomial fits for A*, B*, C*
        doublereal z;
        int ipoly;
        int i, j;
        for (i = 0; i < m_nsp; i++) {
            for (j = i; j < m_nsp; j++) {
                z = m_logt - m_log_eps_k(i,j);
                ipoly = m_poly[i][j];
                if (m_mode == CK_Mode) {
                    m_om22(i,j) = poly6(z, m_om22_poly[ipoly].begin());
                    m_astar(i,j) = poly6(z, m_astar_poly[ipoly].begin());
                    m_bstar(i,j) = poly6(z, m_bstar_poly[ipoly].begin());
                    m_cstar(i,j) = poly6(z, m_cstar_poly[ipoly].begin());
                }
                else {
                    m_om22(i,j) = poly8(z, m_om22_poly[ipoly].begin());
                    m_astar(i,j) = poly8(z, m_astar_poly[ipoly].begin());
                    m_bstar(i,j) = poly8(z, m_bstar_poly[ipoly].begin());
                    m_cstar(i,j) = poly8(z, m_cstar_poly[ipoly].begin());
                }
                m_om22(j,i)  = m_om22(i,j);
                m_astar(j,i) = m_astar(i,j);
                m_bstar(j,i) = m_bstar(i,j);
                m_cstar(j,i) = m_cstar(i,j);
            }
        }
        m_abc_ok = true;

        // evaluate the temperature-dependent rotational relaxation
        // rate

        int k;
        doublereal tr, sqtr;
        for (k = 0; k < m_nsp; k++) {
            tr = m_eps[k]/ m_kbt;
            sqtr = m_sqrt_eps_k[k] / m_sqrt_t;
            m_rotrelax[k] = fmaxx(1.0,m_zrot[k]) * m_frot_298[k]/Frot(tr, sqtr);
        }

        doublereal d;
        doublereal c = 1.2*GasConstant*m_temp;
        for (k = 0; k < m_nsp; k++) {
            d = c * m_visc[k] * m_astar(k,k)/m_mw[k];
            m_bdiff(k,k) = d;
        }

        // internal heat capacities
        const array_fp& cp = ((IdealGasPhase*)m_thermo)->cp_R();
        for (k = 0; k < m_nsp; k++) m_cinternal[k] = cp[k] - 2.5;
    }
}
