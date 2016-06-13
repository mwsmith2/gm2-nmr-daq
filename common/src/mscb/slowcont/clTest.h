//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Mon Sep  9 18:56:59 2013 by ROOT version 5.34/09
// from TTree EventTree/Temperature readings
// found on file: testTree.root
//////////////////////////////////////////////////////////

#ifndef clTest_h
#define clTest_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.

// Fixed size dimensions of array or collections stored in the TTree if any.

class clTest {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

   // Declaration of leaf types
 //TTemps          *Event;
   Int_t           tStamp;
   Float_t         p1val;
   Float_t         p2val;
   Float_t         p3val;

   // List of branches
   TBranch        *b_Event_tStamp;   //!
   TBranch        *b_Event_p1val;   //!
   TBranch        *b_Event_p2val;   //!
   TBranch        *b_Event_p3val;   //!

   clTest(TTree *tree=0);
   virtual ~clTest();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef clTest_cxx
clTest::clTest(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("testTree.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("testTree.root");
      }
      f->GetObject("EventTree",tree);

   }
   Init(tree);
}

clTest::~clTest()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t clTest::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t clTest::LoadTree(Long64_t entry)
{
// Set the environment to read one entry
   if (!fChain) return -5;
   Long64_t centry = fChain->LoadTree(entry);
   if (centry < 0) return centry;
   if (fChain->GetTreeNumber() != fCurrent) {
      fCurrent = fChain->GetTreeNumber();
      Notify();
   }
   return centry;
}

void clTest::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("tStamp", &tStamp, &b_Event_tStamp);
   fChain->SetBranchAddress("p1val", &p1val, &b_Event_p1val);
   fChain->SetBranchAddress("p2val", &p2val, &b_Event_p2val);
   fChain->SetBranchAddress("p3val", &p3val, &b_Event_p3val);
   Notify();
}

Bool_t clTest::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void clTest::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t clTest::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef clTest_cxx
