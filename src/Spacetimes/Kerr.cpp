#pragma once

#include <cmath>

#include "Structs.h"
#include "Spacetimes.h"
#include "Constants.h"

double* Kerr_class::get_ISCO() {

    double Z_1 = 1 + pow(1 - SPIN * SPIN, 1. / 3) * (pow(1 + SPIN, 1. / 3) + pow(1 - SPIN, 1. / 3));
    double Z_2 = sqrt(3 * SPIN * SPIN + Z_1 * Z_1);

    static double r_ISCO[2]{};

    r_ISCO[Inner] = MASS * (3 + Z_2 - sqrt((3 - Z_1) * (3 + Z_1 + 2 * Z_2)));
    r_ISCO[Outer] = MASS * (3 + Z_2 + sqrt((3 - Z_1) * (3 + Z_1 + 2 * Z_2)));

    return r_ISCO;

}

double* Kerr_class::get_Photon_Sphere() {

    static double photon_orbit[2]{};

    photon_orbit[Inner] = 2 * MASS * (1 + cos(2.0 / 3 * acos(SPIN)));
    photon_orbit[Outer] = 2 * MASS * (1 + cos(2.0 / 3 * acos(-SPIN)));

    return photon_orbit;

}

int Kerr_class::update_metric(double State_Vector[]) {

    double M = MASS;
    double a = SPIN;

    double& r     = State_Vector[e_r];
    double& theta = State_Vector[e_theta];

    double r2 = r * r;
    double sin_theta = sin(theta);
    double cos_theta = cos(theta);
    double rho2 = r2 + a * a * cos_theta * cos_theta;
    double delta = r2 - 2 * M * r + a * a;

    this->s_Metric.Metric[0][0] = -(1 - 2 * M * r / rho2);
    this->s_Metric.Metric[0][3] = -2 * M * r * a * sin_theta * sin_theta / rho2;
    this->s_Metric.Metric[3][0] = this->s_Metric.Metric[0][3];
    this->s_Metric.Metric[1][1] = rho2 / delta;
    this->s_Metric.Metric[2][2] = rho2;
    this->s_Metric.Metric[3][3] = (r2 + a * a + 2 * M * r * a * a / rho2 * sin_theta * sin_theta) * sin_theta * sin_theta;

    double sigma2 = (r2 + a * a) * (r2 + a * a) - a * a * delta * sin_theta * sin_theta;

    this->s_Metric.Lapse_function = sqrt(rho2 * delta / sigma2);
    this->s_Metric.Shift_function = 2 * a * r / sigma2;

    return OK;

};

Metric_type Kerr_class::get_metric(double State_vector[]) {

    this->update_metric(State_vector);

    return this->s_Metric;
}

int Kerr_class::update_dr_metric(double State_Vector[]) {

    double M = MASS;
    double a = SPIN;

    double& r     = State_Vector[e_r];
    double& theta = State_Vector[e_theta];

    double r2 = r * r;
    double sin_theta = sin(theta);
    double cos_theta = cos(theta);
    double rho2 = r2 + a * a * cos_theta * cos_theta;
    double delta = r2 - 2 * M * r + a * a;

    this->s_dr_Metric.Metric[0][0] = -2 * M * r / rho2 * (2 * r2 / rho2 - 1);
    this->s_dr_Metric.Metric[0][3] = 2 * M * a * sin_theta * sin_theta / rho2 * (2 * r2 / rho2 - 1);
    this->s_dr_Metric.Metric[3][0] = this->s_dr_Metric.Metric[0][3];
    this->s_dr_Metric.Metric[1][1] = 2 * r / delta * (1 - rho2 / delta * (1 - M / r));
    this->s_dr_Metric.Metric[2][2] = 2 * r;
    this->s_dr_Metric.Metric[3][3] = 2 * (r - M * a * a / rho2 * (2 * r2 / rho2 - 1) * sin_theta * sin_theta) * sin_theta * sin_theta;

    double sigma2 = rho2 * this->s_Metric.Metric[3][3] / sin_theta / sin_theta;
    double dr_sigma2 = 2 * r * this->s_Metric.Metric[3][3] + rho2 * this->s_dr_Metric.Metric[3][3];

    this->s_dr_Metric.Lapse_function = this->s_Metric.Lapse_function * (r / rho2 + (r - M) / delta - dr_sigma2 / 2 / sigma2);
    this->s_dr_Metric.Shift_function = this->s_Metric.Shift_function / r * (1 - r * dr_sigma2 / sigma2);

    return OK;

}

