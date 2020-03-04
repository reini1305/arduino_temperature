// Adjust the pin layout of your display in the openGLCD library itself!
#include <openGLCD.h>
#include <TimerOne.h>
#include <EEPROMex.h>
#include <EEPROMVar.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

// Data wire is plugged into pin 10 on the Arduino
#define ONE_WIRE_BUS 10
#define DHT_BUS 11
#define DHT_TYPE DHT22

#define TEMPERATURE_PRECISION 12
#define MAX_SENSORS           5
#define EEPROM_RESET          450  // one update per hour
#define HISTORY_RESET         80   // one update every 640 seconds
#define SIZE_HISTORY          110
#define FLASH_OFFSET          0

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire           oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// Initialize DHT sensor.
DHT              dht(DHT_BUS, DHT_TYPE);

// arrays to hold device addresses
DeviceAddress    thermometer[MAX_SENSORS];

int8_t           num_sensors;
float            cal_temp = -1.1f;
float            cal_hum = 24;
float            curr_mean_temp;
float            curr_std_temp;
float            min_temp;
float            max_temp;
float            temp_history[SIZE_HISTORY];
unsigned short   humidity_history[SIZE_HISTORY];
float            curr_humidity;
float            min_humidity;
float            max_humidity;
float            feels_like;
unsigned int     curr_idx = 0;
unsigned int     history_counter = HISTORY_RESET;
int              eeprom_counter = EEPROM_RESET;
bool             update_screen = false;
char             temp_buffer[10];
int              draw_temp = 1;

gText textTemp       = gText(SIZE_HISTORY / 3, 2, SIZE_HISTORY / 3 * 2, GLCD.Bottom / 2);
gText textMax        = gText(SIZE_HISTORY / 3 * 2, 2, SIZE_HISTORY, GLCD.Bottom / 2);
gText textMin        = gText(0, 2, SIZE_HISTORY / 3, GLCD.Bottom / 2);
gText textHum        = gText(0, GLCD.Bottom / 2, SIZE_HISTORY / 2, GLCD.Bottom);
gText textFeels      = gText(SIZE_HISTORY / 2, GLCD.Bottom / 2, SIZE_HISTORY, GLCD.Bottom);
gText textTempSmall  = gText(2, GLCD.Bottom - 10, 32, GLCD.Bottom - 1);
gText textLabelUpper = gText(1, 2, SIZE_HISTORY, GLCD.Bottom / 4);
gText textLabelLower = gText(3, GLCD.Bottom / 2 + 2, SIZE_HISTORY, GLCD.Bottom / 4 * 3);

void forceUpdate(void)
{
  update_screen = true;
}

void calcMeanTemperature(void)
{
  curr_humidity = dht.readHumidity() + cal_hum;

  sensors.requestTemperatures();
  curr_mean_temp = 0.0f;
  curr_std_temp = 0.0f;
  for (uint8_t i = 0; i < num_sensors; i++)
    curr_mean_temp += sensors.getTempC(thermometer[i]);
  curr_mean_temp /= num_sensors;

  for (uint8_t i = 0; i < num_sensors; i++)
    curr_std_temp += (sensors.getTempC(thermometer[i]) - curr_mean_temp) * (sensors.getTempC(thermometer[i]) - curr_mean_temp);
  curr_mean_temp += cal_temp;
  curr_std_temp /= num_sensors;
  curr_std_temp = sqrt(curr_std_temp);

  if (curr_std_temp < 0.2)
  {
    if (min_temp > curr_mean_temp)
      min_temp = curr_mean_temp;
    if (max_temp < curr_mean_temp)
      max_temp = curr_mean_temp;
    if (min_humidity > curr_humidity)
      min_humidity = curr_humidity;
    if (max_humidity < curr_humidity)
      max_humidity = curr_humidity;
  }

  feels_like = (curr_humidity < cal_hum) ? curr_mean_temp : dht.computeHeatIndex(curr_mean_temp, curr_humidity, false);
}

void printMeanTemperature(void)
{
  Serial.print(F("Temp C: "));
  Serial.print(curr_mean_temp);
  Serial.print(F(" +- "));
  Serial.print(curr_std_temp);
  Serial.print(F(" "));
  Serial.println(curr_humidity);
}

void refreshEEPROM(void)
{
  if (--eeprom_counter <= 0) {
    EEPROM.updateFloat(FLASH_OFFSET + 0, min_temp);
    EEPROM.updateFloat(FLASH_OFFSET + sizeof(float), max_temp);
    EEPROM.updateBlock(FLASH_OFFSET + 2 * sizeof(float), temp_history, SIZE_HISTORY);
    EEPROM.updateInt(FLASH_OFFSET   + (2 + SIZE_HISTORY)*sizeof(float), curr_idx);
    const unsigned int offset = (3 + SIZE_HISTORY) * sizeof(float);
    EEPROM.updateFloat(FLASH_OFFSET + offset, min_humidity);
    EEPROM.updateFloat(FLASH_OFFSET + offset + sizeof(float), max_humidity);
    EEPROM.updateBlock(FLASH_OFFSET + offset + 2 * sizeof(float), humidity_history, SIZE_HISTORY * sizeof(unsigned short));
    eeprom_counter = EEPROM_RESET;
  }
}

