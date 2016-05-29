/*---------------------------------------------------------------------------*\
file:   an_online_monitor.cxx
author: Matthias W. Smith
email:  mwsmith2@uw.edu

about:  Performs FFTs and plots those as well waveforms for simple
        digitizer input, SIS3302 and SHIM_FIXED modules.  It also does some
        work archiving configuration, runlogs, and merging ROOT output.

\*---------------------------------------------------------------------------*/

//-- std includes ------------------------------------------------------------//
#include <stdio.h>
#include <time.h>
#include <cstring>
#include <iostream>

//--- other includes ---------------------------------------------------------//
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TROOT.h"
#include "TStyle.h"

#define USE_ROOT 1
#include "midas.h"

//--- project includes -------------------------------------------------------//
#include "experim.h"
#include "fid.h"
#include "nmr/common.hh"

//--- globals ----------------------------------------------------------------//

// The analyzer name (client name) as seen by other MIDAS clients   
char *analyzer_name = (char *)"online-monitor"; // MWS set

// analyzer_loop is called with this interval in ms (0 to disable)  
INT analyzer_loop_period = 500;

// default ODB size 
INT odb_size = DEFAULT_ODB_SIZE;

// ODB structures 
RUNINFO runinfo;

int analyze_trigger_event(EVENT_HEADER * pheader, void *pevent);
int analyze_scaler_event(EVENT_HEADER * pheader, void *pevent);

INT analyzer_init(void);
INT analyzer_exit(void);
INT analyzer_loop(void);
INT ana_begin_of_run(INT run_number, char *error);
INT ana_end_of_run(INT run_number, char *error);
INT ana_pause_run(INT run_number, char *error);
INT ana_resume_run(INT run_number, char *error);

// My stop hook.
INT tr_stop_hook(INT run_number, char *error);

//-- Bank list --------------------------------------------------------------//

// We anticipate three SIS3302 digitizers, but only use one for now.
BANK_LIST trigger_bank_list[] = {
  {"SHPF", TID_DWORD, sizeof(daq::shim_platform_st), NULL},
  {"LTRK", TID_DOUBLE, 6, NULL},
  {""}
};

//-- Event request list -----------------------------------------------------//

ANALYZE_REQUEST analyze_request[] = {
  {"online-monitor",                // equipment name 
   {10,                         // event ID 
    TRIGGER_ALL,               // trigger mask 
    GET_NONBLOCKING,           // get some events 
    "BUF1",                  // event buffer 
    TRUE,                      // enabled 
    "", "",},
   analyze_trigger_event,      // analyzer routine 
   NULL,                       // module list 
   trigger_bank_list,          // list 
   10000,                      // RWNT buffer size 
  },
  
  {""}
};

namespace {

// Histograms for a subset of MIDAS banks.
std::string figdir;
bool write_root;
daq::shim_platform_st platform_data;
daq::shim_fixed_st fixed_data;
std::atomic<bool> new_run_to_process;
std::atomic<bool> new_config_to_archive;
std::atomic<bool> new_platform_waveforms;
std::atomic<bool> new_fixed_waveforms;
std::atomic<bool> time_for_breakdown;
std::atomic<int> atomic_run_number;
std::thread merge_root_thread;
std::thread plot_waveforms_thread;
std::thread archive_config_thread;
}

void merge_data_loop();
void plot_waveforms_loop();
void archive_config_loop();

//-- Analyzer Init ---------------------------------------------------------//

