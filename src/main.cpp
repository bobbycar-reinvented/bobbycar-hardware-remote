#include <Arduino.h>

// NVS
#include <ArduinoNvs.h>

// Scroll text
String firstLine = "";
String secondLine = "";
uint firstLineIndex = 0;
uint secondLineIndex = 0;
uint scrollTimer = 0;
uint scrollTimerLimit = SCROLL_TIMER_DEFAULT_LIMIT;

// Display
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C display(0x27, LCD_COLS, LCD_ROWS);

// Buttons
#include "buttons.h"
Buttons button;

// BLE
#define CONFIG_BT_NIMBLE_ROLE_PERIPHERAL_DISABLED
#include <NimBLEDevice.h>
#include <NimBLEAdvertisedDevice.h>
#include "NimBLEEddystoneURL.h"
#include "NimBLEEddystoneTLM.h"
#include "NimBLEBeacon.h"

// JSON
#include <ArduinoJson.h>

// BLE Values
float pBLEVoltages[2];
float bobbycarAvgVoltage;

float pBLETemperatures[2];

uint pBLEErrors[4];

float pBLESpeeds[4];
float bobbycarAvgSpeed;

float pBLEAmperes[4];
float bobbycarTotalCurrent;

float bobbycarTotalPower;

bool bobbycarSupportsRemoteControl = false;

// Arrays
constexpr const int NUM_BOBBYCARS = 10;
std::array<NimBLEAddress, NUM_BOBBYCARS> bobbycar_uuids;
std::array<String, NUM_BOBBYCARS> bobbycar_names;

NimBLEUUID bobbycar_service = NimBLEUUID("0335e46c-f355-4ce6-8076-017de08cee98");
NimBLEUUID pLivestats_uuid = NimBLEUUID("a48321ea-329f-4eab-a401-30e247211524");
NimBLEUUID pRemotecontrol_uuid = NimBLEUUID("4201def0-a264-43e6-946b-6b2d9612dfed");

#include "ble.h"

NimBLEScan *pBLEScan;
NimBLEClient *pClient;
NimBLERemoteCharacteristic *pLivestatsCharacteristic;
NimBLERemoteCharacteristic *pRemotecontrolCharacteristic;

// Analog Sticks
#include "analog_sticks.h"
Inputs inputs;

// Menu
#include "menu.h"
Menu menu;

void clearBobbycars()
{
  for (auto index = 0; index < NUM_BOBBYCARS; index++)
  {
    bobbycar_names[index] = "";
    bobbycar_uuids[index] = NimBLEAddress("00:00:00:00:00:00");
  }
}

int countBobbycarsFound()
{
  int count = 0;
  for (auto index = 0; index < NUM_BOBBYCARS; index++)
  {
    if (bobbycar_names[index] != "")
    {
      count++;
    }
  }
  return count;
}

void scanForDevices()
{
  clearBobbycars();
  display.clear();
  display.setCursor(0, 0);
  display.print("Scan started...");
  Serial.println("Started BLE scan");
  NimBLEScanResults foundDevices = pBLEScan->start(SCAN_TIME, false);
  pBLEScan->clearResults();
  auto count = countBobbycarsFound();
  display.clear();
  Serial.println("Finished BLE scan");
  if (count)
  {
    // Found bobbycars
    display.setCursor(0, 0);
    display.printf("    Found %i", count);
    if (count == 1)
    {
      display.setCursor(0, 1);
      display.print("   bobbycar.    ");
    }
    else
    {
      display.setCursor(0, 1);
      display.print("   bobbycars.   ");
    }
    delay(700);
    menu.switchMenu(MENU_SELECT_BOBBYCAR);
  }
  else
  {
    // No bobbycars
    display.setCursor(0, 0);
    display.print("  No bobbycars  ");
    display.setCursor(0, 1);
    display.print("      found.    ");
    delay(1500);
    menu.switchMenu(MENU_MAIN);
  }
}

String bobbycar_name_list_transform(String name, bool selected)
{
  String outstr;
  if (selected)
  {
    outstr = ">" + name;
  }
  else
  {
    outstr = name;
    if (outstr.length() > 16)
      outstr = outstr.substring(0, 15);
  }
  return outstr;
}

