/********************************************************************\

  Name:         template_async_fe.c
  Created by:   Stefan Ritt, adapted by Brendan Kiburg

  Contents:     Template frontend for asynchronous data collection 

  Features: 

  How to use:  You need to modify the following parameters
              char *frontend_name = " YOUR FRONTEND NAME "
              equipment [] -->  equipment name
                           -->  equipment event id (should be a separate event from the sync daq)
                           -->  equipment type (EQ_PERIODIC for regular sampling)
                           -->  readout mode (RO_ALWAYS runs whenever FE runs, independent of MIDAS run status) 
              readout() --> This routine should be designed to access the hardware, and to store the 
                            relevant results in the /Equipment/YOUR FRONTEND NAME/Variables/YOUR VARIABLES HERE 
                            area of the odb
\********************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "mscb.h"
#include "midas.h"
#include "msystem.h"
#include "class/multi.h"
#include "class/generic.h"
#include "device/mscbdev.h"
#include "device/mscbhvr.h"


/*-- Globals -------------------------------------------------------*/
/* make frontend functions callable from the C framework */
 #ifdef __cplusplus
 extern "C" {
 #endif

/* The frontend name (client name) as seen by other MIDAS clients   */
   char *frontend_name = "Slow Readout FE";

   /* The equipment name, used in both the odb and the Equipment definition */
   const char *equip_name = "Default ASYNC";

/* The frontend file name, don't change it */
char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = TRUE;

/* a frontend status page is displayed with this frequency in ms    */
INT display_period = 1000;

/* maximum event size produced by this frontend */
INT max_event_size = 10000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = 10 * 10000;

INT read_periodic_event(char *pevent, INT off);

static INT readout(char *pevent,INT off);

/*-- Equipment list ------------------------------------------------*/

EQUIPMENT equipment[] = {

  {"Default ASYNC",               /* equipment name, defined above*/
    {1, 0,                     /* event ID, trigger mask */
     "BUF2",                  /* event buffer  -- used to be SYSTEM*/
     EQ_PERIODIC | EQ_EB,                   /* equipment type */
     0,                         /* event source */
     "MIDAS",                   /* format */
     TRUE,                      /* enabled */
     RO_ALWAYS,
     1500,                     /* read every 10 sec */
     0,                     /* event limit (0=no limit) */
     0,                         /* number of sub events */
     120,                        /* log history in 120 seconds */
     "", "", ""} ,
    readout,              /* readout routine */
    //read_periodic_event,			//call routine?
    },          

  {""}
};

#ifdef __cplusplus
 }
#endif


/*-- Dummy routines ------------------------------------------------*/

INT readout(char* pevent, INT off)
{

  // Set up MIDAS output bank. You should pick a different bank name (exactly 4 characters)
  DWORD *pdata;
  bk_init32(pevent);
  bk_create(pevent,"ASYN",TID_DWORD,&pdata);


  
// This section deals with access to the ODB
  HNDLE hDB,hKey;
 INT size;
 
 char keyPath[256];
 char* varName = "Secret";
 sprintf(keyPath,"/Equipment/%s/Variables/%s",equip_name,varName);
 
 cm_get_experiment_database(&hDB,NULL);
 db_find_key(hDB,0,keyPath,&hKey);
 int val = 0;
 size = sizeof(val);
 
 // This is where you would launch a call to your hardware readout if 
 // For the template, we just show an example where we grab the value from the frontend and increment
 // it so that we can see something changing
 // Use the odb key to grab the value for use in our Frontend

 db_get_data(hDB,hKey,&val,&size,TID_INT);

 // Put the computer timestamp in the data
 struct timeval t;
 gettimeofday(&t,NULL); 
 *pdata++ = (int)t.tv_sec;
 
 // Write the value to the MIDAS bank and increment the value
 printf("Value is %d at time %d\n",val,(int)t.tv_sec);
 *pdata++ = val++;

 // Actually update the odbkey. This is important so that the history file can automatically log the variable
 db_set_value(hDB,0,keyPath,&val,sizeof(val),1,TID_INT);

 bk_close(pevent,pdata);
 return bk_size(pevent);

}

INT poll_event(INT source[], INT count, BOOL test)
{

  sleep(2);
  printf("in the poll_event\n");
   return 1;
};
INT interrupt_configure(INT cmd, INT source[], PTYPE adr)
{
printf("in the interrupt_config\n");  
  return 1;
};

/*-- Function to define MSCB variables in a convenient way ---------*/

