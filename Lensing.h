#pragma once

Return_Value_enums Lens(double initial_conditions[], double M, double metric_parameter, double a, double r_throat, double r_in, double r_out, bool lens_from_file,
                        std::ofstream data[], std::ofstream momentum_data[], Spacetimes e_metric, c_Kerr Kerr_class, c_RBH RBH_class, c_Wormhole Wormhole_class,
                        Disk_Models Disk_Model, double disk_alpha, double disk_height_scale, double disk_rad_cutoff, double disk_omega){

    double r_obs     = initial_conditions[e_r];
    double theta_obs = initial_conditions[e_theta];
    double phi_obs   = initial_conditions[e_phi];
    double J         = initial_conditions[3];
    double p_theta_0 = initial_conditions[e_p_theta];
    double p_r_0     = initial_conditions[e_p_r];

    // Initialize initial State Vector
    double State_vector[6] = { r_obs, theta_obs, phi_obs, 0 , p_theta_0, p_r_0 };   

    // Initialize a vector that stores the old state and the test state, used for estimating errors
    double State_vector_test[6]{}, Old_state[6]{};   
    
    // Storing the number of elements in the state vector for readability in later code
    int const Vector_size = sizeof(State_vector) / sizeof(double);  

    // Initialize arrays that store the states and derivatives of the intermidiate integration steps
    double inter_State_vector[RK45_size * Vector_size]{}, Derivatives[RK45_size * Vector_size]{}; 

    // Initialize arrays that store the Flux, Intensity, Redshift from the disk and the image coordinates for each light ray
    double Flux_Novikov_Thorne{}, Intensity_Toroidal_Disk{}, redshift{}, Image_coordiantes[3]{};

    // Set the old State Vector and the Test State Vector to the Initial State Vector
    for (int vector_indexer = e_r; vector_indexer <= e_p_r; vector_indexer += 1) {

        State_vector_test[vector_indexer] = State_vector[vector_indexer];
        Old_state[vector_indexer]         = State_vector[vector_indexer];

    }

    // Initialize two 3D position vectors, used for computing the photon tangent at the observer
    double r2[3]{}, r1[3]{
                          State_vector[e_r] * cos(State_vector[e_phi]) * sin(State_vector[e_theta]),
                          State_vector[e_r] * sin(State_vector[e_phi]) * sin(State_vector[e_theta]),
                          State_vector[e_r] * cos(State_vector[e_theta])
                         };

    // Initialize counters for the Number Of Integration Steps, the Image Order and the Number Of Equator Crossings
    int integration_count{}, Image_Order_Novikov_Thorne{}, Image_Order_Toroidal{}, n_equator_crossings{};

    // Initialize the Initial Step Size and Affine Parameter
    double step = INIT_STEPSIZE;
    double affine_parameter{};

    // Initialize the logical flags and error enums
    bool continue_integration = false;
    bool found_disc[ORDER_NUM]{};

    Return_Value_enums RK45_Status = OK, Disc_model_status = OK;

    while (RK45_Status == OK && integration_count < MAX_INTEGRATION_COUNT) {

        RK45_Status = RK45_EOM(State_vector, Derivatives, &step, r_throat, a, metric_parameter, M, J, 
                               Kerr_class, e_metric, RBH_class, Wormhole_class, &continue_integration, Disk_Model,
                               disk_alpha, disk_height_scale, disk_rad_cutoff, disk_omega, &Intensity_Toroidal_Disk, r_obs, theta_obs);

        // If error estimate, returned from RK45_EOM < RK45_ACCURACY
        if (continue_integration == true) {

            // Initialize the light ray
            if (integration_count == 1) {

                for (int index = 0; index <= ORDER_NUM - 1; index++) {

                    found_disc[index] = false;

                }

                redshift = 0;
                Flux_Novikov_Thorne     = 0;
                Intensity_Toroidal_Disk = 0;

                n_equator_crossings = 0;

                Image_Order_Novikov_Thorne = direct;
                Image_Order_Toroidal       = direct;

                r2[x] = State_vector[e_r] * cos(State_vector[e_phi]) * sin(State_vector[e_theta]);
                r2[y] = State_vector[e_r] * sin(State_vector[e_phi]) * sin(State_vector[e_theta]);
                r2[z] = State_vector[e_r] * cos(State_vector[e_theta]);

                double photon_tangent[3]         = { r1[x] - r2[x], r1[y] - r2[y], r1[z] - r2[z] };
                double photon_LOS_parameter      = -dot_product(r1, r1) / dot_product(r1, photon_tangent);
                double obs_plane_intersection[3] = { r1[x] + photon_LOS_parameter * photon_tangent[x],
                                                     r1[y] + photon_LOS_parameter * photon_tangent[y],
                                                     r1[z] + photon_LOS_parameter * photon_tangent[z] };

                Rorate_to_obs_plane(theta_obs, phi_obs, obs_plane_intersection, Image_coordiantes);

            }
                                    
            if (Disk_event(Novikov_Thorne, State_vector, Old_state, r_in, r_out, disk_alpha, disk_height_scale, disk_rad_cutoff, disk_omega) == Inside_Disk
                && Disk_Model == Novikov_Thorne) {

               Image_Order_Novikov_Thorne = n_equator_crossings;

               redshift = Redshift(e_metric, Disk_Model, J, State_vector,
                                   r_obs, theta_obs, Kerr_class, RBH_class, Wormhole_class);

               Flux_Novikov_Thorne = get_flux(e_metric, State_vector[e_r], r_in, State_vector[e_theta],
                                              Kerr_class, RBH_class, Wormhole_class);

               write_to_file(Image_coordiantes, redshift, Flux_Novikov_Thorne, State_vector, metric_parameter, J,
                             Image_Order_Novikov_Thorne, lens_from_file, data, momentum_data);

               found_disc[Image_Order_Novikov_Thorne] = true;

            }

            if (crossed_equatior(State_vector, Old_state)) {

                n_equator_crossings += 1;

            }

            for (int vector_indexer = 0; vector_indexer <= Vector_size - 1; vector_indexer += 1) {

                Old_state[vector_indexer] = State_vector[vector_indexer];

            }

            // Evaluate logical flags for terminating the integration

            bool scatter            = State_vector[e_r] > r_out && Derivatives[e_r] < 0;
            bool scatter_other_side = State_vector[e_r] < -sqrt(r_out * r_out + r_throat * r_throat);

            bool hit_horizon_kerr = State_vector[e_r] - Kerr_class.get_r_horizon() < 0.05;
            bool hit_horizon_RBH  = State_vector[e_r] - RBH_class.get_r_horizon() < 0.05;

            bool pass_trough_throat = State_vector[e_r] < 0;

            bool terminate_integration = scatter;

            switch (e_metric) {

            case Wormhole:

                scatter = State_vector[e_r] > sqrt(r_out * r_out + r_throat * r_throat) && Derivatives[0] < 0;

                terminate_integration = scatter || scatter_other_side;

                break;

            case Kerr:
                
                terminate_integration = scatter || hit_horizon_kerr;

                break;

            case Reg_Black_Hole:

                terminate_integration = scatter || hit_horizon_RBH;

                break;

            default:

                std::cout << "Wrong metric!" << '\n';

                return ERROR;

            }

            if (terminate_integration) {

                switch (Disk_Model) {
            
                    case Novikov_Thorne:

                        for (int Image_Order_Scan = direct; Image_Order_Scan <= direct; Image_Order_Scan += 1) {

                            if (found_disc[Image_Order_Scan] == false && lens_from_file == false) {

                                write_to_file(Image_coordiantes, 0., 0., State_vector, metric_parameter, J,
                                              Image_Order_Scan, lens_from_file, data, momentum_data);

                            }
                        }

                        integration_count = 0;

                        break;

                    case Optically_Thin_Toroidal:

                        write_to_file(Image_coordiantes, 0., Intensity_Toroidal_Disk, State_vector, metric_parameter, J,
                                      direct, lens_from_file, data, momentum_data);

                        integration_count = 0;

                        break;

                }

                break;

            }

            integration_count += 1;

        }

    }

    return RK45_Status;

}
