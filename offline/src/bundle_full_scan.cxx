/*****************************************************************************\

  file:    bundle_full_scan.cxx
  author:  Matthias W. Smith

  about:   Combine runs the runs that comprise a full scan 
           around the ring.

\*****************************************************************************/

//--- std includes ----------------------------------------------------------//
#include <string>
#include <ctime>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cassert>

//--- other includes --------------------------------------------------------//
#include "TFile.h"
#include "TTree.h"

//--- project includes ------------------------------------------------------//
#include "shim_dataset.hh"
#include "shim_constants.hh"

using namespace std;
using namespace gm2;

int main(int argc, char *argv[])
{
  // The data i/o
  string outfile;
  string runfile;
  string datadir;
  string str;
  stringstream ss;
  istringstream iss;
  ifstream in;
  vector<ShimDataset> dvec;
  vector<int> run_numbers;
  int full_scan_number;

  // Shim structs
  platform_t platform;
  laser_t laser;
  capacitec_t ctec;
  data_flags_t flags;
  mscb_cart_t cart;
  mscb_ring_t ring;
  tilt_sensor_t tilt;
  hall_probe_t hall;
  metrolab_t mlab;
  int run_number;

  // ROOT stuff
  TFile *pf;
  TTree *pt_sync;
  TTree *pt_cart;
  TTree *pt_ring;
  TTree *pt_tilt;
  TTree *pt_hall;
  TTree *pt_mlab;
  TTree *pt_run;

  // Parse the i/o files
  if (argc < 2) {
    cout << "Insufficent arguments, usage:" << endl;
    cout << "bundle_full_scan <full-scan-number>\n";
    exit(1);
  }

  // Set the output and runfile names based on the full scan number.
  full_scan_number = stoi(argv[1]);
  datadir = string("data/crunched/");

  ss.str("");
  ss << "data/bundles/full_scan_" << std::setfill('0');
  ss << std::setw(3) << full_scan_number << ".root";
  outfile = ss.str();

  ss.str("");
  ss << "data/bundles/run_list_full_scan_" << std::setfill('0');
  ss << std::setw(3) << full_scan_number << ".txt";
  runfile = ss.str();

  // Load the run file and parse it into run_numbers vector.
  in.open(runfile);
  std::getline(in, str);
  in.close();
  iss.str(str);

  while (std::getline(iss, str, ' ')) {
    run_numbers.push_back(stoi(str.c_str()));
  }

  for (auto &run : run_numbers) {
    ss.str("");
    ss << datadir << "run_" << std::setfill('0') << std::setw(5);
    ss << run << ".root";

    cout << "loading " << ss.str() << endl;
    dvec.push_back(ShimDataset(ss.str()));
  }

  // Create a new ROOT file.
  pf = new TFile(outfile.c_str(), "recreate");
  pt_sync = new TTree("t_sync", "Synchronous Shim Platform Data");
  pt_cart = new TTree("t_mscb_cart", "Shim Platform MSCB Data");
  pt_ring = new TTree("t_mscb_ring", "Storage Ring MSCB Data");
  pt_tilt = new TTree("t_tilt", "Tilt Sensor Data");
  pt_hall = new TTree("t_hall", "Hall Probe Data");
  pt_mlab = new TTree("t_mlab", "Metrolab Data");

  // Attach the branches to the new output.
  pt_sync->Branch("platform", &platform, gm2::platform_str);
  pt_sync->Branch("laser", &laser, gm2::laser_str);
  pt_sync->Branch("ctec", &ctec, gm2::capacitec_str);
  pt_sync->Branch("flags", &flags, gm2::data_flags_str);
  pt_sync->Branch("run_number", &run_number, "run_number/I");

  pt_cart->Branch("cart", &cart, gm2::mscb_cart_str);
  pt_cart->Branch("run_number", &run_number, "run_number/I");

  pt_ring->Branch("ring", &ring, gm2::mscb_ring_str);
  pt_ring->Branch("run_number", &run_number, "run_number/I");

  pt_tilt->Branch("tilt", &tilt, gm2::tilt_sensor_str);
  pt_tilt->Branch("run_number", &run_number, "run_number/I");

  pt_hall->Branch("hall", &hall, gm2::hall_probe_str);
  pt_hall->Branch("run_number", &run_number, "run_number/I");

  pt_mlab->Branch("mlab", &mlab, gm2::metrolab_str);
  pt_mlab->Branch("run_number", &run_number, "run_number/I");

  for (int idx = 0; idx < dvec.size(); ++idx) {

    static double phi0 = 0.0;
    static double dphi = 0.0;
      
    // If there is no laser data, we want to interpolate.
    if (idx == 0) {
      
      double phi_i = dvec[idx+1][0].laser.phi_2;
      double phi_f = dvec[idx+1][dvec[idx+1].GetSyncEntries() - 1].laser.phi_2;
      dphi = (phi_f - phi_i) / dvec[idx+1].GetSyncEntries();
      phi0 = phi_i - dvec[idx].GetSyncEntries() * dphi;
      
    } else if (idx == dvec.size() - 1) {
      
      double phi_i = dvec[idx-1][0].laser.phi_2;
      double phi_f = dvec[idx-1][dvec[idx-1].GetSyncEntries() - 1].laser.phi_2;
      dphi = (phi_f - phi_i) / dvec[idx-1].GetSyncEntries();
      phi0 = phi_f;
      
    } else {
      
      double phi_i = dvec[idx-1][dvec[idx-1].GetSyncEntries()-1].laser.phi_2;
      double phi_f = dvec[idx+1][0].laser.phi_2;
      
      dphi = (phi_f - phi_i) / dvec[idx].GetSyncEntries();
      phi0 = phi_i;
    }
    
    // Loop over the sync variables.
    for (int i = 0; i < dvec[idx].GetSyncEntries(); ++i) {
 
      platform = dvec[idx][i].platform;
      laser = dvec[idx][i].laser;
      ctec = dvec[idx][i].ctec;
      flags = dvec[idx][i].flags;
      run_number = run_numbers[idx];

      if (!(dvec[idx][0].flags.laser_p1 || dvec[idx][0].flags.laser_p2)) {
        laser.phi_2 = phi0 + i * dphi;
        laser.phi_1 = phi0 + i * dphi - gm2::laser_phi_offset;
      }

      pt_sync->Fill();
    }

    // And the cart slow control.
    for (int i = 0; i < dvec[idx].GetCartEntries(); ++i) {
      
      cart = dvec[idx][i].cart;
      run_number = run_numbers[idx];

      pt_cart->Fill();
    }    

    // And the ring slow control.
    for (int i = 0; i < dvec[idx].GetRingEntries(); ++i) {
      
      ring = dvec[idx][i].ring;
      run_number = run_numbers[idx];

      pt_ring->Fill();
    }    

    for (int i = 0; i < dvec[idx].GetTiltEntries(); ++i) {
      
      tilt = dvec[idx][i].tilt;
      run_number = run_numbers[idx];

      pt_tilt->Fill();
    }    

    for (int i = 0; i < dvec[idx].GetHallEntries(); ++i) {
      
      hall = dvec[idx][i].hall;
      run_number = run_numbers[idx];

      pt_hall->Fill();
    }    

    for (int i = 0; i < dvec[idx].GetMlabEntries(); ++i) {
      
      mlab = dvec[idx][i].mlab;
      run_number = run_numbers[idx];

      pt_mlab->Fill();
    }    

    printf("Done bundling run %05i\n", run_number);;
  }

  pt_sync->Write();  
  pt_cart->Write();
  pt_ring->Write();
  pt_tilt->Write();
  pt_hall->Write();
  pt_mlab->Write();

  pf->Write();
  pf->Close();

  return 0;
}