void mscb_define(char *submaster, char *equipment, char *devname, DEVICE_DRIVER *driver, 
                 int address, unsigned char var_index, char *name, double threshold)
{
   int i, dev_index, chn_index, chn_total;
   char str[256];
   float f_threshold;
   HNDLE hDB;

   cm_get_experiment_database(&hDB, NULL);
//printf(hDB+"\n");
   if (submaster && submaster[0]) {
      sprintf(str, "/Equipment/%s/Settings/Devices/%s/Device", equipment, devname);
      db_set_value(hDB, 0, str, submaster, 32, 1, TID_STRING);
      sprintf(str, "/Equipment/%s/Settings/Devices/%s/Pwd", equipment, devname);
      db_set_value(hDB, 0, str, "mscb110", 32, 1, TID_STRING);
   }

   /* find device in device driver */
   for (dev_index=0 ; driver[dev_index].name[0] ; dev_index++)
      if (equal_ustring(driver[dev_index].name, devname))
         break;

   if (!driver[dev_index].name[0]) {
      cm_msg(MERROR, "Device \"%s\" not present in device driver list", devname);
      return;
   }

   /* count total number of channels */
   for (i=chn_total=0 ; i<=dev_index ; i++)
      chn_total += driver[i].channels;

   chn_index = driver[dev_index].channels;
   sprintf(str, "/Equipment/%s/Settings/Devices/%s/MSCB Address", equipment, devname);
   db_set_value_index(hDB, 0, str, &address, sizeof(int), chn_index, TID_INT, TRUE);
   sprintf(str, "/Equipment/%s/Settings/Devices/%s/MSCB Index", equipment, devname);
   db_set_value_index(hDB, 0, str, &var_index, sizeof(char), chn_index, TID_BYTE, TRUE);

   if (threshold != -1) {
     sprintf(str, "/Equipment/%s/Settings/Update Threshold", equipment);
     f_threshold = (float) threshold;
     db_set_value_index(hDB, 0, str, &f_threshold, sizeof(float), chn_total, TID_FLOAT, TRUE);
   }

   if (name && name[0]) {
      sprintf(str, "/Equipment/%s/Settings/Names Input", equipment);
      db_set_value_index(hDB, 0, str, name, 32, chn_total, TID_STRING, TRUE);
   }

   /* increment number of channels for this driver */
   driver[dev_index].channels++;
}

/*-- Error dispatcher causing communiction alarm -------------------*/

void scfe_error(const char *error)
{
   char str[256];
   printf("why dis\n");
   strlcpy(str, error, sizeof(str));
   cm_msg(MERROR, "scfe_error", str);
   al_trigger_alarm("MSCB", str, "MSCB Alarm", "Communication Problem", AT_INTERNAL);
}


/*INT read_scaler_event(char *pevent,INT off){
printf("int the read scaler\n");	
	return 1;
}*/
/**********************added mbank filler**********************************
INT read_scaler_event(char *pevent, INT off)
{
   float *pdata;
   int fd, a, addr; 
   MSCB_INFO info;
   addr=1;
   fd = 0;
   status = mscb_info(fd, (unsigned short) addr, &info);
   if (status == MSCB_CRC_ERROR) {
      puts("CRC Error");
      return -1;
   } else if (status != MSCB_SUCCESS) {
      puts("No response from node");
      return -1;
   }
//i need to define info
//   mscb_info(fd, (unsigned short) addr, &info);
   // init bank structure 
   bk_init(pevent);

   // create SCLR bank 
   bk_create(pevent, "SCLR", TID_FLOAT, &pdata);

   // read scaler bank (CAMAC) 
   for (a = 0; a < N_SCLR; a++)
	mscbdev_get(info, 0,   pvalue);    

    
   // close SCLR bank 
   bk_close(pevent, pdata);

   // return event size in bytes 
   return bk_size(pevent);
}
*/

/**********************added mbank filler**********************************
INT read_periodic_event(char *pevent, INT off)
{
   float *pdata;
   int fd, a, addr; 
   MSCB_INFO info;
   addr=1;
   fd = 0;
   status = mscb_info(fd, (unsigned short) addr, &info);
   if (status == MSCB_CRC_ERROR) {
      puts("CRC Error");
      return -1;
   } else if (status != MSCB_SUCCESS) {
      puts("No response from node");
      return -1;
   }
//i need to define info
//   mscb_info(fd, (unsigned short) addr, &info);
   // init bank structure 
   bk_init(pevent);

   // create SCLR bank 
   bk_create(pevent, "SCLR", TID_FLOAT, &pdata);

   // read scaler bank (CAMAC) 
   for (a = 0; a < N_SCLR; a++)
	mscbdev_get(info, 0,   pvalue);    

    
   // close SCLR bank 
   bk_close(pevent, pdata);

   // return event size in bytes 
   return bk_size(pevent);
}
*/

