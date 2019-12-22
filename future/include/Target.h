#pragma once

#include "Nucleus.h"

class Target{

public:
    Target() = default;
    Target(vector<Nucleus> nuc, const double tem);

    void add_nucleus( Nucleus &nuc ){ nuclei.push_back(nuc); };
    size_t get_n_nuclei() const { return nuclei.size(); };
    Nucleus& get_nucleus( size_t i ) { return nuclei.at(i); };

    double get_temperature() const { return temperature; };
    void set_temperature( const double t ){ temperature = t; };

    vector<double> energies(const double e_min, const double e_max, const size_t n_energies_per_state);

    vector<double> cross_section(const vector<double> &energies) const;

private:
    vector<Nucleus> nuclei;

    double temperature; // K
};