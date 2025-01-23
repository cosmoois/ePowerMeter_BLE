#pragma once

extern int obd2_eng_temp;
extern int obd2_eng_rpm;
extern int obd2_kmh;
extern float obd2_throttle;
extern float obd2_pedal_d;
extern float obd2_fuel_per;
extern int obd2_vcm_rpm;
extern float obd2_bms_soc;
extern float obd2_bms_volt;
extern float obd2_bms_curr;

#define OBD2_PACKETSIZE  8

// SAE Standard
#define PID_ENGINE_TEMP  0x05   // Engine coolant temperature
#define PID_ENGINE_RPM   0x0C   // Engine speed
#define PID_CAR_SPEED    0x0D   // Vehicle speed
#define PID_THROTTLE_POS 0x11   // Throttle position
#define PID_FUEL_LEVEL   0x2F   // Fuel Tank Level Input
#define PID_PEDAL_D_POS  0x49   // Accelerator pedal position D
// Vehicle specific
#define PID_MOTOR_RPM    0x1254 // [VCM] HV Motor RPM
#define PID_BMS_SOC      0xCF04 // [BMS] HV State of charge
#define PID_BMS_VOLT     0xCF05 // [BMS] HV Battery Voltage
#define PID_BMS_CURR     0xCF08 // [BMS] HV Battery Current

typedef struct {
  long can_id;
  uint16_t pid_code;
} st_cmdtbl;

String obd2_getHeader();
String obd2_getPacket();
bool obd2_ChkResponse(uint8_t *buff);
bool obd2_AnalysisPacket(uint16_t PIDcode, uint8_t prmA, uint8_t prmB);
void obd2_demo_update();
