/**************************************************************************** \

Name:   Metrolab RS232 client
Author: Peter Winter
Email:  winterp@anl.gov

About:  A simple frontend to communicate with the Metrolab via RS232.

\*****************************************************************************/


//--- std includes ----------------------------------------------------------//
#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdio.h>
using namespace std;

//--- other includes -------------------------------------------------------//
#include "midas.h"

//--- global variables ------------------------------------------------------//
#define FRONTEND_NAME "Metrolab"

extern "C" {

  // The frontend name (client name) as seen by other MIDAS clients.
  char *frontend_name = (char*)FRONTEND_NAME;

  // The frontend file name, don't change it.
  char *frontend_file_name = (char*) __FILE__;

  // Frontend_loop is called periodically if this variable is TRUE.
  BOOL frontend_call_loop = FALSE;

  // A frontend status page is displayed with this frequency in ms.
  INT display_period = 1000;

  // Set maximum event size produced by this frontend
  INT max_event_size = 0x80000;

  // Set maximum event size for fragmented events (EQ_FRAGMENTED)
  INT max_event_size_frag = 0x800000;

  // Set buffer size to hold events.
  INT event_buffer_size = 0x800000;

  // Function declarations.
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

  // Define the equipment list.
  EQUIPMENT equipment[] =
    {
      {FRONTEND_NAME,  // equipment name
       {11, 13,         // event ID, trigger mask
        "SYSTEM",      // event buffer (use to be SYSTEM)
        EQ_PERIODIC,   // equipment type
        0,             // not used
        "MIDAS",       // format
        TRUE,          // enabled
        RO_ALWAYS |    // read only when running
        RO_ODB,        // and update ODB
        2000,          // read every 2s
        0,             // stop run after this event limit
        0,             // number of sub events
        1,             // don't log history
        "", "", "",
       },

       read_trigger_event,      // readout routine
       NULL,
       NULL
      },

      {""}
    };

} //extern C

// Define settings struct and string for frontend.
typedef struct {
   char address[128];
} METROLAB_SETTINGS;
METROLAB_SETTINGS metrolab_settings;

#define METROLAB_SETTINGS_STR "\
device_path = STRING : [128] /dev/ttyUSB%i\n\
"

int serial_port;

//--- Frontend Init ---------------------------------------------------------//
INT frontend_init()
{
  HNDLE hDB, hkey;
  INT status, tmp;
  char str[256], filename[256];
  int size;

  cm_get_experiment_database(&hDB, NULL);

  sprintf(str, "/Equipment/%s/Settings", FRONTEND_NAME);
  status = db_create_record(hDB, 0, str, METROLAB_SETTINGS_STR);
  if (status != DB_SUCCESS){
    cm_msg(MERROR, "init", "Could not create record \"%s\"", str);
    return FE_ERR_ODB;
  }

  if (db_find_key(hDB, 0, str, &hkey) == DB_SUCCESS) {
    size = sizeof(METROLAB_SETTINGS);
    db_get_record(hDB, hkey, &metrolab_settings, &size, 0);
  }

  char devname[100];
  struct termios options;

  // Try opening several serial port, since USB serials change number.
  for (int n = 0; n < 4; ++n) {
    sprintf(devname, metrolab_settings.address);
    serial_port = open(devname, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);

    if ((serial_port < 0) && (n == 3)) {
      cm_msg(MERROR, "frontend_init", "error opening device \"%s\"", devname);
      return FE_ERR_HW;
    }
  }

  fcntl(serial_port, F_SETFL, 0); // return immediately if no data

  if (tcgetattr(serial_port, &options) < 0) {
    cm_msg(MERROR, "frontend_init", "error in calling tcgetattr");
    return FE_ERR_HW;
  }

  cfsetospeed(&options, B9600);
  cfsetispeed(&options, B9600);

  // Setting other port stuff.
  options.c_cflag &= ~PARENB;  // Make 8n1
  options.c_cflag &= ~CSTOPB;
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;
  options.c_cflag &= ~CRTSCTS; // no flow control
  options.c_lflag = 0;  // no signaling chars, no echo, no canonical processing
  options.c_oflag = 0;  // no remapping, no delays
  options.c_cc[VMIN] = 0; // read doesn't block
  options.c_cc[VTIME] = 5; // 0.5 seconds read timeout
  options.c_cflag |= CREAD | CLOCAL; // turn on READ & ignore ctrl lines
  options.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
  options.c_oflag &= ~OPOST; // make raw

  tcflush( serial_port, TCIFLUSH );

  if(tcsetattr(serial_port, TCSANOW, &options) < 0) {
    cm_msg(MERROR, "frontend_init", "error in calling tcsetattr");
    return FE_ERR_HW;
  }

  char line[200];
  sprintf(line,"R\r\nA0\r\nH\r\n");
  write(serial_port, line, strlen(line));
  tcflush(serial_port, TCIFLUSH );

  return SUCCESS;
}

//--- Frontend Exit ---------------------------------------------------------//
INT frontend_exit()
{
  return SUCCESS;
}

//--- Begin of Run ----------------------------------------------------------//
INT begin_of_run(INT run_number, char *error)
{
  return SUCCESS;
}

//--- End of Run ------------------------------------------------------------//
INT end_of_run(INT run_number, char *error)
{
  return SUCCESS;
}

//--- Pause Run -------------------------------------------------------------//
INT pause_run(INT run_number, char *error){
  return SUCCESS;
}

//--- Resume Run -----------------------------------------------------------//
INT resume_run(INT run_number, char *error)
{
  return SUCCESS;
}

//--- Frontend Loop ---------------------------------------------------------//
INT frontend_loop()
{
  return SUCCESS;
}

//---------------------------------------------------------------------------//

/**************************************************************************** \

  Readout routines for different events

\*****************************************************************************/

//--- Trigger event routines ------------------------------------------------//
INT poll_event(INT source, INT count, BOOL test) {

  static unsigned int i;

  // fake calibration
  if (test) {
    for (i = 0; i < count; i++) {
      usleep(10);
    }
    return 0;
  }

  return 1;
}

//--- interrupt configuration -----------------------------------------------//
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

//--- event readout ---------------------------------------------------------//
INT read_trigger_event(char *pevent, INT off)
{
  int count = 0;
  char bk_name[10];
  double *pdata;

  double magnetic_field;
  string field_unit;
  double time_stamp;

  char line[200];
  int b, n;
  char status, unit, buf[30];
  double value;

  // And MIDAS output.
  bk_init32(pevent);

  // Get the time as an example
  sprintf(bk_name, "MTR2");

  sprintf(line,"\u0005\n");
  n = write(serial_port, line, strlen(line));
  b = read(serial_port, &buf, 30);

  if (b == 13) {
    status = buf[0];
    char v[12];
    strncpy(v, &buf[1], 9);
    v[11] = '\0';
    value = stof(v);
    unit = buf[10];

  } else {

    cm_msg(MDEBUG, "read_trigger_event", "no valid readback value found");

    return 0;
  }

  bk_create(pevent, bk_name, TID_DOUBLE, &pdata);

  *(pdata++) = (double)status;
  *(pdata++) = value - 1.45;
  *(pdata++) = (double)unit;

  bk_close(pevent, pdata);

  return bk_size(pevent);
};
