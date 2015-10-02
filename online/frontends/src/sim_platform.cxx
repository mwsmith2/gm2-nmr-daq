/********************************************************************\

Name:   sim_platform.cxx
Author: Matthias W. Smith
Email:  mwsmith2@uw.edu

About:  Implements a MIDAS frontend that this simulates the data
        output from shim_platform frontend.

\********************************************************************/

//--- std includes -------------------------------------------------//
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <ctime>
using std::string;

//--- other includes -----------------------------------------------//
#include "midas.h"
#include "TFile.h"
#include "TTree.h"
#include "fid.h"

//--- project includes ---------------------------------------------//
#include "sync_client.hh"
#include "common.hh"

//--- globals ------------------------------------------------------//

extern "C" {
  
  // The frontend name (client name) as seen by other MIDAS clients
  char *frontend_name = (char*) "sim platform";

  // The frontend file name, don't change it.
  char *frontend_file_name = (char*) __FILE__;
  
  // frontend_loop is called periodically if this variable is TRUE
  BOOL frontend_call_loop = FALSE;
  
  // A frontend status page is displayed with this frequency in ms.
  INT display_period = 1000;
  
  // maximum event size produced by this frontend
  INT max_event_size = 0x80000; // 80 kB

  // maximum event size for fragmented events (EQ_FRAGMENTED)
  INT max_event_size_frag = 0x800000;  
  
  // buffer size to hold events
  INT event_buffer_size = 0x800000;
  
  // Function declarations
  INT frontend_init();
  INT frontend_exit();
  INT begin_of_run(INT run_number, char *error);
  INT end_of_run(INT run_number, char *error);
  INT pause_run(INT run_number, char *error);
  INT resume_run(INT run_number, char *error);

  INT frontend_loop();
  INT read_trigger_event(char *pevent, INT off);
  INT poll_event(INT source, INT count, BOOL test);
  INT interrupt_configure(INT cmd, INT source, POINTER_T adr);

  // Equipment list

  EQUIPMENT equipment[] = 
    {
      {"sim-platform", // equipment name 
       { 10, 0,          // event ID, trigger mask 
         "BUF1",      // event buffer (use to be SYSTEM)
         EQ_POLLED |
         EQ_EB,         // equipment type 
         0,             // not used 
         "MIDAS",       // format 
         TRUE,          // enabled 
         RO_RUNNING |   // read only when running 
         RO_ODB,        // and update ODB 
         10,            // poll for 10ms 
         0,             // stop run after this event limit 
         0,             // number of sub events 
         90,             // don't log history 
         "", "", "",
       },
       read_trigger_event,      // readout routine 
      },
      
      {""}
    };

} //extern C

RUNINFO runinfo;

// Anonymous namespace for my "globals"
namespace {
TFile *root_file;
TTree *t;
bool write_root = false;
bool write_midas = true;
std::atomic<bool> run_in_progress;
std::atomic<bool> got_an_event;
std::atomic<bool> finished_sim_event;
daq::shim_platform_st data; // st = short trace
daq::SyncClient *listener;
const int nprobes = SHIM_PLATFORM_CH;
const char *const mbank_name = (char *)"SMPF";
}

//--- Frontend Init -------------------------------------------------//
INT frontend_init() 
{
  //DATA part
  string conf_file;
  HNDLE hDB, hkey;
  INT status;
  char str[256], filename[256];
  int size;
    
  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, "Params/config-dir", &hkey);
    
  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
    if (str[strlen(str) - 1] != DIR_SEPARATOR) {
      strcat(str, DIR_SEPARATOR_STR);
    }
  }

  conf_file = std::string(str);
  conf_file += std::string("sim_platform.json");

  // Open config file for local use.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file, conf);

  // Set up the SUB(trigger) socket.
  string trigger_addr = conf.get<string>("master_address");
  int trigger_port = conf.get<int>("trigger_port");
  listener = new daq::SyncClient(trigger_addr, trigger_port);

  got_an_event = false;
  run_in_progress = false;
  finished_sim_event = false;

  return SUCCESS;
}

//--- Frontend Exit ------------------------------------------------//
INT frontend_exit()
{
  delete listener;

  run_in_progress = false;

  return SUCCESS;
}

//--- Begin of Run --------------------------------------------------*/
INT begin_of_run(INT run_number, char *error)
{
  //HW part
  listener->SetReady();

  //DATA part
  HNDLE hDB, hkey;
  INT status;
  char str[256], filename[256];
  int size;
    
  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, "/Logger/Data dir", &hkey);
    
  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
    if (str[strlen(str) - 1] != DIR_SEPARATOR) {
      strcat(str, DIR_SEPARATOR_STR);
    }
  }
    
  db_find_key(hDB, 0, "/Runinfo", &hkey);
  if (db_open_record(hDB, hkey, &runinfo, sizeof(runinfo), MODE_READ,
		     NULL, NULL) != DB_SUCCESS) {
    cm_msg(MERROR, "analyzer_init", "Cannot open \"/Runinfo\" tree in ODB");
    return 0;
  }
  
  // Get the run number out of the MIDAS database.
  strcpy(filename, str);
  sprintf(str, "run_%05d_fe.root", runinfo.run_number);
  strcat(filename, str);

  if (write_root) {
    // Set up the ROOT data output.
    root_file = new TFile(filename, "recreate");
    t = new TTree("t", "t");
    t->SetAutoSave(0);
    t->SetAutoFlush(0);
    
    std::string br_name("sim_platform");
    t->Branch(br_name.c_str(), &data, daq::shim_platform_st_string);
  }
    
  cm_msg(MINFO, frontend_name, "begin_of_run completed");

  got_an_event = false;
  finished_sim_event = false;
  run_in_progress = true;

  return SUCCESS;
}

