
#define ONBOARD_LED  2

// Motor A Pins
int enable1Pin = 25;
int motor1Pin1 = 26;
int motor1Pin2 = 27;

//Motor B Pins
int motor2Pin1 = 14;
int motor2Pin2 = 12;
int enable2Pin = 18;

// Setting PWM properties
const int freq = 30000;
const int pwmChannel1 = 0;
const int pwmChannel2 = 1;
const int resolution = 8;
int dutyCycle = 255;                   // dutyCycle set to MAX speed
float diffRatio = 0.90;


#include <esp_now.h>
// REPLACE WITH THE MAC Address of your receiver
uint8_t broadcastAddress[] = {0x78, 0xE3, 0x6D, 0x11, 0xF8, 0xF0}; // MTC demo car
// int to be sent
int alarmState = 0;

int simonSolved = -1;
void onReceiveData(const uint8_t *mac, const uint8_t *data, int len) {
  Serial.println("Received from MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5)Serial.print(":");
  }

  int * messagePointer = (int*)data;
  Serial.print("-----> Received: ");
  Serial.print(*messagePointer);
  simonSolved = *messagePointer;
}


#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
const char *ssid     = "garage@eee";
const char *password = "garage@eee";

//time stuff
#include <NTPClient.h>
#include <TimeLib.h>
#include <TM1637.h>
int CLK = 32;
int DIO = 33;
TM1637 tm(CLK, DIO);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 28800, 60000);
char Time[ ] = "TIME:00:00:00";
char Date[ ] = "DATE:00/00/2000";
byte last_second, second_, minute_, hour_, day_, month_;
int year_;

//audio stuff
#include "SoftwareSerial.h"
#define RXD2 16
#define TXD2 17
SoftwareSerial serial2;
// Select storage device to TF card
static int8_t select_SD_card[] = {0x7e, 0x03, 0X35, 0x01, 0xef}; // 7E 03 35 01 EF
// Play with index: /01/001xxx.mp3
static int8_t play_first_song[] = {0x7e, 0x04, 0x41, 0x00, 0x01, 0xef}; // 7E 04 41 00 01 EF
// Play with index: /01/002xxx.mp3
static int8_t play_second_song[] = {0x7e, 0x04, 0x41, 0x00, 0x02, 0xef}; // 7E 04 41 00 02 EF
// Play the song.
static int8_t play[] = {0x7e, 0x02, 0x01, 0xef}; // 7E 02 01 EF
// Pause the song.
static int8_t pau[] = {0x7e, 0x02, 0x02, 0xef}; // 7E 02 02 EF


//telegram bot stuff
#define BOTtoken "5058396616:AAEHjLIRkv_RDnLlbWJDPk3S_3QfHyT1ujk" // your Bot Token (Get from Botfather)
#define CHAT_ID "-797727510"

WiFiClientSecure client;

UniversalTelegramBot bot(BOTtoken, client);

int botRequestDelay = 1000;
unsigned long lastTimeBotRan;


