#include "TTree.h"
#include "TFile.h"
// Read data (CERN staff) from an ascii file and create a root file with a Tree.
// see also a variant in staff.C
// Author: Rene Brun

void treefill(){  

   int           evID = 0.; 
   int           tStamp = 0;
   float         temp[8];
   float         capac[4];
   char*          dmyC [80];
   int		 dmyI[5];

   //TString filename = "firsttreetest.root";
   FILE *fp = fopen("today.txt","r");
   //FILE *fp = fopen("todaybare.txt","r");

   TFile *hfile = TFile::Open("scTree.root","RECREATE");
   TTree *tree = new TTree("slowcont","SLOW CONTROL DATA");
   tree->Branch("EvID",&evID,"EventID/I");
   tree->Branch("Timestamp",&tStamp,"Timestamp/I");
   tree->Branch("Temp1",&temp[0],"Temp1/F");
   tree->Branch("Temp2",&temp[1],"Temp2/F");
   tree->Branch("Capac1",&capac[0],"Capac1/F");
   tree->Branch("Capac2",&capac[1],"Capac2/F");

   char buffer[100];

   //fscanf (file, "%d %d %d %f %d", &i, &z1, &z2, &frequency, &flag);
   while (!feof (fp)){
   //for (int i=0; i<20; i++){
      fscanf(fp,"%d %d %f %f %f %f %f %f %f %f %f %f %f %f %s %s %d %d:%d:%d %d",&evID,&tStamp,&temp[0],&temp[1],&temp[2],&temp[3],&temp[4],&temp[5],&temp[6],&temp[7],&capac[0],&capac[1],&capac[2],&capac[3],&dmyC,&dmyC,&dmyI[0],&dmyI[1],&dmyI[2],&dmyI[3],&dmyI[4]);
      if (tStamp==0) fgets(buffer, 100, fp);//check if an Error: blah line
      if (tStamp!=0) tree->Fill();
      
      /*
      printf("tStamp %d\n",tStamp); 
      for (int i=0; i<7; i++) printf(" temp %d: %f\n",i,temp[i]);
      for (int i=0; i<4; i++) printf(" capac %d: %f\n",i,capac[i]);
      printf(" dmyC: %s\n",dmyC);
      for (int i=0; i<5; i++) printf(" dmy %d: %d\n",i,dmyI[i]);
      */
   }

   //tree->Print();
   tree->Write();

   fclose(fp);
   delete hfile;
}
