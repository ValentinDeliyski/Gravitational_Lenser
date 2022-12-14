#pragma once

#ifndef LENSING

    #define LENSING

    #include "Enumerations.h"
    #include "Spacetimes.h"
    #include "Disk_Models.h"


    class Step_controller {

    public:

        Step_controller(double init_stepsize);

        void update_step();

        double Gain_I;
        double Gain_P;
        double Gain_D;

        double step;

        double current_err;
        double prev_err;
        double sec_prev_err;

        bool continue_integration;

    };

    void get_Radiative_Transfer(double State_Vector[], double Derivatives[], int iteration, double J);

    void RK45(double State_Vector[], double Derivatives[], double J, Step_controller* controller);

    void Lens(Initial_conditions_type* p_Initial_Conditions, std::ofstream data[], std::ofstream momentum_data[]);


#endif 