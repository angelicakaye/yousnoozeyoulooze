#include <esp_now.h>
#include <WiFi.h>

#define RED_LED 15
#define GREEN_LED 2
#define BLUE_LED 4
#define RED_CH 0
#define GREEN_CH 1
#define BLUE_CH 2
#define WHITE 21
#define BLUE 5
#define GREEN 18
#define RED 19
#define RANDOM_PIN 13
#define OUT 23

#define PWM_FREQ 1000
#define PWM_RES 8

int sequence[] = {0, 0, 0, 0, 0};

int counter = 0;

//78:E3:6D:17:12:D8

uint8_t broadcastAddress[] = {0x78, 0xE3, 0x6D, 0x17, 0x12, 0xD8};

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if(status != ESP_NOW_SEND_SUCCESS){
    delay(50);
    int  stopIt = 1;
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &stopIt, sizeof(stopIt));
  }
}

int annoyingRobot = 0;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&annoyingRobot, incomingData, sizeof(annoyingRobot));
  Serial.println("GODDITTT");
  Serial.println(annoyingRobot);
}

void setup() {
  // put your setup code here, to run once:
  pinMode(WHITE, INPUT_PULLUP);
  pinMode(BLUE, INPUT_PULLUP);
  pinMode(GREEN, INPUT_PULLUP);
  pinMode(RED, INPUT_PULLUP);
  pinMode(OUT, OUTPUT);

  pinMode(RANDOM_PIN, INPUT);

  ledcAttachPin(RED_LED, RED_CH);
  ledcAttachPin(GREEN_LED, GREEN_CH);
  ledcAttachPin(BLUE_LED, BLUE_CH);
  ledcSetup(RED_CH, PWM_FREQ, PWM_RES);
  ledcSetup(GREEN_CH, PWM_FREQ, PWM_RES);
  ledcSetup(BLUE_CH, PWM_FREQ, PWM_RES);

  showLED(0, 0, 0);


  Serial.begin(115200);

  randomizeSeq();

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  
  
  // Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  

  
}

void loop() {
  // put your main code here, to run repeatedly:
  //randomSeed(analogRead(RANDOM_PIN));
  //int randomColour = random(1, 5);
  //Serial.println(randomColour);

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  if (annoyingRobot == 1){
  LEDSeq(sequence[counter]);
  counter++;
  if(counter>4) counter = 0;

  int button = buttonPressedCheck();
  if(button > 0 && button == sequence[0]){
    counter = 1;
    while(buttonPressedCheck()!=0){}
    delay(100);
    while(true){
      button = buttonPressedCheck();
      if(button > 0 && button != sequence[counter]){
        break;
      }
      else if(button > 0 && button == sequence[counter]){
        Serial.println("Next");
        while(buttonPressedCheck()!=0){}
        Serial.println("UP");
        counter++;
        delay(100);
      }

      if(counter > 3){
        Serial.println("DIDITT");
        int  stopIt = 1;
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &stopIt, sizeof(stopIt));
        
        showLED(0, 0, 0);
        counter = 0;
        annoyingRobot = 0;
        randomizeSeq();
        break;
      }
    }
    
  }
  
  delay(500);
  }

}


void LEDSeq(int light){
  switch (light) {
      case 1: //white
        showLED(255, 40, 150);
        break;
      case 2: //red
        showLED(255, 0, 0);
        break;
      case 3: //green
        showLED(0, 255, 0);
        break;
      case 4: //blue
        showLED(0, 0, 255);
        break;
      default:
        showLED(0, 0, 0);
        break;
    }
}

void showLED(int red, int green, int blue){
  ledcWrite(RED_CH, red);
  ledcWrite(GREEN_CH, green);
  ledcWrite(BLUE_CH, blue);
}

int buttonPressedCheck(){
  if (!digitalRead(WHITE)) return 1;
  else if (!digitalRead(RED)) return 2;
  else if (!digitalRead(GREEN)) return 3;
  else if (!digitalRead(BLUE)) return 4;
  else return 0;
}

void randomizeSeq(){
  bool state = false;
  for(int i = 0; i < 4; i++){
    state = true;
    while(state){
      randomSeed(analogRead(RANDOM_PIN));
      int randomColour = random(1, 5);
      for(int j = 0; j < 4; j++){
        if(j == i){
          sequence[j] = randomColour;
          state = false;
          break;
        }
        if(randomColour == sequence[j]){
          break;
        }
      }
    }

    Serial.println(sequence[i]);
  }
}
