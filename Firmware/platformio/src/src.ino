#include "Adafruit_Sensor.h"
#include "Adafruit_BME280.h"
#include "Adafruit_Si7021.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "WiFi.h"
#include <DallasTemperature.h>
#include <ESPmDNS.h>
#include <OneWire.h>
#include <Wire.h>
// Local import
#include "config.h"
#include "wifi_credentials.h"

OneWire oneWire(PIN_ONE_WIRE_BUS);
DallasTemperature oneWireSensors(&oneWire);
DeviceAddress heaterSensor;

AsyncWebServer server(80);
int WiFi_status = WL_IDLE_STATUS;

Adafruit_BME280 temp_sensor_in;
Adafruit_Si7021 temp_sensor_out = Adafruit_Si7021();

float temperature_samples_in[PID_SAMPLES] = {0};
float temperature_samples_heater[PID_SAMPLES] = {0};
uint8_t ts_pos = 0;
float target_temperature_in = 0.0;
float max_temperature_heater = 0.0;
float target_fan_percent = 10.0;
uint8_t box_status = 0;
float temperature_in = 0;
float temperature_out = 0;
float temperature_heater = 0;
float humidity_in = 0;
uint8_t fan_duty = 0;
uint8_t humidity_out = 0;
unsigned long wifi_tick_previous = 0;
unsigned long pid_first_millis = 0;
unsigned long pid_last_millis = 0;

void setup()
{
    Serial.begin(115200);
    Serial.println("Setting up GPIO");
    pinMode(PIN_HEATER_CTL, OUTPUT);
    pinMode(PIN_FAN_PWM, OUTPUT);

    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_BLUE, OUTPUT);

    LED_OFF(PIN_LED_RED);
    LED_OFF(PIN_LED_GREEN);
    LED_ON(PIN_LED_BLUE);

    // Heater PWM -- Make sure heater is initially OFF
    ledcSetup(PWM_CH_HEATER, PWM_FREQ_HEATER, PWM_RESOLUTION);
    ledcAttachPin(PIN_HEATER_CTL, PWM_CH_HEATER);
    ledcWrite(PWM_CH_HEATER, HEATER_PWM_OFF);

    // FAN PW -- Make sure FAN PWM is set to 0 (fan OFF)
    ledcSetup(PWM_CH_FAN, PWM_FREQ_FAN, PWM_RESOLUTION);
    ledcAttachPin(PIN_FAN_PWM, PWM_CH_FAN);
    ledcWrite(PWM_CH_FAN, 0);

    // Setup temperature and humidity oneWireSensors
    if (!temp_sensor_out.begin())
    {
        Serial.println("Can't find Si7021 sensor! (out sensor)");
        LED_ALL_OFF();
        while (1)
        {
            LED_ON(PIN_LED_BLUE);
            delay(250);
            LED_OFF(PIN_LED_BLUE);
            delay(250);
        }
    }

    Wire.setClock(10000);
    if (!temp_sensor_in.begin())
    {
        Serial.println("Can't find SHT31! (in sensor)");
        LED_ALL_OFF();
        while (1)
        {
            LED_ON(PIN_LED_GREEN);
            delay(250);
            LED_OFF(PIN_LED_GREEN);
            delay(250);
        }
    }

    oneWireSensors.begin();
    if (!oneWireSensors.getAddress(heaterSensor, 0))
    {
        Serial.println("Unable to find address for Device 0 (heater sensor)");
        while (1)
        {
            LED_ON(PIN_LED_RED);
            delay(250);
            LED_OFF(PIN_LED_RED);
            delay(250);
        }
    }

    // set the resolution to 9 bit (Each Dallas/Maxim device is capable of
    // several different resolutions)
    oneWireSensors.setResolution(heaterSensor, TEMPERATURE_PRECISION);

#if DEF_USE_WEB
    delay(1000);

    // Connect to WiFi
    Serial.print("Connecting to SSID: ");
    Serial.println(ssid);
    while (WiFi_status != WL_CONNECTED)
    {
        Serial.print(".");
        WiFi_status = WiFi.begin(ssid, password);
        // wait 5 seconds and check again
        delay(5000);
    }
    Serial.println("");

    if (!MDNS.begin(MDNS_NAME))
    {
        Serial.println("Error starting mDNS");
    }
    else
    {
        Serial.println((String) "mDNS http://" + MDNS_NAME + ".local");
    }

    // Initialize SPIFFS
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        while (1)
        {
            LED_ON(PIN_LED_RED);
            delay(500);
            LED_OFF(PIN_LED_RED);
            delay(500);
        }
    }
    Serial.print("WiFi IP: ");
    Serial.println(WiFi.localIP());

    setupWebServer();
    server.begin();

#if DEF_DEUG_SPIFFS
    debug_spiffs_files();
