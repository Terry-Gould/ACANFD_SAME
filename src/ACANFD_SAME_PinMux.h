//----------------------------------------------------------------------------------------
// ACANFD_SAME_PinMux.h
//
// Board-support-package driven CAN pin selection for ACANFD_SAME.
//
// The BSP supplies Arduino pin numbers, for example in variant.h:
//   #define PIN_CAN0_TX 7
//   #define PIN_CAN0_RX 8
//   #define PIN_CAN1_TX 9
//   #define PIN_CAN1_RX 10
//
// This file validates those Arduino pins against the MCU's legal CAN TX/RX port/pin/mux
// combinations and then applies the correct PORT PMUX configuration.
//----------------------------------------------------------------------------------------

#pragma once

#include <Arduino.h>

//----------------------------------------------------------------------------------------
// CAN pin macro handling
//
// Do not create PIN_CAN0_* aliases from PIN_CAN_* here. Some BSPs, notably Adafruit
// Feather M4 CAN, use PIN_CAN_TX/PIN_CAN_RX for physical pins that are actually CAN1
// on the SAME5x device, while PIN_CAN1_TX/PIN_CAN1_RX are PA22/PA23 and therefore
// actually CAN0.
//
// Macro names are treated only as candidates. The selected Arduino pins are always
// validated against g_APinDescription[] and the legal CAN port/pin/mux table below.
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------

struct ACANFD_SAME_PinMuxEntry {
  uint8_t mPort ;      // PORT group: A=0, B=1, C=2, D=3
  uint8_t mPin ;       // Pin number inside PORT group
  uint8_t mInstance ;  // CAN instance: CAN0=0, CAN1=1
  uint8_t mMux ;       // Peripheral function: A=0, B=1, ... H=7, I=8
} ;

//----------------------------------------------------------------------------------------
// Legal CAN pin mappings.
//
// SAM D5x/E5x / SAME51 / SAME54:
//   CAN0 TX: PA22 or PA24, function I
//   CAN0 RX: PA23 or PA25, function I
//   CAN1 TX: PB12 or PB14, function H
//   CAN1 RX: PB13 or PB15, function H
//
// SAME70 entries are included only so the pin-validation layer knows about the fixed MCAN
// pins.  The rest of ACANFD_SAME is still primarily a SAM D5x/E5x library.
//----------------------------------------------------------------------------------------

static const ACANFD_SAME_PinMuxEntry kACANFD_SAME_TxPinMuxTable [] = {
  // SAM D5x/E5x / SAME51 / SAME54
  {0, 22, 0, 8}, // PA22, CAN0 TX, function I
  {0, 24, 0, 8}, // PA24, CAN0 TX, function I
  {1, 12, 1, 7}, // PB12, CAN1 TX, function H
  {1, 14, 1, 7}, // PB14, CAN1 TX, function H

  // SAME70 MCAN pins.  Pin-compatible only; full functional support is not implied.
  {1,  2, 0, 0}, // PB02, MCAN0 TX, function A
  {2, 14, 1, 2}, // PC14, MCAN1 TX, function C
  {3, 12, 1, 1}  // PD12, MCAN1 TX, function B
} ;

static const ACANFD_SAME_PinMuxEntry kACANFD_SAME_RxPinMuxTable [] = {
  // SAM D5x/E5x / SAME51 / SAME54
  {0, 23, 0, 8}, // PA23, CAN0 RX, function I
  {0, 25, 0, 8}, // PA25, CAN0 RX, function I
  {1, 13, 1, 7}, // PB13, CAN1 RX, function H
  {1, 15, 1, 7}, // PB15, CAN1 RX, function H

  // SAME70 MCAN pins.  Pin-compatible only; full functional support is not implied.
  {1,  3, 0, 0}, // PB03, MCAN0 RX, function A
  {2, 12, 1, 2}, // PC12, MCAN1 RX, function C
  {3, 28, 1, 1}  // PD28, MCAN1 RX, function B
} ;

//----------------------------------------------------------------------------------------

static inline bool acanfd_same_arduinoPinExists (const int inArduinoPin) {
  if (inArduinoPin < 0) {
    return false ;
  }
  #ifdef PINS_COUNT
    return inArduinoPin < PINS_COUNT ;
  #else
    // Most Arduino SAMD/SAME cores define PINS_COUNT.  If a core does not, do not reject
    // the pin here; g_APinDescription is still the authority used below.
    return true ;
  #endif
}

//----------------------------------------------------------------------------------------

static inline bool acanfd_same_findPinMuxEntry (const ACANFD_SAME_PinMuxEntry * inTable,
                                                const size_t inTableCount,
                                                const int inArduinoPin,
                                                const uint8_t inInstance,
                                                ACANFD_SAME_PinMuxEntry & outEntry) {
  if (!acanfd_same_arduinoPinExists (inArduinoPin)) {
    return false ;
  }

  const uint8_t port = uint8_t (g_APinDescription [inArduinoPin].ulPort) ;
  const uint8_t pin  = uint8_t (g_APinDescription [inArduinoPin].ulPin) ;

  for (size_t i = 0 ; i < inTableCount ; i++) {
    if ((inTable [i].mPort == port) &&
        (inTable [i].mPin == pin) &&
        (inTable [i].mInstance == inInstance)) {
      outEntry = inTable [i] ;
      return true ;
    }
  }
  return false ;
}

//----------------------------------------------------------------------------------------