INT analyzer_init()
{
  HNDLE hDB, hkey;
  char str[256];
  int i, size;

  // Set up data merging thread.
  ::new_run_to_process = false;
  ::new_config_to_archive = false;
  ::new_platform_waveforms = false;
  ::new_fixed_waveforms = false;
  ::time_for_breakdown = false;
  ::atomic_run_number = 0;
  ::merge_root_thread = std::thread(merge_data_loop);
  ::archive_config_thread = std::thread(archive_config_loop);
  ::plot_waveforms_thread = std::thread(plot_waveforms_loop);

  // Register my own stop hook.
  cm_register_transition(TR_STOP, tr_stop_hook, 900);  

  // following code opens ODB structures to make them accessible
  // from the analyzer code as C structures 
  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, "/Params/html-dir", &hkey);
  
  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
    if (str[strlen(str) - 1] != DIR_SEPARATOR) {
      strcat(str, DIR_SEPARATOR_STR);
    }
  }
  
  // Filepaths are too long, so moving to /tmp
  //  figdir = std::string(str) + std::string("static/");
  figdir = std::string("/tmp");
  gROOT->SetBatch(true);
  gStyle->SetOptStat(false);

  // Check if we need to put the images in.
  char keyname[] = "Custom/Images/shim_platform_ch03_fft.gif/Background";
  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, keyname, &hkey);

  if (hkey) {
    return SUCCESS;
  }
   
  //---- user code to book histos ------------------------------------------//
  for (i = 0; i < SHIM_FIXED_CH; i++) {
    char title[32], name[32], key[64], figpath[64];

    // Now add to the odb
    sprintf(name, "shim_fixed_ch%02i_wf", i);
    sprintf(key, "/Custom/Images/%s.gif/Background", name);
    sprintf(figpath, "%s/%s.gif", figdir.c_str(), name);
    db_create_key(hDB, 0, key, TID_STRING);
    db_set_value(hDB, 0, key, figpath, sizeof(figpath), 1, TID_STRING);

    // Now add to the odb
    sprintf(name, "shim_fixed_ch%02i_fft", i);
    sprintf(key, "/Custom/Images/%s.gif/Background", name);
    sprintf(figpath, "%s/%s.gif", figdir.c_str(), name);
    db_create_key(hDB, 0, key, TID_STRING);
    db_set_value(hDB, 0, key, figpath, sizeof(figpath), 1, TID_STRING);
  }

  for (i = 0; i < SHIM_PLATFORM_CH; i++) {
    char title[32], name[32], key[64], figpath[64];

    // Now add to the odb
    sprintf(name, "shim_platform_ch%02i_wf", i);
    sprintf(key, "/Custom/Images/%s.gif/Background", name);
    sprintf(figpath, "%s/%s.gif", figdir.c_str(), name);
    db_create_key(hDB, 0, key, TID_STRING);
    db_set_value(hDB, 0, key, figpath, sizeof(figpath), 1, TID_STRING);

    // Now add to the odb
    sprintf(name, "shim_platform_ch%02i_fft", i);
    sprintf(key, "/Custom/Images/%s.gif/Background", name);
    sprintf(figpath, "%s/%s.gif", figdir.c_str(), name);
    db_create_key(hDB, 0, key, TID_STRING);
    db_set_value(hDB, 0, key, figpath, sizeof(figpath), 1, TID_STRING);
  }

  return SUCCESS;
}

//-- Analyzer Exit ---------------------------------------------------------//

INT analyzer_exit()
{
  time_for_breakdown = true;

  while(!merge_root_thread.joinable());
  
  merge_root_thread.join();

  while(!plot_waveforms_thread.joinable());
  
  plot_waveforms_thread.join();

  while(!archive_config_thread.joinable());
  
  archive_config_thread.join();

  return CM_SUCCESS;
}

//-- Begin of Run ----------------------------------------------------------//

INT ana_begin_of_run(INT run_number, char *error)
{
  return CM_SUCCESS;
}

//-- End of Run ------------------------------------------------------------//

INT ana_end_of_run(INT run_number, char *error)
{
  return CM_SUCCESS;
}

//-- Pause Run -------------------------------------------------------------//


INT ana_pause_run(INT run_number, char *error)
{
  return CM_SUCCESS;
}

//-- Resume Run ------------------------------------------------------------//

INT ana_resume_run(INT run_number, char *error)
{
  return CM_SUCCESS;
}

//-- Analyzer Loop ---------------------------------------------------------//

INT analyzer_loop()
{
  return CM_SUCCESS;
}