/*-- Frontend Init -------------------------------------------------*/

INT frontend_init()
{
  printf(" init 1 \n");

  //printf("init1\n");
  /* set error dispatcher for alarm functionality */
  //mfe_set_error(scfe_error);
  
  printf(" init 2 \n");
  /* set maximal retry count */
  mscb_set_max_retry(100);
  
  /*---- set correct ODB device addresses ----*/
  
  /*
  mscb_define("mscb110", "Environment 2", "SCS2001", mscb_driver, 0x1, 0, "Temperature2 1", 0.01);
  mscb_define("mscb110", "Environment 2", "SCS2001", mscb_driver, 0x1, 1, "Temperature2 2", 0.01);
  mscb_define("mscb110", "Environment 2", "SCS2001", mscb_driver, 0x1, 2, "Temperature2 3", 0.01);
  mscb_define("mscb110", "Environment 2", "SCS2001", mscb_driver, 0x1, 3, "Temperature2 4", 0.01);
  mscb_define("mscb110", "Environment 2", "SCS2001", mscb_driver, 0x1, 4, "Temperature2 5", 0.01);
  mscb_define("mscb110", "Environment 2", "SCS2001", mscb_driver, 0x1, 5, "Temperature2 6", 0.01);
  mscb_define("mscb110", "Environment 2", "SCS2001", mscb_driver, 0x1, 6, "Temperature2 7", 0.01);
  mscb_define("mscb110", "Environment 2", "SCS2001", mscb_driver, 0x1, 7, "Temperature2 8", 0.01);

  mscb_define("mscb110", "Environment 2", "SCS2001", mscb_driver, 0x1, 24, "Capacitec2 1", 0.001);
  mscb_define("mscb110", "Environment 2", "SCS2001", mscb_driver, 0x1, 25, "Capacitec2 2", 0.001);
  mscb_define("mscb110", "Environment 2", "SCS2001", mscb_driver, 0x1, 26, "Capacitec 3", 0.001);
  mscb_define("mscb110", "Environment 2", "SCS2001", mscb_driver, 0x1, 27, "Capacitec 4", 0.001);
  //mscb_define("mscb110", "Environment", "SCS2001", mscb_driver, 0x1, 8, "IO ch0", 0.1);
  //mscb_define("mscb110", "Environment", "SCS2001", mscb_driver, 0x1, 16, "IO ch2", 0.1);
  //mscb_define("mscbxxx.psi.ch", "Environment", "SCS2001", mscb_driver, 1, 1, "Temperature 2", 0.1);
  //mscb_define("mscbxxx.psi.ch", "Environment", "SCS2001", mscb_driver, 1, 2, "Temperature 3", 0.1);
  //mscb_define("mscbxxx.psi.ch", "Environment", "SCS2001", mscb_driver, 1, 3, "Temperature 4", 0.1);
  //printf("init2\n");
  //  */
  
  return CM_SUCCESS;
}
   
/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
   return CM_SUCCESS;
}

/*-- Begin of Run --------------------------------------------------*/

INT begin_of_run(INT run_number, char *error)
{
   return CM_SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/

INT end_of_run(INT run_number, char *error)
{
   return CM_SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/

INT pause_run(INT run_number, char *error)
{
   return CM_SUCCESS;
}

/*-- Resuem Run ----------------------------------------------------*/

INT resume_run(INT run_number, char *error)
{
   return CM_SUCCESS;
}

//fill mbank from /equip/environ/variables
INT read_periodic_event(char *pevent, INT off){
  
  HNDLE hDB;
  cm_get_experiment_database(&hDB, NULL);
  
  HNDLE hkey;
  float temp1[3];
  INT size;
  // get key handle for temp
  db_find_key(hDB, 0, "/Equipment/Environment/Variables/Input", &hkey);
  // return temp
  size = sizeof(temp1);
  db_get_data(hDB, hkey, &temp1, &size, TID_FLOAT);

  //char *pevent;
  bk_init(pevent);
  float *pdata;
  *pdata = temp1[0];
   // create SCLR bank 
   bk_create(pevent, "NEW1", TID_FLOAT, &pdata);

     
// *pdata = 29.3;
printf("%f\n",temp1[0]);
    
   // close SCLR bank 
   bk_close(pevent, pdata);
printf("eo fB\n");
   return bk_size(pevent);


}

/*-- Frontend Loop -------------------------------------------------*/

INT frontend_loop()
{  
   /* don't eat up all CPU time in main thread */
//printf("in the fe loop\n");
   //fillBank();
   ss_sleep(100);

   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
	
