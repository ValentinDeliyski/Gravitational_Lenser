import warnings
import numpy as np

from scipy.stats import beta
from scipy.integrate import quad
from scipy.optimize import root_scalar
from scipy.interpolate import CubicSpline, interp1d

import matplotlib.pyplot as plt

from Support_functions.Spacetimes import *

class Analytical_ray_tracer():

    def __init__(self, Spacetime, Granularity, r_source, r_obs, inclination):

        self.Spacetime   = Spacetime
        self.Granularity = Granularity

        self.r_source     = r_source
        self.r_obs        = r_obs
        self.inclincation = inclination

        self.raw_Impact_params          = []
        self.Interpolated_Impact_params = []

        self.raw_Azimuths          = []
        self.Interpolated_Azimuths = []

        self.Impact_Azimuth_spline = []

        self.raw_Image_angular_coords = []
        self.raw_Image_radial_coords  = []

        self.Image_Spline = []

    def Integrand(self, r, impact_param):

        metric = self.Spacetime.metric(r, np.pi / 2)
        V_eff = 1 + metric[0] / metric[2] * impact_param**2 

        if impact_param == 0:
            return 0

        function = -1 / impact_param**2 * metric[3]**2 / metric[0] / metric[1] * V_eff

        if function < 0 or function != function or function == 0:
            return 0

        return 1 / np.sqrt(function)

    def propagate_rays(self):

        warnings.filterwarnings('ignore')

        metric_source       = self.Spacetime.metric(self.r_source, np.pi / 2)
        impact_param_source = np.sqrt(-metric_source[3] / metric_source[0])

        if self.Spacetime.HAS_PHOTON_SPHERE:

            metric_photon_sphere       = self.Spacetime.metric(self.Spacetime.photon_sphere(), np.pi / 2)
            impact_param_photon_sphere = np.sqrt(-metric_photon_sphere[3] / metric_photon_sphere[0])

        print("Computing direct photon paths...")

        #-------- Calculate the impact parameters for photons that do not encounter a radial turning point --------#

        # NOTE: To get a smoother image at the end it is better to distribute the impact paramters according to a beta distribution.
        # A density parameter of 2 for both arguments seems like a good distribution - weighed more at the ends of the interval

        density_parameter  = 5.5
        distribution_range = np.linspace(0, 1, self.Granularity)

        if self.r_source < 0:
            direct_impact_params = (beta.cdf(distribution_range, density_parameter, density_parameter) * impact_param_photon_sphere).tolist()

        else:
            direct_impact_params = (beta.cdf(distribution_range, density_parameter, density_parameter) * impact_param_source).tolist()

        for impact_param in direct_impact_params:

            azimuthal_distance = quad(self.Integrand, 
                                      self.r_source, # Lower integration limit
                                      self.r_obs,    # Upper integration limit
                                      args  = impact_param, 
                                      limit = 100000,
                                      epsabs = 1e-15,
                                      epsrel = 1e-15)[0]

            self.raw_Azimuths.append(azimuthal_distance)

        self.Impact_Azimuth_spline.append(interp1d(direct_impact_params, self.raw_Azimuths))

        print("Finished computing direct photon paths!")

        if self.r_source > 0:

            print("Computing higher order photon paths...")

            density_parameter  = 5.5

            if self.Spacetime.HAS_PHOTON_SPHERE:

                # NOTE: In the presence of a photon sphere, the turning points will be between it and the source

                higher_order_turning_points = self.r_source + (beta.cdf(distribution_range, density_parameter, density_parameter)) * (self.Spacetime.photon_sphere() - self.r_source)
                metrics_at_turning_points   = self.Spacetime.metric(higher_order_turning_points, np.pi / 2)

                #-------- Calculate the impact parameters for the correspoinding turning points --------#

                higher_order_impact_params  = np.sqrt(-metrics_at_turning_points[3] / metrics_at_turning_points[0])

            else:

                def get_turning_point_equation(r_turning, impact_param, Spacetime):
                
                    alpha = 1 / (1 / 2 - Spacetime.PARAMETER)

                    return pow(r_turning, alpha) - 2 / Spacetime.PARAMETER * pow(r_turning, alpha - 1) - pow(impact_param, alpha)

                # NOTE: Currently only applies to the Janis-Newman-Winicour Naked Singularities
                # In the absence of a photon sphere the turning points are located between the singularity and the source

                r_singularity = 2 * self.Spacetime.MASS / self.Spacetime.PARAMETER
                b_source      = self.r_source * pow(1 - r_singularity / self.r_source, 1 / 2 - self.Spacetime.PARAMETER)

                # NOTE: To properly construct the images we need a very uneven distribution of photon impact parameters
                # The majority of the points must be distributed at the ends of the interval - this is neatly done with a beta distribution

                higher_order_impact_params = b_source * (1 - beta.cdf(distribution_range, density_parameter, density_parameter))

                #-------- Calculate the turning points for the correspoinding impact parameters --------#

                higher_order_turning_points = []

                for impact_param in higher_order_impact_params:

                    roots = root_scalar(get_turning_point_equation, 
                                        x0      = self.r_source,
                                        args    = (impact_param, self.Spacetime), 
                                        maxiter = 100000, 
                                        xtol    = 1e-28, 
                                        method  = 'toms748', 
                                        bracket = [r_singularity - 1, self.r_source + 1])

                    if np.absolute(roots.root - r_singularity) > 1e-10:
                        higher_order_turning_points.append(roots.root)

                    else:
                        higher_order_turning_points.append(r_singularity)

            #-------- Solve the integrals that involve turning points --------#

            for impact_param, turning_point in zip(higher_order_impact_params, higher_order_turning_points):

                branch_1_integral = quad(self.Integrand, 
                                         turning_point, # Lower integration limit
                                         r_obs,         # Upper integration limit
                                         args   = impact_param, 
                                         limit  = 10000,
                                         epsabs = 1e-15,
                                         epsrel = 1e-15)[0]

                branch_2_integral = quad(self.Integrand, 
                                         turning_point, # Lower integration limit
                                         self.r_source,      # Upper integration limit
                                         args   = impact_param, 
                                         limit  = 10000,
                                         epsabs = 1e-15,
                                         epsrel = 1e-15)[0]

                self.raw_Azimuths.append(branch_1_integral + branch_2_integral)

            #-------- Create the interpolation function for the indirect portion of the graph --------#

            self.Impact_Azimuth_spline.append(interp1d(np.flip(higher_order_impact_params), np.flip(self.raw_Azimuths[self.Granularity:])))

            #-------- Collect all the impact parameters into one list --------#

            self.raw_Impact_params.extend(direct_impact_params)
            self.raw_Impact_params.extend(higher_order_impact_params)

            print("Finished computing higher order photon paths!")

        else:
            self.Impact_Azimuth_spline.append(None)
            self.raw_Impact_params.extend(direct_impact_params)

    def combine_splines(self):

        #-------ENUMS-------#

        DIRECT   = 0
        INDIRECT = 1
     
        #--------------------#

        max_Impact_param   = max(self.raw_Impact_params)
        distribution_range = np.linspace(0, 1, 100000)
        density_parameter  = 5.5

        Interpolated_direct_Impact_params = beta.cdf(distribution_range, density_parameter, density_parameter) * max_Impact_param * 0.99
        Interpolated_direct_Azimuth       = self.Impact_Azimuth_spline[DIRECT](Interpolated_direct_Impact_params).tolist()

        if self.Impact_Azimuth_spline[INDIRECT] != None:
            Interpolated_indirect_Impact_params = max_Impact_param + (beta.cdf(distribution_range, density_parameter, density_parameter)) * (self.raw_Impact_params[-1] - max_Impact_param)
            Interpolated_indirect_Azimuth       = self.Impact_Azimuth_spline[INDIRECT](Interpolated_indirect_Impact_params)

            self.Interpolated_Azimuths.extend(Interpolated_direct_Azimuth)
            self.Interpolated_Azimuths.extend(Interpolated_indirect_Azimuth)

            self.Interpolated_Impact_params.extend(Interpolated_direct_Impact_params)
            self.Interpolated_Impact_params.extend(Interpolated_indirect_Impact_params)

        else:
            self.Interpolated_Azimuths      = Interpolated_direct_Azimuth
            self.Interpolated_Impact_params = Interpolated_direct_Impact_params

    def construct_images(self):

        np.seterr(all = "ignore")

        Solution_condition = np.arcsin(1 / np.tan(self.Interpolated_Azimuths) / np.tan(self.inclincation))
        
        Image_count = 0
        start_index = 0
        end_index   = 0

        solution_starts = False
        solution_ends   = False

        for index, _ in enumerate(Solution_condition):

            if index != 0:
                solution_starts =     np.isnan(Solution_condition[index - 1]) and not np.isnan(Solution_condition[index])
                solution_ends   = not np.isnan(Solution_condition[index - 1]) and     np.isnan(Solution_condition[index])

                if index == len(Solution_condition) - 1 and not np.isnan(Solution_condition[index]):
                    solution_ends = True

            if solution_starts:
                start_index = index

            if solution_ends:
                end_index = index

                if end_index - start_index > 20 and Image_count <= 20:

                    self.raw_Image_angular_coords.append(Solution_condition[ start_index:end_index ])
                    self.raw_Image_radial_coords.append(self.Interpolated_Impact_params[ start_index:end_index ])

                    theta_sorted, r_sorted = zip(* sorted(zip(self.raw_Image_angular_coords[Image_count], self.raw_Image_radial_coords[Image_count])) )
                    theta_sorted = np.array(theta_sorted) + np.linspace(0, 1e-10, len(theta_sorted))

                    self.Image_Spline.append(CubicSpline(theta_sorted, r_sorted))
                    Image_count = Image_count + 1

        print("Number of images constructed: ", Image_count)

