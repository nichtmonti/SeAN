from __future__ import print_function

import numpy as np
import matplotlib.pyplot as plt
import time

from scipy import integrate

####################################################################
# This is a python module to test the pseudo-convolution of the
# Breit-Wigner cross section and the velocity distribution.
# It compares different methods to execute the integration and
# tries to assess the errors that are made by approximating the
# integration as a true convolution.
####################################################################

### Set number of bins for calculation
NBINS = 100

### Physical constants
HBARC = 197.3269788e6 # eVfm
HBARC2 = HBARC*HBARC
kB = 8.6173303e-5 # eV/K
ATOMICMASSUNIT = 931.494095e6 # eV7c^2

### Set limits for the calculation
EMIN = 3.562480e6
EMAX = 3.563280e6

### Set parameters of the nuclear resonance
Ei = 3.563000e6
Ji = 0.
J0 = 1.
Gamma0 = 8.16
Gamma = 8.16
M = 6.01512
T = 412.5

### Definitions of functions
# Breit-Wigner cross section
def breit_wigner(E, Ei, Ji, J0, Gamma0, Gamma):
    return 0.5*np.pi*HBARC2/(Ei*Ei)*(2.*Ji + 1.)/(2.*J0 + 1.)*Gamma0*Gamma/((E - Ei)*(E - Ei) + 0.25*Gamma*Gamma)
    
# Maxwell-Boltzmann distribution
def maxwell(v, M, T):
    return np.sqrt(M*ATOMICMASSUNIT/(2.*np.pi*kB*T))*np.exp(-(M*ATOMICMASSUNIT*v*v)/(2*kB*T))

# Doppler-shifted resonance energy
def Elab(v, Enucl):
    return np.sqrt(1.-v*v)/(1+v)*Enucl

# Inverse of the doppler-shifted resonance energy and its derivative
def vp(Elab, Enucl):
    r = Elab/Enucl
    return (1. - r*r)/(1. + r*r)
def dvp(Elab, Enucl):
    r = Elab/Enucl
    return (-4.*r/Enucl)/((1. + r*r)*(1. + r*r))

# Function to integrate over histogram with equidistant binning
def int_hist(bins, hist):
    return np.sum(hist)*(bins[1] - bins[0])
    
def hist_mean(bins, hist):
    return np.sum(bins*hist)/np.sum(hist)
    
energy_bins = np.linspace(EMIN, EMAX, NBINS)
v_bins = np.ones(NBINS)*vp(energy_bins, np.ones(NBINS)*Ei)

cross_section = breit_wigner(energy_bins, Ei, Ji, J0, Gamma0, Gamma)
v_dist = maxwell(v_bins, M, T)

v_dist_mean = hist_mean(energy_bins, v_dist)
v_dist_centroid = np.argmin(np.abs(energy_bins - v_dist_mean))
cross_section_mean = hist_mean(energy_bins, cross_section)
cross_section_centroid = np.argmin(np.abs(energy_bins - cross_section_mean))

### 1) The "exact" solution: Numerical integration (num) using an integration algorithm
# Define a new function that is the product of the cross section and the velocity distribution
maxwell_average = lambda v, E: breit_wigner(E, Elab(v, Ei), Ji, J0, Gamma0, Gamma)*maxwell(v, M, T)

start = time.time()
    
doppler_num = np.zeros(NBINS)
doppler_num_err = np.zeros(NBINS)

for i in range(NBINS):
    doppler_num[i], doppler_num_err[i] = integrate.quad(maxwell_average, v_bins[NBINS - 1], v_bins[0], args=(energy_bins[i], ))

stop = time.time()

print("Numerical integration: ", stop - start, " seconds")
int_num = int_hist(energy_bins, doppler_num)
print("Integral: ", int_num)
print()

### 2) The approximation as a convolution integral which can exploit the Fast Fourier Transform (FFT)

start = time.time()

# Substitute v with E in the integral to have an expression that looks like a convolution integral
v_dist_sub = -v_dist*dvp(energy_bins, Ei) # The minus sign comes in because one would have to switch the limits of the integral when the integration variable is substituted.
doppler_con = np.convolve(cross_section, v_dist_sub, 'same')*(energy_bins[1] - energy_bins[0])

# Rearrange the elements in doppler_fft, because the convolution rearranges the elements like
# [0, 1, ...] -> [0 + x, 1 + x, ...]
# ... did not find out yet how x is related to the input, so if the energy bins
# around the maximum of the cross section are not symmetric, this cross section
# will be shifted compared to the others
# doppler_con = np.roll(doppler_con, NBINS - v_dist_centroid)

stop = time.time()

print("Convolution:", stop - start, "seconds")
int_con = int_hist(energy_bins, doppler_con)
print("Integral:", int_con, "(", (int_con - int_num)/int_num*100., "% relative to 'true' result)")
print()

