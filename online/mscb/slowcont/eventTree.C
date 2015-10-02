#define eventTree_cxx
#include "eventTree.h"
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
#include <time.h>
#include "TPaveStats.h"

using namespace std;

void eventTree::Loop()
{

  cout<<"lo?"<<endl;

   if (fChain == 0) return;

   Long64_t nentries = fChain->GetEntriesFast();

   const static int nEntries = nentries;

   cout<<"nEntries is "<<nEntries<<" nentries is "<<nentries<<endl;

   float probeX[nEntries];

   float probeY1[nEntries];
   float probeY2[nEntries];
   float probeY3[nEntries];
   float probeY4[nEntries];
   float probeY5[nEntries];
   float probeY6[nEntries];
   float probeY7[nEntries];
   float probeY8[nEntries];

   float probe12[nEntries];
   float probe13[nEntries];
   float probe23[nEntries];
   float probe41[nEntries];
   float probe42[nEntries];
   float probe43[nEntries];


   TH1F *probe1hist = new TH1F("probe1hist","",70,0,2);
   TH1F *probe2hist = new TH1F("probe2hist","",70,0,2);
   TH1F *probe3hist = new TH1F("probe3hist","",70,0,2);
   TH1F *probe4hist = new TH1F("probe4hist","",70,0,2);
 
   TH1F *probeDiff_hist12 = new TH1F("probe12Diff","",70,-2,2);
   TH1F *probeDiff_hist13 = new TH1F("probe13Diff","",70,-2,2);
   TH1F *probeDiff_hist23 = new TH1F("probe23Diff","",70,-2,2);
   TH1F *probeDiff_hist41 = new TH1F("probe41Diff","",70,-2,2);
   TH1F *probeDiff_hist42 = new TH1F("probe42Diff","",70,-2,2);
   TH1F *probeDiff_hist43 = new TH1F("probe43Diff","",70,-2,2);

   for (int i=0; i<nEntries; i++){
   
     probeX[i] = 0;

     probeY1[i] = 0.;
     probeY2[i] = 0.;
     probeY3[i] = 0.;
     probeY4[i] = 0.;
     probeY5[i] = 0.;
     probeY6[i] = 0.;
     probeY7[i] = 0.;
     probeY8[i] = 0.;

     probe12[i] = 0.;
     probe13[i] = 0.;
     probe23[i] = 0.;
     probe41[i] = 0.;
     probe42[i] = 0.;
     probe43[i] = 0.;   

   }

   int count = 0;

   Long64_t nbytes = 0, nb = 0;
   for (Long64_t jentry=0; jentry<nentries;jentry++) {
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
            
      if (jentry%500==0) cout<<" processed event "<<jentry<<endl; 

      nb = fChain->GetEntry(jentry);   nbytes += nb;

      probeX[jentry] = tStamp;

      probeY1[jentry] = p1val;
      probeY2[jentry] = p2val;
      probeY3[jentry] = p3val;
      probeY4[jentry] = p4val;
      probeY5[jentry] = p5val;
      probeY6[jentry] = p6val;
      probeY7[jentry] = p7val;
      probeY8[jentry] = p8val;

      probe12[jentry] = p1val - p2val;
      probe13[jentry] = p1val - p3val;
      probe23[jentry] = p2val - p3val;
      probe41[jentry] = p1val - p2val;
      probe42[jentry] = p1val - p3val;
      probe43[jentry] = p2val - p3val;

      probeDiff_hist12->Fill(p1val - p2val);
      probeDiff_hist13->Fill(p1val - p3val);
      probeDiff_hist23->Fill(p2val - p3val);
      probeDiff_hist41->Fill(p4val - p1val);
      probeDiff_hist42->Fill(p4val - p2val);
      probeDiff_hist43->Fill(p4val - p3val);

      probe1hist->Fill(p1val);
      probe2hist->Fill(p2val);
      probe3hist->Fill(p3val);
      probe4hist->Fill(p4val);

      count += 1;
      //cout<<" entry "<<jentry<<" tStamp is "<<tStamp<<" p1 "<<p1val<<" p2 "<<p2val<<" p3 "<<p3val<<" p4 "<<p4val<<endl;

   }

   cout<<"made it out of event loop!"<<endl;

   int markStyle = 6;

   TGraph *probe1 = new TGraph(count,probeX,probeY1);
   probe1->SetMarkerStyle(markStyle);
   probe1->SetMarkerColor(kBlue);
   probe1->SetLineColor(kBlue);

   TGraph *probe2 = new TGraph(count,probeX,probeY2);
   probe2->SetMarkerStyle(markStyle);
   probe2->SetMarkerColor(kRed);
   probe2->SetLineColor(kRed);

   TGraph *probe3 = new TGraph(count,probeX,probeY3);
   probe3->SetMarkerStyle(markStyle);
   probe3->SetMarkerColor(kGreen);
   probe3->SetLineColor(kGreen);

   TGraph *probe4 = new TGraph(count,probeX,probeY4);
   probe4->SetMarkerStyle(markStyle);
   probe4->SetMarkerColor(kCyan);
   probe4->SetLineColor(kCyan);

   TGraph *probe5 = new TGraph(count,probeX,probeY5);
   probe5->SetMarkerStyle(markStyle);
   probe5->SetMarkerColor(95);
   probe5->SetLineColor(95);

   TGraph *probe6 = new TGraph(count,probeX,probeY6);
   probe6->SetMarkerStyle(markStyle);
   probe6->SetMarkerColor(28);
   probe6->SetLineColor(28);

   TGraph *probe7 = new TGraph(count,probeX,probeY7);
   probe7->SetMarkerStyle(markStyle);
   probe7->SetMarkerColor(8);
   probe7->SetLineColor(8);

   TGraph *probe8 = new TGraph(count,probeX,probeY8);
   probe8->SetMarkerStyle(markStyle);
   probe8->SetMarkerColor(9);
   probe8->SetLineColor(9);

   TGraph *probeDiff12 = new TGraph(count,probeX,probe12);
   probeDiff12->SetMarkerStyle(markStyle);
   probeDiff12->SetMarkerColor(kBlue);
   probeDiff12->SetLineColor(kBlue);

   TGraph *probeDiff13 = new TGraph(count,probeX,probe13);
   probeDiff13->SetMarkerStyle(markStyle);
   probeDiff13->SetMarkerColor(kRed);
   probeDiff13->SetLineColor(kRed);

   TGraph *probeDiff23 = new TGraph(count,probeX,probe23);
   probeDiff23->SetMarkerStyle(markStyle);
   probeDiff23->SetMarkerColor(kGreen);
   probeDiff23->SetLineColor(kGreen);

   TGraph *probeDiff41 = new TGraph(count,probeX,probe41);
   probeDiff41->SetMarkerStyle(markStyle);
   probeDiff41->SetMarkerColor(28);
   probeDiff41->SetLineColor(28);

   TGraph *probeDiff42 = new TGraph(count,probeX,probe42);
   probeDiff42->SetMarkerStyle(markStyle);
   probeDiff42->SetMarkerColor(6);
   probeDiff42->SetLineColor(6);

   TGraph *probeDiff43 = new TGraph(count,probeX,probe43);
   probeDiff43->SetMarkerStyle(markStyle);
   probeDiff43->SetMarkerColor(8);
   probeDiff43->SetLineColor(8);

   probe1->GetXaxis()->SetTimeDisplay(1);
   probe1->GetXaxis()->SetTimeFormat("%H:%M %F 1970-01-01 00:00:00");
   probe1->GetXaxis()->SetTitle("Time (H:M)");
   probe1->GetXaxis()->CenterTitle(1);
   probe1->GetYaxis()->SetTitle("Temp. reading (deg C)");
   probe1->GetYaxis()->CenterTitle(1); 
   probe1->GetYaxis()->SetTitleOffset(1.1); 

   probeDiff12->GetXaxis()->SetTimeDisplay(1);
   probeDiff12->GetXaxis()->SetTimeFormat("%H:%M %F 1970-01-01 00:00:00");
   probeDiff12->GetXaxis()->SetTitle("Time (H:M)");
   probeDiff12->GetXaxis()->CenterTitle(1);
   probeDiff12->GetYaxis()->SetTitle("Difference (deg C)");
   probeDiff12->GetYaxis()->CenterTitle(1); 
   probeDiff12->GetYaxis()->SetTitleOffset(1.1); 

   probeDiff_hist12->SetLineWidth(2);
   probeDiff_hist13->SetLineWidth(2);
   probeDiff_hist23->SetLineWidth(2);
   probeDiff_hist12->SetLineColor(kBlue);
   probeDiff_hist13->SetLineColor(kRed);
   probeDiff_hist23->SetLineColor(kGreen);
   probeDiff_hist41->SetLineWidth(2);
   probeDiff_hist42->SetLineWidth(2);
   probeDiff_hist43->SetLineWidth(2);
   probeDiff_hist41->SetLineColor(28);
   probeDiff_hist42->SetLineColor(6);
   probeDiff_hist43->SetLineColor(8);
   
   probeDiff_hist12->GetXaxis()->SetTitle("Difference (deg C)");
   probeDiff_hist12->GetXaxis()->CenterTitle(1);

   TLegend *leg = new TLegend(0.70,0.17,0.89,0.34);
   leg->SetTextSize(0.035);
   leg->SetTextColor(1);
   leg->SetFillColor(0);
   leg->Clear();
   leg->AddEntry(probe1, "probe 1", "pl");
   leg->AddEntry(probe2, "probe 2", "pl");
   leg->AddEntry(probe3, "probe 3", "pl");
   leg->AddEntry(probe4, "probe 4", "pl");
   leg->AddEntry(probe5, "probe 5", "pl");
   leg->AddEntry(probe6, "probe 6", "pl");
   leg->AddEntry(probe7, "probe 7", "pl");
   leg->AddEntry(probe8, "probe 8", "pl");
   leg->SetFillColor(0);
   leg->SetLineColor(0);
   leg->SetTextFont(22);

   TLegend *leg2 = new TLegend(0.70,0.17,0.89,0.34);
   leg2->SetTextSize(0.035);
   leg2->SetTextColor(1);
   leg2->SetFillColor(0);
   leg2->Clear();
   leg2->AddEntry(probeDiff12, "probe 1 - 2", "pl");
   leg2->AddEntry(probeDiff13, "probe 1 - 3", "pl");
   leg2->AddEntry(probeDiff23, "probe 2 - 3", "pl");
   leg2->AddEntry(probeDiff41, "probe 4 - 1", "pl");
   leg2->AddEntry(probeDiff42, "probe 4 - 2", "pl");
   leg2->AddEntry(probeDiff43, "probe 4 - 3", "pl");
   leg2->SetFillColor(0);
   leg2->SetLineColor(0);
   leg2->SetTextFont(22);

   probe1->SetMaximum(probe1->GetHistogram()->GetMaximum() * 1.03);
   probe1->SetMinimum(probe1->GetHistogram()->GetMinimum() * 0.97);

   probe1->SetTitle("");
   probe1->SetMinimum(15);
   probe1->SetMaximum(22);
   probeDiff12->SetTitle("");
   
   probe1->SetTitle("Run started 4/15/14");
   
   TCanvas *probeCanv = new TCanvas();
   probe1->Draw("ap");
   probe2->Draw("p");
   probe3->Draw("p");
   probe4->Draw("p");
   probe5->Draw("p");
   probe6->Draw("p");
   probe7->Draw("p");
   probe8->Draw("p");
   leg->Draw();
   probeCanv->Print("probes.eps");

   return;

   probeDiff_hist12->SetMaximum(10000);

   TCanvas *probeDiffCanv = new TCanvas();
   probeDiff_hist12->Draw();
   
   probeDiffCanv->Modified();
   probeDiffCanv->Update();
   
   TPaveStats *st2 = (TPaveStats*)probeDiffCanv->GetPrimitive("stats");
   st2->SetX1NDC(0.78);
   st2->SetX2NDC(0.98);
   st2->SetY1NDC(0.615);
   st2->SetY2NDC(0.775); 				     	

   probeDiffCanv->Modified();
   probeDiffCanv->Update();

   probeDiff_hist13->Draw("sames");
   probeDiff_hist41->Draw("sames");
   probeDiff_hist42->Draw("sames");
   probeDiff_hist43->Draw("sames");

   probeDiffCanv->Modified();
   probeDiffCanv->Update();
   
   TPaveStats *st3 = (TPaveStats*)probeDiffCanv->GetPrimitive("stats");
   st3->SetX1NDC(0.78);
   st3->SetX2NDC(0.98);
   st3->SetY1NDC(0.615);
   st3->SetY2NDC(0.455); 				     	

   probeDiff_hist23->Draw("sames");   

   leg2->Draw();
   probeDiffCanv->Print("probeDiffs.eps");
   probeDiff12->SetMinimum(-1);
   probeDiff12->SetMaximum(1);

   TCanvas *probeDiffGrs = new TCanvas();
   probeDiff12->Draw("ap");
   probeDiff13->Draw("p");
   probeDiff23->Draw("p");
   probeDiff41->Draw("p");
   probeDiff42->Draw("p");
   probeDiff43->Draw("p");
   leg2->Draw();   
   probeDiffGrs->Print("probeDiffsInTime.eps");

   probe1hist->SetLineColor(kBlue);
   probe1hist->SetLineWidth(2);
   
   probe2hist->SetLineColor(kRed);
   probe2hist->SetLineWidth(2);
   
   probe3hist->SetLineColor(kGreen);
   probe3hist->SetLineWidth(2);

   probe4hist->SetLineColor(kCyan);
   probe4hist->SetLineWidth(2);

   probe1hist->SetMaximum(8000);
   probe1hist->GetXaxis()->SetTitle("Temp. reading (deg C)");
   probe1hist->GetXaxis()->CenterTitle(1);

   TCanvas *probeHist = new TCanvas();
   probe1hist->Draw();
   probe2hist->Draw("sames");
   probe3hist->Draw("sames");
   probe4hist->Draw("sames");
   leg->Draw();
   probeHist->Print("probeHists.eps");

}
