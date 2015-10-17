/*===========================================================================*\

author: Matthias W. Smith
email:  mwsmith2@uw.edu
date:   2015/09/04

about: The program runs on a shim data in a specific format

usage:

./generate_basic_plots <input-file> [output-dir]

\*===========================================================================*/

//--- std includes ----------------------------------------------------------//
#include <iostream>
#include <vector>
#include <array>
#include <cassert>
#include <cmath>

//--- other includes --------------------------------------------------------//
#include <boost/filesystem.hpp>
#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TGraphErrors.h"
#include "TGraph2D.h"
#include "TStyle.h"

//--- project includes ------------------------------------------------------//
#include "fid.h"
#include "shim_structs.hh"

using namespace std;
using namespace gm2;

int main(int argc, char **argv)
{
  // Allocate parameters
  const double dt = 10.0 / SHORT_FID_LN;
  const double t0 = 0.0;
  double platform_coord[SHIM_PLATFORM_CH][2];

  std::vector<double> wf(SHORT_FID_LN, 0.0);
  std::vector<double> tm(SHORT_FID_LN, 0.0);
  std::vector<std::array<double, SHIM_PLATFORM_CH>> platform_freqs_ph;
  std::vector<std::array<double, SHIM_PLATFORM_CH>> platform_freqs_zc;

  string datafile;
  string outfile;
  string outdir;

  // ROOT variables.
  TFile *pf_in;
  TTree *pt_sync;
  TTree *pt_envi;

  TFile *pf_out;
  TTree *pt_out;

  // Declare ROOT variables.
  TCanvas c1;

  assert (argc > 1);
  datafile = string(argv[1]);

  // Set output directory.
  if (argc > 2) {

    outdir = string(argv[2]);

  } else {

    outdir = string("data/recrunched/");
    auto fname = boost::filesystem::path(datafile).filename().string();
    outfile = outdir + fname;
  }

  // Make sure it is a root file.
  assert (datafile.find_last_of(".root") == datafile.size() - 1);
  pf_in = new TFile(datafile.c_str(), "read");

  pt_sync = (TTree *)pf_in->Get("t_sync");
  pt_envi = (TTree *)pf_in->Get("t_envi");

  // Attach the appropriate branches/leaves.
  platform_t idata;
  hamar_t laser;
  capacitec_t ctec;

  pt_sync->SetBranchAddress("platform", &idata.sys_clock[0]);
  pt_sync->SetBranchAddress("laser", &laser.midas_time);
  pt_sync->SetBranchAddress("ctec", &ctec.midas_time);

  pf_out = new TFile(outfile.c_str(), "recreate");
  platform_t odata;

  pt_out = new TTree("t_sync", "Synchronous Shimming Data");
  pt_out->Branch("platform", &odata.sys_clock[0], platform_str);
  pt_out->Branch("laser", &laser.midas_time, hamar_str);
  pt_out->Branch("ctec", &ctec.midas_time, capacitec_str);

  // Set the time vector.
  for (int i = 0; i < SHORT_FID_LN; ++i) {
    tm[i] = i * dt + t0;
  }

  // Go through the data.
  for (int idx = 0; idx < pt_sync->GetEntries(); ++idx) {

    pt_sync->GetEntry(idx);

    // Copy the old data.
    odata = idata;    

    for (int ch = 0; ch < SHIM_PLATFORM_CH; ++ch) {

      // Re-analyze all the FIDs.

      for (int i = 0; i < SHORT_FID_LN; ++i) {
        wf[i] = idata.trace[ch][i];
      }

      fid::FID myfid(wf, tm);

      // Make sure we've allocated space for the frequency array.

      // Make sure we got an FID signal
      if (myfid.isgood()) {
        
        odata.snr[ch] = myfid.snr();
        odata.len[ch] = myfid.fid_time();
        odata.freq[ch] = myfid.CalcPhaseFreq();
        odata.ferr[ch] = myfid.freq_err();
        odata.method[ch] = (ushort)fid::Method::PH;
        odata.health[ch] = myfid.isgood();
        odata.freq_zc[ch] = myfid.CalcZeroCountFreq();
        odata.ferr_zc[ch] = myfid.freq_err();
        
      } else {
       
        odata.snr[ch] = 0.0;
        odata.len[ch] = 0.0;
        odata.freq[ch] = 0.0;
        odata.ferr[ch] = 0.0;
        odata.method[ch] = (ushort)fid::Method::PH;
        odata.health[ch] = myfid.isgood();
        odata.freq_zc[ch] = 0.0;
        odata.ferr_zc[ch] = 0.0;
      }

      cout << endl << "Results for entry " << idx << ", " << ch << endl;
      cout << myfid.GetFreq("PH") << ", " << myfid.GetFreq("ZC") << endl;
      myfid.PrintDiagnosticInfo();
    }

    pt_out->Fill();    
  }

  TTree *pt = pt_envi->CloneTree();
  pt->Write();

  pf_out->Write();
  pf_out->Close();
  pf_in->Close();

  return 0;
}