def plot_splined_data(figure_num, Splines):

    color_index = 0
    figure_spline = plt.figure(figure_num)
    subfigure_spline = figure_spline.add_subplot(111)

    for Spline in np.flip(Splines):  

        Angular_coords_spline = np.linspace(-np.pi / 2, np.pi / 2, 200)

        #---------- Plot the Splined results ----------#

        Radial_coords_spline = Spline(Angular_coords_spline)

        x_coords_spline =  Radial_coords_spline * np.cos(Angular_coords_spline)
        y_coords_spline = -Radial_coords_spline * np.sin(Angular_coords_spline)

        x_coords_spline = np.append( np.append(-x_coords_spline, np.flip(x_coords_spline)), -x_coords_spline[0] ) 
        y_coords_spline = np.append( np.append( y_coords_spline, np.flip(y_coords_spline)),  y_coords_spline[0] )

        subfigure_spline.plot(x_coords_spline, y_coords_spline, color = COLOR_CYCLE[color_index])

        color_index = (color_index + 1) % len(COLOR_CYCLE)

    subfigure_spline.set_title(r'Splined Results From The Integration')
    subfigure_spline.set_xlabel(r'x [M]')
    subfigure_spline.set_ylabel(r'y [M]')
    subfigure_spline.set_aspect(1)

