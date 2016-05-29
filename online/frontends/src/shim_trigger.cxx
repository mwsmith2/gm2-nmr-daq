/********************************************************************\

Name:   shim_trigger.cxx
Author: Matthias W. Smith
Email:  mwsmith2@uw.edu

About:  Implements a frontend that issues synchronized triggers
        to other frontends.
        
\********************************************************************/

//--- std includes -------------------------------------------------//
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <cmath>
#include <ctime>
using std::string;
using std::cout;
using std::endl;

//--- other includes -----------------------------------------------//
#include "midas.h"
#include "TFile.h"
#include "TTree.h"

//--- project includes ---------------------------------------------//
#include "common.hh"
#include "sync_trigger.hh"

//--- globals ------------------------------------------------------//

extern "C" {
  
  // The frontend name (client name) as seen by other MIDAS clients
  char *frontend_name = (char*) "fast-sync-trigger";

  // The frontend file name, don't change it.
  char *frontend_file_name = (char*) __FILE__;
  
  // frontend_loop is called periodically if this variable is TRUE
  BOOL frontend_call_loop = FALSE;
  
  // A frontend status page is displayed with this frequency in ms.
  INT display_period = 1000;
  
  // maximum event size produced by this frontend
  INT max_event_size = 0x80000; // 80 kB

  // maximum event size for fragmented events (EQ_FRAGMENTED)
  INT max_event_size_frag = 0x80000; 
  
  // buffer size to hold events
  INT event_buffer_size = 0x800000; // 
  
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
      {"fast-sync-trigger", // equipment name 
       { 10, 0,         // event ID, trigger mask 
         "BUF1",        // event buffer 
         EQ_POLLED,     // equipment type 
         0,             // not used 
         "MIDAS",       // format 
         TRUE,          // enabled 
         RO_RUNNING |   // read only when running 
         RO_ODB,        // and update ODB 
         10,            // poll for 10ms 
         0,             // stop run after this event limit 
         0,             // number of sub events 
         0,             // don't log history 
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
const char *const mbank_name = (char *)"SHTR";
unsigned long long num_events;

// This is the trigger for the Measurements

daq::SyncTrigger *readout_trigger;
  //Brendan Adding another SyncTrigger in order to coordinate the stepper trigger separately
  // Note -- Everywhere below we repeat everything that happens to "trigger" for "stepper_trigger", except we bind it to a different port (trigger's port +1)
daq::SyncTrigger *stepper_trigger;

}

//--- Frontend Init -------------------------------------------------//
INT frontend_init() 
{
  //DATA part
  HNDLE hDB, hkey;
  INT status, tmp;
  char str[256], filename[256];
  int size;
  
  // Get the config directory and file.
  cm_get_experiment_database(&hDB, NULL);
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

  readout_trigger = new daq::SyncTrigger(trigger_addr, trigger_port);
  readout_trigger->SetName("readout-trigger");

  stepper_trigger = new daq::SyncTrigger(trigger_addr, trigger_port + 30);
  stepper_trigger->SetName("stepper-trigger");

  return SUCCESS;
}

//--- Frontend Exit ------------------------------------------------//
INT frontend_exit()
{
  delete readout_trigger;
  delete stepper_trigger;
  return SUCCESS;
}

//--- Begin of Run --------------------------------------------------*/
INT begin_of_run(INT run_number, char *error)
{
  num_events = 0;

  readout_trigger->FixNumClients();
  readout_trigger->StartTriggers();
  stepper_trigger->FixNumClients();
  stepper_trigger->StartTriggers();
  return SUCCESS;
}

//--- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{
  readout_trigger->FixNumClients(false);
  readout_trigger->StopTriggers();
  stepper_trigger->FixNumClients(false);
  stepper_trigger->StopTriggers();
  return SUCCESS;
}

//--- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error)
{
  readout_trigger->StopTriggers();
  stepper_trigger->StopTriggers();
  return SUCCESS;
}

//--- Resuem Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error)
{

  readout_trigger->StartTriggers();
  stepper_trigger->StartTriggers();
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
  unsigned int i;
  static int poll_number = 0;

  // fake calibration
  if (test) {
    for (i = 0; i < count; i++) {
      usleep(10);
    }
    return 0;
  }
  
  if (poll_number++ % 100 == 0) {

    return 1;

  } else {

    return 0;
  }
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
  return 0;

  DWORD *pdata;

  // And MIDAS output.
  bk_init32(pevent);

  // Copy the shimming trolley data.
  bk_create(pevent, mbank_name, TID_DWORD, &pdata);

  *pdata = num_events++;
  pdata += sizeof(num_events) / sizeof(DWORD);

  bk_close(pevent, pdata);

  return bk_size(pevent);
}

