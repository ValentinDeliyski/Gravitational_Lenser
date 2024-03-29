#pragma once

#include <cmath>

#include "Structs.h"
#include "Spacetimes.h"
#include "Constants.h"

double* Gauss_Bonnet_class::get_ISCO() {

    /**************************************************************************
    |                                                                         |
    |   @ Description: Returns a pointer to the inner and outer ISCO radii.   |
    |     * The outer ISCO is the solution to the equation:                   |
    |       d2r_f(r) + 3 * dr_f(r) / r - 2 * dr_f(r)**2 / f(r) = 0            |
    |     * GAUSS_BONNET_GAMMA is in the range [0, 1.5]                       |
    |                                                                         |
    |   @ Inputs: None                                                        |
    |                                                                         |
    |   @ Ouput: Pointer to the ISCO radii                                    |
    |                                                                         |
    **************************************************************************/

    static double r_ISCO[2]{};

    double fit_coeffs[11] = { 5.99998915, -0.61042681, -0.11593137,  0.07275861, -0.46946788,
                              1.20693793, -1.99054947,  2.05041439, -1.29496979,  0.45787902, -0.07008574 };

    double Gamma2  = GAUSS_BONNET_GAMMA * GAUSS_BONNET_GAMMA;
    double Gamma4  = Gamma2 * Gamma2;
    double Gamma8  = Gamma4 * Gamma4;
    double Gamma10 = Gamma8 * Gamma2;

    r_ISCO[Outer] = fit_coeffs[0]  + 
                    fit_coeffs[1]  * GAUSS_BONNET_GAMMA +
                    fit_coeffs[2]  * Gamma2 +
                    fit_coeffs[3]  * Gamma2 * GAUSS_BONNET_GAMMA +
                    fit_coeffs[4]  * Gamma4 +
                    fit_coeffs[5]  * Gamma4 * GAUSS_BONNET_GAMMA +
                    fit_coeffs[6]  * Gamma4 * Gamma2 + 
                    fit_coeffs[7]  * Gamma8 / GAUSS_BONNET_GAMMA + 
                    fit_coeffs[8]  * Gamma8 + 
                    fit_coeffs[9]  * Gamma8 * GAUSS_BONNET_GAMMA + 
                    fit_coeffs[10] * Gamma10;

    r_ISCO[Inner] = pow(GAUSS_BONNET_GAMMA, 1.0 / 3);

    return r_ISCO;

};

double* Gauss_Bonnet_class::get_Photon_Sphere() {

    /* This expression is the root of a cubic equation */

    double q =  8 * MASS * GAUSS_BONNET_GAMMA;
    double p = -9 * MASS * MASS;

    static double photon_orbits[2]{};

    photon_orbits[Outer] = 2 * sqrt(-p / 3) * cos(1. / 3 * acos(3. / 2 * q / p * sqrt(-3. / p)));
    photon_orbits[Inner] = 2 * sqrt(-p / 3) * cos(1. / 3 * acos(3. / 2 * q / p * sqrt(-3. / p)) + 2. * M_PI / 3);

    return photon_orbits;

};

int Gauss_Bonnet_class::update_metric(double State_Vector[]) {

    double  M     = MASS;
    double& r     = State_Vector[e_r];
    double& theta = State_Vector[e_theta];

    double r2 = r * r;
    double sin_theta = sin(theta);

    double f = 1. + r2 / GAUSS_BONNET_GAMMA / 2. * (1. - sqrt(1. + 8. * GAUSS_BONNET_GAMMA * M / r2 / r));

    this->s_Metric.Metric[0][0] = -f;
    this->s_Metric.Metric[1][1] = 1. / f;
    this->s_Metric.Metric[2][2] = r2;
    this->s_Metric.Metric[3][3] = r2 * sin_theta * sin_theta;
    this->s_Metric.Metric[0][3] = 0.;
    this->s_Metric.Metric[3][0] = 0.;

    this->s_Metric.Lapse_function = -this->s_Metric.Metric[0][0];
    this->s_Metric.Shift_function = 0.;

    return OK;

}

