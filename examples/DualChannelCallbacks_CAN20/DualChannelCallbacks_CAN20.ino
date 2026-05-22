// DualChannelCallbacks_CAN20.ino
// The example shows you how you can use callbacks to recieve known IDs instead of polling.
// This is usefull for applications where you want to act on certain IDs.
// This example configures both CAN0 and CAN1 in normal CAN FD capable mode, but sends
// ordinary Classic CAN 2.0 data frames.
//
// It demonstrates:
//   - using CAN0 and CAN1 at the same time
//   - configuring standard ID hardware filters on each CAN controller
//   - attaching callback functions to selected CAN IDs
//   - dispatching received messages from loop()
//   - periodically transmitting test frames on both CAN channels
//
// CAN0 filter setup:
//   - Standard ID 0x240 -> FIFO0 -> callBackFor0x240()
//   - Standard ID 0x055 -> FIFO0 -> callBackFor0x055()
//   - Non-matching extended frames are rejected
//
// CAN1 filter setup:
//   - Standard ID 0x201 -> FIFO0 -> callBackFor0x201()
//   - Non-matching extended frames are rejected
//
// The transmitted test frames in this example use IDs 0x7FF and 0x696, so they will not
// trigger the callbacks unless another node sends the filtered IDs back onto the bus.
//
// IMPORTANT:
//   <ACANFD_SAME.h> should be included only from the .ino file.
//   From another file, include <ACANFD_SAME-from-cpp.h>.
//   Before including <ACANFD_SAME.h>, define the Message RAM size for CAN0 and CAN1.
//   Maximum required size is 4,352 32-bit words per CAN module.
//   A size of 0 means that CAN module is not configured; its TX/RX pins can be freely
//   used for another function.
//   The beginFD() method checks whether the actual message RAM size is greater than
//   or equal to the required size.
//   Hint: if you do not want to compute the required size manually, print
//   can0.messageRamRequiredMinimumSize() or can1.messageRamRequiredMinimumSize()
//   after beginFD().

#define CAN0_MESSAGE_RAM_SIZE (4352)
#define CAN1_MESSAGE_RAM_SIZE (4352)

#include <ACANFD_SAME.h>

// Called when CAN0 receives a standard frame matching ID 0x240.
static void callBackFor0x240(const CANFDMessage& /* inMessage */) {
  Serial.println("We got a message on CAN0 ID 0x240");
  // Do Something
}

// Called when CAN0 receives a standard frame matching ID 0x055.
static void callBackFor0x055(const CANFDMessage& /* inMessage */) {
  Serial.println("We got a message on CAN0 ID 0x055");
  // Do Something
}

// Called when CAN1 receives a standard frame matching ID 0x201.
static void callBackFor0x201(const CANFDMessage& /* inMessage */) {
  Serial.println("We got a message on CAN1 ID 0x201");
  // Do Something
}

//-----------------------------------------------------------------