void showBobbycarList(uint index)
{
  firstLine = "";
  secondLine = "";
  int length = countBobbycarsFound();
  if (!length)
    return;
  uint first_index = index;
  uint second_index = (index + 1);

  if (first_index >= length)
  {
    return;
  }

  if (length == 1)
  {
    // Only show one element because only one bobbycar was found (1 bobbycar found)
    menu.set_bobby_sel_scroll(false);
    display.clear();
    auto first_index_string = bobbycar_name_list_transform(bobbycar_names[0], true);
    if (first_index_string.length() > 16)
    {
      firstLine = first_index_string;
    }
    else
    {
      display.setCursor(0, 0);
      display.print(first_index_string);
    }
  }
  else
  {
    // Scrolling is needed (2 or more bobbycars found)
    menu.set_bobby_sel_scroll(true);
    display.clear();

    auto first_index_string = bobbycar_name_list_transform(bobbycar_names[first_index], true);
    auto second_index_string = bobbycar_name_list_transform(bobbycar_names[second_index], false);

    if (first_index_string.length() > 16)
    {
      firstLine = first_index_string;
    }
    else
    {
      display.setCursor(0, 0);
      display.print(first_index_string);
    }
    if (second_index_string.length() > 16)
    {
      secondLine = second_index_string;
    }
    else
    {
      display.setCursor(0, 1);
      display.print(second_index_string);
    }
  }
}

void up()
{
  firstLineIndex = 0;
  secondLineIndex = 0;
  scrollTimer = 0;
  scrollTimerLimit = SCROLL_TIMER_DEFAULT_LIMIT;
  menu.up();
}

void down()
{
  firstLineIndex = 0;
  secondLineIndex = 0;
  scrollTimer = 0;
  scrollTimerLimit = SCROLL_TIMER_DEFAULT_LIMIT;
  menu.down();
}

void confirm()
{
  menu.confirm();
}

void back()
{
  menu.back();
}

void resetScrollDisplay()
{
  firstLine = "";
  secondLine = "";
  firstLineIndex = 0;
  secondLineIndex = 0;
  scrollTimer = 0;
  scrollTimerLimit = SCROLL_TIMER_DEFAULT_LIMIT;
}

void handleScroll()
{
  if (firstLine == "" && secondLine == "")
    return;

  if (scrollTimer < scrollTimerLimit)
  {
    scrollTimer++;
    return;
  }
  else
  {
    scrollTimer = 0;
    if (scrollTimerLimit != SCROLL_TIMER_DEFAULT_LIMIT)
      scrollTimerLimit = SCROLL_TIMER_DEFAULT_LIMIT;
  }

  if (firstLine != "")
    firstLineIndex++;
  if (secondLine != "")
    secondLineIndex++;

  if (firstLine.length() && firstLineIndex > (firstLine.length() - 17))
    scrollTimerLimit = 1200;

  if (secondLine.length() && secondLineIndex > (secondLine.length() - 17))
    scrollTimerLimit = 1200;

  if (firstLine.length() && firstLineIndex > (firstLine.length() - 16))
    firstLineIndex = 0;

  if (secondLine.length() && secondLineIndex > (secondLine.length() - 16))
    secondLineIndex = 0;

  auto firstLineLastIndex = (firstLineIndex + 15);
  auto secondLineLastIndex = (secondLineIndex + 15);

  if (firstLineLastIndex > (firstLine.length() + 1))
    firstLineLastIndex = (firstLine.length() + 1);

  if (secondLineLastIndex > (secondLine.length() + 1))
    secondLineLastIndex = (secondLine.length() + 1);

  if (firstLine != "")
  {
    display.setCursor(0, 0);
    display.print(firstLine.substring(firstLineIndex, firstLineLastIndex) + "                ");
  }

  if (secondLine != "")
  {
    display.setCursor(0, 1);
    display.print(secondLine.substring(secondLineIndex, secondLineLastIndex) + "                ");
  }
}

