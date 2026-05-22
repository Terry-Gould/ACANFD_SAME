// This is the bare minimum needed to send a CAN 2.0 message.
// For a version with very detailed comments, basic error handling, and detailed serial prints, see SimpleSendCAN20_CAN0
// For Feather M4 CAN this uses pins PA22 and PA23, you'll need an external transceiver. 
// For all other boards pins are set automatically based on the board support package, or you can use canx.setPins(TX,RX);

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
//   can0.messageRamRequiredMinimumSize () for getting it.

#define CAN0_MESSAGE_RAM_SIZE (1728)
#define CAN1_MESSAGE_RAM_SIZE (0) // Even if only using CAN0, you need to include this.

#include <ACANFD_SAME.h>

void setup() {
  Serial.begin(115200);

  Serial.println("CAN0 bare minimum test");

  // Remove this for Feather-M4-CAN or change to whatever standby pin you are using.
  pinMode(PIN_CAN0_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN0_STANDBY, LOW);  // turn off STANDBY

  ACANFD_SAME_Settings settings(ACANFD_SAME_Settings::CLOCK_48MHz,
                                500 * 1000, // This is the baud rate
                                DataBitRateFactor::x1);

  settings.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD; // Change to INTERNAL_LOOP_BACK if testing without connected to a CAN Bus.

  can0.beginFD(settings);

}  //Set Up is complete

static const uint32_t PERIOD = 1000;  // milliseconds; 1000 ms = 1 time per second
static uint32_t sendTime = PERIOD;   // To keep track of the next scheduled time to send.

void loop() {
  if (millis() >= sendTime) {
    sendTime += PERIOD;

    CANFDMessage frame;                   // This can be named anything you like, you may need multiple names when sending and managing multiple frames.
    frame.id = 0x201;                     // For a standard 11-bit ID, valid range is: 0x000 to 0x7FF, For an extended 29-bit ID, valid range is: 0x00000000 to 0x1FFFFFFF.
    frame.type = CANFDMessage::CAN_DATA;  // For CAN 2.0 options are CAN_REMOTE, CAN_DATA 
    frame.len = 8;                        // How many bytes you are sending, 0-8 for CAN 2.0
    frame.data[0] = 0x11;
    frame.data[1] = 0x22;
    frame.data[2] = 0x33;
    frame.data[3] = 0x44;
    frame.data[4] = 0x55;
    frame.data[5] = 0x66;
    frame.data[6] = 0x77;
    frame.data[7] = 0x88;

    can0.tryToSendReturnStatusFD(frame);
    Serial.println("CAN0 frame sent");
  }
}