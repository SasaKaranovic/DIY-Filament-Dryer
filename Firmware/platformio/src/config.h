#ifndef _DIY_DRYBOX_CFG_H_
#define _DIY_DRYBOX_CFG_H_
// clang-format off
#define MDNS_NAME                   "drybox"
#define PIN_ONE_WIRE_BUS            18
#define PIN_HEATER_CTL              16
#define PIN_FAN_PWM                 17
#define PIN_FAN_TACHO               5
#define PIN_LED_RED                 27
#define PIN_LED_GREEN               14
#define PIN_LED_BLUE                12
#define LED_ON(x)                   do{digitalWrite(x, HIGH);}while(0)
#define LED_OFF(x)                  do{digitalWrite(x, LOW);}while(0)

#define LED_ALL_OFF()               do{digitalWrite(PIN_LED_RED, HIGH);\
                                        digitalWrite(PIN_LED_GREEN, HIGH);\
                                        digitalWrite(PIN_LED_BLUE, HIGH); }while(0)
#define LED_ALL_ON()                do{digitalWrite(PIN_LED_RED, LOW);\
                                        digitalWrite(PIN_LED_GREEN, LOW);\
                                        digitalWrite(PIN_LED_BLUE, LOW); }while(0)
#define LED_STATUS_WIFI             PIN_LED_GREEN
#define LED_STATUS_HEATER           PIN_LED_RED
#define PID_SAMPLES                 5
#define PID_TEMP_PROXIMITY          5
#define PID_KP                      65
#define PID_KI                      81
#define PID_KD                      50

#define PWM_HEATER_INVERT_VALUES    1
#define PWM_FREQ_HEATER             500
#define PWM_FREQ_FAN                20000
#define PWM_RESOLUTION              10
#define PWM_MAX_VALUE               1024L
#define HEATER_DUTY_OFF             0
#define HEATER_PWM_OFF              PWM_MAX_VALUE
#define PWM_CH_HEATER               0
#define PWM_CH_FAN                  2

#define TEMPERATURE_PRECISION       10
#define LIMIT_TEMP_IN_MIN           20
#define LIMIT_TEMP_IN_MAX           70
#define LIMIT_TEMP_HEATER_MIN       20
#define LIMIT_TEMP_HEATER_MAX       75
#define LIMIT_FAN_SPEED_MIN         0
#define LIMIT_FAN_SPEED_MAX         100

#define WIFI_CHECK_CONNECTION_MS    10000

#define DEF_USE_WEB                 1
#define DEF_DEBUG_CALIBRATE_ADC     1
#define DEF_DEBUG_PID               1
#define DEF_DEBUG_HETER_SAMPLES     1
#define DEF_DEBUG_SENSOR_SAMPLES    1
#define DEF_DEBUG_PWM_VALUES        1
#define DEF_DEUG_SPIFFS             0

// clang-format on
#endif