static inline void acanfd_same_applyPinMux (const ACANFD_SAME_PinMuxEntry & inEntry,
                                            const bool inIsTx) {
  PortGroup & portGroup = PORT->Group [inEntry.mPort] ;
  const uint32_t pinMask = uint32_t (1) << inEntry.mPin ;

  if (inIsTx) {
    portGroup.DIRSET.reg = pinMask ;
  }else{
    portGroup.DIRCLR.reg = pinMask ;
  }

  // Keep the previous PINCFG bits, but ensure the peripheral mux is enabled.  INEN is
  // deliberately enabled for both TX and RX to match the previous ACANFD_SAME behaviour.
  portGroup.PINCFG [inEntry.mPin].reg |= PORT_PINCFG_INEN | PORT_PINCFG_PMUXEN ;

  // PMUX registers control pin pairs.  Only update the nibble for this exact pin, leaving
  // the neighbouring odd/even pin untouched.
  volatile uint8_t & pmux = portGroup.PMUX [inEntry.mPin >> 1].reg ;
  if ((inEntry.mPin & 1U) == 0) {
    pmux = uint8_t ((pmux & 0xF0U) | (inEntry.mMux & 0x0FU)) ;
  }else{
    pmux = uint8_t ((pmux & 0x0FU) | ((inEntry.mMux & 0x0FU) << 4)) ;
  }
}

//----------------------------------------------------------------------------------------

static inline bool acanfd_same_tryConfigureCanPins (const int inTxArduinoPin,
                                                    const int inRxArduinoPin,
                                                    const uint8_t inInstance) {
  ACANFD_SAME_PinMuxEntry txEntry = {0, 0, 0, 0} ;
  ACANFD_SAME_PinMuxEntry rxEntry = {0, 0, 0, 0} ;

  if (!acanfd_same_findPinMuxEntry (kACANFD_SAME_TxPinMuxTable,
                                    sizeof (kACANFD_SAME_TxPinMuxTable) / sizeof (kACANFD_SAME_TxPinMuxTable [0]),
                                    inTxArduinoPin,
                                    inInstance,
                                    txEntry)) {
    return false ;
  }

  if (!acanfd_same_findPinMuxEntry (kACANFD_SAME_RxPinMuxTable,
                                    sizeof (kACANFD_SAME_RxPinMuxTable) / sizeof (kACANFD_SAME_RxPinMuxTable [0]),
                                    inRxArduinoPin,
                                    inInstance,
                                    rxEntry)) {
    return false ;
  }

  acanfd_same_applyPinMux (txEntry, true) ;
  acanfd_same_applyPinMux (rxEntry, false) ;
  return true ;
}

//----------------------------------------------------------------------------------------

static inline bool acanfd_same_configureExplicitCanPins (const ACANFD_SAME_Module inModule,
                                                         const int inTxArduinoPin,
                                                         const int inRxArduinoPin) {
  const uint8_t instance = (inModule == ACANFD_SAME_Module::can0) ? 0 : 1 ;
  return acanfd_same_tryConfigureCanPins (inTxArduinoPin, inRxArduinoPin, instance) ;
}

//----------------------------------------------------------------------------------------

static inline bool acanfd_same_configureCanPins (const ACANFD_SAME_Module inModule) {
  switch (inModule) {
  case ACANFD_SAME_Module::can0 :
    // Preferred sane convention: PIN_CAN0_* means actual CAN0.
    #if defined (PIN_CAN0_TX) && defined (PIN_CAN0_RX)
      if (acanfd_same_tryConfigureCanPins (PIN_CAN0_TX, PIN_CAN0_RX, 0)) {
        return true ;
      }
    #endif

    // Adafruit Feather M4 CAN compatibility: their PIN_CAN1_* are PA22/PA23,
    // which are actual CAN0 pins on SAME5x. This is validation based, not board-name based.
    #if defined (PIN_CAN1_TX) && defined (PIN_CAN1_RX)
      if (acanfd_same_tryConfigureCanPins (PIN_CAN1_TX, PIN_CAN1_RX, 0)) {
        return true ;
      }
    #endif

    // Last fallback for BSPs that expose only PIN_CAN_* and happen to map it to actual CAN0.
    #if defined (PIN_CAN_TX) && defined (PIN_CAN_RX)
      if (acanfd_same_tryConfigureCanPins (PIN_CAN_TX, PIN_CAN_RX, 0)) {
        return true ;
      }
    #endif
    break ;

  case ACANFD_SAME_Module::can1 :
    // Preferred sane convention: PIN_CAN1_* means actual CAN1.
    #if defined (PIN_CAN1_TX) && defined (PIN_CAN1_RX)
      if (acanfd_same_tryConfigureCanPins (PIN_CAN1_TX, PIN_CAN1_RX, 1)) {
        return true ;
      }
    #endif

    // Adafruit Feather M4 CAN compatibility: their onboard transceiver is named
    // PIN_CAN_* but is on PB14/PB15, which are actual CAN1 pins on SAME5x.
    #if defined (PIN_CAN_TX) && defined (PIN_CAN_RX)
      if (acanfd_same_tryConfigureCanPins (PIN_CAN_TX, PIN_CAN_RX, 1)) {
        return true ;
      }
    #endif

    // Last fallback for a BSP that accidentally exposes actual CAN1 as PIN_CAN0_*.
    #if defined (PIN_CAN0_TX) && defined (PIN_CAN0_RX)
      if (acanfd_same_tryConfigureCanPins (PIN_CAN0_TX, PIN_CAN0_RX, 1)) {
        return true ;
      }
    #endif
    break ;
  }
  return false ;
}

//----------------------------------------------------------------------------------------
