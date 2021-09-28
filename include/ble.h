#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEAdvertisedDevice.h>
#include "NimBLEEddystoneURL.h"
#include "NimBLEEddystoneTLM.h"
#include "NimBLEBeacon.h"

void add_bobbycar(BLEAdvertisedDevice *advertisedDevice)
{
  auto new_device_name = advertisedDevice->getName().c_str();
  auto highest_index = 0;
  bool save = false;

  for (int index = 0; index < (sizeof(bobbycar_names) / sizeof(bobbycar_names[0])); index++)
  {
    if (bobbycar_names[index] == new_device_name) {
      break;
    }
    if (bobbycar_names[index] == "")
    {
      highest_index = index;
      save = true;
      break;
    }
  }

  if (save)
  {
    assert(highest_index < (sizeof(bobbycar_names) / sizeof(bobbycar_names[0])));
    bobbycar_names[highest_index] = new_device_name;
    bobbycar_uuids[highest_index] = advertisedDevice->getAddress();
  }
}

#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

class BobbyCarBLEScanCallback : public BLEAdvertisedDeviceCallbacks
{
  /*** Only a reference to the advertised device is passed now
      void onResult(BLEAdvertisedDevice advertisedDevice) { **/
  void onResult(BLEAdvertisedDevice *advertisedDevice)
  {
    if (advertisedDevice->haveName())
    {
      String name = String(advertisedDevice->getName().c_str());
      if (String(name).indexOf("bobby") > 0 && advertisedDevice->getServiceUUID() == bobbycar_service)
      {
        add_bobbycar(advertisedDevice);
      } else {
        Serial.printf("Also found %s\n", name.c_str());
      }
    }
  }
};