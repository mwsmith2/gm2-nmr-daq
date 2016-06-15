/**************************************************************************** \

Name:   capacitec.cxx
Author: Brendan Kiburg (adapted from example_sync_fe by Matthias W. Smith
                        and the work done at Fermilab by Paul Nebres)
Email:  kiburg@fnal.gov

About:  This frontend is synchorinzed to the SyncTrigger class in
        shim_trigger, and is responible for pulling values from the odb and
        writing them to the fast DAQ


\*****************************************************************************/

//--- std includes ----------------------------------------------------------//
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mutex>
#include <iostream>
#include <sstream>
#include <string>

using std::string;
using namespace std;

//--- other includes --------------------------------------------------------//
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>
#include "midas.h"

//--- project includes ------------------------------------------------------//
#include "sync_client.hh"
#include "frontend_utils.hh"

//--- globals ---------------------------------------------------------------//
#define FRONTEND_NAME "Capacitec"

extern "C" {

  // The frontend name (client name) as seen by other MIDAS clients
  char *frontend_name = (char *)FRONTEND_NAME;

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
      {FRONTEND_NAME, //"capacitec", // equipment name
       { 10, 0,          // event ID, trigger mask
         "BUF1",      // event buffer (use to be SYSTEM)
         EQ_POLLED |
	 EQ_EB,         // equipment type
         0,             // not used
         "MIDAS",       // format
         TRUE,          // enabled
         RO_RUNNING| RO_ODB,   // read only when running
         100,            // poll every 100ms
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

boost::property_tree::ptree conf;

daq::SyncClient *listener_trigger;
daq::SyncClient *listener_stepper;


//--- Frontend Init -------------------------------------------------------//
INT frontend_init()
{
  int rc = load_settings(frontend_name, conf);
  if (rc != SUCCESS) {
    return rc;
  }

  listener_trigger = new daq::SyncClient(std::string(frontend_name),
                                         conf.get<string>("sync_trigger_addr"),
                                         conf.get<int>("fast_trigger_port"));

  listener_stepper = new daq::SyncClient(std::string(frontend_name),
                                         conf.get<string>("sync_trigger_addr"),
                                         conf.get<int>("fast_trigger_port") + 30);

  cm_msg(MDEBUG, "init_frontend", "success");
  return SUCCESS;
}

//--- Frontend Exit ------------------------------------------------------//
INT frontend_exit()
{
  delete listener_trigger;
  delete listener_stepper;

  return SUCCESS;
}

//--- Begin of Run --------------------------------------------------//
INT begin_of_run(INT run_number, char *error)
{
  listener_trigger->SetReady();
  listener_stepper->UnsetReady();

  return SUCCESS;
}

//--- End of Run ----------------------------------------------------//
INT end_of_run(INT run_number, char *error)
{
  listener_trigger->UnsetReady();
  listener_stepper->UnsetReady();

  return SUCCESS;
}

//--- Pause Run -----------------------------------------------------//
INT pause_run(INT run_number, char *error)
{
  return SUCCESS;
}

//--- Resuem Run ----------------------------------------------------//
INT resume_run(INT run_number, char *error)
{
  return SUCCESS;
}

//--- Frontend Loop -------------------------------------------------//

INT frontend_loop()
{
  return SUCCESS;
}

//-------------------------------------------------------------------//

/********************************************************************\

  Readout routines for different events

\********************************************************************/

//--- Trigger event routines ----------------------------------------//

INT poll_event(INT source, INT count, BOOL test) {

  static unsigned int i;

  // fake calibration
  if (test) {
    for (i = 0; i < count; i++) {
      usleep(10);
    }
    return 0;
  }

  // Checking HasTrigger() has a side effect of setting the trigger to false
  if (listener_trigger->HasTrigger()) {
    cm_msg(MDEBUG, "poll_event", "Received trigger");
    return 1;
  }

  return 0;
}

//--- Interrupt configuration -----------------------------------------------//
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

//--- Event readout ---------------------------------------------------------//
INT read_trigger_event(char *pevent, INT off)
{
  HNDLE hDB, hkey;
  char bk_name[10]; // Bank Name
  double *pdata;    // Place to store data
  float input[12];
  bool first = 1;
  static int count_cycles = 0;

  cm_get_experiment_database(&hDB, NULL);

  // Initialize MIDAS BANK
  bk_init32(pevent);

  sprintf(bk_name, "CTEC");
  bk_create(pevent, bk_name, TID_DOUBLE, (void **)&pdata);

  // This is the stuff we need to move over.
  struct CAPACITEC {
    float OL; //Input[8]
    float IU; //Input[9]
    float IL; //Input[10]
    float OU; //Input[11]
  } trolleyCap;

  db_find_key(hDB, 0, "/Equipment/MSCB Cart/Variables/Input", &hkey);

  if (hkey == NULL) {
    cm_msg(MERROR, "read_trigger_event", "unable to find input key");
  }

  for (uint i = 0; i < 12; ++i) {
    input[i] = 0;
  }

  int size = sizeof(input);

  db_get_data(hDB, hkey, &input, &size, TID_FLOAT);

  trolleyCap.OL = input[8];
  trolleyCap.IU = input[9];
  trolleyCap.IL = input[10];
  trolleyCap.OU = input[11];

  *(pdata++) = trolleyCap.OL;
  *(pdata++) = trolleyCap.IU;
  *(pdata++) = trolleyCap.IL;
  *(pdata++) = trolleyCap.OU;

  if (first) {
    char str[128];
    int size;

    sprintf(str, "/Equipment/%s/Variables/X", frontend_name);
    size = sizeof(count_cycles);
    db_set_value(hDB, 0, str, &count_cycles, size, 1, TID_INT);

    first = false;
    count_cycles++;
  }

  bk_close(pevent, pdata);

  listener_stepper->SetReady();
  listener_trigger->SetReady();

  return bk_size(pevent);
}