Metric_type Gauss_Bonnet_class::get_metric(double State_Vector[]) {

    this->update_metric(State_Vector);

    return this->s_Metric;

}

int Gauss_Bonnet_class::update_dr_metric(double State_Vector[]) {

    double M      = MASS;
    double& r     = State_Vector[e_r];
    double& theta = State_Vector[e_theta];

    double r2 = r * r;
    double sin_theta = sin(theta);

    double f    = 1. + r2 / GAUSS_BONNET_GAMMA / 2. * (1. - sqrt(1. + 8. * GAUSS_BONNET_GAMMA * M / r2 / r));
    double dr_f = 2. / r * (f - 1.) + 6. * M / sqrt(r2 * r2 + 8. * GAUSS_BONNET_GAMMA * M * r);

    this->s_dr_Metric.Metric[0][0] = -dr_f;
    this->s_dr_Metric.Metric[1][1] = -1. / f / f * dr_f;
    this->s_dr_Metric.Metric[2][2] = 2. * r;
    this->s_dr_Metric.Metric[3][3] = 2. * r * sin_theta * sin_theta;
    this->s_dr_Metric.Metric[0][3] = 0.;
    this->s_dr_Metric.Metric[3][0] = 0.;

    this->s_dr_Metric.Lapse_function = -this->s_dr_Metric.Metric[0][0];
    this->s_dr_Metric.Shift_function = 0.;

    return OK;

}

Metric_type Gauss_Bonnet_class::get_dr_metric(double State_Vector[]) {

    this->update_dr_metric(State_Vector);

    return this->s_dr_Metric;

}

int Gauss_Bonnet_class::update_dtheta_metric(double State_Vector[]) {

    double& r = State_Vector[e_r];
    double& theta = State_Vector[e_theta];

    double sin_theta = sin(theta);
    double cos_theta = cos(theta);

    this->s_dtheta_Metric.Metric[0][0] = 0.0;
    this->s_dtheta_Metric.Metric[0][3] = 0.0;
    this->s_dtheta_Metric.Metric[3][0] = this->s_dtheta_Metric.Metric[0][3];
    this->s_dtheta_Metric.Metric[1][1] = 0.0;
    this->s_dtheta_Metric.Metric[2][2] = 0.0;
    this->s_dtheta_Metric.Metric[3][3] = 2 * r * r * sin_theta * cos_theta;

    this->s_dtheta_Metric.Lapse_function = 0.0;
    this->s_dtheta_Metric.Shift_function = 0.0;

    return OK;

}

Metric_type Gauss_Bonnet_class::get_dtheta_metric(double State_Vector[]) {

    this->update_dtheta_metric(State_Vector);

    return this->s_dtheta_Metric;
}

int Gauss_Bonnet_class::update_d2r_metric(double State_Vector[]) {

    double  M     = MASS;
    double& r     = State_Vector[e_r];
    double& theta = State_Vector[e_theta];

    double r2 = r * r;
    double sin_theta = sin(theta);

    double root = sqrt(r2 * r2 + 8 * GAUSS_BONNET_GAMMA * M * r);

    double f     =  1 + r2 / GAUSS_BONNET_GAMMA / 2. * (1 - sqrt(1. + 8. * GAUSS_BONNET_GAMMA * M / r2 / r));
    double dr_f  =  2. / r * (f - 1.) + 6 * M / root;
    double d2r_f = -2. / r2 * (f - 1.) + 2. / r * dr_f - 12. * M / root / root / root * (r2 * r + 2. * GAUSS_BONNET_GAMMA * M);

    this->s_d2r_Metric.Metric[0][0] = -d2r_f;
    this->s_d2r_Metric.Metric[1][1] = 2. / f / f / f * dr_f - 1. / f / f * d2r_f;
    this->s_d2r_Metric.Metric[2][2] = 2.;
    this->s_d2r_Metric.Metric[3][3] = 2. * sin_theta * sin_theta;
    this->s_d2r_Metric.Metric[0][3] = 0.;
    this->s_d2r_Metric.Metric[3][0] = 0.;

    this->s_d2r_Metric.Lapse_function = -this->s_d2r_Metric.Metric[0][0];
    this->s_d2r_Metric.Shift_function = 0.;

    return OK;

}

