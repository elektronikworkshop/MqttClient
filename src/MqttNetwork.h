#ifndef EW_IG_NETWORK_H
#define EW_IG_NETWORK_H

#include <MqttFlash.h>

#if defined(ARDUINO_ARCH_ESP8266)
# include <ESP8266WiFi.h>
# include <ESP8266mDNS.h>
#elif defined(ARDUINO_ARCH_ESP32)
# include <WiFi.h>
# include <ESPmDNS.h>
#else
#endif

#include <PubSubClient.h>

/* TODO: make use of wifi callbacks!
 * mDisconnectHandler = WiFi.onStationModeDisconnected(&onDisconnected);
 *
 * void onDisconnected(const WiFiEventStationModeDisconnected& event)
 *  {
 *      bla bla
 *  }
 */

class NetworkManager
{
public:
  /* If connection is lost, try to reconnect every minute */
  static const unsigned long ConnectRetryMs = 2 * 60UL * 1000UL;

  typedef void (*Callback)(void);

  typedef enum
  {
    StateDisconnected,
    StateConnecting,
    StateConnected,
  } State;

  NetworkManager(Print &print,
                FlashDataMqttClient &flashData,
          TelnetClient *telnetClients,
          size_t numTelnetClients,
          Callback connectCallback = nullptr,
          Callback disconnectCallback = nullptr)
          : m_state(StateDisconnected)
          , m_connectStartMs(0)
          , m_connectTimeoutMs(10000)
          , m_print(print)
          , m_flashData(flashData)
          , m_connectCallback(connectCallback)
          , m_disconnectCallback(disconnectCallback)
          , m_telnetServer(telnetClients, numTelnetClients)
          , m_mqttClient(m_mqttWifiClient)
  { }

  void begin()
  {
    connect();
  }

  void run()
  {
    // TODO: check if connection lost and adjust state and call notifier/event system
    // TODO: if connection lost try to reconnect every now and then

    switch (m_state) {

      case StateDisconnected:
        if (millis() - m_connectStartMs > ConnectRetryMs) {
          connect();
          break;
        }
        break;

      case StateConnecting:
        if (WiFi.status() == WL_CONNECTED) {
          m_print
            << "WiFi connected to:   " << m_flashData.wifiSsid << "\n"
            << "signal strength:     " << WiFi.RSSI() << " dB\n"
            << "IP:                  " << WiFi.localIP() << "\n"
            #if defined(ARDUINO_ARCH_ESP8266)
            << "host name:           " << WiFi.hostname() << "\n"
            #elif defined(ARDUINO_ARCH_ESP32)
            << "host name:           " << WiFi.getHostname() << "\n"
            #endif
            ;
          if (not strlen(m_flashData.mqttServer)) {
            m_print
              << "MQTT server not configured or disabled\n";
          }
          m_state = StateConnected;

          startMdns();

          if (m_connectCallback) {
            m_connectCallback();
          }
          break;
        }

        if (millis() - m_connectStartMs > m_connectTimeoutMs) {
          // Stop any pending request
          WiFi.disconnect();
          m_state = StateDisconnected;
          m_print << "WiFi failed to connect to SSID \"" << m_flashData.wifiSsid << "\" -- timeout\n";
        }
        break;

      case StateConnected:
        if (WiFi.status() != WL_CONNECTED) {
          m_print << "WiFi connection lost\n";
          m_state = StateDisconnected;
          // event...
          if (m_disconnectCallback) {
            m_disconnectCallback();
          }
          break;
        }
        /* connected processing */
        break;
    }

    m_telnetServer.run();
    if (manageMqtt()) {
      m_mqttClient.loop();
    }
  }

