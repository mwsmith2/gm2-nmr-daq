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

bool p1=false;
bool p2=false;
double p2_offset = 1.971; // p2 leads p1 by this angle, https://muon.npl.washington.edu/elog/g2/General+Field+Team/209
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

  void TriggerLaserTracker(float sleeptime, bool DIAG); // Note: obsolete from a time we were using a hardware trigger
  void ResetLaserTracker(int channel, float sleeptime, bool DIAG); // Note: obsolete from a time we were using a hardware trigger
  
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
       { 10, 0,          // event ID, trigger mask 
         "BUF2",      // event buffer (use to be SYSTEM)
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
  // Try to get the P1/P2 definition
  db_find_key(hDB,0,"/Experiment/Run Parameters/Laser Tracker Point", &hkey);
  
  char lp[32];
  if (hkey){
    size = sizeof(lp);
    db_get_data(hDB,hkey,lp,&size,TID_STRING);
    printf("Identified the laser tracker target as %s\n",lp);
    if (lp[0]== 'p' || lp[0]=='P'){
      if (lp[1]=='1') p1 =true;
      if (lp[1]=='2') p2 = true;
    }
  }

  else
    printf("no laser tracker source point found\n");

  if (p1) printf("USING P1\n");
  if (p2) printf("USING P2\n");
  sleep(2);

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
  // Find out what point is being used and hold onto that info
  HNDLE hDB, hkey;
  char lp[32];
  int size;
    
  cm_get_experiment_database(&hDB, NULL);

  // Try to get the P1/P2 definition
  db_find_key(hDB,0,"/Experiment/Run Parameters/Laser Tracker Point", &hkey);
  
  if (hkey){
    size = sizeof(lp);
    db_get_data(hDB,hkey,lp,&size,TID_STRING);
    printf("Identified the laser tracker target as %s\n",lp);
    if (lp[0]== 'p' || lp[0]=='P'){
      if (lp[1]=='1') p1 =true;
      if (lp[1]=='2') p2 = true;
    }
  }

  else
    printf("no laser tracker source point found\n");
  
  if (1){
    if (p1) printf("USING P1\n");
    else if (p2) printf("USING P2\n");
    else printf("NOT USING EITHER P1 OR P2\n");
    sleep(2);
  }
  // We should also keep track of which point is being used
  

  // If lock file exists, delete it so that the laser tracker will trigger
  static FILE* f;  
  f = fopen ("/home/me/portal/Export2.txt", "r");
  if (f !=NULL){
    fclose(f);
    // try to remove it                                                        
    usleep(10000);
    if( remove( "/home/me/portal/Export2.txt" ) != 0 )
      perror( "Error deleting lock file" );
    else
      puts( "Begin of Run: Export2.txt Lock file successfully deleted" );
  }

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
  p1=false;
  p2=false;
  return SUCCESS;
}

