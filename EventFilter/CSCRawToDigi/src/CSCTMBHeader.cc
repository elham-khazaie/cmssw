#include "EventFilter/CSCRawToDigi/interface/CSCTMBHeader.h"
#include "EventFilter/CSCRawToDigi/interface/CSCDMBHeader.h"
#include "EventFilter/CSCRawToDigi/src/cscPackerCompare.h"
#include "EventFilter/CSCRawToDigi/interface/CSCTMBHeader2006.h"
#include "EventFilter/CSCRawToDigi/interface/CSCTMBHeader2007.h"
#include "EventFilter/CSCRawToDigi/interface/CSCTMBHeader2007_rev0x50c3.h"
#include "DataFormats/CSCDigi/interface/CSCCLCTDigi.h"
#include "DataFormats/CSCDigi/interface/CSCCorrelatedLCTDigi.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include <math.h>
#include <string.h> // memcpy

bool CSCTMBHeader::debug = false;
short unsigned int CSCTMBHeader::firmwareVersion=2006;

CSCTMBHeader::CSCTMBHeader():
  theHeaderFormat(new CSCTMBHeader2006())
{
  firmwareVersion = 2006;
}

//CSCTMBHeader::CSCTMBHeader(const CSCTMBStatusDigi & digi) {
//  CSCTMBHeader(digi.header());
//}

CSCTMBHeader::CSCTMBHeader(const unsigned short * buf)
: theHeaderFormat()
{
  ///first determine the format
  if (buf[0]==0xDB0C) {
    firmwareVersion=2007;
    theHeaderFormat = boost::shared_ptr<CSCVTMBHeaderFormat>(new CSCTMBHeader2007(buf));
    if(theHeaderFormat->firmwareRevision() > 0x50c3)
    {   
      theHeaderFormat = boost::shared_ptr<CSCVTMBHeaderFormat>(new CSCTMBHeader2007_rev0x50c3(buf));
    }
  }
  else if (buf[0]==0x6B0C) {
    firmwareVersion=2006;
    theHeaderFormat = boost::shared_ptr<CSCVTMBHeaderFormat>(new CSCTMBHeader2006(buf));

  }
  else {
    edm::LogError("CSCTMBHeader|CSCRawToDigi") <<"failed to determine TMB firmware version!!";
  }
}    
/*
void CSCTMBHeader::swapCLCTs(CSCCLCTDigi& digi1, CSCCLCTDigi& digi2)
{
  bool me11 = (theChamberId.station() == 1 && 
	       (theChamberId.ring() == 1 || theChamberId.ring() == 4));
  if (!me11) return;

  int cfeb1 = digi1.getCFEB();
  int cfeb2 = digi2.getCFEB();
  if (cfeb1 != cfeb2) return;

  bool me1a = (cfeb1 == 4);
  bool me1b = (cfeb1 != 4);
  bool zplus = (theChamberId.endcap() == 1);

  if ( (me1a && zplus) || (me1b && !zplus)) {
    // Swap CLCTs if they have the same quality and pattern # (priority
    // has to be given to the lower key).
    if (digi1.getQuality() == digi2.getQuality() &&
	digi1.getPattern() == digi2.getPattern()) {
      CSCCLCTDigi temp = digi1;
      digi1 = digi2;
      digi2 = temp;

      // Also re-number them.
      digi1.setTrknmb(1);
      digi2.setTrknmb(2);
    }
  }
}
*/


//FIXME Pick which LCT goes first
void CSCTMBHeader::add(const CSCCLCTDigi & digi)
{
  int nLCT = CLCTDigis(0).size();
  if(nLCT == 0) addCLCT0(digi);
  else if(nLCT == 1) addCLCT1(digi);
}

void CSCTMBHeader::add(const CSCCorrelatedLCTDigi & digi)
{
  int nLCT = CorrelatedLCTDigis(0).size();
  if(nLCT == 0) addCorrelatedLCT0(digi);
  else if(nLCT == 1) addCorrelatedLCT1(digi);
}


