# ACANFD_SAME

CAN FD / Classic CAN driver for the CAN0 and CAN1 controllers on SAM E5x / SAME5x / SAME51 / SAME54 microcontrollers, with board-support-package driven CAN pin selection.

## Acknowledgements

This library is based on and derived from the excellent ACANFD_FeatherM4CAN library by Pierre Molinaro:

https://github.com/pierremolinaro/acanfd-feather-m4-can

This fork extends the original work with:

- BSP-driven automatic CAN pin detection
- Multi-board SAME51/SAME54 compatibility
- Smart validation of CAN controller pin mappings
- Adafruit Feather M4 CAN compatibility handling
- Optional explicit pin override APIs
- Additional BSP portability improvements

This library maintains the style/API and is intended to provide more broad support for SAME51/SAME54 class MCUs while keeping the familiar ACANFD / ACAN2517FD message model.

The current code is primarily targeted at SAM D5x/E5x / SAME51 / SAME54 devices. SAME70 CAN pin entries are present in the pin validation table, but this does **not** imply full functional SAME70 support.

## Main features

- Supports CAN0 and CAN1 where available.
- Supports Classic CAN 2.0 data frames and remote frames.
- Supports CAN FD frames up to 64 data bytes.
- Supports CAN FD bit-rate switching.
- Supports standard and extended identifiers.
- Supports RX FIFO0 and RX FIFO1.
- Supports standard and extended hardware filters.
- Supports interrupt-driven receive and transmit completion handling.
- Supports optional transmit acknowledge queue.
- Automatically configures CAN TX/RX pin muxes from the selected Arduino BSP.
- Provides optional `setPins()` override for unusual boards or manual testing.

## Important include rule

Include this header **only from the `.ino` file**:

```cpp
#include <ACANFD_SAME.h>
```

From other C++ source files, include:

```cpp
#include <ACANFD_SAME-from-cpp.h>
```

Before including `<ACANFD_SAME.h>`, define the message RAM allocation for both CAN modules:

```cpp
#define CAN0_MESSAGE_RAM_SIZE (1728)
#define CAN1_MESSAGE_RAM_SIZE (1728)

#include <ACANFD_SAME.h>
```

Use `0` to disable a module:

```cpp
#define CAN0_MESSAGE_RAM_SIZE (0)
#define CAN1_MESSAGE_RAM_SIZE (1728)

#include <ACANFD_SAME.h>
```

`ACANFD_SAME.h` creates the global objects:

```cpp
can0
can1
```

when the corresponding message RAM size is greater than zero.

## Message RAM size

The CAN message RAM is allocated by the sketch using:

```cpp
#define CAN0_MESSAGE_RAM_SIZE (...)
#define CAN1_MESSAGE_RAM_SIZE (...)
```

The value is in **32-bit words**, not bytes.

A common starting point is:

```cpp
#define CAN0_MESSAGE_RAM_SIZE (1728)
#define CAN1_MESSAGE_RAM_SIZE (1728)
```

After `beginFD()`, you can print the actual minimum required size:

```cpp
Serial.print("Message RAM required minimum size: ");
Serial.print(can1.messageRamRequiredMinimumSize());
Serial.println(" words");
```

If the supplied message RAM is too small, `beginFD()` returns an error code.

## Basic CAN1 transmit example

