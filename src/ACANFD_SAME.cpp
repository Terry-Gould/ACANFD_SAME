//----------------------------------------------------------------------------------------
// Page numbers refer to DS60001507G data sheet
//----------------------------------------------------------------------------------------

#include <ACANFD_SAME-from-cpp.h>
#include <ACANFD_SAME_PinMux.h>

//----------------------------------------------------------------------------------------
//    Constructor
//----------------------------------------------------------------------------------------

ACANFD_SAME::ACANFD_SAME (const ACANFD_SAME_Module inModule,
                                          uint32_t * inMessageRAMPtr,
                                          const uint32_t inMessageRamWordSize) :
mModulePtr ((inModule == ACANFD_SAME_Module::can0) ? CAN0 : CAN1),
mMessageRAMPtr (inMessageRAMPtr),
mMessageRamWordSize (inMessageRamWordSize),
mModule (inModule),
mCopyOfMessagesInHardwareTransmitFIFO (),
mTransmitAckQueue () {
}

//----------------------------------------------------------------------------------------
//    Optional CAN TX/RX pin override
//----------------------------------------------------------------------------------------

void ACANFD_SAME::setPins (const int inTxArduinoPin, const int inRxArduinoPin) {
  mUseExplicitPins = true ;
  mExplicitTxArduinoPin = inTxArduinoPin ;
  mExplicitRxArduinoPin = inRxArduinoPin ;
}

//----------------------------------------------------------------------------------------

void ACANFD_SAME::clearPins (void) {
  mUseExplicitPins = false ;
  mExplicitTxArduinoPin = -1 ;
  mExplicitRxArduinoPin = -1 ;
}

//----------------------------------------------------------------------------------------
//    beginFD method
//----------------------------------------------------------------------------------------

uint32_t ACANFD_SAME::beginFD (const ACANFD_SAME_Settings & inSettings,
                                       const ExtendedFilters & inExtendedFilters) {
  return beginFD (inSettings, ACANFD_SAME::StandardFilters (), inExtendedFilters) ;
}

//----------------------------------------------------------------------------------------

