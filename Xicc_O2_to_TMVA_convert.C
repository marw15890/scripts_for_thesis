#include <cstdlib>
#include <iostream>
#include <map>
#include <string>

#include "TChain.h"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "TObjString.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TClass.h"

/// <summary>
/// Macro to convert the output of an O2 task, to split root files for Pt bins and sig/bkg, to be used for TMVA
/// </summary>
void Xicc_O2_to_TMVA_convert()
{
  //TString oldfileName = "/Users/mohammad/alice/Run3Analysisvalidation/codeHF/AnalysisResults_trees_O2.root";
  //TString oldfileName = "/Users/mohammad/alice/analysisresults/bbbar10files/AnalysisResults_trees_O2.root";
  TString oldfileName = "/Users/mohammad/alice/analysisresults/mb10files/AnalysisResults_trees_O2.root";
  //TString oldfileName2 = "/Users/mohammad/alice/analysisresults/bbbar10files/AnalysisResults_trees_O2.root";
  TString tempfileName = "/Users/mohammad/alice/Run3Analysisvalidation/codeHF/AnalysisResults_trees_O2_TMP.root";
  TString newfileDir = "/Users/mohammad/alice/mergeddata/";
  TString newfileNamePrefix = "Xicc_binned";
  
  TString createdir = "mkdir -p " + newfileDir; //create directory if it doesn't exist yet
  gSystem->Exec(createdir);
  
  // PtBins - settings
  //const Int_t nPtBins = 12;
  //Float_t ptBins[nPtBins + 1] = {0., 0.5, 1., 2., 3., 4., 5., 7., 10., 13., 16., 20., 24.};

  TFile oldFile(oldfileName);
  //TFile oldFile2(oldfileName2);
  TTree* oldtree;
  //TTree* oldtree2;
  TList* list = new TList;

  for (int i = 0; i < 6000; i++) {          //Merge all output from all AOD files and only take relevant tree
    TString objectstring = Form("DF_%d/O2hfxicc4full", i);
    oldFile.GetObject(objectstring, oldtree);
    if (oldtree == nullptr) {continue;}
    list->Add(oldtree); 
  }
  /*for (int i = 0; i < 6000; i++) {          //Merge all output from all AOD files and only take relevant tree
    TString objectstring = Form("DF_%d/O2hfxicc4full", i);
    oldFile2.GetObject(objectstring, oldtree2);
    if (oldtree2 == nullptr) {continue;}
    list->Add(oldtree2);
   }*/
  TFile TMPFile(tempfileName, "RECREATE");

  auto newtree = TTree::MergeTrees(list);
  newtree->SetName("O2hfxicc4full");

  TMPFile.Write();

  TTree* tmptree;

  TMPFile.GetObject("O2hfxicc4full", tmptree);
  if (tmptree == nullptr) {
    printf("tree not found");
  }
  
  Long64_t nentries = tmptree->GetEntries();
  float PtEntry;
  tmptree->SetBranchAddress("fPt", &PtEntry);
  Char_t MCflagEntry;
  tmptree->SetBranchAddress("fMCflag", &MCflagEntry);

  //for (int i = 0; i < nPtBins; i++) { //Split all events into different Pt bins
    //Float_t PtLow = ptBins[i];
    //Float_t PtHigh = ptBins[i + 1];
    TString SB = "_bkg";
    for (Int_t j = 0; j < 2; j++){ //j==1 signal, j==0 bkg
      if (j == 1) SB = "_signal";

      TString newfileName = newfileDir + newfileNamePrefix + SB + ".root" ;//+ Form("_Pt%.1f.root", PtLow);
      TFile newFile(newfileName, "RECREATE");
      TTree* newtree = tmptree->CloneTree(0);
      newtree->SetName("O2hfxicc4full");

      for (Long64_t i = 0; i < nentries; i++) {
        tmptree->GetEntry(i);
        if ((Float_t)MCflagEntry != j) continue; //skip if not correct sig/bkg
        //if (PtEntry >= PtLow && PtEntry < PtHigh)
          newtree->Fill();
      }
      newFile.Write();
    }
  //}

  TString DelTMPFile = "rm " + tempfileName;
  gSystem->Exec(DelTMPFile);
}