```cpp
#define CAN0_MESSAGE_RAM_SIZE (0)
#define CAN1_MESSAGE_RAM_SIZE (1728)

#include <ACANFD_SAME.h>

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000) {
    delay(10);
  }

  // If your board has a CAN transceiver standby pin, handle it in the sketch.
  // The library configures CAN TX/RX pin muxes only; it does not control transceiver standby.
  #ifdef PIN_CAN1_STANDBY
    pinMode(PIN_CAN1_STANDBY, OUTPUT);
    digitalWrite(PIN_CAN1_STANDBY, LOW); // low = normal mode on common CAN transceivers
  #endif

  ACANFD_SAME_Settings settings(
    ACANFD_SAME_Settings::CLOCK_48MHz,
    500 * 1000,
    DataBitRateFactor::x1
  );

  settings.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;

  const uint32_t errorCode = can1.beginFD(settings);
  if (errorCode == 0) {
    Serial.println("CAN1 configuration ok");
  } else {
    Serial.print("CAN1 configuration error: 0x");
    Serial.println(errorCode, HEX);
  }
}

void loop() {
  static uint32_t nextSend = 1000;

  if (millis() >= nextSend) {
    nextSend += 1000;

    CANFDMessage frame;
    frame.id = 0x201;
    frame.ext = false;
    frame.type = CANFDMessage::CAN_DATA;
    frame.len = 8;
    frame.data[0] = 0x11;
    frame.data[1] = 0x22;
    frame.data[2] = 0x33;
    frame.data[3] = 0x44;
    frame.data[4] = 0x55;
    frame.data[5] = 0x66;
    frame.data[6] = 0x77;
    frame.data[7] = 0x88;

    const uint32_t sendStatus = can1.sendFrame(frame);
    if (sendStatus == 0) {
      Serial.println("sent");
    } else {
      Serial.print("send error: 0x");
      Serial.println(sendStatus, HEX);
    }
  }
}
```

In `NORMAL_FD` mode, successful transmission on a real CAN bus normally requires another active CAN node to acknowledge the frame. If no other node is present, the controller may repeatedly retry, fill the transmit buffers, and `tryToSendReturnStatusFD()` may eventually return `kTransmitBufferOverflow`.

For software-only testing, use loopback mode:

```cpp
settings.mModuleMode = ACANFD_SAME_Settings::INTERNAL_LOOP_BACK;
```

or, to exercise more of the TX/RX path:

```cpp
settings.mModuleMode = ACANFD_SAME_Settings::EXTERNAL_LOOP_BACK;
```

## Settings object

A typical settings object is:

```cpp
ACANFD_SAME_Settings settings(
  ACANFD_SAME_Settings::CLOCK_48MHz,
  500 * 1000,
  DataBitRateFactor::x1
);
```

Arguments:

1. CAN clock selection.
2. Arbitration bit rate in bit/s.
3. CAN FD data phase multiplier.

Examples:

```cpp
ACANFD_SAME_Settings s0(ACANFD_SAME_Settings::CLOCK_48MHz, 500000, DataBitRateFactor::x1);  // 500 kbit/s arbitration, 500 kbit/s data
ACANFD_SAME_Settings s1(ACANFD_SAME_Settings::CLOCK_48MHz, 500000, DataBitRateFactor::x4);  // 500 kbit/s arbitration, 2 Mbit/s data
ACANFD_SAME_Settings s2(ACANFD_SAME_Settings::CLOCK_48MHz, 1000000, DataBitRateFactor::x5); // 1 Mbit/s arbitration, 5 Mbit/s data
```

Useful settings fields:

```cpp
settings.mModuleMode = ACANFD_SAME_Settings::NORMAL_FD;
settings.mDriverTransmitFIFOSize = 20;
settings.mDriverReceiveFIFO0Size = 10;
settings.mDriverReceiveFIFO1Size = 0;
settings.mHardwareRxFIFO0Size = 64;
settings.mHardwareRxFIFO1Size = 0;
settings.mHardwareTransmitTxFIFOSize = 24;
settings.mHardwareDedicacedTxBufferCount = 8;
settings.mEnableRetransmission = true;
```

Supported module modes:

```cpp
ACANFD_SAME_Settings::NORMAL_FD
ACANFD_SAME_Settings::INTERNAL_LOOP_BACK
ACANFD_SAME_Settings::EXTERNAL_LOOP_BACK
ACANFD_SAME_Settings::BUS_MONITORING
```

## CAN TX/RX pin detection

This version contains BSP-driven CAN TX/RX pin detection in:

```cpp
src/ACANFD_SAME_PinMux.h
```

The BSP supplies Arduino pin numbers in `variant.h`. The library then looks up those Arduino pins in `g_APinDescription[]`, validates the underlying MCU port/pin against a legal CAN pinmux table, and applies the correct PORT PMUX setting.

The library does **not** blindly trust the macro name. The actual MCU port/pin decides whether the selected pins belong to CAN0 or CAN1.

### Recommended BSP naming