//-- Analyze Events --------------------------------------------------------//

INT analyze_trigger_event(EVENT_HEADER * pheader, void *pevent)
{
  cm_msg(MINFO, analyzer_name, "analyzing event");

  // We need these for each FID, so keep them allocated.
  HNDLE hDB, hkey;
  unsigned int ch, idx;
  DWORD *pvme;
  float *pfreq;
  double *plaser;
  static double prev_phi = -1;
  static double prev_dipole = -1;
  static double this_phi = -1;
  static double this_dipole = -1;

  // Look for the first SHIM_PLATFORM traces.
  if (bk_locate(pevent, "SHPF", &pvme) != 0) {

    platform_data = *reinterpret_cast<daq::shim_platform_st *>(pvme);
    
    cm_get_experiment_database(&hDB, NULL);
    
    // First set the frequencies
    db_find_key(hDB, 0, "/Custom/Data/shim-platform/freq", &hkey);

    if (hkey) {
      db_set_data(hDB, hkey,
                  platform_data.freq, 
                  sizeof(platform_data.freq), 
                  SHIM_PLATFORM_CH, 
                  TID_DOUBLE);
    }

    // Now the probe health.
    db_find_key(hDB, 0, "/Custom/Data/shim-platform/health", &hkey);

    if (hkey) {
      db_set_data(hDB, hkey,
                  platform_data.freq,
                  sizeof(platform_data.health), 
                  SHIM_PLATFORM_CH, 
                  TID_WORD);
    }

    // Give the event a timestamp.
    db_find_key(hDB, 0, "/Custom/Data/shim-platform/timestamp", &hkey);

    if (hkey) {
      auto time = platform_data.sys_clock[0];
      db_set_data(hDB, hkey, &time, sizeof(time), 1, TID_DOUBLE); 
    }

    // And increment the monotonic event ref.
    db_find_key(hDB, 0, "/Custom/Data/shim-platform/event-id", &hkey);

    if (hkey) {
      double id;
      int size = sizeof(id);
      db_get_data(hDB, hkey, &id, &size, TID_DOUBLE); 

      id += 1;
      db_set_data(hDB, hkey, &id, sizeof(id), 1, TID_DOUBLE); 
    }
    
    if (bk_locate(pevent, "LTRK", &plaser) != 0) {
      printf("Found laser\n");
      this_phi = plaser[4];

      // Calculate the average field.
      this_dipole = 0.0;
      for (int idx = 0; idx < 25; ++idx) {
        this_dipole += pfreq[idx];
      }
      this_dipole /= 25.0;

      if (prev_phi > this_phi) {
        prev_phi -= 360.0;
      }

      printf("this_phi = %.2f, prev_phi = %.2f\n", this_phi, prev_phi);

      // Fill the values in the ODB if we can interpolate a point.
      if (this_phi - prev_phi < 0.5) {

        // Interpolate a new value.
        double new_phi = int(2 * this_phi) / 2.0;
        double w1 = (new_phi - prev_phi) / (this_phi - prev_phi);
        double w2 = (this_phi - new_phi) / (this_phi - prev_phi);
        double new_dipole = w1 * prev_dipole + w2 * this_dipole;

        double tmp_dipole = 0.0;
        int size;

        // Get and replace the old value.
        db_find_key(hDB, 0, "/Custom/Data/field-data/this-dipole", &hkey);
        db_get_data(hDB, hkey, &tmp_dipole, &size, TID_DOUBLE);
        db_set_data(hDB, hkey, &new_dipole, sizeof(new_dipole), 1, TID_DOUBLE);

        db_find_key(hDB, 0, "/Custom/Data/field-data/prev-dipole", &hkey);
        db_set_data(hDB, hkey, &tmp_dipole, sizeof(tmp_dipole), 1, TID_DOUBLE);
      }

      prev_phi = this_phi;
      prev_dipole = this_dipole;
    }

    // Now let the plot loop know.
    new_platform_waveforms = true;
  }

  if (bk_locate(pevent, "SHFX", &pvme) != 0) {

    fixed_data = *reinterpret_cast<daq::shim_fixed_st *>(pvme);

    new_fixed_waveforms = true;
  }


  return CM_SUCCESS;
}