def plot_raw_data(figure_num, Radial_coords_raw, Angular_coords_raw):

    color_index = 0
    figure_raw = plt.figure(figure_num)
    subfigure_raw = figure_raw.add_subplot(111)

    try:

        for Image_order, _ in np.ndenumerate(Radial_coords_raw):

            #---------- Plot the raw results ----------#

            x_coords_raw =  Radial_coords_raw[Image_order] * np.cos(Angular_coords_raw[Image_order])
            y_coords_raw = -Radial_coords_raw[Image_order] * np.sin(Angular_coords_raw[Image_order])

            x_coords_raw = np.append( np.append(-x_coords_raw, np.flip(x_coords_raw)), -x_coords_raw[0] ) 
            y_coords_raw = np.append( np.append( y_coords_raw, np.flip(y_coords_raw)),  y_coords_raw[0] )

            subfigure_raw.plot(x_coords_raw, y_coords_raw, color = COLOR_CYCLE[color_index])

            color_index = (color_index + 1) % len(COLOR_CYCLE)
    
    except:

        x_coords_raw =  Radial_coords_raw * np.cos(Angular_coords_raw)
        y_coords_raw = -Radial_coords_raw * np.sin(Angular_coords_raw)

        x_coords_raw = np.append( np.append(-x_coords_raw, np.flip(x_coords_raw)), -x_coords_raw[0] ) 
        y_coords_raw = np.append( np.append( y_coords_raw, np.flip(y_coords_raw)),  y_coords_raw[0] )

        subfigure_raw.plot(x_coords_raw, y_coords_raw, color = COLOR_CYCLE[color_index])

        color_index = (color_index + 1) % len(COLOR_CYCLE)

    subfigure_raw.set_title(r'Direct Results From The Integration')
    subfigure_raw.set_xlabel(r'x [M]')
    subfigure_raw.set_ylabel(r'y [M]')
    subfigure_raw.set_aspect(1)

