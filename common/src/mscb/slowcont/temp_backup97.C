/********************************************************************\

  Name:         temp.c
  Created by:   Joe Nash

  Contents:     looks for an INPT bank and reads values into another test 
		bank, TEMP

  $Id: temp.c

\********************************************************************/

/*-- Include files -------------------------------------------------*/

/* standard includes */
#include <stdio.h>
#include <string.h>
#include <time.h>

/* midas includes */
#include "midas.h"
#include "experim.h"
#include "analyzer.h"

/* root includes */
#include <TH1D.h>
#include <TH2D.h>
#include <TTree.h>
#include "TFile.h"

/*-- Module declaration --------------------------------------------*/

INT temp_copy(EVENT_HEADER *, void *);
INT temp_init(void);
INT temp_eor(INT run_number);
INT temp_exit(void);

ANA_MODULE temp_module = {
   "temp",       		/* module name           */
   "Joe Nash",               /* author                */
   temp_copy,                /* event routine         */
   NULL,                	/* BOR routine           */
   temp_eor,                  /* EOR routine           */
   temp_init,                        /* init routine          */
   temp_exit,                        /* exit routine          */
   NULL,                        /* parameter structure   */
   0,                           /* structure size        */
   NULL,                        /* initial parameters    */
};

TH2D *probe1;
TH2D *probe2;
TH2D *probe3;

/*-- init routine --------------------------------------------------*/

INT temp_init(void)
{
   /* book histos */
  //probe1 = h1_book<TH1D>("probe1", "probe1", 200, 23, 26);
  //probe2 = h1_book<TH1D>("probe2", "probe2", 200, 23, 26);
  //probe3 = h1_book<TH1D>("probe3", "probe3", 200, 23, 26);

  probe1 = new TH2D("probe1", "probe1", 200,1378410851,1378411000,200, 23, 26);
  probe2 = new TH2D("probe2", "probe2", 200,1378410851,1378411000,200, 23, 26);
  probe3 = new TH2D("probe3", "probe3", 200,1378410851,1378411000,200, 23, 26);

   return SUCCESS;
}


/*-----exit routine-------------*/

INT temp_exit(void)
{
   
   TFile *out = new TFile("tempHists.root","RECREATE");
   probe1->Write(probe1->GetName());
   probe2->Write(probe2->GetName());
   probe3->Write(probe3->GetName());
   out->Close();
   return SUCCESS;
}

/*-- EOR routine ---------------------------------------------------*/

INT temp_eor(INT run_number)
{
   return SUCCESS;
}

/*-- event routine -------------------------------------------------*/

INT temp_copy(EVENT_HEADER * pheader, void *pevent)
{
   INT end, start, n;
   float *pin;
   float *pout;

   /* look for temp bank */
   n = bk_locate(pevent, "INPT", &pin);
   if (n == 0)
      return 1;

   /* create  bank */
   bk_create(pevent, "TEMP", TID_FLOAT, &pout);

   //set copy bounds indicies
   start =0; end =4; 

   /* copy partial bank*/
   for (INT i = start; i < end; i++) {
	if ( i >= n ) continue;
     	pout[i] = (float) pin[i] ;
	//printf("%d %f\n",i,pout[i]);
   }
	probe1->Fill(pheader->time_stamp,pout[0]);
	probe2->Fill(pheader->time_stamp,pout[1]);
        probe3->Fill(pheader->time_stamp,pout[2]);

   /* close bank */
   bk_close(pevent, pout + (end-start));

   return SUCCESS;
}