void livestatsCallback(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  if (pLivestatsCharacteristic->canRead())
  {
    auto str = pLivestatsCharacteristic->getValue().c_str();
    StaticJsonDocument<384> doc;
    if (const auto error = deserializeJson(doc, str))
    {
      Serial.printf("Error when deserializing: %s\n", str);
      return;
    }
    else
    {

      pBLEVoltages[0] = doc["v"][0];
      pBLEVoltages[1] = doc["v"][1];

      pBLETemperatures[0] = doc["t"][0];
      pBLETemperatures[1] = doc["t"][1];

      pBLEErrors[0] = doc["e"][0];
      pBLEErrors[1] = doc["e"][1];
      pBLEErrors[2] = doc["e"][2];
      pBLEErrors[3] = doc["e"][3];

      pBLESpeeds[0] = doc["s"][0];
      pBLESpeeds[1] = doc["s"][1];
      pBLESpeeds[2] = doc["s"][2];
      pBLESpeeds[3] = doc["s"][3];

      pBLEAmperes[0] = doc["a"][0];
      pBLEAmperes[1] = doc["a"][1];
      pBLEAmperes[2] = doc["a"][2];
      pBLEAmperes[3] = doc["a"][3];

      bobbycarAvgVoltage = ((pBLEVoltages[0] + pBLEVoltages[1]) / 2);
      bobbycarTotalCurrent = (pBLEAmperes[0] + pBLEAmperes[1] + pBLEAmperes[2] + pBLEAmperes[3]);
      bobbycarTotalPower = (bobbycarAvgVoltage * bobbycarTotalCurrent);
      bobbycarAvgSpeed = ((pBLESpeeds[0] + pBLESpeeds[1] + pBLESpeeds[2] + pBLESpeeds[3]) / 4);
    }
  }
}

void discoverStatsCharacteristic()
{
  auto *pService = pClient->getService(bobbycar_service);
  if (pService == nullptr)
  {
    display.clear();
    display.setCursor(0, 0);
    display.print("  Device is not ");
    display.setCursor(0, 1);
    display.print("   a bobbycar!  ");
    delay(1200);
    return;
  }
  else
  {
    pLivestatsCharacteristic = pService->getCharacteristic(pLivestats_uuid);
    pRemotecontrolCharacteristic = pService->getCharacteristic(pRemotecontrol_uuid);
    bobbycarSupportsRemoteControl = (pRemotecontrolCharacteristic != nullptr);
    Serial.println(bobbycarSupportsRemoteControl ? "Supports Remotecontrol" : "No remote control support");
    if (pLivestatsCharacteristic == nullptr)
    {
      return;
    }
    else
    {
      firstLine = "";
      secondLine = "";
      pLivestatsCharacteristic->subscribe(true, livestatsCallback);
    }
  }
}

void serial_print_bobbycars()
{
  Serial.println("");
  for (int index = 0; index < 10; index++)
  {
    Serial.printf("%i -> ", index);
    Serial.print(bobbycar_names[index]);
    Serial.print("   ");
    Serial.println(bobbycar_uuids[index].toString().c_str());
  }
}

void showConnected(uint index)
{
  display.setCursor(0, 0);
  display.print("Connected to");
  display.setCursor(0, 1);
  if (bobbycar_names[index].length() > 16)
  {
    resetScrollDisplay();
    secondLine = bobbycar_names[index];
  }
  else
  {
    display.print(bobbycar_names[index].c_str());
  }
}

String buildEmptyMessage()
{
  String output;
  StaticJsonDocument<128> doc;

  doc["fl"] = 0;
  doc["fr"] = 0;
  doc["bl"] = 0;
  doc["br"] = 0;
  doc["anim"] = 0;

  serializeJson(doc, output);
  return output;
}

String buildRemoteMessage()
{
  String output;
  StaticJsonDocument<128> doc;

  auto leftStickButton = inputs.getButtonValue(ANALOG_LEFT_BUTTON);
  auto rightStickButton = inputs.getButtonValue(ANALOG_RIGHT_BUTTON);

  auto fl = inputs.getWheelValue(WHEEL_FRONT_LEFT);
  auto fr = inputs.getWheelValue(WHEEL_FRONT_RIGHT);
  auto bl = inputs.getWheelValue(WHEEL_BACK_LEFT);
  auto br = inputs.getWheelValue(WHEEL_BACK_RIGHT);

  int blink = 0;

  if (leftStickButton && rightStickButton)
  {
    blink = 3;
  }
  else if (leftStickButton && !rightStickButton)
  {
    blink = 2;
  }
  else if (!leftStickButton && rightStickButton)
  {
    blink = 1;
  }

  doc["fl"] = fl;
  doc["fr"] = fr;
  doc["bl"] = bl;
  doc["br"] = br;
  doc["anim"] = blink;

  serializeJson(doc, output);
  return output;
}

void connectToBobbycar(uint index)
{
  auto name = bobbycar_names[index];
  auto uuid = bobbycar_uuids[index];

  Serial.printf("Connecting to %s...\n", name.c_str());

  if (pClient->connect(uuid))
  {
    Serial.println("Connected");
    menu.switchMenu(MENU_CONNECTED_TO_BOBBYCAR);
    discoverStatsCharacteristic();
  }
  else
  {
    Serial.println("Failed to connect");
  }
}

