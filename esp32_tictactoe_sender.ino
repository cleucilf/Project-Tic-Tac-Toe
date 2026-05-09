#include <Key.h>
#include <Keypad.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


#define SDA_LCD 13
#define SCL_LCD 14

const char* ssid = "Cléucil";
const char* password = "12345678";
const char* mqtt_server = "cleucilcs2600.duckdns.org";

WiFiClient espClient;

PubSubClient client(espClient);

LiquidCrystal_I2C lcd(0x27, 16, 2);

char keys[4][4] = {

  {'1', '2', '3', 'A'},

  {'4', '5', '6', 'B'},

  {'7', '8', '9', 'C'},

  {'*', '0', '#', 'D'}

};

byte rowPins[4] = {33, 27, 26, 25};

byte colPins[4] = {32, 21, 22, 23};

Keypad myKeypad = Keypad(makeKeymap(keys), rowPins, colPins, 4, 4);

String currentStatus = "";

String currentBoard = "";

bool twoPlayerMode = false;

bool i2CAddrTest(uint8_t addr) {

  Wire.beginTransmission(addr);

  if (Wire.endTransmission() == 0) {

    return true;

  }

  return false;

}

void showTwoLines(String line1, String line2) {

  lcd.clear();

  lcd.setCursor(0, 0);

  lcd.print(line1.substring(0, 16));

  lcd.setCursor(0, 1);

  lcd.print(line2.substring(0, 16));

}

void showStatusOnLCD() {

  String line1 = currentStatus;

  String line2 = currentBoard;

  if (line1.length() > 16) {

    line1 = line1.substring(0, 16);

  }

  if (line2.length() > 16) {

    line2 = line2.substring(0, 16);

  }

  showTwoLines(line1, line2);

}

void callback(char* topic, byte* message, unsigned int length) {

  String msg = "";

  for (int i = 0; i < length; i++) {

    msg += (char)message[i];

  }

  String topicText = String(topic);

  if (topicText == "tictactoe/status") {

    currentStatus = msg;

    Serial.print("Status: ");

    Serial.println(msg);

  } else if (topicText == "tictactoe/board") {

    currentBoard = msg;

    Serial.print("Board: ");

    Serial.println(msg);

  } else if (topicText == "tictactoe/mode") {

    if (msg == "2") {

      twoPlayerMode = true;

    } else {

      twoPlayerMode = false;

    }

    Serial.print("Mode: ");

    Serial.println(msg);

  }

  if (!twoPlayerMode) {

    showTwoLines("Standby", "Need 2-player");

  } else if (currentStatus != "Player 2 turn") {

    showTwoLines("Standby", "Wait turn");

  } else {

    showStatusOnLCD();

  }

}

void reconnect() {

  while (!client.connected()) {

    Serial.print("Attempting MQTT connection... ");

    if (client.connect("ESP32TicTacToe")) {

      Serial.println("connected");

      client.subscribe("tictactoe/status");

      client.subscribe("tictactoe/board");

      client.subscribe("tictactoe/mode");

    } else {

      Serial.print("failed, rc=");

      Serial.print(client.state());

      Serial.println(" try again in 2 seconds");

      delay(2000);

    }

  }

}

void setup() {

  Serial.begin(115200);

  Wire.begin(SDA_LCD, SCL_LCD);

  if (!i2CAddrTest(0x27)) {

    lcd = LiquidCrystal_I2C(0x3F, 16, 2);

  }

  lcd.init();

  lcd.backlight();

  showTwoLines("Connecting WiFi", "");

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");

  }

  Serial.println();

  Serial.println("WiFi connected");

  Serial.print("ESP32 IP: ");

  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);

  client.setCallback(callback);

  reconnect();

  showTwoLines("Standby", "Wait for mode");

}

void loop() {

  char keyPressed;

  if (!client.connected()) {

    reconnect();

  }

  client.loop();

  keyPressed = myKeypad.getKey();

  if (keyPressed) {

    Serial.print("Key pressed: ");

    Serial.println(keyPressed);

    if (!twoPlayerMode) {

      showTwoLines("Standby", "Need 2-player");

      return;

    }

    if (currentStatus != "Player 2 turn") {

      showTwoLines("Standby", "Wait turn");

      return;

    }

    if (keyPressed >= '1' && keyPressed <= '9') {

      char moveText[2];

      moveText[0] = keyPressed;

      moveText[1] = '\0';

      client.publish("tictactoe/player2", moveText);

      currentStatus = "Sent move:";

      currentBoard = String(keyPressed);

      showStatusOnLCD();

    }

    if (keyPressed == '0') {

      client.publish("tictactoe/player2", "0");

      currentStatus = "Sent quit";

      currentBoard = "";

      showStatusOnLCD();

    }

  }

}