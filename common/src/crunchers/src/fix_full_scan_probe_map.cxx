/********************************************************************\

  file:    fix_full_scan_probe_map.cxx
  author:  Matthias W. Smith

  about:   Combine runs that comprise a full scan around the ring,
           and corrects for the probe mapping.

\********************************************************************/

//--- std includes -------------------------------------------------//
#include <string>
#include <ctime>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cassert>

//--- other includes -----------------------------------------------//
#include "TFile.h"
#include "TTree.h"

//--- project includes ---------------------------------------------//
#include "shim_dataset.hh"

using namespace std;
using namespace gm2;

void copy_platform_ch(platform_t& pa, int a, platform_t& pb, int b);

int main(int argc, char *argv[])
{
  // The data i/o
  string outfile;
  string datadir;
  stringstream ss;
  vector<ShimDataset> dvec;
  vector<int> run_numbers;

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
  TTree *pt_mlab;
  TTree *pt_hall;
  TTree *pt_run;

  // Parse the i/o files
  if (argc < 4) {
    cout << "Insufficent arguments, usage:" << endl;
    cout << "bundle_full_scan <output-file> <input-data-dir> [run list]\n";
    exit(1);
  }

  outfile = string(argv[1]);
  datadir = string(argv[2]);

  if (datadir[datadir.size() - 1] != '/') {
    datadir += string("/");
  }

  for (int i = 3; i < argc; ++i) {
    ss.str("");
    ss << datadir << "full_scan_" << std::setfill('0') << std::setw(3);
    ss << stoi(argv[i]) << ".root";

    cout << "loading " << ss.str() << endl;
    run_numbers.push_back(stoi(argv[i]));
    dvec.push_back(ShimDataset(ss.str()));
  }

  // Create a new ROOT file.
  pf = new TFile(outfile.c_str(), "recreate");
  pt_sync = new TTree("t_sync", "Synchronous Shim Data");
  pt_cart = new TTree("t_cart", "Asynchronous SCS200 Data");
  pt_ring = new TTree("t_ring", "Asynchronous SCS200 Data");
  pt_tilt = new TTree("t_tilt", "Asynchronous Tilt Sensor Data");
  pt_hall = new TTree("t_hall", "Asynchronous Tilt Sensor Data");
  pt_mlab = new TTree("t_mlab", "Asynchronous Metrolab Sensor Data");

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

  platform_t p_blank;

  for (int idx = 0; idx < dvec.size(); ++idx) {

    // Loop over the sync variables.
    for (int i = 0; i < dvec[idx].GetSyncEntries(); ++i) {

      platform = dvec[idx][i].platform; 
      platform_t p = dvec[idx][i].platform; 

      for (int j = 0; j < SHIM_PLATFORM_CH; ++j) {

        switch (j) 
          {
          case 2:
            copy_platform_ch(platform, j, p, 8);
            break;
          case 3:
            copy_platform_ch(platform, j, p, 20);
            break;
          case 6:
            copy_platform_ch(platform, j, p, 3);
            break;
          case 7:
            copy_platform_ch(platform, j, p, 10);
            break;
          case 8:
            copy_platform_ch(platform, j, p, 18);
            break;
          case 10:
            copy_platform_ch(platform, j, p, 14);
            break;
          case 14:
            copy_platform_ch(platform, j, p, 6);
            break;
          case 17:
            copy_platform_ch(platform, j, p, 7);
            break;
          case 18:
            copy_platform_ch(platform, j, p_blank, 0);
            break;
          case 20:
            copy_platform_ch(platform, j, p, 17);
            break;
          default:
            break;
          }
      }
      
      laser = dvec[idx][i].laser;
      ctec = dvec[idx][i].ctec;
      flags = dvec[idx][i].flags;
      run_number = run_numbers[idx];

      pt_sync->Fill();
    }

    // And the slow control.
    for (int i = 0; i < dvec[idx].GetCartEntries(); ++i) {
      
      cart = dvec[idx][i].cart;
      run_number = run_numbers[idx];

      pt_cart->Fill();
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
  }

  pt_sync->Write();  
  pt_mlab->Write();
  pt_hall->Write();
  pt_tilt->Write();
  pt_cart->Write();
  pt_ring->Write();

  pf->Write();
  pf->Close();

  return 0;
}

void copy_platform_ch(platform_t& pa, int a, platform_t& pb, int b) 
{
  pa.sys_clock[a] = pb.sys_clock[b];
  pa.gps_clock[a] = pb.gps_clock[b];
  pa.dev_clock[a] = pb.dev_clock[b];
  pa.snr[a] = pb.snr[b];
  pa.len[a] = pb.len[b];
  pa.freq[a] = pb.freq[b];
  pa.ferr[a] = pb.ferr[b];
  pa.freq_zc[a] = pb.freq_zc[b];
  pa.ferr_zc[a] = pb.ferr_zc[b];
  pa.health[a] = pb.health[b];
  pa.method[a] = pb.method[b];
  std::copy(&pb.trace[b][0], &pb.trace[b][SAVE_FID_LN], &pa.trace[a][0]);
}
