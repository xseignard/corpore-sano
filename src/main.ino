#include <Adafruit_TLC59711.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <wiring_private.h>

#include "./Bumper/Bumper.h"

#define BOARD_ID 10
// enable/disable debug through serial usb
#define DEBUG 1

#define LED 18
// enable/disable led feedback (blink for received and sent messages)
#define LED_FEEDBACK 1

//-----------
// Networking stuff
//-----------
// arduino network configuration
// TODO: increment mac adress and IP
byte mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, BOARD_ID};
IPAddress ip(2, 0, 0, BOARD_ID);
unsigned int localPort = 8888;
// server IP and port
IPAddress serverIP(2, 0, 0, 254);
unsigned int serverPort = 8889;
EthernetUDP Udp;

//-----------
// Led driver stuff (TLC59711)
//-----------
SPIClass SPI2(&sercom2, 2, 5, 4, SPI_PAD_0_SCK_3, SERCOM_RX_PAD_2);
Adafruit_TLC59711 tlc = Adafruit_TLC59711(2, &SPI2);

//-----------
// Bumpers!
//-----------
// Bumper(int id, int btnPin, int buzzerPin, Adafruit_TLC59711* ledDriver);
Bumper bumper0(0, 4, 23, 0, &tlc);
Bumper bumper1(1, 5, 24, 1, &tlc);
Bumper bumper2(2, 6, 30, 2, &tlc);
Bumper bumper3(3, 7, 31, 3, &tlc);
Bumper bumper4(4, 0, 16, 6, &tlc);
Bumper bumper5(5, 1, 15, 7, &tlc);
Bumper bumper6(6, 2, 25, 22, &tlc);
Bumper bumper7(7, 3, 19, 38, &tlc);
#define BUMPER_COUNT 8
Bumper bumpers[] = {bumper0, bumper1, bumper2, bumper3, bumper4, bumper5, bumper6, bumper7};

//-----------
// Misc
//-----------
String pressedBumpers = "P";
String releasedBumpers = "R";
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];

/**
 * Setup called when arduino is powered on.
 * Handles the ethernet connexion setup.
 */
void setup() {
  initArduino();
  blink();
}

/**
 * Check for incoming UDP messages and send some when a bumper is pressed.
 */
void loop() {
  // check for incoming UDP data
  int packetSize = Udp.parsePacket();
  if (packetSize > 0) {
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    handleCommand();
  }
  // check for bumpers state
  for (int i = 0; i < BUMPER_COUNT; i++) {
    if (bumpers[i].isPressed()) {
      pressedBumpers += bumpers[i].getId();
      pressedBumpers += "#";
    }
    if (bumpers[i].isReleased()) {
      releasedBumpers += bumpers[i].getId();
      releasedBumpers += "#";
    }
  }
  // send data only if there is some pressed/released bumpers
  if (pressedBumpers != "P") sendData(pressedBumpers);
  if (releasedBumpers != "R") sendData(releasedBumpers);
  // reset the list of pressed/released bumpers
  pressedBumpers = "P";
  releasedBumpers = "R";
}

/**
 * Handles incoming UDP commands.
 * Each command consist of the following:
 * - bumperId
 * - action: B for buzzer, C for color, D for setting debounce time
 * - actionValue:
 *  - 1 or 0 (on/off) for B
 *  - R;G;B for C, where R, G and B are 00000 to 65535
 *  - 000 to 999 for D (in ms)
 *
 * Examples:
 * 0B1 turns on buzzer 0
 * 7C65535;65535;65535 turns RGB led 7 to white
 * 3D050 turns debounce time to 50ms for button 3
 *
 * Commands length:
 * - 3 for B
 * - 19 for C
 * - 5 for D
 */