uint32_t ACANFD_SAME::beginFD (const ACANFD_SAME_Settings & inSettings,
                                       const StandardFilters & inStandardFilters,
                                       const ExtendedFilters & inExtendedFilters) {
  uint32_t errorCode = inSettings.CANFDBitSettingConsistency () ;
//------------------------------------------------------ Check settings
  if (inSettings.mHardwareRxFIFO0Size > 64) {
    errorCode |= kHardwareRxFIFO0SizeGreaterThan64 ;
  }
  if (inSettings.mHardwareRxFIFO1Size > 64) {
    errorCode |= kHardwareRxFIFO1SizeGreaterThan64 ;
  }
  if (inSettings.mHardwareTransmitTxFIFOSize > 32) {
    errorCode |= kHardwareTransmitFIFOSizeGreaterThan32 ;
  }
  if (inSettings.mHardwareTransmitTxFIFOSize < 2) {
    errorCode |= kHardwareTransmitFIFOSizeLowerThan2 ;
  }
  if (inSettings.mHardwareDedicacedTxBufferCount > 30) {
    errorCode |= kDedicacedTransmitTxBufferCountGreaterThan30 ;
  }
  if ((inSettings.mHardwareTransmitTxFIFOSize + inSettings.mHardwareDedicacedTxBufferCount) > 32) {
    errorCode |= kTxBufferCountGreaterThan32 ;
  }
  if (inStandardFilters.count () > 128) {
    errorCode |= kStandardFilterCountGreaterThan128 ;
  }
  if (inExtendedFilters.count () > 128) {
    errorCode |= kExtendedFilterCountGreaterThan128 ;
  }
//------------------------------------------------------ Enable CAN Clock (48 MHz)
  switch (mModule) {
  case ACANFD_SAME_Module::can0 :
    GCLK->PCHCTRL [CAN0_GCLK_ID].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK1 ;
    MCLK->AHBMASK.reg |= MCLK_AHBMASK_CAN0 ;
    break ;
  case ACANFD_SAME_Module::can1 :
    GCLK->PCHCTRL [CAN1_GCLK_ID].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK1 ;
    MCLK->AHBMASK.reg |= MCLK_AHBMASK_CAN1 ;
    break ;
  }
//------------------------------------------------------ Start configuring CAN module
  mModulePtr->CCCR.reg = CAN_CCCR_INIT ; // Page 1123
  while ((mModulePtr->CCCR.reg & CAN_CCCR_INIT) == 0) {
  }
//------------------------------------------------------ Enable configuration change
  mModulePtr->CCCR.reg = CAN_CCCR_INIT | CAN_CCCR_CCE ;
  mModulePtr->CCCR.reg = CAN_CCCR_INIT | CAN_CCCR_CCE | CAN_CCCR_TEST ;
  uint32_t cccr  = CAN_CCCR_BRSE | CAN_CCCR_FDOE ;
//------------------------------------------------------ Select mode
  mModulePtr->TEST.reg = 0 ;
  switch (inSettings.mModuleMode) {
  case ACANFD_SAME_Settings::NORMAL_FD :
    break ;
  case ACANFD_SAME_Settings::INTERNAL_LOOP_BACK :
    mModulePtr->TEST.reg = CAN_TEST_LBCK ;
    cccr |= CAN_CCCR_MON | CAN_CCCR_TEST ;
    break ;
  case ACANFD_SAME_Settings::EXTERNAL_LOOP_BACK :
    mModulePtr->TEST.reg = CAN_TEST_LBCK ;
    cccr |= CAN_CCCR_TEST ;
    break ;
  case ACANFD_SAME_Settings::BUS_MONITORING :
    cccr |= CAN_CCCR_MON ;
    break ;
  }
  if (!inSettings.mEnableRetransmission) {
    cccr |= CAN_CCCR_DAR ; // Page 1123
  }
//------------------------------------------------------ Set nominal Bit Timing and Prescaler (page 1125)
  mModulePtr->NBTP.reg =
    (uint32_t (inSettings.mArbitrationSJW - 1) << 25)
  |
    (uint32_t (inSettings.mBitRatePrescaler - 1) << 16)
  |
    (uint32_t (inSettings.mArbitrationPhaseSegment1 - 1) << 8)
  |
    (uint32_t (inSettings.mArbitrationPhaseSegment2 - 1) << 0)
  ;
//     Serial.print ("NBTP 0x") ;
//     Serial.println (mModulePtr->NBTP.reg, HEX) ;
//------------------------------------------------------ Set data Bit Timing and Prescaler (page 1119)
  mModulePtr->DBTP.reg =
    CAN_DBTP_TDC // Enable Transceiver Delay Compensation ?
  |
    (uint32_t (inSettings.mBitRatePrescaler - 1) << 16)
  |
    (uint32_t (inSettings.mDataPhaseSegment1 - 1) << 8)
  |
    (uint32_t (inSettings.mDataPhaseSegment2 - 1) << 4)
  |
    (uint32_t (inSettings.mDataSJW - 1) << 0)
  ;
//------------------------------------------------------ Transmitter Delay Compensation
  mModulePtr->TDCR.reg = uint32_t (inSettings.mTransceiverDelayCompensation) << 8 ; // Page 1134
//------------------------------------------------------ Global Filter Configuration (page 1148)
  mModulePtr->GFC.reg =
    (uint32_t (inSettings.mNonMatchingStandardFrameReception) << 4)
  |
    (uint32_t (inSettings.mNonMatchingExtendedFrameReception) << 2)
  |
    (uint32_t (inSettings.mDiscardReceivedStandardRemoteFrames) << 1)
  |
    (uint32_t (inSettings.mDiscardReceivedExtendedRemoteFrames) << 0)
  ;
//------------------------------------------------------ Configure message RAM
//    mModulePtr->MRCFG.reg = 3 ; // Page 1118
  uint32_t * ptr = mMessageRAMPtr ;
//--- Allocate Standard ID Filters (0 ... 128 elements -> 0 ... 128 words)
   mModulePtr->SIDFC.reg =
    (uint32_t (ptr) & 0xFFFFU) // Standard ID Filter Configuration, page 1269
  |
    (inStandardFilters.count () << 16) // Standard filter count
  ;
  mStandardFilterCallBackArray.setCapacity (inStandardFilters.count ()) ;
  for (uint32_t i=0 ; i<inStandardFilters.count () ; i++) {
    *ptr = inStandardFilters.filterAtIndex (i) ; // Page 1149
    ptr += 1 ;
    mStandardFilterCallBackArray.append (inStandardFilters.callBackAtIndex (i)) ;
  }
//--- Allocate Extended ID Filters (0 ... 64 elements -> 0 ... 128 words)
  mModulePtr->XIDFC.reg =
    (uint32_t (ptr) & 0xFFFFU) // Standard ID Filter Configuration, page 1150
  |
    (inExtendedFilters.count () << 16) // Standard filter count
  ;
  mExtendedFilterCallBackArray.setCapacity (inExtendedFilters.count ()) ;
  for (uint32_t i=0 ; i<inExtendedFilters.count () ; i++) {
    *ptr = inExtendedFilters.firstWordAtIndex (i) ;
    ptr += 1 ;
    *ptr = inExtendedFilters.secondWordAtIndex (i) ;
    ptr += 1 ;
    mExtendedFilterCallBackArray.append (inExtendedFilters.callBackAtIndex (i)) ;
  }
//--- Allocate Rx FIFO 0 (0 ... 64 elements -> 0 ... 1152 words)
  mRxFIFO0Pointer = ptr ;
  mHardwareRxFIFO0Payload = inSettings.mHardwareRxFIFO0Payload ;
  mModulePtr->RXF0C.reg = // Page 1155
    (uint32_t (ptr) & 0xFFFFU) // FOSA
  |
    (uint32_t (inSettings.mHardwareRxFIFO0Size) << 16) // F0S
  ;
  mModulePtr->RXESC.reg |= uint32_t (inSettings.mHardwareRxFIFO0Payload) ; // Rx FIFO 0 element size (page 1162)
  ptr += inSettings.mHardwareRxFIFO0Size * ACANFD_SAME_Settings::wordCountForPayload (mHardwareRxFIFO0Payload) ;
//--- Allocate Rx FIFO 1 (0 ... 64 elements -> 0 ... 1152 words)
  mRxFIFO1Pointer = ptr ;
  mHardwareRxFIFO1Payload = inSettings.mHardwareRxFIFO1Payload ;
  mModulePtr->RXF1C.reg = // Page 1159
    (uint32_t (ptr) & 0xFFFFU) // FOSA
  |
    (uint32_t (inSettings.mHardwareRxFIFO1Size) << 16) // F0S
  ;
  mModulePtr->RXESC.reg |= uint32_t (inSettings.mHardwareRxFIFO1Payload) << 4 ; // Rx FIFO 1 element size (page 1162)
  ptr += inSettings.mHardwareRxFIFO1Size * ACANFD_SAME_Settings::wordCountForPayload (mHardwareRxFIFO1Payload) ;
//--- Allocate Rx Buffers (0 ... 64 elements -> 0 ... 1152 words)
//       EMPTY
//--- Allocate Tx Event / FIFO (0 ... 32 elements -> 0 ... 64 words)
//       EMPTY
//--- Allocate Tx Buffers (0 ... 32 elements -> 0 ... 576 words)
  mHardwareTxBufferPayload = inSettings.mHardwareTransmitBufferPayload ;
  mModulePtr->TXESC.reg = uint32_t (mHardwareTxBufferPayload) ; // page 1166
  mTxBuffersPointer = ptr ;
  mModulePtr->TXBC.reg = // Page 1164
    (uint32_t (ptr) & 0xFFFFU) // Tx Buffer start address
  |
    (inSettings.mHardwareTransmitTxFIFOSize << 24) // Number of Transmit FIFO / Queue buffers
  |
    (inSettings.mHardwareDedicacedTxBufferCount << 16) // Number of Dedicaced Tx buffers
  ;
  mCopyOfMessagesInHardwareTransmitFIFO.initWithCapacity (inSettings.mHardwareTransmitTxFIFOSize) ;
  const uint32_t txBufferCount = inSettings.mHardwareDedicacedTxBufferCount + inSettings.mHardwareTransmitTxFIFOSize ;
  ptr += txBufferCount * ACANFD_SAME_Settings::wordCountForPayload (mHardwareTxBufferPayload) ;
  mEndOfMessageRamPointer = ptr ;
//------------------------------------------------------ Display Message RAM Allocation
  const uint32_t requiredSize = mEndOfMessageRamPointer - mMessageRAMPtr ;
  if (requiredSize > mMessageRamWordSize) {
    errorCode |= kMessageRamTooSmall ;
  }
  if (uint32_t (mEndOfMessageRamPointer) > 0x2000FFFF) {
    errorCode |= kMessageRamTooSmall ;
  }
  if (errorCode == 0) {
  //------------------------------------------------------ Configure Driver buffers
    mDriverTransmitFIFO.initWithCapacity (inSettings.mDriverTransmitFIFOSize) ;
    mDriverReceiveFIFO0.initWithCapacity (inSettings.mDriverReceiveFIFO0Size) ;
    mDriverReceiveFIFO1.initWithCapacity (inSettings.mDriverReceiveFIFO1Size) ;
    mNonMatchingStandardMessageCallBack = inSettings.mNonMatchingStandardMessageCallBack ;
    mNonMatchingExtendedMessageCallBack = inSettings.mNonMatchingExtendedMessageCallBack ;
  //------------------------------------------------------ Interrupts
    uint32_t interruptRegister = CAN_IE_RF0NE ; // Receive FIFO 0 Non Empty
    interruptRegister |= CAN_IE_RF1NE ; // Receive FIFO 1 Non Empty
    interruptRegister |= CAN_IE_TCE ; // Enable Transmission Completed Interrupt: page 1141
    mModulePtr->IE.reg = interruptRegister ;
    mModulePtr->TXBTIE.reg = ~ 0 ;
    mModulePtr->ILS.reg = 0 ; // All interrupt on EINT0
    switch (mModule) {
    case ACANFD_SAME_Module::can0 :
      NVIC_EnableIRQ (CAN0_IRQn) ;
      break ;
    case ACANFD_SAME_Module::can1 :
      NVIC_EnableIRQ (CAN1_IRQn) ;
      break ;
    }
    mModulePtr->ILE.reg = CAN_ILE_EINT0 ; // Enable Interrupt 0
  //------------------------------------------------------ Select TX and RX pins
  //  Default behaviour: try sane BSP names first, then validate other candidate macros
  //  by actual MCU port/pin so Adafruit-style misnamed CAN pins still work.
  //  Optional override: user may call setPins(tx, rx) before beginFD().
    const bool canPinsConfigured = mUseExplicitPins
      ? acanfd_same_configureExplicitCanPins (mModule, mExplicitTxArduinoPin, mExplicitRxArduinoPin)
      : acanfd_same_configureCanPins (mModule) ;
    if (!canPinsConfigured) {
      errorCode |= kInvalidCANPinMux ;
    }
  //------------------------------------------------------ Activate CAN controller
    if (errorCode == 0) {
      mModulePtr->CCCR.reg = CAN_CCCR_INIT | CAN_CCCR_CCE | cccr ; // Page 1123
      mModulePtr->CCCR.reg = CAN_CCCR_INIT | cccr ; // Page 1123, reset CCE bit
      mModulePtr->CCCR.reg = cccr ; // Page 1123, reset INIT bit
      while ((mModulePtr->CCCR.reg & CAN_CCCR_INIT) != 0) {
      }
    }
  }
//--- Return error code (0 --> no error)
  return errorCode ;
}

