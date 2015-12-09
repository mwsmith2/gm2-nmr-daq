/********************************************************************\

  file:    shim_data_bundler.cxx
  author:  Matthias W. Smith

  about:   Takes rome root trees and bundles them together.

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
using std::cout;
using std::endl;

//--- other includes -----------------------------------------------//
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "TTree.h"
#include "TFile.h"

//--- project includes ---------------------------------------------//
#include "shim_structs.hh"

int main(int argc, char *argv[])
{
  int run_number;
  bool laser_swap;
  std::string datadir;
  std::string laser_point;
  std::stringstream ss;

  // Declare the data structures.
  gm2::platform_t platform;
  gm2::hamar_t laser;
  gm2::capacitec_t ctec;
  gm2::metrolab_t mlab;
  gm2::scs2000_t envi;
  gm2::tilt_sensor_t tilt;
  gm2::sync_flags_t flags;

  // Initialize the times to check for bad data.
  for (int i = 0; i < SHIM_PLATFORM_CH; ++i) {
    platform.sys_clock[i] = 0;
  }
  laser.midas_time = 0;
  ctec.midas_time = 0;
  mlab.midas_time = 0;
  envi.midas_time = 0;
  tilt.midas_time = 0;

  // And the ROOT variables.
  TFile *pf_platform = nullptr;
  TTree *pt_platform = nullptr;

  TFile *pf_rome_laser = nullptr;
  TTree *pt_rome_laser = nullptr;

  TFile *pf_rome_ctec = nullptr;
  TTree *pt_rome_ctec = nullptr;

  TFile *pf_rome_envi = nullptr;
  TTree *pt_rome_envi = nullptr;

  TFile *pf_rome_tilt = nullptr;
  TTree *pt_rome_tilt = nullptr;

  TFile *pf_rome_mlab = nullptr;
  TTree *pt_rome_mlab = nullptr;

  TFile *pf_all = nullptr;
  TTree *pt_sync = nullptr;
  TTree *pt_envi = nullptr;
  TTree *pt_tilt = nullptr;
  TTree *pt_mlab = nullptr;

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
  pf_rome_envi = new TFile(ss.str().c_str(), "read");

  if (!pf_rome_envi->IsZombie()) {
    pt_rome_envi = (TTree *)pf_rome_envi->Get("g2slow");
  }

  // And the tilt sensor data.
  ss.str("");
  ss << datadir << "rome/tilt_tree_run" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";
  pf_rome_tilt = new TFile(ss.str().c_str(), "read");

  if (!pf_rome_tilt->IsZombie()) {
    pt_rome_tilt = (TTree *)pf_rome_tilt->Get("g2tilt");
  }

  // And the metrolab data.
  ss.str("");
  ss << datadir << "rome/metrolab_tree_run" << std::setfill('0');
  ss << std::setw(5) << run_number << ".root";
  pf_rome_mlab = new TFile(ss.str().c_str(), "read");

  if (!pf_rome_tilt->IsZombie()) {
    pt_rome_mlab = (TTree *)pf_rome_mlab->Get("g2metrolab");
  }

  // And the platform data.
  ss.str("");
  ss << datadir << "root/platform_run_" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";

  pf_platform = new TFile(ss.str().c_str(), "read");
  if (!pf_platform->IsZombie()) {
    pt_platform = (TTree *)pf_platform->Get("t_shpf");
  }

  // Now get the final combined file into play.
  ss.str("");
  ss << datadir << "shim/run_" << std::setfill('0') << std::setw(5);
  ss << run_number << ".root";
  
  pf_all = new TFile(ss.str().c_str(), "recreate");
  pt_sync = new TTree("t_sync", "Synchronous Shim Data");
  pt_envi = new TTree("t_envi", "Asynchronous SCS200 Data");
  pt_tilt = new TTree("t_tilt", "Asynchronous Tilt Sensor Data");
  pt_mlab = new TTree("t_mlab", "Asynchronous Metrolab Data");

  // Attach the branches to the final output
  pt_sync->Branch("platform", &platform, gm2::platform_str);
  pt_sync->Branch("laser", &laser, gm2::hamar_str);
  pt_sync->Branch("ctec", &ctec, gm2::capacitec_str);
  pt_sync->Branch("flags", &flags.platform_data, gm2::sync_flags_str);
  pt_envi->Branch("envi", &envi, gm2::scs2000_str);
  pt_tilt->Branch("tilt", &tilt, gm2::tilt_sensor_str);
  pt_mlab->Branch("mlab", &mlab, gm2::metrolab_str);

  // Attach to the data files if they exist.
  if (pt_rome_laser != nullptr) {

    flags.laser_data = true;
    
    if (run_number < 787) {
      
      if (laser_point == string("P1")) {

        flags.laser_p1 = true;
    
        pt_rome_laser->SetBranchAddress("Timestamp", &laser.midas_time);
        pt_rome_laser->SetBranchAddress("position_rad", &laser.r_1);
        pt_rome_laser->SetBranchAddress("position_height", &laser.phi_1);
        pt_rome_laser->SetBranchAddress("position_azim", &laser.z_1);
        laser.r_2 = 0.0;
        laser.z_2 = 0.0;
        laser.phi_2 = 0.0;

      } else if (laser_point == string("P2")) {

        flags.laser_p2 = true;

        pt_rome_laser->SetBranchAddress("Timestamp", &laser.midas_time);
        pt_rome_laser->SetBranchAddress("position_rad", &laser.r_2);
        pt_rome_laser->SetBranchAddress("position_height", &laser.phi_2);
        pt_rome_laser->SetBranchAddress("position_azim", &laser.z_2);
        laser.r_1 = 0.0;
        laser.z_1 = 0.0;
        laser.phi_1 = 0.0;

      } else {

        pt_rome_laser->SetBranchAddress("Timestamp", &laser.midas_time);
        pt_rome_laser->SetBranchAddress("position_rad", &laser.r_1);
        pt_rome_laser->SetBranchAddress("position_height", &laser.phi_1);
        pt_rome_laser->SetBranchAddress("position_azim", &laser.z_1);
        laser.r_2 = 0.0;
        laser.z_2 = 0.0;
        laser.phi_2 = 0.0;
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

        pt_rome_laser->SetBranchAddress("Timestamp", &laser.midas_time);
        pt_rome_laser->SetBranchAddress("p1_rad", &laser.r_2);
        pt_rome_laser->SetBranchAddress("p1_phi", &laser.phi_2);
        pt_rome_laser->SetBranchAddress("p1_height", &laser.z_2);
        pt_rome_laser->SetBranchAddress("p2_rad", &laser.r_1);
        pt_rome_laser->SetBranchAddress("p2_phi", &laser.phi_1);
        pt_rome_laser->SetBranchAddress("p2_height", &laser.z_1);
   
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
  }

  if (pt_rome_ctec != nullptr) {

    flags.ctec_data = true;

    pt_rome_ctec->SetBranchAddress("Timestamp", &ctec.midas_time);
    pt_rome_ctec->SetBranchAddress("cap_0", &ctec.outer_lo);
    pt_rome_ctec->SetBranchAddress("cap_1", &ctec.inner_up);
    pt_rome_ctec->SetBranchAddress("cap_2", &ctec.inner_lo);
    pt_rome_ctec->SetBranchAddress("cap_3", &ctec.outer_up);
  }

  if (pt_rome_mlab != nullptr) {

    flags.mlab_data = true;

    pt_rome_mlab->SetBranchAddress("Timestamp", &mlab.midas_time);
    pt_rome_mlab->SetBranchAddress("Mtr1", &mlab.field);
    pt_rome_mlab->SetBranchAddress("Mtr0", &mlab.state);
    pt_rome_mlab->SetBranchAddress("Mtr2", &mlab.units);
  }

  if (pt_platform != nullptr) {
    flags.platform_data = true;
    pt_platform->SetBranchAddress("shim_platform", &platform.sys_clock[0]);
  }

  if (pt_rome_envi != nullptr) {
    flags.envi_data = true;
    pt_rome_envi->SetBranchAddress("Timestamp", &envi.midas_time);
    pt_rome_envi->SetBranchAddress("Temp0", &envi.temp[0]);
    pt_rome_envi->SetBranchAddress("Temp1", &envi.temp[1]);
    pt_rome_envi->SetBranchAddress("Temp2", &envi.temp[2]);
    pt_rome_envi->SetBranchAddress("Temp3", &envi.temp[3]);
    pt_rome_envi->SetBranchAddress("Temp4", &envi.temp[4]);
    pt_rome_envi->SetBranchAddress("Temp5", &envi.temp[5]);
    pt_rome_envi->SetBranchAddress("Temp6", &envi.temp[6]);
    pt_rome_envi->SetBranchAddress("Temp7", &envi.temp[7]);
    pt_rome_envi->SetBranchAddress("Capac0", &envi.ctec[0]);
    pt_rome_envi->SetBranchAddress("Capac1", &envi.ctec[1]);
    pt_rome_envi->SetBranchAddress("Capac2", &envi.ctec[2]);
    pt_rome_envi->SetBranchAddress("Capac3", &envi.ctec[3]);
  }
  
  if (pt_rome_tilt != nullptr) {
    flags.tilt_data = true;
    pt_rome_tilt->SetBranchAddress("Timestamp", &tilt.midas_time);
    pt_rome_tilt->SetBranchAddress("Tilt0", &tilt.temp);
    pt_rome_tilt->SetBranchAddress("Tilt1", &tilt.phi);
    pt_rome_tilt->SetBranchAddress("Tilt2", &tilt.rad);
  }

  int num_sync_events = 0;

  if (pt_platform != nullptr) {

    int nplatform = pt_platform->GetEntries();

    if (nplatform == 0) {

      cout << "Platform Data is nil." << endl;

    } else {

      num_sync_events = pt_platform->GetEntries();
      cout << "num_sync_events = " << num_sync_events << endl;
    }
  }

  if (pt_rome_laser != nullptr) {
    int nlaser = pt_rome_laser->GetEntries();

    if (nlaser == 0) {
      cout << "Laser Data is nil." << endl;

    } else if ((num_sync_events == 0) || (num_sync_events > nlaser)) {
      
      num_sync_events = pt_rome_laser->GetEntries();
      cout << "num_sync_events = " << num_sync_events << endl;
    }
  }

  if (pt_rome_ctec != nullptr) {
    int nctec = pt_rome_ctec->GetEntries();

    if (nctec == 0) {
      cout << "Capacitec Data is nil." << endl;

    } else if ((num_sync_events == 0) || (num_sync_events > nctec)) {

      num_sync_events = pt_rome_ctec->GetEntries();
      cout << "num_sync_events = " << num_sync_events << endl;
    }
  }

  for (int i = 0; i < num_sync_events; ++i) {
    
    if (pt_platform != nullptr) {
      pt_platform->GetEntry(i);
    }

    if (pt_rome_laser != nullptr) {
      pt_rome_laser->GetEntry(i);

      if (laser_point == string("P1")) {
        laser.phi_2 = laser.phi_1 + gm2::laser_phi_offset_p2_to_p1;
        // // RUN 1014 AND 1018 were labeled as pointing to P1, but actually pointing to P2
        // if (run_number == 1014 || run_number== 1018)
        //   {
        //     laser.phi_2 = laser.phi_1 ;// don't apply any offset 
        //     laser.phi_1 = laser.phi_2 - gm2::laser_phi_offset_p2_to_p1;
        //   }

        if (laser.phi_2 >= 360.0) {
          laser.phi_2 -= 360.0;
        }

      } else if (laser_point == string("P2")) {
        laser.phi_1 = laser.phi_2 - gm2::laser_phi_offset_p2_to_p1;
        

        if (laser.phi_1 <= 0.0) {
          laser.phi_1 += 360.0;
        }
      }        
    }

    if (pt_rome_ctec != nullptr) {
      pt_rome_ctec->GetEntry(i);
    }

    pt_sync->Fill();
  }

  if (pt_rome_envi != nullptr) {
    for (int i = 0; i < pt_rome_envi->GetEntries(); ++i) {
      pt_rome_envi->GetEntry(i);
      pt_envi->Fill();
    }
  }

  if (pt_rome_tilt != nullptr) {
    for (int i = 0; i < pt_rome_tilt->GetEntries(); ++i) {
      pt_rome_tilt->GetEntry(i);
      pt_tilt->Fill();
    }
  }


  if (pt_rome_mlab != nullptr) {
    for (int i = 0; i < pt_rome_mlab->GetEntries(); ++i) {
      pt_rome_mlab->GetEntry(i);
      
      if (mlab.state == 'L') {
        
        mlab.locked = true;
        
      } else {
        
        mlab.locked = false;
      }
      
      if (mlab.units == 'T') {
        
        mlab.is_tesla = true;
        
      } else {
        
        mlab.is_tesla = false;
      }
      
      pt_mlab->Fill();
    }
  }

  pf_all->Write();
  pf_all->Close();

  pf_rome_laser->Close(); 
  pf_rome_ctec->Close();
  pf_rome_envi->Close();
  pf_rome_tilt->Close();
  pf_rome_tilt->Close();
  pf_platform->Close();

  return 0;
}
