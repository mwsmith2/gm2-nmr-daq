/********************************************************************\

  Name:         experim.h
  Created by:   ODBedit program

  Contents:     This file contains C structures for the "Experiment"
                tree in the ODB and the "/Analyzer/Parameters" tree.

                Additionally, it contains the "Settings" subtree for
                all items listed under "/Equipment" as well as their
                event definition.

                It can be used by the frontend and analyzer to work
                with these information.

                All C structures are accompanied with a string represen-
                tation which can be used in the db_create_record function
                to setup an ODB structure which matches the C structure.

  Created on:   Tue Mar 10 16:03:12 2015

\********************************************************************/

#ifndef EXCL_SHIM_PLATFORM

#define SHIM_PLATFORM_COMMON_DEFINED

typedef struct {
  WORD      event_id;
  WORD      trigger_mask;
  char      buffer[32];
  INT       type;
  INT       source;
  char      format[8];
  BOOL      enabled;
  INT       read_on;
  INT       period;
  double    event_limit;
  DWORD     num_subevents;
  INT       log_history;
  char      frontend_host[32];
  char      frontend_name[32];
  char      frontend_file_name[256];
  char      status[256];
  char      status_color[32];
  BOOL      hidden;
} SHIM_PLATFORM_COMMON;

#define SHIM_PLATFORM_COMMON_STR(_name) const char *_name[] = {\
"[.]",\
"Event ID = WORD : 1",\
"Trigger mask = WORD : 0",\
"Buffer = STRING : [32] BUF1",\
"Type = INT : 130",\
"Source = INT : 0",\
"Format = STRING : [8] MIDAS",\
"Enabled = BOOL : y",\
"Read on = INT : 257",\
"Period = INT : 500",\
"Event limit = DOUBLE : 0",\
"Num subevents = DWORD : 0",\
"Log history = INT : 90",\
"Frontend host = STRING : [32] nmr-daq",\
"Frontend name = STRING : [32] shim platform",\
"Frontend file name = STRING : [256] src/shim_platform.cxx",\
"Status = STRING : [256] shim platform@nmr-daq",\
"Status color = STRING : [32] #00FF00",\
"Hidden = BOOL : n",\
"",\
NULL }

#endif

#ifndef EXCL_SHIM_TRIGGER

#define SHIM_TRIGGER_COMMON_DEFINED

typedef struct {
  WORD      event_id;
  WORD      trigger_mask;
  char      buffer[32];
  INT       type;
  INT       source;
  char      format[8];
  BOOL      enabled;
  INT       read_on;
  INT       period;
  double    event_limit;
  DWORD     num_subevents;
  INT       log_history;
  char      frontend_host[32];
  char      frontend_name[32];
  char      frontend_file_name[256];
  char      status[256];
  char      status_color[32];
  BOOL      hidden;
} SHIM_TRIGGER_COMMON;

#define SHIM_TRIGGER_COMMON_STR(_name) const char *_name[] = {\
"[.]",\
"Event ID = WORD : 1",\
"Trigger mask = WORD : 0",\
"Buffer = STRING : [32] SYSTEM",\
"Type = INT : 2",\
"Source = INT : 0",\
"Format = STRING : [8] MIDAS",\
"Enabled = BOOL : y",\
"Read on = INT : 257",\
"Period = INT : 100",\
"Event limit = DOUBLE : 0",\
"Num subevents = DWORD : 0",\
"Log history = INT : 0",\
"Frontend host = STRING : [32] nmr-daq",\
"Frontend name = STRING : [32] shim master trigger",\
"Frontend file name = STRING : [256] src/shim_trigger.cxx",\
"Status = STRING : [256] shim master trigger@nmr-daq",\
"Status color = STRING : [32] #00FF00",\
"Hidden = BOOL : n",\
"",\
NULL }

#endif

#ifndef EXCL_ENVIRONMENT

#define ENVIRONMENT_COMMON_DEFINED

typedef struct {
  WORD      event_id;
  WORD      trigger_mask;
  char      buffer[32];
  INT       type;
  INT       source;
  char      format[8];
  BOOL      enabled;
  INT       read_on;
  INT       period;
  double    event_limit;
  DWORD     num_subevents;
  INT       log_history;
  char      frontend_host[32];
  char      frontend_name[32];
  char      frontend_file_name[256];
  char      status[256];
  char      status_color[32];
  BOOL      hidden;
} ENVIRONMENT_COMMON;

#define ENVIRONMENT_COMMON_STR(_name) const char *_name[] = {\
"[.]",\
"Event ID = WORD : 10",\
"Trigger mask = WORD : 0",\
"Buffer = STRING : [32] SYSTEM",\
"Type = INT : 16",\
"Source = INT : 0",\
"Format = STRING : [8] MIDAS",\
"Enabled = BOOL : y",\
"Read on = INT : 255",\
"Period = INT : 1000",\
"Event limit = DOUBLE : 0",\
"Num subevents = DWORD : 0",\
"Log history = INT : 6",\
"Frontend host = STRING : [32] nmr-daq",\
"Frontend name = STRING : [32] SC Frontend",\
"Frontend file name = STRING : [256] mscb_fe.c",\
"Status = STRING : [256] Ok",\
"Status color = STRING : [32] #00FF00",\
"Hidden = BOOL : n",\
"",\
NULL }