  void
  connect()
  {
    if (m_state != StateDisconnected) {
      return;
    }

    if (strlen(m_flashData.wifiSsid) == 0 or strlen(m_flashData.wifiPass) == 0) {
      m_print << "WiFi SSID (\"" << m_flashData.wifiSsid << "\") or password (\"" << m_flashData.wifiPass << "\") not set:\n  can not connect to network.\n  please set up your SSID and password\n";

      printVisibleNetworks(m_print);

      return;
    }

    m_connectStartMs = millis();

    // Set WiFi mode to station (as opposed to AP or AP_STA)
    WiFi.mode(WIFI_STA);
#if defined(ARDUINO_ARCH_ESP8266)
    // https://github.com/esp8266/Arduino/issues/2826
    WiFi.hostname(myHostName());
#endif
    WiFi.begin(m_flashData.wifiSsid, m_flashData.wifiPass);
#if defined(ARDUINO_ARCH_ESP32)
    WiFi.setHostname(myHostName());
#endif

    m_state = StateConnecting;
  }

  void
  disconnect()
  {
    if (m_state == StateDisconnected) {
      return;
    }

    WiFi.disconnect();
    m_state = StateDisconnected;
  }

  State
  getState() const
  {
    return m_state;
  }

  bool
  isConnected() const
  {
    return m_state == StateConnected;
  }

  static void
  printVisibleNetworks(Print& print)
  {
    byte n = WiFi.scanNetworks();
    if (n) {
      print << "visible network SSIDs:\n";
      for (int i = 0; i < n; i++) {
        print << "  " << WiFi.SSID(i) << " (" << WiFi.RSSI(i) << " dB)\n";
      }
    }
  }

  PubSubClient &
  getMqttClient()
  {
    return m_mqttClient;
  }

  const char * myHostName()
  {
    const char* hostName = m_flashData.hostName;
    if (strlen(hostName) == 0) {
      m_print << "host name with length zero detected. defaulting to \"" << DefaultHostName << "\"\n";
      hostName = DefaultHostName;
    }
    return hostName;
  }

private:
  void
  startMdns()
  {
    const char *hn = myHostName();

    if (!MDNS.begin(hn)) {
      m_print.println("Error setting up MDNS responder!");
    } else {
      MDNS.addService("telnet", "tcp", 23);
      m_print << "published telnet host name: " << hn << "\n";
    }
  }

  bool
  manageMqtt()
  {
    if (m_state != StateConnected) {
      return false;
    }

    if (m_mqttClient.connected()) {
      return true;
    }

    /* Network connected but MQTT not connected - (re-) connect ... */

    /* Limit reconnect attempts */
    const unsigned long ConnectRetryMs = 60 * 1000;
    static unsigned long lastConnectAttemptMs = millis() - 2 * ConnectRetryMs;
    if (millis() - lastConnectAttemptMs < ConnectRetryMs) {
      return false;
    }
    lastConnectAttemptMs = millis();

    if (not strlen(m_flashData.mqttServer)) {
//      Log << "MQTT server not configured\n";
      return false;
    }

    m_mqttClient.setServer(m_flashData.mqttServer, m_flashData.mqttPort);

    m_print.print("Attempting MQTT connection...");

    if (m_mqttClient.connect(m_flashData.mqttClientName,
                           m_flashData.mqttUser,
                           m_flashData.mqttPass)) {
      m_print.println("MQTT connected");
      return true;
    } else {
      m_print.print("failed, rc = ");
      m_print.print(m_mqttClient.state());
      m_print.println(", retrying later");
      return false;
    }
  }


  State m_state;
  unsigned long m_connectStartMs;
  unsigned long m_connectTimeoutMs;

  Print &m_print;
  FlashDataMqttClient &m_flashData;

  Callback m_connectCallback;
  Callback m_disconnectCallback;

  TelnetServer m_telnetServer;

  WiFiClient m_mqttWifiClient;
  PubSubClient m_mqttClient;
};



/* Check out ESP8266WiFiMulti wifiMulti
 * https://tttapa.github.io/ESP8266/Chap08%20-%20mDNS.html
 */



#endif /* EW_IG_NETWORK_H */