void setup() {

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting.");

  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected");
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org


  timeClient.begin();
  tm.init();
  // set brightness; 0-7
  tm.set(2);
  serial2.begin(9600, SWSERIAL_8N1, RXD2, TXD2, false);
  delay(100);

  send_command_to_MP3_player(select_SD_card, 5); //in set up
  //send_command_to_MP3_player(play_second_song, 6); // to move

  bot.sendMessage(CHAT_ID, "Coo Coo! Set up Completed.", "");

  // ESPNOW STYUFF
  Serial.println("MAC Address: ");
  Serial.print(WiFi.macAddress());
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // register peer
  esp_now_peer_info_t peerInfo;

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  Serial.println("ESP Now Init Success!");


  pinMode(ONBOARD_LED, OUTPUT);
  //when we press the EN button on ESP32
  // sets the pins for motors as outputs:
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
  // configure LED PWM functionalitites
  ledcSetup(pwmChannel1, freq, resolution);
  ledcSetup(pwmChannel2, freq, resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(enable1Pin, pwmChannel1);
  ledcAttachPin(enable2Pin, pwmChannel2);
  Serial.println("Set-Up Complete! Loop will start in 2 seconds.");
  randomSeed(analogRead(13));
  digitalWrite(ONBOARD_LED, HIGH);

}



void send_command_to_MP3_player(int8_t command[], int len) {
  for (int i = 0; i < len; i++) {
    serial2.write(command[i]);
    Serial.print(command[i], HEX);
  }
  delay(10);
}

int randDuration;
int randAction;
int alarmTime = -1 ;
void loop() {
  esp_now_register_recv_cb(onReceiveData);
  if (simonSolved == 1) {
    //if solved stop playing
    Serial.println("Simon Solved: Alarm Stopped!");
    send_command_to_MP3_player(pau, 4);
    bot.sendMessage(CHAT_ID, "Well, someone is finally awake pfft", "");
    simonSolved = 0;
    alarmState = 0;
  }

  timeClient.update();
  unsigned long unix_epoch = timeClient.getEpochTime();    // Get Unix epoch time from the NTP server

  second_ = second(unix_epoch);
  if (last_second != second_) {
    minute_ = minute(unix_epoch);
    hour_   = hour(unix_epoch);
    day_    = day(unix_epoch);
    month_  = month(unix_epoch);
    year_   = year(unix_epoch);


    Time[12] = second_ % 10 + 48;
    Time[11] = second_ / 10 + 48;
    Time[9]  = minute_ % 10 + 48;
    Time[8]  = minute_ / 10 + 48;
    Time[6]  = hour_   % 10 + 48;
    Time[5]  = hour_   / 10 + 48;


    Date[5]  = day_   / 10 + 48;
    Date[6]  = day_   % 10 + 48;
    Date[8]  = month_  / 10 + 48;
    Date[9]  = month_  % 10 + 48;
    Date[13] = (year_   / 10) % 10 + 48;
    Date[14] = year_   % 10 % 10 + 48;

    Serial.println(Time);
    Serial.println(Date);

    //int timeNum = hour_*10000 + minute_*100 + second_;
    int timeNum = hour_ * 100 + minute_;
    Serial.println("Display Time: ");
    Serial.println(timeNum);
    //Serial.println(alarmTime);
    displayNumber(timeNum);
    last_second = second_;

    if (alarmTime == timeNum && alarmState == 0 && second_ < 10) {
      Serial.println("Alarm Time");
      send_command_to_MP3_player(play_second_song, 6); // to move
      alarmState = 1;

      Serial.println(alarmState);

      // Send message via ESP-NOW
      esp_err_t result =  esp_now_send(broadcastAddress, (uint8_t *) &alarmState, sizeof(int));

      if (result == ESP_OK) {
        Serial.println("Sent with success");
      }
      else {
        Serial.println("Error sending the data");
      }
    } else if (alarmState == 1) {
      randDuration = random(1000, 4000);
      randAction = random(1, 8);

      Serial.println(randAction);
      Serial.println(randDuration);

      carMove(randAction, randDuration)     ;
      carMove(0, 10);
    }



  }
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

}

void displayNumber(int num) {

  tm.display(3, num / 1 % 10);
  tm.display(2, num / 10 % 10);
  tm.display(1, num / 100 % 10);
  tm.display(0, num / 1000 % 10);
}



void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));
  for (int i = 0; i < numNewMessages; i++) {

    String chat_id = String(bot.messages[i].chat_id);
    Serial.println(chat_id);
    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }

    String text = bot.messages[i].text;
    Serial.println(text);

    String keyboardJson = "[[\"Kick Vibin\", \"Annoy him even more\"],[\"Spray face with water\", \"Fly off into the sunset\"]]";

    String from_name = bot.messages[i].from_name;
    if (text == "/start") {
      String welcome = "Successfully annoyed Vibin for 15 mins. What else do you want to do?";
      bot.sendMessageWithReplyKeyboard(chat_id, welcome, "", keyboardJson, true);
      Serial.println("Sent");
    } else if (text.substring(0, 10) == "/set_alarm") {
      Serial.println(text.substring(0, 10));
      Serial.println(text.indexOf(":"));
      int divider = text.indexOf(":");
      alarmTime = text.substring(11, divider).toInt() * 100 + text.substring(divider + 1).toInt();

      Serial.println(alarmTime);
    }

  }
}


