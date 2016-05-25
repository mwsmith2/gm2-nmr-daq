/*============================================================================*\

  file:    template_analyzer_solo.cxx
  author:  Matthias W. Smith

  about:   A template that works with the nmr daq setup.  This
           analyzer operates without loading modules.

\*============================================================================*/

//--- other includes ---------------------------------------------------------//

//--- std includes -----------------------------------------------------------//
#include <string>
#include <ctime>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <iomanip>
using std::string;
using std::cout;
using std::endl;

//--- other includes ---------------------------------------------------------//
#include "midas.h"
#include "experim.h"
#include "TTree.h"
#include "TFile.h"

//--- project includes -------------------------------------------------------//

//--- global variables -------------------------------------------------------//

// The analyzer name (client name) as seen by other MIDAS clients
char *analyzer_name = "template-analyzer";

// analyzer_loop is called with this interval in ms (0 to disable)
INT analyzer_loop_period = 0;

// default ODB size
INT odb_size = DEFAULT_ODB_SIZE;

//--- bank declaration -------------------------------------------------------//

BANK_LIST analyzer_bank_list[] = {

  {"XMPL", TID_DOUBLE, 1, NULL},
  {""},
};

//--- Event request list -----------------------------------------------------//
INT ana_trigger_event(EVENT_HEADER *pheader, void *pevent);
INT ana_begin_of_run(INT run_number, char *error);
INT ana_end_of_run(INT run_number, char *error);

ANALYZE_REQUEST analyze_request[] = {
  {"sync-template",             // equipment name
   {1,                          // event ID
    TRIGGER_ALL,                // trigger mask 
     GET_NONBLOCKING,           // get events without blocking producer 
     "SYSTEM",                  // event buffer 
     TRUE,                      // enabled 
     "", "",}
    ,
    ana_trigger_event,          // analyzer routine 
    NULL,                       // module list 
    analyzer_bank_list,    // bank list 
    }
   ,

   {""}
   ,
};



//--- init routine -----------------------------------------------------------//

INT analyzer_init()
{
  return SUCCESS;
}


//--- exit routine -----------------------------------------------------------//

INT analyzer_exit(void)
{
   return SUCCESS;
}

//--- bor routine ------------------------------------------------------------//

INT ana_begin_of_run(INT run_number, char *error)
{
   return SUCCESS;
}

//--- eor routine ------------------------------------------------------------//

INT ana_end_of_run(INT run_number, char *error)
{
   return SUCCESS;
}

//--- pause run --------------------------------------------------------------//

INT ana_pause_run(INT run_number, char *error)
{
   return CM_SUCCESS;
}

//--- resume run -------------------------------------------------------------//

INT ana_resume_run(INT run_number, char *error)
{
   return CM_SUCCESS;
}

//--- analyzer Loop ----------------------------------------------------------//

INT analyzer_loop()
{
   return CM_SUCCESS;
}

//--- event routine ----------------------------------------------------------//

INT ana_trigger_event(EVENT_HEADER *pheader, void *pevent)
{
   INT end, start, n;
   DWORD *pdata;

   // Look for shim platform data bank
   n = bk_locate(pevent, "XMPL", &pdata);

   // Return if we didn't find anything.
   if (n == 0) {

     cout << "Couldn't find XMPL bank." << endl;
     return 1;

   } else {
     
     cout << "Found XMPL bank." << endl;
     cout << "Time in microseconds: " << pdata[0] << endl;
   }

   return SUCCESS;
}
