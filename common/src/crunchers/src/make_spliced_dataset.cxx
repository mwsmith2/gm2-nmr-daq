/*===========================================================================*\

author: Matthias W. Smith
email:  mwsmith2@uw.edu
date:   2016/07/22

about:  Merges extracted datasets in a smart way that replaces only an
      optionally give phi range

usage:

./make_spliced_dataset <base-file> <new-datafiles ...>

\*===========================================================================*/


//--- std includes ----------------------------------------------------------//
#include <iostream>
#include <cmath>

//--- other includes --------------------------------------------------------//
#include <boost/filesystem.hpp>
#include "TFile.h"
#include "TTree.h"
#include "TTreeIndex.h"
#include "TCanvas.h"

//--- project includes ------------------------------------------------------//
#include "fid.h"
#include "shim_structs.hh"
#include "shim_analyzer.hh"

using namespace std;
using namespace gm2;

int main(int argc, char *argv[])
{
  string oldfile;
  string newfile;
  string outfile;
  string outdir;
  stringstream ss;

  // Declare the data structures.
  field_t old_field;
  laser_t old_laser;
  capacitec_t old_ctec;
  metrolab_t old_mlab;
  mscb_cart_t old_cart;
  mscb_ring_t old_ring;
  tilt_sensor_t old_tilt;
  hall_probe_t old_hall;
  data_flags_t old_flags;

  field_t new_field;
  laser_t new_laser;
  capacitec_t new_ctec;
  metrolab_t new_mlab;
  mscb_cart_t new_cart;
  mscb_ring_t new_ring;
  tilt_sensor_t new_tilt;
  hall_probe_t new_hall;
  data_flags_t new_flags;

  field_t out_field;
  laser_t out_laser;
  capacitec_t out_ctec;
  metrolab_t out_mlab;
  mscb_cart_t out_cart;
  mscb_ring_t out_ring;
  tilt_sensor_t out_tilt;
  hall_probe_t out_hall;
  data_flags_t out_flags;

  // And the ROOT variables.
  TFile *pf_old;
  TTree *pt_old;

  TFile *pf_new;
  TTree *pt_new;

  TFile *pf_out;
  TTree *pt_out;

  if (argc < 3) {
    cerr << "Insufficent arguments, usage:" << endl;
    cerr << "make_spliced_dataset <base-file> <new-file>" << endl;
    exit(1);
  }

  oldfile = string(argv[1]);
  newfile = string(argv[2]);

  // Make sure it is a root file.
  if (oldfile.find_last_of(".root") != oldfile.size() - 1) {
    cerr << "Error: input was not a root file." << endl;
    exit(2);
  }

  // Set output directory.
  outdir = string("data/spliced/");
  auto fname = boost::filesystem::path(oldfile).filename().string();
  outfile = outdir + fname;

  // Attach to the trees.
  pf_old = new TFile(oldfile.c_str(), "read");
  pt_old = (TTree *)pf_old->Get("t");

  // Make sure it's an extracted file
  if (pf_old->GetListOfKeys()->Contains("t_sync")) {
    cerr << "Error: input was not a an extracted dataset." << endl;
    exit(3);
  }

  pt_old->SetBranchAddress("field", &old_field.sys_clock[0]);
  pt_old->SetBranchAddress("ctec", &old_ctec.midas_time);
  pt_old->SetBranchAddress("flags", &old_flags.platform_data);
  pt_old->SetBranchAddress("laser", &old_laser.midas_time);
  pt_old->SetBranchAddress("cart", &old_cart.midas_time);
  pt_old->SetBranchAddress("ring", &old_ring.midas_time);
  pt_old->SetBranchAddress("tilt", &old_tilt.midas_time);
  pt_old->SetBranchAddress("hall", &old_hall.volt);
  pt_old->SetBranchAddress("mlab", &old_mlab.field);

  pf_new = new TFile(newfile.c_str(), "read");
  pt_new = (TTree *)pf_new->Get("t");

  pt_new->SetBranchAddress("field", &new_field.sys_clock[0]);
  pt_new->SetBranchAddress("ctec", &new_ctec.midas_time);
  pt_new->SetBranchAddress("flags", &new_flags.platform_data);
  pt_new->SetBranchAddress("laser", &new_laser.midas_time);
  pt_new->SetBranchAddress("cart", &new_cart.midas_time);
  pt_new->SetBranchAddress("ring", &new_ring.midas_time);
  pt_new->SetBranchAddress("tilt", &new_tilt.midas_time);
  pt_new->SetBranchAddress("hall", &new_hall.volt);
  pt_new->SetBranchAddress("mlab", &new_mlab.field);

  // Now get the final combined file into play.
  pf_out = new TFile(outfile.c_str(), "recreate");
  pt_out = new TTree("t", "Spliced Shim Data");

  // Attach the branches to the final output.
  pt_out->Branch("field", &out_field, gm2::field_str);
  pt_out->Branch("laser", &out_laser, gm2::laser_str);
  pt_out->Branch("ctec", &out_ctec, gm2::capacitec_str);
  pt_out->Branch("flags", &out_flags.platform_data, gm2::data_flags_str);
  pt_out->Branch("cart", &out_cart, gm2::mscb_cart_str);
  pt_out->Branch("ring", &out_ring, gm2::mscb_ring_str);
  pt_out->Branch("tilt", &out_tilt, gm2::tilt_sensor_str);
  pt_out->Branch("hall", &out_hall, gm2::hall_probe_str);
  pt_out->Branch("mlab", &out_mlab, gm2::metrolab_str);

  // Go through the data, and sort by azimuth.
  int n = 0;
  if (pt_old->GetEntries() > 0) {

    n = pt_old->BuildIndex("10000*laser.phi_2", "Entry$");

  } else {
    cout << "No synchronous entries.  Exiting." << endl;
    exit(3);
  }

  TTreeIndex *index = (TTreeIndex *)pt_old->GetTreeIndex();

  // Get the range of the new data.
  pt_new->GetEntry(0);
  double phi_min = new_laser.phi_2;

  pt_new->GetEntry(pt_new->GetEntries() - 1);
  double phi_max = new_laser.phi_2;

  bool done_splicing_data = false;
  bool contains_zero = false;
  bool use_old_data = true;

  if (phi_max < phi_min) {
    contains_zero = true;
  }

  printf("phi range: [%.2f, %.2f]\n", phi_min, phi_max);

  for (int idx = 0; idx < index->GetN(); ++idx) {

    pt_old->GetEntry(index->GetIndex()[idx]);

    if (contains_zero) {
      use_old_data = old_laser.phi_2 >= phi_max;
      use_old_data &= old_laser.phi_2 <= phi_min;
    } else {
      use_old_data = old_laser.phi_2 >= phi_max;
      use_old_data |= old_laser.phi_2 <= phi_min;
    }

    cout << "bools: " << use_old_data << ", " << done_splicing_data << endl;

    if (use_old_data) {

      done_splicing_data = false;

      out_mlab = old_mlab;
      out_hall = old_hall;
      out_tilt = old_tilt;
      out_ring = old_ring;
      out_cart = old_cart;
      out_flags = old_flags;
      out_ctec = old_ctec;
      out_laser = old_laser;
      out_field = old_field;
      pt_out->Fill();
      printf("phi = %.2f, %.2f, filled old\n", old_laser.phi_2, new_laser.phi_2);

    } else if (!done_splicing_data) {

      done_splicing_data = true;

      for (int j = 0; j < pt_new->GetEntries(); ++j) {

        pt_new->GetEntry(j);

        if (!contains_zero) {

          out_mlab = new_mlab;
          out_hall = new_hall;
          out_tilt = new_tilt;
          out_ring = new_ring;
          out_cart = new_cart;
          out_flags = new_flags;
          out_ctec = new_ctec;
          out_laser = new_laser;
          out_field = new_field;
          printf("phi = %.2f, %.2f, filled new\n", old_laser.phi_2, new_laser.phi_2);
          pt_out->Fill();

        } else if ((old_laser.phi_2 <= phi_max) && (new_laser.phi_2 >= 0)) {

          out_mlab = new_mlab;
          out_hall = new_hall;
          out_tilt = new_tilt;
          out_ring = new_ring;
          out_cart = new_cart;
          out_flags = new_flags;
          out_ctec = new_ctec;
          out_laser = new_laser;
          out_field = new_field;
          pt_out->Fill();
          printf("phi = %.2f, %.2f, filled new\n", old_laser.phi_2, new_laser.phi_2);

        } else if ((old_laser.phi_2 >= phi_min) && (new_laser.phi_2 >= phi_min)) {
          out_mlab = new_mlab;
          out_hall = new_hall;
          out_tilt = new_tilt;
          out_ring = new_ring;
          out_cart = new_cart;
          out_flags = new_flags;
          out_ctec = new_ctec;
          out_laser = new_laser;
          out_field = new_field;
          pt_out->Fill();
          printf("phi = %.2f, %.2f, filled new\n", old_laser.phi_2, new_laser.phi_2);
        }
      }
    }
  }

  pf_out->Write();
  pf_out->Close();
  pf_old->Close();
  pf_new->Close();

  return 0;
}