CSCTMBHeader2007 CSCTMBHeader::tmbHeader2007()   const {
  CSCTMBHeader2007 * result = dynamic_cast<CSCTMBHeader2007 *>(theHeaderFormat.get());
  if(result == 0) 
  {
    throw cms::Exception("Could not get 2007 TMB header format");
  }
  return *result;
}


CSCTMBHeader2006 CSCTMBHeader::tmbHeader2006()   const {
  CSCTMBHeader2006 * result = dynamic_cast<CSCTMBHeader2006 *>(theHeaderFormat.get());
  if(result == 0)
  {
    throw cms::Exception("Could not get 2006 TMB header format");
  }
  return *result;
}


void CSCTMBHeader::selfTest()
{
  static bool debug = false;
  const int testversion = 2006; // version to be tested; default is 2006.

  // tests packing and unpacking
  for(int station = 1; station <= 4; ++station) {
    for(int iendcap = 1; iendcap <= 2; ++iendcap) {
      CSCDetId detId(iendcap, station, 1, 1, 0);

      // the next-to-last is the BX, which only gets
      // saved in two bits... I guess the bxnPreTrigger is involved?
      //CSCCLCTDigi clct0(1, 1, 4, 0, 0, 30, 3, 0, 1); // valid for 2006
      // In 2007 firmware, there are no distrips, so the 4th argument (strip
      // type) should always be set to 1 (halfstrips).
      CSCCLCTDigi clct0(1, 1, 4, 1, 0, 30, 3, 0, 1); // valid for 2007
      CSCCLCTDigi clct1(1, 1, 2, 1, 1, 31, 1, 2, 2);

      // BX of LCT (8th argument) is 1-bit word (the least-significant bit
      // of ALCT's bx).
      CSCCorrelatedLCTDigi lct0(1, 1, 2, 10, 98, 5, 0, 1, 0, 0, 0, 0);
      CSCCorrelatedLCTDigi lct1(2, 1, 2, 20, 15, 9, 1, 0, 0, 0, 0, 0);

      CSCTMBHeader tmbHeader;
      tmbHeader.firmwareVersion = testversion;
      tmbHeader.addCLCT0(clct0);
      tmbHeader.addCLCT1(clct1);
      tmbHeader.addCorrelatedLCT0(lct0);
      tmbHeader.addCorrelatedLCT1(lct1);
      std::vector<CSCCLCTDigi> clcts = tmbHeader.CLCTDigis(detId.rawId());
      assert(cscPackerCompare(clcts[0],clct0));
      assert(cscPackerCompare(clcts[1],clct1));
std::cout << "TESTED " << station << " " << iendcap << std::endl;
      if (debug) {
	std::cout << "Match for: " << clct0 << "\n";
	std::cout << "           " << clct1 << "\n \n";
      }

      std::vector<CSCCorrelatedLCTDigi> lcts = tmbHeader.CorrelatedLCTDigis(detId.rawId());
      assert(cscPackerCompare(lcts[0], lct0));
      assert(cscPackerCompare(lcts[1], lct1));
      if (debug) {
	std::cout << "Match for: " << lct0 << "\n";
	std::cout << "           " << lct1 << "\n";
      }

      // try packing and re-packing, to make sure they're the same
      unsigned short int * data = tmbHeader.data();
      CSCTMBHeader newHeader(data);
      clcts = newHeader.CLCTDigis(detId.rawId());
      assert(cscPackerCompare(clcts[0],clct0));
      assert(cscPackerCompare(clcts[1],clct1));
      lcts = newHeader.CorrelatedLCTDigis(detId.rawId());
      assert(cscPackerCompare(lcts[0], lct0));
      assert(cscPackerCompare(lcts[1], lct1));


    }
  }
}


std::ostream & operator<<(std::ostream & os, const CSCTMBHeader & hdr) {
  hdr.theHeaderFormat->print(os);
  return os;
}
