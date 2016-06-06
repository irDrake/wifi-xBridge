/*
 * Configuration.c - Library for managing configuration reading and saving in the EEPROM memory
 */


/*
 * The data structure is as follow
 * 
 * 1st character is ¶ to ensure that data is valid
 * 4 next chars are for TransmitterId in uint32_t format
 * 
 * Next characters until '¬' character is the App engine address
 * Next characters until next '¬' is the hotspot wifi name (Default is "wifi-xBridge")
 * Next characters until next '¬' is the wifi password (Default none)
 * Next configs are wifi SSID and password all separated by '¬'(0xAC) character and will finish with NUL (0x00) character 
 * Wifi SSID ¬ Wifi Password ¬
 * Wifi 2 SSID ¬ Wifi 2 Password ¬
 * Wifi 3 SSID ¬ Wifi 3 Password (NUL)
 * ex:
 * ¶2g1bmyaddress.appspot.com¬wifi1¬password1¬wifi2¬password2·
 */
 
#include "Configuration.h"

const static char CONFIGURATION_SEPARATOR = '¬';
DexcomHelper Configuration::_dexcomHelper;

/*
 * Constructor
 */
Configuration::Configuration()
{
  _loaded = false;
}
/*
 * This method will save the transmitter Id to the EEPROM
 */
void Configuration::setTransmitterId(uint32_t transmitterId) {
  BridgeConfig* bridgeConfig = getBridgeConfig();
  Serial.print("setTransmitterId: ");
  Serial.print(transmitterId);
  Serial.print("\r\n");
  bridgeConfig->transmitterId = transmitterId;
  //EEPROM_writeAnything(1, transmitterId);
}

/*
 * Configuration::setAppEngineAddress
 * ----------------------------------
 * This method will save the App Engine Address
 */
void Configuration::setAppEngineAddress(String address) {
  BridgeConfig* bridgeConfig = getBridgeConfig();
  
  Serial.print("Set App engine address to: ");
  Serial.print(address);
  Serial.print("\r\n");
  bridgeConfig->appEngineAddress = address;
}

/*
 * Configuration::saveSSID
 * -----------------------
 * This method save a new SSID in EEPROM
 */
void Configuration::saveSSID(String ssidName, String ssidPassword) {
  BridgeConfig* bridgeConfig = getBridgeConfig();
  // Create wifi Data Object
  WifiData* wifiData = (WifiData*)calloc(1, sizeof(WifiData));
  // Add to saved wifi list
  bridgeConfig->wifiList->add(*wifiData);
}

/*
 * Configuration::getWifiData
 * --------------------------
 * This method will get the specified saved Wifi Data
 */
WifiData Configuration::getWifiData(int position) {
  BridgeConfig* bridgeConfig = getBridgeConfig();
  return bridgeConfig->wifiList->get(position);
}

/*
 * Configuration::getWifiCount
 * ---------------------------
 * This method will return the number of saved Wifi
 */
int Configuration::getWifiCount() {
  BridgeConfig* bridgeConfig = getBridgeConfig();
  Serial.print("Got config");
  LinkedList<WifiData>* wifiList = bridgeConfig->wifiList;
  Serial.print("Got wifi List");
  Serial.print("Size is ");
  Serial.print(bridgeConfig->wifiList->size());
  return bridgeConfig->wifiList->size();
}

/*
 * This method will get the transmitter Id from the EEPROM
 */
uint32_t Configuration::getTransmitterId() {
  uint32_t transmitterId;
  //byte value = EEPROM.read(1);
  //transmitterId = value;
  EEPROM_readAnything(1, transmitterId);
  BridgeConfig* bridgeConfig = getBridgeConfig();
  return bridgeConfig->transmitterId;
  /*
  uint32_t transmitterId;
  //byte value = EEPROM.read(1);
  //transmitterId = value;
  EEPROM_readAnything(1, transmitterId);
  return transmitterId;*/
}

/*
 * This method will get the Google App Engine Address
 */
String Configuration::getAppEngineAddress() {
  BridgeConfig* bridgeConfig = getBridgeConfig();

  Serial.print("App engine address: ");
  Serial.print(bridgeConfig->appEngineAddress);
  if(bridgeConfig->appEngineAddress.length() > 0)
  {
    return bridgeConfig->appEngineAddress;
  }
  else
  {
    return "";
  }
}

/*
 * Configuration::getBridgeConfig
 * ------------------------------
 * This method will get the bridge configuration and load it if neccesary
 */
BridgeConfig* Configuration::getBridgeConfig() {
  if(!_loaded){
    _bridgeConfig = LoadConfig();
  }
  return _bridgeConfig;
}

/*
 * Configuration::LoadConfig
 * -------------------------
 * This method will load the configuration object from EEPROM
 * returns: The bridge configuration struct
 */
