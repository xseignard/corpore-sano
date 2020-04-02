#include <Adafruit_TLC59711.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <wiring_private.h>

#include "./Bumper/Bumper.h"

#define DEBUG 1

const int boardId = 2;

//-----------
// Networking stuff
//-----------
// arduino network configuration
// TODO: increment mac adress and IP
byte mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, boardId};
IPAddress ip(2, 0, 0, boardId);
unsigned int localPort = 8888;
// server IP and port
IPAddress serverIP(2, 0, 0, 254);
unsigned int serverPort = 8889;
EthernetUDP Udp;

//-----------
// Led driver stuff (TLC59711)
//-----------
SPIClass SPI2(&sercom2, 3, 5, 4, SPI_PAD_0_SCK_3, SERCOM_RX_PAD_1);
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
    handleCommands();
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
 * Each command is separated by '#'. Each command consist of the following:
 * - bumperId
 * - action: B for buzzer, C for color
 * - actionValue:
 *  - 1 or 0 (on/off) for B
 *  - R;G;B for C, where R, G and B are 00000 to 65535
 *
 * Examples:
 * 0B1 turns on buzzer 0
 * 7C65535;65535;65535 turns RGB led 7 to white
 *
 * Commands length:
 * - 3 for B
 * - 19 for C
 */
void handleCommands() {
  // read each command pair
  char* command = strtok(packetBuffer, "#");
  while (command != 0) {
    // only keep commands that have a length of 3 or 19
    // silentely ignore others
    switch (strlen(command)) {
      // on/off buzzer
      case 3: {
        int id = command[0] - '0';
        int value = command[2] - '0';
#if DEBUG
        SerialUSB.print(bumpers[id].getId());
        SerialUSB.print(": ");
        SerialUSB.print(value);
#endif
        bumpers[id].buzz(!!value);
        break;
      }
      // RGB led
      case 19: {
        int id = command[0] - '0';
        char tmp[5];
        tmp[0] = command[2];
        tmp[1] = command[3];
        tmp[2] = command[4];
        tmp[3] = command[5];
        tmp[4] = command[6];
        int r = atoi(tmp);
        tmp[0] = command[8];
        tmp[1] = command[9];
        tmp[2] = command[10];
        tmp[3] = command[11];
        tmp[4] = command[12];
        int g = atoi(tmp);
        tmp[0] = command[14];
        tmp[1] = command[15];
        tmp[2] = command[16];
        tmp[3] = command[17];
        tmp[4] = command[18];
        int b = atoi(tmp);
#if DEBUG
        SerialUSB.print(bumpers[id].getId());
        SerialUSB.print(": ");
        SerialUSB.print(r);
        SerialUSB.print("/");
        SerialUSB.print(g);
        SerialUSB.print("/");
        SerialUSB.println(b);
#endif
        bumpers[id].rgb((uint16_t)r, (uint16_t)g, (uint16_t)b);
        break;
      }
      default:
        break;
    }
    // find the next command in input string
    command = strtok(0, "#");
  }
}

/**
 * Creates and send an UDP packet to the node.js server
 */
void sendData(String data) {
#if DEBUG
  SerialUSB.println(data);
#endif
  Udp.beginPacket(serverIP, serverPort);
  Udp.print(data);
  Udp.endPacket();
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
  // led driver
  tlc.begin();
  pinPeripheral(3, PIO_SERCOM_ALT);
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
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
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
  for (int i = 0; i < BUMPER_COUNT; i++) {
    bumpers[i].rgb(65535, 65535, 65535);
  }
  delay(1000);
  for (int i = 0; i < BUMPER_COUNT; i++) {
    bumpers[i].rgb(0, 0, 0);
  }
  delay(1000);
}
