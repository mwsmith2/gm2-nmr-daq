/********************************************************************\

  Name:         temp.c
  Created by:   Joe Nash, modified by Joe Grange.  A nominally appropriate team.

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
#include <TBranch.h>
#include "TFile.h"

/* my includes */
#include "TTemps.h"

/*-- Module declaration --------------------------------------------*/

INT temp_copy(EVENT_HEADER *, void *);
INT temp_init(void);
INT temp_eor(INT run_number);
INT temp_exit(void);

extern TTemps* gData;

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

TH1D *probe1;
TH1D *probe2;
TH1D *probe3;

static TFile *fTreeFile = NULL;
static TTree *fEventTree = NULL;
static TBranch *fEventBranch = NULL;

//char *timeStamp[300];
//double *tempVals[300];

/*-- init routine --------------------------------------------------*/

INT temp_init(void)
{

  fTreeFile = TFile::Open("tempTree.root", "recreate");   

  fTreeFile->cd();
  
  fEventTree = new TTree("EventTree","Temperature readings");
  fEventTree->SetAutoSave(300000000); // autosave when 300 Mbyte written.
  fEventTree->SetMaxVirtualSize(300000000); // 300 Mbyte
  int split = 1;
  int bufsize = 64000;
  Int_t branchstyle = 1;

  if (split < 0) {branchstyle = 0; split = -1-split;}
  TTree::SetBranchStyle(branchstyle);

  fEventBranch = fEventTree->Branch("Event", "TTemps", &gData, bufsize, split);
  fEventBranch->SetAutoDelete(kFALSE);

   /* book histos */

  probe1 = h1_book<TH1D>("probe1", "probe1", 200, 23, 26);
  probe2 = h1_book<TH1D>("probe2", "probe2", 200, 23, 26);
  probe3 = h1_book<TH1D>("probe3", "probe3", 200, 23, 26);

  //char *timeStamp[30];

   return SUCCESS;
}


/*-----exit routine-------------*/

INT temp_exit(void)
{
  
   fTreeFile->Write();
   fTreeFile->Close();
 
   //TFile *out = new TFile("tempHists1D.root","RECREATE");
   //probe1->Write(probe1->GetName());
   //probe2->Write(probe2->GetName());
   //probe3->Write(probe3->GetName());
   //out->Close();
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

        probe1->Fill(pout[0]);
        probe2->Fill(pout[1]);
        probe3->Fill(pout[2]);

   /* close bank */
   bk_close(pevent, pout + (end-start));

   gData->tStamp = pheader->time_stamp;
   gData->p1val = pout[0];
   gData->p2val = pout[1];
   gData->p3val = pout[2];

   fEventTree->Fill();

   return SUCCESS;
}
