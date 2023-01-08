
 /// As input data is used a toy-MC sample consisting of four Gaussian-distributed
 /// and linearly correlated input variables.
 /// The methods to be used can be switched on and off by means of booleans, or
 /// via the prompt command, for example:
 ///
 ///     root -l ./TMVAClassification.CUndefined control sequence \"
 ///
 /// (note that the backslashes are mandatory)
 /// If no method given, a default set of classifiers is used.
 /// The output file "TMVA.root" can be analysed with the use of dedicated
 /// macros (simply say: root -l <macro.C>), which can be conveniently
 /// invoked through a GUI that will appear at the end of the run.
 /// Launch the GUI via the command:
 ///
 ///     root -l ./TMVAGui.C
 ///
 /// You can also compile and run the example with the following commands
 ///
 ///     make
 ///     ./TMVAClassification <Methods>
 ///
 /// where: `<Methods> = "method1 method2"` are the TMVA classifier names
 /// example:
 ///
 ///     ./TMVAClassification Fisher LikelihoodPCA BDT
 ///
 /// If no method given, a default set is of classifiers is used
 ///
 /// - Project   : TMVA - a ROOT-integrated toolkit for multivariate data analysis
 /// - Package   : TMVA
 /// - Root Macro: TMVAClassification



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
 

 #include "TMVA/Factory.h"
 #include "TMVA/DataLoader.h"
 #include "TMVA/Tools.h"
 #include "TMVA/TMVAGui.h"

 int Xicc_BDT_test( TString myMethodList = "" )
 {
    // The explicit loading of the shared libTMVA is done in TMVAlogon.C, defined in .rootrc
    // if you use your private .rootrc, or run from a different directory, please copy the
    // corresponding lines from .rootrc

    // Methods to be processed can be given as an argument; use format:
    //
    //     mylinux~> root -l TMVAClassification.CUndefined control sequence \"

    //---------------------------------------------------------------
    // This loads the library
    TMVA::Tools::Instance();

    // Default MVA methods to be trained + tested
    std::map<std::string,int> Use;

    // Boosted Decision Trees
    Use["BDT"]             = 1; // uses Adaptive Boost
    Use["BDTG"]            = 1; // uses Gradient Boost
    Use["BDTB"]            = 0; // uses Bagging
    Use["BDTD"]            = 0; // decorrelation + Adaptive Boost
    Use["BDTF"]            = 1; // allow usage of fisher discriminant for node splitting
    //
    // Friedman's RuleFit method, ie, an optimised series of cuts ("rules")
    Use["RuleFit"]         = 0;
    // ---------------------------------------------------------------

    std::cout << std::endl;
    std::cout << "==> Start TMVAClassification" << std::endl;

    // Select methods (don't look at this code - not of interest)
    if (myMethodList != "") {
       for (std::map<std::string,int>::iterator it = Use.begin(); it != Use.end(); it++) it->second = 0;

       std::vector<TString> mlist = TMVA::gTools().SplitString( myMethodList, ',' );
       for (UInt_t i=0; i<mlist.size(); i++) {
          std::string regMethod(mlist[i]);

          if (Use.find(regMethod) == Use.end()) {
             std::cout << "Method \"" << regMethod << "\" not known in TMVA under this name. Choose among the following:" << std::endl;
             for (std::map<std::string,int>::iterator it = Use.begin(); it != Use.end(); it++) std::cout << it->first << " ";
             std::cout << std::endl;
             return 1;
          }
          Use[regMethod] = 1;
       }
    }

    // --------------------------------------------------------------------------------------------------

    // Here the preparation phase begins

    // Read training and test data
    // (it is also possible to use ASCII format as input -> see TMVA Users Guide)
    TFile *inputSignal(0);
    TString fnamesig = "/Users/mohammad/alice/mergeddata/signal730precut/Xicc_binned_signal.root"; 
    if (!gSystem->AccessPathName( fnamesig )) {
       inputSignal = TFile::Open( fnamesig ); // check if file in local directory exists
    }
    if (!inputSignal) {
       std::cout << "ERROR: could not open data file" << std::endl;
       exit(1);
    }
    std::cout << "--- TMVAClassification       : Using input file: " << inputSignal->GetName() << std::endl;

    TFile *inputBackground(0);
    TString fnamebkg = "/Users/mohammad/alice/mergeddata/bbbar9620precuts/Xicc_binned_bkg.root"; 
    if (!gSystem->AccessPathName(fnamebkg)) {
      inputBackground = TFile::Open(fnamebkg); // check if file in local directory exists
    }
    if (!inputBackground) {
      std::cout << "ERROR: could not open data file" << std::endl;
      exit(1);
    }
    std::cout << "--- TMVAClassification       : Using input file: " << inputBackground->GetName() << std::endl;

    // Register the training and test trees
    TTree* signalTree = (TTree*)inputSignal->Get("O2hfxicc4full"); //Get("DF_0/O2hfxicc4full")
    TTree* backgroundTree = (TTree*)inputBackground->Get("O2hfxicc4full");

    signalTree->Print();
    backgroundTree->Print();
    signalTree->AutoSave();
    backgroundTree->AutoSave();
    for (int i = 0; i < 2; i++)
    {
      
    
    
    // Create a ROOT output file where TMVA will store ntuples, histograms, etc.
    TString outfileName= Form("TMVA%.d",  i);

    //TString outfileName( "TMVA.root" );
    //TFile* outputFile = TFile::Open( outfileName, "RECREATE" );
    TFile* outputFile = TFile::Open(outfileName, "RECREATE" );

    // Create the factory object. Later you can choose the methods whose performance you'd like to investigate. The factory is
    // the only TMVA object you have to interact with
    //
    // The first argument is the base of the name of all the weightfiles in the directory weight/
    //
    // The second argument is the output file for the training results
    // All TMVA output can be suppressed by removing the "!" (not) in
    // front of the "Silent" argument in the option string
    TMVA::Factory *factory = new TMVA::Factory( "TMVAClassification", outputFile,
                                                "!V:!Silent:Color:DrawProgressBar:Transformations=I;D;P;G,D:AnalysisType=Classification" );

    TMVA::DataLoader *dataloader=new TMVA::DataLoader("dataset");
    // If you wish to modify default settings
    //    (TMVA::gConfig().GetVariablePlotting()).fTimesRMS = 8.0;
    //    (TMVA::gConfig().GetIONames()).fWeightFileDir = "myWeightDirectory";

    // We define the input variables that will be used for the MVA training

    dataloader->AddVariable("fCPA", "fCPA", "units", 'F' );
    dataloader->AddVariable("fDecayLength", "fDecayLength", "units", 'F');
    dataloader->AddVariable("fDecayLengthXY", "fDecayLengthXY", "units", 'F');
    dataloader->AddVariable("fImpactParameter0", "fImpactParameter0", "units", 'F');
    dataloader->AddVariable("fImpactParameter1", "fImpactParameter1", "units", 'F');
    dataloader->AddVariable("fImpactParameter2", "fImpactParameter2", "units", 'F');
    dataloader->AddVariable("fImpactParameter3", "fImpactParameter3", "units", 'F');
    dataloader->AddVariable("fCPAXY", "fCPAXY", "units", 'F');
    dataloader->AddVariable("fChi2PCA", "fChi2PCA", "units", 'F');
    dataloader->AddVariable("fM", "fM", "units", 'F');


    //dataloader->AddVariable("fNSigTOFPi0", "fNSigTOFPi0", "units", 'F');
    //dataloader->AddVariable("fNSigTOFPi1", "fNSigTOFPi1", "units", 'F');
    //dataloader->AddVariable("fNSigTOFPi2", "fNSigTOFPi2", "units", 'F');
    //dataloader->AddVariable("fNSigTOFKa0", "fNSigTOFKa0", "units", 'F');
    //dataloader->AddVariable("fNSigTOFKa1", "fNSigTOFKa1", "units", 'F');
    //dataloader->AddVariable("fNSigTOFKa2", "fNSigTOFKa2", "units", 'F');
    //dataloader->AddVariable("fNSigTOFPr0", "fNSigTOFPr0", "units", 'F');
    //dataloader->AddVariable("fNSigTOFPr1", "fNSigTOFPr1", "units", 'F');
    //dataloader->AddVariable("fNSigTOFPr2", "fNSigTOFPr2", "units", 'F');

    // we add so-called "Spectator variables", which are not used in the MVA training,
    // but will appear in the final "TestTree" produced by TMVA. This TestTree will contain the
    // input variables, the response values of all trained MVAs, and the spectator variables

    //dataloader->AddSpectator( "spec1 := var1*2",  "Spectator 1", "units", 'F' );
    //dataloader->AddSpectator( "spec2 := var1*3",  "Spectator 2", "units", 'F' );
    dataloader->AddSpectator("fM", "fM", "units", 'F');
    dataloader->AddSpectator("fPt", "fPt", "Gev", 'F');


    // global event weights per tree (see below for setting event-wise weights)
    Double_t signalWeight     = 1.0;
    Double_t backgroundWeight = 1.0;

    // You can add an arbitrary number of signal or background trees
    dataloader->AddSignalTree    ( signalTree,     signalWeight );
    dataloader->AddBackgroundTree( backgroundTree, backgroundWeight );

    // To give different trees for training and testing, do as follows:
   
    //     dataloader->AddSignalTree( signalTrainingTree, signalTrainWeight, "Training" );
    //     dataloader->AddSignalTree( signalTestTree,     signalTestWeight,  "Test" );

    // End of tree registration

    // Set individual event weights (the variables must exist in the original TTree)
    // -  for signal    : `dataloader->SetSignalWeightExpression    ("weight1*weight2");`
    // -  for background: `dataloader->SetBackgroundWeightExpression("weight1*weight2");`
    
    //dataloader->SetBackgroundWeightExpression( "weight" );

    // Apply additional cuts on the signal and background samples (can be different)
    TCut mycuts =" ";//"fM< 4 && fCPA>0.9995 && fDecayLength>0.01 && fChi2PCA < 0.5e-5"; // for example: TCut mycuts = "abs(var1)<0.5 && abs(var2-0.5)<1";
    TCut mycutb =" ";//"fM< 4 && fCPA>0.9995 && fDecayLength>0.01 && fChi2PCA < 0.5e-5"; // for example: TCut mycutb = "abs(var1)<0.5";
    // for loop (binning)
    if (i == 0 ){ mycuts= "fPt > 4.0 && fPt < 6.0 && fM< 4 && fM >3.2";}
    else if (i == 1){ mycuts= "fPt > 6.0 && fPt < 15.0 && fM< 4 && fM >3.2";}
    // Tell the dataloader how to use the training and testing events
    //
    // If no numbers of events are given, half of the events in the tree are used
    // for training, and the other half for testing:
    //
    //    dataloader->PrepareTrainingAndTestTree( mycut, "SplitMode=random:!V" );
    //
    // To also specify the number of testing events, use:
    //
    //    dataloader->PrepareTrainingAndTestTree( mycut,
    //         "NSigTrain=3000:NBkgTrain=3000:NSigTest=3000:NBkgTest=3000:SplitMode=Random:!V" );
    



    int NsigTrain, NsigTest, NbkgTrain, NbkgTest ;
   //new 70-30 revised thesis
    if (i == 0){ NsigTrain = 160; NsigTest = 70; NbkgTrain = 287150    ; NbkgTest = 123000;};
    if (i == 1) { NsigTrain = 17500; NsigTest = 7500; NbkgTrain =  548800  ; NbkgTest = 235200;};




    //new 70-30 revised thesis
    //if (i == 0){ NsigTrain = 160; NsigTest = 70; NbkgTrain = 287150    ; NbkgTest = 123000;};
    //if (i == 1) { NsigTrain = 14440; NsigTest = 6190; NbkgTrain =  540500  ; NbkgTest = 231640;};


    //50-50
    //if (i == 0){ NsigTrain = 50; NsigTest = 10; NbkgTrain = 153450    ; NbkgTest = 107400;};
    //if (i == 1) { NsigTrain = 2850; NsigTest = 2850; NbkgTrain =  102350  ; NbkgTest = 102350;};
    //70-30 for the thesis first submission
    //if (i == 0){ NsigTrain = 50; NsigTest = 10; NbkgTrain = 153450    ; NbkgTest = 107400;};
    //if (i == 1) { NsigTrain = 4000; NsigTest = 1700; NbkgTrain =  143270   ; NbkgTest = 61400;};
   //85-15
    //if (i == 0){ NsigTrain = 50; NsigTest = 10; NbkgTrain = 153450    ; NbkgTest = 107400;};
    //if (i == 1) { NsigTrain = 4860; NsigTest = 850; NbkgTrain =  174000   ; NbkgTest = 30700;};
    //if (i == 1) { NsigTrain = 4000; NsigTest = 1700; NbkgTrain =  3920   ; NbkgTest = 1680;};
    //70-30 precuts
    //int NbkgTrain = 2887760 ;
    //int NsigTrain = 8100  ;
    //int NbkgTest =  1237600;
    //int NsigTest = 3500;
    /*
    //70-30 tight cuts
    int NbkgTrain = 2460 ;
    int NsigTrain = 7950  ;
    int NbkgTest =  1050;
    int NsigTest = 3400;
    //50-50 tightcuts
    int NbkgTrain = 1756 ;
    int NsigTrain = 5684  ;
    int NbkgTest =  1756;
    int NsigTest = 5684;*/

   //50-50 precuts
    //int NbkgTrain = 2062600 ;
    //int NsigTrain = 5830  ;
    //int NbkgTest =  2062600;
    //int NsigTest = 5830;

    /*int NsigTrain = signalTree->GetEntries() / 2;
    long long Nmaxbkg = 300000;
    int NbkgTrain = min(Nmaxbkg, backgroundTree->GetEntries()/2);
    int NsigTest = signalTree->GetEntries() / 2;
    int NbkgTest = min(Nmaxbkg, backgroundTree->GetEntries()/2);*/

    dataloader->PrepareTrainingAndTestTree(mycuts, NsigTrain, NbkgTrain, NsigTest, NbkgTest, "SplitMode=Random:NormMode=NumEvents:!V");

    // ### Book MVA methods
    //
    // Please lookup the various method configuration options in the corresponding cxx files, eg:
    // src/MethoCuts.cxx, etc, or here: http://tmva.sourceforge.net/optionRef.html
    // it is possible to preset ranges in the option string in which the cut optimisation should be done:
    // "...:CutRangeMin[2]=-1:CutRangeMax[2]=1"...", where [2] is the third input variable

    // Boosted Decision Trees
    if (Use["BDTG"]) // Gradient Boost
       factory->BookMethod( dataloader, TMVA::Types::kBDT, "BDTG",
                            "!H:!V:NTrees=1000:MinNodeSize=2.5%:BoostType=Grad:Shrinkage=0.10:UseBaggedBoost:BaggedSampleFraction=0.5:nCuts=20:MaxDepth=2" );

    if (Use["BDT"])  // Adaptive Boost
       factory->BookMethod( dataloader, TMVA::Types::kBDT, "BDT",
                            "!H:!V:NTrees=850:MinNodeSize=2.5%:MaxDepth=3:BoostType=AdaBoost:AdaBoostBeta=0.5:UseBaggedBoost:BaggedSampleFraction=0.5:SeparationType=GiniIndex:nCuts=20" );

    if (Use["BDTB"]) // Bagging
       factory->BookMethod( dataloader, TMVA::Types::kBDT, "BDTB",
                            "!H:!V:NTrees=400:BoostType=Bagging:SeparationType=GiniIndex:nCuts=20" );

    if (Use["BDTD"]) // Decorrelation + Adaptive Boost
       factory->BookMethod( dataloader, TMVA::Types::kBDT, "BDTD",
                            "!H:!V:NTrees=400:MinNodeSize=5%:MaxDepth=3:BoostType=AdaBoost:SeparationType=GiniIndex:nCuts=20:VarTransform=Decorrelate" );

    if (Use["BDTF"])  // Allow Using Fisher discriminant in node splitting for (strong) linearly correlated variables
       factory->BookMethod( dataloader, TMVA::Types::kBDT, "BDTF",
                            "!H:!V:NTrees=50:MinNodeSize=2.5%:UseFisherCuts:MaxDepth=3:BoostType=AdaBoost:AdaBoostBeta=0.5:SeparationType=GiniIndex:nCuts=20" );

    // RuleFit -- TMVA implementation of Friedman's method
    if (Use["RuleFit"])
       factory->BookMethod( dataloader, TMVA::Types::kRuleFit, "RuleFit",
                            "H:!V:RuleFitModule=RFTMVA:Model=ModRuleLinear:MinImp=0.001:RuleMinDist=0.001:NTrees=20:fEventsMin=0.01:fEventsMax=0.5:GDTau=-1.0:GDTauPrec=0.01:GDStep=0.01:GDNSteps=10000:GDErrScale=1.02" );

    
    //
    // --------------------------------------------------------------------------------------------------
    // We can optimize the setting (configuration) of the MVAs using the set of training events
    // STILL EXPERIMENTAL and only implemented for BDT's !
    //
    //factory->OptimizeAllMethods("SigEffAt001","Scan");
    factory->OptimizeAllMethods("ROCIntegral","FitGA");
    //
    // --------------------------------------------------------------------------------------------------

    // Now we can train, test, and evaluate the MVAs
    //
    // Train MVAs using the set of training events
    factory->TrainAllMethods();

    // Evaluate all MVAs using the set of test events
    factory->TestAllMethods();

    // Evaluate and compare performance of all configured MVAs
    factory->EvaluateAllMethods();

    // --------------------------------------------------------------

    // Save the output
    outputFile->Close();

    std::cout << "==> Wrote root file: " << outputFile->GetName() << std::endl;
    std::cout << "==> TMVAClassification is done!" << std::endl;

    delete factory;
    delete dataloader;
    // Launch the GUI for the root macros
    if (!gROOT->IsBatch()) TMVA::TMVAGui( outfileName );
    }
    return 0;
 }

 int main( int argc, char** argv )
 {
    // Select methods (don't look at this code - not of interest)
    TString methodList;
    for (int i=1; i<argc; i++) {
       TString regMethod(argv[i]);
       if(regMethod=="-b" || regMethod=="--batch") continue;
       if (!methodList.IsNull()) methodList += TString(",");
       methodList += regMethod;
    }
    return Xicc_BDT_test(methodList);
 }

