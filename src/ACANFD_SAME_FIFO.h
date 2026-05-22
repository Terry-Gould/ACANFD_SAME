//----------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------

#include <ACANFD_SAME_CANFDMessage.h>

//----------------------------------------------------------------------------------------

class ACANFD_SAME_FIFO final {

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Default constructor
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: ACANFD_SAME_FIFO (void) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Destructor
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: ~ ACANFD_SAME_FIFO (void) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Private properties
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  private: CANFDMessage * mBuffer ;
  private: uint16_t mCapacity ;
  private: uint16_t mReadIndex ;
  private: uint16_t mCount ;
  private: uint16_t mPeakCount ; // > mCapacity if overflow did occur

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline uint16_t capacity (void) const { return mCapacity ; }
  public: inline uint16_t count (void) const { return mCount ; }
  public: inline bool isEmpty (void) const { return mCount == 0 ; }
  public: inline bool isFull (void) const { return mCount == mCapacity ; }
  public: inline uint16_t peakCount (void) const { return mPeakCount ; }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // initWithCapacity
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: void initWithCapacity (const uint16_t inCapacity) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // append
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: bool append (const CANFDMessage & inMessage) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Remove
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: bool remove (CANFDMessage & outMessage) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // removeAll
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: void removeAll (void) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // removeAllKeepingCapacity
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: void removeAllKeepingCapacity (void) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Reset Peak Count
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline void resetPeakCount (void) { mPeakCount = mCount ; }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // No copy
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: ACANFD_SAME_FIFO (const ACANFD_SAME_FIFO &) = delete ;
  public: ACANFD_SAME_FIFO & operator = (const ACANFD_SAME_FIFO &) = delete ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

} ;

//----------------------------------------------------------------------------------------
