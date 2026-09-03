// Shows how to set up the library and send a CAN 2.0 message on both channels with basic error handling with detailed comments.
// For a bare minimum example with less comments and no error handling, see BareMinimumSendCAN20_DUAL.

// IMPORTANT:
//   <ACANFD_SAME.h> should be included only from the .ino file
//   From another file, include <ACANFD_SAME-from-cpp.h>
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

// Required only when TinyUSB is selected, so the external Adafruit TinyUSB Library provides USB Serial.
#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

void setup() {
  Serial.begin(115200);
  // This halts the sketch for 5 seconds to allow the serial port to open so you don't miss any serial prints,
  // after 5 seconds the sketch will continue to run without serial port being connected.
  const uint32_t serialWaitStart = millis();
  while (!Serial && (millis() - serialWaitStart < 5000)) {
    delay(10);
  }

  Serial.println("CAN0 and CAN1 simple test");

  //Remove this for Feather-M4-CAN or change to what ever standby pins you maybe using for CAN0
  pinMode(PIN_CAN0_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN0_STANDBY, LOW); // turn off STANDBY

  // Change this to PIN_CAN_STANDBY for Feather-M4-CAN and turn on PIN_CAN_BOOSTEN
  pinMode(PIN_CAN1_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN1_STANDBY, LOW);

  // This looks more complicated than it is:
  // The variable name "settingsx" is arbitrary. You could call it channel0_settings, can0Settings, etc.
  // CLOCK_48MHz is the normal/default CAN clock and is suitable for most applications.
  // The next argument is abitration bit rate, for classic CAN 2.0 this is the only baud rate that matters and the only bus speed.
  // 500 x 1000 is simply 500000 bits/s (baud rate).
  // You could write 500000 instead, but 500 * 1000 makes "500 kbit/s" obvious at a glance.
  // For CAN FD, this arbitration bit rate is used for the arbitration phase.
  // For CAN FD, the data phase bit rate is arbitration bit rate * DataBitRateFactor
  // Examples:
  //   500 kbit/s arbitration, x1  -> 500 kbit/s data phase
  //   500 kbit/s arbitration, x4  -> 2 Mbit/s data phase
  //   500 kbit/s arbitration, x10 -> 5 Mbit/s data phase
  ACANFD_SAME_Settings settings0(ACANFD_SAME_Settings::CLOCK_48MHz, 500 * 1000, DataBitRateFactor::x1);
  ACANFD_SAME_Settings settings1(ACANFD_SAME_Settings::CLOCK_48MHz, 500 * 1000, DataBitRateFactor::x1);

  // The entire following section can be deleted if needed, if can be overwhelming to beginngers, its just to give you an
  // idea of what is going on inside the library, it's simply printing out most of the settings properties,
  // you'll never really need this in a normal sketch.
  // Start of settings print:
  Serial.print("CAN0 Bit Rate prescaler: ");
  Serial.println(settings0.mBitRatePrescaler);

  Serial.print("CAN1 Bit Rate prescaler: ");
  Serial.println(settings1.mBitRatePrescaler);

  Serial.print("CAN0 Arbitration Phase segment 1: ");
  Serial.println(settings0.mArbitrationPhaseSegment1);

  Serial.print("CAN1 Arbitration Phase segment 1: ");
  Serial.println(settings1.mArbitrationPhaseSegment1);

  Serial.print("CAN0 Arbitration Phase segment 2: ");
  Serial.println(settings0.mArbitrationPhaseSegment2);

  Serial.print("CAN1 Arbitration Phase segment 2: ");
  Serial.println(settings1.mArbitrationPhaseSegment2);

  Serial.print("CAN0 Arbitration SJW: ");
  Serial.println(settings0.mArbitrationSJW);

  Serial.print("CAN1 Arbitration SJW: ");
  Serial.println(settings1.mArbitrationSJW);

  Serial.print("CAN0 Actual Arbitration Bit Rate: ");
  Serial.print(settings0.actualArbitrationBitRate());
  Serial.println(" bit/s");

  Serial.print("CAN1 Actual Arbitration Bit Rate: ");
  Serial.print(settings1.actualArbitrationBitRate());
  Serial.println(" bit/s");

  Serial.print("CAN0 Arbitration Sample point: ");
  Serial.print(settings0.arbitrationSamplePointFromBitStart());
  Serial.println("%");

  Serial.print("CAN1 Arbitration Sample point: ");
  Serial.print(settings1.arbitrationSamplePointFromBitStart());
  Serial.println("%");

  Serial.print("CAN0 Exact Arbitration Bit Rate ? ");
  Serial.println(settings0.exactArbitrationBitRate() ? "yes" : "no");

  Serial.print("CAN1 Exact Arbitration Bit Rate ? ");
  Serial.println(settings1.exactArbitrationBitRate() ? "yes" : "no");

  Serial.print("CAN0 Data Phase segment 1: ");
  Serial.println(settings0.mDataPhaseSegment1);

  Serial.print("CAN1 Data Phase segment 1: ");
  Serial.println(settings1.mDataPhaseSegment1);

  Serial.print("CAN0 Data Phase segment 2: ");
  Serial.println(settings0.mDataPhaseSegment2);

  Serial.print("CAN1 Data Phase segment 2: ");
  Serial.println(settings1.mDataPhaseSegment2);

  Serial.print("CAN0 Data SJW: ");
  Serial.println(settings0.mDataSJW);

  Serial.print("CAN1 Data SJW: ");
  Serial.println(settings1.mDataSJW);

  Serial.print("CAN0 Actual Data Bit Rate: ");
  Serial.print(settings0.actualDataBitRate());
  Serial.println(" bit/s");

  Serial.print("CAN1 Actual Data Bit Rate: ");
  Serial.print(settings1.actualDataBitRate());
  Serial.println(" bit/s");

  Serial.print("CAN0 Data Sample point: ");
  Serial.print(settings0.dataSamplePointFromBitStart());
  Serial.println("%");

  Serial.print("CAN1 Data Sample point: ");
  Serial.print(settings1.dataSamplePointFromBitStart());
  Serial.println("%");

  Serial.print("CAN0 Exact Data Bit Rate ? ");
  Serial.println(settings0.exactDataBitRate() ? "yes" : "no");

  Serial.print("CAN1 Exact Data Bit Rate ? ");
  Serial.println(settings1.exactDataBitRate() ? "yes" : "no");
  // End of settings print.


  // The following sets the bus mode, options are:
  // NORMAL_FD:
  // Normal operating mode. Use this for transmitting and receiving real CAN / CAN FD messages on the bus.
  // For successful transmission in normal mode, another active CAN node must be present on the bus.
  //
  // INTERNAL_LOOP_BACK:
  // Internal controller loopback. Useful for testing software without a CAN transceiver or another CAN node.
  // Frames sent by the controller are looped back internally and can be received by the same controller,
  // but they are not actually transmitted onto the physical CAN bus.
  //
  // EXTERNAL_LOOP_BACK: External loopback through the CAN TX/RX pins/transceiver path.
  // Useful for testing more of the hardware path than internal loopback.
  // This normally requires the CAN transceiver and the transmitted frame is looped back and can be received by the same controller.
  //
  // BUS_MONITORING:
  // Listen-only / bus monitoring mode. Use this to receive frames from an existing CAN bus without actively participating in the bus.
  // The controller does not transmit normal frames and does not acknowledge received frames.
  settings0.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;
  settings1.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;

  // This is required to set up the CAN controller:
  // Creating the variable is not strictly needed. '.beginFD' returns an error code so this can be useful for debug.
  const uint32_t errorCode0 = can0.beginFD(settings0);
  const uint32_t errorCode1 = can1.beginFD(settings1);

  // This is useful when we want to tightly control memory usage.
  // We can run the sketch and then adjust the '#define CAN0_MESSAGE_RAM_SIZE (1728)'
  // based on the returned value.
  Serial.print("CAN0 Message RAM required minimum size: ");
  Serial.print(can0.messageRamRequiredMinimumSize());
  Serial.println(" words");
  Serial.print("CAN1 Message RAM required minimum size: ");
  Serial.print(can1.messageRamRequiredMinimumSize());
  Serial.println(" words");

  // This just prints to tell us if the .beginFD was ok or if there was an error.
  // Delete this if you are only using: can0.beginFD(settings);
  if (0 == errorCode0) {
    Serial.println("CAN0 configuration ok");
  } else {
    Serial.print("Error CAN0 configuration: 0x");
    Serial.println(errorCode0, HEX);
  }
  if (0 == errorCode1) {
    Serial.println("CAN1 configuration ok");
  } else {
    Serial.print("Error CAN1 configuration: 0x");
    Serial.println(errorCode1, HEX);
  }
}  //Set Up is complete