//--- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{
  listener->UnsetReady();

  // Make sure we write the ROOT data.
  if (run_in_progress && write_root) {
    t->Write();
    root_file->Close();
    
    delete root_file;
  }
  run_in_progress = false;
  
  return SUCCESS;
}

//--- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error)
{
  return SUCCESS;
}

//--- Resuem Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error)
{
  return SUCCESS;
}

//--- Frontend Loop -------------------------------------------------*/

INT frontend_loop()
{
  // If frontend_call_loop is true, this routine gets called when
  // the frontend is idle or once between every event
  return SUCCESS;
}

//-------------------------------------------------------------------*/

/********************************************************************\
  
  Readout routines for different events

\********************************************************************/

//--- Trigger event routines ----------------------------------------*/

INT poll_event(INT source, INT count, BOOL test) {

  static unsigned int i;

  // fake calibration
  if (test) {
    for (i = 0; i < count; i++) {
      usleep(10);
    }
    return 0;
  }
  
  return (listener->HasTrigger() == true)? 1 : 0;
}

//--- Interrupt configuration ---------------------------------------*/

INT interrupt_configure(INT cmd, INT source, PTYPE adr)
{
  switch (cmd) {
  case CMD_INTERRUPT_ENABLE:
    break;
  case CMD_INTERRUPT_DISABLE:
    break;
  case CMD_INTERRUPT_ATTACH:
    break;
  case CMD_INTERRUPT_DETACH:
    break;
  }
  return SUCCESS;
}

//--- Event readout -------------------------------------------------*/

INT read_trigger_event(char *pevent, INT off)
{
  using namespace daq;
  static unsigned long long num_events;
  static unsigned long long events_written;

  int count = 0;
  char bk_name[10];
  DWORD *pdata;

  if (!run_in_progress) return 0;

  // Set up the array of field strengths.
  // implement multipoles in future.
  static std::vector<double> field_strength;
  if (field_strength.size() == 0) {
    field_strength.assign(nprobes, 50.0);
    fid::addnoise(field_strength, 100.0);
  }

  // Simulate the data
  for (int idx = 0; idx < nprobes; ++idx) {
    static std::vector<double> tm;
    static std::vector<double> wf(SHORT_FID_LN);
    static fid::FidFactory ff;

    if (tm.size() == 0) {
      tm.resize(SHORT_FID_LN);
      for (int i = 0; i < SHORT_FID_LN; ++i) {
        tm[i] = 0.001 * i;
      }
    }
    
    fid::sim::freq_larmor = fid::sim::freq_ref + field_strength[idx];
    ff.IdealFid(wf, tm, true);

    fid::FID myfid(tm, wf);
    
    // Make sure we got an FID signal
    if (myfid.isgood()) {
      
      data.freq[idx] = myfid.CalcPhaseFreq();
      data.ferr[idx] = myfid.freq_err();
      data.snr[idx] = myfid.snr();
      data.len[idx] = myfid.fid_time();
      
    } else {
      
      myfid.PrintDiagnosticInfo();
      data.freq[idx] = -1.0;
      data.ferr[idx] = -1.0;
      data.snr[idx] = -1.0;
      data.len[idx] = -1.0;
    }

    data.method[idx] = fid::Method::PH;
    data.health[idx] = myfid.isgood();
    
    for (int i = 0; i < myfid.wf().size(); ++i) {
      data.trace[idx][i] = myfid.wf()[i];
    }

    struct timeval tv;
    gettimeofday(&tv, nullptr);
    data.sys_clock[idx] = tv.tv_sec + tv.tv_usec / 1.0e6;
    data.gps_clock[idx] = tv.tv_sec + tv.tv_usec / 1.0e6;
    data.dev_clock[idx] = tv.tv_sec + tv.tv_usec / 1.0e6;
  }

  if (write_root) {
    // Now that we have a copy of the latest event, fill the tree.
    t->Fill();
    num_events++;

    if (num_events % 1000 == 1) {

      t->AutoSave("SaveSelf,FlushBaskets");
      root_file->Flush();

    }
  }

  // And MIDAS output.
  bk_init32(pevent);

  if (write_midas) {

    // Copy the shimming trolley data.
    bk_create(pevent, mbank_name, TID_DWORD, &pdata);

    memcpy(pdata, &data, sizeof(data));
    pdata += sizeof(data) / sizeof(DWORD);

    bk_close(pevent, pdata);
  }

  // Reset the listening after sleeping for some time.
  usleep(2.0e5);
  std::cout << "Resetting the listener." << std::endl;
  listener->SetReady();

  return bk_size(pevent);
}
