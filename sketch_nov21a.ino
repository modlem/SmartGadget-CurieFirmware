/******************************************************************************
Sensirion SHT-1x sensor firmware for Intel Curie
running with Smart Gadget Android and iPhone application
******************************************************************************/
#include <SHT1X.h>
#include <CurieBLE.h>

// Services
BLEPeripheral blePeripheral;       // BLE Peripheral Device (the board you're programming)
BLEService deviceInfoService("180A"); // BLE Device Information Service
BLEService batteryService("180F"); // BLE Battery Service
BLEService loggerService("0000F234-B38D-4985-720E-0F993A68EE41"); // Sensirion Logger Service (proprietary)
BLEService humidityService("00001234-B38D-4985-720E-0F993A68EE41"); // Sensirion Humidity Service (proprietary)
BLEService temperatureService("00002234-B38D-4985-720E-0F993A68EE41"); // Sensirion Temperature Service (proprietary)

// Device Information Service Characteristics
BLECharacteristic systemIdStringCharacteristic("2A23", BLERead, 8);
BLECharacteristic manufacturersNameStringCharacteristic("2A29", BLERead, 12);
BLECharacteristic modelNumberStringCharacteristic("2A24", BLERead, 11);
BLECharacteristic serialNumberStringCharacteristic("2A25", BLERead, 8);
BLECharacteristic hardwareRevisionStringCharacteristic("2A27", BLERead, 1);
BLECharacteristic firmwareRevisionStringCharacteristic("2A26", BLERead, 3);
BLECharacteristic softwareRevisionStringCharacteristic("2A28", BLERead, 3);

// Battery Service Characteristics
BLEUnsignedCharCharacteristic batteryLevelCharacteristic("2A19", BLERead | BLENotify);

// Sensirion Logger Service Characteristics
BLECharacteristic loggerServiceSyncTimeMsCharacteristic("0000F235-B38D-4985-720E-0F993A68EE41", BLEWrite, 8);
BLECharacteristic loggerServiceOldestSampleTimeMsCharacteristic("0000F236-B38D-4985-720E-0F993A68EE41", BLERead | BLEWrite, 8);
BLECharacteristic loggerServiceNewestSampleTimeMsCharacteristic("0000F237-B38D-4985-720E-0F993A68EE41", BLERead | BLEWrite, 8);
BLECharCharacteristic loggerServiceStartLoggerDownloadCharacteristic("0000F238-B38D-4985-720E-0F993A68EE41", BLEWrite | BLENotify);
BLEUnsignedIntCharacteristic loggerServiceLoggerIntervalMsCharacteristic("0000F239-B38D-4985-720E-0F993A68EE41", BLERead | BLEWrite);

// Sensirion Humidity Service Characteristics
BLECharacteristic humidityCharacteristic("00001235-B38D-4985-720E-0F993A68EE41", BLERead | BLENotify, 4);

// Sensirion Temperature Service Characteristics
BLECharacteristic temperatureCharacteristic("00002235-B38D-4985-720E-0F993A68EE41", BLERead | BLENotify, 4);

float tempC = 0;
float tempF = 0;
float humidity = 0;

// No batteries. Always 100%
const unsigned char batteryLevel = 100;

uint64_t _zero = 0;
unsigned int logInterval = 600000;

//Create an instance of the SHT1X sensor
SHT1x sht15(8, 9);//Data, SCK