//----------------------------------------------------------------------------------------

uint32_t ACANFD_SAME::messageRamRequiredMinimumSize (void) {
  return mEndOfMessageRamPointer - mMessageRAMPtr ;
}

//----------------------------------------------------------------------------------------
//   RECEPTION
//----------------------------------------------------------------------------------------

bool ACANFD_SAME::availableFD0 (void) {
  noInterrupts () ;
    const bool hasMessage = !mDriverReceiveFIFO0.isEmpty () ;
  interrupts () ;
  return hasMessage ;
}

//----------------------------------------------------------------------------------------

bool ACANFD_SAME::receiveFD0 (CANFDMessage & outMessage) {
  noInterrupts () ;
    const bool hasMessage = mDriverReceiveFIFO0.remove (outMessage) ;
  interrupts () ;
  return hasMessage ;
}

//----------------------------------------------------------------------------------------

bool ACANFD_SAME::availableFD1 (void) {
  noInterrupts () ;
    const bool hasMessage = !mDriverReceiveFIFO1.isEmpty () ;
  interrupts () ;
  return hasMessage ;
}

//----------------------------------------------------------------------------------------

bool ACANFD_SAME::receiveFD1 (CANFDMessage & outMessage) {
  noInterrupts () ;
    const bool hasMessage = mDriverReceiveFIFO1.remove (outMessage) ;
  interrupts () ;
  return hasMessage ;
}

