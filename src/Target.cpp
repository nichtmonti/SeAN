#include "Target.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

#include "TCanvas.h"
#include "TLegend.h"

using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::stringstream;
using std::regex;
using std::scientific;


//Target::Target(unsigned int ne, unsigned int nz, string name, unsigned int number) : z_bins(nz, 0.), incident_beam_bins(ne, 0.), dopplercs_bins(ne, 0.), massattenuation_bins(ne, 0.), transmitted_beam_bins(ne, 0.){
//	target_name = name;
//	target_number = number;
//
//	crossSection = new CrossSection();
//	absorption = new Absorption();
//
//	photon_flux_density_bins.reserve(ne*nz);
//	resonance_absorption_density_bins.reserve(ne*nz);
//
//	for(unsigned int i = 0; i < nz; ++i){
//		photon_flux_density_bins.push_back(vector<double>(ne, 0.));
//		resonance_absorption_density_bins.push_back(vector<double>(ne, 0.));
//	}
//};

Target::~Target(){
	delete crossSection;
	delete absorption;

	delete &crosssection_at_rest_histogram;
	delete &velocity_distribution_histogram;
	delete &velocity_distribution_histogram;
	delete &z_bins;
	delete &energy_boosted;
	delete &vdist_norm;
	delete &vdist_centroid;

	delete &photon_flux_density_histogram;
	delete &resonance_absorption_density_histogram;

	delete &incident_beam_histogram;
	delete &crosssection_histogram;
	delete &massattenuation_histogram;
	delete &transmitted_beam_histogram;
};

void Target::initialize(vector<double> &energy_bins){
	unsigned int nenergies = (unsigned int) settings.energy[target_number].size();

	// Reserve space for the histograms
	for(unsigned int i = 0; i < nenergies; ++i){
		crosssection_at_rest_histogram.push_back(vector<double>(settings.nbins_e, 0.));
		velocity_distribution_bins.push_back(vector<double>(settings.nbins_e, 0.));
		velocity_distribution_histogram.push_back(vector<double>(settings.nbins_e, 0.));
	}

	crosssection_histogram.reserve(settings.nbins_e);
	z_bins.reserve(settings.nbins_z);

	photon_flux_density_histogram.reserve(settings.nbins_e*settings.nbins_z);
	resonance_absorption_density_histogram.reserve(settings.nbins_e*settings.nbins_z);

	energy_boosted.reserve(nenergies);

	// Initialize input reader and (if necessary) read mass attenuation
	inputReader = new InputReader();

	// Initialize calculators
	crossSection = new CrossSection(settings);
	absorption = new Absorption();

	// Initialize plotter
	plotter = new Plotter();

	// Shift resonance energies due to target velocity
	boostEnergies();
	// Calculate z bins
	calculateZBins();
	// Calculate cross section
	calculateCrossSectionAtRest(energy_bins);
	// Calculate velocity distribution
	calculateVelocityDistribution(energy_bins);

}

void Target::calculateCrossSection(vector<double> &energy_bins){

	if(settings.exact){
		;
	} else{
		crossSection->fft_input(energy_bins, crosssection_at_rest_histogram, velocity_distribution_histogram, energy_boosted);
		crossSection->dopplershiftFFT(energy_bins, crosssection_histogram, crosssection_at_rest_histogram, velocity_distribution_bins, velocity_distribution_histogram, vdist_norm, vdist_centroid);
	}
}

void Target::boostEnergies(){
	double beta = settings.velocity[target_number]/SPEEDOFLIGHT;
	
	unsigned int nenergies = (unsigned int) settings.energy[target_number].size();

	for(unsigned int i = 0; i < nenergies; ++i)
		energy_boosted.push_back((1. + beta)/sqrt(1. - beta*beta)*settings.energy[target_number][i]);
}

void Target::calculateCrossSectionAtRest(vector<double> &energy_bins){

	for(unsigned int i = 0; i < settings.energy.size(); ++i){
		crosssection_at_rest_histogram.push_back(vector <double>(settings.nbins_e, 0.));
	}

	crossSection->breit_wigner(energy_bins, crosssection_at_rest_histogram, energy_boosted, target_number);
}

void Target::calculateVelocityDistribution(vector<double> &energy_bins){

	crossSection->calculateVelocityBins(energy_bins, velocity_distribution_bins, energy_boosted, target_number);

	switch(settings.vDist[target_number]){
		case vDistModel::zero:
			for(unsigned int i = 0; i < settings.nbins_e; ++i){
				for(unsigned int j = 0; j < crosssection_at_rest_histogram.size(); ++j){
					crosssection_histogram[i] = crosssection_at_rest_histogram[i][j];
				}
			}
			break;

		case vDistModel::mb:
		case vDistModel::mba:
			crossSection->maxwell_boltzmann(energy_bins, velocity_distribution_bins, velocity_distribution_histogram, energy_boosted, target_number);
			break;

		case vDistModel::arb:
			break;
	}

	vDistInfo();
}

