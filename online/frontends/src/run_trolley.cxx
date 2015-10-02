/********************************************************************\

Name:   run_trolley.cxx
Author: Matthias W. Smith
Email:  mwsmith2@uw.edu

About:  Implements a MIDAS frontend that is aware of the 
        multiplexers.  It configures them, takes data in a 
        sequence, and builds an event from all the data.
        
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

//--- project includes ---------------------------------------------//
#include "event_manager_trg_seq.hh"
#include "sync_client.hh"
#include "common.hh"

//--- globals ------------------------------------------------------//

extern "C" {
  
  // The frontend name (client name) as seen by other MIDAS clients
  char *frontend_name = (char*) "run-trolley";

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
  INT interrupt_configure(INT cmd, INT source, PTYPE adr);

  // Equipment list

  EQUIPMENT equipment[] = 
    {
      {"run-trolley", // equipment name 
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
TFile* root_file;
TTree* t;
bool write_root = false;
bool write_midas = true;
std::atomic<bool> run_in_progress;
std::atomic<bool> got_an_event;
daq::run_trolley_st data; // st = short trace
daq::EventManagerTrgSeq *event_manager;
daq::SyncClient *listener;
const int nprobes = RUN_TROLLEY_CH;
const char *const mbank_name = (char *)"RNTR";
}

//--- Frontend Init -------------------------------------------------//
INT frontend_init() 
{
  //DATA part
  string conf_file;
  HNDLE hDB, hkey;
  INT status, tmp;
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

  db_find_key(hDB, 0, "Params/config-file/shim-fixed", &hkey);
  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
    
    conf_file += std::string(str);
    
  } else {

    conf_file += std::string("fnal/fe_shim_fixed.json");
  }

  event_manager = new daq::EventManagerTrgSeq(conf_file, nprobes);

  // Get the config for the synchronized trigger.
  db_find_key(hDB, 0, "Params/sync-trigger-address", &hkey);

  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
  }

  string trigger_addr(str);

  db_find_key(hDB, 0, "Params/fast-trigger-port", &hkey);

  if (hkey) {
    size = sizeof(tmp);
    db_get_data(hDB, hkey, &tmp, &size, TID_INT);
  }

  int trigger_port(tmp);

  listener = new daq::SyncClient(trigger_addr, trigger_port);

  got_an_event = false;
  run_in_progress = false;

  return SUCCESS;
}

//--- Frontend Exit ------------------------------------------------//
INT frontend_exit()
{
  delete event_manager;
  delete listener;

  run_in_progress = false;

  return SUCCESS;
}

//--- Begin of Run --------------------------------------------------*/
INT begin_of_run(INT run_number, char *error)
{
  //HW part
  event_manager->BeginOfRun();
  listener->SetReady();

  //DATA part
  HNDLE hDB, hkey;
  INT status;
  BOOL flag;
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
  sprintf(str, "run_%05d.root", runinfo.run_number);
  strcat(filename, str);

  // Get the parameter for root output.
  db_find_key(hDB, 0, "/Params/root-output", &hkey);

  if (hkey) {
    size = sizeof(flag);
    db_get_data(hDB, hkey, &flag, &size, TID_BOOL);

    write_root = flag;
  }

  if (write_root) {
    // Set up the ROOT data output.
    root_file = new TFile(filename, "recreate");
    t = new TTree("t_rntr", "NMR Trolley Data");
    t->SetAutoSave(0);
    t->SetAutoFlush(0);
    
    std::string br_name("run_trolley");
    t->Branch(br_name.c_str(), &data, daq::run_trolley_st_string);
  }
    
  run_in_progress = true;
  cm_msg(MINFO, frontend_name, "begin_of_run complete.");

  return SUCCESS;
}

//--- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{
  event_manager->EndOfRun();
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
  event_manager->PauseRun();
  return SUCCESS;
}

//--- Resuem Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error)
{
  event_manager->ResumeRun();
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
  
  if (listener->HasTrigger()) {
    std::cout << "Issuing trigger." << std::endl;
    event_manager->IssueTrigger();
  }

  if (event_manager->HasEvent()) {
    std::cout << "Got data." << std::endl;
  }
  return ((event_manager->HasEvent() == true)? 1 : 0);
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

  // Copy the data
  auto shim_data = event_manager->GetCurrentEvent();

  for (int idx = 0; idx < nprobes; ++idx) {
    std::vector<double> tm(SHORT_FID_LN);
    std::vector<double> wf(SHORT_FID_LN);

    for (int n = 0; n < SHORT_FID_LN; ++n) {
      tm[n] = 0.001 * n;
      wf[n] = shim_data.trace[idx][n*10 + 1];
      data.trace[idx][n] = wf[n];
    }

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
  }
  
  std::copy(shim_data.sys_clock.begin(), 
            shim_data.sys_clock.begin() + nprobes,
            &data.sys_clock[0]);

  std::copy(shim_data.gps_clock.begin(), 
            shim_data.gps_clock.begin() + nprobes,
            &data.gps_clock[0]);

  std::copy(shim_data.dev_clock.begin(), 
            shim_data.dev_clock.begin() + nprobes,
            &data.dev_clock[0]);
  
  std::copy(shim_data.snr.begin(), 
            shim_data.snr.begin() + nprobes,
            &data.snr[0]);
  
  std::copy(shim_data.len.begin(), 
            shim_data.len.begin() + nprobes,
            &data.len[0]);
  
  std::copy(shim_data.freq.begin(), 
            shim_data.freq.begin() + nprobes,
            &data.freq[0]);
  
  std::copy(shim_data.ferr.begin(), 
            shim_data.ferr.begin() + nprobes,
            &data.ferr[0]);

  std::copy(shim_data.freq_zc.begin(), 
            shim_data.freq_zc.begin() + nprobes,
            &data.freq_zc[0]);

  std::copy(shim_data.ferr_zc.begin(), 
            shim_data.ferr_zc.begin() + nprobes,
            &data.ferr_zc[0]);

  std::copy(shim_data.method.begin(), 
            shim_data.method.begin() + nprobes,
            &data.method[0]);

  std::copy(shim_data.health.begin(), 
            shim_data.health.begin() + nprobes,
            &data.health[0]);

  // for (int idx = 0; idx < nprobes; ++idx) {
  //   std::copy(shim_data.trace[idx].begin(), 
  //         shim_data.trace[idx].end(), 
  //         &data.trace[idx][0]);
  // }
  
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

  // Pop the event now that we are done copying it.
  std::cout << "Popping event manager data." << std::endl;
  event_manager->PopCurrentEvent();
  listener->SetReady();

  return bk_size(pevent);
}
