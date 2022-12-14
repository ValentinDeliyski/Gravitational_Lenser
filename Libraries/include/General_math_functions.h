#pragma once

#ifndef GENERAL_MATH_FUNCTIONS

	#define GENERAL_MATH_FUNCTIONS

	int mat_vec_multiply_4D(double Matrix[4][4], double Vector[4], double result[4]);

	double vector_norm(double Vector[], int Vector_size);

	int Rorate_to_obs_plane(double theta_obs, double phi_obs, double Image_point[3], double rotated_Image_point[3]);

	double my_max(double vector[]);

	bool crossed_equatior(double State_vector[], double Old_State_Vector[]);

	bool interpolate_crossing(double State_Vector[], double Old_State_Vector[], double Crossing_coords[]);

	double dot_product(double vector_1[3], double vector_2[3]);

#endif