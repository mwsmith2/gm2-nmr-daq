#include "TTree.h"
#include "TFile.h"
#include "TH1F.h"
#include "TGraph.h"
#include "TAxis.h"
#include "TCanvas.h"
#include <iostream>
#include "TTimeStamp.h"

using namespace std;

int month(TString mon);

void onlinedispHists(){  

   char           str[10];
   float         temp[8];
   float         capac[4];
   int           tStamp = 0;
   int           year = 0;
   int           dayD = 0;
   int 		 hour = 0;
   int           min = 0;
   int           sec = 0;
   int           mon = 0;

   time_t rawtime;
   struct tm*  time_;
   time(&rawtime);
   time_ = localtime(&rawtime);

   int yr = time_->tm_year-100;//2-digit time since 1900
   int mo = time_->tm_mon+1;
   int day = time_->tm_mday;

   TString convMo = Form("%d",mo);
   TString convDay = Form("%d",day);

   //terrible hacky code
   if (mo==1) convMo = "01";
   if (mo==2) convMo = "02";
   if (mo==3) convMo = "03";
   if (mo==4) convMo = "04";
   if (mo==5) convMo = "05";
   if (mo==6) convMo = "06";
   if (mo==7) convMo = "07";
   if (mo==8) convMo = "08";
   if (mo==9) convMo = "09";

   if (day==1) convDay = "01";
   if (day==2) convDay = "02";
   if (day==3) convDay = "03";
   if (day==4) convDay = "04";
   if (day==5) convDay = "05";
   if (day==6) convDay = "06";
   if (day==7) convDay = "07";
   if (day==8) convDay = "08";
   if (day==9) convDay = "09";

   TString filename = "/home/cenpa/Applications/midas/online/display/hstfiles/";
   filename += "temp1_";
   filename += yr;
   filename += convMo;
   filename += convDay;
   filename += ".txt";
   //filename = "hstfiles/temp1_test.txt";

   printf(" looking for the file %s\n",filename.Data());

   FILE *fp = fopen(filename.Data(),"r");
   //FILE *fp = fopen("todaybare.txt","r");

   TString outName = "/home/cenpa/Applications/midas/online/display/rootfiles/";
   outName += yr;
   outName += convMo;
   outName += convDay;
   outName += ".root";

   TFile *hfile = TFile::Open(outName.Data(),"RECREATE");
   /* could make tree.  just do hists for online viewer for now.
   TTree *tree = new TTree("slowcont","SLOW CONTROL DATA");
   tree->Branch("Timestamp",&tStamp,"Timestamp/I");
   tree->Branch("Temp1",&temp[0],"Temp1/F");
   tree->Branch("Temp2",&temp[1],"Temp2/F");
   tree->Branch("Capac1",&capac[0],"Capac1/F");
   tree->Branch("Capac2",&capac[1],"Capac2/F");
   */

   TGraph *ltkGr = new TGraph();

   char buffer[100];
   int pt = 0;
   float distConv = 3.57;//V/MM

   int tStamp_last = 0;
   int fscfRet = 0;
   int lines_to_skip = 31;
   int numMonth = 0;
   TTimeStamp datetotstamp;

   TGraph *temp1 = new TGraph();

   while (!feof (fp)){

     for (int i=0; i<lines_to_skip; i++) fgets(buffer, 100, fp);

     fscfRet = fscanf(fp,"%s %d %d:%d:%d %d %f",str,&dayD,&hour,&min,&sec,&year,&temp[0]);
     //cout<<" fsc ret (should be 6) "<<fscfRet<<" temp "<<temp[0]<<" month "<<str<<endl;

     mon = month(str);  

     datetotstamp.Set(year,mon,dayD,hour,min,sec,0,0,0);

     tStamp = datetotstamp.GetSec();

     pt = temp1->GetN();
     temp1->SetPoint(pt,tStamp,temp[0]);

     //tree->Fill();

   }

   //get temp2
   TGraph *temp2 = new TGraph();
   filename = "/home/cenpa/Applications/midas/online/display/hstfiles/";
   filename += "temp2_";
   filename += yr;
   filename += convMo;
   filename += convDay;
   filename += ".txt";   

   printf(" looking for the file %s\n",filename.Data());

   FILE *fp2 = fopen(filename.Data(),"r");

   lines_to_skip = 30;

   while (!feof (fp2)){

     //cout<<"temp2!"<<endl;

     for (int i=0; i<lines_to_skip; i++) fgets(buffer, 100, fp2);

     fscfRet = fscanf(fp2,"%s %d %d:%d:%d %d %f",str,&dayD,&hour,&min,&sec,&year,&temp[0]);
     //cout<<" fsc ret (should be 7) "<<fscfRet<<" temp "<<temp[0]<<" month "<<str<<endl;

     mon = month(str);  

     datetotstamp.Set(year,mon,dayD,hour,min,sec,0,0,0);

     tStamp = datetotstamp.GetSec();

     pt = temp2->GetN();
     temp2->SetPoint(pt,tStamp,temp[0]);

   }

   //get capac1
   TGraph *dist1 = new TGraph();
   filename = "/home/cenpa/Applications/midas/online/display/hstfiles/";
   filename += "capac1_";
   filename += yr;
   filename += convMo;
   filename += convDay;
   filename += ".txt";   

   printf(" looking for the file %s\n",filename.Data());

   FILE *fp3 = fopen(filename.Data(),"r");

   lines_to_skip = 30;

   while (!feof (fp3)){

     //cout<<"capac1!"<<endl;

     for (int i=0; i<lines_to_skip; i++) fgets(buffer, 100, fp3);

     fscfRet = fscanf(fp3,"%s %d %d:%d:%d %d %f",str,&dayD,&hour,&min,&sec,&year,&temp[0]);
     //cout<<" fsc ret (should be 7) "<<fscfRet<<" temp "<<temp[0]<<" month "<<str<<endl;

     mon = month(str);  

     datetotstamp.Set(year,mon,dayD,hour,min,sec,0,0,0);

     tStamp = datetotstamp.GetSec();

     pt = dist1->GetN();
     dist1->SetPoint(pt,tStamp,temp[0]);

   }

   //get capac2
   TGraph *dist2 = new TGraph();
   filename = "/home/cenpa/Applications/midas/online/display/hstfiles/";
   filename += "capac2_";
   filename += yr;
   filename += convMo;
   filename += convDay;
   filename += ".txt";   

   printf(" looking for the file %s\n",filename.Data());

   FILE *fp4 = fopen(filename.Data(),"r");

   lines_to_skip = 30;

   while (!feof (fp4)){

     //cout<<"capac2!"<<endl;

     for (int i=0; i<lines_to_skip; i++) fgets(buffer, 100, fp4);

     fscfRet = fscanf(fp4,"%s %d %d:%d:%d %d %f",str,&dayD,&hour,&min,&sec,&year,&temp[0]);
     //cout<<" fsc ret (should be 7) "<<fscfRet<<" temp "<<temp[0]<<" month "<<str<<endl;

     mon = month(str);  

     datetotstamp.Set(year,mon,dayD,hour,min,sec,0,0,0);

     tStamp = datetotstamp.GetSec();

     pt = dist2->GetN();
     dist2->SetPoint(pt,tStamp,temp[0]);

   }

   
   //get frequency 1
   TGraph *freq1 = new TGraph();
   filename = "/home/cenpa/Applications/midas/online/display/hstfiles/";
   filename += "freq1_";
   filename += yr;
   filename += convMo;
   filename += convDay;
   filename += ".txt";   

   printf(" looking for the file %s\n",filename.Data());

   FILE *fp5 = fopen(filename.Data(),"r");

   lines_to_skip = 18;

   int ch;

   while ((ch = fgetc(fp5)) != EOF){

     for (int i=0; i<lines_to_skip; i++) fgets(buffer, 100, fp5);

     fscfRet = fscanf(fp5,"%s %d %d:%d:%d %d %f",str,&dayD,&hour,&min,&sec,&year,&temp[0]);
     cout<<" fsc ret (should be 7) "<<fscfRet<<" freq "<<temp[0]<<" month "<<str<<endl;

     mon = month(str);  

     datetotstamp.Set(year,mon,dayD,hour,min,sec,0,0,0);

     tStamp = datetotstamp.GetSec();

     pt = freq1->GetN();
     freq1->SetPoint(pt,tStamp,temp[0]);

     cout<<" tstamp "<<tStamp<<" freq "<<temp[0]<<endl;

   }

   //set attributes.  should it be done elsewhere?
   temp1->GetXaxis()->SetTimeDisplay(1);
   temp1->GetXaxis()->SetTimeFormat("#splitline{%H:%M}{%d/%m} %F 1970-01-01 00:00:00");
   temp1->GetXaxis()->SetTitle("Time (H:M, dd/mm)");
   temp1->GetXaxis()->SetLabelOffset(0.015);
   temp1->GetXaxis()->SetTitleOffset(1.5);
   temp1->GetYaxis()->SetTitleOffset(1.5);
   temp1->GetYaxis()->SetTitle("deg C");
   temp1->GetYaxis()->CenterTitle(1);
   temp1->SetMarkerStyle(7);
   temp1->SetMarkerColor(2);

   temp2->GetXaxis()->SetTimeDisplay(1);
   temp2->GetXaxis()->SetTimeFormat("#splitline{%H:%M}{%d/%m} %F 1970-01-01 00:00:00");
   temp2->GetXaxis()->SetTitle("Time (H:M, dd/mm)");
   temp2->GetXaxis()->SetLabelOffset(0.015);
   temp2->GetXaxis()->SetTitleOffset(1.5);
   temp2->GetYaxis()->SetTitleOffset(1.5);
   temp2->GetYaxis()->SetTitle("deg C");
   temp2->GetYaxis()->CenterTitle(1);
   temp2->SetMarkerStyle(7);
   temp2->SetMarkerColor(4);

   dist1->GetXaxis()->SetTimeDisplay(1);
   dist1->GetXaxis()->SetTimeFormat("#splitline{%H:%M}{%d/%m} %F 1970-01-01 00:00:00");
   dist1->GetXaxis()->SetLabelOffset(0.03);
   dist1->GetXaxis()->SetTitleOffset(3);
   dist1->GetYaxis()->SetTitle("distance (mm)");
   dist1->GetYaxis()->CenterTitle(1);
   dist2->GetXaxis()->SetTimeDisplay(1);
   dist2->GetXaxis()->SetTimeFormat("#splitline{%H:%M}{%d/%m} %F 1970-01-01 00:00:00");
   dist2->GetXaxis()->SetLabelOffset(0.03);
   dist2->GetXaxis()->SetTitleOffset(1.5);
   dist2->GetYaxis()->SetTitle("distance (mm)");
   dist2->GetYaxis()->CenterTitle(1);
   dist1->GetYaxis()->SetLabelSize(0.05);
   dist2->GetYaxis()->SetLabelSize(0.05);
   dist1->GetYaxis()->SetTitleSize(0.05);
   dist2->GetYaxis()->SetTitleSize(0.05);
   dist1->GetYaxis()->SetTitleOffset(0.8);
   dist2->GetYaxis()->SetTitleOffset(0.8);
   dist1->SetMarkerStyle(7);
   dist2->SetMarkerStyle(7);
   dist1->SetMarkerColor(2);
   dist2->SetMarkerColor(4);
   dist1->GetXaxis()->SetLabelSize(0.05);
   dist2->GetXaxis()->SetLabelSize(0.05);

   freq1->GetXaxis()->SetTimeDisplay(1);
   freq1->GetXaxis()->SetTimeFormat("#splitline{%H:%M}{%d/%m} %F 1970-01-01 00:00:00");
   freq1->GetXaxis()->SetTitle("Time (H:M, dd/mm)");
   freq1->GetXaxis()->SetLabelOffset(0.015);
   freq1->GetXaxis()->SetTitleOffset(1.5);
   freq1->GetYaxis()->SetTitleOffset(1.5);
   freq1->GetYaxis()->SetTitle("extracted frequency (Hz)");
   freq1->GetYaxis()->CenterTitle(1);
   freq1->SetMarkerStyle(7);
   freq1->SetMarkerColor(2);
   if (freq1->GetN()==2) freq1->SetTitle("NO DATA!");

   if (temp2->GetHistogram()->GetMaximum() > temp1->GetHistogram()->GetMaximum()) temp1->GetHistogram()->SetMaximum(temp2->GetHistogram()->GetMaximum());
   if (temp2->GetHistogram()->GetMinimum() < temp1->GetHistogram()->GetMinimum()) temp1->GetHistogram()->SetMinimum(temp2->GetHistogram()->GetMinimum());

   ltkGr->GetXaxis()->SetTimeDisplay(1);
   ltkGr->GetXaxis()->SetTimeFormat("#splitline{%H:%M}{%d/%m} %F 1970-01-01 00:00:00");
   ltkGr->GetXaxis()->SetTitle("Time (H:M, dd/mm)");
   ltkGr->GetXaxis()->SetLabelOffset(0.015);
   ltkGr->GetXaxis()->SetTitleOffset(1.5);
   ltkGr->GetYaxis()->SetTitle("laser tracker placeholder var");
   ltkGr->GetYaxis()->CenterTitle(1);
   ltkGr->SetMarkerStyle(7);
   ltkGr->SetMarkerColor(2);
   if (ltkGr->GetN()==1) ltkGr->SetTitle("NO RUNS TAKEN THIS DAY");

   //tree->Write();
   temp1->Write("temp1");
   temp2->Write("temp2");
   dist1->Write("dist1");
   dist2->Write("dist2");
   freq1->Write("freq1");
   ltkGr->Write("ltkVar");

   //new TCanvas();
   //ltkGr->Draw("ap");

   fclose(fp);
   fclose(fp2);
   fclose(fp3);
   fclose(fp4);
   fclose(fp5);
   //delete hFile;
}

int month(TString mon){

  int monVal = 0;

  if (strcmp("Jan",mon)==0) monVal = 1;

  return monVal;

}

