// -------------------------------------------------------------------------
// -----                        R3BGfi source file                     -----
// -----                  Created 26/03/09  by D.Bertini               -----
// -------------------------------------------------------------------------
#include "R3BGfi.h"

#include "R3BGeoGfi.h"
#include "R3BGfiPoint.h"
#include "R3BGeoGfiPar.h"

#include "FairGeoInterface.h"
#include "FairGeoLoader.h"
#include "FairGeoNode.h"
#include "FairGeoRootBuilder.h"
#include "FairRootManager.h"
#include "FairStack.h"
#include "FairRuntimeDb.h"
#include "FairRun.h"
#include "FairVolume.h"

#include "TClonesArray.h"
#include "TGeoMCGeometry.h"
#include "TParticle.h"
#include "TVirtualMC.h"
#include "TObjArray.h"

// includes for modeling
#include "TGeoManager.h"
#include "TParticle.h"
#include "TVirtualMC.h"
#include "TGeoMatrix.h"
#include "TGeoMaterial.h"
#include "TGeoMedium.h"
#include "TGeoBBox.h"
#include "TGeoPara.h"
#include "TGeoPgon.h"
#include "TGeoSphere.h"
#include "TGeoArb8.h"
#include "TGeoCone.h"
#include "TGeoBoolNode.h"
#include "TGeoCompositeShape.h"
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;



// -----   Default constructor   -------------------------------------------
R3BGfi::R3BGfi() : FairDetector("R3BGfi", kTRUE, kSTS) {
  ResetParameters();
  fGfiCollection = new TClonesArray("R3BGfiPoint");
  fPosIndex = 0;
  kGeoSaved = kFALSE;
  flGeoPar = new TList();
  flGeoPar->SetName( GetName());
  fVerboseLevel = 1;
}
// -------------------------------------------------------------------------



// -----   Standard constructor   ------------------------------------------
R3BGfi::R3BGfi(const char* name, Bool_t active) 
  : FairDetector(name, active, kSTS) {
  ResetParameters();
  fGfiCollection = new TClonesArray("R3BGfiPoint");
  fPosIndex = 0;
  kGeoSaved = kFALSE;
  flGeoPar = new TList();
  flGeoPar->SetName( GetName());
  fVerboseLevel = 1;
}
// -------------------------------------------------------------------------



// -----   Destructor   ----------------------------------------------------
R3BGfi::~R3BGfi() {

  if ( flGeoPar ) delete flGeoPar;
  if (fGfiCollection) {
    fGfiCollection->Delete();
    delete fGfiCollection;
  }
}
// -------------------------------------------------------------------------



