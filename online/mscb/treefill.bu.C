#include "TTree.h"
#include "TFile.h"
// Read data (CERN staff) from an ascii file and create a root file with a Tree.
// see also a variant in staff.C
// Author: Rene Brun

void treefill(){  
 
   int           tStamp;
   float         temp[8];
   float         capac[4];

   //The input file cern.dat is a copy of the CERN staff data base
   //from 1988
   TString filename = "firsttreetest.root";
   FILE *fp = fopen("todaybare.txt","r");

   TFile *hfile = TFile::Open("test.root","RECREATE");
   TTree *tree = new TTree("T","SLOW CONTROL DATA");
   tree->Branch("Timestamp",&tStamp,"Timestamp/I");
   tree->Branch("Temp 1",&temp[0],"Tmp1/F");
   tree->Branch("Temp 2",&temp[1],"Tmp2/F");
   char line[800];
   while (fgets(&line,800,fp)) {
      fscanf (file, "%d %d %d %f %d", &nMeasTmp, &z1tmp, &z2tmp, &freqTmp, &flagTmp);
      sscanf(&line[0],"%d %f %f",&tStamp,&temp[0],&temp[1]);
      tree->Fill();
   }
  fscanf (file, "%d %d %d %f %d", &i, &z1, &z2, &frequency, &flag);    
  while (!feof (file)){ 

   tree->Print();
   tree->Write();

   fclose(fp);
   delete hfile;
}
