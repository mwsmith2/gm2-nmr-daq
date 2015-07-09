#include <iostream>

void ltk()
{
  /*****************************************************************/
  // Prepare the canvas
  gStyle->SetOptStat("ne");
  gStyle->SetPalette(1);
  TCanvas *shimCanvas = (TCanvas *) gROOT->GetListOfCanvases()->At(0);
  shimCanvas->Clear();
  /*****************************************************************/

  shimCanvas->cd(); 
  TGraph* ltkHist = (TGraph *)getObject("ltkVar");

  if (ltkHist->GetN()>1) ltkHist->Draw("ap");
  else std::cout<<"\n\n NO DATA FOR LASER TRACKER \n\n"<<std::endl;


}