// -----   Public method ProcessHits  --------------------------------------
Bool_t R3BGfi::ProcessHits(FairVolume* vol) {
//      cout << " -I process hit called for:" <<  vol->GetName() << endl;
// Set parameters at entrance of volume. Reset ELoss.

//    if ( vol ) {
//        cout << " Name Id:copy "
//            << vol->getName() << " : " << vol->getMCid() << " : " << vol->getCopyNo() << endl;
//        Int_t copyNo=0;
//        cout << " Geant: " << gMC->CurrentVolID(copyNo) << ":" << copyNo << endl;
//    }

    if ( gMC->IsTrackEntering() ) {
    fELoss  = 0.;
    fTime   = gMC->TrackTime() * 1.0e09;
    fLength = gMC->TrackLength();
    gMC->TrackPosition(fPosIn);
    gMC->TrackMomentum(fMomIn);
  }

  // Sum energy loss for all steps in the active volume
  fELoss += gMC->Edep();

  // Set additional parameters at exit of active volume. Create R3BGfiPoint.
  if ( gMC->IsTrackExiting()    ||
       gMC->IsTrackStop()       ||
       gMC->IsTrackDisappeared()   ) {
    fTrackID  = gMC->GetStack()->GetCurrentTrackNumber();
    fVolumeID = vol->getMCid();
    gMC->TrackPosition(fPosOut);
    gMC->TrackMomentum(fMomOut);
    if (fELoss == 0. ) return kFALSE;
    
    if (gMC->IsTrackExiting()) {
      const Double_t* oldpos;
      const Double_t* olddirection;
      Double_t newpos[3];
      Double_t newdirection[3];
      Double_t safety;
      
      gGeoManager->FindNode(fPosOut.X(),fPosOut.Y(),fPosOut.Z());
      oldpos = gGeoManager->GetCurrentPoint();
      olddirection = gGeoManager->GetCurrentDirection();
      
//       cout << "1st direction: " << olddirection[0] << "," << olddirection[1] << "," << olddirection[2] << endl;

      for (Int_t i=0; i<3; i++){
	newdirection[i] = -1*olddirection[i];
      }
      
      gGeoManager->SetCurrentDirection(newdirection);
      TGeoNode *bla = gGeoManager->FindNextBoundary(2);
      safety = gGeoManager->GetSafeDistance();


      gGeoManager->SetCurrentDirection(-newdirection[0],-newdirection[1],-newdirection[2]);
      
      for (Int_t i=0; i<3; i++){
	newpos[i] = oldpos[i] - (3*safety*olddirection[i]);
      }

      if ( fPosIn.Z() < 30. && newpos[2] > 30.02 ) {
	cerr << "2nd direction: " << olddirection[0] << "," << olddirection[1] << "," << olddirection[2] 
	     << " with safety = " << safety << endl;
	cerr << "oldpos = " << oldpos[0] << "," << oldpos[1] << "," << oldpos[2] << endl;
	cerr << "newpos = " << newpos[0] << "," << newpos[1] << "," << newpos[2] << endl;
      }

      fPosOut.SetX(newpos[0]);
      fPosOut.SetY(newpos[1]);
      fPosOut.SetZ(newpos[2]);
    }

    AddHit(fTrackID, fVolumeID,
	   TVector3(fPosIn.X(),   fPosIn.Y(),   fPosIn.Z()),
	   TVector3(fPosOut.X(),  fPosOut.Y(),  fPosOut.Z()),
	   TVector3(fMomIn.Px(),  fMomIn.Py(),  fMomIn.Pz()),
	   TVector3(fMomOut.Px(), fMomOut.Py(), fMomOut.Pz()),
	   fTime, fLength, fELoss);
    
    // Increment number of GfiPoints for this track
    FairStack* stack = (FairStack*) gMC->GetStack();
    stack->AddPoint(kSTS);
    
    ResetParameters();
  }

  return kTRUE;
}

// -----   Public method EndOfEvent   -----------------------------------------
void R3BGfi::BeginEvent() {

//  if (! kGeoSaved ) {
//      SaveGeoParams();
//  cout << "-I STS geometry parameters saved " << endl;
//  kGeoSaved = kTRUE;
//  }

}
// -----   Public method EndOfEvent   -----------------------------------------
void R3BGfi::EndOfEvent() {

  if (fVerboseLevel) Print();
  fGfiCollection->Clear();

  ResetParameters();
}
// ----------------------------------------------------------------------------



// -----   Public method Register   -------------------------------------------
void R3BGfi::Register() {
  FairRootManager::Instance()->Register("GFIPoint", GetName(), fGfiCollection, kTRUE);
}
// ----------------------------------------------------------------------------



// -----   Public method GetCollection   --------------------------------------
TClonesArray* R3BGfi::GetCollection(Int_t iColl) const {
  if (iColl == 0) return fGfiCollection;
  else return NULL;
}
// ----------------------------------------------------------------------------



// -----   Public method Print   ----------------------------------------------
void R3BGfi::Print() const {
  Int_t nHits = fGfiCollection->GetEntriesFast();
  cout << "-I- R3BGfi: " << nHits << " points registered in this event." 
       << endl;
}
// ----------------------------------------------------------------------------



// -----   Public method Reset   ----------------------------------------------
void R3BGfi::Reset() {
  fGfiCollection->Clear();
  ResetParameters();
}
// ----------------------------------------------------------------------------



// -----   Public method CopyClones   -----------------------------------------
void R3BGfi::CopyClones(TClonesArray* cl1, TClonesArray* cl2, Int_t offset) {
  Int_t nEntries = cl1->GetEntriesFast();
  cout << "-I- R3BGfi: " << nEntries << " entries to add." << endl;
  TClonesArray& clref = *cl2;
  R3BGfiPoint* oldpoint = NULL;
   for (Int_t i=0; i<nEntries; i++) {
   oldpoint = (R3BGfiPoint*) cl1->At(i);
    Int_t index = oldpoint->GetTrackID() + offset;
    oldpoint->SetTrackID(index);
    new (clref[fPosIndex]) R3BGfiPoint(*oldpoint);
    fPosIndex++;
  }
   cout << " -I- R3BGfi: " << cl2->GetEntriesFast() << " merged entries."
       << endl;
}

