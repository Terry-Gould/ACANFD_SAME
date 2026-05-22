//----------------------------------------------------------------------------------------

#include <ACANFD_SAME_FIFO.h>

//----------------------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------------------

ACANFD_SAME_FIFO::ACANFD_SAME_FIFO (void) :
mBuffer (nullptr),
mCapacity (0),
mReadIndex (0),
mCount (0),
mPeakCount (0) {
}

//----------------------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------------------

ACANFD_SAME_FIFO:: ~ ACANFD_SAME_FIFO (void) {
  delete [] mBuffer ;
}

//----------------------------------------------------------------------------------------
// initWithCapacity
//----------------------------------------------------------------------------------------

void ACANFD_SAME_FIFO::initWithCapacity (const uint16_t inCapacity) {
  delete [] mBuffer ;
  mBuffer = new CANFDMessage [inCapacity] ;
  mCapacity = inCapacity ;
  mReadIndex = 0 ;
  mCount = 0 ;
  mPeakCount = 0 ;
}

//----------------------------------------------------------------------------------------
// append
//----------------------------------------------------------------------------------------

bool ACANFD_SAME_FIFO::append (const CANFDMessage & inMessage) {
  const bool ok = mCount < mCapacity ;
  if (ok) {
    uint16_t writeIndex = mReadIndex + mCount ;
    if (writeIndex >= mCapacity) {
      writeIndex -= mCapacity ;
    }
    mBuffer [writeIndex] = inMessage ;
    mCount += 1 ;
    if (mPeakCount < mCount) {
      mPeakCount = mCount ;
    }
  }
  return ok ;
}

//----------------------------------------------------------------------------------------
// Remove
//----------------------------------------------------------------------------------------

bool ACANFD_SAME_FIFO::remove (CANFDMessage & outMessage) {
  const bool ok = mCount > 0 ;
  if (ok) {
    outMessage = mBuffer [mReadIndex] ;
    mCount -= 1 ;
    mReadIndex += 1 ;
    if (mReadIndex == mCapacity) {
      mReadIndex = 0 ;
    }
  }
  return ok ;
}

//----------------------------------------------------------------------------------------
// removeAll
//----------------------------------------------------------------------------------------

void ACANFD_SAME_FIFO::removeAll (void) {
  delete [] mBuffer ; mBuffer = nullptr ;
  mCapacity = 0 ;
  mReadIndex = 0 ;
  mCount = 0 ;
  mPeakCount = 0 ;
}

//----------------------------------------------------------------------------------------
// removeAllKeepingCapacity
//----------------------------------------------------------------------------------------

void ACANFD_SAME_FIFO::removeAllKeepingCapacity (void) {
  mReadIndex = 0 ;
  mCount = 0 ;
  mPeakCount = 0 ;
}

//----------------------------------------------------------------------------------------
