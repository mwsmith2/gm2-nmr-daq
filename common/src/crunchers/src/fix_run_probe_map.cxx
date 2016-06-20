/********************************************************************\

  file:    fix_run_probe_map.cxx
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
  int run_number;

  // shim structs
  platform_t platform;
  laser_t laser;
  capacitec_t ctec;
  mscb_cart_t cart;
  mscb_ring_t ring;
  tilt_sensor_t tilt;
  hall_probe_t hall;
  metrolab_t mlab;
  data_flags_t flags;

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
  if (argc != 4) {
    cout << "Insufficent arguments, usage:" << endl;
    cout << "fix_run_probe_map <output-file> <input-data-dir> <run-number>\n";
    exit(1);
  }

  outfile = string(argv[1]);
  datadir = string(argv[2]);

  if (datadir[datadir.size() - 1] != '/') {
    datadir += string("/");
  }

  ss.str("");
  ss << datadir << "run_" << std::setfill('0') << std::setw(5);
  ss << stoi(argv[3]) << ".root";

  ShimDataset d(ss.str());

  cout << "loading " << ss.str() << endl;
  run_number = stoi(argv[3]);
  
  // Create a new ROOT file.
  pf = new TFile(outfile.c_str(), "recreate");
  pt_sync = new TTree("t_sync", "Synchronous Shim Data");
  pt_cart = new TTree("t_cart", "Asynchronous SCS200 Data");
  pt_ring = new TTree("t_ring", "Asynchronous SCS200 Data");
  pt_tilt = new TTree("t_tilt", "Asynchronous Tilt Sensor Data");
  pt_hall = new TTree("t_hall", "Asynchronous Hall Probe Platform Data");
  pt_mlab = new TTree("t_mlab", "Asynchronous Metrolab Data");

  // Attach the branches to the new output.
  pt_sync->Branch("platform", &platform, gm2::platform_str);
  pt_sync->Branch("laser", &laser, gm2::laser_str);
  pt_sync->Branch("ctec", &ctec, gm2::capacitec_str);
  pt_sync->Branch("flags", &flags, gm2::data_flags_str);
  pt_cart->Branch("cart", &cart, gm2::mscb_cart_str);
  pt_ring->Branch("ring", &ring, gm2::mscb_ring_str);
  pt_tilt->Branch("tilt", &tilt, gm2::tilt_sensor_str);
  pt_hall->Branch("hall", &hall, gm2::hall_probe_str);
  pt_mlab->Branch("mlab", &mlab, gm2::metrolab_str);

  platform_t p_blank; // Used to substitute probe index 18
  p_blank.health[0] = 0;

  // Loop over the sync variables.
  for (int i = 0; i < d.GetSyncEntries(); ++i) {

    if (run_number < 1475) {

      platform = d[i].platform; 
      platform_t p = d[i].platform; 

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
    } else if (run_number == 1654) {
      
      // This run had an off by one problem in the sequencing,
      // so I'm hardcoding the permutation back.

      cout << "Remapping sequence for run 1654" << endl;

      platform = d[i].platform; 
      platform_t p = d[i].platform; 

      // copy_platform_ch(platform, 0, p, 4);
      // copy_platform_ch(platform, 1, p, 16);
      // copy_platform_ch(platform, 2, p, 1);
      // copy_platform_ch(platform, 3, p, 2);
      // copy_platform_ch(platform, 4, p, 3);
      // copy_platform_ch(platform, 5, p, 24);
      // copy_platform_ch(platform, 6, p, 5);
      // copy_platform_ch(platform, 7, p, 6);
      // copy_platform_ch(platform, 8, p, 7);
      // copy_platform_ch(platform, 9, p, 27);
      // copy_platform_ch(platform, 10, p, 9);
      // copy_platform_ch(platform, 11, p, 10);
      // copy_platform_ch(platform, 12, p, 11);
      // copy_platform_ch(platform, 13, p, 12);
      // copy_platform_ch(platform, 14, p, 13);
      // copy_platform_ch(platform, 15, p, 14);
      // copy_platform_ch(platform, 16, p, 15);
      // copy_platform_ch(platform, 17, p, 26);
      // copy_platform_ch(platform, 18, p, 17);
      // copy_platform_ch(platform, 19, p, 18);
      // copy_platform_ch(platform, 20, p, 19);
      // copy_platform_ch(platform, 21, p, 20);
      // copy_platform_ch(platform, 22, p, 21);
      // copy_platform_ch(platform, 23, p, 22);
      // copy_platform_ch(platform, 24, p, 23);
      // copy_platform_ch(platform, 25, p, 8);
      // copy_platform_ch(platform, 26, p, 25);
      // copy_platform_ch(platform, 27, p, 0);

      copy_platform_ch(platform, 0, p, 27);
      copy_platform_ch(platform, 1, p, 2);
      copy_platform_ch(platform, 2, p, 3);
      copy_platform_ch(platform, 3, p, 4);
      copy_platform_ch(platform, 4, p, 0);
      copy_platform_ch(platform, 5, p, 6);
      copy_platform_ch(platform, 6, p, 7);
      copy_platform_ch(platform, 7, p, 8);
      copy_platform_ch(platform, 8, p, 25);
      copy_platform_ch(platform, 9, p, 10);
      copy_platform_ch(platform, 10, p, 11);
      copy_platform_ch(platform, 11, p, 12);
      copy_platform_ch(platform, 12, p, 13);
      copy_platform_ch(platform, 13, p, 14);
      copy_platform_ch(platform, 14, p, 15);
      copy_platform_ch(platform, 15, p, 16);
      copy_platform_ch(platform, 16, p, 1);
      copy_platform_ch(platform, 17, p, 18);
      copy_platform_ch(platform, 18, p, 19);
      copy_platform_ch(platform, 19, p, 20);
      copy_platform_ch(platform, 20, p, 21);
      copy_platform_ch(platform, 21, p, 22);
      copy_platform_ch(platform, 22, p, 23);
      copy_platform_ch(platform, 23, p, 24);
      copy_platform_ch(platform, 24, p, 5);
      copy_platform_ch(platform, 25, p, 26);
      copy_platform_ch(platform, 26, p, 17);
      copy_platform_ch(platform, 27, p, 9);

    } else {

      // Simply copy the data.
      platform = d[i].platform; 
    }

    laser = d[i].laser;
    ctec = d[i].ctec;
    flags = d[i].flags;
    pt_sync->Fill();
  }

  // And the slow control.
  for (int i = 0; i < d.GetCartEntries(); ++i) {
      
    cart = d[i].cart;
    pt_cart->Fill();
  }    

  // And the slow control.
  for (int i = 0; i < d.GetRingEntries(); ++i) {
      
    ring = d[i].ring;
    pt_ring->Fill();
  }    

  for (int i = 0; i < d.GetTiltEntries(); ++i) {
      
    tilt = d[i].tilt;
    pt_tilt->Fill();
  }    

  for (int i = 0; i < d.GetHallEntries(); ++i) {
      
    hall = d[i].hall;
    pt_hall->Fill();
  }    

  for (int i = 0; i < d.GetMlabEntries(); ++i) {
      
    mlab = d[i].mlab;
    pt_mlab->Fill();
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