void plot_waveforms_loop()
{
  // We need these for each FID, so keep them allocated.
  static char title[32], name[32];
  static std::vector<double> wf;
  static std::vector<double> tm;
  static TCanvas c1("c1", "Online Monitor", 360, 270);
  static TH1F *ph_wfm = nullptr;
  static TH1F *ph_fft = nullptr;

  unsigned int ch, idx;
  float *pfreq;

  // Look for the first SHIM_PLATFORM traces.
  while (!time_for_breakdown) {

    if (new_platform_waveforms) {
      
      cm_msg(MINFO, "online_analyzer", "Processing a shim_platform event.");
      
      wf.resize(SHORT_FID_LN);
      tm.resize(SHORT_FID_LN);
      
      // Set up the time vector.
      for (idx = 0; idx < SHORT_FID_LN; idx++){
        tm[idx] = idx * 0.001;  // @1 MHz, t = [0ms, 10ms]
      }

      // Copy and analyze each channel's FID separately.
      for (ch = 0; ch < SHIM_PLATFORM_CH; ++ch) {

        std::copy(&platform_data.trace[ch][0], 
                  &platform_data.trace[ch + 1][0], 
                  wf.begin());

        fid::Fid myfid(wf, tm);
        
        sprintf(name, "shim_platform_ch%02i_wf", ch);
        sprintf(title, "Channel %i Trace", ch);
        ph_wfm = new TH1F(name, title, SHORT_FID_LN, myfid.tm()[0], 
                          myfid.tm()[myfid.tm().size() - 1]);

        // Make sure ROOT lets us delete completely.
        ph_wfm->SetDirectory(0);
        
        sprintf(name, "shim_platform_ch%02i_fft", ch);
        sprintf(title, "Channel %i Fourier Transform", ch);
        ph_fft = new TH1F(name, title, SHORT_FID_LN, myfid.fftfreq()[0], 
                          myfid.fftfreq()[myfid.fftfreq().size() - 1]);

        // Make sure ROOT lets us delete completely.
        ph_fft->SetDirectory(0);
        
        // One histogram gets the waveform and another with the fft power.
        for (idx = 0; idx < myfid.psd().size(); ++idx){
          ph_wfm->SetBinContent(idx, myfid.wf()[idx]);
          ph_fft->SetBinContent(idx, myfid.psd()[idx]);
        }
        
        // The waveform has more samples.
        for (; idx < SHORT_FID_LN; ++idx) {
          ph_wfm->SetBinContent(idx, myfid.wf()[idx]);
        }
        
        c1.Clear();
        c1.SetLogx(0);
        c1.SetLogy(0);
        ph_wfm->Draw();
        c1.Print(TString::Format("%s/%s.gif", 
                                 figdir.c_str(), 
                                 ph_wfm->GetName()));
        c1.Clear();
        c1.SetLogx(1);
        c1.SetLogy(1);
        ph_fft->Draw();
        c1.Print(TString::Format("%s/%s.gif", 
                                 figdir.c_str(), 
                                 ph_fft->GetName()));
        
        if (ph_wfm != nullptr) {
          delete ph_wfm;
        }
        
        if (ph_fft != nullptr) {
          delete ph_fft;
        }
      }

      new_platform_waveforms = false;
    }
    
    if (new_fixed_waveforms) {
      // Look for the first SHIM_FIXED traces.
      cm_msg(MINFO, "online_analyzer", "Processing a shim_fixed event.");
      
      wf.resize(SHORT_FID_LN);
      tm.resize(SHORT_FID_LN);
      
      // Set up the time vector.
      for (idx = 0; idx < SHORT_FID_LN; idx++){
        tm[idx] = idx * 0.0001;  // @10 MHz, t = [0ms, 10ms]
      }
      
      // Copy and analyze each channel's FID separately.
      for (ch = 0; ch < SHIM_FIXED_CH; ++ch) {
        
        std::copy(&fixed_data.trace[ch][0], 
                  &fixed_data.trace[ch + 1][0],
                  wf.begin());
        
        fid::Fid myfid(wf, tm);
        
        sprintf(name, "shim_fixed_ch%02i_wf", ch);
        sprintf(title, "NMR Probe %i Trace", ch + 1);
        ph_wfm = new TH1F(name, title, SHORT_FID_LN, myfid.tm()[0], 
                          myfid.tm()[myfid.tm().size() - 1]);

        // Make sure ROOT lets us delete completely.
        ph_wfm->SetDirectory(0);
        
        sprintf(name, "shim_fixed_ch%02i_fft", ch);
        sprintf(title, "NMR Probe %i Fourier Transform", ch + 1);
        ph_fft = new TH1F(name, title, SHORT_FID_LN, myfid.fftfreq()[0], 
                          myfid.fftfreq()[myfid.fftfreq().size() - 1]);
        
        // Make sure ROOT lets us delete completely.
        ph_fft->SetDirectory(0);
        
        // One histogram gets the waveform and another with the fft power.
        for (idx = 0; idx < myfid.psd().size(); ++idx){
          ph_wfm->SetBinContent(idx, myfid.wf()[idx]);
          ph_fft->SetBinContent(idx, myfid.psd()[idx]);
        }
        
        // The waveform has more samples.
        for (; idx < SHORT_FID_LN; ++idx) {
          ph_wfm->SetBinContent(idx, myfid.wf()[idx]);
        }
        
        c1.Clear();
        c1.SetLogx(0);
        c1.SetLogy(0);
        ph_wfm->Draw();
        c1.Print(TString::Format("%s/%s.gif", figdir.c_str(), ph_wfm->GetName()));
        
        c1.Clear();
        c1.SetLogx(1);
        c1.SetLogy(1);
        ph_fft->Draw();
        c1.Print(TString::Format("%s/%s.gif", figdir.c_str(), ph_fft->GetName()));
        
        if (ph_wfm != nullptr) {
          delete ph_wfm;
        }
        
        if (ph_fft != nullptr) {
          delete ph_fft;
        }
      }

      new_fixed_waveforms = false;
    }

    usleep(50000);
  }
}