// -----   Private method AddHit   --------------------------------------------
R3BGfiPoint* R3BGfi::AddHit(Int_t trackID, Int_t detID, TVector3 posIn,
			    TVector3 posOut, TVector3 momIn, 
			    TVector3 momOut, Double_t time, 
			    Double_t length, Double_t eLoss) {
  TClonesArray& clref = *fGfiCollection;
  Int_t size = clref.GetEntriesFast();
  if (fVerboseLevel>1) 
    cout << "-I- R3BGfi: Adding Point at (" << posIn.X() << ", " << posIn.Y() 
	 << ", " << posIn.Z() << ") cm,  detector " << detID << ", track "
	 << trackID << ", energy loss " << eLoss*1e06 << " keV" << endl;
  return new(clref[size]) R3BGfiPoint(trackID, detID, posIn, posOut,
				      momIn, momOut, time, length, eLoss);
}
// -----   Public method ConstructGeometry   ----------------------------------
void R3BGfi::ConstructGeometry() {

  // out-of-file geometry definition
   Double_t dx,dy,dz;
   Double_t dx1, dx2, dy1, dy2;
   Double_t vert[20], par[20];
   Double_t theta, phi, h1, bl1, tl1, alpha1, h2, bl2, tl2, alpha2;
   Double_t twist;
   Double_t origin[3];
   Double_t rmin, rmax, rmin1, rmax1, rmin2, rmax2;
   Double_t r, rlo, rhi;
   Double_t a,b;
   Double_t point[3], norm[3];
   Double_t rin, stin, rout, stout;
   Double_t thx, phx, thy, phy, thz, phz;
   Double_t alpha, theta1, theta2, phi1, phi2, dphi;
   Double_t tr[3], rot[9];
   Double_t z, density, radl, absl, w;
   Double_t lx,ly,lz,tx,ty,tz;
   Double_t xvert[50], yvert[50];
   Double_t zsect,x0,y0,scale0;
   Int_t nel, numed, nz, nedges, nvert;

   TGeoBoolNode *pBoolNode = 0;


/****************************************************************************/
// Material definition

 // Vacuum
  TGeoMaterial *matVacuum = new TGeoMaterial("Vacuum", 0,0,0);
  TGeoMedium *pMed1 = new TGeoMedium("Vacuum",1, matVacuum);
  pMed1->Print();

// Mixture: Air
  nel     = 2;
  density = 0.001290;
  TGeoMixture*
  pMat2 = new TGeoMixture("Air", nel,density);
  a = 14.006740;   z = 7.000000;   w = 0.700000;  // N
  pMat2->DefineElement(0,a,z,w);
  a = 15.999400;   z = 8.000000;   w = 0.300000;  // O
  pMat2->DefineElement(1,a,z,w);
  pMat2->SetIndex(1);
  // Medium: Air
  numed   = 1;  // medium number
  TGeoMedium*
  pMed2 = new TGeoMedium("Air", numed,pMat2);

  // Mixture: plasticForGFI
  nel     = 2;
  density = 1.032000;
  TGeoMixture*
      pMat35 = new TGeoMixture("plasticForGFI", nel,density);
  a = 12.010700;   z = 6.000000;   w = 0.914708;  // C
  pMat35->DefineElement(0,a,z,w);
  a = 1.007940;   z = 1.000000;   w = 0.085292;  // H
  pMat35->DefineElement(1,a,z,w);
  pMat35->SetIndex(34);
  // Medium: plasticForGFI
  numed   = 34;  // medium number
  TGeoMedium*
      pMed35 = new TGeoMedium("plasticForGFI", numed,pMat35);

 // Material: Aluminum
  a       = 26.980000;
  z       = 13.000000;
  density = 2.700000;
  radl    = 8.875105;
  absl    = 388.793113;

  TGeoMaterial *matAl = new TGeoMaterial("Aluminum", a,z,density,radl,absl);
  TGeoMedium* pMed21 = new TGeoMedium("Aluminum",3, matAl);
  pMed21->Print();


   // TRANSFORMATION MATRICES
   // Combi transformation: 
   dx = 73.700000;
   dy = 0.000000;
   dz = 525.400000;
   // Rotation: 
   thx = 106.700000;    phx = 0.000000;
   thy = 90.000000;    phy = 90.000000;
   thz = 16.700000;    phz = 0.000000;
   TGeoRotation *pMatrix3 = new TGeoRotation("",thx,phx,thy,phy,thz,phz);
   TGeoCombiTrans*
   pMatrix2 = new TGeoCombiTrans("", dx,dy,dz,pMatrix3);
   // Combi transformation: 
   dx = 0.000000;
   dy = 0.000000;
   dz = 0.000000;
   // Rotation: 
   thx = 90.000000;    phx = 0.000000;
   thy = 90.000000;    phy = 90.000000;
   thz = 0.000000;    phz = 0.000000;
   TGeoRotation *pMatrix7 = new TGeoRotation("",thx,phx,thy,phy,thz,phz);
   TGeoCombiTrans*
   pMatrix6 = new TGeoCombiTrans("", dx,dy,dz,pMatrix7);
   // Combi transformation: 
   dx = 0.000000;
   dy = 27.000000;
   dz = 0.000000;
   // Rotation: 
   thx = 90.000000;    phx = 0.000000;
   thy = 90.000000;    phy = 90.000000;
   thz = 0.000000;    phz = 0.000000;
   TGeoRotation *pMatrix9 = new TGeoRotation("",thx,phx,thy,phy,thz,phz);
    TGeoCombiTrans*
   pMatrix8 = new TGeoCombiTrans("", dx,dy,dz,pMatrix9);
   // Combi transformation: 
   dx = 0.000000;
   dy = -27.000000;
   dz = 0.000000;
   // Rotation: 
   thx = 90.000000;    phx = 0.000000;
   thy = 90.000000;    phy = 90.000000;
   thz = 0.000000;    phz = 0.000000;
   TGeoRotation *pMatrix11 = new TGeoRotation("",thx,phx,thy,phy,thz,phz);
    TGeoCombiTrans*
   pMatrix10 = new TGeoCombiTrans("", dx,dy,dz,pMatrix11);
   // Combi transformation: 
   dx = 27.000000;
   dy = 0.000000;
   dz = 0.000000;
   // Rotation: 
   thx = 90.000000;    phx = 0.000000;
   thy = 90.000000;    phy = 90.000000;
   thz = 0.000000;    phz = 0.000000;
   TGeoRotation *pMatrix13 = new TGeoRotation("",thx,phx,thy,phy,thz,phz);
    TGeoCombiTrans*
   pMatrix12 = new TGeoCombiTrans("", dx,dy,dz,pMatrix13);
   // Combi transformation: 
   dx = -27.000000;
   dy = 0.000000;
   dz = 0.000000;
   // Rotation: 
   thx = 90.000000;    phx = 0.000000;
   thy = 90.000000;    phy = 90.000000;
   thz = 0.000000;    phz = 0.000000;
   TGeoRotation *pMatrix15 = new TGeoRotation("",thx,phx,thy,phy,thz,phz);
   TGeoCombiTrans*
   pMatrix14 = new TGeoCombiTrans("", dx,dy,dz,pMatrix15);
   // Combi transformation: 
   dx = 141.800000;
   dy = 0.000000;
   dz = 727.300000;
   // Rotation: 
   thx = 106.700000;    phx = 0.000000;
   thy = 90.000000;    phy = 90.000000;
   thz = 16.700000;    phz = 0.000000;
   TGeoRotation *pMatrix5 = new TGeoRotation("",thx,phx,thy,phy,thz,phz);
   TGeoCombiTrans*
   pMatrix4 = new TGeoCombiTrans("", dx,dy,dz,pMatrix5);


   // World definition
   TGeoVolume* pWorld = gGeoManager->GetTopVolume();
   pWorld->SetVisLeaves(kTRUE);

   // SHAPES, VOLUMES AND GEOMETRICAL HIERARCHY
   // Shape: GFIBoxWorld type: TGeoBBox
   dx = 29.000000;
   dy = 29.000000;
   dz = 0.050000;
   TGeoShape *pGFIBoxWorld_2 = new TGeoBBox("GFIBoxWorld", dx,dy,dz);
   // Volume: GFILogWorld
   TGeoVolume*
   pGFILogWorld_82ab420 = new TGeoVolume("GFILogWorld",pGFIBoxWorld_2, pMed1);
   pGFILogWorld_82ab420->SetVisLeaves(kTRUE);
   pWorld->AddNode(pGFILogWorld_82ab420, 0, pMatrix2);
   pWorld->AddNode(pGFILogWorld_82ab420, 1, pMatrix4);
   // Shape: GFIBox type: TGeoBBox
   dx = 25.000000;
   dy = 25.000000;
   dz = 0.050000;
   TGeoShape *pGFIBox_3 = new TGeoBBox("GFIBox", dx,dy,dz);
   // Volume: GFILog
   TGeoVolume*
   pGFILog_82ab5c8 = new TGeoVolume("GFILog",pGFIBox_3, pMed35);
   pGFILog_82ab5c8->SetVisLeaves(kTRUE);
   pGFILogWorld_82ab420->AddNode(pGFILog_82ab5c8, 0, pMatrix6);

   // Shape: UpFrame type: TGeoBBox
   dx = 29.000000;
   dy = 2.000000;
   dz = 0.050000;
   TGeoShape *pUpFrame_4 = new TGeoBBox("UpFrame", dx,dy,dz);
   // Volume: logicUpFrame
   TGeoVolume*
   plogicUpFrame_82ab760 = new TGeoVolume("logicUpFrame",pUpFrame_4, pMed21);
   plogicUpFrame_82ab760->SetVisLeaves(kTRUE);
   pGFILogWorld_82ab420->AddNode(plogicUpFrame_82ab760, 0, pMatrix8);
   // Shape: DownFrame type: TGeoBBox
   dx = 29.000000;
   dy = 2.000000;
   dz = 0.050000;
   TGeoShape *pDownFrame_5 = new TGeoBBox("DownFrame", dx,dy,dz);
   // Volume: logicDownFrame
   TGeoVolume*
   plogicDownFrame_82ab928 = new TGeoVolume("logicDownFrame",pDownFrame_5, pMed21);
   plogicDownFrame_82ab928->SetVisLeaves(kTRUE);
   pGFILogWorld_82ab420->AddNode(plogicDownFrame_82ab928, 0, pMatrix10);
   // Shape: RightFrame type: TGeoBBox
   dx = 2.000000;
   dy = 25.000000;
   dz = 0.050000;
   TGeoShape *pRightFrame_6 = new TGeoBBox("RightFrame", dx,dy,dz);
   // Volume: logicRightFrame
   TGeoVolume*
   plogicRightFrame_82abac8 = new TGeoVolume("logicRightFrame",pRightFrame_6, pMed21);
   plogicRightFrame_82abac8->SetVisLeaves(kTRUE);
   pGFILogWorld_82ab420->AddNode(plogicRightFrame_82abac8, 0, pMatrix12);
   // Shape: LeftFrame type: TGeoBBox
   dx = 2.000000;
   dy = 25.000000;
   dz = 0.050000;
   TGeoShape *pLeftFrame_7 = new TGeoBBox("LeftFrame", dx,dy,dz);
   // Volume: logicLeftFrame
   TGeoVolume*
   plogicLeftFrame_82abc68 = new TGeoVolume("logicLeftFrame",pLeftFrame_7, pMed21);
   plogicLeftFrame_82abc68->SetVisLeaves(kTRUE);
   pGFILogWorld_82ab420->AddNode(plogicLeftFrame_82abc68, 0, pMatrix14);

  // Add the sensitive part
   AddSensitiveVolume(pGFILog_82ab5c8);
   fNbOfSensitiveVol+=1;
}



