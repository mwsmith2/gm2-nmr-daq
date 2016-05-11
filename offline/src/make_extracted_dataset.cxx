/*===========================================================================*\

author: Matthias W. Smith
email:  mwsmith2@uw.edu
date:   2016/05/10

about:  The program runs on a crunched file and packages all the data
        with units converted and traces dropped.  A dataset that has
        gone through a sieve.

usage:

./make_extracted_dataset <input-file> [output-dir]

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
  int run_number;
  bool laser_swap;
  string datafile;
  string outfile;
  string outdir;
  stringstream ss;

  // Declare the data structures.
  platform_t iplatform;
  laser_t ilaser;
  capacitec_t ictec;
  metrolab_t imlab;
  mscb_cart_t icart;
  mscb_ring_t iring;
  tilt_sensor_t itilt;
  hall_probe_t ihall;
  data_flags_t iflags;

  field_t ofield;
  laser_t olaser;
  capacitec_t octec;
  metrolab_t omlab;
  mscb_cart_t ocart;
  mscb_ring_t oring;
  tilt_sensor_t otilt;
  hall_probe_t ohall;
  data_flags_t oflags;

  // And the ROOT variables.
  TFile *pf_in;
  TTree *pt_sync;
  TTree *pt_cart;
  TTree *pt_ring;
  TTree *pt_tilt;
  TTree *pt_hall;
  TTree *pt_mlab;

  TFile *pf_out;
  TTree *pt_out;

  if (argc < 2) {
    cout << "Insufficent arguments, usage:" << endl;
    cout << "make_extracted_dataset <input-file> [output-dir]" << endl;
    exit(1);
  }

  datafile = string(argv[1]);

  // Set output directory.
  if (argc > 2) {

    outdir = string(argv[2]);

  } else {

    outdir = string("data/extracted/");
    auto fname = boost::filesystem::path(datafile).filename().string();
    outfile = outdir + fname;
  }

  // Make sure it is a root file.
  assert (datafile.find_last_of(".root") == datafile.size() - 1);
  pf_in = new TFile(datafile.c_str(), "read");

  pt_sync = (TTree *)pf_in->Get("t_sync");
  pt_sync->SetBranchAddress("platform", &iplatform.sys_clock[0]);
  pt_sync->SetBranchAddress("ctec", &ictec.midas_time);
  pt_sync->SetBranchAddress("flags", &iflags.platform_data);
  pt_sync->SetBranchAddress("laser", &ilaser.midas_time);

  pt_cart = (TTree *)pf_in->Get("t_mscb_cart");
  pt_cart->SetBranchAddress("cart", &icart.midas_time);

  pt_ring = (TTree *)pf_in->Get("t_mscb_ring");
  pt_ring->SetBranchAddress("ring", &iring.midas_time);

  pt_tilt = (TTree *)pf_in->Get("t_tilt");
  pt_tilt->SetBranchAddress("tilt", &itilt.midas_time);

  pt_hall = (TTree *)pf_in->Get("t_hall");
  pt_hall->SetBranchAddress("hall", &ihall.volt);

  pt_mlab = (TTree *)pf_in->Get("t_mlab");
  pt_mlab->SetBranchAddress("mlab", &imlab.field);

  // Now get the final combined file into play.
  pf_out = new TFile(outfile.c_str(), "recreate");
  pt_out = new TTree("t", "Derived Shim Data");

  // Attach the branches to the final output.
  pt_out->Branch("field", &ofield, gm2::field_str);
  pt_out->Branch("laser", &olaser, gm2::laser_str);
  pt_out->Branch("ctec", &octec, gm2::capacitec_str);
  pt_out->Branch("flags", &oflags.platform_data, gm2::data_flags_str);
  pt_out->Branch("cart", &ocart, gm2::mscb_cart_str);
  pt_out->Branch("ring", &oring, gm2::mscb_ring_str);
  pt_out->Branch("tilt", &otilt, gm2::tilt_sensor_str);
  pt_out->Branch("hall", &ohall, gm2::hall_probe_str);
  pt_out->Branch("mlab", &omlab, gm2::metrolab_str);

  // Go through the data, and sort by azimuth.
  int n = pt_sync->BuildIndex("10000*laser.phi_2", "Entry$");
  TTreeIndex *index = (TTreeIndex *)pt_sync->GetTreeIndex();

  for (int idx = 0; idx < index->GetN(); ++idx) {

    pt_sync->GetEntry(index->GetIndex()[idx]);

    // Copy the old data that needs to be saved.
    olaser = ilaser;
    octec = ictec;
    oflags = iflags;

    octec.inner_up *= 127;
    octec.inner_lo *= 127;
    octec.outer_up *= 127;
    octec.outer_lo *= 127;

    // Set the time for interpolations.
    int j = 0;
    double t = ictec.midas_time;
    double w0 = 0.0;
    double w1 = 0.0;

    // Interpolate the cart TTree.
    mscb_cart_t cart_0;

    pt_cart->GetEntry(j++);
    while (icart.midas_time < t && j < pt_cart->GetEntries()) {
      cart_0 = icart;
      pt_cart->GetEntry(j++);
    }

    w0 = (t - cart_0.midas_time) / (icart.midas_time - cart_0.midas_time);
    w1 = 1 - w0;

    ocart.midas_time = t;

    for (int i = 0; i < 8; ++i) {
      ocart.temp[i] = w0 * cart_0.temp[i] + w1 * icart.temp[i];
    }

    for (int i = 0; i < 4; ++i) {
      ocart.ctec[i] = w0 * cart_0.ctec[i] + w1 * icart.ctec[i];
      ocart.ctec[i] *= 127;
    }


    // Interpolate the ring TTree.
    mscb_ring_t ring_0;

    pt_ring->GetEntry(j++);
    while (iring.midas_time < t && j < pt_ring->GetEntries()) {
      ring_0 = iring;
      pt_ring->GetEntry(j++);
    }

    w0 = (t - ring_0.midas_time) / (iring.midas_time - ring_0.midas_time);
    w1 = 1 - w0;

    oring.midas_time = t;

    for (int i = 0; i < 12; ++i) {
      oring.temp[i] = w0 * ring_0.temp[i] + w1 * iring.temp[i];
    }

    // Interpolate the tilt sensor TTree.
    j = 0;
    tilt_sensor_t tilt_0;

   pt_tilt->GetEntry(j++);
    while (itilt.midas_time < t && j < pt_tilt->GetEntries()) {
      tilt_0 = itilt;
      pt_tilt->GetEntry(j++);
    }

    w0 = (t - tilt_0.midas_time) / (itilt.midas_time - tilt_0.midas_time);
    w1 = 1 - w0;

    otilt.midas_time = t;

    otilt.temp = w0 * tilt_0.temp + w1 * itilt.temp;
    otilt.phi = w0 * tilt_0.phi + w1 * itilt.phi;
    otilt.rad = w0 * tilt_0.rad + w1 * itilt.rad;

    hall_probe_t hall_0;
    
    pt_hall->GetEntry(j++);
    while (ihall.midas_time < t && j < pt_hall->GetEntries()) {
      hall_0 = ihall;
      pt_hall->GetEntry(j++);
    }

    w0 = (t - hall_0.midas_time) / (ihall.midas_time - hall_0.midas_time);
    w1 = 1 - w0;

    ohall.midas_time = t;

    ohall.volt = w0 * hall_0.volt + w1 * ihall.volt;
    ohall.temp = w0 * hall_0.temp + w1 * ihall.temp;

    // Interpolate the metrolab TTree.
    j = 0;
    metrolab_t mlab_0;

    pt_mlab->GetEntry(j++);
    while (imlab.midas_time < t && j < pt_mlab->GetEntries()) {
      mlab_0 = imlab;
      pt_mlab->GetEntry(j++);
    }

    w0 = (t - mlab_0.midas_time) / (imlab.midas_time - mlab_0.midas_time);
    w1 = 1 - w0;

    omlab = imlab;
    omlab.midas_time = t;
    omlab.field = w0 * mlab_0.field + w1 * imlab.field;

    // Now trim down and record the field data.
    for (int i = 0; i < SHIM_PLATFORM_CH; ++i) {
      ofield.sys_clock[i] = iplatform.sys_clock[i];
      ofield.freq[i] = iplatform.freq[i];
      ofield.snr[i] = iplatform.snr[i];
      ofield.len[i] = iplatform.len[i];
    }

    // Fill in probe 19 with average of closest probes.
    if (oflags.missing_probe19) {
      ofield.freq[18] = 0.296 * ofield.freq[19];
      ofield.freq[18] += 0.296 * ofield.freq[17];
      ofield.freq[18] += 0.204 * ofield.freq[6];
      ofield.freq[18] += 0.204 * ofield.freq[5];

      iplatform.freq[18] = ofield.freq[18];
    }

    auto mp = fit_multipoles(iplatform);

    for (int i = 0; i < num_multipoles; ++i) {
      ofield.multipole[i] = mp[i] / field_khz_to_ppm;
    }

    pt_out->Fill();
  }

  pf_out->Write();
  pf_out->Close();
  pf_in->Close();

  return 0;
}
