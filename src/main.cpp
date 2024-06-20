#include <Arduino.h>
#include "display_lib.h"
#include "ble_lib.h"
#include "obd2_lib.h"

byte rcv_data[12];

const String init_tbl[] = {
  "ATZ",    // reset all
  "ATD",    // set all to Default
  "ATE0",   // Echo off
  "ATD0",   // display of the DLC off
  // "ATD1",   // display of the DLC on (Headers offでは効かない？)
  // "ATH1",   // Headers on
  "ATH0",   // Headers off
  "ATSP7",  // Set Protocol to 7 and save it (29bit ID, 500kbps)
  "ATM0",   // Memory off
  "ATS0",   // printing of spaces off
  "ATAT1",  // Adaptive Timing of auto1
  "ATAL",   // Allow Long (>7 byte) messages
  "ATST64", // Set Timeout to 64x4 msec
};

#define SW1_PIN 3   // onboard switch

bool demo_mode = false;

void setup() {
  pinMode(SW1_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");

  if (digitalRead(SW1_PIN) == LOW) {
    demo_mode = true;
  }

  display_init();

  if (demo_mode == false) {
    ble_init(); // BLEデバイスのスキャンは30s

    while (ble_connectToServer() == false) {
      delay(100);
    }

    for (size_t i = 0; i < (sizeof(init_tbl) / sizeof(init_tbl[0])); i++) {
      ble_write(init_tbl[i]);

      while (ble_promptchk() == false) {
        delay(1);
      }
      // delay(100);
    }

    delay(3000);  // 安全のため、ACC ON後、車両のシステムが安定したと思えるあたりまでCAN通信を待ち合わせる

    display_inpane_draw();
  }
}

void loop() {
  if (demo_mode == true) {
    obd2_demo_update();
    display_inpane_draw();
  } else {
    if (ble_connectedchk()) {
      // ヘッダ設定
      ble_write(obd2_getHeader());
      while (ble_promptchk() == false) {
        delay(1);
      }

      // コマンド送信
      for (size_t i = 0; i < 12; i++) {
        rcv_data[i] = 0;
      }
      ble_write(obd2_getPacket());
      while (ble_promptchk() == false) {
        delay(1);
      }

      // コマンド受信解析
      if (obd2_ChkResponse(rcv_data) == true) {
        display_inpane_draw();  // 描画に150msほどかかるので待ち時間を入れていない
      } else {
        delay(100);
      }
    }
  }
}
