// Shows the bare minimum on how to set up the library and send a CAN 2.0 message on both channels.
// // For a version with very detailed comments, basic error handling, and detailed serial prints, see SimpleSendCAN20_DUAL.

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

#define CAN0_MESSAGE_RAM_SIZE (1728)
#define CAN1_MESSAGE_RAM_SIZE (1728)

#include <ACANFD_SAME.h>

void setup() {
  Serial.begin(115200);

  // This halts the sketch for 5 seconds to allow the serial port to open so you don't miss any serial prints,
  // after 5 seconds the sketch will continue to run without serial port being connected.
  const uint32_t serialWaitStart = millis();
  while (!Serial && (millis() - serialWaitStart < 5000)) {
    delay(10);
  }

  Serial.println("CAN0 and CAN1 bare minimum test");

  //Remove this for Feather-M4-CAN or change to what ever standby pins you maybe using for CAN0
  pinMode(PIN_CAN0_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN0_STANDBY, LOW);  // turn off STANDBY

  // Change this to PIN_CAN_STANDBY for Feather-M4-CAN and turn on PIN_CAN_BOOSTEN
  pinMode(PIN_CAN1_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN1_STANDBY, false);

  ACANFD_SAME_Settings settings0(ACANFD_SAME_Settings::CLOCK_48MHz,
                                 500 * 1000,  // This is the baud rate
                                 DataBitRateFactor::x1);
  ACANFD_SAME_Settings settings1(ACANFD_SAME_Settings::CLOCK_48MHz,
                                 500 * 1000,  // This is the baud rate
                                 DataBitRateFactor::x1);


  settings0.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;  // Change to INTERNAL_LOOP_BACK if testing without connected to a CAN Bus.
  settings1.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;

  can0.beginFD(settings0);
  can1.beginFD(settings1);

}  //Set Up is complete


static const uint32_t PERIOD = 200;  // milliseconds; 500 ms = 5 times per second
static uint32_t sendTime = PERIOD;   // To keep track of the next scheduled time to send.

void loop() {
  if (millis() >= sendTime) {
    sendTime += PERIOD;

    CANFDMessage frame0;                   // This can be named anything you like, you may need multiple names when sending and managing multiple frames.
    frame0.id = 0x101;                     // For a standard 11-bit ID, valid range is: 0x000 to 0x7FF, For an extended 29-bit ID, valid range is: 0x00000000 to 0x1FFFFFFF.
    frame0.ext = false;                    // This line is optional here because the default is normally false, but setting it explicitly makes the example clearer.
    frame0.type = CANFDMessage::CAN_DATA;  // For CAN 2.0 options are CAN_REMOTE, CAN_DATA
    frame0.len = 8;                        // How many bytes you are sending, 0-8 for CAN 2.0
    frame0.data[0] = 0x11;
    frame0.data[1] = 0x22;
    frame0.data[2] = 0x33;
    frame0.data[3] = 0x44;
    frame0.data[4] = 0x55;
    frame0.data[5] = 0x66;
    frame0.data[6] = 0x77;
    frame0.data[7] = 0x88;

    CANFDMessage frame1;
    frame1.id = 0x201;
    frame1.ext = false;
    frame1.type = CANFDMessage::CAN_DATA;
    frame1.len = 8;
    frame1.data[0] = 0x1E;
    frame1.data[1] = 0x14;
    frame1.data[2] = 0xFF;
    frame1.data[3] = 0xFF;
    frame1.data[4] = 0x55;
    frame1.data[5] = 0x66;
    frame1.data[6] = 0x77;
    frame1.data[7] = 0x88;

    can0.tryToSendReturnStatusFD(frame0);
    Serial.println("CAN0 frame sent");
    can1.tryToSendReturnStatusFD(frame1);
    Serial.println("CAN1 frame sent");
  }
}