For new boards, use explicit controller-numbered names:

```cpp
#define PIN_CAN0_STANDBY (5)
#define PIN_CAN1_STANDBY (6)
#define PIN_CAN0_TX      (7)
#define PIN_CAN0_RX      (8)
#define PIN_CAN1_TX      (9)
#define PIN_CAN1_RX      (10)
```

where `PIN_CAN0_TX/RX` are physically connected to the real CAN0 peripheral and `PIN_CAN1_TX/RX` are physically connected to the real CAN1 peripheral.

For SAM D5x/E5x / SAME51 / SAME54, the legal CAN pinmux combinations currently recognised by the library are:

| Controller | TX pin | RX pin | Peripheral function |
|---|---:|---:|---:|
| CAN0 | PA22 | PA23 | I |
| CAN0 | PA24 | PA25 | I |
| CAN1 | PB12 | PB13 | H |
| CAN1 | PB14 | PB15 | H |

The library also contains SAME70 MCAN pin entries for validation only. Full SAME70 functional support is not guaranteed.

### Adafruit Feather M4 CAN compatibility

The Adafruit Feather M4 CAN BSP uses non-intuitive CAN macro names:

```cpp
#define PIN_CAN_TX      (42) // PB14, real CAN1 TX
#define PIN_CAN_RX      (43) // PB15, real CAN1 RX
#define PIN_CAN1_TX     (12) // PA22, real CAN0 TX
#define PIN_CAN1_RX     (13) // PA23, real CAN0 RX
```

This library handles that automatically by trying candidate pin pairs and validating the real MCU port/pin against the requested CAN controller.

Therefore, on the Feather M4 CAN, this should work without editing the installed Adafruit BSP:

```cpp
#define CAN0_MESSAGE_RAM_SIZE (0)
#define CAN1_MESSAGE_RAM_SIZE (1728)
#include <ACANFD_SAME.h>

// Adafruit onboard CAN transceiver is on the real CAN1 pins PB14/PB15.
const uint32_t errorCode = can1.beginFD(settings);
```

The library will reject `PIN_CAN1_TX/RX` for `can1` if those pins resolve to real CAN0, then accept `PIN_CAN_TX/RX` if those pins resolve to real CAN1.

But note that to use the Feather M4 CAN onboard CAN Bus transceiver you should use CAN1.

### Explicit pin override

Automatic detection is the default. For unusual boards or debugging, you can explicitly state the Arduino TX/RX pins before `beginFD()`:

```cpp
can1.setPins(PIN_CAN_TX, PIN_CAN_RX);
const uint32_t errorCode = can1.beginFD(settings);
```

or:

```cpp
can0.setPins(12, 13);
const uint32_t errorCode = can0.beginFD(settings);
```

The explicit pins are still validated. If the pins do not belong to the requested CAN controller, `beginFD()` returns `kInvalidCANPinMux`.

To return to automatic BSP detection:

```cpp
can1.clearPins();
```

Call `setPins()` or `clearPins()` before `beginFD()`.

## Transceiver standby / enable pins

The library configures CAN TX/RX peripheral muxing only. It does **not** control external transceiver pins such as standby, enable, boost-enable, or silent-mode pins.

Handle those in the sketch or board-specific application code.

Examples:

```cpp
#ifdef PIN_CAN1_STANDBY
  pinMode(PIN_CAN1_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN1_STANDBY, LOW);
#endif
```

Adafruit Feather M4 CAN style:

```cpp
#ifdef PIN_CAN_STANDBY
  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, LOW);
#endif

#ifdef PIN_CAN_BOOSTEN
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, HIGH);
#endif
```

Check your transceiver datasheet and board schematic for the correct polarity.

## Sending messages

`CANFDMessage` can be used for both Classic CAN and CAN FD.

For normal use, send frames with `sendFrame()`:

```cpp
CANFDMessage frame;
frame.id = 0x123;
frame.ext = false;
frame.type = CANFDMessage::CAN_DATA;
frame.len = 8;
frame.data[0] = 0x01;
frame.data[1] = 0x02;
frame.data[2] = 0x03;
frame.data[3] = 0x04;
frame.data[4] = 0x05;
frame.data[5] = 0x06;
frame.data[6] = 0x07;
frame.data[7] = 0x08;

const uint32_t status = can1.sendFrame(frame);
```

