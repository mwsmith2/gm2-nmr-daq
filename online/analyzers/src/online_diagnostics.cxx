/********************************************************************\

  file:    convert_shim_data.cxx
  author:  Matthias W. Smith base on envi.c from Joe Grange

  about:   Look for SHPF and SHFX midas banks and make diagnostic
           plots from them.

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

#include "TFile.h"
#include "TStyle.h"
#include "TGraphPolar.h"

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"

//--- project includes ---------------------------------------------//
#include "nmr/common.hh"
#include "nmr/midas_helper_functions.hh"

//--- Globals ------------------------------------------------------//

// MWS
namespace {

daq::shim_platform_st shpf_data;
daq::shim_fixed_st shfx_data;

int shpf_size = sizeof(shpf_data) / sizeof(DWORD);
int shfx_size = sizeof(shfx_data) / sizeof(DWORD);

TFile *pf = nullptr;
TGraphPolar *pgr_status = nullptr;
boost::property_tree::ptree conf;
}

/* The analyzer name (client name) as seen by other MIDAS clients   */
char *analyzer_name = "shim-diagnostics";

/* analyzer_loop is called with this interval in ms (0 to disable)  */
INT analyzer_loop_period = 0;

/* default ODB size */
INT odb_size = DEFAULT_ODB_SIZE;

/*-- Module declaration --------------------------------------------*/

BANK_LIST shim_bank_list[] = {

  {"SHPF", TID_DWORD, shpf_size, NULL},
  {"SHFX", TID_DWORD, shfx_size, NULL},
  {""},
};

/*-- Event request list --------------------------------------------*/
INT ana_shim_diagnostics(EVENT_HEADER *pheader, void *pevent);
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
   ana_shim_diagnostics,       /* analyzer routine */
   NULL,                       /* module list */
   shim_bank_list,             /* bank list */
  },

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
  const int size = 1024;
  char str[size];
  HNDLE hDB, hkey;

  int run_num;
  string datadir;
  string confdir;
    
  cm_get_experiment_database(&hDB, NULL);
  db_find_key(hDB, 0, "/Logger/Data dir", &hkey);

  if (hkey) {
    db_get_data(hDB, hkey, &str, &size, TID_STRING);
    datadir = std::string(str);
    if (datadir[datadir.size()-1] != DIR_SEPARATOR_STR) {
      datadir.append(DIR_SEPARATOR_STR);
    }
  }

  datadir.append(string("diag/"));

  hkey = NULL;
  RUNINFO runinfo;
  db_find_key(hDB, 0, "/Runinfo", &hkey);

  // Get the run number
  if (hkey) {
    db_open_record(hDB, hkey, &runinfo, sizeof(runinfo), MODE_READ, NULL, NULL);
  }
  run_num = runinfo.run_number;

  // Open the config file with probe coordinates.
  db_find_key(hDB, 0, "/Params/config-dir", &hkey);

  if (hkey) {
    db_get_data(hDB, hkey, &str, &size, TID_STRING);
    confdir = std::string(str);
  }
  
  auto conf_file = confdir + string("/shim-platform/probe_coordinates.json");
  boost::property_tree::read_json(conf_file, conf);

  string outfile;
  std::stringstream ss;
  std::ios::fmtflags flags(ss.flags());

  ss << datadir << "run_";
  ss << std::setfill('0') << std::setw(5) << run_num;
  ss.flags(flags);
  ss << ".root";
  ss >> outfile;

  pf = new TFile(outfile.c_str(), "recreate");

  pgr_status = new TGraphPolar(SHIM_PLATFORM_CH);

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

INT ana_shim_diagnostics(EVENT_HEADER *pheader, void *pevent)
{
  INT rc, end, start, n;
  DWORD *pdata;
  
  // Look for shim platform data bank
  n = bk_locate(pevent, "SHPF", &pdata);

  // Return if we didn't find anything.
  if (n == 0) return 1;

  shpf_data = *reinterpret_cast<daq::shim_platform_st *>(pdata);

  std::cout << "Making polar plot." << std::endl;
  for (n = 0; n < SHIM_PLATFORM_CH; ++n) {
    static string radstring;
    static string phistring;
    
    radstring = std::to_string(n) + string(".r");
    phistring = std::to_string(n) + string(".phi");
    
    if (n == 0) {

      gStyle->SetMarkerColor(kGreen);
      pgr_status->SetPoint(n, conf.get<double>(phistring), conf.get<double>(phistring));

    } else {

      gStyle->SetMarkerColor(kRed);
      pgr_status->SetPoint(n, conf.get<double>(phistring), conf.get<double>(phistring));
    }
  }
  
  return SUCCESS;
}
