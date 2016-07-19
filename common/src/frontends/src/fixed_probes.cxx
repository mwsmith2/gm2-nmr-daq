/*****************************************************************************\

Name:   fixed_probes.cxx
Author: Matthias W. Smith
Email:  mwsmith2@uw.edu

About:  Implements a MIDAS frontend that is aware of the
        multiplexers.  It configures them, takes data in a
        sequence, and builds an event from all the data.

\*****************************************************************************/

//--- std includes ----------------------------------------------------------//
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

//--- other includes --------------------------------------------------------//
#include "midas.h"
#include "TFile.h"
#include "TTree.h"

//--- project includes ------------------------------------------------------//
#include "sync_client.hh"
#include "fixed_probe_sequencer.hh"
#include "frontend_utils.hh"

//--- globals ---------------------------------------------------------------//
#define FRONTEND_NAME "Fixed Probes"

extern "C" {

  // The frontend name (client name) as seen by other MIDAS clients
  char *frontend_name = (char *)FRONTEND_NAME;

  // The frontend file name, don't change it.
  char *frontend_file_name = (char *)__FILE__;

  // frontend_loop is called periodically if this variable is TRUE
  BOOL frontend_call_loop = FALSE;

  // A frontend status page is displayed with this frequency in ms.
  INT display_period = 1000;

  // maximum event size produced by this frontend
  INT max_event_size = 0x800000; // 8 MB

  // maximum event size for fragmented events (EQ_FRAGMENTED)
  INT max_event_size_frag = 0x100000;

  // buffer size to hold events
  INT event_buffer_size = 0x1000000; // 64 MB

  // Function declarations
  INT frontend_init();
  INT frontend_exit();
  INT begin_of_run(INT run_number, char *error);
  INT end_of_run(INT run_number, char *error);
  INT pause_run(INT run_number, char *error);
  INT resume_run(INT run_number, char *error);

  INT frontend_loop();
  INT read_fixed_probe_event(char *pevent, INT off);
  INT poll_event(INT source, INT count, BOOL test);
  INT interrupt_configure(INT cmd, INT source, POINTER_T adr);

  // Equipment list

  EQUIPMENT equipment[] =
    {
      {FRONTEND_NAME,  // equipment name
       { 18, 13,        // event ID, trigger mask
         "SYSTEM",      // event buffer (use to be SYSTEM)
         EQ_PERIODIC,   // equipment type
         0,             // not used
         "MIDAS",       // format
         TRUE,          // enabled
         RO_RUNNING,    // read only when running
         10,            // poll for 10ms
         0,             // stop run after this event limit
         0,             // number of sub events
         0,             // don't log history
         "", "", "",
       },
       read_fixed_probe_event,      // readout routine
      },

      {""}
    };

} //extern C

RUNINFO runinfo;

// Anonymous namespace for my "globals"
namespace {
double step_size = 0.4;  //  23.375 mm per 1 step size // So pretty close to 1 inch per 1 step_size (note: this calibration occurred using a chord of about 3 meters, so the string was not moving along the azimuth. So there is a missing cos(theta) -- where theta is small -- that needs to be included

double event_rate_limit = 10.0;
int num_steps = 50; // num_steps was 50
int num_shots = 1;
int trigger_count = 0;
int event_number = 0;
bool write_root = false;
bool save_full_waveforms = false;
bool write_midas = true;
bool use_stepper = true;
bool ino_stepper_type = false;

TFile *pf;
TTree *pt_shim;
TTree *pt_full;

std::atomic<bool> run_in_progress;
std::mutex data_mutex;

boost::property_tree::ptree conf;
std::string nmr_sequence_conf_file;

gm2::fixed_t data;
gm2::long_fixed_t full_data;
gm2::FixedProbeSequencer *event_manager;

const int nprobes = SHIM_FIXED_CH;
const char *const mbank_name = (char *)"FXPR";
}

void trigger_loop();
void set_json_tmpfiles();
int load_device_classes();

