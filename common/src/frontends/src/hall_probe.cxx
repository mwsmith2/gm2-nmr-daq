/**************************************************************************** \

Name:   Hall Probe Platform
Author: Matthias Smith
Email:  mwsmith2@uw.edu

About:  Aggregates the data sources for the radial field measurement
    into a single front-end. It reads out the voltage of the Keithley
    2100, the current on the Yokogawa, and the temperature of the
    Yoctopuce PT100.

\*****************************************************************************/


//--- std includes ----------------------------------------------------------//
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

//--- other includes --------------------------------------------------------//
#include "yocto_api.h"
#include "yocto_temperature.h"
#include "midas.h"

//--- globals ---------------------------------------------------------------//

#define FRONTEND_NAME "Hall Probe"

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
       {15, 15,         // event ID, trigger mask
        "SYSTEM",      // event buffer (use to be SYSTEM)
        EQ_PERIODIC,   // equipment type
        0,             // not used
        "MIDAS",       // format
        TRUE,          // enabled
        RO_ALWAYS |    // read only when running
        RO_ODB,        // and update ODB
        1000,          // read every 1s
        0,             // stop run after this event limit
        0,             // number of sub events
        1,             // don't log history
        "", "", "",
       },

       read_trigger_event, // readout routine
       NULL,
       NULL
      },

      {""}
    };

} //extern C

typedef struct {
  char pt100_dev[128];
} HALL_PROBE_PLATFORM_SETTINGS;

HALL_PROBE_PLATFORM_SETTINGS hall_probe_platform_settings;

#define HALL_PROBE_PLATFORM_SETTINGS_STR "\
PT100 Device Name = STRING : [128] PT100MK1-1DA0A\n\
"

YTemperature *temp_probe;
int keithley_port;
int yokogawa_port;

//--- Frontend Init -------------------------------------------------//
INT frontend_init()
{
  HNDLE hDB, hkey;
  INT status, tmp;
  char str[256], filename[256];
  int size;

  std::string err;

  cm_get_experiment_database(&hDB, NULL);

  sprintf(str, "/Equipment/%s/Settings", FRONTEND_NAME);
  status = db_create_record(hDB, 0, str, HALL_PROBE_PLATFORM_SETTINGS_STR);
  if (status != DB_SUCCESS){
    cm_msg(MERROR, "init", "could not create record %s", str);
    return FE_ERR_ODB;
  }

  if (db_find_key(hDB, 0, str, &hkey) == DB_SUCCESS) {
    size = sizeof(HALL_PROBE_PLATFORM_SETTINGS);
    db_get_record(hDB, hkey, &hall_probe_platform_settings, &size, 0);
  } else {
    cm_msg(MERROR, "init", "could not load settings from ODB");
  }

  // Get the handle for the Keithley 2100
  char pt100_devname[128];
  char keithley_devname[128];
  char yokogawa_devname[128];

  std::stringstream ss;
  std::string s;
  std::ifstream in("/dev/usbtmc0");

  if (in.good()) {
    ss << in.rdbuf();
    in.close();
  }

  for (int i = 0; i < 16; ++i) {
    std::getline(ss, s, '\n');

    if (s.find("KEITHLEY") != s.size() - 1) {
      std::cout << s << std::endl;
      sprintf(keithley_devname, "/dev/usbtmc%i", i);
    }

    if (s.find("Yokogawa") != s.size() - 1) {
      std::cout << s << std::endl;
      sprintf(yokogawa_devname, "/dev/usbtmc%i", i);
    }
  }

  keithley_port = open(keithley_devname, O_RDWR);

  // If the default didn't exist, skip it.
  if (keithley_port == -1) {
    cm_msg(MERROR, "init", "could not find Keithley 2100 device");
    return FE_ERR_HW;
  }

  if (yokogawa_port == -1) {
    cm_msg(MERROR, "init", "could not find Yokogawa 2100 device");
    return FE_ERR_HW;
  }

  // Get a handle for the temperature probe.
  yRegisterHub("usb", err);

  sprintf(pt100_devname, "%s.temperature",
          hall_probe_platform_settings.pt100_dev);

  temp_probe = yFindTemperature(pt100_devname);

  if (temp_probe == nullptr) {
    cm_msg(MERROR, "init", "Could not find Yoctopuce PT100 device");
    return FE_ERR_HW;
  }

  return SUCCESS;
}


//--- Frontend Exit ------------------------------------------------//
INT frontend_exit()
{
  return SUCCESS;
}


//--- Begin of Run --------------------------------------------------//
INT begin_of_run(INT run_number, char *error)
{
  return SUCCESS;
}


//--- End of Run -----------------------------------------------------//
INT end_of_run(INT run_number, char *error)
{
  return SUCCESS;
}


//--- Pause Run -----------------------------------------------------//
INT pause_run(INT run_number, char *error)
{
  return SUCCESS;
}


//--- Resume Run ----------------------------------------------------//

INT resume_run(INT run_number, char *error)
{
  return SUCCESS;
}

//--- Frontend Loop -------------------------------------------------//

INT frontend_loop()
{
  // If frontend_call_loop is true, this routine gets called when
  // the frontend is idle or once between every event

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

    for (i = 0; i < count; i++) usleep(10);

    return 0;
  }

  return 0;
}


//--- Interrupt configuration ---------------------------------------//

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
  char line[128];
  char buf[128];
  int n, b;

  // Hall Probe Platform
  sprintf(bk_name, "HPPF");

  // And MIDAS output
  bk_init32(pevent);
  bk_create(pevent, bk_name, TID_DOUBLE, &pdata);

  if (temp_probe->isOnline()) {

    *(pdata++) = temp_probe->get_currentValue();

  } else {

    cm_msg(MINFO, "read_trigger_event", "temperature probe not online");
    *(pdata++) = -1.0;
  }

  if (keithley_port > -1) {

    sprintf(line, "MEAS:VOLT:DC?\n");
    n = write(keithley_port, line, strlen(line));
    b = read(keithley_port, &buf, strlen(buf));

    *(pdata++) = atof(buf);

  } else {

    cm_msg(MINFO, "read_trigger_event", "Keithley DMM not online");
    *(pdata++) = -1.0;
  }

  // if (yokogawa_port > 0) {
  //   sprintf(line, "MEAS:VOLT:DC?\n");
  //   n = write(serial_port, line, strlen(line));
  //   b = read(serial_port, &buf, strlen(buf));

  //   *(pdata++) = atof(buf);
  // }

  bk_close(pevent, pdata);

  return bk_size(pevent);
};