`sendFrame()` sends through the normal transmit FIFO/Queue path. It ignores `frame.idx`, so it is the recommended function for normal sketches, gateway sketches, and pass-through sketches that forward a frame received from another CAN channel.

CAN FD frame with bit-rate switching:

```cpp
CANFDMessage frame;
frame.id = 0x123;
frame.ext = false;
frame.type = CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH;
frame.len = 64;
for (uint8_t i = 0; i < frame.len; i++) {
  frame.data[i] = i;
}

const uint32_t status = can1.sendFrame(frame);
```

Valid CAN FD lengths are:

```cpp
0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64
```

Use:

```cpp
frame.pad();
```

to pad an intermediate length to the next valid CAN FD DLC size.

`sendFrame()` returns `0` if the frame was accepted into the transmit path.

Transmit status values:

| Value | Name | Meaning |
|---:|---|---|
| 0 | success | Frame accepted into transmit path |
| 1 | `kInvalidMessage` | Invalid frame, often invalid length/type combination |
| 2 | `kTransmitBufferIndexTooLarge` | Dedicated TX buffer index is out of range |
| 3 | `kTransmitBufferOverflow` | Driver/hardware transmit queue is full |

### Sending with a dedicated TX buffer

For advanced applications, a frame can be sent through a specific dedicated TX buffer:

```cpp
CANFDMessage frame;
frame.id = 0x123;
frame.ext = false;
frame.type = CANFDMessage::CAN_DATA;
frame.len = 8;

const uint32_t status = can1.sendFrameToBuffer(frame, 0);
```

`sendFrameToBuffer(frame, 0)` sends through dedicated TX buffer 0.

The buffer index is zero-based:

```text
0 = dedicated TX buffer 0
1 = dedicated TX buffer 1
2 = dedicated TX buffer 2
```

The number of available dedicated TX buffers is configured with:

```cpp
settings.mHardwareDedicacedTxBufferCount
```

If the selected buffer does not exist, `sendFrameToBuffer()` returns `kTransmitBufferIndexTooLarge`.

If the selected buffer exists but is already waiting to transmit, `sendFrameToBuffer()` returns `kTransmitBufferOverflow`.

### Compatibility send function

The original ACANFD-style send function is still available:

```cpp
const uint32_t status = can1.tryToSendReturnStatusFD(frame);
```

This function uses `frame.idx` to select the transmit path:

```text
frame.idx = 0      normal TX FIFO
frame.idx = 1      dedicated TX buffer 0
frame.idx = 2      dedicated TX buffer 1
frame.idx = N      dedicated TX buffer N - 1
```

The same behaviour is also available through the clearer compatibility alias:

```cpp
const uint32_t status = can1.sendFrameUsingIndex(frame);
```

`sendFrameUsingIndex()` is only an alias for `tryToSendReturnStatusFD()`.

For new sketches, prefer `sendFrame()` or `sendFrameToBuffer()` because they make the transmit path explicit.

### Note about `frame.idx`

`frame.idx` is a driver field inherited from the original ACANFD-style API.

On receive, the library may use `frame.idx` to store the receive filter index that matched the frame.

On transmit, `tryToSendReturnStatusFD()` and `sendFrameUsingIndex()` use `frame.idx` as a transmit path selector.

This means a received frame should not normally be forwarded with `tryToSendReturnStatusFD()` unless you intentionally want to reuse the received `idx` value as transmit control.

For gateway and pass-through sketches, use:

```cpp
can1.sendFrame(frame);
```

instead of:

```cpp
can1.tryToSendReturnStatusFD(frame);
```
## Receiving messages

Polling FIFO0:

```cpp
CANFDMessage frame;
if (can1.receiveFD0(frame)) {
  Serial.print("received ID 0x");
  Serial.println(frame.id, HEX);
}
```

Polling FIFO1:

```cpp
CANFDMessage frame;
if (can1.receiveFD1(frame)) {
  Serial.print("received ID 0x");
  Serial.println(frame.id, HEX);
}
```