void set_json_tmpfiles()
{
  // Allocations
  char tmp_file[256];
  std::string conf_file;
  boost::property_tree::ptree pt;
  boost::property_tree::ptree pt_child;

  // Copy the wfd config file.
  snprintf(tmp_file, 128, "/tmp/gm2-nmr-config_XXXXXX.json");
  mkstemps(tmp_file, 5);
  conf_file = std::string(tmp_file);

  // Copy the json, and set to the temp file.
  pt = conf.get_child("devices.sis_3302");
  boost::property_tree::write_json(conf_file, pt.get_child("sis_3302_0"));
  pt.clear();
  pt.put("sis_3302_0", conf_file);

  conf.get_child("devices.sis_3302").erase("sis_3302_0");
  conf.put_child("devices.sis_3302", pt);

  // Copy the trigger sequence file.
  snprintf(tmp_file, 128, "/tmp/gm2-nmr-config_XXXXXX.json");
  mkstemps(tmp_file, 5);
  conf_file = std::string(tmp_file);

  // Copy the json, and set to the temp file.
  pt = conf.get_child("trg_seq_file");
  conf.erase("trg_seq_file");
  conf.put<std::string>("trg_seq_file", conf_file);
  boost::property_tree::write_json(conf_file, pt);

  // Now the mux configuration
  snprintf(tmp_file, 128, "/tmp/gm2-nmr-config_XXXXXX.json");
  mkstemps(tmp_file, 5);
  conf_file = std::string(tmp_file);

  // Copy the json, and set to the temp file.
  pt = conf.get_child("mux_conf_file");
  conf.erase("mux_conf_file");
  conf.put<std::string>("mux_conf_file",  conf_file);
  boost::property_tree::write_json(conf_file, pt);

  // And the fid params.
  snprintf(tmp_file, 128, "/tmp/gm2-nmr-config_XXXXXX.json");
  mkstemps(tmp_file, 5);
  conf_file = std::string(tmp_file);

  // Copy the json, and set to the temp file.
  pt = conf.get_child("fid_conf_file");
  conf.erase("fid_conf_file");
  conf.put<std::string>("fid_conf_file", conf_file);
  boost::property_tree::write_json(conf_file, pt);

  // Now save the config to a temp file and feed it to the Event Manager.
  snprintf(tmp_file, 128, "/tmp/gm2-nmr-config_XXXXXX.json");
  mkstemps(tmp_file, 5);
  nmr_sequence_conf_file = std::string(tmp_file);
  boost::property_tree::write_json(nmr_sequence_conf_file, conf);
}

int load_device_classes()
{
  // Allocations
  char str[256];
  std::string conf_file;

  // Set up the event mananger.
  event_manager = new gm2::FixedProbeSequencer(nmr_sequence_conf_file, nprobes);
  event_rate_limit = conf.get<double>("event_rate_limit");

  return SUCCESS;
}

//--- Frontend Init ----------------------------------------------------------//
INT frontend_init()
{
  INT rc = load_settings(frontend_name, conf);

  if (rc != SUCCESS) {
    // Error already logged in load_settings.
    return rc;
  }

  set_json_tmpfiles();

  rc = load_device_classes();
  if (rc != SUCCESS) {
    // Error already logged in load_device_classes.
    return rc;
  }

  run_in_progress = false;

  cm_msg(MINFO, "init", "Shim Fixed initialization complete");
  return SUCCESS;
}

//--- Frontend Exit ---------------------------------------------------------//
INT frontend_exit()
{
  run_in_progress = false;

  delete event_manager;

  cm_msg(MINFO, "exit", "Shim Fixed teardown complete");
  return SUCCESS;
}

//--- Begin of Run --------------------------------------------------//
INT begin_of_run(INT run_number, char *error)
{
  INT rc = load_settings(frontend_name, conf);

  if (rc != SUCCESS) {
    return rc;
  }

  set_json_tmpfiles();

  rc = load_device_classes();
  if (rc != SUCCESS) {
    return rc;
  }

   // ODB parameters
  HNDLE hDB, hkey;
  char str[256];
  int size;
  BOOL flag;

  // Set up the data.
  std::string datadir;
  std::string filename;

  // Grab the database handle.
  cm_get_experiment_database(&hDB, NULL);

  // Get the run info out of the ODB.
  db_find_key(hDB, 0, "/Runinfo", &hkey);
  if (db_open_record(hDB, hkey, &runinfo, sizeof(runinfo), MODE_READ,
		     NULL, NULL) != DB_SUCCESS) {
    cm_msg(MERROR, "begin_of_run", "Can't open \"/Runinfo\" in ODB");
    return CM_DB_ERROR;
  }

  // Get the data directory from the ODB.
  snprintf(str, sizeof(str), "/Logger/Data dir");
  db_find_key(hDB, 0, str, &hkey);

  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
    datadir = std::string(str);
  }

  // Set the filename
  snprintf(str, sizeof(str), "root/fixed_run_%05d.root", runinfo.run_number);

  // Join the directory and filename using boost filesystem.
  filename = (boost::filesystem::path(datadir) / boost::filesystem::path(str)).string();

  // Get the parameter for root output.
  db_find_key(hDB, 0, "/Experiment/Run Parameters/Root Output", &hkey);

  if (hkey) {
    size = sizeof(flag);
    db_get_data(hDB, hkey, &flag, &size, TID_BOOL);

    write_root = flag;
    write_root = true;
  }

  // Check if we want to save full waveforms.
  if (write_root) {

    db_find_key(hDB, 0, "/Experiment/Run Parameters/Save Full Waveforms", &hkey);

    if (hkey) {
      size = sizeof(flag);
      db_get_data(hDB, hkey, &flag, &size, TID_BOOL);

      save_full_waveforms = flag;
    }
  }

  if (write_root) {
    // Set up the ROOT data output.
    pf = new TFile(filename.c_str(), "recreate");
    pt_shim = new TTree("t_fxpr", "Shim Fixed Probe Data");
    pt_shim->SetAutoSave(5);
    pt_shim->SetAutoFlush(20);

    std::string br_name("shim_fixed");

    pt_shim->Branch(br_name.c_str(), &data.sys_clock[0], gm2::fixed_str);

    if (save_full_waveforms) {
      std::string br_name("full_shim_fixed");
      pt_full->SetAutoSave(5);
      pt_full->SetAutoFlush(20);

      pt_full->Branch(br_name.c_str(),
                      &full_data.sys_clock[0],
                      gm2::long_fixed_str);
    }
  }

  // HW part
  event_manager->BeginOfRun();
  event_rate_limit = conf.get<double>("event_rate_limit");

  event_number = 0;
  run_in_progress = true;

  cm_msg(MLOG, "begin_of_run", "Completed successfully");

  return SUCCESS;
}