void updateHistory(void)
{
  if (--history_counter == 0) {
    if (curr_std_temp < 0.2) {
      temp_history[curr_idx] = curr_mean_temp;
      humidity_history[curr_idx] = curr_humidity * 10;
      curr_idx = (curr_idx + 1) % SIZE_HISTORY;
    }
    history_counter = HISTORY_RESET;
  }
}

void convertTemperatureToString(char* tempbuffer, float temperature)
{
  sprintf(tempbuffer, "%d.%d", (int)temperature, min(9, round(((int)(temperature * 100.f) - ((int)temperature) * 100.f) / 10.f)));
}

void refreshDisplay(void)
{
  refreshEEPROM();

  calcMeanTemperature();
  updateHistory();
  float disp_temp = constrain(curr_mean_temp, min_temp, max_temp);

  GLCD.ClearScreen();

  // Draw thermometer
  GLCD.DrawCircle(128 - 6, 64 - 6, 5);
  GLCD.DrawCircle(128 - 6, 4, 3);
  GLCD.FillRect(128 - 8, 6, 5, 64 - 10, PIXEL_OFF);
  GLCD.DrawLine(128 - 9, 6, 128 - 9, 64 - 10);
  GLCD.DrawLine(128 - 3, 6, 128 - 3, 64 - 10);
  GLCD.DrawVBarGraph(GLCD.Right - 6, GLCD.Bottom - 6, 3, -(GLCD.Height - 10), 0, min_temp * 10, max_temp * 10, disp_temp * 10);

  if (draw_temp == 1) {
    // draw labels
    textLabelUpper.DrawString(F("   Min     Current    Max"), gTextfmt_left, gTextfmt_top);
    textLabelLower.DrawString(F("  Humidity       Feels Like"), gTextfmt_left, gTextfmt_top);
    // Update min/max values
    convertTemperatureToString(temp_buffer, disp_temp);
    textTemp.DrawString(temp_buffer, gTextfmt_center, gTextfmt_center);
    convertTemperatureToString(temp_buffer, max_temp);
    textMax.DrawString(temp_buffer, gTextfmt_center, gTextfmt_center);
    convertTemperatureToString(temp_buffer, min_temp);
    textMin.DrawString(temp_buffer, gTextfmt_center, gTextfmt_center);
    convertTemperatureToString(temp_buffer, feels_like);
    textFeels.DrawString(temp_buffer, gTextfmt_center, gTextfmt_bottom);
    sprintf(temp_buffer, "%d%%", (int)curr_humidity);
    textHum.DrawString(temp_buffer, gTextfmt_center, gTextfmt_bottom);

    // draw seperators
    GLCD.DrawLine(0, GLCD.Bottom / 2 - 5, SIZE_HISTORY - 1, GLCD.Bottom / 2 - 5);
    GLCD.DrawLine(SIZE_HISTORY / 3, 0, SIZE_HISTORY / 3, GLCD.Bottom / 2 - 5);
    GLCD.DrawLine(SIZE_HISTORY / 3 * 2, 0, SIZE_HISTORY / 3 * 2, GLCD.Bottom / 2 - 5);
    if (curr_mean_temp > 30)
      draw_temp = 2;
    else
      draw_temp = 0;
  } else if (draw_temp == 0) {
    // draw the history
    int y_old;
    if (temp_history[(curr_idx + 1) % SIZE_HISTORY] > 0)
      y_old = constrain((temp_history[(curr_idx + 1) % SIZE_HISTORY] - min_temp) / ((max_temp - min_temp + 0.1) / 64), 1, 63);
    else
      y_old = 0;
    for (int i = 2; i < SIZE_HISTORY; i++) {
      if (temp_history[(curr_idx + i) % SIZE_HISTORY] > 0) {
        int y = constrain((temp_history[(curr_idx + i) % SIZE_HISTORY] - min_temp) / ((max_temp - min_temp + 0.1) / 64), 1, 63);
        GLCD.DrawLine(i - 1, GLCD.Bottom - y_old, i, GLCD.Bottom - y);
        GLCD.DrawLine(i - 1, GLCD.Bottom - y_old - 1, i, GLCD.Bottom - y - 1);
        GLCD.DrawLine(i - 1, GLCD.Bottom - y_old + 1, i, GLCD.Bottom - y + 1);
        y_old = y;
      }
    }
    if (humidity_history[(curr_idx + 1) % SIZE_HISTORY] > 0)
      y_old = constrain((humidity_history[(curr_idx + 1) % SIZE_HISTORY] / 10.f - min_humidity) / ((max_humidity - min_humidity + 0.1) / 64), 1, 63);
    else
      y_old = 0;
    for (int i = 2; i < SIZE_HISTORY; i++) {
      if (humidity_history[(curr_idx + i) % SIZE_HISTORY] > 0) {
        int y = constrain((humidity_history[(curr_idx + i) % SIZE_HISTORY] / 10.f - min_humidity) / ((max_humidity - min_humidity + 0.1) / 64), 1, 63);
        GLCD.DrawLine(i - 1, GLCD.Bottom - y_old, i, GLCD.Bottom - y);
        y_old = y;
      }
    }
    // draw the grid (every hour)
    for (int x = SIZE_HISTORY - 1; x>1; x-=SIZE_HISTORY/(HISTORY_RESET * 8 / 60)){
      for (int y = GLCD.Top; y < GLCD.Bottom; y += 5)
        GLCD.SetDot(x, y, PIXEL_ON);
    }
    // draw the current temp small
    convertTemperatureToString(temp_buffer, disp_temp);
    textTempSmall.DrawString(temp_buffer, gTextfmt_left, gTextfmt_center);
    draw_temp = 1;
  } else if (draw_temp == 2) {
    GLCD.DrawBitmap(weather, GLCD.Width / 2 - 32, GLCD.Height / 2 - 32);
    // draw the current temp small
    convertTemperatureToString(temp_buffer, disp_temp);
    textTempSmall.DrawString(temp_buffer, gTextfmt_left, gTextfmt_center);
    draw_temp = 0;
  }
  // Draw surrounding rectangle
  GLCD.DrawRect(0, 0, SIZE_HISTORY, GLCD.Bottom + 1);

  update_screen = false;
}