### 3) Again the convolution integral, but broken down to the single steps:
# - pad the histograms to avoid a circular Fourier transform
# - (real) Fourier transformation of both the cross section and the velocity distribution
# - Multiplication of the Fourier transformed lists
# - Back-transformation of the product

start = time.time()
# Substitute v with E in the integral to have an expression that looks like a convolution integral
v_dist_sub = -v_dist*dvp(energy_bins, Ei) # The minus sign comes in because one would have to switch the limits of the integral when the integration variable is substituted.

#cross_section_padded = np.pad(cross_section, (0, 2*NBINS - 1), "constant")*(energy_bins[1] - energy_bins[0])
#v_dist_sub_padded = np.pad(v_dist_sub, (0, 2*NBINS - 1), "constant")
cross_section_padded = np.pad(cross_section, (0, 0), "constant")*(energy_bins[1] - energy_bins[0])
v_dist_sub_padded = np.pad(v_dist_sub, (0, 0), "constant")

#cross_section_fft = np.fft.rfft(cross_section_padded, norm = "ortho")
#v_dist_sub_fft = np.fft.rfft(v_dist_sub_padded, norm = "ortho")
cross_section_fft = np.fft.rfft(cross_section_padded)
v_dist_sub_fft = np.fft.rfft(v_dist_sub_padded)

doppler_fft_fft = cross_section_fft*v_dist_sub_fft

#doppler_fft = np.fft.irfft(doppler_fft_fft, norm = "ortho")
doppler_fft = np.fft.irfft(doppler_fft_fft)

# Rearrange the elements in doppler_fft, because the Fourier transformation rearranges the elements like
# [0, 1, ...] -> [0 + v_dist_centroid, 1 + v_dist_centroid, ...]
doppler_fft = np.roll(doppler_fft, -v_dist_centroid)

stop = time.time()

print("FFT:", stop - start, "seconds")
int_fft = int_hist(energy_bins, doppler_fft)
print("Integral:", int_fft, "(", (int_fft - int_num)/int_num*100., "% relative to 'true' result)")
print()

### 4) Numerical integration using the trapezoidal rule
    
doppler_tra = np.zeros(NBINS)

start = time.time()

for i in range(NBINS):
    for j in range(NBINS - 1):
        doppler_tra[i] += 0.5*(maxwell_average(v_bins[j], energy_bins[i]) + maxwell_average(v_bins[j], energy_bins[i]))*(v_bins[j] - v_bins[j + 1])

stop = time.time()

print("Trapezoidal rule:", stop - start, "seconds")
int_tra = int_hist(energy_bins, doppler_tra)
print("Integral:", int_tra, "(", (int_tra - int_num)/int_num*100., "% relative to 'true' result)")
print()

### Plot the results

plt.figure("Cross section")
plt.subplot(411)
plt.ylabel(r"$\sigma / fm^2$")
cross_section_plot, = plt.plot(energy_bins, cross_section, color = "black", label = r"$\sigma(E)$")
plt.legend(handles=[cross_section_plot])

plt.subplot(412)
plt.ylabel(r"$w(v_\parallel(E))$")
v_dist_plot, = plt.plot(energy_bins, v_dist, color = "black", label = r"$w(v_\parallel(E))$")
plt.legend(handles=[v_dist_plot])

plt.subplot(413)
plt.ylabel(r"$\sigma / fm^2$")
doppler_num_plot, = plt.plot(energy_bins, doppler_num, color = "red", label = "Numerical integration")
# Uncomment to plot the error estimate of the numerical integration
#plt.plot(energy_bins, doppler_num + doppler_num_err, linestyle="--", color = "red")
#plt.plot(energy_bins, doppler_num - doppler_num_err, linestyle="--", color = "red")
doppler_con_plot, = plt.plot(energy_bins, doppler_con, color = "chartreuse", label = "Convolution")
doppler_fft_plot, = plt.plot(energy_bins, doppler_fft, color = "royalblue", label = "FFT")
doppler_tra_plot, = plt.plot(energy_bins, doppler_tra, color = "grey", label = "Trapezoidal rule")
plt.legend(handles=[doppler_num_plot, doppler_con_plot, doppler_fft_plot, doppler_tra_plot])

plt.subplot(414)
plt.xlabel("Energy / eV")
plt.ylabel("rel. deviation")
doppler_num_con_plot, = plt.plot(energy_bins, (doppler_con - doppler_num)/doppler_num, color = "chartreuse", label = "num vs. con")
doppler_num_fft_plot, = plt.plot(energy_bins, (doppler_fft - doppler_num)/doppler_num, color = "royalblue", label = "num vs. fft")
doppler_num_tra_plot, = plt.plot(energy_bins, (doppler_tra - doppler_num)/doppler_num, color = "grey", label = "num vs. tra")
plt.legend(handles=[doppler_num_con_plot, doppler_num_fft_plot, doppler_num_tra_plot])