// These variables are used by the periodic transmit loop.
// PERIOD controls how often the CAN message is sent.
// sendTime stores the next millis() time at which the message should be sent.
// sentCount counts how many frames have been successfully queued/sent.
static const uint32_t PERIOD = 200;  // milliseconds; 500 ms = 5 times per second
static uint32_t sendTime = PERIOD;   // To keep track of the next scheduled time to send.
static uint32_t sentCount0 = 0;      // To count how many message have been sent on CAN0.
static uint32_t sentCount1 = 0;      // To count how many message have been sent on CAN1.

//-----------------------------------------------------------------

void loop() {
  if (millis() >= sendTime) {
    sendTime += PERIOD;

    // CANFDMessage is the ACANFD_SAME frame object.
    // Despite the name, this can be used to send either a classic CAN 2.0 frame or a CAN FD frame

    // The frame.type field determines the frame type, which can be:
    // CAN_DATA or CAN_REMOTE for 2.0 or,
    // CAN FD without bit-rate switch, or CAN FD with bit-rate switch.

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

    // This sends the message:
    // Creating the variable 'sendStatus' is not strictly needed. sendFrame returns an error code so this can be useful for debug.
    // You could just use:
    // can0.sendFrame(frame)
    // Remember 'frame' comes from the variable name we created above, we could have used can0.sendFrame(frame201) or similar.
    const uint32_t sendStatus0 = can0.sendFrame(frame0);
    const uint32_t sendStatus1 = can1.sendFrame(frame1);

    // This just prints to tell us if sendFrame was ok or if there was an error.
    // This is only for debugging. If you do not care about reporting errors, you can omit this block.
    // Delete this if you are only using: can0.sendFrame(frame);
    if (sendStatus0 == 0) {
      sentCount0 += 1;
      Serial.print("CAN0 Sent ");
      Serial.println(sentCount0);
    } else {
      Serial.print("CAN0 Sent error 0x");
      Serial.println(sendStatus0);
    }
    if (sendStatus1 == 0) {
      sentCount1 += 1;
      Serial.print("CAN1 Sent ");
      Serial.println(sentCount1);
    } else {
      Serial.print("CAN1 Sent error 0x");
      Serial.println(sendStatus1);
    }
  }
}