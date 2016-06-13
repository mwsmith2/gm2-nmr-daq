#include "TF1.h" 
#include "TF2.h"
#include "TFile.h" 
#include <iostream> 
#include "TLegend.h" 
#include "TStyle.h" 
#include "TCanvas.h" 
#include "TLatex.h" 
#include "TPostScript.h" 
#include "THistPainter.h" 
#include "TMath.h" 
#include "TMatrixD.h" 
#include "TH1F.h"
#include "TH2F.h"
#include "TRandom.h"
#include "TPaveStats.h"
#include "TArrow.h"
#include "THStack.h"

using namespace std;

/*
to run:

root -b -q calc.C++
*/

TString tempName, fileName;

TH2D *probe1;
TH2D *probe2;
TH2D *probe3;

void tempDisp(){

  TFile *tempFile = new TFile("tempHists.root");

  tempFile->GetObject("probe1",probe1);
  tempFile->GetObject("probe2",probe2);
  tempFile->GetObject("probe3",probe3);

  probe1->SetMarkerSize(40);
  probe1->SetMarkerColor(kBlue);

  probe2->SetMarkerSize(40);
  probe2->SetMarkerColor(kBlack);

  probe3->SetMarkerSize(40);
  probe3->SetMarkerColor(kRed);

  TCanvas *test = new TCanvas();
  probe1->Draw("p");
  probe2->Draw("p same");
  probe3->Draw("p same");
  test->Print("test.eps");

}
