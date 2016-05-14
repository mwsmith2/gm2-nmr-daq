/*===========================================================================*\
v
author: Matthias W. Smith
email:  mwsmith2@uw.edu
date:   2016/05/10

about: The program combines root trees from shimming into a single file.

usage:

./make_shim_dataset <run-number> [output-dir]

\*===========================================================================*/

//--- std includes ----------------------------------------------------------//
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <array>
#include <cmath>

//--- other includes --------------------------------------------------------//
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TGraphErrors.h"
#include "TGraph2D.h"
#include "TStyle.h"

//--- project includes ------------------------------------------------------//
#include "shim_structs.hh"

using namespace std;
using namespace gm2;

int main(int argc, char **argv)
{
  // Use to make checksum changeable at will.
  const int id = 2L; 

  // Allocate parameters
  const double dt = 10.0 / SHORT_FID_LN;
  const double t0 = 0.0;
  double platform_coord[SHIM_PLATFORM_CH][2];

  // Attach the appropriate branches/leaves.
  platform_t platform;
  laser_t laser;
  capacitec_t ctec;
  data_flags_t flags;
  string laser_point;
  bool laser_swap;

  int run_number;
  string datafile;
  string outfile;
  string datadir("data/shim");
  string outdir("data/crunched");
  stringstream ss;

  // Input files and trees.
  TFile *pf_shpf;
  TFile *pf_ltrk;
  TFile *pf_ctec;
  TFile *pf_cart;
  TFile *pf_ring;
  TFile *pf_tilt;
  TFile *pf_hall;
  TFile *pf_mlab;

  TTree *pt_shpf;
  TTree *pt_ltrk;
  TTree *pt_ctec;
  TTree *pt_cart;
  TTree *pt_ring;
  TTree *pt_tilt;
  TTree *pt_hall;
  TTree *pt_mlab;

  // Single output file.
  TFile *pf_out;
  TTree *pt_sync; // combine some of the variables

  // Declare ROOT variables.
  TCanvas c1;

  if (argc < 2) {
    cout << "Insufficent arguments, usage:" << endl;
    cout << "./make_shim_dataset <run-number> [output-dir]";
    cout << endl;
    exit(1);
  }

  run_number = stoi(argv[1]);

  // Set input directory.
  if (argc > 2) {

    datadir = string(argv[2]);

    if (datadir[datadir.size() -1 ] != '/') {
      datadir += string("/");
    }
  }

  // Set output directory.
  if (argc > 3) {

    outdir = string(argv[3]);

    if (outdir[outdir.size() -1 ] != '/') {
      outdir += string("/");
    }
  }

  // Open the input files, first do the platform data.
  ss.str("");
  ss << datadir << "/platform_tree_" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";
  datafile = ss.str();

  pf_shpf = new TFile(datafile.c_str(), "read");
  
  if (!pf_shpf->IsZombie()) {

    pt_shpf = (TTree *)pf_shpf->Get("t_shpf");
    flags.platform_data = true;

  } else {
    
    pt_shpf = new TTree("t_shpf", "Shim Platform Data");
    flags.platform_data = false;
  }

  // Now load the laser tracker data.
  ss.str("");
  ss << datadir << "/laser_tree_" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";
  datafile = ss.str();

  pf_ltrk= new TFile(datafile.c_str(), "read");
  if (!pf_ltrk->IsZombie()) {

    pt_ltrk = (TTree *)pf_ltrk->Get("t_ltrk");
    flags.laser_data = true;
    flags.laser_p1 = false;
    flags.laser_p2 = false;
    flags.laser_swap = false;

  } else {
    
    pt_ltrk = new TTree("t_ltrk", "Laser Tracker Data");
    flags.laser_data = false;
    flags.laser_p1 = false;
    flags.laser_p2 = false;
    flags.laser_swap = false;
  }

  // Now load the capacitec data.
  ss.str("");
  ss << datadir << "/capacitec_tree_" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";
  datafile = ss.str();

  pf_ctec= new TFile(datafile.c_str(), "read");
  if (!pf_ctec->IsZombie()) {

    pt_ctec = (TTree *)pf_ctec->Get("t_ctec");
    flags.laser_data = true;

  } else {
    
    pt_ctec = new TTree("t_ctec", "Capacitec Sensor Data");
    flags.laser_data = false;
  }

  // Now load the cart scs2000 slow control data.
  ss.str("");
  ss << datadir << "/mscb_cart_tree_" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";
  datafile = ss.str();

  pf_cart= new TFile(datafile.c_str(), "read");
  if (!pf_cart->IsZombie()) {

    pt_cart = (TTree *)pf_cart->Get("t_mscb_cart");
    flags.mscb_cart_data = true;

  } else {
    
    pt_cart = new TTree("t_mscb_cart", "MSCB Data from Shimming Cart");
    flags.mscb_cart_data = false;
  }

  // Now load the ring scs2000 slow control data.
  ss.str("");
  ss << datadir << "/mscb_ring_tree_" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";
  datafile = ss.str();

  pf_ring= new TFile(datafile.c_str(), "read");
  if (!pf_ring->IsZombie()) {

    pt_ring = (TTree *)pf_ring->Get("t_mscb_ring");
    flags.mscb_ring_data = true;

  } else {

    pt_ring = new TTree("t_mscb_ring", "MSCB Data from Ring Environment");
    flags.mscb_ring_data = false;
  }

  // Now load the tilt sensor data.
  ss.str("");
  ss << datadir << "/tilt_sensor_tree_" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";
  datafile = ss.str();

  pf_tilt= new TFile(datafile.c_str(), "read");
  if (!pf_tilt->IsZombie()) {

    pt_tilt = (TTree *)pf_tilt->Get("t_tilt");
    flags.tilt_data = true;

  } else {
    
    pt_tilt = new TTree("t_tilt", "Tilt Sensor Data");
    flags.tilt_data = false;
  }

  // Now load the hall probe platform.
  ss.str("");
  ss << datadir << "/hall_probe_tree_" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";
  datafile = ss.str();

  pf_hall= new TFile(datafile.c_str(), "read");
  if (!pf_hall->IsZombie()) {

    pt_hall = (TTree *)pf_hall->Get("t_hall");
    flags.hall_data = true;

  } else {
    
    pt_hall = new TTree("t_hall", "Hall Probe Data");
    flags.hall_data = false;
  }

  // Now load the Metrolab NMR data.
  ss.str("");
  ss << datadir << "/metrolab_tree_" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";
  datafile = ss.str();

  pf_mlab= new TFile(datafile.c_str(), "read");
  if (!pf_mlab->IsZombie()) {

    pt_mlab = (TTree *)pf_mlab->Get("t_mlab");
    flags.mlab_data = true;

  } else {
    
    pt_mlab = nullptr;
    flags.mlab_data = false;
  }

  // And set the output file.
  ss.str("");
  ss << outdir << "/run_" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";
  outfile = ss.str();

  pt_shpf->SetBranchAddress("platform", &platform.sys_clock[0]);
  pt_ltrk->SetBranchAddress("laser", &laser.midas_time);
  pt_ctec->SetBranchAddress("ctec", &ctec.midas_time);

  pf_out = new TFile(outfile.c_str(), "recreate");
  pt_sync = new TTree("t_sync", "Synchronous Shimming Data");
  pt_sync->Branch("platform", &platform.sys_clock[0], platform_str);
  pt_sync->Branch("laser", &laser.midas_time, laser_str);
  pt_sync->Branch("ctec", &ctec.midas_time, capacitec_str);
  pt_sync->Branch("flags", &flags.platform_data, data_flags_str);

  // Set the data flag for missing probe 19.
  if (run_number < 1475) {
    flags.missing_probe19 = true;
  }

  // Load the run attributes.
  string attr_file("/home/newg2/Applications/gm2-nmr/");
  attr_file += string("resources/log/run_attributes.json");

  boost::property_tree::ptree pt;
  boost::property_tree::read_json(attr_file, pt);

  ss.str("");
  ss << std::setfill('0') << std::setw(5) << run_number << ".laser_point";
  laser_point = pt.get<string>(ss.str());

  ss.str("");
  ss << std::setfill('0') << std::setw(5) << run_number << ".laser_swap";
  laser_swap = pt.get<bool>(ss.str());

  // Go through the data.
  for (int idx = 0; idx < pt_shpf->GetEntries(); ++idx) {

    pt_shpf->GetEntry(idx);
    pt_ltrk->GetEntry(idx);
    pt_ctec->GetEntry(idx);

    if (run_number < 787) {
      
      if (laser_point == string("P1")) {
        
        flags.laser_p1 = true;
    
      } else if (laser_point == string("P2")) {

        flags.laser_p2 = true;

        laser.r_2 = laser.r_1;
        laser.z_2 = laser.z_1;
        laser.phi_2 = laser.phi_1;

        laser.r_1 = 0.0;
        laser.z_1 = 0.0;
        laser.phi_1 = 0.0;

      }

    } else {

      if (laser_point == string("P1")) {
        flags.laser_p1 = true;
      }
      
      if (laser_point == string("P2")) {
        flags.laser_p2 = true;
      }
      
      if (laser_swap) {

        flags.laser_swap = true;
        
        double r_2 = laser.r_2;
        double z_2 = laser.z_2;
        double phi_2 = laser.phi_2;
        
        laser.r_2 = laser.r_1;
        laser.z_2 = laser.z_1;
        laser.phi_2 = laser.phi_1;

        laser.r_1 = r_2;
        laser.z_1 = z_2;
        laser.phi_1 = phi_2;
      }
    } 
    
    pt_sync->Fill();
  }

  TTree *pt_async_0 = pt_cart->CloneTree();
  pt_async_0->Write();

  TTree *pt_async_1 = pt_ring->CloneTree();
  pt_async_1->Write();

  TTree *pt_async_2 = pt_tilt->CloneTree();
  pt_async_2->Write();

  TTree *pt_async_3 = pt_hall->CloneTree();
  pt_async_3->Write();

  TTree *pt_async_4 = pt_mlab->CloneTree();
  pt_async_4->Write();

  pf_out->Write();
  pf_out->Close();

  pf_shpf->Close();
  pf_ltrk->Close();
  pf_ctec->Close();
  pf_cart->Close();
  pf_ring->Close();
  pf_mlab->Close();
  pf_tilt->Close();
  pf_hall->Close();

  return 0;
}
