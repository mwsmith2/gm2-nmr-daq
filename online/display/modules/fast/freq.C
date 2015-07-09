#include <iostream>
#include "TGraph.h"

void freq()
{
  /*****************************************************************/
  // Prepare the canvas
  gStyle->SetOptStat("ne");
  gStyle->SetPalette(1);
  TCanvas *shimCanvas = (TCanvas *) gROOT->GetListOfCanvases()->At(0);
  shimCanvas->Clear();
  /*****************************************************************/

  shimCanvas->cd(); 
  TGraph* freqGr = (TGraph *)getObject("freq1");

  freqGr->SetMarkerStyle(20);

  if (freqGr->GetN()>1) freqGr->Draw("ap");
  else std::cout<<"\n\n NO DATA FOR FREQUENCY \n\n"<<std::endl;


}