void Target::plot(vector<double> &energy_bins){
	
	plotter->plot1DHistogram(energy_bins, crosssection_histogram, "crosssection");
}

//void Target::plotCrossSection(vector<double> &energy_bins){
//
//	stringstream filename;
//	filename << PLOT_OUTPUT_DIR << target_name << "_cross_section.pdf";
//	stringstream canvasname;
//	canvasname << target_name << "_canvas";
//	TCanvas *canvas = new TCanvas(canvasname.str().c_str(), target_name.c_str(), 0, 0, 800, 500);
//	TLegend *legend = new TLegend(CS_PLOT_LEGEND_X1, CS_PLOT_LEGEND_Y1, CS_PLOT_LEGEND_X2, CS_PLOT_LEGEND_Y2);
//
//	crossSection->plot_crosssection(energy_bins, crosssection_bins, target_name, canvas, legend, "Cross section");
//
//	legend->Draw();
//	canvas->SaveAs(filename.str().c_str());
//	delete canvas;
//}


//void Target::calculateIncidentBeam(vector<double> &energy_bins, string beam_ID, vector<double> beamParams){
//	if(beam_ID == "const")
//		absorption->const_beam(energy_bins, incident_beam_bins, beamParams);
//
//	if(beam_ID == "gauss")
//		absorption->gauss_beam(energy_bins, incident_beam_bins, beamParams);
//}
//
void Target::calculateZBins(){
	
	double delta_z = settings.thickness[target_number]/settings.nbins_z;

	for(unsigned int i = 0; i < settings.nbins_z; ++i){
		z_bins[i] = i*delta_z;
	}
}
//
//void Target::calculateZBins(double z0, double z1){
//	
//	double delta_z = (z1-z0)/NBINS_Z;
//
//	for(unsigned int i = 0; i < NBINS_Z; ++i){
//		z_bins[i] = i*delta_z + z0;
//	}
//}
//
//void Target::calculateDopplerShift(vector<double> &energy_bins){
//	if(vDist_ID != "maxwell_boltzmann_approximation"){
//		crossSection->integration_input(crosssection_bins, vdist_bins);
//		crossSection->dopplershift(dopplercs_bins, energy_bins, crosssection_bins, velocity_bins, vdist_bins, vdist_norm, e0_list);
//	}
//}
//
//void Target::calculateDopplerShiftFFT(vector<double> &energy_bins){
//	if(vDist_ID != "maxwell_boltzmann_approximation"){
//		crossSection->fft_input(energy_bins, crosssection_bins, vdist_bins, e0_list);
//		crossSection->dopplershiftFFT(dopplercs_bins, energy_bins, crosssection_bins, velocity_bins, vdist_bins, vdist_norm, vdist_centroid);
//	}
//}
//
//void Target::calculateMassAttenuation(vector<double> &energy_bins){
//	if(massAttenuation_ID == "0"){
//	;} else{
//		//absorption->read_massattenuation_NIST(energy_bins, massattenuation_bins, massAttenuation_ID, mass);
//	}
//}
//
//void Target::setIncidentBeam(double &trans_beam_bins){
//	for(unsigned int i = 0; i < settings.nbins_e; ++i)
//		incident_beam_bins[i] = (&trans_beam_bins)[i];
//}
//
//void Target::calculatePhotonFluxDensity(){
//	absorption->photon_flux_density(dopplercs_bins, massattenuation_bins, z_bins, incident_beam_bins, photon_flux_density_bins);
//}
//
//void Target::calculateTransmittedBeam(){
//	for(unsigned int i = 0; i < settings.nbins_e; ++i)
//		transmitted_beam_bins[i] = photon_flux_density_bins[i][NBINS_Z - 1];
//}
//
//void Target::calculateResonanceAbsorptionDensity(){
//	absorption->resonance_absorption_density(dopplercs_bins, photon_flux_density_bins, resonance_absorption_density_bins);
//}
//
//double Target::integrateEZHistogram(vector<double> &energy_bins, vector<double> &z_bins, vector<vector<double> > &ezhist){
//
//	// Area of a bin in 2D plane
//	double bin_area = (energy_bins[1] - energy_bins[0])*(z_bins[1] - z_bins[0]);
//
//	// Crude implementation of the Riemann integral as a sum of bin contents times their dimension in energy- and z-direction. Since there are only settings.nbins_e-1 spaces between settings.nbins_e bins, leave out the last bin in each loop.
//	double integral = 0.;
//
//	#pragma omp parallel for reduction (+:integral)
//	for(unsigned int i = 0; i < settings.nbins_e - 1; ++i){
//		for(unsigned int j = 0; j < NBINS_Z - 1; ++j){
//			integral += ezhist[i][j]; 
//		}
//	}
//
//	// Implementation that starts at the 1st and not the 0th bin to check the validity of this approximation
////	double integral = 0.;
////	for(int i = 1; i < settings.nbins_e; ++i){
////		for(int j = 1; j < NBINS_Z; ++j){
////			integral += bin_area*ezhist[i][j]; 
////		}
////	}
//
//	return bin_area*integral;
//}
//
//double Target::integrateEEHistogram(vector<double> &energy_bins, vector<vector<double> > &eehist){
//
//	// Area of a bin in 2D plane
//	double bin_area = (energy_bins[1] - energy_bins[0])*(energy_bins[1] - energy_bins[0]);
//
//	// Crude implementation of the Riemann integral as a sum of bin contents times their dimension in energy- and z-direction. Since there are only settings.nbins_e-1 spaces between settings.nbins_e bins, leave out the last bin in each loop.
//	double integral = 0.;
//
//	for(unsigned int i = 0; i < settings.nbins_e - 1; ++i){
//		for(unsigned int j = 0; j < settings.nbins_e - 1; ++j){
//			integral += bin_area*eehist[i][j]; 
//		}
//	}
//
//	return integral;
//}
//
//void Target::calculateAbsorption(vector<double> &energy_bins){
//
//	double absorption = integrateEZHistogram(energy_bins, z_bins, resonance_absorption_density_bins);
//
//	cout << "TARGET #" << target_number << ": '" << target_name << "'" << endl;
//	cout << "INT ALPHA dE dZ = " << absorption << endl;
//}
//
//void Target::plotVelocityDistribution(){
//
//	stringstream filename;
//	filename << PLOT_OUTPUT_DIR << target_name << "_velocity_distribution.pdf";
//	stringstream canvasname;
//	canvasname << target_name << "_canvas";
//	TCanvas *canvas = new TCanvas(canvasname.str().c_str(), target_name.c_str(), 0, 0, 800, 500);
//	TLegend *legend = new TLegend(V_PLOT_LEGEND_X1, V_PLOT_LEGEND_Y1, V_PLOT_LEGEND_X2, V_PLOT_LEGEND_Y2);
//
//	// It is enough to plot the velocity distribution in the binning of one of the excited states, because it is the same for all states of a target.
//	crossSection->plot_vdist(velocity_bins[0], vdist_bins[0], target_name, canvas, legend, "Velocity Distribution");
//
//	legend->Draw();
//	canvas->SaveAs(filename.str().c_str());
//	delete canvas;
//}
//
//void Target::plotDopplerShift(vector<double> &energy_bins){
//
//	stringstream filename;
//	filename << PLOT_OUTPUT_DIR << target_name << "_doppler_shift.pdf";
//	stringstream canvasname;
//	canvasname << target_name << "_canvas";
//	TCanvas *canvas = new TCanvas(canvasname.str().c_str(), target_name.c_str(), 0, 0, 800, 500);
//	TLegend *legend = new TLegend(CS_PLOT_LEGEND_X1, CS_PLOT_LEGEND_Y1, CS_PLOT_LEGEND_X2, CS_PLOT_LEGEND_Y2);
//
//	crossSection->plot_dopplershift(energy_bins, crosssection_bins, dopplercs_bins, target_name, canvas, legend, "Doppler-shifted cross section");
//
//	legend->Draw();
//	canvas->SaveAs(filename.str().c_str());
//	delete canvas;
//	
//}
//
//void Target::plotMassAttenuation(vector<double> &energy_bins){
//
//	stringstream filename;
//	filename << PLOT_OUTPUT_DIR << target_name << "_mass_attenuation.pdf";
//	stringstream canvasname;
//	canvasname << target_name << "_canvas";
//	TCanvas *canvas = new TCanvas(canvasname.str().c_str(), target_name.c_str(), 0, 0, 800, 500);
//	TLegend *legend = new TLegend(MU_PLOT_LEGEND_X1, MU_PLOT_LEGEND_Y1, MU_PLOT_LEGEND_X2, MU_PLOT_LEGEND_Y2);
//
//	absorption->plot_massattenuation(energy_bins, massattenuation_bins, target_name, canvas, legend, "Mass attenuation coefficient");
//
//	legend->Draw();
//	canvas->SaveAs(filename.str().c_str());
//	delete canvas;
//	
//}
//
//void Target::plotMu(){
//
//	stringstream filename;
//	filename << PLOT_OUTPUT_DIR << target_name << "_mu.pdf";
//	stringstream canvasname;
//	canvasname << target_name << "_canvas";
//	TCanvas *canvas = new TCanvas(canvasname.str().c_str(), target_name.c_str(), 0, 0, 800, 500);
//	TLegend *legend = new TLegend(MU_PLOT_LEGEND_X1, MU_PLOT_LEGEND_Y1, MU_PLOT_LEGEND_X2, MU_PLOT_LEGEND_Y2);
//
//	canvas->SetLogx();
//	canvas->SetLogy();
//
//	absorption->plot_total_massattenuation(target_name, canvas, legend, "Mass attenuation #mu");
//
//	legend->Draw();
//	canvas->SaveAs(filename.str().c_str());
//	delete canvas;
//	
//}
//
//void Target::plotPhotonFluxDensity(vector<double> &energy_bins){
//
//	stringstream filename;
//	filename << PLOT_OUTPUT_DIR << target_name << "_phi.pdf";
//	stringstream canvasname;
//	canvasname << target_name << "_canvas";
//	TCanvas *canvas = new TCanvas(canvasname.str().c_str(), target_name.c_str(), 0, 0, 800, 500);
//	TLegend *legend = new TLegend(MU_PLOT_LEGEND_X1, MU_PLOT_LEGEND_Y1, MU_PLOT_LEGEND_X2, MU_PLOT_LEGEND_Y2);
//
//	absorption->plot_photon_flux_density(energy_bins, z_bins, photon_flux_density_bins, target_name, canvas, legend, "Photon flux density #Phi");
//
//	legend->Draw();
//	canvas->SaveAs(filename.str().c_str());
//	delete canvas;
//}
//
//void Target::plotResonanceAbsorptionDensity(vector<double> &energy_bins){
//
//	stringstream filename;
//	filename << PLOT_OUTPUT_DIR << target_name << "_alpha.pdf";
//	stringstream canvasname;
//	canvasname << target_name << "_canvas";
//	TCanvas *canvas = new TCanvas(canvasname.str().c_str(), target_name.c_str(), 0, 0, 800, 500);
//	TLegend *legend = new TLegend(MU_PLOT_LEGEND_X1, MU_PLOT_LEGEND_Y1, MU_PLOT_LEGEND_X2, MU_PLOT_LEGEND_Y2);
//
//	absorption->plot_resonance_absorption_density(energy_bins, z_bins, resonance_absorption_density_bins, target_name, canvas, legend, "Resonance absorption density #alpha");
//
//	legend->Draw();
//	canvas->SaveAs(filename.str().c_str());
//	delete canvas;
//	
//}
//
//void Target::print(){
//	cout << "TARGET #" << target_number << ": '" << target_name << "'" << endl;
//	cout << "GROUND STATE J = " << j0 << endl;
//	cout << "RESONANCES:" << endl;
//	cout << "E0 AT REST/eV\tE0 BOOSTED/eV\tGAMMA0\tGAMMA\tJ" << endl;
//	
//	for(unsigned int i = 0; i < e0_list.size(); ++i)
//		cout << e0_at_rest_list[i] << "\t" << e0_list[i] << "\t" << gamma0_list[i] << "\t" << gamma_list[i] << "\t" << jj_list[i] << endl;
//
//	cout << "VELOCITY DISTRIBUTION = " << vDist_ID;
//
//	if( vDistParams.size() ){
//		cout << " ( ";
//		for(unsigned int i = 0; i < vDistParams.size(); ++i)
//			cout << vDistParams[i] << " ";
//
//		cout << ")" << endl;
//	} else{
//		cout << endl;
//	}
//
//	cout << "MASS = " << mass << " u" << endl;
//	cout << "MASS ATTENUATION = " << massAttenuation_ID << endl;
//	cout << "TARGET THICKNESS = " << z << " atoms / fm^2" << endl;
//	cout << "TARGET VELOCITY = " << vz << " m/s" << endl;
//}
//
void Target::write(vector<double> &energy_bins){
	
	stringstream filename;
	filename << settings.targetNames[target_number] << "_energy_bins";
	writer->write1DHistogram(energy_bins, filename.str(), "Energy / eV");

//	print1DVector(energy_bins,  "Energy / eV", TXT_OUTPUT_DIR + target_name + "_energy_bins.txt");
//	print1DVector(incident_beam_bins, "Incident beam / a.u.", TXT_OUTPUT_DIR + target_name + "_incident_beam.txt");
//	print1DVector(dopplercs_bins, "Doppler-broadened cross section / eV fm^2", TXT_OUTPUT_DIR + target_name + "_doppler_shift.txt");
//	print1DVector(massattenuation_bins, "Mass attenuation / fm^2 / atom", TXT_OUTPUT_DIR + target_name + "_mass_attenuation.txt");
//	print1DVector(z_bins, "z / atoms / fm^2", TXT_OUTPUT_DIR + target_name + "_z_bins.txt");
//	print1DVector(transmitted_beam_bins, "Transmitted beam / a.u.", TXT_OUTPUT_DIR + target_name + "_transmitted_beam.txt");
//
//	print2DVector(photon_flux_density_bins, "Photon flux density / a.u.", TXT_OUTPUT_DIR + target_name + "_phi.txt");
//	print2DVector(resonance_absorption_density_bins, "Resonance absorption density / a.u.", TXT_OUTPUT_DIR + target_name + "_alpha.txt");
//
//	print2DVector(crosssection_bins, "Cross section / eV fm^2", TXT_OUTPUT_DIR + target_name + "_cross_section.txt");
//	print2DVector(velocity_bins, "Velocity / c", TXT_OUTPUT_DIR + target_name + "_velocity_bins.txt");
//	print2DVector(vdist_bins, "Velocity distribution", TXT_OUTPUT_DIR + target_name + "_velocity_distribution.txt");
}
//
//void Target::print1DVector(vector<double> &vec, string column, string filename){
//	
//	ofstream ofile(filename);
//
//        if(!ofile.is_open()){
//                cout << "Error: Target.cpp: print1DVector(): File '" << filename << "' could not be opened." << endl;
//		abort();
//	}
//        cout << "> Writing output file '" << filename << "'" << endl;
//
//	ofile.precision(8);
//	ofile << COMMENT << " " << column << endl;
//	for(unsigned int i = 0; i < vec.size(); ++i){
//		ofile << scientific << vec[i] << endl;
//	}
//}
//
//void Target::print2DVector(vector< vector<double> > (&vec), string column, string filename){
//	
//	ofstream ofile(filename);
//
//        if(!ofile.is_open()){
//                cout << "Error: Target.cpp: print2DVector(): File '" << filename << "' could not be opened." << endl;
//		abort();
//	}
//        cout << "> Writing output file '" << filename << "'" << endl;
//
//	ofile.precision(8);
//	ofile << COMMENT << " "  << column << endl;
//	ofile << COMMENT << " f(E0, z0)" << endl;
//	ofile << COMMENT << " f(E0, z1)" << endl;
//	ofile << COMMENT << " ... " << endl;
//	ofile << COMMENT << " f(E1, z0)" << endl;
//	ofile << COMMENT << " f(E1, z1)" << endl;
//	ofile << COMMENT << " ... " << endl;
//	for(unsigned int i = 0; i < vec.size(); ++i){
//		for(unsigned int j = 0; j < vec[i].size(); ++j){
//			ofile << scientific << vec[i][j] << endl;
//		}
//	}
//}
//
void Target::vDistInfo(){
	
//	 Determine the integral of the velocity distribution and the centroid of the distribution.
//	 The integral is needed to normalize the pseudo-convolution with the cross section, since this should conserve the area under the resonance curve.
//	 The centroid is needed for the FFT convolution. The algorithm which is used re-orders the input array by the shift of the centroid of the velocity distribution from the array centroid.
	double norm = 0.;
	double sum = 0.;
	double weightedsum = 0.;
	double velocity_bin_low = 0.;
	double velocity_bin_high = 0.;
	double vdist = 0.;

	for(unsigned int i = 0; i < velocity_distribution_bins.size(); ++i){

		sum = 0.;
		norm = 0.;
		weightedsum = 0.;

		for(unsigned int j = 0; j < settings.nbins_e - 1; ++j){
			vdist = velocity_distribution_histogram[i][j];
			velocity_bin_low = velocity_distribution_bins[i][j];
			velocity_bin_high = velocity_distribution_bins[i][j + 1];
			norm += vdist*(velocity_bin_high - velocity_bin_low);
			weightedsum += vdist*j;
			sum += vdist;
		}

		vdist_norm.push_back(fabs(norm));
		vdist_centroid.push_back((unsigned int) (weightedsum/sum));
	}
}
