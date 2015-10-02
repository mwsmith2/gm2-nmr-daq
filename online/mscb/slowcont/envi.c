/********************************************************************\

  Name:         envi.c
  Created by:   Joe Nash, modified by Joe Grange.  A nominally appropriate team.

  Contents:     looks for an INPT bank and reads values into another test 
		bank, TEMP

  $Id: envi.c

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
#include "TEnvi.h"

/*-- Module declaration --------------------------------------------*/

INT envi_copy(EVENT_HEADER *, void *);
INT envi_init(void);
INT envi_eor(INT run_number);
INT envi_exit(void);

extern TEnvi* gData;

ANA_MODULE envi_module = {
   "envi",       		/* module name           */
   "Joe Nash",               /* author                */
   envi_copy,                /* event routine         */
   NULL,                	/* BOR routine           */
   envi_eor,                  /* EOR routine           */
   envi_init,                        /* init routine          */
   envi_exit,                        /* exit routine          */
   NULL,                        /* parameter structure   */
   0,                           /* structure size        */
   NULL,                        /* initial parameters    */
};

TH1D *probe1;
TH1D *probe2;
TH1D *probe3;
TH1D *probe4;
TH1D *probe5;
TH1D *probe6;
TH1D *probe7;
TH1D *probe8;

TH1D *capac1;
TH1D *capac2;
TH1D *capac3;
TH1D *capac4;

static TFile *fTreeFile = NULL;
static TTree *fEventTree = NULL;
static TBranch *fEventBranch = NULL;

/*-- init routine --------------------------------------------------*/

INT envi_init(void)
{

  TString fileName = "envTree_";

  time_t rawtime;
  struct tm*  time_;
  time(&rawtime);
  time_ = localtime(&rawtime);

  fileName += Form("%i_%i__%i_%i_%i",time_->tm_hour,time_->tm_min,time_->tm_mday,time_->tm_mon+1,time_->tm_year+1900);//add timestamp for when analyzer run

  fileName += ".root";

  fTreeFile = TFile::Open(fileName.Data(), "recreate");   

  fTreeFile->cd();
  
  fEventTree = new TTree("EventTree","Environment");
  fEventTree->SetAutoSave(300000000); // autosave when 300 Mbyte written.
  fEventTree->SetMaxVirtualSize(300000000); // 300 Mbyte
  int split = 1;
  int bufsize = 64000;
  Int_t branchstyle = 1;

  if (split < 0) {branchstyle = 0; split = -1-split;}
  TTree::SetBranchStyle(branchstyle);

  fEventBranch = fEventTree->Branch("Event", "TEnvi", &gData, bufsize, split);
  fEventBranch->SetAutoDelete(kFALSE);

   /* book histos */

  probe1 = h1_book<TH1D>("probe1", "probe1", 200, 23, 26);
  probe2 = h1_book<TH1D>("probe2", "probe2", 200, 23, 26);
  probe3 = h1_book<TH1D>("probe3", "probe3", 200, 23, 26);
  probe4 = h1_book<TH1D>("probe4", "probe4", 200, 23, 26);
  probe5 = h1_book<TH1D>("probe5", "probe5", 200, 23, 26);
  probe6 = h1_book<TH1D>("probe6", "probe6", 200, 23, 26);
  probe7 = h1_book<TH1D>("probe7", "probe7", 200, 23, 26);
  probe8 = h1_book<TH1D>("probe8", "probe8", 200, 23, 26);

  capac1 = h1_book<TH1D>("capac1", "capac1", 200, 0, 12);
  capac2 = h1_book<TH1D>("capac2", "capac2", 200, 0, 12);
  capac3 = h1_book<TH1D>("capac3", "capac3", 200, 0, 12);
  capac4 = h1_book<TH1D>("capac4", "capac4", 200, 0, 12);

  //char *timeStamp[30];

   return SUCCESS;
}


/*-----exit routine-------------*/

INT envi_exit(void)
{
  
   fTreeFile->Write();
   fTreeFile->Close();
 
   return SUCCESS;
}

/*-- EOR routine ---------------------------------------------------*/

INT envi_eor(INT run_number)
{
   return SUCCESS;
}

/*-- event routine -------------------------------------------------*/

INT envi_copy(EVENT_HEADER * pheader, void *pevent)
{

   INT end, start, n;
   float *pin;
   float *pout;

   /* look for envi bank */
   n = bk_locate(pevent, "INPT", &pin);
   if (n == 0)
      return 1;

   /* create  bank */
   bk_create(pevent, "TEMP", TID_FLOAT, &pout);

   //set copy bounds indicies
   start =0; end =11; 

   /* copy partial bank*/
   for (INT i = start; i < end; i++) {
	if ( i >= n ) continue;
     	pout[i] = (float) pin[i] ;
	//printf("%d %f\n",i,pout[i]);
        
   }

        probe1->Fill(pout[0]);
        probe2->Fill(pout[1]);
        probe3->Fill(pout[2]);
        probe4->Fill(pout[3]);
        probe5->Fill(pout[4]);
        probe6->Fill(pout[5]);
        probe7->Fill(pout[6]);
        probe8->Fill(pout[7]);

        capac1->Fill(pout[8]);
        capac2->Fill(pout[9]);
        capac3->Fill(pout[10]);
        capac4->Fill(pout[11]);

   /* close bank */
   bk_close(pevent, pout + (end-start));

   gData->tStamp = pheader->time_stamp;
   gData->p1val = pout[0];
   gData->p2val = pout[1];
   gData->p3val = pout[2];
   gData->p4val = pout[3];
   gData->p5val = pout[4];
   gData->p6val = pout[5];
   gData->p7val = pout[6];
   gData->p8val = pout[7];
   gData->c1val = pout[8];
   gData->c2val = pout[9];
   gData->c3val = pout[10];
   gData->c4val = pout[11];

   fEventTree->Fill();

   return SUCCESS;
}
