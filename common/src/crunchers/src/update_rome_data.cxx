/********************************************************************\

  file:    update_rome_data.cxx
  author:  Matthias W. Smith

  about:   Update the 4 rome trees that connect to a crunched 
           platform file.

\********************************************************************/

//--- std includes -------------------------------------------------//
#include <string>
#include <ctime>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cassert>
using std::string;

//--- other includes -----------------------------------------------//
#include "TTree.h"
#include "TFile.h"

//--- project includes ---------------------------------------------//
#include "shim_structs.hh"

int main(int argc, char *argv[])
{
  int run_number;
  std::string datadir;
  std::string laser_point;
  std::stringstream ss;

  // Declare the data structures.
  gm2::platform_t platform;
  gm2::laser_t laser;
  gm2::capacitec_t ctec;
  gm2::mscb_cart_t cart;
  gm2::tilt_sensor_t tilt;

  // Initialize the times to check for bad data.
  for (int i = 0; i < SHIM_PLATFORM_CH; ++i) {
    platform.sys_clock[i] = 0;
  }
  laser.midas_time = 0;
  ctec.midas_time = 0;
  cart.midas_time = 0;
  tilt.midas_time = 0;

  // And the ROOT variables.
  TFile *pf_platform = nullptr;
  TTree *pt_platform = nullptr;

  TFile *pf_rome_laser = nullptr;
  TTree *pt_rome_laser = nullptr;

  TFile *pf_rome_ctec = nullptr;
  TTree *pt_rome_ctec = nullptr;

  TFile *pf_rome_cart = nullptr;
  TTree *pt_rome_cart = nullptr;

  TFile *pf_rome_tilt = nullptr;
  TTree *pt_rome_tilt = nullptr;

  TFile *pf_all = nullptr;
  TTree *pt_sync = nullptr;
  TTree *pt_cart = nullptr;
  TTree *pt_tilt = nullptr;

  // All the program needs is a run number, and optionally a data dir.
  assert(argc > 0);
  run_number = std::stoi(argv[1]);

  if (argc > 2) {
    
    datadir = std::string(argv[2]);

    if (datadir[datadir.size() - 1] != '/') {
      datadir.append("/");
    }

  } else {
    
    datadir = std::string("data/");
  }

  // Load the rome_laser data.
  ss.str("");
  ss << datadir << "rome/laser_tree_run" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";
  pf_rome_laser = new TFile(ss.str().c_str(), "read");
  
  if (!pf_rome_laser->IsZombie()) {
    pt_rome_laser = (TTree *)pf_rome_laser->Get("g2laser");
  }

  ss.str("");
  ss << datadir << "rome/cap_tree_run" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";
  pf_rome_ctec = new TFile(ss.str().c_str(), "read");
  
  if (!pf_rome_ctec->IsZombie()) {
    pt_rome_ctec = (TTree *)pf_rome_ctec->Get("g2cap");
  }

  // And the slow control data.
  ss.str("");
  ss << datadir << "rome/sc_tree_run" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";
  pf_rome_cart = new TFile(ss.str().c_str(), "read");

  if (!pf_rome_cart->IsZombie()) {
    pt_rome_cart = (TTree *)pf_rome_cart->Get("g2slow");
  }

  // And the cart control data.
  ss.str("");
  ss << datadir << "rome/tilt_tree_run" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";
  pf_rome_tilt = new TFile(ss.str().c_str(), "read");

  if (!pf_rome_tilt->IsZombie()) {
    pt_rome_tilt = (TTree *)pf_rome_tilt->Get("g2tilt");
  }

  // And the platform data.
  ss.str("");
  ss << datadir << "crunched/run_" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";

  pf_platform = new TFile(ss.str().c_str(), "read");
  if (!pf_platform->IsZombie()) {
    pt_platform = (TTree *)pf_platform->Get("t_sync");
  }

  // Now get the final combined file into play.
  ss.str("");
  ss << datadir << "temp.root";
  
  pf_all = new TFile(ss.str().c_str(), "recreate");
  pt_sync = new TTree("t_sync", "Synchronous Shim Data");
  pt_cart = new TTree("t_cart", "Asynchronous SCS200 Data");
  pt_tilt = new TTree("t_tilt", "Asynchronous Tilt Sensor Data");

  // Attach the branches to the final output
  pt_sync->Branch("platform", &platform, gm2::platform_str);
  pt_sync->Branch("laser", &laser, gm2::laser_str);
  pt_sync->Branch("ctec", &ctec, gm2::capacitec_str);
  pt_cart->Branch("cart", &cart, gm2::mscb_cart_str);
  pt_tilt->Branch("tilt", &tilt, gm2::tilt_sensor_str);

  // Attach to the data files if they exist.
  if (pt_rome_laser != nullptr) {

    if (run_number < 787) {
      pt_rome_laser->SetBranchAddress("Timestamp", &laser.midas_time);
      pt_rome_laser->SetBranchAddress("position_rad", &laser.r_1);
      pt_rome_laser->SetBranchAddress("position_height", &laser.phi_1);
      pt_rome_laser->SetBranchAddress("position_azim", &laser.z_1);
      laser.r_2 = 0.0;
      laser.z_2 = 0.0;
      laser.phi_2 = 0.0;

    } else {

      pt_rome_laser->SetBranchAddress("Timestamp", &laser.midas_time);
      pt_rome_laser->SetBranchAddress("p1_rad", &laser.r_1);
      pt_rome_laser->SetBranchAddress("p1_phi", &laser.phi_1);
      pt_rome_laser->SetBranchAddress("p1_height", &laser.z_1);
      pt_rome_laser->SetBranchAddress("p2_rad", &laser.r_2);
      pt_rome_laser->SetBranchAddress("p2_phi", &laser.phi_2);
      pt_rome_laser->SetBranchAddress("p2_height", &laser.z_2);
    }
  }

  if (pt_rome_ctec != nullptr) {

    pt_rome_ctec->SetBranchAddress("Timestamp", &ctec.midas_time);
    pt_rome_ctec->SetBranchAddress("cap_0", &ctec.outer_lo);
    pt_rome_ctec->SetBranchAddress("cap_1", &ctec.inner_up);
    pt_rome_ctec->SetBranchAddress("cap_2", &ctec.inner_lo);
    pt_rome_ctec->SetBranchAddress("cap_3", &ctec.outer_up);
  }

  if (pt_platform != nullptr) {
    pt_platform->SetBranchAddress("platform", &platform.sys_clock[0]);
  }

  if (pt_rome_cart != nullptr) {
    pt_rome_cart->SetBranchAddress("Timestamp", &cart.midas_time);
    pt_rome_cart->SetBranchAddress("Temp0", &cart.temp[0]);
    pt_rome_cart->SetBranchAddress("Temp1", &cart.temp[1]);
    pt_rome_cart->SetBranchAddress("Temp2", &cart.temp[2]);
    pt_rome_cart->SetBranchAddress("Temp3", &cart.temp[3]);
    pt_rome_cart->SetBranchAddress("Temp4", &cart.temp[4]);
    pt_rome_cart->SetBranchAddress("Temp5", &cart.temp[5]);
    pt_rome_cart->SetBranchAddress("Temp6", &cart.temp[6]);
    pt_rome_cart->SetBranchAddress("Temp7", &cart.temp[7]);
    pt_rome_cart->SetBranchAddress("Capac0", &cart.ctec[0]);
    pt_rome_cart->SetBranchAddress("Capac1", &cart.ctec[1]);
    pt_rome_cart->SetBranchAddress("Capac2", &cart.ctec[2]);
    pt_rome_cart->SetBranchAddress("Capac3", &cart.ctec[3]);
  }
  
  if (pt_rome_tilt != nullptr) {

    pt_rome_tilt->SetBranchAddress("Timestamp", &tilt.midas_time);
    pt_rome_tilt->SetBranchAddress("Tilt0", &tilt.temp);
    pt_rome_tilt->SetBranchAddress("Tilt1", &tilt.phi);
    pt_rome_tilt->SetBranchAddress("Tilt2", &tilt.rad);
  }

  int num_sync_events = 0;
  if (pt_platform != nullptr) {
    num_sync_events = pt_platform->GetEntries();
  }

  if (pt_rome_laser != nullptr) {
    int nlaser = pt_rome_laser->GetEntries();
    if ((num_sync_events > nlaser) && (nlaser != 0)) {
      num_sync_events = pt_rome_laser->GetEntries();
    }
  }

  if (pt_rome_ctec != nullptr) {
    int nctec = pt_rome_ctec->GetEntries();
    if ((num_sync_events > nctec) && (nctec != 0)) {
      num_sync_events = pt_rome_ctec->GetEntries();
    }
  }

  for (int i = 0; i < num_sync_events; ++i) {
    
    if (pt_platform != nullptr) {
      pt_platform->GetEntry(i);
    }

    if (pt_rome_laser != nullptr) {
      pt_rome_laser->GetEntry(i);
    }

    if (pt_rome_ctec != nullptr) {
      pt_rome_ctec->GetEntry(i);
    }

    pt_sync->Fill();
  }

  if (pt_rome_cart != nullptr) {
    for (int i = 0; i < pt_rome_cart->GetEntries(); ++i) {
      pt_rome_cart->GetEntry(i);
      pt_cart->Fill();
    }
  }

  if (pt_rome_tilt != nullptr) {
    for (int i = 0; i < pt_rome_tilt->GetEntries(); ++i) {
      pt_rome_tilt->GetEntry(i);
      pt_tilt->Fill();
    }
  }

  ss.str("");
  ss << datadir << "crunched/run_" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";

  pf_all->Write();
  pf_all->Cp(ss.str().c_str());
  pf_all->Close();

  ss.str("");
  ss << datadir << "temp.root";
  std::remove(ss.str().c_str());

  pf_rome_laser->Close(); 
  pf_rome_ctec->Close();
  pf_rome_cart->Close();
  pf_rome_tilt->Close();
  pf_rome_tilt->Close();
  pf_platform->Close();

  return 0;
}
