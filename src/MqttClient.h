/** MqttClient - A rapid prototyping MQTT client framework .
 *
 * Copyright (C) 2018 Elektronik Workshop <hoi@elektronikworkshop.ch>
 * http://elektronikworkshop.ch
 *
 *
 ***
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _EW_MQTT_CLIENT_H_
#define _EW_MQTT_CLIENT_H_

#include <StreamCmd.h>
#include <TelnetServer.h>
#include <MqttFlash.h>
#include <MqttNetwork.h>

template<class FlashSettingsType,
         size_t _NumCommandSets    =   2,
         size_t _MaxCommands       =  48,
         size_t _CommandBufferSize = 128,
         size_t _MaxCommandSize    =  16>
class CliMqttClient
  : public StreamCmd<_NumCommandSets,
                     _MaxCommands,
                     _CommandBufferSize,
                     _MaxCommandSize>
{
private:

protected:
  FlashSettingsType &m_flashSettings;
  NetworkManager &m_networkManager;

public:
  typedef StreamCmd<_NumCommandSets,
                     _MaxCommands,
                     _CommandBufferSize,
                     _MaxCommandSize> SCBase;
  using SCBase::next;
  using SCBase::stream;
  using SCBase::reset;
  using SCBase::current;
  using SCBase::getOpt;
  using SCBase::addCommand;
  using SCBase::setDefaultHandler;

  using SCBase::ArgOk;
  using SCBase::ArgNone;
  using SCBase::ArgInvalid;
  using SCBase::ArgTooSmall;
  using SCBase::ArgTooBig;
  using SCBase::ArgNoMatch;


  CliMqttClient(Stream& stream,
      FlashSettingsType &flashSettings,
      NetworkManager &networkManager,
      char eolChar = '\n',
      const char* prompt = NULL)
    : SCBase(stream, eolChar, prompt)
    , m_flashSettings(flashSettings)
    , m_networkManager(networkManager)
  {
    /* WARNING: Due to the static nature of StreamCmd any overflow of the command list will go unnoticed, since this object is initialized in global scope.
     */
    addCommand("help",      &CliMqttClient::cmdHelp);
    addCommand(".",         &CliMqttClient::cmdHelp);
    addCommand("n.",        &CliMqttClient::cmdHelp);

    addCommand("debug",     &CliMqttClient::cmdDebug);

    addCommand("n.rssi",    &CliMqttClient::cmdNetworkRssi);
    addCommand("n.list",    &CliMqttClient::cmdNetworkList);
    addCommand("n.ssid",    &CliMqttClient::cmdNetworkSsid);
    addCommand("n.ssidr",    &CliMqttClient::cmdNetworkSsid); /* undocumented wifi SSID reset */
    addCommand("n.pass",    &CliMqttClient::cmdNetworkPass);
    addCommand("n.passr",    &CliMqttClient::cmdNetworkPass); /* undocumented wifi pass reset */
    addCommand("n.connect", &CliMqttClient::cmdNetworkConnect);
    addCommand("n.host",    &CliMqttClient::cmdNetworkHostName);
    addCommand("n.telnet",  &CliMqttClient::cmdNetworkTelnet);
    addCommand("n.info",    &CliMqttClient::cmdNetworkInfo);

    addCommand("m.server", &CliMqttClient::cmdMqttServer);
    addCommand("m.port", &CliMqttClient::cmdMqttPort);
    addCommand("m.user", &CliMqttClient::cmdMqttUser);
    addCommand("m.pass", &CliMqttClient::cmdMqttPass);
    addCommand("m.client", &CliMqttClient::cmdMqttClient);

    setDefaultHandler(&CliMqttClient::cmdInvalid);
  }

  Print& printHex(const uint8_t *data, uint8_t len)
  {
    return printHex(stream(), data, len);
  }

  static Print& printHex(Print &s, const uint8_t *data, uint8_t len, const char* sep = " ")
  {
    for (uint8_t i = 0; i < len; i++) {
      char buf[4] = {0};
      sprintf(buf, "%02x", data[i]);
      s.print(buf);
      s.print(i == len - 1 ? "" : sep);
    }
    return s;
  }

