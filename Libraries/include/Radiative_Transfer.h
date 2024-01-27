#pragma once
#pragma once

#ifndef RADIATIVE_TRANSFER

#define RADIATIVE_TRANSFER

#include"Enumerations.h"

void Implicit_Trapezoid_Radiative_Transfer(double const emission_functions[STOKES_PARAM_NUM],
                                           double const absorbtion_functions[STOKES_PARAM_NUM],
                                           double const faradey_functions[STOKES_PARAM_NUM],
                                           double const step,
                                           double Intensity[STOKES_PARAM_NUM]);

void Analytic_Radiative_Transfer(double const emission_functions[STOKES_PARAM_NUM],
                                 double const absorbtion_functions[STOKES_PARAM_NUM],
                                 double const faradey_functions[STOKES_PARAM_NUM],
                                 double const step,
                                 double Intensity[STOKES_PARAM_NUM]);

#endif