/*
void R3BGfi::ConstructGeometry() {
  
  FairGeoLoader*    geoLoad = FairGeoLoader::Instance();
  FairGeoInterface* geoFace = geoLoad->getGeoInterface();
  R3BGeoGfi*       stsGeo  = new R3BGeoGfi();
  stsGeo->setGeomFile(GetGeometryFileName());
  geoFace->addGeoModule(stsGeo);

  Bool_t rc = geoFace->readSet(stsGeo);
  if (rc) stsGeo->create(geoLoad->getGeoBuilder());
  TList* volList = stsGeo->getListOfVolumes();
  // store geo parameter
  FairRun *fRun = FairRun::Instance();
  FairRuntimeDb *rtdb= FairRun::Instance()->GetRuntimeDb();
  R3BGeoGfiPar* par=(R3BGeoGfiPar*)(rtdb->getContainer("R3BGeoGfiPar"));
  TObjArray *fSensNodes = par->GetGeoSensitiveNodes();
  TObjArray *fPassNodes = par->GetGeoPassiveNodes();

  TListIter iter(volList);
  FairGeoNode* node   = NULL;
  FairGeoVolume *aVol=NULL;

  while( (node = (FairGeoNode*)iter.Next()) ) {
      aVol = dynamic_cast<FairGeoVolume*> ( node );
       if ( node->isSensitive()  ) {
           fSensNodes->AddLast( aVol );
       }else{
           fPassNodes->AddLast( aVol );
       }
  }
  par->setChanged();
  par->setInputVersion(fRun->GetRunId(),1);
  ProcessNodes( volList );

}
*/

ClassImp(R3BGfi)