BridgeConfig* Configuration::LoadConfig() {
  Serial.print("Load configuration\r\n");
  BridgeConfig* config = (BridgeConfig*)calloc(1, sizeof(BridgeConfig));// BridgeConfig();
  String eepromData;
  bool continueReading = true;
  bool separatorFound = false;
  bool readingSSID = true;
  bool appEngineRead = false;
  bool hotspotNameRead = false;
  bool hotspotPasswordRead = false;
  char firstChar = EEPROM.read(0);
  String nextSSID = "";
  String nextPassword = "";
  if (firstChar == 182) { //'¶'
    Serial.print("Configuration Valid\r\n");
    // Configuration is valid
    // Read transmitter ID
    EEPROM_readAnything(1, config->transmitterId);
    Serial.print("Transmitter ID:");
    int i = 4;
    config->appEngineAddress = "";
    config->hotSpotName = "";
    config->hotSpotPassword = "";
    while(continueReading) {
      byte newChar = EEPROM.read(i);
      Serial.print("Char: ");
      Serial.print(newChar);
      if (!(newChar == 0x00 || newChar == 255 || i == 4095)) // End of configuration
      {
        if (newChar == CONFIGURATION_SEPARATOR)
        {
          separatorFound = true;
        }
        if (separatorFound)
        {
          Serial.print("Separator found\r\n");
          if (!appEngineRead)
          {
            appEngineRead = true;
            Serial.print("App engine address:");
            Serial.print(eepromData);
            Serial.print("\r\n");
            config->appEngineAddress = eepromData;
            Serial.print("App engine variable: \r\n");
            Serial.print(config->appEngineAddress);
            Serial.print("\r\n");
          }
          else if (!hotspotNameRead)
          {
            hotspotNameRead = true;
            config->hotSpotName = eepromData;
          }
          else if (!hotspotPasswordRead)
          {
            hotspotPasswordRead = true;
            config->hotSpotPassword = eepromData;
          }
          else // Everything else is saved wifi SSID and Passwords
          {
            /*if (readingSSID)
            {
              nextSSID = eepromData;
            }
            else
            {
              nextPassword = eepromData;
              WifiData newWifi = WifiData();
              newWifi.ssid = nextSSID;
              newWifi.password = nextPassword;
              config->wifiList->add(newWifi);
            }
            readingSSID = !readingSSID;*/
          }
          eepromData = ""; // Reset data to read
        }
        else
        {
          eepromData = eepromData + char(newChar);
        }
      }
      else {
        continueReading = false;
      }
      
      i++;
    }
  }
  else
  {
    Serial.write("firstChar was: ");
    Serial.write(firstChar);
    // Configuration is invalid
    config->appEngineAddress = "";
    config->hotSpotName = "";
    config->hotSpotPassword = "";
  }
  _loaded = true;
  return config;
}

/*
 * Configuration::SaveConfig
 * -------------------------
 * This method will save the Data back to the EEPROM
 */
void Configuration::SaveConfig() {
  int position;
  if(!_loaded) {
    _bridgeConfig = LoadConfig();
  }
  Serial.print("Save configuration\r\n");
  WriteEEPROM(0, '¶');
  uint32_t transmitterId = Configuration::getTransmitterId();
  EEPROM_writeAnything(1, transmitterId);
  Serial.print("Transmitter written\r\n");
  position = 4;

  Serial.print("Transmitter App engine address position: ");
  Serial.print(position);
  Serial.print("\r\n");
  // Write App engine address
  Configuration::WriteStringToEEPROM(position, _bridgeConfig->appEngineAddress);
  position = position + _bridgeConfig->appEngineAddress.length();
  Configuration::WriteEEPROM(position , CONFIGURATION_SEPARATOR);
  position++;
  
  Serial.print("HotSpotWifi Name");
  Serial.print(position);
  Serial.print("\r\n");
  
  // now write hotspot wifi name
  Configuration::WriteStringToEEPROM(position, _bridgeConfig->hotSpotName);
  position = position + _bridgeConfig->hotSpotName.length();
  Configuration::WriteEEPROM(position , CONFIGURATION_SEPARATOR);
  position++;
  Serial.print("HotSpotWifi Password");
  Serial.print(position);
  Serial.print("\r\n");
  // now write hotspot wifi password
  Configuration::WriteStringToEEPROM(position, _bridgeConfig->hotSpotPassword);
  position = position + _bridgeConfig->hotSpotPassword.length();
  Configuration::WriteEEPROM(position , CONFIGURATION_SEPARATOR);
  position++;
  /*
  // Now write all saved wifi ssid and password
  int arrayLength = _bridgeConfig->wifiList->size();
  for(int i = 0; i < arrayLength; i ++)
  {
    WifiData wifiData = _bridgeConfig->wifiList->get(i);
    EEPROM_writeAnything(4, wifiData.ssid);
    position = position + wifiData.ssid.length();
    Configuration::WriteEEPROM(position, CONFIGURATION_SEPARATOR);
    position++;
    EEPROM_writeAnything(4, wifiData.password);
    position = position + wifiData.password.length();
    Configuration::WriteEEPROM(position, CONFIGURATION_SEPARATOR);
    position++;
  }*/
  Configuration::WriteEEPROM(position , 0x00); // NUL character at the end
  EEPROM.commit();
  Serial.print("Committed");
  _loaded = false;
  free(_bridgeConfig);
}

/*
 * Configuration::WriteStringToEEPROM
 * ----------------------------------
 * This method will save a string to the specified EEPROM position
 */
void Configuration::WriteStringToEEPROM(int position, String data)
{
  for(int i = 0; i < data.length(); i++){
    Configuration::WriteEEPROM(position + i, data.charAt(i));
  }
}

/*
 * Configuration::WriteEEPROM
 * --------------------------
 * This method will save a character to EEPROM if the current caracter is different at this position
 */
void Configuration::WriteEEPROM(int position, char data)
{
  //if(EEPROM.read(position) != data)
  //{
    EEPROM.write(position, data);
  //}
}

