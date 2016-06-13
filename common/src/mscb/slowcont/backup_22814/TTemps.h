#ifndef TTemps_h__
#define TTemps_h__

#include <TObject.h>

/** This class holds all of the data created by analysis modules.  */

class TTemps : public TObject{
  public:

  TTemps();
  virtual ~TTemps();

  Int_t tStamp;
  Float_t p1val;
  Float_t p2val;
  Float_t p3val;
  Float_t p4val;

  ClassDef(TTemps, 1)
};
#endif