//----------------------------------------------------------------------------------------

bool ACANFD_SAME::dispatchReceivedMessage (void) {
  CANFDMessage message ;
  bool result = false ;
  if (receiveFD0 (message)) {
    result = true ;
    internalDispatchReceivedMessage (message) ;
  }
  if (receiveFD1 (message)) {
    result = true ;
    internalDispatchReceivedMessage (message) ;
  }
  return result ;
}

//----------------------------------------------------------------------------------------

bool ACANFD_SAME::dispatchReceivedMessageFIFO0 (void) {
  CANFDMessage message ;
  const bool result = receiveFD0 (message) ;
  if (result) {
    internalDispatchReceivedMessage (message) ;
  }
  return result ;
}

//----------------------------------------------------------------------------------------

bool ACANFD_SAME::dispatchReceivedMessageFIFO1 (void) {
  CANFDMessage message ;
  const bool result = receiveFD1 (message) ;
  if (result) {
    internalDispatchReceivedMessage (message) ;
  }
  return result ;
}

//----------------------------------------------------------------------------------------

void ACANFD_SAME::internalDispatchReceivedMessage (const CANFDMessage & inMessage) {
  const uint32_t filterIndex = inMessage.idx ;
  ACANFDCallBackRoutine callBack = nullptr ;
  if (inMessage.ext) {
    if (filterIndex == 255) {
      callBack = mNonMatchingStandardMessageCallBack ;
    }else if (filterIndex < mExtendedFilterCallBackArray.count ()) {
      callBack = mExtendedFilterCallBackArray [filterIndex] ;
    }
  }else{ // Standard message
    if (filterIndex == 255) {
      callBack = mNonMatchingExtendedMessageCallBack ;
    }else if (filterIndex < mStandardFilterCallBackArray.count ()) {
      callBack = mStandardFilterCallBackArray [filterIndex] ;
    }
  }
  if (callBack != nullptr) {
    callBack (inMessage) ;
  }
}