def plot_angle_impact_param_grapth(figure_num, Splined_Impact_params, Splined_Azimuths, Inclination):

    fig    = plt.figure(figure_num)
    subfig = fig.add_subplot(111)

    branch_split_index   = np.asarray(Splined_Impact_params == np.max(Splined_Impact_params)).nonzero()[0][0]
    figure_maximum_index = np.asarray(Splined_Azimuths      == np.max(Splined_Azimuths)).nonzero()[0][0]

    max_image_order      = np.min( [int(np.max(Splined_Azimuths) / np.pi + 1), 20] )

    print(figure_maximum_index)

    #----------- Plot the line trough the maximum -----------#

    x     = Splined_Impact_params[figure_maximum_index]
    y_max = max_image_order * np.pi
    y_min = 0

    subfig.plot([x, x], [y_min, y_max], linestyle = "--", color = "black")

    #----------- Plot the main curve of the figure -----------#

    subfig.plot(Splined_Impact_params, Splined_Azimuths)

    #----------- Plot the point where the two curve branches meet  -----------#

    subfig.plot(Splined_Impact_params[branch_split_index], Splined_Azimuths[branch_split_index], 'ro', color = "red")

    #----------- Plot the fill-in for different image orders -----------#

    FILL_COLOR_CYCLE = ["bisque", "lightskyblue", "lightsteelblue"]
    color_index      = 0

    image_angles      = np.linspace(-np.pi, np.pi, 1000)
    solution_interval = -np.arccos(np.sin(image_angles) * np.tan(Inclination) / np.sqrt(np.sin(image_angles)**2 * np.tan(Inclination)**2 + 1)) + np.pi

    solution_start = np.min(solution_interval)
    solution_end   = np.max(solution_interval)

    params = {"ytick.color" : "black",
              "xtick.color" : "black",
              "axes.labelcolor" : "black",
              "axes.edgecolor" : "black",
              "text.usetex" : True,
              "font.family" : "serif",
              "font.serif" : ["Computer Modern Serif"]}
    plt.rcParams.update(params)

    for order in range(max_image_order):

        x = [0, 1.1 * np.max(Splined_Impact_params)]

        y_lower = [solution_start + order * np.pi, solution_start + order * np.pi]
        y_upper = [solution_end   + order * np.pi, solution_end   + order * np.pi]

        subfig.plot(x, y_lower, color = "black", linestyle = ":")
        subfig.plot(x, y_upper, color = "black", linestyle = ":")
        subfig.fill_between(x, y_lower, y_upper, color = FILL_COLOR_CYCLE[color_index], alpha = 0.2)
        
        color_index = (color_index + 1) % 3

        subfig.text(x = 0.8 * np.max(Splined_Impact_params),
                    y = (y_upper[0] + y_lower[0]) / 2, 
                    s = '$k = {}$'.format(order),
                    fontstyle = 'italic')

    subfig.set_ylim([0, max_image_order * np.pi])
    subfig.set_xlim([0, 1.1 * np.max(Splined_Impact_params)])

    subfig.set_xlabel(r'$\xi,\,[M]$')
    subfig.set_ylabel(r'$\Delta\phi,\,[rad]$')

    aspect_ratio = 2 * np.max(Splined_Impact_params) / (max_image_order * np.pi)

    subfig.set_aspect(aspect_ratio)

if __name__ == "__main__":

    COLOR_CYCLE = ["blue", "red", "black"]

    #------    Constants      -------#

    DEG_TO_RAD = np.pi / 180

    #------ Metric Parameters -------#

    WH_THROAT = 1    # [ M ]
    WH_ALPHA  = 0.2  # [ - ]
        
    RBH_PARAM = 1    # [ - ]

    JNW_PARAM = 0.48  # [ - ]

    #------      Metrics      -------#

    SCH = Schwarzschild()
    WH  = Wormhole(r_throat = WH_THROAT, parameter = WH_ALPHA)
    RBH = Regular_Black_Hole(parameter = RBH_PARAM)
    JNW = JNW_Naked_Singularity(parameter = JNW_PARAM)

    Spacetime_dict ={"Schwarzshild":       SCH, 
                     "Wormhole":            WH,
                     "Regular Black Hole": RBH,
                     "Naked Singularity":  JNW}

    Active_spacetime = "Schwarzshild"

    #----- Observer / Source  -------#

    r_obs = 1e3                         # [ M ]
    inclination_obs = 80 * DEG_TO_RAD   # [ rad ]

    ray_tracer = Analytical_ray_tracer(SCH, 500, 6, r_obs, inclination_obs)

    ray_tracer.propagate_rays()
    ray_tracer.combine_splines()
    ray_tracer.construct_images()

    if Spacetime_dict[Active_spacetime].HAS_PHOTON_SPHERE:
        plot_splined_data(figure_num = 1, 
                          Splines = ray_tracer.Image_Spline)
        
    else:
        plot_raw_data(figure_num = 2, 
                      Radial_coords_raw  = ray_tracer.raw_Impact_params, 
                      Angular_coords_raw = ray_tracer.raw_Azimuths)

    plot_angle_impact_param_grapth(figure_num = 3,
                                   Splined_Impact_params = ray_tracer.Interpolated_Impact_params, 
                                   Splined_Azimuths      = ray_tracer.Interpolated_Azimuths,
                                   Inclination           = inclination_obs)
    plt.show()