void setup()
{
  Serial.begin(BAUD_RATE);
  Serial.println("Booting...");

  // Init lcd display
  display.init();
  display.backlight();
  display.clear();
  display.setCursor(0, 0);
  display.print("Booting...");
  display.setCursor(0, 1);
  display.print("Bobbycar Remote");
  delay(10);

  // NVS
  display.setCursor(0, 0);
  display.print("NVS...      ");
  NVS.begin();
  int fSteer = NVS.getInt("fSteer");
  int bSteer = NVS.getInt("bSteer");
  int fDrive = NVS.getInt("fDrive");
  int bDrive = NVS.getInt("bDrive");

  if (fSteer == 0)
  {
    fSteer = 100;
    NVS.setInt("fSteer", 100);
  }
  if (bSteer == 0)
  {
    bSteer = 0;
    NVS.setInt("bSteer", 0);
  }
  if (fDrive == 0)
  {
    fDrive = 75;
    NVS.setInt("fDrive", 75);
  }
  if (bDrive == 0)
  {
    bDrive = 100;
    NVS.setInt("bDrive", 100);
  }

  // Load calibration
  int lXMCal = NVS.getInt("lXMCal"); // lXM leftXMiddle
  int lXSCal = NVS.getInt("lXSCal"); // lXS leftXStart
  int lXECal = NVS.getInt("lXECal"); // lXE leftXEnd

  int lYMCal = NVS.getInt("lYMCal");
  int lYSCal = NVS.getInt("lYSCal");
  int lYECal = NVS.getInt("lYECal");

  int rXMCal = NVS.getInt("rXMCal");
  int rXSCal = NVS.getInt("rXSCal");
  int rXECal = NVS.getInt("rXECal");

  int rYMCal = NVS.getInt("rYMCal");
  int rYSCal = NVS.getInt("rYSCal");
  int rYECal = NVS.getInt("rYECal");

  if (lXMCal == 0)
  {
    lXMCal = LEFT_ANALOG_X_MIDDLE;
    NVS.setInt("lXMCal", LEFT_ANALOG_X_MIDDLE);
  }
  if (lXSCal == 0)
  {
    lXSCal = LEFT_ANALOG_X_START;
    NVS.setInt("lXSCal", LEFT_ANALOG_X_START);
  }
  if (lXECal == 0)
  {
    lXECal = LEFT_ANALOG_X_END;
    NVS.setInt("lXECal", LEFT_ANALOG_X_END);
  }

  if (lYMCal == 0)
  {
    lYMCal = LEFT_ANALOG_Y_MIDDLE;
    NVS.setInt("lYMCal", LEFT_ANALOG_Y_MIDDLE);
  }
  if (lYSCal == 0)
  {
    lYSCal = LEFT_ANALOG_Y_START;
    NVS.setInt("lYSCal", LEFT_ANALOG_Y_START);
  }
  if (lYECal == 0)
  {
    lYECal = LEFT_ANALOG_Y_END;
    NVS.setInt("lYECal", LEFT_ANALOG_Y_END);
  }

  if (rXMCal == 0)
  {
    rXMCal = RIGHT_ANALOG_X_MIDDLE;
    NVS.setInt("rXMCal", RIGHT_ANALOG_X_MIDDLE);
  }
  if (rXSCal == 0)
  {
    rXSCal = RIGHT_ANALOG_X_START;
    NVS.setInt("rXSCal", RIGHT_ANALOG_X_START);
  }
  if (rXECal == 0)
  {
    rXECal = RIGHT_ANALOG_X_END;
    NVS.setInt("rXECal", RIGHT_ANALOG_X_END);
  }

  if (rYMCal == 0)
  {
    rYMCal = RIGHT_ANALOG_Y_MIDDLE;
    NVS.setInt("rYMCal", RIGHT_ANALOG_Y_MIDDLE);
  }
  if (rYSCal == 0)
  {
    rYSCal = RIGHT_ANALOG_Y_START;
    NVS.setInt("rYSCal", RIGHT_ANALOG_Y_START);
  }
  if (rYECal == 0)
  {
    rYECal = RIGHT_ANALOG_Y_END;
    NVS.setInt("rYECal", RIGHT_ANALOG_Y_END);
  }

  NVS.commit();

  // BLE
  display.setCursor(0, 0);
  display.print("BLE...    ");
  NimBLEDevice::init("");
  pBLEScan = NimBLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new BobbyCarBLEScanCallback());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  pClient = NimBLEDevice::createClient();

  // Buttons
  button.setPin(BUTTONS_CONFIRM, BUTTON_CONFIRM_PIN);
  button.setPin(BUTTONS_BACK, BUTTON_BACK_PIN);
  button.setPin(BUTTONS_UP, BUTTON_UP_PIN);
  button.setPin(BUTTONS_DOWN, BUTTON_DOWN_PIN);

  button.setCallback(BUTTONS_CONFIRM, confirm);
  button.setCallback(BUTTONS_BACK, back);
  button.setCallback(BUTTONS_UP, up);
  button.setCallback(BUTTONS_DOWN, down);

  button.init();

  // Analog sticks
  inputs.setPin(ANALOG_LEFT_X, LEFT_ANALOG_X_PIN);
  inputs.setPin(ANALOG_LEFT_Y, LEFT_ANALOG_Y_PIN);
  inputs.setPin(ANALOG_LEFT_BUTTON, LEFT_ANALOG_BTN_PIN);

  inputs.setPin(ANALOG_RIGHT_X, RIGHT_ANALOG_X_PIN);
  inputs.setPin(ANALOG_RIGHT_Y, RIGHT_ANALOG_Y_PIN);
  inputs.setPin(ANALOG_RIGHT_BUTTON, RIGHT_ANALOG_BTN_PIN);

  inputs.init();

  if (!digitalRead(BUTTON_CONFIRM_PIN))
  {
    display.clear();
    display.setCursor(0, 0);
    display.print(" Erasing NVS... ");
    NVS.eraseAll(true);
    NVS.commit();
    delay(1000);
    ESP.restart();
  }

  inputs.setPWMs(fSteer, bSteer, fDrive, bDrive);
  inputs.setCalibrationValues(lXMCal, lXSCal, lXECal, lYMCal, lYSCal, lYECal, rXMCal, rXSCal, rXECal, rYMCal, rYSCal, rYECal);

  // Setup done
  display.setCursor(0, 0);
  display.print("Done.     ");
  Serial.println("Done.");
  menu.switchMenu(MENU_MAIN);
}