void setup(void)
{
  GLCD.Init();
  // Show splash screen :)
  GLCD.DrawBitmap(weather, GLCD.Width / 2 - 32, GLCD.Height / 2 - 32);
  textTemp.SelectFont(Callibri15);
  textMin.SelectFont(Callibri15);
  textMax.SelectFont(Callibri15);
  textHum.SelectFont(Cooper21);
  textFeels.SelectFont(Cooper21);
  textTempSmall.SelectFont(System5x7);
  textLabelUpper.SelectFont(Callibri10);
  textLabelLower.SelectFont(Callibri10);

  // start serial port
  Serial.begin(9600);
  // Start up the library
  sensors.begin();

  dht.begin();

  // locate devices on the bus
  Serial.print(F("Locating devices..."));
  Serial.print(F("Found "));
  num_sensors = sensors.getDeviceCount();
  Serial.print(num_sensors, DEC);
  Serial.println(F(" devices."));

  // report parasite power requirements
  Serial.print(F("Parasite power is: "));
  if (sensors.isParasitePowerMode()) Serial.println(F("ON"));
  else Serial.println(F("OFF"));

  // method 1: by index
  for (uint8_t i = 0; i < num_sensors; i++) {
    sensors.getAddress(thermometer[i], i);
    //printAddress(thermometer[i]);
    // set the resolution to 12 bit
    sensors.setResolution(thermometer[i], TEMPERATURE_PRECISION);
  }

  // load history from EEPROM
  min_temp = EEPROM.readFloat(FLASH_OFFSET + 0);
  max_temp = EEPROM.readFloat(FLASH_OFFSET + sizeof(float));
  EEPROM.readBlock(FLASH_OFFSET + 2 * sizeof(float), temp_history, SIZE_HISTORY);
  curr_idx = EEPROM.readInt(FLASH_OFFSET + (2 + SIZE_HISTORY) * sizeof(float));
  const unsigned int offset = (3 + SIZE_HISTORY) * sizeof(float);
  min_humidity = EEPROM.readFloat(FLASH_OFFSET + offset);
  max_humidity = EEPROM.readFloat(FLASH_OFFSET + offset + 1 * sizeof(float));
  EEPROM.readBlock(FLASH_OFFSET + offset + 2 * sizeof(float), humidity_history, SIZE_HISTORY * sizeof(unsigned char));

  calcMeanTemperature();
  Timer1.initialize(8800000);
  Timer1.attachInterrupt(forceUpdate);
}

void loop(void)
{
  if (Serial.available() > 0)
  {
    // read the incoming byte:
    uint8_t incomingByte = Serial.read();

    if (incomingByte == 'r')
    {
      // reset min, max temperature
      max_temp = -1000;
      min_temp = 1000;
      curr_idx = SIZE_HISTORY - 1;
      for (int i = 0; i < SIZE_HISTORY; i++) {
        temp_history[i] = 0;
        humidity_history[i] = 0;
      }
      max_humidity = 0;
      min_humidity = 100;

      calcMeanTemperature();
      eeprom_counter = 1;
      refreshEEPROM();
    }
    else
      printMeanTemperature();
  }
  if (update_screen)
    refreshDisplay();
}