//-- Run Control Hooks -----------------------------------------------------//
INT tr_stop_hook(INT run_number, char *error) 
{
  // Set the data merger info.
  atomic_run_number = run_number;
  new_run_to_process = true;
  new_config_to_archive = true;
  
  return CM_SUCCESS;
}


//-- Run Control Hooks -----------------------------------------------------//
void merge_data_loop()
{
  //DATA part
  HNDLE hDB, hkey;
  INT status;
  char str[256], filename[256];
  int size, run_number;

  TFile *pf_shim_platform;
  TFile *pf_shim_fixed;
  TFile *pf_final;

  TTree *pt_old_shim_platform;
  TTree *pt_old_shim_fixed;
  TTree *pt_shim_platform;
  TTree *pt_shim_fixed;
    
  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, "/Logger/Data dir", &hkey);
    
  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
    if (str[strlen(str) - 1] != DIR_SEPARATOR) {
      strcat(str, DIR_SEPARATOR_STR);
    }
  }

  while (!time_for_breakdown) {
    
    run_number = atomic_run_number;

    if (new_run_to_process && write_root) {
      // Open all the files.
      sprintf(filename, "%sfe_shim_platform_run_%05i.root", str, run_number);
      pf_shim_platform = new TFile(filename);
      if (!pf_shim_platform->IsZombie()) {
        pt_old_shim_platform = (TTree *)pf_shim_platform->Get("t_shim_platform");
        std::cout << "Opened shim_platform file.\n";
      } else {
        std::cout << "Failed to open shim_platform file.\n";
      }

      sprintf(filename, "%s/fe_shim_fixed_run_%05i.root", str, run_number);
      pf_shim_fixed = new TFile(filename);
      if (!pf_shim_fixed->IsZombie()) {
        pt_old_shim_fixed = (TTree *)pf_shim_fixed->Get("t_shim_fixed");
        std::cout << "Opened shim_fixed file.\n";
      } else {
        std::cout << "Failed to open shim_fixed file.\n";
      }

      sprintf(filename, "%s/run_%05i.root", str, run_number);
      pf_final = new TFile(filename, "recreate");

      // Copy the trees from the other two and remove them.
      pt_shim_platform = pt_old_shim_platform->CloneTree();
      pt_shim_fixed = pt_old_shim_fixed->CloneTree();

      pt_shim_platform->Print();
      pt_shim_fixed->Print();

      pf_final->Write();

      pf_final->Close();
      pf_shim_platform->Close();
      pf_shim_fixed->Close();

      // Make sure the file was written and clean up.
      pf_final = new TFile(filename);
  
      if (!pf_final->IsZombie()) {
        char cmd[256];
        sprintf(cmd, "rm %s/fe_sis33*_run_%05i.root", str, run_number);
        system(cmd);
      }
    
      pf_final->Close();

      ::new_run_to_process = false;
    }
    
    usleep(100000);
  }
}


