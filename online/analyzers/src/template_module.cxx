/*============================================================================*\

  file:    template_module.cxx
  author:  Matthias W. Smith based on 
           midas/examples/experiment/scaler.c by Stefan Ritt

  about:   Template analyzer module. This module looks for an
           XMPL bank which contains the time.

\*============================================================================*/

//--- std includes -----------------------------------------------------------//
#include <string>
#include <iostream>
#include <cstdio>
#include <ctime>

//--- midas includes ---------------------------------------------------------//
#include "midas.h"
#include "experim.h"

//--- module declaration -----------------------------------------------------//

INT template_event(EVENT_HEADER *, void *);
INT template_bor(INT run_number);
INT template_eor(INT run_number);

ANA_MODULE template_module = {
   "sync-template-module",      // module name
   "Matthias W. Smith",         // author
   template_event,              // event routine
   template_bor,                // BOR routine
   template_eor,                // EOR routine
   NULL,                        // init routine
   NULL,                        // exit routine
   NULL,                        // parameter structure
   0,                           // structure size
   NULL,                        // initial parameters
};

//-- BOR routine ---------------------------------------------------//

INT template_bor(INT run_number)
{
   return SUCCESS;
}

//-- EOR routine ---------------------------------------------------//

INT template_eor(INT run_number)
{
   return SUCCESS;
}

//-- event routine -------------------------------------------------//

INT template_event(EVENT_HEADER * pheader, void *pevent)
{
  INT rc;
  DWORD *pdata;
  
  // Look for XMPL bank.
  rc = bk_locate(pevent, "XMPL", &pdata);

  // See if we found the bank.
  if (rc == 0) return 1;

  // Print the time
  std::cout << "template-module found time value, " << *pdata;
  std::cout << " in bank XMPL." << std::endl;

  return SUCCESS;
}
