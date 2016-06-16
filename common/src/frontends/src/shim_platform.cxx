/*****************************************************************************\

Name:   shim_platform.cxx
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
#include "curl/curl.h"

//--- project includes ------------------------------------------------------//
#include "sync_client.hh"
#include "dio_stepper_motor.hh"
#include "ino_stepper_motor.hh"
#include "nmr_sequencer.hh"
#include "frontend_utils.hh"

//--- globals ---------------------------------------------------------------//
#define FRONTEND_NAME "Shim Platform"

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
  INT max_event_size = 0x100000; // 1 MB

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
  INT read_platform_event(char *pevent, INT off);
  INT poll_event(INT source, INT count, BOOL test);
  INT interrupt_configure(INT cmd, INT source, POINTER_T adr);

  // Equipment list

  EQUIPMENT equipment[] =
    {
      {FRONTEND_NAME,  // equipment name
       { 10, 0,        // event ID, trigger mask
         "BUF1",       // event buffer (use to be SYSTEM)
         EQ_POLLED |
         EQ_EB,         // equipment type
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
       read_platform_event,      // readout routine
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
bool write_root = false;
bool write_midas = true;
bool use_stepper = true;
bool ino_stepper_type = false;

TFile *root_file;
TTree *t;

std::atomic<bool> run_in_progress;
std::atomic<bool> ready_to_move;
std::atomic<bool> thread_live;
std::thread trigger_thread;
std::mutex data_mutex;

boost::property_tree::ptree conf;

daq::SyncClient *readout_listener;  // for readout.
daq::SyncClient *stepper_listener; // for the stepper motor
daq::DioStepperMotor *dio_stepper = nullptr;
daq::InoStepperMotor *ino_stepper = nullptr;

gm2::platform_t data; // st = short trace
gm2::NmrSequencer *event_manager;

const int nprobes = SHIM_PLATFORM_CH;
const char *const mbank_name = (char *)"SHPF";
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

  // And finally the stepper motor.
  snprintf(tmp_file, 128, "/tmp/gm2-nmr-config_XXXXXX.json");
  mkstemps(tmp_file, 5);
  conf_file = std::string(tmp_file);

  // Copy the json, and set to the temp file.
  pt = conf.get_child("stepper_motor_file");
  conf.erase("stepper_motor_file");
  conf.put<std::string>("stepper_motor_file", conf_file);
  boost::property_tree::write_json(conf_file, pt);

  // Now save the config to a temp file and feed it to the Event Manager.
  snprintf(tmp_file, 128, "/tmp/gm2-nmr-config_XXXXXX.json");
  mkstemps(tmp_file, 5);
  conf_file = std::string(tmp_file);
  boost::property_tree::write_json(conf_file, conf);
}

int load_device_classes()
{
  // Allocations
  char str[256];
  std::string conf_file;

  // Set up the event mananger.
  conf_file = conf.get<std::string>("trg_seq_file");
  event_manager = new gm2::NmrSequencer(conf_file, nprobes);

  if (conf.get<std::string>("stepper_type") == "INO") {

    ino_stepper_type = true;

  } else {

    ino_stepper_type = false;
  }

  if (ino_stepper_type) {

    conf_file = conf.get<std::string>("stepper_motor_file");
    ino_stepper = new daq::InoStepperMotor(conf_file);

  } else {

    conf_file = conf.get<std::string>("stepper_motor_file");
    dio_stepper = new daq::DioStepperMotor(0x0, daq::BOARD_B, conf_file);
  }

  readout_listener = new daq::SyncClient(std::string(frontend_name),
                                         conf.get<string>("sync_trigger_addr"),
                                         conf.get<int>("fast_trigger_port"));

  //Brendan : Piggy-back off of the port information already extracted for the listener, and increment it by 1

  stepper_listener = new daq::SyncClient(std::string(frontend_name),
                                         conf.get<string>("sync_trigger_addr"),
                                         conf.get<int>("fast_trigger_port") + 30);

  // Load the steppper motor params.
  step_size = conf.get<double>("step_size");
  event_rate_limit = conf.get<double>("event_rate_limit");
  num_steps = conf.get<int>("num_steps");
  num_shots = conf.get<int>("num_shots");
  use_stepper = conf.get<bool>("use_stepper");

  return SUCCESS;
}

//--- Frontend Init ----------------------------------------------------------//
INT frontend_init()
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

  thread_live = true;
  run_in_progress = false;
  ready_to_move = false;

  trigger_thread = std::thread(::trigger_loop);

  cm_msg(MINFO, "init", "Shim Platform initialization complete");
  return SUCCESS;
}

//--- Frontend Exit ---------------------------------------------------------//
INT frontend_exit()
{
  run_in_progress = false;
  thread_live = false;

  if (trigger_thread.joinable()) {
    trigger_thread.join();
  }

  delete event_manager;
  delete readout_listener;
  delete stepper_listener;

  if (dio_stepper != nullptr) {
    delete dio_stepper;
    dio_stepper = nullptr;
  }

  if (ino_stepper != nullptr) {
    delete ino_stepper;
    ino_stepper = nullptr;
  }

  cm_msg(MINFO, "exit", "Shim Platform teardown complete");
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
  snprintf(str, sizeof(str), "root/platform_run_%05d.root", runinfo.run_number);

  // Join the directory and filename using boost filesystem.
  filename = (boost::filesystem::path(datadir) / boost::filesystem::path(str)).string();

  // Get the parameter for root output.
  db_find_key(hDB, 0, "/Experiment/Run Parameters/Root Output", &hkey);

  if (hkey) {
    size = sizeof(flag);
    db_get_data(hDB, hkey, &flag, &size, TID_BOOL);

    write_root = flag;
  }

  if (write_root) {
    // Set up the ROOT data output.
    root_file = new TFile(filename.c_str(), "recreate");
    t = new TTree("t_shpf", "Shim Platform Data");
    t->SetAutoSave(5);
    t->SetAutoFlush(20);

    std::string br_name("shim_platform");

    t->Branch(br_name.c_str(), &data.sys_clock[0], gm2::platform_str);
  }

  //HW part
  event_manager->BeginOfRun();
  readout_listener->SetReady();
  stepper_listener->UnsetReady(); // Make sure the stepper is not set yet

  // Reload the step structure.
  step_size = conf.get<double>("step_size");
  event_rate_limit = conf.get<double>("step_size");
  num_steps = conf.get<int>("num_steps");

  if (num_steps < 1) {
    num_steps = INT_MAX; // basically infinite.
  }

  // And finally the bool.
  use_stepper = conf.get<bool>("use_stepper");

  run_in_progress = true;
  ready_to_move = false;

  cm_msg(MLOG, "begin_of_run", "Completed successfully");

  return SUCCESS;
}

//--- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{
  event_manager->EndOfRun();
  readout_listener->UnsetReady();
  stepper_listener->UnsetReady();

  // Make sure we write the ROOT data.
  if (run_in_progress && write_root) {
    t->Write();
    root_file->Write();
    root_file->Close();

    delete root_file;
  }

  run_in_progress = false;
  ready_to_move = false;

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

  static unsigned int i;

  // fake calibration
  if (test) {
    for (i = 0; i < count; i++) {
      usleep(10);
    }
    return 0;
  }

  if (readout_listener->HasTrigger()) {
    cm_msg(MINFO, "poll_event",
           "Got trigger, beginning multiplexer sequence");
    event_manager->IssueTrigger();
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

INT read_platform_event(char *pevent, INT off)
{
  cm_msg(MLOG, "read_platform_event", "got data");
  using namespace daq;
  static unsigned long long num_events;
  static unsigned long long events_written;

  // Allocate vectors for the FIDs
  static std::vector<double> tm;
  static std::vector<double> wf;

  int count = 0;
  char bk_name[10];
  DWORD *pdata;

  if (!run_in_progress) return 0;

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
      wf[n] = shim_data.trace[idx][n*10 + 1];
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
    cm_msg(MINFO, "read_platform_event", "Filling TTree");
    // Now that we have a copy of the latest event, fill the tree.
    t->Fill();
    num_events++;

    if (num_events % 10 == 1) {

      cm_msg(MINFO, frontend_name, "flushing TTree.");
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
  cm_msg(MINFO, "read_platform_event", "Finished with event, popping from queue");
  event_manager->PopCurrentEvent();

  // Brendan: At this point the multiplexors are done (and in fact all the data have been copied to root, midas, etc), but we have no guarantee that the laser tracker, environment, etc.... other frontends might still be going. So we really need to check in with the SyncTrigger that everyone ELSE is done before moving the motor

  // E.G.
  // Have a second SyncClient called stepper_listener
  // At this point issue the Trigger to stepper_listener

  // When stepper_listener is done, then unset stepper_listener->IsReady(), and set listenerReady() to true

  // OR.....
  // if we could query the listener within a loop, waiting for n-1 Clients to be done (that means everyone else)
  // then we could set ready_to_move true when that condition is met


  // The point of this if/else is to set the listener to Ready. in the scheme where we have to use a stepper, we need to first make sure we wait for all
  // non-stepper frontends to finish, and we delegate the responsibility of setting readout_listener->SetReady to the stepper thread
  // if the stepper isn't used, then we simply ignore the stepper_listener and set our readout_listener->SetReady directly
  if (use_stepper == true) {
    stepper_listener->SetReady();
    // ready_to_move = true;

  } else {

    readout_listener->SetReady();
  }

  return bk_size(pevent);
}

void trigger_loop()
{
  bool rc = false;
  int num_events = 0;
  unsigned int ss_idx = 0;

  // Now launch the state machine thread
  while (thread_live) {

    if (run_in_progress) {

       // Each loop in here is one data point with the stepper motor.
      while (run_in_progress) {

        if (use_stepper && (stepper_listener->HasTrigger() == false)) {

          // Wait for the next event.
          usleep(1000);

        } else if (!use_stepper) {

          // Bypass the stepper motor phase if not in use.
          readout_listener->SetReady();

        } else {

          // stepper_listener Had a Trigger, and now HasTrigger gets set back to false

          cm_msg(MINFO, "trigger_loop", "READY TO MOVE: issuing step");

          ss_idx = (ss_idx + 1) % num_shots;

          // If last step, end the run.
          if (num_events >= num_steps) {

            cm_msg(MINFO, "trigger_loop", "Step sequence complete");

            // Reset for next measurement.
            num_events = 0;

            // Request an end run from MIDAS
            char str[256];
            //	  cm_transition(TR_STOP, 0, str, sizeof(str),
            //			TR_MTHREAD | TR_ASYNC, FALSE);

            CURL *curl;
            CURLcode res;
            curl = curl_easy_init();
            if (curl) {

              string cmd;
              cmd += "nmr-daq";
              cmd += "?cmd=Stop";

              FILE *f = fopen("/dev/null", "w+");

              curl_easy_setopt(curl, CURLOPT_URL, cmd.c_str());
              curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
              curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)f);

              res = curl_easy_perform(curl);

              if (res != CURLE_OK) {
                cm_msg(MERROR,
                       "stepper_loop",
                       "curl_easy_perform() failed: %s",
                        curl_easy_strerror(res));
              }

              // always cleanup
              fclose(f);
              curl_easy_cleanup(curl);
            }

            // Wait for the file to be written and exit.
            while (run_in_progress) {
              usleep(1000);
            }

          } else if (ss_idx == 0) {

            // Set up for the next data point.
            if (ino_stepper_type) {
              auto rc = ino_stepper->MoveCmForward(step_size);

            } else {

              auto rc = dio_stepper->MoveCmForward(step_size);
            }

            usleep(1.0e6 / event_rate_limit);
            ++num_events;
            ready_to_move = false;
            readout_listener->SetReady();

            // Make sure there isn't a trigger waiting
            stepper_listener->UnsetReady();
            for (int i = 0; i < 10; ++i) {
              readout_listener->HasTrigger();
            }

            cm_msg(MLOG, "trigger_loop",
                   "step: %i, %i",
                   ss_idx,
                   num_events);

          }
        }
      }
    }

    // If we aren't running, just sleep a bit.
    if (!run_in_progress) {
      usleep(25000);
      std::this_thread::yield();
    }
  }
}