void handleCommand() {
#if LED_FEEDBACK
  digitalWrite(LED, HIGH);
#endif
  // read command
  int id = packetBuffer[0] - '0';
  char action = packetBuffer[1];
  if (action == 'B') {
    // buzzer action
    int value = packetBuffer[2] - '0';
#if DEBUG
    SerialUSB.print(bumpers[id].getId());
    SerialUSB.print(": ");
    SerialUSB.print(value);
    SerialUSB.println(" (buzzer)");
#endif
    bumpers[id].buzz(!!value);
  } else if (action == 'C') {
    // rgb action
    char tmp[5];
    tmp[0] = packetBuffer[2];
    tmp[1] = packetBuffer[3];
    tmp[2] = packetBuffer[4];
    tmp[3] = packetBuffer[5];
    tmp[4] = packetBuffer[6];
    int r = atoi(tmp);
    tmp[0] = packetBuffer[8];
    tmp[1] = packetBuffer[9];
    tmp[2] = packetBuffer[10];
    tmp[3] = packetBuffer[11];
    tmp[4] = packetBuffer[12];
    int g = atoi(tmp);
    tmp[0] = packetBuffer[14];
    tmp[1] = packetBuffer[15];
    tmp[2] = packetBuffer[16];
    tmp[3] = packetBuffer[17];
    tmp[4] = packetBuffer[18];
    int b = atoi(tmp);
#if DEBUG
    SerialUSB.print(bumpers[id].getId());
    SerialUSB.print(": ");
    SerialUSB.print(r);
    SerialUSB.print("/");
    SerialUSB.print(g);
    SerialUSB.print("/");
    SerialUSB.print(b);
    SerialUSB.println(" (rgb)");
#endif
    bumpers[id].rgb((uint16_t)r, (uint16_t)g, (uint16_t)b);
  } else if (action == 'D') {
    // debounce time
    char tmp[3];
    tmp[0] = packetBuffer[2];
    tmp[1] = packetBuffer[3];
    tmp[2] = packetBuffer[4];
    int debounce = atoi(tmp);
#if DEBUG
    SerialUSB.print(bumpers[id].getId());
    SerialUSB.print(": ");
    SerialUSB.print(debounce);
    SerialUSB.println(" (debounce time)");
#endif
    bumpers[id].setDebounce(debounce);
  }
#if LED_FEEDBACK
  digitalWrite(LED, LOW);
#endif
}

/**
 * Creates and send an UDP packet to the node.js server
 */
void sendData(String data) {
#if DEBUG
  SerialUSB.println(data);
#endif
#if LED_FEEDBACK
  digitalWrite(LED, HIGH);
#endif
  Udp.beginPacket(serverIP, serverPort);
  Udp.print(data);
  Udp.endPacket();
#if LED_FEEDBACK
  digitalWrite(LED, LOW);
#endif
}

/**
 * Init arduino
 */
void initArduino() {
  // Serial comms
#if DEBUG
  SerialUSB.begin(9600);
  delay(5000);
#endif
#if LED_FEEDBACK
  // mcu led pin
  pinMode(LED, OUTPUT);
#endif
  // led driver
  tlc.begin();
  // miso not needed, don't define it
  // pinPeripheral(2, PIO_SERCOM);
  pinPeripheral(4, PIO_SERCOM_ALT);
  pinPeripheral(5, PIO_SERCOM);
  tlc.write();
  // ethernet
  Ethernet.begin(mac, ip);
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    while (true) {
#if DEBUG
      SerialUSB.println("Cannot find the w5500...");
#endif
      delay(1000);
    }
  }
  if (Ethernet.linkStatus() != LinkON) {
    while (true) {
#if DEBUG
      SerialUSB.println("Ethernet cable seems to be not connected...");
#endif
      delay(1000);
    }
  }
  // start UDP
  Udp.begin(localPort);
}

/**
 * Blink white the bumpers
 */
void blink() {
#if LED_FEEDBACK
  digitalWrite(LED, HIGH);
#endif
  for (int i = 0; i < BUMPER_COUNT; i++) {
    bumpers[i].rgb(65535, 65535, 65535);
  }
  delay(1000);
#if LED_FEEDBACK
  digitalWrite(LED, LOW);
#endif
  for (int i = 0; i < BUMPER_COUNT; i++) {
    bumpers[i].rgb(0, 0, 0);
  }
  delay(1000);
}