#define ENVIRONMENT_SETTINGS_DEFINED

typedef struct {
  struct {
    struct {
      char      device[256];
      char      pwd[32];
      INT       mscb_address[12];
      BYTE      mscb_index[12];
      BOOL      debug;
      INT       retries;
    } scs2001;
    struct {
      char      device[32];
      char      pwd[32];
    } output;
  } devices;
  float     update_threshold[12];
  char      names_input[12][32];
  float     input_offset[12];
  float     input_factor[12];
} ENVIRONMENT_SETTINGS;

#define ENVIRONMENT_SETTINGS_STR(_name) const char *_name[] = {\
"[Devices/SCS2001]",\
"Device = STRING : [256] mscb110",\
"Pwd = STRING : [32] mscb110",\
"MSCB Address = INT[12] :",\
"[0] 1",\
"[1] 1",\
"[2] 1",\
"[3] 1",\
"[4] 1",\
"[5] 1",\
"[6] 1",\
"[7] 1",\
"[8] 1",\
"[9] 1",\
"[10] 1",\
"[11] 1",\
"MSCB Index = BYTE[12] :",\
"[0] 0",\
"[1] 1",\
"[2] 2",\
"[3] 3",\
"[4] 4",\
"[5] 5",\
"[6] 6",\
"[7] 7",\
"[8] 24",\
"[9] 25",\
"[10] 26",\
"[11] 27",\
"Debug = BOOL : n",\
"Retries = INT : 10",\
"",\
"[Devices/Output]",\
"Device = STRING : [32] mscb110",\
"Pwd = STRING : [32] mscb110",\
"",\
"[.]",\
"Update Threshold = FLOAT[12] :",\
"[0] 0.01",\
"[1] 0.01",\
"[2] 0.01",\
"[3] 0.01",\
"[4] 0.01",\
"[5] 0.01",\
"[6] 0.01",\
"[7] 0.01",\
"[8] 0.001",\
"[9] 0.001",\
"[10] 0.001",\
"[11] 0.001",\
"Names Input = STRING[12] :",\
"[32] Temperature 1",\
"[32] Temperature 2",\
"[32] Temperature 3",\
"[32] Temperature 4",\
"[32] Temperature 5",\
"[32] Temperature 6",\
"[32] Temperature 7",\
"[32] Temperature 8",\
"[32] Capacitec 1",\
"[32] Capacitec 2",\
"[32] Capacitec 3",\
"[32] Capacitec 4",\
"Input Offset = FLOAT[12] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"[4] 0",\
"[5] 0",\
"[6] 0",\
"[7] 0",\
"[8] 0",\
"[9] 0",\
"[10] 0",\
"[11] 0",\
"Input Factor = FLOAT[12] :",\
"[0] 1",\
"[1] 1",\
"[2] 1",\
"[3] 1",\
"[4] 1",\
"[5] 1",\
"[6] 1",\
"[7] 1",\
"[8] 1",\
"[9] 1",\
"[10] 1",\
"[11] 1",\
"",\
NULL }

#endif

#ifndef EXCL_EB

#define EB_COMMON_DEFINED

typedef struct {
  WORD      event_id;
  WORD      trigger_mask;
  char      buffer[32];
  INT       type;
  INT       source;
  char      format[8];
  BOOL      enabled;
  INT       read_on;
  INT       period;
  double    event_limit;
  DWORD     num_subevents;
  INT       log_history;
  char      frontend_host[32];
  char      frontend_name[32];
  char      frontend_file_name[256];
  char      status[256];
  char      status_color[32];
  BOOL      hidden;
} EB_COMMON;

#define EB_COMMON_STR(_name) const char *_name[] = {\
"[.]",\
"Event ID = WORD : 1",\
"Trigger mask = WORD : 0",\
"Buffer = STRING : [32] SYSTEM",\
"Type = INT : 0",\
"Source = INT : 0",\
"Format = STRING : [8] MIDAS",\
"Enabled = BOOL : y",\
"Read on = INT : 0",\
"Period = INT : 0",\
"Event limit = DOUBLE : 0",\
"Num subevents = DWORD : 0",\
"Log history = INT : 0",\
"Frontend host = STRING : [32] nmr-daq",\
"Frontend name = STRING : [32] Ebuilder",\
"Frontend file name = STRING : [256] ebuser.c",\
"Status = STRING : [256] ",\
"Status color = STRING : [32] ",\
"hidden = BOOL : n",\
"",\
NULL }

#define EB_SETTINGS_DEFINED

typedef struct {
  INT       number_of_fragment;
  BOOL      user_build;
  char      user_field[64];
  BOOL      fragment_required[2];
} EB_SETTINGS;

