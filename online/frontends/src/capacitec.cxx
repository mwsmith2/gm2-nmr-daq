/********************************************************************  \

Name:   capacitec.cxx
Author: Brendan Kiburg (adapted from example_sync_fe by Matthias W. Smith
                        and the work done at Fermilab by Paul Nebres)
Email:  kiburg@fnal.gov

About:  This frontend is synchorinzed to the SyncTrigger class in 
        shim_trigger, and is responible for pulling values from the odb and 
        writing them to the fast DAQ

        
\********************************************************************/

//--- std includes -------------------------------------------------//
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

//--- other includes -----------------------------------------------//
#include "midas.h"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"

//--- project includes ---------------------------------------------//
#include "sync_client.hh"

//--- globals ------------------------------------------------------//

#define FE_NAME "capacitec"

extern "C" {
  
  // The frontend name (client name) as seen by other MIDAS clients
  // fe_name , as above
  char * frontend_name = FE_NAME;
  
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
      {FE_NAME, //"capacitec", // equipment name
       { 10, 0,          // event ID, trigger mask 
         "BUF1",      // event buffer (use to be SYSTEM)
         EQ_POLLED |
	 EQ_EB,         // equipment type 
         0,             // not used 
         "MIDAS",       // format 
         TRUE,          // enabled 
         RO_RUNNING| RO_ODB,   // read only when running 
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

// @sync: begin boilerplate
daq::SyncClient *listener;
daq::SyncClient *listenerStepper;
// @sync: end boilderplate

//--- Frontend Init -------------------------------------------------//
INT frontend_init() 
{
  // @sync: begin boilerplate
  //DATA part
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
  listenerStepper = new daq::SyncClient(trigger_addr, trigger_port+30);
 
 // @sync: end boilderplate
  // Note that if no address is specifed the SyncClient operates
  // on localhost automatically.

  return SUCCESS;
}

//--- Frontend Exit ------------------------------------------------//
INT frontend_exit()
{
  // @sync: begin boilerplate
  delete listener;
  delete listenerStepper;

  // @sync: end boilerplate

  return SUCCESS;
}

//--- Begin of Run --------------------------------------------------*/
INT begin_of_run(INT run_number, char *error)
{
  // @sync: begin boilerplate
  listener->SetReady();
  listenerStepper->UnsetReady(); // don't move until we say so
  // @sync: end boilerplate

  return SUCCESS;
}

//--- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{
  listener->UnsetReady();
  listenerStepper->UnsetReady();
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
  
  // @sync: begin boilerplate
  // Checking HasTrigger() has a side effect of setting the trigger to false
  // 

  if (listener->HasTrigger()) {
    cout << "Received trigger\n"<<endl;
    return 1;
  } // if HasTrigger()
  
  // @sync: end boilerplate
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

INT read_trigger_event(char *pevent, INT off)
{
  //  int count = 0;
  char bk_name[10]; // Bank Name
  double *pdata;    // Place to store data
  
  // Initialize MIDAS BANK
  bk_init32(pevent); 
  // Get the time as an example
  sprintf(bk_name, "CTEC");
  bk_create(pevent, bk_name, TID_DOUBLE, &pdata);
  
  HNDLE hDB,hkey;
  cm_get_experiment_database(&hDB, NULL);
  static int count_cycles=0;
  bool first = 1;
  
  // ---  This is the stuff we need to move over ----- //

  struct CAPACITEC {
    float OL; //Input[8]
    float IU; //Input[9]
    float IL; //Input[10]
    float OU; //Input[11]
  } trolleyCap;

  db_find_key(hDB, 0, "/Equipment/Environment/Variables/Input", &hkey);
  if (hkey==NULL){printf("unable to find input key\n");}
  float input[12];
  
  for (auto i=0; i<12;++i){ input[i]=0;}
  int size=sizeof(input);
  printf("size of input is %d\n",size);

  db_get_data(hDB,hkey,&input,&size,TID_FLOAT);

  trolleyCap.OL=input[8];
  trolleyCap.IU=input[9];
  trolleyCap.IL=input[10];
  trolleyCap.OU=input[11];
   
  *(pdata++)=trolleyCap.OL;  
  *(pdata++)=trolleyCap.IU;
  *(pdata++)=trolleyCap.IL;
  *(pdata++)=trolleyCap.OU;

  if (first)
    {
      first=false;
      db_set_value(hDB, 0, "/Equipment/capacitec/Variables/X", 
		   &count_cycles, sizeof(count_cycles), 1, TID_INT); 
      count_cycles++;
    }

  bk_close(pevent, pdata);
  
  // @sync: begin boilerplate
  // Let the steppertrigger listener know we are ready to move to the next
  printf("set listener stepper ready\n");


  listenerStepper->SetReady();
  listener->SetReady();
  // @sync: end boilerplate

  return bk_size(pevent);
}