//----------------------------------------------------------------------------------------
//   EMISSION
//----------------------------------------------------------------------------------------

bool ACANFD_SAME::sendBufferNotFullForIndex (const uint32_t inMessageIndex) {
  bool canSend = false ;
  noInterrupts () ;
    if (inMessageIndex == 0) { // Send via Tx FIFO ?
      canSend = !mDriverTransmitFIFO.isFull () ;
    }else{ // Send via dedicaced Tx Buffer ?
      const uint32_t numberOfDedicacedTxBuffers = (mModulePtr->TXBC.reg >> 16) & 0x3F ; // Page 1164
      if (inMessageIndex <= numberOfDedicacedTxBuffers) {
        const uint32_t txBufferIndex = inMessageIndex - 1 ;
        canSend = (mModulePtr->TXBRP.reg & (1U << txBufferIndex)) == 0 ; // Page 1167
      }
    }
  interrupts () ;
  return canSend ;
}

//----------------------------------------------------------------------------------------

uint32_t ACANFD_SAME::tryToSendReturnStatusFD (const CANFDMessage & inMessage) {
  noInterrupts () ;
    uint32_t sendStatus = 0 ;
    if (!inMessage.isValid ()) {
      sendStatus = kInvalidMessage ;
    }else if (inMessage.idx == 0) { // Send via Tx FIFO ?
      const uint32_t txfqs = mModulePtr->TXFQS.reg ; // Page 1165
      const uint32_t hardwareTransmitFifoFreeLevel = txfqs & 0x3F ; // Page 1165
      if ((hardwareTransmitFifoFreeLevel > 0) && mDriverTransmitFIFO.isEmpty ()) {
        const uint32_t putIndex = (txfqs >> 16) & 0x1F ;
        writeTxBuffer (inMessage, putIndex) ;
        if (isTransmitAckEnabled ()) {
          mCopyOfMessagesInHardwareTransmitFIFO.append (inMessage) ;
        }
      }else if (mDriverTransmitFIFO.isFull ()) {
        sendStatus = kTransmitBufferOverflow ;
      }else{
        mDriverTransmitFIFO.append (inMessage) ;
      }
    }else{ // Send via dedicaced Tx Buffer ?
      const uint32_t numberOfDedicacedTxBuffers = (mModulePtr->TXBC.reg >> 16) & 0x3F ; // Page 1164
      if (inMessage.idx <= numberOfDedicacedTxBuffers) {
        const uint32_t txBufferIndex = inMessage.idx - 1 ;
        const bool hardwareTxBufferIsEmpty = (mModulePtr->TXBRP.reg & (1U << txBufferIndex)) == 0 ; // Page 1167
        if (hardwareTxBufferIsEmpty) {
          writeTxBuffer (inMessage, txBufferIndex) ;
        }else{
          sendStatus = kTransmitBufferOverflow ;
        }
      }else{
        sendStatus = kTransmitBufferIndexTooLarge ;
      }
    }
  interrupts () ;
  return sendStatus ;
}

//----------------------------------------------------------------------------------------

void ACANFD_SAME::writeTxBuffer (const CANFDMessage & inMessage,
                                         const uint32_t inTxBufferIndex) {
//--- Compute Tx Buffer address
  uint32_t * txBufferPtr = mTxBuffersPointer ;
  txBufferPtr += inTxBufferIndex * ACANFD_SAME_Settings::wordCountForPayload (mHardwareTxBufferPayload) ;
//--- Identifier and extended bit
  if (inMessage.ext) {
    txBufferPtr [0] = (inMessage.id & 0x1FFFFFFFU) | (1U << 30) ;
  }else{
    txBufferPtr [0] = (inMessage.id & 0x7FFU) << 18 ;
  }
//---Control
  uint32_t lengthCode ;
  if (inMessage.len > 48) {
    lengthCode = 15 ;
  }else if (inMessage.len > 32) {
    lengthCode = 14 ;
  }else if (inMessage.len > 24) {
    lengthCode = 13 ;
  }else if (inMessage.len > 20) {
    lengthCode = 12 ;
  }else if (inMessage.len > 16) {
    lengthCode = 11 ;
  }else if (inMessage.len > 12) {
    lengthCode = 10 ;
  }else if (inMessage.len > 8) {
    lengthCode = 9 ;
  }else{
    lengthCode = inMessage.len ;
  }
  txBufferPtr [1] = uint32_t (lengthCode) << 16 ;
//---
  const uint32_t lg = ACANFD_SAME_Settings::frameDataByteCountForPayload (mHardwareTxBufferPayload) ;
  const uint32_t sentCount = (lg < inMessage.len) ? lg : inMessage.len ;
//---
  switch (inMessage.type) {
  case CANFDMessage::CAN_REMOTE :
    txBufferPtr [0] |= 1 << 29 ; // Set RTR bit
    break ;
  case CANFDMessage::CAN_DATA :
    txBufferPtr [2] = inMessage.data32 [0] ;
    txBufferPtr [3] = inMessage.data32 [1] ;
    break ;
  case CANFDMessage::CANFD_NO_BIT_RATE_SWITCH :
    { txBufferPtr [1] |= 1 << 21 ; // Set FDF bit
      const uint32_t wc = (sentCount + 3) / 4 ;
      for (uint32_t i=0 ; i<wc ; i++) {
        txBufferPtr [i+2] = inMessage.data32 [i] ;
      }
    }
    break ;
  case CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH :
    { txBufferPtr [1] |= (1 << 21) | (1 << 20) ; // Set FDF and BRS bits
      const uint32_t wc = (sentCount + 3) / 4 ;
      for (uint32_t i=0 ; i<wc ; i++) {
        txBufferPtr [i+2] = inMessage.data32 [i] ;
      }
    }
    break ;
  }
//---Request transmit
  mModulePtr->TXBAR.reg = 1U << inTxBufferIndex ; // Page 1168
}