Metric_type Gauss_Bonnet_class::get_d2r_metric(double State_Vector[]) {

    this->update_d2r_metric(State_Vector);

    return this->s_d2r_Metric;

}

int Gauss_Bonnet_class::get_initial_conditions_from_file(Initial_conditions_type* p_Initial_Conditions, double J_data[], double p_theta_data[], int photon) {

    double& r_obs     = p_Initial_Conditions->init_Pos[e_r];
    double& theta_obs = p_Initial_Conditions->init_Pos[e_theta];

    p_Initial_Conditions->init_Three_Momentum[e_phi] = -J_data[photon] * sin(theta_obs);
    p_Initial_Conditions->init_Three_Momentum[e_theta] = p_theta_data[photon];

    double& J = p_Initial_Conditions->init_Three_Momentum[e_phi];
    double  f = 1. + r_obs * r_obs / 2. / GAUSS_BONNET_GAMMA * (1. - sqrt(1. + 8. * GAUSS_BONNET_GAMMA * MASS / r_obs / r_obs / r_obs));

    double rad_potential = 1. - f * J * J / (r_obs * r_obs);

    double(*metric)[4] = p_Initial_Conditions->init_metric;

    p_Initial_Conditions->init_Three_Momentum[e_r] = sqrt(rad_potential) * metric[1][1];

    return OK;

}

int Gauss_Bonnet_class::get_EOM(double State_vector[], double Derivatives[]){

    double& r = State_vector[e_r];
    double& J = State_vector[e_p_phi];

    double sin1 = sin(State_vector[e_theta]);
    double sin2 = sin1 * sin1;

    double cos1 = cos(State_vector[e_theta]);
    double cos2 = cos1 * cos1;

    double root = sqrt(1. + 8. * GAUSS_BONNET_GAMMA * MASS / r / r / r);

    double f    = 1. + r * r / GAUSS_BONNET_GAMMA / 2. * (1. - root);
    double dr_f = 2. / r * (f - 1.) + 6. * MASS / root / r / r;

    *(Derivatives + e_r      ) = f * State_vector[e_p_r];
    *(Derivatives + e_theta  ) = 1. / (r * r) * State_vector[e_p_theta];
    *(Derivatives + e_phi    ) = J / (r * r * sin2);
    *(Derivatives + e_p_phi  ) = 0.0;
    *(Derivatives + e_p_theta) = cos1 / (r * r * sin1 * sin2) * J * J;

    double r_term_1 = -1. / 2 * (1.0 / f / f + State_vector[e_p_r] * State_vector[e_p_r]) * dr_f;
    double r_term_2 = 1.0 / r / r / r * (State_vector[e_p_theta] * State_vector[e_p_theta] + J * J / sin2);

    *(Derivatives + e_p_r) = r_term_1 + r_term_2;

    return OK;

}

bool Gauss_Bonnet_class::terminate_integration(double State_vector[], double Derivatives[]) {

    bool scatter = State_vector[e_r] > 100 && Derivatives[e_r] < 0;

    bool too_high_order = State_vector[e_phi] * State_vector[e_phi] > 5 * M_PI * 5 * M_PI;

    double r_horizon{};
    bool hit_horizon = false;

    if (GAUSS_BONNET_GAMMA <= 1) {

        r_horizon = MASS + sqrt(MASS * MASS - GAUSS_BONNET_GAMMA * GAUSS_BONNET_GAMMA);
        hit_horizon = State_vector[e_r] - r_horizon < 1e-5;

    }

    return scatter || too_high_order || hit_horizon;

};
