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
  hamar_t laser;
  capacitec_t ctec;
  scs2000_t envi;
  tilt_sensor_t tilt;
  int run_number;

  // ROOT stuff
  TFile *pf;
  TTree *pt_sync;
  TTree *pt_envi;
  TTree *pt_tilt;
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
  ss << "data/full_scans/full_scan_run_" << std::setfill('0');
  ss << std::setw(5) << full_scan_number << ".root";
  outfile = ss.str();

  ss.str("");
  ss << "data/full_scans/run_list_full_scan_" << std::setfill('0');
  ss << std::setw(5) << full_scan_number << ".txt";
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
  pt_sync = new TTree("t_sync", "Synchronous Shim Data");
  pt_envi = new TTree("t_envi", "Asynchronous SCS200 Data");
  pt_tilt = new TTree("t_tilt", "Asynchronous Tilt Sensor Data");

  // Attach the branches to the new output.
  pt_sync->Branch("platform", &platform, gm2::platform_str);
  pt_sync->Branch("laser", &laser, gm2::hamar_str);
  pt_sync->Branch("ctec", &ctec, gm2::capacitec_str);
  pt_sync->Branch("run_number", &run_number, "run_number/I");

  pt_envi->Branch("envi", &envi, gm2::scs2000_str);
  pt_envi->Branch("run_number", &run_number, "run_number/I");

  pt_tilt->Branch("tilt", &tilt, gm2::tilt_sensor_str);
  pt_tilt->Branch("run_number", &run_number, "run_number/I");

  for (int idx = 0; idx < dvec.size(); ++idx) {

    // Loop over the sync variables.
    for (int i = 0; i < dvec[idx].GetSyncEntries(); ++i) {
 
      platform = dvec[idx][i].platform;
      laser = dvec[idx][i].laser;
      ctec = dvec[idx][i].ctec;
      run_number = run_numbers[idx];

      pt_sync->Fill();
    }

    // And the slow control.
    for (int i = 0; i < dvec[idx].GetEnviEntries(); ++i) {
      
      envi = dvec[idx][i].envi;
      run_number = run_numbers[idx];

      pt_envi->Fill();
    }    

    for (int i = 0; i < dvec[idx].GetTiltEntries(); ++i) {
      
      tilt = dvec[idx][i].tilt;
      run_number = run_numbers[idx];

      pt_tilt->Fill();
    }    
  }

  pt_sync->Write();  
  pt_envi->Write();
  pt_tilt->Write();

  pf->Write();
  pf->Close();

  return 0;
}