//--- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{
  event_manager->EndOfRun();

  // Make sure we write the ROOT data.
  if (run_in_progress && write_root) {

    pt_shim->Write();

    if (save_full_waveforms) {
      pt_full->Write();
    }

    pf->Write();
    pf->Close();

    delete pf;
  }

  run_in_progress = false;

  cm_msg(MLOG, "end_of_run", "Completed successfully");
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

  static bool triggered = false;

  // fake calibration
  if (test) {
    for (int i = 0; i < count; i++) {
      usleep(10);
    }
    return 0;
  }

  // if (!triggered) {
  //   event_manager->IssueTrigger();
  //   triggered = true;
  // }

  // if (triggered && event_manager->HasEvent()) {

  //   triggered = false;
  //   return 1;

  // } else {

  //   return 0;
  // }
  return 0;
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

INT read_fixed_probe_event(char *pevent, INT off)
{
  using namespace daq;
  static bool triggered;
  static unsigned long long num_events;
  static unsigned long long events_written;

  // Allocate vectors for the FIDs
  static std::vector<double> tm;
  static std::vector<double> wf;

  int count = 0;
  char bk_name[10];
  DWORD *pdata;

  if (!triggered) {
    event_manager->IssueTrigger();
    triggered = true;
  }

  if (triggered && !event_manager->HasEvent()) {
    return 0;

  } else {

    cm_msg(MDEBUG, "read_fixed_event", "got data");
  }

  auto shim_data = event_manager->GetCurrentEvent();

  if ((shim_data.sys_clock[0] == 0) && (shim_data.sys_clock[nprobes-1] == 0)) {
    return 0;
  }

  // Set the time vector.
  if (tm.size() == 0) {

    tm.resize(SAVE_FID_LN);
    wf.resize(SAVE_FID_LN);

    for (int n = 0; n < SAVE_FID_LN; ++n) {
      tm[n] = 0.001 * n;
    }
  }

  data_mutex.lock();

  for (int idx = 0; idx < nprobes; ++idx) {

    for (int n = 0; n < SAVE_FID_LN; ++n) {
      wf[n] = shim_data.trace[idx][n*10 + 1]; // Offset avoid spikes in sis3302
      data.trace[idx][n] = wf[n];
    }

    fid::Fid myfid(tm, wf);

    // Make sure we got an FID signal
    if (myfid.isgood()) {

      data.freq[idx] = myfid.CalcPhaseFreq();
      data.ferr[idx] = myfid.freq_err();
      data.snr[idx] = myfid.snr();
      data.len[idx] = myfid.fid_time();

    } else {

      myfid.DiagnosticInfo();
      data.freq[idx] = -1.0;
      data.ferr[idx] = -1.0;
      data.snr[idx] = -1.0;
      data.len[idx] = -1.0;
    }
  }

  cm_msg(MINFO, frontend_name, "copying the data from event");
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

  data_mutex.unlock();

  if (write_root) {
    cm_msg(MINFO, "read_fixed_event", "Filling TTree");
    // Now that we have a copy of the latest event, fill the tree.
    pt_shim->Fill();

    if (save_full_waveforms) {
      pt_full->Fill();
    }

    num_events++;

    if (num_events % 10 == 1) {

      cm_msg(MINFO, frontend_name, "flushing TTree.");
      pt_shim->AutoSave("SaveSelf,FlushBaskets");

      if (save_full_waveforms) {
        pt_full->AutoSave("SaveSelf,FlushBaskets");
      }

      pf->Flush();
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
  cm_msg(MINFO, "read_fixed_event", "Finished with event, popping from queue");
  event_manager->PopCurrentEvent();

  // Let the front-end know we are ready for another trigger.
  triggered = false;

  return bk_size(pevent);
}
