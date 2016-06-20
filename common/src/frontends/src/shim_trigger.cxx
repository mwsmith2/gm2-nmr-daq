/*****************************************************************************\

Name:   shim_trigger.cxx
Author: Matthias W. Smith
Email:  mwsmith2@uw.edu

About:  Implements a frontend that issues synchronized triggers
        to other frontends.

\*****************************************************************************/

//--- std includes ----------------------------------------------------------//
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

//--- other includes --------------------------------------------------------//
#include "midas.h"
#include "TFile.h"
#include "TTree.h"

//--- project includes ------------------------------------------------------//
#include "common.hh"
#include "sync_trigger.hh"
#include "frontend_utils.hh"

//--- global variables ------------------------------------------------------//
#define FRONTEND_NAME "Sync Trigger"
#define BANK_NAME "SHTR"

extern "C" {

  // The frontend name (client name) as seen by other MIDAS clients
  char *frontend_name = (char *)FRONTEND_NAME;
  char *bank_name = (char *)BANK_NAME;

  // The frontend file name, don't change it.
  char *frontend_file_name = (char*)__FILE__;

  // frontend_loop is called periodically if this variable is TRUE
  BOOL frontend_call_loop = FALSE;

  // A frontend status page is displayed with this frequency in ms.
  INT display_period = 1000;

  // maximum event size produced by this frontend
  INT max_event_size = 0x100;

  // maximum event size for fragmented events (EQ_FRAGMENTED)
  INT max_event_size_frag = 0x100;

  // buffer size to hold events
  INT event_buffer_size = 0x800;

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
      {FRONTEND_NAME,   // equipment name
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

// Anonymous namespace for my "global" variables.
namespace {
unsigned long long num_events;

// This is the trigger for the Measurements.
daq::SyncTrigger *readout_trigger;

// This is the trigger for the stepper motor movement.
daq::SyncTrigger *stepper_trigger;

boost::property_tree::ptree conf;

}

//--- Frontend Init ---------------------------------------------------------//
INT frontend_init()
{
  int rc = load_settings(frontend_name, conf);
  if (rc != SUCCESS) {
    // Error already logged in load_settings.
    return rc;
  }

  string trigger_addr = conf.get<string>("sync_trigger_addr");
  int trigger_port = conf.get<int>("fast_trigger_port");

  readout_trigger = new daq::SyncTrigger(trigger_addr, trigger_port);
  readout_trigger->SetName("readout-trigger");

  stepper_trigger = new daq::SyncTrigger(trigger_addr, trigger_port + 30);
  stepper_trigger->SetName("stepper-trigger");

  return SUCCESS;
}

//--- Frontend Exit ---------------------------------------------------------//
INT frontend_exit()
{
  delete readout_trigger;
  delete stepper_trigger;
  return SUCCESS;
}

//--- Begin of Run ---------------------------------------------------------//
INT begin_of_run(INT run_number, char *error)
{
  num_events = 0;

  readout_trigger->FixNumClients();
  readout_trigger->StartTriggers();
  stepper_trigger->FixNumClients();
  stepper_trigger->StartTriggers();
  return SUCCESS;
}

//--- End of Run -----------------------------------------------------------//
INT end_of_run(INT run_number, char *error)
{
  readout_trigger->StopTriggers();
  stepper_trigger->StopTriggers();
  return SUCCESS;
}

//--- Pause Run ------------------------------------------------------------//
INT pause_run(INT run_number, char *error)
{
  readout_trigger->StopTriggers();
  stepper_trigger->StopTriggers();
  return SUCCESS;
}

//--- Resuem Run -----------------------------------------------------------//
INT resume_run(INT run_number, char *error)
{

  readout_trigger->StartTriggers();
  stepper_trigger->StartTriggers();
  return SUCCESS;
}

//--- Frontend Loop --------------------------------------------------------//

INT frontend_loop()
{
  return SUCCESS;
}

//--------------------------------------------------------------------------//


//--- Trigger event routines -----------------------------------------------//

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

//--- Interrupt configuration ----------------------------------------------//

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

//--- Event readout --------------------------------------------------------//

INT read_trigger_event(char *pevent, INT off)
{
  return 0;

  DWORD *pdata;

  // And MIDAS output.
  bk_init32(pevent);

  // Send a small event to keep the Logger happy.
  bk_create(pevent, bank_name, TID_DWORD, &pdata);

  *pdata = num_events++;
  pdata += sizeof(num_events) / sizeof(DWORD);

  bk_close(pevent, pdata);

  return bk_size(pevent);
}