Metric_type Kerr_class::get_dr_metric(double State_Vector[]) {

    this->update_dr_metric(State_Vector);

    return this->s_dr_Metric;
}

int Kerr_class::update_dtheta_metric(double State_Vector[]) {

    double M = MASS;
    double a = SPIN;

    double& r = State_Vector[e_r];
    double& theta = State_Vector[e_theta];

    double r2 = r * r;
    double sin_theta = sin(theta);
    double cos_theta = cos(theta);
    double rho2 = r2 + a * a * cos_theta * cos_theta;
    double delta = r2 - 2 * M * r + a * a;

    this->s_dtheta_Metric.Metric[0][0] = 4 * M / rho2 / rho2 * (a * a * cos_theta * sin_theta);
    this->s_dtheta_Metric.Metric[0][3] = -4 * M * a * sin_theta * cos_theta / rho2 * (1 - a * a * sin_theta * sin_theta);
    this->s_dtheta_Metric.Metric[3][0] = this->s_dr_Metric.Metric[0][3];
    this->s_dtheta_Metric.Metric[1][1] = -2 * a * a * cos_theta * sin_theta / delta;
    this->s_dtheta_Metric.Metric[2][2] = 0.0;
    this->s_dtheta_Metric.Metric[3][3] = 4 * M * r * a * a * sin_theta * cos_theta / rho2 * (1 + a * a * sin_theta * sin_theta / rho2) * sin_theta * sin_theta + 
                                         2 * this->s_Metric.Metric[3][3] / sin_theta * cos_theta;

    double sigma2 = rho2 * this->s_Metric.Metric[3][3] / sin_theta / sin_theta;
    double dtheta_sigma2 = -2 * a * a * delta * sin_theta * cos_theta;

    this->s_dtheta_Metric.Lapse_function = this->s_Metric.Lapse_function * a * a * sin_theta * cos_theta * (delta / sigma2 - 1 / rho2);
    this->s_dtheta_Metric.Shift_function = -this->s_Metric.Shift_function * dtheta_sigma2 / sigma2;

    return OK;

}

Metric_type Kerr_class::get_dtheta_metric(double State_Vector[]) {

    this->update_dtheta_metric(State_Vector);

    return this->s_dtheta_Metric;
}

int Kerr_class::update_d2r_metric(double State_Vector[]) {

    double M = MASS;
    double a = SPIN;

    double& r     = State_Vector[e_r];
    double& theta = State_Vector[e_theta];

    double r2 = r * r;
    double sin_theta = sin(theta);
    double cos_theta = cos(theta);
    double rho2 = r2 + a * a * cos_theta * cos_theta;
    double delta = r2 - 2 * M * r + a * a;

    this->s_d2r_Metric.Metric[0][0] = 4 * M * r / rho2 / rho2 * (4 * r2 / rho2 - 3);
    this->s_d2r_Metric.Metric[0][3] = -4 * M * a * r * sin_theta * sin_theta / rho2 / rho2 * (4 * r2 / rho2 - 3);
    this->s_d2r_Metric.Metric[3][0] = this->s_d2r_Metric.Metric[0][3];
    this->s_d2r_Metric.Metric[1][1] = 2 / delta * (1 - 4 * (r2 - r * M) / delta + rho2 / delta * (2 * (r - M) * (r - M) / delta - 1));
    this->s_d2r_Metric.Metric[2][2] = 2.0;
    this->s_d2r_Metric.Metric[3][3] = 2 * (1 + 2 * M * a * a * r / rho2 / rho2 * (4 * r2 / rho2 - 3) * sin_theta * sin_theta) * sin_theta * sin_theta;

    double sigma2     = rho2 * this->s_Metric.Metric[3][3] / sin_theta / sin_theta;
    double dr_sigma2  = (2 * r * this->s_Metric.Metric[3][3] + rho2 * this->s_dr_Metric.Metric[3][3]) / sin_theta / sin_theta;
    double d2r_sigma2 = (2 * this->s_Metric.Metric[3][3] + 4 * r * this->s_dr_Metric.Metric[3][3] + rho2 * this->s_d2r_Metric.Metric[3][3]) / sin_theta / sin_theta;

    double& N    = this->s_Metric.Lapse_function;
    double& dr_N = this->s_dr_Metric.Lapse_function;
    this->s_d2r_Metric.Lapse_function = dr_N * dr_N / N + N / rho2 * (1 - 2 * r2 / rho2 + rho2 / delta * (1 - (r - M) * (r - M) / delta) - rho2 / sigma2 / 2 * (d2r_sigma2 - dr_sigma2 * dr_sigma2 / sigma2));

    double& omega    = this->s_Metric.Shift_function;
    double& dr_omega = this->s_dr_Metric.Shift_function;
    this->s_d2r_Metric.Shift_function = -omega / r2 * (1 - r * dr_omega / omega + r * dr_sigma2 / sigma2) * (1 - r * dr_sigma2 / sigma2);

    return OK;

}

