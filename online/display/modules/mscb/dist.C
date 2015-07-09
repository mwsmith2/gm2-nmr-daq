#include "TLegend.h"

void dist()
{
  /*****************************************************************/
  // Prepare the canvas
  gStyle->SetOptStat("ne");
  gStyle->SetPalette(1);
  TCanvas *shimCanvas = (TCanvas *) gROOT->GetListOfCanvases()->At(0);
  shimCanvas->Clear();
  shimCanvas->Divide(1,2);
  /*****************************************************************/

  TLegend *leg1 = new TLegend(0.14,0.807,0.239,0.9);
  TLegend *leg2 = new TLegend(0.14,0.807,0.239,0.9);

  shimCanvas->cd(1); 
  TH1* dist1 = (TH1 *)getObject("dist1");
  TH1* dist2 = (TH1 *)getObject("dist2");
  dist1->Draw("ap");
  leg1->AddEntry(temp1,"Probe 1","p");
  leg1->Draw();

  shimCanvas->cd(2);
  dist2->Draw("ap");  

  leg2->AddEntry(temp2,"Probe 2","p");
  leg2->Draw();

}
