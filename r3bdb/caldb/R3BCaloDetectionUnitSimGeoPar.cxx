#include "R3BCaloDetectionUnitSimGeoPar.h"

#include "FairDbLogFormat.h"
#include "FairDbLogService.h"
#include "FairDbOutTableBuffer.h"         // for FairDbOutRowStream
#include "FairDbStatement.h"            // for FairDbStatement
#include "FairDbStreamer.h"            // for FairDbStatement

#include "FairParamList.h"              // for FairParamList

#include "Riosfwd.h"                    // for ostream
#include "TString.h"                    // for TString

#include <stdlib.h>                     // for exit
#include <memory>                       // for auto_ptr, etc
#include <vector>                       // for vector, vector<>::iterator


using namespace std;

ClassImp(R3BCaloDetectionUnitSimGeoPar);

#include "FairDbReader.tpl"
template class  FairDbReader<R3BCaloDetectionUnitSimGeoPar>;

#include "FairDbWriter.tpl"
template class  FairDbWriter<R3BCaloDetectionUnitSimGeoPar>;



R3BCaloDetectionUnitSimGeoPar::R3BCaloDetectionUnitSimGeoPar(const char* name, const char* title, const char* context, Bool_t own)
  : FairDbObjTableMap(name,title,context, own),
	fCompId(0),
	fCrystalTypeId(0),
    fDZ(0.0),
    fVertices(new TObjArray(NVERTICES)),   
	fDbEntry(0),  // Set the default Db Entry to the first slot
	fParam_Writer(NULL), //  Writer Meta-Class for SQL IO
	fParam_Reader(NULL), //  Reader Meta-Class for SQL IO
    fMultConn(FairDbTableInterfaceStore::Instance().fConnectionPool)
{

}


R3BCaloDetectionUnitSimGeoPar::~R3BCaloDetectionUnitSimGeoPar()
{
   
  if (fVertices) {
         fVertices->Delete();
         delete fVertices;
         fVertices=NULL;
  }
   
  if (fParam_Writer) {
    delete fParam_Writer;
    fParam_Writer=NULL;
  }

  if (fParam_Reader) {
    delete fParam_Reader;
    fParam_Reader=NULL;
  }

}


void R3BCaloDetectionUnitSimGeoPar::putParams(FairParamList* list)
{
  std::cout<<"-I- R3BCaloDetectionUnitSimGeoPar::putParams() called"<<std::endl;
  if(!list) { return; }
  list->add("comp_id",  fCompId);
  list->add("crystal_id", fCrystalTypeId);
  list->add("half_z", fDZ);
  list->addObject("vertices",  fVertices);
}

Bool_t R3BCaloDetectionUnitSimGeoPar::getParams(FairParamList* list)
{
  if (!list) { return kFALSE; }
  if (!list->fill("comp_id", &fCompId)) { return kFALSE; }
  if (!list->fill("crystal_id", &fCrystalTypeId)) { return kFALSE; }
  if (!list->fill("half_z", &fDZ)) { return kFALSE; }
  if (!list->fillObject("vertices", fVertices)) { return kFALSE; }
  return kTRUE;
}

void R3BCaloDetectionUnitSimGeoPar::clear()
{
  fCompId=fCrystalTypeId=0;
  fDZ=0.;
  // <DB> Not so much overhead here.
  fVertices->Delete();
 
  if (fParam_Writer) { fParam_Writer->Reset(); }
  if (fParam_Reader) { fParam_Reader->Reset(); }

}


string R3BCaloDetectionUnitSimGeoPar::GetTableDefinition(const char* Name)
{

  string sql("create table ");
  if ( Name ) { sql += Name; }
  else { sql += "R3BCALODETECTIONUNITSIMGEOPAR"; }
  sql += "( SEQNO                 INT NOT NULL,";
  sql += "  ROW_ID                INT NOT NULL,";
  sql += "  COMP_ID               INT,";
  sql += "  CRYSTAL_ID            INT,";
  sql += "  DZ                    DOUBLE,";
  sql += "  VERTICES              TEXT,";
  sql += "  primary key(SEQNO,ROW_ID))";
  return sql;
}



void R3BCaloDetectionUnitSimGeoPar::Fill(FairDbResultPool& res_in,
                        const FairDbValRecord* valrec)
{
  // Clear all structures
  clear();
  InitArray();
  FairDbStreamer b_v(fVertices);
  res_in >> fCompId  >> fCrystalTypeId   >> fDZ  >> b_v;
  b_v.Fill(fVertices); 
}

void R3BCaloDetectionUnitSimGeoPar::Store(FairDbOutTableBuffer& res_out,
                         const FairDbValRecord* valrec) const
{
  FairDbStreamer b_v(fVertices);
  res_out << fCompId  << fCrystalTypeId   << fDZ << b_v;
}


void R3BCaloDetectionUnitSimGeoPar::fill(UInt_t rid)
{
  // Get Reader Meta Class
  fParam_Reader=GetParamReader();

  // Define a Context
  ValTimeStamp ts(rid);
  ValCondition context(FairDbDetector::kCal,DataType::kData,ts);

  // Activate reading for this Context
  fParam_Reader->Activate(context, GetVersion());

  // By default use the latest row entry
  // (Other rows would correspond to outdated versions)
  Int_t numRows = fParam_Reader->GetNumRows();
  if ( numRows > 1 ) { numRows = 1; }

  for (int i = 0; i < numRows; ++i) {
    R3BCaloDetectionUnitSimGeoPar* cgd = (R3BCaloDetectionUnitSimGeoPar*) fParam_Reader->GetRow(i);
    if (!cgd) { continue; }
    fCompId = cgd->GetCompId();
    fCrystalTypeId =  cgd->GetCrystalType();
    fDZ  = cgd->GetDz();
    fVertices =  cgd->GetVertices();
  }

}