void setup() {
  // Wake/enable the external CAN transceivers if the BSP exposes standby pins.
  // The ACANFD_SAME library configures the MCU CAN TX/RX pin muxes, but it does not
  // control external transceiver standby/enable pins.

  //Remove this for Feather-M4-CAN or change to what ever standby pins you maybe using for CAN0
  pinMode(PIN_CAN0_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN0_STANDBY, LOW);  // turn off STANDBY

  // Change this to PIN_CAN_STANDBY for Feather-M4-CAN and turn on PIN_CAN_BOOSTEN
  pinMode(PIN_CAN1_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN1_STANDBY, LOW);

  Serial.begin(115200);
  while (!Serial) {
    delay(50);
  }
  Serial.println("CAN0 and CAN1 dispatch/loopback test");

  // Configure CAN0 and CAN1 for 500 kbit/s arbitration/data rate.
  // DataBitRateFactor::x1 means the CAN FD data phase is the same rate as arbitration.
  ACANFD_SAME_Settings settings0(ACANFD_SAME_Settings::CLOCK_48MHz, 500 * 1000, DataBitRateFactor::x1);
  ACANFD_SAME_Settings settings1(ACANFD_SAME_Settings::CLOCK_48MHz, 500 * 1000, DataBitRateFactor::x1);

  // Use normal bus operation. This requires a correctly wired/terminated CAN bus and
  // at least one other active node to acknowledge transmitted frames.
  // Use INTERNAL_LOOP_BACK for testing without connection to a CAN Bus.
  settings0.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;
  settings1.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;

  // CAN0 standard filters:
  // Classic filter with mask 0x7FF means "match this exact 11-bit standard ID".
  ACANFD_SAME::StandardFilters standardFilters0;
  standardFilters0.addClassic(0x240, 0x7FF, ACANFD_SAME_FilterAction::FIFO0, callBackFor0x240);
  standardFilters0.addClassic(0x055, 0x7FF, ACANFD_SAME_FilterAction::FIFO0, callBackFor0x055);

  // Reject non-matching extended frames on CAN0.
  // Standard frame behaviour not matching the above filters remains at the library default
  // unless explicitly changed.
  settings0.mNonMatchingExtendedFrameReception = ACANFD_SAME_FilterAction::REJECT;

  // CAN1 standard filters:
  ACANFD_SAME::StandardFilters standardFilters1;
  standardFilters1.addClassic(0x201, 0x7FF, ACANFD_SAME_FilterAction::FIFO0, callBackFor0x201);

  // Reject non-matching extended frames on CAN1.
  settings1.mNonMatchingExtendedFrameReception = ACANFD_SAME_FilterAction::REJECT;

  //--- Allocate FIFOs
  settings0.mHardwareRxFIFO0Size = 64;
  settings0.mDriverReceiveFIFO0Size = 64;
  settings1.mHardwareRxFIFO0Size = 64;
  settings1.mDriverReceiveFIFO0Size = 64;
  settings0.mHardwareRxFIFO1Size = 64;
  settings0.mDriverReceiveFIFO1Size = 64;
  settings1.mHardwareRxFIFO1Size = 64;
  settings1.mDriverReceiveFIFO1Size = 64;

  // Start both CAN controllers.
  const uint32_t errorCode0 = can0.beginFD(settings0, standardFilters0);
  const uint32_t errorCode1 = can1.beginFD(settings1, standardFilters1);

  // Print actual message RAM usage so the fixed 4352-word allocation can be reduced later.
  Serial.print("CAN0 Message RAM required minimum size: ");
  Serial.print(can0.messageRamRequiredMinimumSize());
  Serial.println(" words");
  Serial.print("CAN1 Message RAM required minimum size: ");
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

static const uint32_t PERIOD7FF = 10;
static uint32_t gtime7FF = PERIOD7FF;
static uint32_t gSentCount7FF = 0;
static const uint32_t PERIOD696 = 10;
static uint32_t gtime696 = PERIOD696;
static uint32_t gSentCount696 = 0;

void loop() {
  // Dispatch received messages and call the matching callback functions.
  // This must be called regularly if using callbacks.
  can0.dispatchReceivedMessage();
  can1.dispatchReceivedMessage();

  // Send a Classic CAN frame on CAN0 with ID 0x7FF every 10 ms.
  if (gtime7FF <= millis()) {
    gtime7FF += PERIOD7FF;
    CANMessage frame7FF;
    frame7FF.id = 0x7FF;
    frame7FF.len = 8;
    frame7FF.data[0] = 0x11;
    frame7FF.data[1] = 0x22;
    frame7FF.data[2] = 0x33;
    frame7FF.data[3] = 0x44;
    frame7FF.data[4] = 0x55;
    frame7FF.data[5] = 0x66;
    frame7FF.data[6] = 0x77;
    frame7FF.data[7] = 0x88;
    const uint32_t sendStatus7FF = can0.tryToSendReturnStatusFD(frame7FF);
    if (sendStatus7FF == 0) {
      gSentCount7FF += 1;
      Serial.print("Sent CAN0 ");
      Serial.println(gSentCount7FF);
    } else {
      Serial.print("Sent CAN0 error 0x");
      Serial.println(sendStatus7FF);
    }
  }

  // Send a Classic CAN frame on CAN1 with ID 0x696 every 10 ms.
  if (gtime696 <= millis()) {
    gtime696 += PERIOD696;
    CANMessage frame696;
    frame696.id = 0x696;
    frame696.len = 8;
    frame696.data[0] = 0x11;
    frame696.data[1] = 0x22;
    frame696.data[2] = 0x33;
    frame696.data[3] = 0x44;
    frame696.data[4] = 0x55;
    frame696.data[5] = 0x66;
    frame696.data[6] = 0x77;
    frame696.data[7] = 0x88;
    const uint32_t sendStatus696 = can1.tryToSendReturnStatusFD(frame696);
    if (sendStatus696 == 0) {
      gSentCount696 += 1;
      Serial.print("Sent CAN1 ");
      Serial.println(gSentCount696);
    } else {
      Serial.print("Sent CAN1 error 0x");
      Serial.println(sendStatus696);
    }
  }
}