void setup()
{
  Serial.begin(9600); // Open serial connection to report values to host

  blePeripheral.setLocalName("SHT-15");
  blePeripheral.setAdvertisedServiceUuid(deviceInfoService.uuid());
  blePeripheral.setAdvertisedServiceUuid(batteryService.uuid());
  blePeripheral.setAdvertisedServiceUuid(loggerService.uuid());
  blePeripheral.setAdvertisedServiceUuid(humidityService.uuid());
  blePeripheral.setAdvertisedServiceUuid(temperatureService.uuid());
  
  blePeripheral.addAttribute(deviceInfoService);
  blePeripheral.addAttribute(systemIdStringCharacteristic);
  blePeripheral.addAttribute(manufacturersNameStringCharacteristic);
  blePeripheral.addAttribute(modelNumberStringCharacteristic);
  blePeripheral.addAttribute(serialNumberStringCharacteristic);
  blePeripheral.addAttribute(hardwareRevisionStringCharacteristic);
  blePeripheral.addAttribute(firmwareRevisionStringCharacteristic);
  blePeripheral.addAttribute(softwareRevisionStringCharacteristic);
  
  blePeripheral.addAttribute(batteryService);
  blePeripheral.addAttribute(batteryLevelCharacteristic);

  blePeripheral.addAttribute(loggerService);
  blePeripheral.addAttribute(loggerServiceSyncTimeMsCharacteristic);
  blePeripheral.addAttribute(loggerServiceOldestSampleTimeMsCharacteristic);
  blePeripheral.addAttribute(loggerServiceNewestSampleTimeMsCharacteristic);
  blePeripheral.addAttribute(loggerServiceStartLoggerDownloadCharacteristic);
  blePeripheral.addAttribute(loggerServiceLoggerIntervalMsCharacteristic);
  
  blePeripheral.addAttribute(humidityService);
  blePeripheral.addAttribute(humidityCharacteristic);
  
  blePeripheral.addAttribute(temperatureService);
  blePeripheral.addAttribute(temperatureCharacteristic);

  systemIdStringCharacteristic.setValue((const unsigned char*)"00000000", 8);
  manufacturersNameStringCharacteristic.setValue((const unsigned char*)"Sensirion AG", 12);
  modelNumberStringCharacteristic.setValue((const unsigned char*)"SmartGadget", 11);
  serialNumberStringCharacteristic.setValue((const unsigned char*)"00000000", 8);
  hardwareRevisionStringCharacteristic.setValue((const unsigned char*)"1", 1);
  firmwareRevisionStringCharacteristic.setValue((const unsigned char*)"1.3", 3);
  softwareRevisionStringCharacteristic.setValue((const unsigned char*)"0.1", 3);

  batteryLevelCharacteristic.setValue(batteryLevel);

  loggerServiceSyncTimeMsCharacteristic.setValue((unsigned char*)&_zero, sizeof(uint64_t));
  loggerServiceOldestSampleTimeMsCharacteristic.setValue((unsigned char*)&_zero, sizeof(uint64_t));
  loggerServiceNewestSampleTimeMsCharacteristic.setValue((unsigned char*)&_zero, sizeof(uint64_t));
  loggerServiceStartLoggerDownloadCharacteristic.setValue(0);
  loggerServiceLoggerIntervalMsCharacteristic.setValue(logInterval);
  
  humidityCharacteristic.setValue((unsigned char*)&humidity, sizeof(float));

  temperatureCharacteristic.setValue((unsigned char*)&tempC, sizeof(float));

  blePeripheral.begin();
}
//-------------------------------------------------------------------------------------------
void loop()
{
  BLECentral central = blePeripheral.central();
  readSensor();
  //printOut();
  if (central) {
    batteryLevelCharacteristic.setValue(batteryLevel);
    humidityCharacteristic.setValue((unsigned char*)&humidity, sizeof(float));
    temperatureCharacteristic.setValue((unsigned char*)&tempC, sizeof(float));
  }
  delay(1000);
}
//-------------------------------------------------------------------------------------------
void readSensor()
{
  // Read values from the sensor
  tempC = sht15.readTemperatureC();
  tempF = sht15.readTemperatureF();
  humidity = sht15.readHumidity();  
}
//-------------------------------------------------------------------------------------------
void printOut()
{
  Serial.print(" Temp = ");
  Serial.print(tempF);
  Serial.print("F, ");
  Serial.print(tempC);
  Serial.println("C");
  Serial.print(" Humidity = ");
  Serial.print(humidity); 
  Serial.println("%");
}