//----------------------------------------------------------------------------------------
//   TRANSMIT MESSAGE ACKS
//----------------------------------------------------------------------------------------

void ACANFD_SAME::setTransmitAckCapacity (const uint32_t inCapacity) {
  noInterrupts () ;
    if (inCapacity == 0) {
      mCopyOfMessagesInHardwareTransmitFIFO.removeAllKeepingCapacity () ;
      mTransmitAckQueue.removeAll () ;
    }else{
      mTransmitAckQueue.initWithCapacity (inCapacity) ;
    }
  interrupts () ;
}

//----------------------------------------------------------------------------------------

bool ACANFD_SAME::hasTransmitAck (void) const {
  noInterrupts () ;
    const bool result = mTransmitAckQueue.count () > 0 ;
  interrupts () ;
  return result ;
}

//----------------------------------------------------------------------------------------

bool ACANFD_SAME::getTransmitAck (CANFDMessage & outMessage) {
  noInterrupts () ;
    const bool result = mTransmitAckQueue.remove (outMessage) ;
  interrupts () ;
  return result ;
}

//----------------------------------------------------------------------------------------
//   INTERRUPT SERVICE ROUTINES
//----------------------------------------------------------------------------------------

static void getMessageFrom (uint32_t * inMessageRamAddress,
                            const ACANFD_SAME_Settings::Payload inPayLoad,
                            CANFDMessage & outMessage) {
  const uint32_t lg = ACANFD_SAME_Settings::frameDataByteCountForPayload (inPayLoad) ;
  const uint32_t w0 = inMessageRamAddress [0] ;
  outMessage.id = w0 & 0x1FFFFFFF ;
  const bool remote = (w0 & (1 << 29)) != 0 ;
  outMessage.ext = (w0 & (1 << 30)) != 0 ;
//   const bool esi = (w0 & (1 << 31)) != 0 ;
  if (!outMessage.ext) {
    outMessage.id >>= 18 ;
  }
  const uint32_t w1 = inMessageRamAddress [1] ;
  const uint32_t dlc = (w1 >> 16) & 0xF ;
  static const uint8_t CANFD_LENGTH_FROM_CODE [16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64} ;
  outMessage.len = CANFD_LENGTH_FROM_CODE [dlc] ;
  const bool fdf = (w1 & (1 << 21)) != 0 ;
  const bool brs = (w1 & (1 << 20)) != 0 ;
  if (fdf) { // CANFD frame
    outMessage.type = brs ? CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH : CANFDMessage::CANFD_NO_BIT_RATE_SWITCH ;
  }else if (remote) {
    outMessage.type = CANFDMessage::CAN_REMOTE ;
  }else{
    outMessage.type = CANFDMessage::CAN_DATA ;
  }
//--- Filter index
  if ((w1 & (1U << 31)) != 0) { // Filter index available ? Page 1177-1178
    outMessage.idx = 255 ; // Not available
  }else{
    const uint32_t filterIndex = (w1 >> 24) & 0x7F ;
    outMessage.idx = uint8_t (filterIndex) ;
  }
//--- Get data
  if (outMessage.type != CANFDMessage::CAN_REMOTE) {
    const uint32_t receiveByteCount = (lg < outMessage.len) ? lg : outMessage.len ;
    const uint32_t wc = (receiveByteCount + 3) / 4 ;
    for (uint32_t i=0 ; i<wc ; i++) {
      outMessage.data32 [i] = inMessageRamAddress [i+2] ;
    }
    for (uint32_t i=receiveByteCount ; i<outMessage.len ; i++) {
      outMessage.data [i] = 0xCC ;
    }
  }
}

//----------------------------------------------------------------------------------------

