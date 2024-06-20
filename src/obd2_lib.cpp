#include <Arduino.h>
#include "can_lib.h"
#include "obd2_lib.h"

const st_cmdtbl cmd_tbl[] = {
  // [VCM] HV Motor RPMを２回に１回発行する
  {CANID_VCM, PID_MOTOR_RPM},     // 定期発行
  // {CANID_STD, PID_PEDAL_D_POS},
  {CANID_BMS, PID_BMS_VOLT},
  {CANID_VCM, PID_MOTOR_RPM},     // 定期発行
  // {CANID_STD, PID_THROTTLE_POS},
  {CANID_BMS, PID_BMS_CURR},
  {CANID_VCM, PID_MOTOR_RPM},     // 定期発行
  {CANID_STD, PID_ENGINE_RPM},
  {CANID_VCM, PID_MOTOR_RPM},     // 定期発行
  {CANID_BMS, PID_BMS_SOC},
  {CANID_VCM, PID_MOTOR_RPM},     // 定期発行
  {CANID_STD, PID_ENGINE_TEMP},
};

int obd2_eng_temp = -99;
int obd2_eng_rpm = 0;
int obd2_kmh = 0;
float obd2_throttle = 0;
float obd2_pedal_d = 0;
float obd2_fuel_per = -99;
int obd2_vcm_rpm = 0;
float obd2_bms_soc = 0;
float obd2_bms_volt = 0;
float obd2_bms_curr = 0;

uint32_t cmd_count = 0;
uint8_t set_canbuf[OBD2_PACKETSIZE] = {0};

String obd2_getHeader() {
  char str[32];
  sprintf(str, "ATSH%06X", cmd_tbl[cmd_count].can_id & 0x00FFFFFF); // SH ww xx yy zz を受け付けないため6桁指定にする（先頭の0x18はELM327が付与）
  return String(str);
}

String obd2_getPacket() {
  char str[32];

  if (cmd_tbl[cmd_count].can_id == CANID_STD) {
    sprintf(str, "01%02X", cmd_tbl[cmd_count].pid_code);
  } else {
    sprintf(str, "22%04X", cmd_tbl[cmd_count].pid_code);
  }

  cmd_count++;
  if (cmd_count >= (sizeof(cmd_tbl) / sizeof(cmd_tbl[0]))) cmd_count = 0;

  return String(str);
}

bool obd2_ChkResponse(uint8_t *buff) {
  uint16_t PIDcode;
  uint8_t prmA;
  uint8_t prmB;

  // Response
  if (buff[1] == 0x41) {  // show current data
    PIDcode = (uint16_t)buff[2];
    prmA = buff[3];
    prmB = buff[4];
  } else if (buff[1] == 0x62) {  // response to service 22h request
    PIDcode = ((uint16_t)buff[2] << 8) | (uint16_t)buff[3];
    prmA = buff[4];
    prmB = buff[5];
  }
  // etc
  else {
    return false;
  }

  return obd2_AnalysisPacket(PIDcode, prmA, prmB);
}

bool obd2_AnalysisPacket(uint16_t PIDcode, uint8_t prmA, uint8_t prmB) {
  union prm_data {
    struct {
      uint8_t b;
      uint8_t a;
    };
    uint16_t us_data;
    int16_t s_data;
  };

  prm_data obd2data;
  obd2data.a = prmA;
  obd2data.b = prmB;

  switch(PIDcode) {
    case PID_ENGINE_TEMP:   // 水温
      obd2_eng_temp = obd2data.a - 40;
      break;
    case PID_ENGINE_RPM:    // エンジン回転数
      obd2_eng_rpm = obd2data.us_data / 4;
      break;
    case PID_CAR_SPEED:     // 車速
      obd2_kmh = obd2data.a;
      break;
    case PID_THROTTLE_POS:  // スロットル位置
      obd2_throttle = (float)(obd2data.a) * 100 / 255;
      break;
    case PID_FUEL_LEVEL:    // 残燃料(%)
      obd2_fuel_per = (float)(obd2data.a) * 100 / 255;
      break;
    case PID_PEDAL_D_POS:   // アクセルペダル位置D
      obd2_pedal_d = (float)(obd2data.a) * 100 / 255;
      break;
    case PID_MOTOR_RPM:     // [VCM] Motor RPM
      obd2_vcm_rpm = obd2data.s_data / 2;
      break;
    case PID_BMS_SOC:       // [BMS] HV State of charge
      obd2_bms_soc = (float)(obd2data.a) / 2;
      break;
    case PID_BMS_VOLT:      // [BMS] HV Battery Voltage
      obd2_bms_volt = (float)(obd2data.us_data) / 100;
      break;
    case PID_BMS_CURR:      // [BMS] HV Battery Current
      obd2_bms_curr = (float)((int32_t)obd2data.us_data - 32000) / 20;
      break;
    default:
      return false;
  }

  return true;
}

void obd2_demo_update() {
  union prm_data {
    struct {
      uint8_t b;
      uint8_t a;
    };
    uint16_t us_data;
  };

  static uint16_t cnt = 0;
  prm_data obd2data;
  obd2data.us_data = cnt;

  // lost_packet_cnt++;

  obd2_AnalysisPacket(PID_ENGINE_TEMP, obd2data.a, obd2data.b);
  obd2_AnalysisPacket(PID_ENGINE_RPM, obd2data.a, obd2data.b);
  obd2_AnalysisPacket(PID_CAR_SPEED, obd2data.a, obd2data.b);
  obd2_AnalysisPacket(PID_THROTTLE_POS, obd2data.a, obd2data.b);
  obd2_AnalysisPacket(PID_FUEL_LEVEL, obd2data.a, obd2data.b);
  obd2_AnalysisPacket(PID_PEDAL_D_POS, obd2data.a, obd2data.b);
  obd2_AnalysisPacket(PID_MOTOR_RPM, obd2data.a, obd2data.b);
  obd2_AnalysisPacket(PID_BMS_SOC, obd2data.a, obd2data.b);
  obd2_AnalysisPacket(PID_BMS_VOLT, obd2data.a, obd2data.b);
  obd2_AnalysisPacket(PID_BMS_CURR, obd2data.a, obd2data.b);

  cnt += 97;
}
