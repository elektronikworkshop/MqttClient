#ifndef _EW_MQTT_CLIENT_FLASH_H_
#define _EW_MQTT_CLIENT_FLASH_H_

#include <FlashSettings.h>

#ifndef DefaultHostName
#  define DefaultHostName "mqtt-client"
#endif

#ifndef DefaultTelnetPass
#  define DefaultTelnetPass "h4ckm3"
#endif

#ifndef DefaultMqttPort
#  define DefaultMqttPort 1883
#endif

const unsigned int MaxWifiSsidLen = 63; // excluding zero termination
const unsigned int MaxWifiPassLen = 63; // excluding zero termination
const unsigned int MaxHostNameLen = 63; // excluding zero termination
const unsigned int MaxTelnetPassLen = 63; // excluding zero termination
const unsigned int MinTelnetPassLen =  5;

const unsigned int MaxMqttServerNameLen = 63;// excluding zero termination
const unsigned int MaxMqttUserNameLen = 63;// excluding zero termination
const unsigned int MaxMqttPassLen = 63; // excluding zero termination
const unsigned int MaxMqttClientNameLen = 63; // excluding zero termination

// TODO: generate default host name from MAC address
// mqtt-client-a38fb1b2 or so

struct FlashDataMqttClient
  : public FlashDataBase
{
  char wifiSsid[MaxWifiSsidLen + 1];
  char wifiPass[MaxWifiPassLen + 1];

  char hostName[MaxHostNameLen + 1];
  bool telnetEnabled;
  char telnetPass[MaxTelnetPassLen + 1];

  bool debug;

  char mqttServer[MaxMqttServerNameLen + 1];
  uint16_t mqttPort;
  char mqttUser[MaxMqttUserNameLen + 1];
  char mqttPass[MaxMqttPassLen + 1];
  char mqttClientName[MaxMqttClientNameLen + 1];


  FlashDataMqttClient()
    /* router SSID */
    : wifiSsid{""}
    /* router password */
    , wifiPass{""}

    , hostName{DefaultHostName}
    , telnetEnabled(true)
    , telnetPass{DefaultTelnetPass}

    , debug(false)

    , mqttServer{""}
    , mqttPort(DefaultMqttPort)
    , mqttUser{""}
    , mqttPass{""}
    , mqttClientName{""}

  { }
};

#endif // _EW_MQTT_CLIENT_FLASH_H_