void archive_config_loop()
{
  using namespace boost::property_tree;

  //DATA part
  HNDLE hDB, hkey;
  INT status;
  char str[256], filename[256];
  int size, run_number;
  std::string histdir;
  std::string confdir;
  std::string logdir;

  cm_get_experiment_database(&hDB, NULL);

  // Get the history directory.
  db_find_key(hDB, 0, "/Logger/History dir", &hkey);
  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
    if (str[strlen(str) - 1] != DIR_SEPARATOR) {
      strcat(str, DIR_SEPARATOR_STR);
    }
  }

  histdir = std::string(str);

  // Get the log directory.
  db_find_key(hDB, 0, "/Logger/Log Dir", &hkey);
  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
    if (str[strlen(str) - 1] != DIR_SEPARATOR) {
      strcat(str, DIR_SEPARATOR_STR);
    }
  }

  logdir = std::string(str);

  // Get the config directory.
  db_find_key(hDB, 0, "/Params/config-dir", &hkey);
  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
    if (str[strlen(str) - 1] != DIR_SEPARATOR) {
      strcat(str, DIR_SEPARATOR_STR);
    }
  }

  confdir = std::string(str);

  while (!time_for_breakdown) {
    
    run_number = atomic_run_number;

    if (new_config_to_archive) {

      sprintf(str, "%srun%05d_config.json", histdir.c_str(), run_number);
      std::string conf_archive(str);
      std::string conf_file;
      std::string str1;

      ptree pt_archive;
      ptree pt_temp;

      // Open all the files.
      db_find_key(hDB, 0, "/Params/config-file/shim-platform", &hkey);
      if (hkey) {

        db_get_data(hDB, hkey, str, &size, TID_STRING);
        conf_file = confdir + std::string(str);

        try {
          read_json(conf_file, pt_temp);
          pt_archive.put_child("fe_shim_platform", pt_temp);

        } catch(...) {}
      }

      // Open all the files.
      db_find_key(hDB, 0, "/Params/config-file/shim-fixed", &hkey);
      if (hkey) {

        db_get_data(hDB, hkey, str, &size, TID_STRING);
        conf_file = confdir + std::string(str);

        try {
          read_json(conf_file, pt_temp);
          pt_archive.put_child("fe_shim_fixed", pt_temp);

        } catch(...) {}
      }

      // Open all the files.
      db_find_key(hDB, 0, "/Params/config-file/run-trolley", &hkey);
      if (hkey) {

        db_get_data(hDB, hkey, str, &size, TID_STRING);
        conf_file = confdir + std::string(str);

        try {
          read_json(conf_file, pt_temp);
          pt_archive.put_child("fe_run_trolley", pt_temp);

        } catch(...) {}
      }

      // Open all the files.
      db_find_key(hDB, 0, "/Params/config-file/run-fixed", &hkey);
      if (hkey) {

        db_get_data(hDB, hkey, str, &size, TID_STRING);
        conf_file = confdir + std::string(str);

        try {
          read_json(conf_file, pt_temp);
          pt_archive.put_child("fe_run_fixed", pt_temp);
        
        } catch(...) {}
      }

      // Now go through all the keys, and add the ones that are json files.
      std::vector<std::string> keys;

      // Get the keys first, otherwise it iterates over added keys too.
      for (auto &kv : pt_archive)
        keys.push_back(kv.first);

      // Check the devices for each front-end.
      for (auto &key : keys) {

        for (auto &val : pt_archive.get_child(key).get_child("devices")) {

          for (auto &val2 : val.second) {

            try {
              str1 = std::string(val2.second.data());
              
            } catch (...) {
              
              continue;
            }

            if (str1.find("json") != std::string::npos) {
              
              conf_file = confdir + str1;
              read_json(conf_file, pt_temp);
              
              pt_archive.put_child(val2.first, pt_temp);
              
            }
          }
        }
        
        // And all the other first level keys.
        for (auto &val : pt_archive.get_child(key)) {
          
          try {
            str1 = std::string(val.second.data());

          } catch (...) {

            continue;
          }

          if (str1.find("json") != std::string::npos) {
            
            conf_file = confdir + str1;
            read_json(conf_file, pt_temp);
          
            pt_archive.put_child(val.first, pt_temp);
          }
        }
      }

      // Write out the archive of config data.
      write_json(conf_archive, pt_archive, std::locale(), false);

      // Now take care of the runlog file with Comments and tags.
      ptree pt_runlog;
      ptree pt_run;
      std::string runlog_file = logdir + "midas_runlog.json";

      try {
        read_json(runlog_file, pt_runlog);

      } catch (...) {}

      // Get the comment from the ODB
      db_find_key(hDB, 0, "/Experiment/Run Parameters/Comment", &hkey);
      if (hkey) {
        size = sizeof(str);
        db_get_data(hDB, hkey, str, &size, TID_STRING);

        pt_run.put("comment", std::string(str));
      }

      // Get the comment from the ODB
      db_find_key(hDB, 0, "/Experiment/Run Parameters/Comment", &hkey);
      if (hkey) {
        size = sizeof(str);
        db_get_data(hDB, hkey, str, &size, TID_STRING);

        pt_run.put("comment", std::string(str));
      }

      // Get the comment from the ODB
      db_find_key(hDB, 0, "/Experiment/Run Parameters/Tags", &hkey);
      if (hkey) {
        size = sizeof(str);
        db_get_data(hDB, hkey, str, &size, TID_STRING);
    
        ptree pt_tags;
        std::string tag;
        std::istringstream ss(str);
        while (std::getline(ss, tag, ',')) {
          ptree pt_tag;
          pt_tag.put("", tag);
          pt_tags.push_back(std::make_pair("", pt_tag));
        }
    
        pt_run.put_child("tags", pt_tags);
      }

      // Get the comment from the ODB
      db_find_key(hDB, 0, "/Experiment/Runinfo/Start time binary", &hkey);
      if (hkey) {
        size = sizeof(str);
        db_get_data(hDB, hkey, str, &size, TID_STRING);

        pt_run.put("start_time", std::string(str));
      }

      // Get the comment from the ODB
      db_find_key(hDB, 0, "/Experiment/Runinfo/Stop time binary", &hkey);
      if (hkey) {
        size = sizeof(str);
        db_get_data(hDB, hkey, str, &size, TID_STRING);

        pt_run.put("stop_time", std::string(str));
      }
  
      sprintf(str, "run_%05d", run_number);
      pt_runlog.put_child(std::string(str), pt_run);

      write_json(runlog_file, pt_runlog, std::locale(), true);

      // Close the loop
      ::new_config_to_archive = false;
    }

    usleep(100000);
  }
}
