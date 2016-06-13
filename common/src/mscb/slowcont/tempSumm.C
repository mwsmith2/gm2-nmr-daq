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

void tempSumm()
{

  const Int_t nx = 5;
  char *labels[nx] = {"9/30","10/15","10/31","11/19","12/19"};

  TH1F *probe1summ = new TH1F("probe1summ","",nx,0,nx);
  TH1F *probe2summ = new TH1F("probe2summ","",nx,0,nx);
  TH1F *probe3summ = new TH1F("probe3summ","",nx,0,nx);
  TH1F *probe4summ = new TH1F("probe4summ","",nx,0,nx);

  probe1summ->Fill(labels[0],0.910);//keep track of these from SC elog:36
  probe1summ->SetBinError(1,0.057);
  probe1summ->Fill(labels[1],1.088);
  probe1summ->SetBinError(2,0.018);
  probe1summ->Fill(labels[2],0.64);
  probe1summ->SetBinError(3,0.12);
  probe1summ->Fill(labels[3],0.72);
  probe1summ->SetBinError(4,0.057);
  probe1summ->Fill(labels[4],0.81);
  probe1summ->SetBinError(5,0.043);

  probe2summ->Fill(labels[0],1.280);
  probe2summ->SetBinError(1,0.026);
  probe2summ->Fill(labels[1],1.269);
  probe2summ->SetBinError(2,0.021);
  probe2summ->Fill(labels[2],1.05);
  probe2summ->SetBinError(3,0.08);
  probe2summ->Fill(labels[3],1.67);
  probe2summ->SetBinError(4,0.050);
  probe2summ->Fill(labels[4],1.42);
  probe2summ->SetBinError(5,0.028);

  probe3summ->Fill(labels[0],0.833);
  probe3summ->SetBinError(1,0.026);
  probe3summ->Fill(labels[1],1.423);
  probe3summ->SetBinError(2,0.035);
  probe3summ->Fill(labels[2],1.24);
  probe3summ->SetBinError(3,0.118);
  probe3summ->Fill(labels[3],1.60);
  probe3summ->SetBinError(4,0.055);
  probe3summ->Fill(labels[4],1.34);
  probe3summ->SetBinError(5,0.038);

  probe4summ->Fill(labels[0],-999);
  probe4summ->SetBinError(1,-999);
  probe4summ->Fill(labels[1],0.340);
  probe4summ->SetBinError(2,0.071);
  probe4summ->Fill(labels[2],0.38);
  probe4summ->SetBinError(3,0.082);
  probe4summ->Fill(labels[3],0.59);
  probe4summ->SetBinError(4,0.054);
  probe4summ->Fill(labels[4],0.48);
  probe4summ->SetBinError(5,0.119);

  probe1summ->GetXaxis()->SetTitle("run date");
  probe1summ->GetXaxis()->CenterTitle(1);
  probe1summ->GetYaxis()->SetTitle("temp. (C)");
  probe1summ->GetYaxis()->CenterTitle(1);

  probe1summ->SetMarkerColor(kBlue);
  probe1summ->SetLineColor(kBlue);
  probe2summ->SetMarkerColor(kRed);
  probe2summ->SetLineColor(kRed);
  probe3summ->SetMarkerColor(kGreen);
  probe3summ->SetLineColor(kGreen);
  probe4summ->SetMarkerColor(kCyan);
  probe4summ->SetLineColor(kCyan);

  probe1summ->SetMarkerStyle(20);
  probe2summ->SetMarkerStyle(20);
  probe3summ->SetMarkerStyle(20);
  probe4summ->SetMarkerStyle(20);

  probe1summ->SetMarkerSize(0.5);
  probe2summ->SetMarkerSize(0.5);
  probe3summ->SetMarkerSize(0.5);
  probe4summ->SetMarkerSize(0.5);

  probe1summ->SetMinimum(0);
  probe1summ->SetMaximum(2);

  probe1summ->GetXaxis()->SetLabelSize(0.05);

  TLegend *leg = new TLegend(0.155,0.691,0.344,0.860);
  leg->SetTextSize(0.035);
  leg->SetTextColor(1);
  leg->SetFillColor(0);
  leg->Clear();
  leg->AddEntry(probe1summ, "probe 1", "pl");
  leg->AddEntry(probe2summ, "probe 2", "pl");
  leg->AddEntry(probe3summ, "probe 3", "pl");
  leg->AddEntry(probe4summ, "probe 4", "pl");
  leg->SetFillColor(0);
  leg->SetLineColor(0);
  leg->SetTextFont(22);

  gStyle->SetOptStat(0000);

  TCanvas *tempSumm = new TCanvas();
  probe1summ->Draw("e1p");
  probe2summ->Draw("e1p same");
  probe3summ->Draw("e1p same");
  probe4summ->Draw("e1p same");
  leg->Draw();
  tempSumm->Print("tempSumm.eps");  

}