//this is the carMove function created so as to make my life easier
//it takes in two parameters a string: Action and an int variable actionDur
//the idea is to make a readable function which allows the robot to move for a specified amount of time

// 0: stop
//     1  
//  8    2 
//7        3
//  6    4
//     5 
void carMove(int action, int actionDur) {

  Serial.println("Motor Action: " + action );
  ledcWrite(pwmChannel1, dutyCycle);
  ledcWrite(pwmChannel2, dutyCycle);

  switch (action) {
    case 1:
      // Move the DC motor forward at maximum speed
      digitalWrite(motor1Pin1, HIGH);
      digitalWrite(motor1Pin2, LOW);
      digitalWrite(motor2Pin1, HIGH);
      digitalWrite(motor2Pin2, LOW);
      delay(actionDur);
      break;

    case 5:
      // Move Backwards
      digitalWrite(motor1Pin1, LOW);
      digitalWrite(motor1Pin2, HIGH);
      digitalWrite(motor2Pin1, LOW);
      digitalWrite(motor2Pin2, HIGH);
      delay(actionDur);
      break;

    case 0:
      // Stop
      digitalWrite(motor1Pin1, LOW);
      digitalWrite(motor1Pin2, LOW);
      digitalWrite(motor2Pin1, LOW);
      digitalWrite(motor2Pin2, LOW);
      break;

    case 7:
      // Turn Left
      digitalWrite(motor1Pin1, LOW);
      digitalWrite(motor1Pin2, HIGH);
      digitalWrite(motor2Pin1, HIGH);
      digitalWrite(motor2Pin2, LOW);
      delay(actionDur/2);
      break;

    case 3:
      // Turn Right
      digitalWrite(motor1Pin1, HIGH);
      digitalWrite(motor1Pin2, LOW);
      digitalWrite(motor2Pin1, LOW);
      digitalWrite(motor2Pin2, HIGH);
      delay(actionDur/2);
      break;

    case 2:
      // Tilt Forward Right
      ledcWrite(pwmChannel1, dutyCycle);
      ledcWrite(pwmChannel2, (int)dutyCycle * diffRatio);
      digitalWrite(motor1Pin1, HIGH);
      digitalWrite(motor1Pin2, LOW);
      digitalWrite(motor2Pin1, HIGH);
      digitalWrite(motor2Pin2, LOW);
      delay(actionDur);
      break;




    case 4:
      // Tilt Backwards Right
      ledcWrite(pwmChannel1, dutyCycle);
      ledcWrite(pwmChannel2, (int)dutyCycle * diffRatio);
      digitalWrite(motor1Pin1, LOW);
      digitalWrite(motor1Pin2, HIGH);
      digitalWrite(motor2Pin1, LOW);
      digitalWrite(motor2Pin2, HIGH);
      delay(actionDur);
      break;


    case 6:
      // Tilt Backwards Left
      ledcWrite(pwmChannel1, (int)dutyCycle * diffRatio);
      ledcWrite(pwmChannel2, dutyCycle);
      digitalWrite(motor1Pin1, LOW);
      digitalWrite(motor1Pin2, HIGH);
      digitalWrite(motor2Pin1, LOW);
      digitalWrite(motor2Pin2, HIGH);
      delay(actionDur);
      break;

    case 8:
      // Tilt Forward Left
      ledcWrite(pwmChannel1, (int)dutyCycle * diffRatio);
      ledcWrite(pwmChannel2, dutyCycle);
      digitalWrite(motor1Pin1, HIGH);
      digitalWrite(motor1Pin2, LOW);
      digitalWrite(motor2Pin1, HIGH);
      digitalWrite(motor2Pin2, LOW);
      delay(actionDur);
      break;
  }




}