#define EB_SETTINGS_STR(_name) const char *_name[] = {\
"[.]",\
"Number of Fragment = INT : 2",\
"User build = BOOL : n",\
"User Field = STRING : [64] 100",\
"Fragment Required = BOOL[2] :",\
"[0] y",\
"[1] y",\
"",\
NULL }

#endif

#ifndef EXCL_DEFAULT_ASYNC

#define DEFAULT_ASYNC_COMMON_DEFINED

typedef struct {
  WORD      event_id;
  WORD      trigger_mask;
  char      buffer[32];
  INT       type;
  INT       source;
  char      format[8];
  BOOL      enabled;
  INT       read_on;
  INT       period;
  double    event_limit;
  DWORD     num_subevents;
  INT       log_history;
  char      frontend_host[32];
  char      frontend_name[32];
  char      frontend_file_name[256];
  char      status[256];
  char      status_color[32];
  BOOL      hidden;
} DEFAULT_ASYNC_COMMON;

#define DEFAULT_ASYNC_COMMON_STR(_name) const char *_name[] = {\
"[.]",\
"Event ID = WORD : 1",\
"Trigger mask = WORD : 0",\
"Buffer = STRING : [32] SYSTEM",\
"Type = INT : 129",\
"Source = INT : 0",\
"Format = STRING : [8] MIDAS",\
"Enabled = BOOL : y",\
"Read on = INT : 255",\
"Period = INT : 1500",\
"Event limit = DOUBLE : 0",\
"Num subevents = DWORD : 0",\
"Log history = INT : 120",\
"Frontend host = STRING : [32] nmr-daq",\
"Frontend name = STRING : [32] Slow Readout FE",\
"Frontend file name = STRING : [256] template_async_fe.c",\
"Status = STRING : [256] Slow Readout FE@nmr-daq",\
"Status color = STRING : [32] #00FF00",\
"Hidden = BOOL : n",\
"",\
NULL }

#endif

#ifndef EXCL_LASER_TRACKER

#define LASER_TRACKER_COMMON_DEFINED

typedef struct {
  WORD      event_id;
  WORD      trigger_mask;
  char      buffer[32];
  INT       type;
  INT       source;
  char      format[8];
  BOOL      enabled;
  INT       read_on;
  INT       period;
  double    event_limit;
  DWORD     num_subevents;
  INT       log_history;
  char      frontend_host[32];
  char      frontend_name[32];
  char      frontend_file_name[256];
  char      status[256];
  char      status_color[32];
  BOOL      hidden;
} LASER_TRACKER_COMMON;

#define LASER_TRACKER_COMMON_STR(_name) const char *_name[] = {\
"[.]",\
"Event ID = WORD : 1",\
"Trigger mask = WORD : 0",\
"Buffer = STRING : [32] BUF2",\
"Type = INT : 130",\
"Source = INT : 0",\
"Format = STRING : [8] MIDAS",\
"Enabled = BOOL : y",\
"Read on = INT : 1",\
"Period = INT : 10",\
"Event limit = DOUBLE : 0",\
"Num subevents = DWORD : 0",\
"Log history = INT : 20",\
"Frontend host = STRING : [32] nmr-daq",\
"Frontend name = STRING : [32] Laser Tracker",\
"Frontend file name = STRING : [256] src/laser_tracker.cxx",\
"Status = STRING : [256] Laser Tracker@nmr-daq",\
"Status color = STRING : [32] #00FF00",\
"Hidden = BOOL : n",\
"",\
NULL }

#endif

#ifndef EXCL_SHIM_UW_TEST

#define SHIM_UW_TEST_COMMON_DEFINED

typedef struct {
  WORD      event_id;
  WORD      trigger_mask;
  char      buffer[32];
  INT       type;
  INT       source;
  char      format[8];
  BOOL      enabled;
  INT       read_on;
  INT       period;
  double    event_limit;
  DWORD     num_subevents;
  INT       log_history;
  char      frontend_host[32];
  char      frontend_name[32];
  char      frontend_file_name[256];
  char      status[256];
  char      status_color[32];
  BOOL      hidden;
} SHIM_UW_TEST_COMMON;

#define SHIM_UW_TEST_COMMON_STR(_name) const char *_name[] = {\
"[.]",\
"Event ID = WORD : 1",\
"Trigger mask = WORD : 0",\
"Buffer = STRING : [32] SYSTEM",\
"Type = INT : 130",\
"Source = INT : 0",\
"Format = STRING : [8] MIDAS",\
"Enabled = BOOL : y",\
"Read on = INT : 257",\
"Period = INT : 10",\
"Event limit = DOUBLE : 0",\
"Num subevents = DWORD : 0",\
"Log history = INT : 0",\
"Frontend host = STRING : [32] nmr-daq",\
"Frontend name = STRING : [32] shim uw test",\
"Frontend file name = STRING : [256] src/shim_uw_test.cxx",\
"Status = STRING : [256] shim uw test@nmr-daq",\
"Status color = STRING : [32] #00FF00",\
"Hidden = BOOL : n",\
"",\
NULL }

#endif