//--- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error)
{
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
    
    printf("\t\t\t\t\tListener has a trigger\n");
    
    m.lock();
    f = fopen ("/home/me/portal/Export2.txt", "r");
    if (f !=NULL ){
      fclose(f);
      
      usleep(10000);
      
      if (remove("/home/me/portal/Export2.txt") != 0) {
	perror( "Error deleting lock file" );
	return 0;

      } else {
	
	puts( "Lock file successfully deleted" );
      }
      
      m.unlock();
      return 1;

    } else { // no lock file found, forward the trigger anyways
      
      m.unlock();
      return 1;
    }
    
    m.unlock();
    
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
  sprintf(bk_name, "LTRK");
  bk_create(pevent, bk_name, TID_DOUBLE, &pdata);

  
  HNDLE hDB;
  cm_get_experiment_database(&hDB, NULL);
  static int count_cycles=0;
  bool first = 1;
  
  bool readRealData = (p1 || p2);


  std::string::size_type sz;
  FILE *ptr_file;
  char line[1000];
  char *til;
  char *sp;

  struct LaserLine {
    double r;
    double theta;
    double z;
    double t; // time
  } ;

  const int nLaserLines=1;
  LaserLine dataLine[nLaserLines]; // array of laserlines based on how many lines we expect to see in the export file
  int lineNumber =0;

  // if we are pointing at a point, then try to read a file
  if (readRealData){
    do {
      ptr_file = fopen("/home/me/portal/Export2.txt", "r");
      usleep(10000);
    } while (ptr_file == NULL);
    
    fclose(ptr_file);
    
    // Open Data File (we have already checked for lock file)
    ptr_file=fopen("/home/me/portal/Export1.txt","r"); 
    if (!ptr_file) {
      printf("Error when opening file\n");
    }
    
    // LOOP over valid lines in the file
    
    lineNumber = 0;
    while (fgets(line,1000,ptr_file)!=NULL) {
      string sline(line);
      stringstream ss(sline);
      cout << "line is" << sline <<endl;    
      int tokenNumber =0;    
      string token;
      while (getline(ss,token,','))
	{
	  //	cout << "\t" << token << endl;
	  stringstream stream_token;
	  stream_token<<setprecision(10);
	  stream_token<<token;
	  
	  double value;
	  stream_token >> value;
	  cout <<value<<endl;
	  
	  // throw out the label and the time
	  switch (tokenNumber){
	  case 1:
	    dataLine[lineNumber].r = value;
	  case 2:
	    dataLine[lineNumber].theta = value;
	  case 3: 
	    dataLine[lineNumber].z = value;
	  default:
	    break;
	  } //switch case
	  ++ tokenNumber;
	} //loop over tokens
      ++lineNumber;
    }//loop over lines in the file
  } // end if (readRealData)

  else{ // no file handling and simply put in dummy values

    for (int line =0;line<nLaserLines;++line){
      dataLine[line].r = -1;
      dataLine[line].theta =-1;
      dataLine[line].z = -1;
    }
  }
  
  //Regardless of how we got here, stuff the data into the structures
  
  // Include theta conversion here
  for (int i=0;i<nLaserLines;++i)
    {
      cout << "r,theta,z "<<dataLine[i].r<<","<<dataLine[i].theta<<","<<dataLine[i].z<<endl;
      LaserLine currLine = dataLine[i];
      LaserLine p1_data = dataLine[i];
      LaserLine p2_data = dataLine[i];

      if (p1) {
	p1_data.theta = 360 + (-1*p1_data.theta); //convert to clockwise
	
	p2_data.r = 0;
	p2_data.z = 0;
	p2_data.t = 0;
	p2_data.theta = p1_data.theta+p2_offset;
	
	if (p2_data.theta > 360) p2_data.theta =p2_data.theta - 360;
      }
      else if (p2){ // convert to clockwise
	p2_data.theta = 360 + (-1*p2_data.theta);
	p1_data.r=0;
	p1_data.z=0;
	p1_data.t=0;
	//derive p1
	
	p1_data.theta = p2_data.theta - p2_offset;
	if (p2_data.theta <0) p1_data.theta =p1_data.theta + 360;
      }
      else {
	printf("Neither is used, both points will be the same!");
      }
      /*
       *(pdata++)=dataLine[i].r;
       *(pdata++)=360 + (-1*p2_data.theta);
       *(pdata++)=dataLine[i].z;
       */


      //Write it out to pdata
       //Use p1/p2 knowledge to make adjustments
       *(pdata++)=p1_data.r;
       *(pdata++)=p1_data.theta;
       *(pdata++)=p1_data.z;

       *(pdata++)=p2_data.r;
       *(pdata++)=p2_data.theta;
       *(pdata++)=p2_data.z;
       
     }

  if (first)
    {
      first=false;
      db_set_value(hDB, 0, "/Equipment/laser-tracker/Variables/X", 
		   &count_cycles, sizeof(count_cycles), 1, TID_INT); 
      count_cycles++;
    }
 
  // only delete the files if you opened them
  if (readRealData) 
    {
      fclose(ptr_file); 
      
      
      if( remove( "/home/me/portal/Export1.txt" ) != 0 ) {
	
	perror( "Error deleting file" );
	
      } else {
	
	puts( "File successfully deleted" );
      }
    } //

  // End Paul code
  

  bk_close(pevent, pdata);
  
  // @sync: begin boilerplate
  // Let the steppertrigger listener know we are ready to move to the next
  printf("set listener stepper ready\n");


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
         printf("\t\t[1] equals %f, [2] equals %f\n", output[1], output[2]);
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
  
  int channel_set = 1;  // The Set signal   channel number
  int channel_reset = 2;
  
  //send a Set Signal (Change channel 5 value to 1)
  SetChannel(channel_set,1.0, hDB, hKey, sleeptime, DIAG);
  SetChannel(channel_set,0.0, hDB, hKey, 0.001, DIAG);


  // temporarily, create a lock file
}

void ResetLaserTracker(int channel = 2,float sleeptime=1.0, bool DIAG=false){
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
      //      printf("index %d = %f\n",i, tval);
    }
  }
  
  
  //send a Set Signal (Change channel 5 value to 1)
  SetChannel(channel,1.0, hDB, hKey, sleeptime, DIAG);
  SetChannel(channel,0.0, hDB, hKey, 0.0001, DIAG);
  
}
