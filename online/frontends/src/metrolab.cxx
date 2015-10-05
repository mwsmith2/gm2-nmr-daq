/*******************************************************************\

Name: Metrolab_client.cxx
Author: Luke P. K. Baines (The Beefeater)
Email: Baines-L-10@kcs.org.uk

\******************************************************************/

// ---std includes -----------------------------------------------//
#include <sys/socket.h>
#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>
using namespace std;

//--- other includes -----------------------------------------------//          
#include "midas.h"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
#include <zmq.hpp>

 //--- project includes ---------------------------------------------//          
#include "sync_client.hh"

 //--- globals ------------------------------------------------------//          

#define FRONTEND_NAME "metrolab"

extern "C" {

  // The frontend name (client name) as seen by other MIDAS clients             
  char *frontend_name = (char*)FRONTEND_NAME;

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
      {FRONTEND_NAME,  // equipment name                     
       {11, 0,         // event ID, trigger mask
        "SYSTEM",      // event buffer (use to be SYSTEM)
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
daq::SyncClient *listener = nullptr;
zmq::socket_t *metrolab_sck;
// @sync: end boilderplate

std::queue<std::string> data_queue;
std::string metrolab_addr;

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

  // not sure that metrolab needs to by synced.
  // listener = new daq::SyncClient(trigger_addr, trigger_port);

  // @sync: end boilderplate                                                  
  // Note that if no address is specifed the SyncClient operates                
  // on localhost automatically.        

  return SUCCESS;
}

//--- Frontend Exit ------------------------------------------------//          
INT frontend_exit()
{
  // @sync: begin boilerplate                                                   
  if (listener != nullptr) {
    delete listener;
  }
  // @sync: end boilerplate                                                     

  return SUCCESS;
}

//--- Begin of Run --------------------------------------------------//         
INT begin_of_run(INT run_number, char *error)
{
  // @sync: begin boilerplate
  if (listener != nullptr) {
    listener->SetReady();
  }
  // @sync: end boilerplate       

  HNDLE hDB, hkey;
  INT status, tmp;
  char str[256], filename[256];
  int size;

  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, "Params/metrolab-addr", &hkey);

  if (hkey) {
    size = sizeof(str);
    db_get_data(hDB, hkey, str, &size, TID_STRING);
    metrolab_addr = std::string(str);
  }
  
  auto msg = std::string("Metrolab socket listening on ") + metrolab_addr;
  cm_msg(MINFO, frontend_name, msg.c_str());

  metrolab_sck = new zmq::socket_t(daq::msg_context, ZMQ_SUB);

  // Set some useful options like timeout (so it doesn't block forever).
  int timeout = 250; // in ms
  metrolab_sck->setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

  timeout = 0; // in seconds
  metrolab_sck->setsockopt(ZMQ_LINGER, &timeout, sizeof(timeout));

  // Connect to the server socket and subscribe to all messages.
  cout << "Connecting metrolab socket." << endl;
  metrolab_sck->connect(metrolab_addr);
  metrolab_sck->setsockopt(ZMQ_SUBSCRIBE, "", 0);

  return SUCCESS;
}

//--- End of Run -----------------------------------------------------//        
INT end_of_run(INT run_number, char *error)
{
  // @sync: begin boilerplate                                                  
  if (listener != nullptr) {
    listener->UnsetReady();
  }
  // @sync: end boilerplate       

  delete metrolab_sck;
  return SUCCESS;
}

//--- Pause Run -----------------------------------------------------*/        
INT pause_run(INT run_number, char *error){
  return SUCCESS;
}

//--- Resume Run ----------------------------------------------------*/       
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
  static zmq::message_t msg(256);
  bool rc;
  int pollcount = 0;
  int pollmax = 100;

  // fake calibration                                                           
  if (test) {
    for (i = 0; i < count; i++) {
      usleep(10);
    }
    return 0;
  }

  // @sync: begin boilerplate                                                  
  
  if (listener != nullptr) {
    if (listener->HasTrigger()) {
      // User: Issue trigger here.
      // Note that HasTrigger only returns true once.  It resets the
      // trigger when it reads true to avoid multiple reads.
    }
    // @sync: end boilerplate
  }

  // Check for event, add to queue if found.
  do {

    try {
      rc = metrolab_sck->recv(&msg, ZMQ_DONTWAIT);

    } catch (...) {

      continue;
    }
    usleep(10);

  } while ((rc == false) && (++pollcount < pollmax));

  if (rc == true) {
    data_queue.push(std::string(msg.data()));
  }

  if (data_queue.size() > 0) {

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
  int count = 0;
  char bk_name[10];
  double *pdata;

  double magnetic_field;
  string field_unit;
  double time_stamp;

  // @user: readout routine here.                                              
  // And MIDAS output.                                                          
  bk_init32(pevent);

  // Get the time as an example                                                
  sprintf(bk_name, "MTRL");

  bk_create(pevent, bk_name, TID_DOUBLE, &pdata);

  istringstream iss(data_queue.front());
  iss >> magnetic_field >> field_unit >> time_stamp;

  *(pdata++) = time_stamp;
  *(pdata++) = magnetic_field;
  
  // Field unit is either Tesla or frequency in Mhz.
  if (field_unit == std::string("T")) {
    *(pdata++) = 100.0;
  } else {
    *(pdata++) = -100.0;
  }

  bk_close(pevent, pdata);

  // Pop the event now that we've read it.
  data_queue.pop();

  // @sync: begin boilerplate                                                  
  // Let the trigger listener know we are ready for the next event.  
  if (listener != nullptr) {
    listener->SetReady();
  }
  // @sync: end boilerplate                                                     

  return bk_size(pevent);
};