Metric_type Kerr_class::get_d2r_metric(double State_Vector[]) {

    this->update_d2r_metric(State_Vector);

    return this->s_d2r_Metric;
}


int Kerr_class::get_initial_conditions_from_file(Initial_conditions_type* p_Initial_Conditions, double J_data[], double p_theta_data[], int photon) {

    double M = MASS;
    double a = SPIN;

    double& r_obs = p_Initial_Conditions->init_Pos[e_r];
    double& theta_obs = p_Initial_Conditions->init_Pos[e_theta];

    p_Initial_Conditions->init_Three_Momentum[e_phi] = -J_data[photon] * sin(theta_obs);
    p_Initial_Conditions->init_Three_Momentum[e_theta] = p_theta_data[photon];

    double& J = p_Initial_Conditions->init_Three_Momentum[e_phi];
    double& p_theta = p_Initial_Conditions->init_Three_Momentum[e_theta];

    double delta = pow(r_obs, 2) + pow(a, 2) - 2 * M * r_obs;
    double K = pow(p_theta, 2) + pow(cos(theta_obs), 2) * (pow(J / sin(theta_obs), 2) - pow(a, 2));

    double rad_potential = pow(r_obs * r_obs + a * a - a * J, 2) - delta * (pow(J - a, 2) + K);

    p_Initial_Conditions->init_Three_Momentum[e_r] = sqrt(rad_potential) / delta;

    return OK;
}

int Kerr_class::get_EOM(double State_vector[], double Derivatives[]) {

    double& r = State_vector[e_r];
    double r2 = r * r;

    double& J = State_vector[e_p_phi];

    double sin1 = sin(State_vector[e_theta]);
    double sin2 = sin1 * sin1;

    double cos1 = cos(State_vector[e_theta]);
    double cos2 = cos1 * cos1;

    double rho2 = r2 + SPIN * SPIN * cos2;

    double P = r2 + SPIN * SPIN - SPIN * J;
    double delta = r2 - 2 * MASS * r + SPIN * SPIN;
    double F = P * P - delta * ((J - SPIN) * (J - SPIN) + cos2 * (J * J / sin2 - SPIN * SPIN));

    double& p_r     = State_vector[e_p_r    ];
    double& p_theta = State_vector[e_p_theta];

    *(Derivatives + e_r    ) = delta / rho2 * p_r;
    *(Derivatives + e_theta) = 1.0 / rho2 * p_theta;
    *(Derivatives + e_phi  ) = 1.0 / (delta * rho2) * (P * SPIN + delta * (J / sin2 - SPIN));
    *(Derivatives + e_p_phi) = 0.0;

    double theta_term_1 = -(delta * p_r * p_r + p_theta * p_theta) * SPIN * SPIN * cos1 * sin1 / (rho2 * rho2);
    double theta_term_2 = F * SPIN * SPIN * cos1 * sin1 / (delta * rho2 * rho2) + (J * J * cos1 / (sin2 * sin1) - SPIN * SPIN * cos1 * sin1) / rho2;

    *(Derivatives + e_p_theta) = theta_term_1 + theta_term_2;

    double r_term_1 = p_r * p_r / (rho2) * (MASS - r * (1 - delta / rho2)) + p_theta * p_theta * r / (rho2 * rho2);
    double r_term_2 = (2 * P * r - (r - MASS) * ((J - SPIN) * (J - SPIN) + cos2 * (J * J / (sin2) - SPIN * SPIN))) / (delta * rho2)
                    - F * (rho2 * (r - MASS) + r * delta) / (delta * delta * rho2 * rho2);

    *(Derivatives + e_p_r) = r_term_1 + r_term_2;

    return OK;

}

bool Kerr_class::terminate_integration(double State_vector[], double Derivatives[]) {

    bool scatter = State_vector[e_r] > 30 && Derivatives[e_r] < 0;

    double r_horizon = MASS * (1 + sqrt(1 - SPIN * SPIN));

    bool hit_horizon = State_vector[e_r] - r_horizon < 1e-5;

    return scatter || hit_horizon;

};