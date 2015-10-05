/********************************************************************\

Name:   laser_tracker.cxx
Author: Brendan Kiburg (adapted from example_sync_fe by Matthias W. Smith
                        and the work done at Fermilab by Paul Nebres)
Email:  kiburg@fnal.gov

About:  This frontend is synchorinzed to the SyncTrigger class in 
        shim_trigger, and is responible for interacting with the 
        windows-based Spatial Analyzer software that controls the 
        laser tracker frontend
        
\********************************************************************/

//--- std includes -------------------------------------------------//
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <mutex>
using std::string;

//--- other includes -----------------------------------------------//
#include "midas.h"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"

//--- project includes ---------------------------------------------//
#include "sync_client.hh"

//--- globals ------------------------------------------------------//

#define FE_NAME "laser-tracker"

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

  void TriggerLaserTracker(float sleeptime, bool DIAG);
  
  int SetChannel(int channel, 
                 float newsetvalue, 
                 HNDLE hdb, 
                 HNDLE hKey, 
                 float sleeptime,
                 BOOL DIAG);
    
  // Equipment list

  EQUIPMENT equipment[] = 
    {
      {FE_NAME, //"Laser Tracker", // equipment name
       { 1, 0,          // event ID, trigger mask 
         "BUF2",      // event buffer (use to be SYSTEM)
         EQ_POLLED |
	 EQ_EB,         // equipment type 
         0,             // not used 
         "MIDAS",       // format 
         TRUE,          // enabled 
         RO_RUNNING, //| RO_ODB,   // read only when running 
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

  static FILE* f;
  static std::mutex m;
  if (listener->HasTrigger()) {
    
    printf("Listener has a trigger\n");
    // User: Issue trigger here.
    
    // In our case this means doing the set/reset sequence for the laser tracker hardware
    TriggerLaserTracker(0.10, true); // 0.5 seconds, with diagnostics

    f = fopen("/home/newg2/Applications/gm2-nmr/resources/laser-files/lock.txt","w");
    fputs("sample",f);
    fclose(f);
    // temporarily create the lock file so we can check if it exists
    // do this temp in the TriggerLaserTracker function
  }
  
  // @sync: end boilerplate
  
  // In our case we need to now poll for the lock file
  // when it exists, it's time to readout the event
  // otherwise, just 
  
  m.lock();
  f = fopen ("/home/newg2/Applications/gm2-nmr/resources/laser-files/lock.txt", "r");
  if (f !=NULL){
    fclose(f);
    if( remove( "/home/newg2/Applications/gm2-nmr/resources/laser-files/lock.txt" ) != 0 )
      perror( "Error deleting file" );
    else
      puts( "File successfully deleted" );
    m.unlock();
    return 1;
  }
  
  m.unlock();
  
  return 0;

  /*  
  bool LockFileExists = true; // change to actually look for the lock file
  static int counter = 0;
  if (counter++ == 5000){ // poll one hundred times, then declare us ready
    //LockFileExists){
    // delete the LockFile
    counter = 0;
    return 1;
    
    
  }
  return 0;
  */
  //  return listener->HasTrigger(); // User: Check device for event here.
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
  
  // @user: readout routine here.
  // And MIDAS output.
  bk_init32(pevent);

  // Get the time as an example
  sprintf(bk_name, "LTRK");
  bk_create(pevent, bk_name, TID_DOUBLE, &pdata);

  *(pdata++) = daq::systime_us() * 1.0e-6;


  // BEGIN Paul code
  
  HNDLE hDB;
  cm_get_experiment_database(&hDB, NULL);

  
  ////////////Testprgm///////////////
  FILE *ptr_file;
  double tilnum;
  char line[1000];
  char *til;
  char *sp;
  bool keepers[22] = {0,1,1,1,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0};
  //Check for lock file
  //grep for file?
  //potentially use [ -f file ]
  //loop to keep checking
  //copy file
  //if file found, cp it


  // Need new location on DAQ machine
  //Utilize Matthias' light sleep here while looping and looking for the creation of a lock file.

  //  ptr_file=fopen("//media//sf_VBSF//Main_p2-Meas.txt","r"); //open lock file
  ptr_file=fopen("/home/newg2/Applications/gm2-nmr/resources/laser-files/Main_p2-Meas.txt","r"); //open lock file
  if (!ptr_file) {
    printf("Error when opening file\n");
  }
  //Delete lock file
  //rm file

  static int count_cycles=0;
  bool first = 1;

  while (fgets(line,1000,ptr_file)!=NULL) {
    
    //std::cout<<"Copying data from file to MIDAS bank"<<std::endl;
    
    char rawline[1000];
    strcpy(rawline,line);
    til = strtok_r(line,",", &sp);
    int index = 0;
    while (til != NULL) {
      if (keepers[index++]==1) {
	if (index == 7) {
	  char temptil[64];
	  strcpy (temptil,til);
	  char* newtil;
	  double newtilnum;
	  newtil = strtok(temptil,"/: ");
	  while (newtil != NULL) {
	    newtilnum = atof(newtil);
	    *pdata++ = newtilnum*1000;
	    newtil = strtok(NULL,"/: ");
	    
	    
	  } 
	    
	}
	else if (index == 11) {
	  char temptil2[64];
	  strcpy (temptil2,til);
	  char *newtil2;
	  double newtilnum2;
	  newtil2 = strtok(temptil2,"Weather:T=C ");
	  while (newtil2 != NULL) {
	    newtilnum2 = atof(newtil2);
	    *pdata++ = newtilnum2*1000;
	    newtil2 = strtok(NULL,"Weather:T=C ");
	  }
	}
	else {
	  tilnum=atof(til);
	  *pdata++ = tilnum*1000;
	 
	}
	
	// take the first three and set them in the 

      } // (keepers index)
      til = strtok_r(NULL,",",&sp);
    }
    if (first)
	  {
	    first=false;
	    db_set_value(hDB, 0, "/Equipment/Laser Tracker/Variables/X", 
			 &count_cycles, sizeof(count_cycles), 1, TID_INT); 
	    count_cycles++;
	  }
  }
  fclose(ptr_file);
  
  
  // End Paul code
  

  bk_close(pevent, pdata);
  
  // @sync: begin boilerplate
  // Let the steppertrigger listener know we are ready to move to the next
  printf("set listener stepper ready\n");

  // TODO remove when syncronized
  sleep(0.5); // protect against huge data rates

  listenerStepper->SetReady();
  listener->SetReady();
  // @sync: end boilerplate

  return bk_size(pevent);
}

// input sleeptime in seconds
int SetChannel(int channel, float newsetvalue, HNDLE hDB, HNDLE hKey, float sleeptime, BOOL DIAG){
  
  //  return; // Don't do anything!
  INT size=0;
  float output[7]; 
  int i=0; for (; i<7; ++i){
    output[i]=99;
  }

  size = sizeof(output);
  
  db_set_value_index(hDB, 0, "/Equipment/Environment/Variables/Output", 
		      &newsetvalue, sizeof(newsetvalue), channel, TID_FLOAT, FALSE); 
   if (DIAG) {
     db_get_data(hDB, hKey, &output, &size, TID_FLOAT);
     printf("[5] equals %f, [6] equals %f\n", output[5], output[6]);
   }
   sleeptime *= 1000000.;
   usleep(sleeptime);
   return 0;
 }

void TriggerLaserTracker(float sleeptime=1.0, bool DIAG=false){
  HNDLE hDB, hKey;
  INT size;
  float output[7];
  int i = 0;
  for(;i <7; ++i) {
    output[i]=99;
  }

  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, "/Equipment/Environment/Variables/Output", &hKey);
  size = sizeof(output);
  db_get_data(hDB, hKey, &output, &size, TID_FLOAT);    
  
  if (DIAG)	{
    //  printf("size is %d\n",size);
    i = 0;
    for(; i < 7; ++i) {
      float tval = output[i];
      //printf("index %d = %f\n",i, tval);
    }
  }
  
  int channel_set =5;  // The Set signal   channel number
  int channel_reset =6;
  
  //send a Set Signal (Change channel 5 value to 1)
  SetChannel(channel_set,1.0, hDB, hKey, sleeptime, DIAG);
  
  SetChannel(channel_set,0.0, hDB, hKey, sleeptime, DIAG);
  
  SetChannel(channel_reset,1.0, hDB, hKey, sleeptime, DIAG);
  
  SetChannel(channel_reset,0.0, hDB, hKey,sleeptime, DIAG);


  // temporarily, create a lock file
}
