#define EventTree_cxx
#include "EventTree.h"
#include <TH2.h>
#include <TStyle.h>
#include <TCanvas.h>
#include "TF1.h"
#include "TH1F.h"
#include "TFile.h"
#include <iostream>
#include <fstream>
#include "TLegend.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TLatex.h"
#include "TPostScript.h"
#include "THistPainter.h"
#include "TMath.h"
#include "TMatrixD.h"
#include "THStack.h"
#include "TGraph.h"
#include "TH2F.h"
#include "TLine.h"
#include "TArrow.h"

using namespace std;

void EventTree::Loop()
{

   if (fChain == 0) return;

   Long64_t nentries = fChain->GetEntriesFast();

   int probe1x[nentries];
   double probe1y[nentries];

   int probe2x[nentries];
   double probe2y[nentries];

   int probe3x[nentries];
   double probe3y[nentries];
 
   for (int i=0; i<nentries; i++){
   
     probe1x[i] = 0;
     probe1y[i] = 0;

     probe2x[i] = 0;
     probe2y[i] = 0;

     probe3x[i] = 0;
     probe3y[i] = 0;
   
   }

   Long64_t nbytes = 0, nb = 0;
   for (Long64_t jentry=0; jentry<nentries;jentry++) {
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
      nb = fChain->GetEntry(jentry);   nbytes += nb;

      probe1x[jentry] = tStamp;
      
      //cout<<"tStamp is "<<tStamp<<endl;
      
      // if (Cut(ientry) < 0) continue;
   }
}
