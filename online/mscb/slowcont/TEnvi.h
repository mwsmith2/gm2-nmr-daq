#ifndef TEnvi_h__
#define TEnvi_h__

#include <TObject.h>

/** This class holds all of the data created by analysis modules.  */

class TEnvi : public TObject{
  public:

  TEnvi();
  virtual ~TEnvi();

  Int_t tStamp;
  Float_t p1val;
  Float_t p2val;
  Float_t p3val;
  Float_t p4val;
  Float_t p5val;
  Float_t p6val;
  Float_t p7val;
  Float_t p8val;

  Float_t c1val;
  Float_t c2val;
  Float_t c3val;
  Float_t c4val;

  ClassDef(TEnvi, 1)
};
#endif
