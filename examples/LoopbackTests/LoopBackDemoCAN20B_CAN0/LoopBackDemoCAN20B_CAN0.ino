// CAN0 external LoopBackDemo for SAME CAN board
// No external hardware required.
// For Feather M4 CAN this uses pins PA22 and PA23, you'll need an external transceiver.
// For all other boards pins are set automatically based on the board support package, or you can use canx.setPins(TX,RX); 

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
//   can0.messageRamRequiredMinimumSize () for getting it.

#define CAN0_MESSAGE_RAM_SIZE (1728)
#define CAN1_MESSAGE_RAM_SIZE (0)

#include <ACANFD_SAME.h>

// Required only when TinyUSB is selected, so the external Adafruit TinyUSB Library provides USB Serial.
#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(50);
  }
  Serial.println("CAN0 loopback test");

  // Remove this for Feather-M4-CAN or change to whatever standby pin you are using.
  pinMode(PIN_CAN0_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN0_STANDBY, false);  // turn off STANDBY

  // This sends data to the transceiver and loops it back to the RX so you can read them back.
  // Change to INTERNAL_LOOP_BACK if you want to test without the transceiver, this will then only test the internal controller.
  ACANFD_SAME_Settings settings(ACANFD_SAME_Settings::CLOCK_48MHz, 500 * 1000, DataBitRateFactor::x1);

  Serial.print("Bit Rate prescaler: ");
  Serial.println(settings.mBitRatePrescaler);
  Serial.print("Arbitration Phase segment 1: ");
  Serial.println(settings.mArbitrationPhaseSegment1);
  Serial.print("Arbitration Phase segment 2: ");
  Serial.println(settings.mArbitrationPhaseSegment2);
  Serial.print("Arbitration SJW: ");
  Serial.println(settings.mArbitrationSJW);
  Serial.print("Actual Arbitration Bit Rate: ");
  Serial.print(settings.actualArbitrationBitRate());
  Serial.println(" bit/s");
  Serial.print("Arbitration Sample point: ");
  Serial.print(settings.arbitrationSamplePointFromBitStart());
  Serial.println("%");
  Serial.print("Exact Arbitration Bit Rate ? ");
  Serial.println(settings.exactArbitrationBitRate() ? "yes" : "no");
  Serial.print("Data Phase segment 1: ");
  Serial.println(settings.mDataPhaseSegment1);
  Serial.print("Data Phase segment 2: ");
  Serial.println(settings.mDataPhaseSegment2);
  Serial.print("Data SJW: ");
  Serial.println(settings.mDataSJW);
  Serial.print("Actual Data Bit Rate: ");
  Serial.print(settings.actualDataBitRate());
  Serial.println(" bit/s");
  Serial.print("Data Sample point: ");
  Serial.print(settings.dataSamplePointFromBitStart());
  Serial.println("%");
  Serial.print("Exact Data Bit Rate ? ");
  Serial.println(settings.exactDataBitRate() ? "yes" : "no");

  settings.mModuleMode = ACANFD_SAME_Settings::EXTERNAL_LOOP_BACK;

  const uint32_t errorCode = can0.beginFD(settings);

  Serial.print("Message RAM required minimum size: ");
  Serial.print(can0.messageRamRequiredMinimumSize());
  Serial.println(" words");

  if (0 == errorCode) {
    Serial.println("can0 configuration ok");
  } else {
    Serial.print("Error can0 configuration: 0x");
    Serial.println(errorCode, HEX);
  }
}

//-----------------------------------------------------------------

static const uint32_t PERIOD = 1000;
static uint32_t gBlinkDate = PERIOD;
static uint32_t gSentCount = 0;
static uint32_t gReceiveCount = 0;

//-----------------------------------------------------------------

void loop() {
  if (gBlinkDate <= millis()) {
    gBlinkDate += PERIOD;
    CANMessage frame;
    frame.id = 0x7FF;
    // frame.ext = true ;
    // frame.rtr = true ;
    frame.len = 8;
    frame.data[0] = 0x11;
    frame.data[1] = 0x22;
    frame.data[2] = 0x33;
    frame.data[3] = 0x44;
    frame.data[4] = 0x55;
    frame.data[5] = 0x66;
    frame.data[6] = 0x77;
    frame.data[7] = 0x88;

    const uint32_t sendStatus = can0.sendFrame(frame);
    
    if (sendStatus == 0) {
      gSentCount += 1;
      Serial.print("Sent ");
      Serial.println(gSentCount);
    } else {
      Serial.print("Sent error 0x");
      Serial.println(sendStatus);
    }
  }
  //--- Receive frame
  CANFDMessage frame;
  if (can0.receiveFD0(frame)) {
    gReceiveCount += 1;
    Serial.print("Received ");
    Serial.println(gReceiveCount);
  }
}