#endif

#endif
    Serial.println("Ready to go.");
    LED_OFF(PIN_LED_BLUE);
}

//
//   ** Main loop **
//
void loop()
{
    // Sample temperature and humidity from all available sensors
    if (!readHeaterTemperature(&temperature_heater))
    {
        // Handle heater sensor error
        Serial.println("Failed to read heater temperature!");
    }
    sample_sens_in_and_out();

// Debug
#if DEF_DEBUG_HETER_SAMPLES
    Serial.println((String)temperature_in + "\t" + temperature_heater + "\t" +
                   target_temperature_in);
#endif

    // Check if we have WiFi connection, if not try to reconnect
    check_wifi_connection();

    // Dry box is active
    if (box_status)
    {
        LED_ON(LED_STATUS_HEATER);
        // Sample temperature values
        if (sample_temperatures(temperature_in, temperature_heater))
        {
            // Recalculate PWM value for the heater
            heater_recalc_pwm();
        }
    }
    // Drybox is off. Turn/Keep off the heater
    else
    {
        LED_OFF(LED_STATUS_HEATER);
        set_heater_duty(HEATER_DUTY_OFF);
    }
}

void debug_spiffs_files(void)
{
    File root = SPIFFS.open("/");
    File file = root.openNextFile();

    while (file)
    {
        Serial.print("FILE: ");
        Serial.println(file.name());
        file = root.openNextFile();
    }
}

void sample_sens_in_and_out(void)
{
    float tmp_temp = 0.0;
    float tmp_humid = 0.0;
    uint8_t i = 5;

    while (i--)
    {
        tmp_temp = temp_sensor_out.readTemperature();

        // delay(5);
        tmp_humid = temp_sensor_out.readHumidity();
        if (!isnan(tmp_temp) && !isnan(tmp_humid))
        {
            break;
        }
        delay(50);
    }

    // Make sure final values are valid
    if (!isnan(tmp_temp) && !isnan(tmp_humid))
    {
        temperature_out = tmp_temp;
        humidity_out = tmp_humid;
#if DEF_DEBUG_SENSOR_SAMPLES
        Serial.println((String) "TempOut:" + temperature_out + " - HumidOut:" + humidity_out);
#endif
    }
    // Otherwise we have a problem
    else
    {
        Serial.println("Out sensor: I2C error!");
    }

    // Read SHT temperature and humidity
    while (i--)
    {
        tmp_temp = temp_sensor_in.readTemperature();
        // delay(5);
        tmp_humid = temp_sensor_in.readHumidity();
        if (!isnan(tmp_temp) && !isnan(tmp_humid))
        {
            break;
        }
        delay(50);
    }

    // Make sure final values are valid
    if (!isnan(tmp_temp) && !isnan(tmp_humid))
    {
        temperature_in = tmp_temp;
        humidity_in = tmp_humid;
#if DEF_DEBUG_SENSOR_SAMPLES
        Serial.println((String) "TempIn:" + temperature_in + " - HumidIn:" + humidity_in);
#endif
    }
    // Otherwise we have a problem
    else
    {
        Serial.println("In sensor: I2C error!");
    }
}

bool readHeaterTemperature(float *fpTemp)
{
    float tempC;
    uint8_t i = 5;

    while (i--)
    {
        oneWireSensors.requestTemperatures();
        delay(20);
        tempC = oneWireSensors.getTempC(heaterSensor);
        if (tempC != DEVICE_DISCONNECTED_C)
        {
            *fpTemp = tempC;
#if DEF_DEBUG_SENSOR_SAMPLES
            Serial.print("Temp C: ");
            Serial.println(tempC);
#endif
            return true;
        }
        else
        {
#if DEF_DEBUG_SENSOR_SAMPLES
            Serial.println((String) "Heater sensor fault: " + tempC);
#endif
        }
        delay(20);
    }

    return false;
}

void set_fan_duty(uint8_t duty)
{
    if (duty > 100)
    {
        duty = 100;
    }

    fan_duty = duty;
    // changed code here
    uint32_t pwm_raw_fan = ((PWM_MAX_VALUE * (duty)) / 100.0);
#if DEF_DEBUG_PWM_VALUES
    Serial.println((String) "Setting fan to " + pwm_raw_fan);
#endif
    ledcWrite(PWM_CH_FAN, pwm_raw_fan);
}

void set_heater_duty(uint8_t duty)
{
    uint32_t pwm_raw_heater = 0;
    if (duty > 100)
    {
        duty = 100;
    }

    pwm_raw_heater = ((PWM_MAX_VALUE * duty) / 100.0);

#if DEF_DEBUG_PWM_VALUES
    Serial.println((String) "Setting heater duty " + duty + "%");
#endif
    set_heater_pwm(pwm_raw_heater);
}