bool nothingPressed()
{
  return !(inputs.getButtonValue(ANALOG_LEFT_BUTTON) || inputs.getButtonValue(ANALOG_RIGHT_BUTTON) || inputs.getAxisValue(ANALOG_LEFT_X) || inputs.getAxisValue(ANALOG_LEFT_Y) || inputs.getAxisValue(ANALOG_RIGHT_X) || inputs.getAxisValue(ANALOG_RIGHT_Y));
}

void calibrationScreen()
{
  display.clear();
  delay(1000);
  display.setCursor(0, 0);
  display.print(" Move left stick");
  display.setCursor(0, 1);
  display.print("   in circles   ");

  int LeftXMin = analogRead(LEFT_ANALOG_X_PIN);
  int LeftXMax = LeftXMin;
  int LeftXMiddle = LeftXMin;
  int LeftYMin = analogRead(LEFT_ANALOG_Y_PIN);
  int LeftYMax = LeftYMin;
  int LeftYMiddle = LeftYMin;

  int RightXMin = analogRead(RIGHT_ANALOG_X_PIN);
  int RightXMax = RightXMin;
  int RightXMiddle = RightXMin;
  int RightYMin = analogRead(RIGHT_ANALOG_Y_PIN);
  int RightYMax = RightYMin;
  int RightYMiddle = RightYMin;

  bool confirm_pressed = false;

  while (!confirm_pressed)
  {
    confirm_pressed = !digitalRead(BUTTON_CONFIRM_PIN);
    auto rawLeftX = analogRead(LEFT_ANALOG_X_PIN);
    auto rawLeftY = analogRead(LEFT_ANALOG_Y_PIN);

    // Max
    if (rawLeftX > LeftXMax)
      LeftXMax = rawLeftX;
    if (rawLeftY > LeftYMax)
      LeftYMax = rawLeftY;

    // Min
    if (rawLeftX < LeftXMin)
      LeftXMin = rawLeftX;
    if (rawLeftY < LeftYMin)
      LeftYMin = rawLeftY;

    display.setCursor(0, 0);
    display.printf("%i<LX<%i       ", LeftXMin, LeftXMax);

    display.setCursor(0, 1);
    display.printf("%i<LY<%i       ", LeftYMin, LeftYMax);
    delay(10);
  }
  display.clear();
  Serial.printf("lx: [%i < rawLeftX < %i] ly: [%i < rawLeftY < %i] Middle: [%i,%i]\n", LeftXMin, LeftXMax, LeftYMin, LeftYMax, LeftXMiddle, LeftYMiddle);

  confirm_pressed = false;

  delay(1000);

  while (!confirm_pressed)
  {
    confirm_pressed = !digitalRead(BUTTON_CONFIRM_PIN);
    auto rawRightX = analogRead(RIGHT_ANALOG_X_PIN);
    auto rawRightY = analogRead(RIGHT_ANALOG_Y_PIN);

    // Max
    if (rawRightX > RightXMax)
      RightXMax = rawRightX;
    if (rawRightY > RightYMax)
      RightYMax = rawRightY;

    // Min
    if (rawRightX < RightXMin)
      RightXMin = rawRightX;
    if (rawRightY < RightYMin)
      RightYMin = rawRightY;

    display.setCursor(0, 0);
    display.printf("%i<RX<%i       ", RightXMin, RightXMax);

    display.setCursor(0, 1);
    display.printf("%i<RY<%i       ", RightYMin, RightYMax);
    delay(10);
  }
  display.clear();
  Serial.printf("lx: [%i < rawRightX < %i] ly: [%i < rawRightY < %i] Middle: [%i,%i]\n", RightXMin, RightXMax, RightYMin, RightYMax, RightXMiddle, RightYMiddle);

  NVS.setInt("lXMCal", LeftXMiddle);
  NVS.setInt("lXSCal", LeftXMin);
  NVS.setInt("lXECal", LeftXMax);

  NVS.setInt("lYMCal", LeftYMiddle);
  NVS.setInt("lYSCal", LeftYMin);
  NVS.setInt("lYECal", LeftYMax);

  NVS.setInt("rXMCal", RightXMiddle);
  NVS.setInt("rXSCal", RightXMin);
  NVS.setInt("rXECal", RightXMax);

  NVS.setInt("rYMCal", RightYMiddle);
  NVS.setInt("rYSCal", RightYMin);
  NVS.setInt("rYECal", RightYMax);

  NVS.commit();

  delay(1000);

  menu.switchMenu(MENU_MAIN);
}

