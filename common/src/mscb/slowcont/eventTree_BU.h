//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Mon Sep 23 16:28:30 2013 by ROOT version 5.34/09
// from TTree EventTree/Temperature readings
// found on file: tempTree.root
//////////////////////////////////////////////////////////

#ifndef eventTree_h
#define eventTree_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.

// Fixed size dimensions of array or collections stored in the TTree if any.

class eventTree {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

   // Declaration of leaf types
 //TTemps          *Event;
   Int_t           tStamp;
   Float_t         p1val;
   Float_t         p2val;
   Float_t         p3val;
   Float_t         p4val;
   Float_t         p5val;
   Float_t         p6val;
   Float_t         p7val;
   Float_t         p8val;

   // List of branches
   TBranch        *b_Event_tStamp;   //!
   TBranch        *b_Event_p1val;   //!
   TBranch        *b_Event_p2val;   //!
   TBranch        *b_Event_p3val;   //!
   TBranch        *b_Event_p4val;   //!
   TBranch        *b_Event_p5val;   //!
   TBranch        *b_Event_p6val;   //!
   TBranch        *b_Event_p7val;   //!
   TBranch        *b_Event_p8val;   //!

   eventTree(TTree *tree=0);
   virtual ~eventTree();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef eventTree_cxx
eventTree::eventTree(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("tempTree.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("tempTree_13_33__19_12_2013.root");
         //f = new TFile("tempTree_19_32__17_3_2014.root");//JG:  tempTree_19_32__17_3_2014.root = run 30
      }
      f->GetObject("EventTree",tree);

   }
   Init(tree);
}

eventTree::~eventTree()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t eventTree::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t eventTree::LoadTree(Long64_t entry)
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

void eventTree::Init(TTree *tree)
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
   fChain->SetBranchAddress("p4val", &p4val, &b_Event_p4val);
   fChain->SetBranchAddress("p5val", &p5val, &b_Event_p5val);
   fChain->SetBranchAddress("p6val", &p6val, &b_Event_p6val);
   fChain->SetBranchAddress("p7val", &p7val, &b_Event_p7val);
   fChain->SetBranchAddress("p8val", &p8val, &b_Event_p8val);
   Notify();
}

Bool_t eventTree::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void eventTree::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t eventTree::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef eventTree_cxx