void ACANFD_SAME::interruptServiceRoutine (void) {
  CANFDMessage message ;
  bool loop = true ;
  while (loop) {
    const uint32_t it = mModulePtr->IR.reg ;
    if ((it & CAN_IR_RF0N) != 0) { // Receive FIFO 0 Non Empty
    //--- Get read index
      const uint32_t readIndex = (mModulePtr->RXF0S.reg >> 8) & 0x3F ;
    //--- Compute message RAM address
      uint32_t * address = mRxFIFO0Pointer ;
      address += readIndex * ACANFD_SAME_Settings::wordCountForPayload (mHardwareRxFIFO0Payload) ;
    //--- Get message
      getMessageFrom (address, mHardwareRxFIFO0Payload, message) ;
    //--- Clear receive flag
      mModulePtr->RXF0A.reg = readIndex ;
    //--- Interrupt Acknowledge
      mModulePtr->IR.reg = CAN_IR_RF0N ;
    //--- Enter message into driver receive buffer 0
      mDriverReceiveFIFO0.append (message) ;
    }else if ((it & CAN_IR_RF1N) != 0) { // Receive FIFO 1 Non Empty
    //--- Get read index
      const uint32_t readIndex = (mModulePtr->RXF1S.reg >> 8) & 0x3F ;
    //--- Compute message RAM address
      uint32_t * address = mRxFIFO1Pointer ;
      address += readIndex * ACANFD_SAME_Settings::wordCountForPayload (mHardwareRxFIFO1Payload) ;
    //--- Get message
      getMessageFrom (address, mHardwareRxFIFO1Payload, message) ;
    //--- Clear receive flag
      mModulePtr->RXF1A.reg = readIndex ;
    //--- Interrupt Acknowledge
      mModulePtr->IR.reg = CAN_IR_RF1N ;
    //--- Enter message into driver receive buffer 1
      mDriverReceiveFIFO1.append (message) ;
    }else if ((it & CAN_IR_TC) != 0) { // Transmission complete interrupt
    //--- Interrupt Acknowledge
      mModulePtr->IR.reg = CAN_IR_TC ;
    //--- Register sent message into transmit ack queue ?
      if (mCopyOfMessagesInHardwareTransmitFIFO.remove (message) && isTransmitAckEnabled ()) {
        mTransmitAckQueue.append (message) ;
      }
    //--- Write message into transmit fifo ?
      bool writeMessage = true ;
      while (writeMessage) {
        const uint32_t txfqs = mModulePtr->TXFQS.reg ; // Page 1165
        const uint32_t txFifoFreeLevel = txfqs & 0x3F ;
        if ((txFifoFreeLevel > 0) && mDriverTransmitFIFO.remove (message)) {
          const uint32_t putIndex = (txfqs >> 16) & 0x1F ;
          writeTxBuffer (message, putIndex) ;
          if (isTransmitAckEnabled ()) {
            mCopyOfMessagesInHardwareTransmitFIFO.append (message) ;
          }
        }else{
          writeMessage = false ;
        }
      }
    }else{
      loop = false ;
    }
  }
}

//----------------------------------------------------------------------------------------

ACANFD_SAME::Status::Status (Can * inModulePtr) :
mErrorCount (uint16_t (inModulePtr->ECR.reg)),
mProtocolStatus (inModulePtr->PSR.reg) {
}

//----------------------------------------------------------------------------------------
//    Standard filters
//----------------------------------------------------------------------------------------

bool ACANFD_SAME::StandardFilters::addSingle (const uint16_t inIdentifier,
                                                      const ACANFD_SAME_FilterAction inAction,
                                                      const ACANFDCallBackRoutine inCallBack) {
  return addDual (inIdentifier, inIdentifier, inAction, inCallBack) ;
}

//----------------------------------------------------------------------------------------

bool ACANFD_SAME::StandardFilters::addDual (const uint16_t inIdentifier1,
                                                    const uint16_t inIdentifier2,
                                                    const ACANFD_SAME_FilterAction inAction,
                                                    const ACANFDCallBackRoutine inCallBack) {
  const bool ok = (inIdentifier1 <= 0x7FF) && (inIdentifier2 <= 0x7FF) ;
  if (ok) {
    uint32_t filter = inIdentifier2 ;
    filter |= uint32_t (inIdentifier1) << 16 ;
    filter |= (1U << 30) ; // Dual filter (page 1182)
    filter |= ((uint32_t (inAction) + 1) << 27) ; // Filter action (page 1182)
    mFilterArray.append (filter) ;
    mCallBackArray.append (inCallBack) ;
  }
  return ok ;
}

//----------------------------------------------------------------------------------------

