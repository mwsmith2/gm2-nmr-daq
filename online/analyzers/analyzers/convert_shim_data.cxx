/********************************************************************\

  file:    convert_shim_data.cxx
  author:  Matthias W. Smith base on envi.c from Joe Grange

  about:   Look for SHPF and SHFX midas banks and convert them
           into root trees.

\********************************************************************/

//--- std includes -------------------------------------------------//
#include <string>
#include <ctime>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <iomanip>
using std::string;

//--- other includes -----------------------------------------------//
#include "midas.h"
#include "experim.h"
#include "TTree.h"
#include "TFile.h"

//--- project includes ---------------------------------------------//
#include "nmr/common.hh"
#include "nmr/midas_helper_functions.hh"

//--- Globals ------------------------------------------------------//

// MWS
namespace {

const int nprobes = SHIM_PLATFORM_CH;
daq::shim_platform_st shim_data;
int shim_data_size = sizeof(shim_data) / sizeof(DWORD);

TFile *pf = nullptr;
TTree *pt = nullptr;
}

/* The analyzer name (client name) as seen by other MIDAS clients   */
char *analyzer_name = "Shim Data Converter";

/* analyzer_loop is called with this interval in ms (0 to disable)  */
INT analyzer_loop_period = 0;

/* default ODB size */
INT odb_size = DEFAULT_ODB_SIZE;

/*-- Module declaration --------------------------------------------*/

BANK_LIST shim_platform_bank_list[] = {

  {"SHPF", TID_DWORD, shim_data_size, NULL},
  {""},
};

/*-- Event request list --------------------------------------------*/
INT ana_convert_event(EVENT_HEADER *pheader, void *pevent);
INT ana_begin_of_run(INT run_number, char *error);
INT ana_end_of_run(INT run_number, char *error);

ANALYZE_REQUEST analyze_request[] = {
   {"shim-platform",            /* equipment name */
    {1,                         /* event ID */
     TRIGGER_ALL,               /* trigger mask */
     GET_NONBLOCKING,           /* get events without blocking producer */
     "BUF1",                    /* event buffer */
     TRUE,                      /* enabled */
     "", "",}
    ,
    ana_convert_event,          /* analyzer routine */
    NULL,                       /* module list */
    shim_platform_bank_list,    /* bank list */
    }
   ,

   {""}
   ,
};



/*-- init routine --------------------------------------------------*/

INT analyzer_init()
{
  return SUCCESS;
}


/*-----exit routine-------------*/

INT analyzer_exit(void)
{
   return SUCCESS;
}

/*-- BOR routine ---------------------------------------------------*/

INT ana_begin_of_run(INT run_number, char *error)
{
  // Get the midas data directory
  int size = 1024;
  char str[1024];
  HNDLE hDB, hkey;

  int run_num;
  string datadir;
    
  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, "/Logger/Data dir", &hkey);

  if (hkey) {
    db_get_data(hDB, hkey, &str, &size, TID_STRING);
    datadir = std::string(str);
    if (datadir[datadir.size()-1] != DIR_SEPARATOR_STR) {
      datadir.append(DIR_SEPARATOR_STR);
    }
  }
  
  std::cout << datadir << std::endl;

  hkey = NULL;
  RUNINFO runinfo;
  db_find_key(hDB, 0, "/Runinfo", &hkey);

  if (hkey) {
    db_open_record(hDB, hkey, &runinfo, sizeof(runinfo), MODE_READ, NULL, NULL);
  }
  
  run_num = runinfo.run_number;
  std::cout << run_num << std::endl; 

  string outfile;
  std::stringstream ss;
  std::ios::fmtflags flags(ss.flags());

  ss << datadir << "run_";
  ss << std::setfill('0') << std::setw(5) << run_num;
  ss.flags(flags);
  ss << ".root";
  ss >> outfile;

  std::cout << outfile << std::endl;

  pf = new TFile(outfile.c_str(), "recreate");   
  pt = new TTree("t", "Shim platform data");

  pt->SetAutoSave(300000000); // autosave when 300 Mbyte written.
  pt->SetMaxVirtualSize(300000000); // 300 Mbyte
    
  string br_name;
  string br_vars;
  string ch_str;

  ss.clear();
  ss.str(string());

  ss << "clock_sec[" << nprobes << "]/l:clock_usec[" << nprobes;
  ss << "]/l:fid_time[" << nprobes << "]/D:snr[" << nprobes;
  ss << "]/D:freq[" << nprobes << "]/D:freq_err[" << nprobes;
  ss << "]/D:trace[" << nprobes << "][" << SHORT_FID_LN << "]/s";

  br_name = string("shim_platform");
  br_vars = ss.str();

  pt->Branch(br_name.c_str(), &shim_data, br_vars.c_str());
  return SUCCESS;
}

/*-- EOR routine ---------------------------------------------------*/

INT ana_end_of_run(INT run_number, char *error)
{
   pf->Write();
   pf->Close();
 
   return SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/

INT ana_pause_run(INT run_number, char *error)
{
   return CM_SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/

INT ana_resume_run(INT run_number, char *error)
{
   return CM_SUCCESS;
}

/*-- Analyzer Loop -------------------------------------------------*/

INT analyzer_loop()
{
   return CM_SUCCESS;
}

/*-- event routine -------------------------------------------------*/

INT ana_convert_event(EVENT_HEADER *pheader, void *pevent)
{
   INT end, start, n;
   DWORD *pdata;

   // Look for shim platform data bank
   n = bk_locate(pevent, "SHPF", &pdata);

   // Return if we didn't find anything.
   if (n == 0) return 1;

   shim_data = *reinterpret_cast<daq::shim_platform_st *>(pdata);

   pt->Fill();

   return SUCCESS;
}
