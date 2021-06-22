#include <WiFi.h>
#include <WebServer.h>
#include <HardwareSerial.h>

#define TX2 17
#define RX2 16

/* Nome e Password hotspot creato */
const char* ssid = "Casa-Domodotica";  // Enter SSID here
const char* password = "12345678";  //Enter Password here

/* Indirizzo IP ESP32 */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

/* Funzioni */
void sendUpdatedPage(bool commandSuccess);
bool readStatus(int state[]);
bool checkSuccessResponse();

/* Variabile per tenere lo stato letto da arduino 
 * Contiene in ordine, temperatura, umidita e stato allarme
 */
int state[3] = {0, 0, 0};

/* Server web */
WebServer server(80);

/* Seriale per comunicare con arduino */
HardwareSerial Arduino(2);

void setup() {
  Serial.begin(115200);
  // inizializza seriale con arduino
  Arduino.begin(38400, SERIAL_8N1, RX2, TX2);

  // crea un hotspot
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  
  server.on("/", handleOnConnect);
  server.on("/led1", [](){ handleRoomLight(1); });
  server.on("/led2", [](){ handleRoomLight(2); });
  server.on("/led3", [](){ handleRoomLight(3); });
  server.on("/led4", [](){ handleRoomLight(4); });
  server.on("/led5", [](){ handleRoomLight(5); });
  server.on("/led6", [](){ handleRoomLight(6); });
  server.on("/cancello", handleCancello);
  server.on("/toggle-alarm", handleToggleAlarm);
  server.begin();
}

void loop() {
  // il loop esegue solamente la pagina web
  server.handleClient();
}

/** Funzioni chiamate dalla pagina web **/

/* Chiamata dalla pagina iniziale */
void handleOnConnect() {
   sendUpdatedPage(false);
}

/* Chiamata ogni volta che si preme su un bottone
 * per la luce. LightIndex è l'indice della luce (0, 1, 2..)
 */
void handleRoomLight(int lightIndex) {
  // invia il comando ad arudino per accendere le luci (0, 1, 2, ...)
  Arduino.println(lightIndex);
  Arduino.flush();
  bool success = checkSuccessResponse();
  sendUpdatedPage(success);
}

/* Chiamata per attivare/ disattivare l'allarme */
void handleToggleAlarm() {
  // invia il comando 7 ad arduino per attivare/ disattivare l'allarme
  Arduino.println(7);
  Arduino.flush();
  bool success = checkSuccessResponse();
  sendUpdatedPage(success);
}

/* Chiamata per aprire/ chiudere cancello */
void handleCancello() {
  Arduino.println(8);
  Arduino.flush();
  bool success = checkSuccessResponse();
  sendUpdatedPage(success);
}

/* Chiede e legge lo stato da Arduino
 * e.s. temperature, humidita, stato allarme
 */
bool readStatus(int state[]) {
  Arduino.println(10); // invia il comando 10 ad arduino per leggere lo stato
  Arduino.flush();
  delay(50);
  bool success = checkSuccessResponse();
  if(success) {
    // nel vettore state in posizione 0 -> temperatura
    state[0] = Arduino.parseInt();
    // in posizione 1 -> umidita
    state[1] = Arduino.parseInt();
    // in posizione 2 -> stato allarme
    state[2] = Arduino.parseInt();
    return true;
  }
  return false;
}

/* Controlla se Arduino risponde con successo al comando
 * ritorna true se risponde con successo, false altrimenti
 */
bool checkSuccessResponse() {
  /*int readed = -1;
  if(Arduino.available() > 0) {
    while(readed <= 0) {
      // read the incoming byte:
      readed = Arduino.parseInt();
    }
  }
  return (readed == 1) ? true : false;*/
  return true;
}

/** Converte lo stato di allarme (0, 1, 2)
 *  in stringa (no, inserito, in allarme)
 */
String alarmToString(int state[]) {
  // state[2] prende il valore dello stato dell'allarme che puo essere 0, 1 o 2
  if(state[2] == 0) {
    return "No";
  } else if(state[2] == 1) {
    return "Inserito";
  } else {
    return "In Allarme";
  }
}

/** Invia la pagina web ai dispostivi collegati (es. computer, telefono) */
void sendUpdatedPage(bool commandSuccess) {
  delay(100);
  readStatus(state);
  server.send(200, "text/html", generateHtml(commandSuccess, state));
}

/** Genera la pagina web(HTML) con bottoni e stato */
String generateHtml(bool lastCommandSuccess, int state[]) {
  String commandResult = (lastCommandSuccess) ? 
                          "<p class=\"command-result\">Ok</p>\n" :
                          "<p class=\"command-result hidden\"></p>\n";
  String temperatureTxt = (String)"<td>" + state[0] + "</td>"; // scrive sulla pagine la temeperatura
  String humidityTxt = (String)"<td>" + state[1] + "</td>"; // scriva sulla pagina l'umdita
  String alarmTxt = (String)"<td>" + alarmToString(state) + "</td>"; // scrive sulla pagine lo stato allarme

  // crea un bottone con scritto Inserisci o Disattiva in base allo stato attuale
  String alarmButton = (state[2] == 0) ? "Inserisci" : "Disattiva"; 
  
  String str = "<!DOCTYPE html>\n"
    "<html>\n"
    "\n"
    "    <head>\n"
    "        <title>Casa Domotica</title>\n"
    "    </head>\n"
    "    <body>\n"
    "<div>"
    "    <table>"
    "        <tbody>"
    "            <tr>"
    "                <td>Temperature</td>" + 
                     temperatureTxt +
    "            </tr>"
    "            <tr>"
    "                 <td>Umidita</td>" +
                      humidityTxt +
    "            </tr>"
    "            <tr>"
    "                 <td>Stato allarme</td>" +
                      alarmTxt + 
    "            </tr>"
    "        </tbody>"
    "    </table>"
    "</div>" +
    commandResult +
    "        <a href=\"/led1\"><button >Stanza 1</button></a>\n"
    "        <a href=\"/led2\"><button >Stanza 2</button></a>\n"
    "        <a href=\"/led3\"><button >Stanza 3</button></a>\n"
    "        <a href=\"/led4\"><button >Stanza 4</button></a>\n"
    "        <a href=\"/led5\"><button >Stanza 5</button></a>\n"
    "        <a href=\"/led6\"><button >Stanza 6</button></a>\n"
    "        <a href=\"/cancello\"><button >Cancello</button></a>\n"
    "        <a href=\"/toggle-alarm\"><button>" + alarmButton + "</button></a>\n"
    "    </body>\n"
    "    <style>\n"
    "        .hidden {\n"
    "            display: none;\n"
    "            visibility: hidden;\n"
    "        }\n"
    "        .command-result {\n"
    "            border: 1px solid black;\n"
    "        }\n"
    "    </style>\n"
    "</html>";
  return str;
}
