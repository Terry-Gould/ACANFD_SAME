// This example uses both CAN channels to create a simple bidirectional CAN bridge.

// CAN1 -> CAN0:
//   All frames received on CAN1 are forwarded unchanged to CAN0.

// CAN0 -> CAN1:
//   Standard ID 0x240 is routed to FIFO0, modified if required, then forwarded to CAN1.
//   All other standard IDs and all extended IDs are routed to FIFO1 and forwarded unchanged.

// For standard ID 0x240, bytes 0 and 1 are treated as a big-endian 16-bit value.
// If that value is greater than MAX_240_VALUE, it is capped before the frame is
// forwarded. The rest of the frame is left unchanged.

// This demonstrates:
//  - using CAN0 and CAN1 at the same time
//  - bidirectional frame forwarding
//  - using FIFO0 for frames that need inspection/modification
//  - using FIFO1 for raw pass-through traffic
//  - configuring hardware filters for a specific standard CAN ID

// Good examples of this would be:
//  - Limiting/scalling a speed signal going to one module without affectin the main CAN Bus.
//  - Bridging two CAN buses while modifying selected signals
//  - Agricultural/off-highway/race track vehicle integration

// IMPORTANT:
//   <ACANFD_SAME.h> should be included only from the .ino file
//   From an other file, include <ACANFD_SAME-from-cpp.h>
//   Before including <ACANFD_SAME.h>, you should define
//   Message RAM size for CAN0 and Message RAM size for CAN1.
//   Maximum required size is 4,352 (4,352 32-bit words).
//   A 0 size means the CAN module is not configured; its TxCAN and RxCAN pins
//   can be freely used for an other function.
//   The begin method checks if actual size is greater or equal to required size.
//   Hint: if you do not want to compute required size, print
//   can1.messageRamRequiredMinimumSize () for getting it.

#define CAN0_MESSAGE_RAM_SIZE (4352)
#define CAN1_MESSAGE_RAM_SIZE (4352)

#include <ACANFD_SAME.h>

//-----------------------------------------------------------------
static const uint16_t MAX_240_VALUE = 10000;
//-----------------------------------------------------------------

static void forwardCAN1ToCAN0(void) {
  CANFDMessage frame;

  while (can1.receiveFD0(frame)) {
    can0.tryToSendReturnStatusFD(frame);
  }

  while (can1.receiveFD1(frame)) {
    can0.tryToSendReturnStatusFD(frame);
  }
}

static void forwardCAN0ToCAN1FIFO1(void) {
  CANFDMessage frame;

  while (can0.receiveFD1(frame)) {
    can1.tryToSendReturnStatusFD(frame);
  }
}

static void forwardFiltered0x240CAN0ToCAN1(void) {
  CANFDMessage inFrame;

  while (can0.receiveFD0(inFrame)) {
    CANFDMessage outFrame = inFrame;

    if ((!inFrame.ext) && (inFrame.id == 0x240) && (inFrame.len >= 2)) {
      uint16_t value = ((uint16_t)inFrame.data[0] << 8) | (uint16_t)inFrame.data[1];

      if (value > MAX_240_VALUE) {
        value = MAX_240_VALUE;
      }

      outFrame.data[0] = (uint8_t)(value >> 8);
      outFrame.data[1] = (uint8_t)(value & 0xFF);
    }

    can1.tryToSendReturnStatusFD(outFrame);
  }
}

//-----------------------------------------------------------------

void setup() {

  //Remove this for Feather-M4-CAN or change to what ever standby pins you maybe using for CAN0
  pinMode(PIN_CAN0_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN0_STANDBY, LOW); // turn off STANDBY

  // Change this to PIN_CAN_STANDBY for Feather-M4-CAN and turn on PIN_CAN_BOOSTEN
  pinMode(PIN_CAN1_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN1_STANDBY, LOW); // turn off STANDBY

  Serial.begin(115200);

  Serial.println("Example a Pass Through with Filter");

  //--- CAN0:
  //    standard 0x240 -> FIFO0 for edit path
  //    all other standard IDs -> FIFO1 raw pass-through
  //    all extended IDs -> FIFO1 raw pass-through
  ACANFD_SAME_Settings settings0(ACANFD_SAME_Settings::CLOCK_48MHz, 500 * 1000, DataBitRateFactor::x1);
  settings0.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;
  settings0.mNonMatchingStandardFrameReception = ACANFD_SAME_FilterAction::FIFO1;
  settings0.mNonMatchingExtendedFrameReception = ACANFD_SAME_FilterAction::FIFO1;
  settings0.mHardwareRxFIFO0Size = 64;
  settings0.mDriverReceiveFIFO0Size = 64;
  settings0.mHardwareRxFIFO1Size = 64;
  settings0.mDriverReceiveFIFO1Size = 64;

  ACANFD_SAME::StandardFilters standardFilters0;
  standardFilters0.addClassic(0x240, 0x7FF, ACANFD_SAME_FilterAction::FIFO0, nullptr);

  //--- CAN1:
  //    all standard IDs -> FIFO0 raw pass-through
  //    all extended IDs -> FIFO0 raw pass-through
  ACANFD_SAME_Settings settings1(ACANFD_SAME_Settings::CLOCK_48MHz, 500 * 1000, DataBitRateFactor::x1);
  settings1.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;
  settings1.mNonMatchingStandardFrameReception = ACANFD_SAME_FilterAction::FIFO0;
  settings1.mNonMatchingExtendedFrameReception = ACANFD_SAME_FilterAction::FIFO0;
  settings1.mHardwareRxFIFO0Size = 64;
  settings1.mDriverReceiveFIFO0Size = 64;
  settings1.mHardwareRxFIFO1Size = 0;
  settings1.mDriverReceiveFIFO1Size = 0;

  const uint32_t errorCode0 = can0.beginFD(settings0, standardFilters0);
  const uint32_t errorCode1 = can1.beginFD(settings1);

  Serial.print("CAN 0 Message RAM required minimum size: ");
  Serial.print(can0.messageRamRequiredMinimumSize());
  Serial.println(" words");
  Serial.print("CAN 1 Message RAM required minimum size: ");
  Serial.print(can1.messageRamRequiredMinimumSize());
  Serial.println(" words");

  if (0 == errorCode0) {
    Serial.println("can0 configuration ok");
  } else {
    Serial.print("Error can0 configuration: 0x");
    Serial.println(errorCode0, HEX);
  }

  if (0 == errorCode1) {
    Serial.println("can1 configuration ok");
  } else {
    Serial.print("Error can1 configuration: 0x");
    Serial.println(errorCode1, HEX);
  }
}

//-----------------------------------------------------------------

void loop() {
  forwardCAN1ToCAN0();
  forwardCAN0ToCAN1FIFO1();
  forwardFiltered0x240CAN0ToCAN1();
}