bool ACANFD_SAME::StandardFilters::addRange (const uint16_t inIdentifier1,
                                                     const uint16_t inIdentifier2,
                                                     const ACANFD_SAME_FilterAction inAction,
                                                     const ACANFDCallBackRoutine inCallBack) {
  const bool ok = (inIdentifier1 <= inIdentifier2) && (inIdentifier2 <= 0x7FF) ;
  if (ok) {
    uint32_t filter = inIdentifier2 ;
    filter |= uint32_t (inIdentifier1) << 16 ;
    filter |= ((uint32_t (inAction) + 1) << 27) ; // Filter action (page 1182)
    mFilterArray.append (filter) ;  // Filter type is 0 (RANGE, page 1182)
    mCallBackArray.append (inCallBack) ;
  }
  return ok ;
}

//----------------------------------------------------------------------------------------

bool ACANFD_SAME::StandardFilters::addClassic (const uint16_t inIdentifier,
                                                       const uint16_t inMask,
                                                       const ACANFD_SAME_FilterAction inAction,
                                                       const ACANFDCallBackRoutine inCallBack) {
  const bool ok = (inIdentifier <= 0x7FF)
               && (inMask <= 0x7FF)
               && ((inIdentifier & inMask) == inIdentifier) ;
  if (ok) {
    uint32_t filter = inMask ;
    filter |= uint32_t (inIdentifier) << 16 ;
    filter |= (2U << 30) ; // Classic filter (page 1182)
    filter |= ((uint32_t (inAction) + 1) << 27) ; // Filter action (page 1182)
    mFilterArray.append (filter) ;
    mCallBackArray.append (inCallBack) ;
  }
  return ok ;
}

//----------------------------------------------------------------------------------------
//    Extended filters
//----------------------------------------------------------------------------------------

static const uint32_t MAX_EXTENDED_IDENTIFIER = 0x1FFFFFFF ;

//----------------------------------------------------------------------------------------

bool ACANFD_SAME::ExtendedFilters::addSingle (const uint32_t inIdentifier,
                                                      const ACANFD_SAME_FilterAction inAction,
                                                      const ACANFDCallBackRoutine inCallBack) {
  return addDual (inIdentifier, inIdentifier, inAction, inCallBack) ;
}

//----------------------------------------------------------------------------------------

bool ACANFD_SAME::ExtendedFilters::addDual (const uint32_t inIdentifier1,
                                                    const uint32_t inIdentifier2,
                                                    const ACANFD_SAME_FilterAction inAction,
                                                    const ACANFDCallBackRoutine inCallBack) {
  const bool ok = (inIdentifier1 <= MAX_EXTENDED_IDENTIFIER)
               && (inIdentifier2 <= MAX_EXTENDED_IDENTIFIER) ;
  if (ok) {
    uint32_t filter = inIdentifier1 ;
    filter |= ((uint32_t (inAction) + 1) << 29) ; // Filter action (page 1182)
    mFilterArray.append (filter) ;
    filter = inIdentifier2 ;
    filter |= (1U << 30) ; // Dual filter (page 1182)
    mFilterArray.append (filter) ;
    mCallBackArray.append (inCallBack) ;
  }
  return ok ;
}

//----------------------------------------------------------------------------------------

bool ACANFD_SAME::ExtendedFilters::addRange (const uint32_t inIdentifier1,
                                                     const uint32_t inIdentifier2,
                                                     const ACANFD_SAME_FilterAction inAction,
                                                     const ACANFDCallBackRoutine inCallBack) {
  const bool ok = (inIdentifier1 <= inIdentifier2)
               && (inIdentifier2 <= MAX_EXTENDED_IDENTIFIER) ;
  if (ok) {
    uint32_t filter = inIdentifier1 ; // Filter type is 0 (RANGE, page 1182)
    filter |= ((uint32_t (inAction) + 1) << 29) ; // Filter action (page 1182)
    mFilterArray.append (filter) ;
    filter = inIdentifier2 ;
    mFilterArray.append (filter) ;  // Filter type is 0 (RANGE, page 1182)
    mCallBackArray.append (inCallBack) ;
  }
  return ok ;
}

//----------------------------------------------------------------------------------------

bool ACANFD_SAME::ExtendedFilters::addClassic (const uint32_t inIdentifier,
                                                       const uint32_t inMask,
                                                       const ACANFD_SAME_FilterAction inAction,
                                                       const ACANFDCallBackRoutine inCallBack) {
  const bool ok = (inIdentifier <= MAX_EXTENDED_IDENTIFIER)
               && (inMask <= MAX_EXTENDED_IDENTIFIER)
               && ((inIdentifier & inMask) == inIdentifier) ;
  if (ok) {
    uint32_t filter = inIdentifier ;
    filter |= ((uint32_t (inAction) + 1) << 29) ; // Filter action (page 1182)
    mFilterArray.append (filter) ;
    filter = inMask ;
    filter |= (2U << 30) ; // Classic filter (page 1182)
    mFilterArray.append (filter) ;
    mCallBackArray.append (inCallBack) ;
  }
  return ok ;
}

//----------------------------------------------------------------------------------------