protected:

  /* Serial command interface */

  virtual const char* helpGeneral(void)
  {
    return
    "help\n"
    "  print this help\n"
    "debug [on|off]\n"
    "  no argument: show if debug logging is enabled\n"
    "     on  enable debug logging\n"
    "    off  disable debug logging\n"
    ;
  }

  virtual const char* helpNetwork()
  {
    return
  "n.rssi\n"
  "  display the current connected network strength (RSSI)\n"
  "n.list\n"
  "  list the visible networks\n"
  "n.ssid [ssid]\n"
  "  with argument: set wifi SSID\n"
  "  without: show current wifi SSID\n"
  "n.pass [password]\n"
  "  with argument: set wifi password\n"
  "  without: show current wifi password\n"
  "n.host [hostname]\n"
  "  shows the current host name when invoked without argument\n"
  "  sets a new host name when called with argument\n"
  "n.connect\n"
  "  connect to configured wifi network\n"
  "n.telnet [params]\n"
  "  without [params] this prints the telnet configuration\n"
  "  with [params] the telnet server can be configured as follows\n"
  "    on\n"
  "      enables the telnet server\n"
  "    off\n"
  "      disables the telnet server\n"
  "    pass <pass>\n"
  "      sets the telnet login password to <password>\n"
  "n.info\n"
  " print network setup info\n"
  ;
  }

  virtual const char* helpMqtt()
  {
    return
    "m.server [server url]\n"
    "  with argument: set MQTT server\n"
    "  without: show current MQTT server\n"
    "n.port [port]\n"
    "  with argument: set MQTT server port\n"
    "  without: show current MQTT server port\n"
    "m.user [user]\n"
    "  with argument: set MQTT user name\n"
    "  without: show current MQTT user name\n"
    "m.pass [pass]\n"
    "  with argument: set MQTT user password\n"
    "  without: show current MQTT user password\n"
    "m.client [client]\n"
    "  with argument: set MQTT client name\n"
    "  without: show current MQTT client name\n"
    ;
  }

  /* TODO: add interface for customizing inherited help */
  virtual void cmdHelp()
  {
    const char* arg = current();

           if (strncmp(arg, ".", 1) == 0) {
      stream() << helpGeneral();
    } else if (strncmp(arg, "n.", 2) == 0) {
      stream() << helpNetwork();
    } else if (strncmp(arg, "m.", 2) == 0) {
      stream() << helpMqtt();
    } else {
      stream()
        << "----------------\n"
        << helpGeneral()
        << "NETWORK\n"
        << helpNetwork()
        << "----------------\n"
        ;
    }
  }

  void cmdDebug()
  {
    enum {OFF = 0, ON};
    size_t idx(0);
    switch (getOpt(idx, "off", "on")) {
      case ArgOk:
        switch (idx) {
          case ON:
            if (m_flashSettings.debug) {
              stream() << "debug logging already enabled\n";
              return;
            }
            stream() << "enabling debug logging\n";
            m_flashSettings.debug = true;
            break;
          case OFF:
            if (not m_flashSettings.debug) {
              stream() << "debug logging already disabled\n";
              return;
            }
            stream() << "disabling debug logging\n";
            m_flashSettings.debug = false;
            break;
        }
        break;
      case ArgNone:
        stream() << "debug logging " << (m_flashSettings.debug ? "en" : "dis") << "abled\n";
        return;
      default:
        stream() << "invalid arguments\n";
        return;
    }
// TODO
//    Debug.enable(m_flashSettings.debug);
    m_flashSettings.update();
  }

  void cmdVersion()
  {
    PrintVersion(stream());
  }

  // networking commands

  void cmdNetworkRssi()
  {
    stream() << "RSSI: " << WiFi.RSSI() << " dB\n";
  }

  void cmdNetworkSsid()
  {
    const char* arg;
    if (strcmp(current(), "n.ssidr") == 0) {
      arg = "";
    } else {
      arg = next();

      /* revert all strtok effects to get the plain password with spaces etc.
       * "arg" now points to the start of the password and the password contains
       * all spaces.
       */
      reset();

      if (not arg) {
        stream() << "SSID: " << m_flashSettings.wifiSsid << "\n";
        return;
      }
    }
    strncpy(m_flashSettings.wifiSsid, arg, MaxWifiSsidLen);
    m_flashSettings.update();

    stream() << "SSID \"" << arg << "\" stored to flash\n";
  }

  void cmdNetworkPass()
  {
    const char* arg;
    if (strcmp(current(), "n.passr") == 0) {
      arg = "";
    } else {
      arg = next();

      /* revert all strtok effects to get the plain password with spaces etc.
       * "arg" now points to the start of the password and the password contains
       * all spaces.
       */
      reset();

      if (not arg) {
        /* this is perhaps not a good idea, but can come in handy for now */
        stream() << "wifi password: " << m_flashSettings.wifiPass << "\n";
        return;
      }
    }
    strncpy(m_flashSettings.wifiPass, arg, MaxWifiPassLen);
    m_flashSettings.update();

    stream() << "wifi pass \"" << arg << "\" stored to flash\n";
  }

  void cmdNetworkConnect()
  {
    m_networkManager.disconnect();
    m_networkManager.connect();
  }

  void cmdNetworkList()
  {
    byte n = WiFi.scanNetworks();
    if (n) {
      stream() << "visible network SSIDs:\n";
      for (int i = 0; i < n; i++) {
        stream() << "  " << WiFi.SSID(i) << " (" << WiFi.RSSI(i) << " dB)\n";
      }
    }
  }

  void cmdNetworkHostName()
  {
    const char* arg = next();
    if (not arg) {
      stream() << "host name: " << m_flashSettings.hostName << "\n";
      return;
    }

    if (strlen(arg) == 0) {
      stream() << "the host name can not be empty\n";
      return;
    }

    strncpy(m_flashSettings.hostName, arg, MaxHostNameLen);
    m_flashSettings.update();

    stream() << "new host name \"" << arg << "\" stored to flash. restarting network...\n";

    m_networkManager.disconnect();
    m_networkManager.connect();
  }

  void cmdNetworkTelnet()
  {
    size_t idx(0);
    enum {ON = 0, OFF, PASS};
    switch (getOpt(idx, "on", "off", "pass")) {
      case ArgOk:
        switch (idx) {
          case ON:
            stream() << "telnet server ";
            if (not m_flashSettings.telnetEnabled) {
              m_flashSettings.telnetEnabled = true;
              stream() << " now enabled\n";
            } else {
              stream() << "already enabled\n";
              return;
            }
            break;
          case OFF:
            stream() << "telnet server ";
            if (m_flashSettings.telnetEnabled) {
              m_flashSettings.telnetEnabled = false;
              stream() << " now disabled\n";
            } else {
              stream() << "already disabled\n";
              return;
            }
            break;
          case PASS:
          {
            const char* pass = next();
            if (not pass or strlen(pass) == 0) {
              stream() << "password must have at least a length of " << MinTelnetPassLen << "\n";
              return;
            }
            strncpy(m_flashSettings.telnetPass, pass, MaxTelnetPassLen);
            break;
          }
        }
        break;
      case ArgNone:
        stream() << "the telnet server is " << (m_flashSettings.telnetEnabled ? "on" : "off") << ", the login password is \"" << m_flashSettings.telnetPass << "\"\n";
        return;
      default:
        stream() << "invalid argument \"" << current() << "\", see \"help\" for proper use\n";
        return;
    }

    m_flashSettings.update();
  }

  void cmdNetworkInfo()
  {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    #define confi(x) (strlen(x) ? x : "not configured")
    stream()
      << "MAC:              "; printHex(stream(), mac, sizeof(mac), ":") << "\n"
      << "SSID:             " << m_flashSettings.wifiSsid << "\n"
#if defined(ARDUINO_ARCH_ESP8266)
      << "host name:        " << WiFi.hostname() << "\n"
#elif defined(ARDUINO_ARCH_ESP32)
      << "host name:        " << WiFi.getHostname() << "\n"
#endif
      << "MQTT server:      " << confi(m_flashSettings.mqttServer) << "\n"
      << "MQTT user:        " << confi(m_flashSettings.mqttUser) << "\n"
      << "MQTT client name: " << confi(m_flashSettings.mqttClientName) << "\n"
      ;
    if (WiFi.status() == WL_CONNECTED) {
      stream()
        << "WiFi:             connected\n"
        << "signal strength:  " << WiFi.RSSI() << " dB\n"
        << "IP:               " << WiFi.localIP() << "\n"
        ;
      if (strlen(m_flashSettings.mqttServer)) {
        stream()
          << "MQTT server " << (m_networkManager.getMqttClient().connected() ? "" : "dis") << "connected" << "\n";
      }
    } else {
      stream()
        << "WiFi:             disconnected\n"
        ;
    }
  }
  // MQTT commands

  void cmdMqttServer()
  {
    const char* arg = next();
    if (not arg) {
      stream() << "MQTT server: " << m_flashSettings.mqttServer << "\n";
      return;
    }

    if (strlen(arg) == 0) {
      stream() << "the MQTT server name can not be empty\n";
      return;
    }

    strncpy(m_flashSettings.mqttServer, arg, MaxMqttServerNameLen);
    m_flashSettings.update();

    stream() << "new MQTT server name \"" << arg << "\" stored to flash. restarting mqtt...\n";

    /* TODO mqtt restart */
  }

  void cmdMqttPort()
  {
    /* TODO */
  }

  void cmdMqttUser()
  {
    const char* arg = next();

    /* revert all strtok effects to get the plain password with spaces etc.
     * "arg" now points to the start of the password and the password contains
     * all spaces.
     */
    reset();

    if (not arg) {
      stream() << "MQTT user: " << m_flashSettings.mqttUser << "\n";
      return;
    }
    strncpy(m_flashSettings.mqttUser, arg, MaxMqttUserNameLen);
    m_flashSettings.update();

    /* TODO restart MQTT */
  }

  void cmdMqttPass()
  {
    const char* arg = next();

    /* revert all strtok effects to get the plain password with spaces etc.
     * "arg" now points to the start of the password and the password contains
     * all spaces.
     */
    reset();

    if (not arg) {
      stream() << "MQTT password: " << m_flashSettings.mqttPass << "\n";
      return;
    }
    strncpy(m_flashSettings.mqttPass, arg, MaxMqttPassLen);
    m_flashSettings.update();

    /* TODO restart MQTT */
  }

  void cmdMqttClient()
  {
    const char* arg = next();

    /* revert all strtok effects to get the plain password with spaces etc.
     * "arg" now points to the start of the password and the password contains
     * all spaces.
     */
    reset();

    if (not arg) {
      stream() << "MQTT client: " << m_flashSettings.mqttClientName << "\n";
      return;
    }
    strncpy(m_flashSettings.mqttClientName, arg, MaxMqttClientNameLen);
    m_flashSettings.update();

    /* TODO restart MQTT */
  }


  void cmdInvalid(const char *command)
  {
    if (strlen(command)) {
      stream() << "what do you mean by \"" << command << "\"? try the \"help\" command\n";
    }
  }

};



#endif /* _EW_MQTT_CLIENT_H_ */