void set_heater_pwm(uint32_t pwm)
{
    if (pwm > PWM_MAX_VALUE)
    {
        pwm = PWM_MAX_VALUE;
    }

#if PWM_HEATER_INVERT_VALUES
    pwm = PWM_MAX_VALUE - pwm;
#endif
#if DEF_DEBUG_PWM_VALUES
    Serial.println((String) "set_heater_duty: " + pwm + "/" + PWM_MAX_VALUE);
#endif
    ledcWrite(PWM_CH_HEATER, pwm);
}

bool sample_temperatures(float in, float heater)
{
    temperature_samples_in[ts_pos] = in;
    temperature_samples_heater[ts_pos] = heater;

    ts_pos++;
    if (ts_pos == 0)
    {
        // Record milliseconds of our first sample
        pid_first_millis = millis();
    }
    if (ts_pos >= PID_SAMPLES)
    {
        pid_last_millis = millis();
        ts_pos = 0;
        return true;
    }
    else
    {
        return false;
    }
}

void heater_recalc_pwm(void)
{
    // We are using a simple PID-like control loop to calculate PWM value for a
    // heater Heater is heating up our box to a target temperature and keeping
    // it steady after/if that temperature is reached. At the same time we have
    // to ensure heater does not go beyond our "safe" temperature and start
    // damaging itself/fillament/cables/enclosure etc.
    //
    // As a simple solution, we will use the following flow
    // 1. Check if heater temperature is higher or equal to the maximum set
    // heater temperature
    //    - if True, use heater max temperature as the target for our PID
    // controller
    //    - if False, use enclosure maximum temperature as the target for
    // our PID controller

    // changed code here
    if ((temperature_in >= target_temperature_in) || (temperature_heater >= max_temperature_heater)){
        set_heater_pwm(0);
        return;
    }

    float average = 0;
    float pid_del_p;
    float pid_del_i;
    float pid_del_d;
    int32_t pid_val_p;
    int32_t pid_val_i;
    int32_t pid_val_d;
    int32_t pwm_val;
    float pid_target_temperature = 0.0;
    float pid_temperature = 0.0;
    float pid_temperature_previous = 0.0;
    unsigned long pid_time_diff;

    // Make sure values are valid
    if (isnan(temperature_heater))
    {
        Serial.println("Invalid temperature values");
        Serial.println((String) "temp_heater:" + temperature_heater);
        Serial.println((String) "target_temperature_in:" + target_temperature_in);
        set_heater_duty(HEATER_DUTY_OFF);
        return;
    }

    // Inside temperature has NOT reached the target temperature
    // However, heater temperature is approaching it's maximum allowed
    // temperature. Then, regulate heater max temperature
    if ((temperature_heater >= (max_temperature_heater - PID_TEMP_PROXIMITY)) &&
        (temperature_in < target_temperature_in))
    {
        Serial.println("Using HEATER temperature as target.");
        for (uint8_t i = 0; i < PID_SAMPLES; i++)
        {
            average += temperature_samples_heater[i];
        }
        average = average / PID_SAMPLES;

        pid_target_temperature = max_temperature_heater;
        pid_temperature = temperature_heater;
        pid_temperature_previous = temperature_samples_heater[PID_SAMPLES - 1];
    }
    // Heater is not close to it's maximum allowed temperature
    // Or, inside temperature has already reached the target value
    else
    {
        Serial.println("Using IN temperature as target.");
        for (uint8_t i = 0; i < PID_SAMPLES; i++)
        {
            average += temperature_samples_in[i];
        }
        average = average / PID_SAMPLES;

        pid_target_temperature = target_temperature_in;
        pid_temperature = temperature_in;
        pid_temperature_previous = temperature_samples_in[PID_SAMPLES - 1];
    }

    // P
    pid_del_p = pid_target_temperature - pid_temperature;
    pid_val_p = (uint32_t)(pid_del_p)*PID_KP;

    // I
    pid_del_i = pid_target_temperature - average;
    pid_val_i = (uint32_t)(pid_del_i)*PID_KI;

    // D
    pid_del_d = pid_target_temperature - pid_temperature_previous;
    pid_time_diff = pid_last_millis - pid_first_millis;
    if (pid_time_diff > 0)
    {
        pid_val_d = (uint32_t)(pid_del_d) / pid_time_diff;
        pid_val_d = (uint32_t)(pid_val_d)*PID_KD;
    }
    else
    {
        pid_val_d = 0;
    }

    // P + I + D
    pwm_val = pid_val_p + pid_val_i + pid_val_d;

#if DEF_DEBUG_PID
    Serial.println((String) "Pd: " + pid_del_p);
    Serial.println((String) "Pv: " + pid_val_p);
    // Serial.println((String)"Ia: "+ sum);
    Serial.println((String) "Id: " + pid_del_i);
    Serial.println((String) "Iv: " + pid_val_i);
    Serial.println((String) "Dd: " + pid_del_d);
    Serial.println((String) "Dv: " + pid_val_d);
    Serial.println((String) "Calc PWM val: " + pwm_val);
#endif

    if (pwm_val > PWM_MAX_VALUE)
    {
        pwm_val = PWM_MAX_VALUE;
    }
    else if (pwm_val < 0)
    {
        pwm_val = 0;
    }

#if DEF_DEBUG_PID
    Serial.println((String) "New PWM val: " + pwm_val);
#endif
    set_heater_pwm(pwm_val);
}