void loop()
{
  handleScroll();
  button.handle();
  if (menu.updateSticks())
    inputs.update();
  delayMicroseconds(500);
  if (menu.getMenu() == MENU_CONNECTED_TO_BOBBYCAR && nothingPressed())
  {
    static int timer = 0;
    if (0 < timer && timer < 100)
    {
      display.setCursor(0, 0);
      char buf1[30];
      snprintf(buf1, 30, "%.2fkm/h [%u%u%u%u]     ", bobbycarAvgSpeed, pBLEErrors[0], pBLEErrors[1], pBLEErrors[2], pBLEErrors[3]);
      display.print(String(buf1).substring(0, 15));

      display.setCursor(0, 1);
      char buf2[30];
      snprintf(buf2, 30, "%.1fW  %.1fA            ", bobbycarTotalPower, bobbycarTotalCurrent);
      display.print(String(buf2).substring(0, 15));
    }
    else if (100 < timer && timer < 200)
    {

      display.setCursor(0, 1);
      char buf2[30];
      snprintf(buf2, 30, "f:%.1fC b:%.1fC         ", pBLETemperatures[0], pBLETemperatures[1]);
      display.print(String(buf2).substring(0, 15));
    }
    else if (timer > 200)
    {
      timer = 0;
    }
    timer++;
  }
  if (menu.getMenu() == MENU_CONNECTED_TO_BOBBYCAR && bobbycarSupportsRemoteControl)
  {
    if (pRemotecontrolCharacteristic != nullptr)
    {
      if (pRemotecontrolCharacteristic->canWrite())
      {
        static bool alreadySentEmptyMessage = true;
        if (!nothingPressed())
        {
          alreadySentEmptyMessage = false;
          std::string sendToBobbycar = buildRemoteMessage().c_str();
          pRemotecontrolCharacteristic->writeValue(sendToBobbycar);
        }
        else if (alreadySentEmptyMessage == false)
        {
          alreadySentEmptyMessage = true;
          std::string emptyMessage = buildEmptyMessage().c_str();
          pRemotecontrolCharacteristic->writeValue(emptyMessage);
        }
      }
    }
  }
}