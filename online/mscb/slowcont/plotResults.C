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

/*
to run:

root -b -q calc.C++
*/

TH1F *nPiwFullError_WS;
TH1F *nPiwStatError_WS;
TH1F *nPiwFullError_RS;
TH1F *nPiwStatError_RS;

void plotResults(){

// declare the fit functions
TF1 *myf_RS;
TF1 *myf_WS;

myf_RS = new TF1("myf_RS", "pol0[0]",0,5);
myf_WS = new TF1("myf_WS", "pol0[0]",0,5);

TF1 *myf_RS_stat;
TF1 *myf_WS_stat;

myf_RS_stat    = new TF1("myf_RS_stat", "pol0[0]",0,7);
myf_WS_stat    = new TF1("myf_WS_stat", "pol0[0]",0,7);

const Int_t nx = 8;
char *labels[nx] = {"jul07 1+2 abs","jul07 0 abs","sep09","mar10", "oct10", "may11","mar12","ALL"};

gROOT->SetStyle("Plain");
gStyle->SetOptStat(0000);
gStyle->SetErrorX(0);

nPiwFullError_WS = new TH1F("nPiwFullError_WS","nPi, full errors",8,0,8);
nPiwStatError_WS = new TH1F("nPiwStatError_WS","nPi, stat error only",8,0,8);

nPiwFullError_RS = new TH1F("nPiwFullError_RS","",8,0,8);
nPiwStatError_RS = new TH1F("nPiwStatError_RS","",8,0,8);

nPiwFullError_RS->Fill(labels[0],1.19);
nPiwFullError_WS->Fill(labels[0],1.13);
nPiwFullError_RS->SetBinError(1,pow(pow(0.31,2) + pow(0.005,2),0.5));
nPiwFullError_WS->SetBinError(1,pow(pow(0.28,2) + pow(0.005,2),0.5));

nPiwFullError_RS->Fill(labels[1],1.27);
nPiwFullError_WS->Fill(labels[1],0.88);
nPiwFullError_RS->SetBinError(2,pow(pow(0.21,2) + pow(0.005,2),0.5));
nPiwFullError_WS->SetBinError(2,pow(pow(0.24,2) + pow(0.005,2),0.5));

nPiwFullError_RS->Fill(labels[2],1.11);
nPiwFullError_WS->Fill(labels[2],1.01);
nPiwFullError_RS->SetBinError(3,pow(pow(0.22,2) + pow(0.005,2),0.5));
nPiwFullError_WS->SetBinError(3,pow(pow(0.25,2) + pow(0.005,2),0.5));

nPiwFullError_RS->Fill(labels[3],1.25);
nPiwFullError_WS->Fill(labels[3],0.96);
nPiwFullError_RS->SetBinError(4,pow(pow(0.22,2) + pow(0.005,2),0.5));
nPiwFullError_WS->SetBinError(4,pow(pow(0.26,2) + pow(0.005,2),0.5));

nPiwFullError_RS->Fill(labels[4],1.23);
nPiwFullError_WS->Fill(labels[4],0.89);
nPiwFullError_RS->SetBinError(5,pow(pow(0.22,2) + pow(0.005,2),0.5));
nPiwFullError_WS->SetBinError(5,pow(pow(0.25,2) + pow(0.005,2),0.5));

nPiwFullError_RS->Fill(labels[5],1.22);
nPiwFullError_WS->Fill(labels[5],0.88);
nPiwFullError_RS->SetBinError(6,pow(pow(0.22,2) + pow(0.005,2),0.5));
nPiwFullError_WS->SetBinError(6,pow(pow(0.24,2) + pow(0.005,2),0.5));

nPiwFullError_RS->Fit("myf_RS", "RNOQ");
nPiwFullError_WS->Fit("myf_WS", "RNOQ");

nPiwFullError_RS->Fill(labels[6],1.25);
nPiwFullError_WS->Fill(labels[6],0.89);
nPiwFullError_RS->SetBinError(7,pow(pow(0.22,2) + pow(0.005,2),0.5));
nPiwFullError_WS->SetBinError(7,pow(pow(0.25,2) + pow(0.005,2),0.5));

nPiwFullError_RS->Fill(labels[7],1.21);
nPiwFullError_WS->Fill(labels[7],0.93);
nPiwFullError_RS->SetBinError(8,pow(pow(0.22,2) + pow(0.005,2),0.5));
nPiwFullError_WS->SetBinError(8,pow(pow(0.23,2) + pow(0.005,2),0.5));

nPiwStatError_RS->Fill(labels[0],1.15);
nPiwStatError_WS->Fill(labels[0],1.11);
nPiwStatError_RS->SetBinError(1,pow(pow(0.06,2) + pow(0.005,2),0.5));
nPiwStatError_WS->SetBinError(1,pow(pow(0.10,2) + pow(0.005,2),0.5));

nPiwStatError_RS->Fill(labels[1],1.24);
nPiwStatError_WS->Fill(labels[1],0.87);
nPiwStatError_RS->SetBinError(2,pow(pow(0.03,2) + pow(0.005,2),0.5));
nPiwStatError_WS->SetBinError(2,pow(pow(0.07,2) + pow(0.005,2),0.5));

nPiwStatError_RS->Fill(labels[2],1.12);
nPiwStatError_WS->Fill(labels[2],1.02);
nPiwStatError_RS->SetBinError(3,pow(pow(0.04,2) + pow(0.005,2),0.5));
nPiwStatError_WS->SetBinError(3,pow(pow(0.08,2) + pow(0.005,2),0.5));

nPiwStatError_RS->Fill(labels[3],1.25);
nPiwStatError_WS->Fill(labels[3],0.97);
nPiwStatError_RS->SetBinError(4,pow(pow(0.08,2) + pow(0.005,2),0.5));
nPiwStatError_WS->SetBinError(4,pow(pow(0.11,2) + pow(0.005,2),0.5));

nPiwStatError_RS->Fill(labels[4],1.23);//oct10
nPiwStatError_WS->Fill(labels[4],0.97);
nPiwStatError_RS->SetBinError(5,pow(pow(0.05,2) + pow(0.005,2),0.5));
nPiwStatError_WS->SetBinError(5,pow(pow(0.09,2) + pow(0.005,2),0.5));

nPiwStatError_RS->Fill(labels[5],1.21);//may11
nPiwStatError_WS->Fill(labels[5],0.90);
nPiwStatError_RS->SetBinError(6,pow(pow(0.04,2) + pow(0.005,2),0.5));
nPiwStatError_WS->SetBinError(6,pow(pow(0.07,2) + pow(0.005,2),0.5));

nPiwStatError_RS->Fill(labels[6],1.26);//mar12
nPiwStatError_WS->Fill(labels[6],0.89);
nPiwStatError_RS->SetBinError(7,pow(pow(0.03,2) + pow(0.005,2),0.5));
nPiwStatError_WS->SetBinError(7,pow(pow(0.06,2) + pow(0.005,2),0.5));

nPiwStatError_RS->Fit("myf_RS_stat", "RNOQ");
nPiwStatError_WS->Fit("myf_WS_stat", "RNOQ");

nPiwStatError_RS->Fill(labels[7],1.21);
nPiwStatError_WS->Fill(labels[7],0.94);
nPiwStatError_RS->SetBinError(8,pow(pow(0.02,2) + pow(0.005,2),0.5));
nPiwStatError_WS->SetBinError(8,pow(pow(0.03,2) + pow(0.005,2),0.5));

nPiwFullError_RS->SetMarkerColor(kRed);
nPiwFullError_RS->SetLineColor(kRed);
nPiwFullError_RS->SetLineWidth(2);
nPiwFullError_RS->SetMarkerStyle(20);
nPiwFullError_RS->SetMarkerSize(2);

nPiwFullError_WS->SetMarkerColor(kBlue);
nPiwFullError_WS->SetLineColor(kBlue);
nPiwFullError_WS->SetLineWidth(2);
nPiwFullError_WS->SetMarkerStyle(20);
nPiwFullError_WS->SetMarkerSize(2);

nPiwFullError_WS->SetMinimum(0.4);
nPiwFullError_WS->SetMaximum(1.8);
nPiwFullError_WS->SetBarWidth(10);
nPiwFullError_RS->SetBarWidth(10);
nPiwStatError_WS->SetBarWidth(10);
nPiwStatError_RS->SetBarWidth(10);

nPiwStatError_RS->SetMarkerColor(kRed);
nPiwStatError_RS->SetLineColor(kRed);
nPiwStatError_RS->SetLineWidth(2);
nPiwStatError_RS->SetMarkerStyle(20);
nPiwStatError_RS->SetMarkerSize(2);

nPiwStatError_WS->SetMarkerColor(kBlue);
nPiwStatError_WS->SetLineColor(kBlue);
nPiwStatError_WS->SetLineWidth(2);
nPiwStatError_WS->SetMarkerStyle(20);
nPiwStatError_WS->SetMarkerSize(2);

nPiwStatError_WS->SetMinimum(0.75);
nPiwStatError_WS->SetMaximum(1.50);
nPiwStatError_WS->SetBarWidth(0);

TLatex *t = new TLatex();
t->SetNDC();
t->SetTextSize(0.035);
t->SetTextColor(kRed);

TLatex *d = new TLatex();
d->SetNDC();
d->SetTextSize(0.035);
d->SetTextColor(kBlue);

TString npip = "nPip";
TString npim = "nPim";

std::cout<<" fit result is "<<myf_RS_stat->GetParameter(0)<<std::endl;

TString chisqRS = Form("%4.2f%s",myf_RS_stat->GetParameter(0),"; ");
chisqRS += "#chi^{2}/DOF = ";
chisqRS += Form("%4.1f%s",myf_RS_stat->GetChisquare(),"/");
chisqRS += myf_RS_stat->GetNDF();
chisqRS += "; Prob = ";
chisqRS += Form("%4.0f%s",TMath::Prob(myf_RS_stat->GetChisquare(),myf_RS_stat->GetNDF()) * 100," %");

TString chisqWS = Form("%4.2f%s",myf_WS_stat->GetParameter(0),"; ");
chisqWS += "#chi^{2}/DOF = ";
chisqWS += Form("%4.1f%s",myf_WS_stat->GetChisquare(),"/");
chisqWS += myf_RS_stat->GetNDF();
chisqWS += "; Prob = ";
chisqWS += Form("%4.0f%s",TMath::Prob(myf_WS_stat->GetChisquare(),myf_WS_stat->GetNDF()) * 100," %");

myf_RS->SetLineColor(kRed);
myf_WS->SetLineColor(kBlue);

myf_RS_stat->SetLineColor(kRed);
myf_WS_stat->SetLineColor(kBlue);

TCanvas *c1 = new TCanvas();
nPiwFullError_WS->Draw("e1 p");
nPiwFullError_RS->Draw("e1 p same");
//myf_RS->Draw("same");
//myf_WS->Draw("same");
t->DrawLatex(0.83,0.85,npim);
d->DrawLatex(0.83,0.80,npip);
c1->Print("nPifullError.eps");

TCanvas *c2 = new TCanvas();
nPiwStatError_WS->Draw("e1 p");
nPiwStatError_RS->Draw("e1 p same");
myf_RS_stat->Draw("same");
myf_WS_stat->Draw("same");
t->DrawLatex(0.15,0.85,chisqRS);
d->DrawLatex(0.15,0.80,chisqWS);
t->DrawLatex(0.83,0.85,npim);
d->DrawLatex(0.83,0.80,npip);
c2->Print("nPistatError_wChisq.eps");

TCanvas *c3 = new TCanvas();
nPiwStatError_WS->Draw("e1 p");
nPiwStatError_RS->Draw("e1 p same");
t->DrawLatex(0.83,0.85,npim);
d->DrawLatex(0.83,0.80,npip);
c3->Print("nPistatError.eps");

std::cout<<" chisq RS is "<<myf_RS->GetChisquare()<<" ndf is "<<myf_RS->GetNDF()<<std::endl;
std::cout<<" chisq WS is "<<myf_WS->GetChisquare()<<" ndf is "<<myf_WS->GetNDF()<<std::endl;

std::cout<<" stat chisq RS is "<<myf_RS_stat->GetChisquare()<<" ndf is "<<myf_RS_stat->GetNDF()<<std::endl;
std::cout<<" stat chisq WS is "<<myf_WS_stat->GetChisquare()<<" ndf is "<<myf_WS_stat->GetNDF()<<std::endl;

}
