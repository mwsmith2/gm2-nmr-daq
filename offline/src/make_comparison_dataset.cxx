/*===========================================================================*\

author: Matthias W. Smith
email:  mwsmith2@uw.edu
date:   2015/09/04

about:  The program runs on a crunched file and packages all the data
        with units converted and traces dropped.  A dataset that has
        gone through a sieve.

usage:

./combine_datasets <output-file> <ttree-name> [input-files ...]

\*===========================================================================*/


//--- std includes ----------------------------------------------------------//
#include <iostream>
#include <cmath>

//--- other includes --------------------------------------------------------//
#include <boost/filesystem.hpp>
#include "TFile.h"
#include "TTree.h"

#include "shim_structs.hh"

using namespace std;
using namespace gm2;

int main(int argc, char *argv[])
{
  // std variables.
  vector<string> datafiles;
  string outfile;

  // Structs to connect to trees for comparison tree.
  field_t field_i;
  hamar_t laser_i;
  capacitec_t ctec_i;
  metrolab_t mlab_i;
  scs2000_t envi_i;
  tilt_sensor_t tilt_i;
  sync_flags_t flags_i;

  field_t field_f;
  hamar_t laser_f;
  capacitec_t ctec_f;
  metrolab_t mlab_f;
  scs2000_t envi_f;
  tilt_sensor_t tilt_f;
  sync_flags_t flags_f;

  field_t field_d;
  hamar_t laser_d;
  capacitec_t ctec_d;
  metrolab_t mlab_d;
  scs2000_t envi_d;
  tilt_sensor_t tilt_d;
  sync_flags_t flags_d;

  // And the ROOT variables.
  vector<TFile *>pf_in_vec;
  vector<TTree *>pt_in_vec;

  TFile *pf_out;

  if (argc < 3) {
    cout << "Insufficent arguments, usage:" << endl;
    cout << "./combine_datasets <output-file> <ttree-name> [input-files ...]";
    cout << endl;
    exit(1);
  }

  outfile = string(argv[1]);

  for (int i = 2; i < argc; ++i) {
    datafiles.push_back(argv[i]);
  }

  for (auto & datafile : datafiles) {
    pf_in_vec.push_back(new TFile(datafile.c_str(), "read"));
  }

  for (auto & pf_in : pf_in_vec) {
    pt_in_vec.push_back((TTree *)pf_in->Get("t"));
  }

  // Now get the final combined file into play.
  pf_out = new TFile(outfile.c_str(), "recreate");

  for (int i = 0; i < pt_in_vec.size(); ++i) {

    boost::filesystem::path p(datafiles[i]);
    TTree *pt = pt_in_vec[i]->CloneTree();

    pt->SetName((string("t") + to_string(i)).c_str());
    pt->SetTitle(p.filename().c_str());

    pt->Write();
  }

  // Attach the branches to the final output
  pt_in_vec[0]->SetBranchAddress("field", &field_i);
  pt_in_vec[0]->SetBranchAddress("laser", &laser_i);
  pt_in_vec[0]->SetBranchAddress("ctec", &ctec_i);
  pt_in_vec[0]->SetBranchAddress("flags", &flags_d.platform_data);
  pt_in_vec[0]->SetBranchAddress("envi", &envi_i);
  pt_in_vec[0]->SetBranchAddress("tilt", &tilt_i);
  pt_in_vec[0]->SetBranchAddress("mlab", &mlab_i);

  // Go through the data and interpolate difference from first file.
  for (int idx = 1; idx < pt_in_vec.size(); ++idx) {

  	static double phi_start, phi_stop;
    static int j = -1;
  	static bool interpolation_started;
    boost::filesystem::path p(datafiles[idx]);
  	TTree *pt = new TTree("t", "Default");

    pt->SetName((string("d") + to_string(idx)).c_str());
    pt->SetTitle(p.filename().c_str());

  	// Attach the branches to the final output
  	pt_in_vec[idx]->SetBranchAddress("field", &field_f);
  	pt_in_vec[idx]->SetBranchAddress("laser", &laser_f);
  	pt_in_vec[idx]->SetBranchAddress("ctec", &ctec_f);
  	pt_in_vec[idx]->SetBranchAddress("flags", &flags_d.platform_data);
  	pt_in_vec[idx]->SetBranchAddress("envi", &envi_f);
  	pt_in_vec[idx]->SetBranchAddress("tilt", &tilt_f);
  	pt_in_vec[idx]->SetBranchAddress("mlab", &mlab_f);

  	// Attach the branches to the final output
  	pt->Branch("field", &field_d, gm2::field_str);
  	pt->Branch("laser", &laser_d, gm2::hamar_str);
  	pt->Branch("ctec", &ctec_d, gm2::capacitec_str);
  	pt->Branch("flags", &flags_d.platform_data, gm2::sync_flags_str);
  	pt->Branch("envi", &envi_d, gm2::scs2000_str);
  	pt->Branch("tilt", &tilt_d, gm2::tilt_sensor_str);
  	pt->Branch("mlab", &mlab_d, gm2::metrolab_str);

   	pt_in_vec[idx]->GetEntry(0);
  	phi_start = laser_f.phi_2;

  	pt_in_vec[idx]->GetEntry(pt_in_vec[idx]->GetEntries() - 1);
  	phi_stop = laser_f.phi_2;

  	interpolation_started = false;
  	j = 0;

  	// Interpolate the comparison dataset in phi.
  	for (int i = 0; i < pt_in_vec[0]->GetEntries(); ++i) {

      static double phi, phi_0, phi_1, w0, w1;
      static hamar_t laser_0;

    	pt_in_vec[0]->GetEntry(i);

    	// Set phi for interpolations.
    	phi = laser_i.phi_2;

    	if ((phi < phi_start) && (!interpolation_started)) {
    		continue;
    	}

    	if ((phi > phi_stop) && (interpolation_started)) {
    		continue;
    	}

    	if (phi < phi_start) {
    		phi += 360.0;
    	}

    	interpolation_started = true;

    	// Fine the interpolation angle from laser.
    	do {
    		pt_in_vec[idx]->GetEntry(++j);
    	} while ((laser_f.phi_2 < phi) && (j < pt_in_vec[idx]->GetEntries()));

      --j;

    	pt_in_vec[idx]->GetEntry(j);
    	laser_0 = laser_f;
    		  
    	pt_in_vec[idx]->GetEntry(j + 1);

    	phi_0 = laser_0.phi_2;
    	phi_1 = laser_f.phi_2;
    	w0 = (phi - phi_0) / (phi_1 - phi_0);
    	w1 = 1 - w0;

    	printf("phi: %f, %f, %f, %f\n", phi, phi_0, phi_1, w0);

      laser_d = laser_f;

    	// Now actually interpolate all the stuff in azimuth.
    	pt_in_vec[idx]->GetEntry(j);

    	for (int n = 0; n < 28; ++n) {

    		field_d.sys_clock[n] = w0 * field_f.sys_clock[n];
        field_d.freq[n] = w0 * field_f.freq[n];
        field_d.snr[n] = w0 * field_f.snr[n];
        field_d.len[n] = w0 * field_f.len[n];

      	if (n < 16) {
       		field_d.multipole[n] = w0 * field_f.multipole[n];
          if (n == 0) {
            printf("w0 - field.multipole[0] = %.3f\n", field_d.multipole[n]);
          }
       	}
      }

      ctec_d.inner_up = w0 * ctec_f.inner_up;
      ctec_d.inner_lo = w0 * ctec_f.inner_lo;
      ctec_d.outer_up = w0 * ctec_f.outer_up;
      ctec_d.outer_lo = w0 * ctec_f.outer_lo;

    	// Now actually interpolate all the stuff in azimuth.
    	pt_in_vec[idx]->GetEntry(j + 1);

    	for (int n = 0; n < 28; ++n) {

       	field_d.sys_clock[n] += w1 * field_f.sys_clock[n];
       	field_d.freq[n] += w1 * field_f.freq[n];
      	field_d.snr[n] += w1 * field_f.snr[n];
       	field_d.len[n] += w1 * field_f.len[n];

       	if (n < 16) {
       		field_d.multipole[n] += w1 * field_f.multipole[n];
          if (n == 0) {
            printf("w1 - field.multipole[0] = %.3f\n", field_d.multipole[n]);
          }
       	}
      }

      ctec_d.inner_up += w1 * ctec_f.inner_up;
      ctec_d.inner_lo += w1 * ctec_f.inner_lo;
      ctec_d.outer_up += w1 * ctec_f.outer_up;
      ctec_d.outer_lo += w1 * ctec_f.outer_lo;

      // And subtract off the value from the first file.
      for (int n = 0; n < 28; ++n) {

        field_d.sys_clock[n] -= field_i.sys_clock[n];
        field_d.freq[n] -= field_i.freq[n];
        field_d.snr[n] -= field_i.snr[n];
        field_d.len[n] -= field_i.len[n];

        if (n < 16) {
          field_d.multipole[n] -= field_i.multipole[n];
          if (n == 0) {
            printf("w1 - field.multipole[0] = %.3f\n", field_d.multipole[n]);
          }
        }         
      }

      ctec_d.inner_up -= ctec_i.inner_up;
      ctec_d.inner_lo -= ctec_i.inner_lo;
      ctec_d.outer_up -= ctec_i.outer_up;
      ctec_d.outer_lo -= ctec_i.outer_lo;

      pt->Fill();
    }
  }

  pf_out->Write();
  pf_out->Close();

  for (auto &pf_in : pf_in_vec) {
    pf_in->Close();
  }

  return 0;
}