void setupWebServer(void)
{
    server.onNotFound([](AsyncWebServerRequest *request)
                      {
        Serial.println("404:");
        Serial.println(request->url());
        request->send(404); });

    // // send a file when /index is requested
    server.on("/index.html", HTTP_ANY, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.html"); });

    // send a file when /index is requested
    server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.html"); });

    server.serveStatic("/img/", SPIFFS, "/img/");
    server.serveStatic("/css/", SPIFFS, "/css/");
    server.serveStatic("/js/", SPIFFS, "/js/");
    server.serveStatic("/webfonts/", SPIFFS, "/webfonts/");

    // Get dry box status
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        char buff[200] = {0};
        int len;
        len =
            snprintf(buff, 200,
                     "{\"status\":%d, \"target_temp_in\":%f, "
                     "\"max_temp_heater\":%f, "
                     "\"temp_in\":%f, \"temp_heater\":%f, \"humid_in\":%f, "
                     "\"fan_speed\":%d, \"temp_out\":%f, \"humid_out\":%d}",
                     box_status, target_temperature_in, max_temperature_heater,
                     temperature_in, temperature_heater, humidity_in, fan_duty,
                     temperature_out, humidity_out);

        if (len)
        {
            request->send(200, "text/plain", buff);
        }
        else
        {
            request->send(500, "text/plain", "{\"status\": \"Internal server error\"}");
        } });

    // Turn OFF dry box
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        box_status = 0;
        set_fan_duty(0);
        set_heater_duty(HEATER_DUTY_OFF);
        target_temperature_in = 0;
        max_temperature_heater = 0;
        request->send(200, "text/plain", "{\"status\": \"OK\"}"); });

    // Turn ON dry box and set target temperature, max heater temperature and fan speed
    server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        // Check if temperature, heater temperature and fan speed arguments are present
        if (request->hasParam("temperature") && request->hasParam("heater") &&
            request->hasParam("fanspeed"))
        {
            String str_temperature;
            String str_heater;
            String str_fanspeed;
            int32_t temperature = 0;
            int32_t heater = 0;
            int32_t fanspeed = 0;

            str_temperature = request->getParam("temperature")->value();
            temperature = str_temperature.toInt();

            str_heater = request->getParam("heater")->value();
            heater = str_heater.toInt();

            str_fanspeed = request->getParam("fanspeed")->value();
            fanspeed = str_fanspeed.toInt();

#if DEF_DEUG_WEB_API
            Serial.println((String) "Target temp: " + temperature + "C Heater: " + heater + "C Fan Speed: " + fanspeed);
#endif

            // Check drybox temperature and heater temperature limit
            if ( (temperature <= LIMIT_TEMP_IN_MAX) && (heater <= LIMIT_TEMP_HEATER_MAX) )
            {
                target_temperature_in = temperature;
                max_temperature_heater = heater;
                set_fan_duty(fanspeed);
                box_status = 1;
                request->send(200, "text/plain", "{\"status\": \"OK\"}");
                return;
            }
            // If either of them are out of range, return bad request status
            else
            {
                request->send(400, "text/plain", "{\"status\": \"Bad request. Limit error!\"}");
                return;
            }

        }
        // Not all arguments are present in the request
        else
        {
            request->send(400, "text/plain", "{\"status\": \"Bad request\"}");
            return;
        } });
}

void check_wifi_connection(void)
{
    unsigned long currentMillis = millis();

    // Check WiFi status every WIFI_CHECK_CONNECTION_MS miliseconds
    if ((currentMillis - wifi_tick_previous) >= WIFI_CHECK_CONNECTION_MS)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            LED_ON(LED_STATUS_WIFI);
            LED_OFF(PIN_LED_BLUE);
        }
        else
        {
            LED_OFF(LED_STATUS_WIFI);
            LED_ON(PIN_LED_BLUE);
            Serial.println("Reconnecting to WiFi...");
            WiFi.disconnect();
            WiFi.reconnect();
        }

        wifi_tick_previous = currentMillis;
    }
}