void R3BCaloDetectionUnitSimGeoPar::store(UInt_t rid)
{

  // Boolean IO test variable
  Bool_t fail= kFALSE;

  // Create a unique statement on choosen DB entry
  auto_ptr<FairDbStatement> stmtDbn(fMultConn->CreateStatement(GetDbEntry()));
  if ( ! stmtDbn.get() ) {
    cout << "-E-  R3BCaloDetectionUnitSimGeoPar::Store()  Cannot create statement for Database_id: " << GetDbEntry()
         << "\n    Please check the FAIRDB_TSQL_* environment.  Quitting ... " << endl;
    exit(1);
  }

  // Check if for this DB entry the table already exists.
  // If not call the corresponding Table Definition Function
  std::vector<std::string> sql_cmds;
  TString atr(GetName());
  atr.ToUpper();

  if (! fMultConn->GetConnection(GetDbEntry())->TableExists("R3BCALODETECTIONUNITSIMGEOPAR") ) {
    sql_cmds.push_back(FairDb::GetValDefinition("R3BCALODETECTIONUNITSIMGEOPAR").Data());
    sql_cmds.push_back(R3BCaloDetectionUnitSimGeoPar::GetTableDefinition());
  }

  // Packed SQL commands executed internally via SQL processor
  std::vector<std::string>::iterator itr(sql_cmds.begin()), itrEnd(sql_cmds.end());
  while( itr != itrEnd ) {
    std::string& sql_cmd(*itr++);
    stmtDbn->Commit(sql_cmd.c_str());
    if ( stmtDbn->PrintExceptions() ) {
      fail = true;
      cout << "-E- R3BCaloDetectionUnitSimGeoPar::Store() ******* Error Executing SQL commands ***********  " << endl;
    }

  }

  // Refresh list of tables in connected database
  // for the choosen DB entry
  fMultConn->GetConnection(GetDbEntry())->SetTableExists();

  // Writer Meta-Class Instance
  fParam_Writer = GetParamWriter();


  // Activate Writer Meta-Class with the proper
  // Validity Time Interval (run_id encapsulated)

  // Writer Activate() arguments list
  //                      <Arguments>                   <Type>                  <Comments>
  //
  //                      Validity Interval,            ValInterval
  //                      Composition      ,            Int_t                   set via cont. factory
  //                      Version          ,            Int_t                   set via cont. factory
  //                      DbEntry          ,            Int_t                   set via cont. factory
  //                      LogTitle         ,            std::string             set via cont. factory

  fParam_Writer->Activate(GetValInterval(rid),GetComboNo(), GetVersion(),GetDbEntry(),"Califa Crystal Shapes");

  // Object to Table mapping
  *fParam_Writer<< (*this);

  // Check for eventual IO problems
  if ( !fParam_Writer->Close() ) {
    fail = true;
    cout << "-E- R3BCaloDetectionUnitSimGeoPar::Store() ******** Cannot do IO on class: " << GetName() <<  endl;
  }

}

void R3BCaloDetectionUnitSimGeoPar::Print()
{
  std::cout<<"   R3BCaloDetectionUnitSimGeoPar  TRAP Parameters: "<<std::endl;
  std::cout<<"   fCompId: "<<  fCompId <<  std::endl;
  std::cout<<"   fCrystalType: "<<  fCrystalTypeId <<  std::endl;
  std::cout<<"   fDz: "<<  fDZ <<  std::endl;
  std::cout<<"   Vertices: " <<  std::endl;
  for (Int_t i=0;i<NVERTICES;i++) {
	TVector3* vtx = (TVector3*) fVertices->At(i);
    if (vtx) vtx->Print();
  }
}

Bool_t R3BCaloDetectionUnitSimGeoPar::SetShape(TGeoArb8*  trap ){

  // Reset Structures
  clear();
  // Read new one
  Double_t vtx[3*NVERTICES];
  trap->SetPoints(vtx);

  fDZ = trap->GetDz();
 
  for (Int_t i=0;i<NVERTICES;i++){
	fVertices->AddAt(new TVector3(vtx[3*i], vtx[3*i+1], vtx[3*i+2]),i);
  }

  return kTRUE;
}

Bool_t R3BCaloDetectionUnitSimGeoPar::Compare(const R3BCaloDetectionUnitSimGeoPar& that ) const {
  //  
  Bool_t test_h =  (fCompId   == that.fCompId)
 	               &&  (fCrystalTypeId   == that.fCrystalTypeId)
 	               &&  (fDZ   == that.fDZ);
  
 
  Bool_t test_d=kTRUE;
  for(Int_t i=0; i<NVERTICES;i++){
    TVector3* a = (TVector3*) fVertices->At(i);
    TVector3* b = (TVector3*) that.fVertices->At(i);
    if ( !a || !b ){
	  cout << "-E- R3BCaloDetectionUnitSimGeoPar::Compare() no vertices ! " << endl;
	  test_d = kFALSE;
      break;
    }      
	if ( (*a) != (*b) ) test_d = kFALSE;
  }
  
  return (test_h && test_d); 
}

void R3BCaloDetectionUnitSimGeoPar::InitArray(){
  // Remap TObjArray
  if (!fVertices){ 
	fVertices = new TObjArray(NVERTICES);
	for(Int_t i=0;i<NVERTICES;i++) fVertices->Add(new TVector3());
  }else  {
    fVertices->Delete();
	for(Int_t i=0;i<NVERTICES;i++) fVertices->Add(new TVector3());
  }
 
}