You can also use:

```cpp
can1.dispatchReceivedMessage();
can1.dispatchReceivedMessageFIFO0();
can1.dispatchReceivedMessageFIFO1();
```

with callbacks configured through standard or extended filters.

## Filters

Standard ID filters:

```cpp
ACANFD_SAME::StandardFilters filters;
filters.addSingle(0x123, ACANFD_SAME_FilterAction::FIFO0);
filters.addClassic(0x200, 0x700, ACANFD_SAME_FilterAction::FIFO1);

const uint32_t errorCode = can1.beginFD(settings, filters);
```

Extended ID filters:

```cpp
ACANFD_SAME::ExtendedFilters extFilters;
extFilters.addSingle(0x18FF50E5, ACANFD_SAME_FilterAction::FIFO0);

const uint32_t errorCode = can1.beginFD(settings, extFilters);
```

With both standard and extended filters:

```cpp
ACANFD_SAME::StandardFilters stdFilters;
ACANFD_SAME::ExtendedFilters extFilters;

const uint32_t errorCode = can1.beginFD(settings, stdFilters, extFilters);
```

Non-matching frame behaviour is controlled by:

```cpp
settings.mNonMatchingStandardFrameReception = ACANFD_SAME_FilterAction::FIFO0;
settings.mNonMatchingExtendedFrameReception = ACANFD_SAME_FilterAction::FIFO0;
```

Available actions:

```cpp
ACANFD_SAME_FilterAction::FIFO0
ACANFD_SAME_FilterAction::FIFO1
ACANFD_SAME_FilterAction::REJECT
```

## Transmit acknowledgements

Transmit acknowledgement storage is optional:

```cpp
can1.setTransmitAckCapacity(32);
```

Then:

```cpp
CANFDMessage ack;
if (can1.getTransmitAck(ack)) {
  Serial.print("transmitted ID 0x");
  Serial.println(ack.id, HEX);
}
```

Disable and clear transmit acknowledgements with:

```cpp
can1.setTransmitAckCapacity(0);
```

## `beginFD()` error codes

`beginFD()` returns `0` on success. Non-zero values are bit flags and may be ORed together.

| Hex value | Name | Meaning |
|---:|---|---|
| `0x00100000` | `kMessageRamTooSmall` | Allocated message RAM is too small |
| `0x00200000` | `kMessageRamNotInFirst64kio` | Message RAM is not in the required address range |
| `0x00400000` | `kHardwareRxFIFO0SizeGreaterThan64` | RX FIFO0 hardware size is too large |
| `0x00800000` | `kHardwareTransmitFIFOSizeGreaterThan32` | Hardware TX FIFO size is too large |
| `0x01000000` | `kDedicacedTransmitTxBufferCountGreaterThan30` | Dedicated TX buffer count is too large |
| `0x02000000` | `kTxBufferCountGreaterThan32` | Total TX buffer count exceeds 32 |
| `0x04000000` | `kHardwareTransmitFIFOSizeLowerThan2` | Hardware TX FIFO size is too small |
| `0x08000000` | `kHardwareRxFIFO1SizeGreaterThan64` | RX FIFO1 hardware size is too large |
| `0x10000000` | `kStandardFilterCountGreaterThan128` | Too many standard filters |
| `0x20000000` | `kExtendedFilterCountGreaterThan128` | Too many extended filters |
| `0x40000000` | `kInvalidCANPinMux` | The selected TX/RX Arduino pins do not validate as legal pins for the requested CAN controller |

Lower bits may also be returned by `ACANFD_SAME_Settings::CANFDBitSettingConsistency()` for invalid bit timing settings.

### `kInvalidCANPinMux` / `0x40000000`

This error means the library could not configure the CAN TX/RX pins for the requested module.

Common causes:

- The selected BSP does not define suitable CAN pin macros.
- `g_APinDescription[]` does not map the Arduino pin numbers to the expected MCU port/pin.
- `can0` is being used with pins that are actually CAN1 pins, or vice versa.
- `setPins()` was called with pins that do not belong to the requested controller.
- The selected board variant is not the variant you think it is.

Example debug print:

```cpp
const uint32_t errorCode = can1.beginFD(settings);
if (errorCode != 0) {
  Serial.print("beginFD error 0x");
  Serial.println(errorCode, HEX);
}
```

## Common troubleshooting

### `beginFD()` returns `0x40000000`

The CAN TX/RX pinmux validation failed. Check the board variant macros and `g_APinDescription[]` entries for the selected board.

For a sane SAME51/SAME54 BSP, typical definitions are:

```cpp
#define PIN_CAN0_TX 7  // Arduino pin resolving to PA22 or PA24
#define PIN_CAN0_RX 8  // Arduino pin resolving to PA23 or PA25
#define PIN_CAN1_TX 9  // Arduino pin resolving to PB12 or PB14
#define PIN_CAN1_RX 10 // Arduino pin resolving to PB13 or PB15
```

### `tryToSendReturnStatusFD()` eventually returns `3`

`3` is `kTransmitBufferOverflow`. The transmit path is full.

In normal CAN mode, this commonly happens when the controller is trying to transmit but no other CAN node acknowledges the frame. Check:

- another active CAN node is connected
- CANH/CANL are not swapped
- bus termination is correct
- both nodes use the same arbitration bit rate
- the transceiver is not in standby/silent mode
- the correct CAN controller is being used for the physical transceiver

CAN acknowledgement is handled automatically by the CAN controller/transceiver hardware, but it still requires at least one other CAN node on the bus to receive the frame and drive the ACK bit.

### No CANH/CANL activity

Check the transceiver control pins first. This library configures the MCU TX/RX mux only; it does not wake or enable the external transceiver.

Also check whether you are probing the MCU TX pin or the CANH/CANL side. A disabled transceiver may still allow MCU TX activity but no CANH/CANL activity.

### CAN0 works but CAN1 does not, or vice versa

Check that the BSP pin names match the real controller, or rely on automatic validation if using a known odd BSP such as Adafruit Feather M4 CAN.

For custom boards, prefer explicit controller-numbered names:

```cpp
PIN_CAN0_TX / PIN_CAN0_RX -> real CAN0
PIN_CAN1_TX / PIN_CAN1_RX -> real CAN1
```

## Example sketches

The library includes example sketches under `examples/`, including:

- `BareMinimumSendCAN20_CAN0`
- `SimpleSendCAN20_CAN0`
- `SimpleSendCAN20_CAN1`
- `LoopBackDemoCAN20B_CAN0`
- `LoopBackDemoCAN20B_CAN1`
- `LoopBackDemoCANFD_CAN0`
- `LoopBackDemoCANFD_CAN1`
- `LoopBackDemoCANFD_CAN1_dispatch`
- `LoopBackDemoCANFD_CAN1_StandardFilters`
- `LoopBackDemoCANFD_CAN1_ExtendedFilters`
- `LoopBackDemoCANFDIntensive_CAN0`
- `LoopBackDemoCANFDIntensive_CAN1`
- `LoopBackDemoCANFDIntensive_CAN1_FIFO01`
- `LoopBackDemoCANFDIntensive_CAN1_payload`
- `TestingTransceiverDelayCompensation_CAN1`
- `DualChannelCallbacks_CAN20`
- `PassThrough_ModifyMessage`
- `BareMinimumSendCAN20_DUAL`
- `SimpleSendCAN20_DUAL`

## Notes for board package authors

For new SAME51/SAME54 variants, define CAN pins in `variant.h` using explicit controller numbering:

```cpp
#define PIN_CAN0_STANDBY (...)
#define PIN_CAN1_STANDBY (...)
#define PIN_CAN0_TX      (...)
#define PIN_CAN0_RX      (...)
#define PIN_CAN1_TX      (...)
#define PIN_CAN1_RX      (...)
```

Then ensure `variant.cpp` maps those Arduino pin numbers to the correct MCU port/pin in `g_APinDescription[]`.

You do not need to define Adafruit-style aliases:

```cpp
#define PIN_CAN_TX ...
#define PIN_CAN_RX ...
```

unless your board package intentionally wants those public names for compatibility with other libraries. ACANFD_SAME does not require them for sane BSPs.

