#include <iostream>
#include "TLegend.h"
#include "TGraph.h"

using namespace std;

void temp()
{
  /*****************************************************************/
  // Prepare the canvas
  gStyle->SetOptStat("ne");
  gStyle->SetPalette(1);
  TCanvas *shimCanvas = (TCanvas *) gROOT->GetListOfCanvases()->At(0);
  shimCanvas->Clear();
  /*****************************************************************/

  TLegend *leg = new TLegend(0.14,0.807,0.5,0.9);

  shimCanvas->cd(); 
  TGraph* temp1 = (TGraph *)getObject("temp1");
  TGraph* temp2 = (TGraph *)getObject("temp2");

  temp1->Draw("ap");
  temp2->Draw("p same");  

  int n1 = temp1->GetN();
  int n2 = temp2->GetN();

  double lastRead1 = 0.;
  double lastRead2 = 0.;
  double dmy = 0;

  temp1->GetPoint(n1-1,dmy,lastRead1);
  temp2->GetPoint(n2-1,dmy,lastRead2);

  leg->AddEntry(temp1,Form("Probe 1, last read %2.2f C",lastRead1),"p");  
  leg->AddEntry(temp2,Form("Probe 2, last read %2.2f C",lastRead2),"p");
  leg->Draw();

}
