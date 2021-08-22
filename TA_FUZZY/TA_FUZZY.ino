#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif
#include "FirebaseESP32.h"
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <LiquidCrystal_I2C.h>
#include <HX711_ADC.h>
#include <Fuzzy.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

LiquidCrystal_I2C lcd(0x27,16,2);

#define RELAY 23

//Sensor ultrasonic
#define echoPin1 13
#define initPin1 14
int distance = 0;

//pins sensor berat
const int HX711_dout = 26; //mcu > HX711 dout pin
const int HX711_sck = 25; //mcu > HX711 sck pin
HX711_ADC LoadCell(HX711_dout, HX711_sck);
const int calVal_eepromAdress = 0;
unsigned long t = 0;

//pin led
int pin_led = 2;

String formattedDate;
String dayStamp;
String timeStamp;

// SSID and Password of your WiFi router
const char* ssid = "First_Blood";
const char* password = "First_Blood8";

Fuzzy *fuzzy = new Fuzzy();

// Define Firebase host and auth
#define FIREBASE_HOST "https://tugas-akhir-cffaa-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "dDx4iIpPkRuZEtYSi2kSYVbwxQcSdKgpNVF1mlPL"

//--------------------------------------------------Inisialisasi Variabel Firebase---------------------------------------------
FirebaseData fbdo, ambil_resi_app, ambil_kondisi, ambil_key;
//------------------------------------------------END Inisialisasi Variabel Firebase-------------------------------------------

// FuzzyInput sensor ultrasonic
FuzzySet *penuh         = new FuzzySet(0, 10, 10, 20);
FuzzySet *setengah      = new FuzzySet(15, 30, 40, 50);
FuzzySet *kosong        = new FuzzySet(45, 60, 80, 100);

// FuzzyInput sensor berat
FuzzySet *tdkberat  = new FuzzySet(0, 0, 50,150);
FuzzySet *sberat    = new FuzzySet(100, 1000, 10000, 15000);
FuzzySet *berat     = new FuzzySet(12500, 30000, 40000, 50000);

// FuzzyOutput
FuzzySet *redup            = new FuzzySet(0, 0, 60, 100);
FuzzySet *setengahl        = new FuzzySet(70, 140, 140, 180);
FuzzySet *terang           = new FuzzySet(130, 180, 255, 255);

void setup() {

      printawal();
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH);
  //pin ultrasonik
  pinMode(initPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(pin_led , OUTPUT);

  // inisialisasi lcd
  lcd.init(); 
// Print pesan ke lcd
  lcd.backlight();


  WiFi.begin(ssid, password);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // Make the On Board Flashing LED on the process of connecting to the wifi router.
  }

    Serial.println("");
    Serial.print("Successfully connected to : ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    //sensor berat
    LoadCell.begin();
    float calibrationValue; // calibration value (see example file "Calibration.ino")
    //kalibrasi dari berat laptop
    calibrationValue = -28.59; // uncomment this if you want to set the calibration value in the sketch

    #if defined(ESP8266)|| defined(ESP32)
      EEPROM.begin(512); 
    #endif
      //waktu stabil sampe ke 0
    unsigned long stabilizingtime = 2000; // presisi tepat setelah power-up dapat ditingkatkan dengan menambahkan beberapa detik waktu stabilisasi
    boolean _tare = true; //tanpa masukin kalibrasi lagi
    LoadCell.start(stabilizingtime, _tare);
    if (LoadCell.getTareTimeoutFlag()) {
      Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
      while (1);
    }
    else {
      LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
      Serial.println("Startup is complete");
    }

    // FuzzyInput1
      FuzzyInput *ultrasonik = new FuzzyInput(1);
      ultrasonik->addFuzzySet(penuh);
      ultrasonik->addFuzzySet(setengah);
      ultrasonik->addFuzzySet(kosong);
      fuzzy->addFuzzyInput(ultrasonik);
    
    //FuzzyInput2
      FuzzyInput *load = new FuzzyInput(4);
      load->addFuzzySet(tdkberat);
      load->addFuzzySet(sberat);
      load->addFuzzySet(berat);
      fuzzy->addFuzzyInput(load);
    
    // FuzzyOutput
      FuzzyOutput *led = new FuzzyOutput(1);
      led->addFuzzySet(redup);
      led->addFuzzySet(setengahl);
      led->addFuzzySet(terang);
      fuzzy->addFuzzyOutput(led);
    
    // Building FuzzyRule////////////////////////////////////////////////////////////////// 1
      FuzzyRuleAntecedent *kosong_tdkberat = new FuzzyRuleAntecedent();
      kosong_tdkberat->joinWithAND(kosong, tdkberat);
    
      FuzzyRuleConsequent *led_redup1 = new FuzzyRuleConsequent();
      led_redup1->addOutput(redup);
    
      FuzzyRule *fuzzyRule1 = new FuzzyRule(1, kosong_tdkberat, led_redup1);
      fuzzy->addFuzzyRule(fuzzyRule1);
    
    // Building FuzzyRule////////////////////////////////////////////////////////////////// 2
      FuzzyRuleAntecedent *setengah_tdkberat = new FuzzyRuleAntecedent();
      setengah_tdkberat->joinWithAND(setengah, tdkberat);
    
      FuzzyRuleConsequent *led_setengahl2 = new FuzzyRuleConsequent();
      led_setengahl2->addOutput(setengahl);
    
      FuzzyRule *fuzzyRule2 = new FuzzyRule(2, setengah_tdkberat, led_setengahl2);
      fuzzy->addFuzzyRule(fuzzyRule2);
    
    // Building FuzzyRule////////////////////////////////////////////////////////////////// 3
      FuzzyRuleAntecedent *penuh_tdkberat = new FuzzyRuleAntecedent();
      penuh_tdkberat->joinWithAND(penuh, tdkberat);
    
      FuzzyRuleConsequent *led_terang3 = new FuzzyRuleConsequent();
      led_terang3->addOutput(terang);
    
      FuzzyRule *fuzzyRule3 = new FuzzyRule(3, penuh_tdkberat, led_terang3);
      fuzzy->addFuzzyRule(fuzzyRule3);
    
    // Building FuzzyRule////////////////////////////////////////////////////////////////// 4
      FuzzyRuleAntecedent *kosong_berat = new FuzzyRuleAntecedent();
      kosong_berat->joinWithAND(kosong, berat);
    
      FuzzyRuleConsequent *led_setengahl4 = new FuzzyRuleConsequent();
      led_setengahl4->addOutput(setengahl);
    
      FuzzyRule *fuzzyRule4 = new FuzzyRule(4, kosong_berat, led_setengahl4);
      fuzzy->addFuzzyRule(fuzzyRule4);
    
    // Building FuzzyRule////////////////////////////////////////////////////////////////// 5
      FuzzyRuleAntecedent *setengah_berat = new FuzzyRuleAntecedent();
      setengah_berat->joinWithAND(setengah, berat);
    
      FuzzyRuleConsequent *led_setengahl5 = new FuzzyRuleConsequent();
      led_setengahl5->addOutput(setengahl);
    
      FuzzyRule *fuzzyRule5 = new FuzzyRule(5, setengah_berat, led_setengahl5);
      fuzzy->addFuzzyRule(fuzzyRule5);
    
    // Building FuzzyRule////////////////////////////////////////////////////////////////// 6
      FuzzyRuleAntecedent *penuh_berat = new FuzzyRuleAntecedent();
      penuh_berat->joinWithAND(setengah, berat);
    
      FuzzyRuleConsequent *led_terang6 = new FuzzyRuleConsequent();
      led_terang6->addOutput(terang);
    
      FuzzyRule *fuzzyRule6 = new FuzzyRule(6, penuh_berat, led_terang6);
      fuzzy->addFuzzyRule(fuzzyRule6);      
      
    // Building FuzzyRule////////////////////////////////////////////////////////////////// 7
      FuzzyRuleAntecedent *kosong_sberat = new FuzzyRuleAntecedent();
      kosong_sberat->joinWithAND(kosong, sberat);
    
      FuzzyRuleConsequent *led_setengahl7 = new FuzzyRuleConsequent();
      led_setengahl7->addOutput(setengahl);
    
      FuzzyRule *fuzzyRule7 = new FuzzyRule(7, kosong_sberat, led_setengahl7);
      fuzzy->addFuzzyRule(fuzzyRule7);
    
    // Building FuzzyRule////////////////////////////////////////////////////////////////// 8
      FuzzyRuleAntecedent *setengah_sberat = new FuzzyRuleAntecedent();
      setengah_sberat->joinWithAND(setengah, sberat);
    
      FuzzyRuleConsequent *led_setengahl8 = new FuzzyRuleConsequent();
      led_setengahl8->addOutput(setengahl);
    
      FuzzyRule *fuzzyRule8 = new FuzzyRule(8, setengah_sberat, led_setengahl8);
      fuzzy->addFuzzyRule(fuzzyRule8);
    
    // Building FuzzyRule////////////////////////////////////////////////////////////////// 9
      FuzzyRuleAntecedent *penuh_sberat = new FuzzyRuleAntecedent();
      penuh_sberat->joinWithAND(setengah, sberat);
    
      FuzzyRuleConsequent *led_terang9 = new FuzzyRuleConsequent();
      led_terang9->addOutput(terang);
    
      FuzzyRule *fuzzyRule9 = new FuzzyRule(9, penuh_berat, led_terang9);
      fuzzy->addFuzzyRule(fuzzyRule9);

    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Firebase.reconnectWiFi(true);
    
    timeClient.begin();
    timeClient.setTimeOffset(25200);
    Firebase.set(fbdo, "SatusValid", 0);
}

void loop() {
    printawal();
  
//--------------------------------------------------Ambil Waktu Server---------------------------------------------
    while(!timeClient.update()) {
      timeClient.forceUpdate();
    }
    formattedDate = timeClient.getFormattedDate();
    int splitT = formattedDate.indexOf("T");
    dayStamp = formattedDate.substring(0, splitT);
    timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  
    String tanggal = dayStamp;
    String waktu = timeStamp;
    delay(500);
//------------------------------------------------END Ambil Waktu Server-------------------------------------------

    distance = getDistance(initPin1, echoPin1);
    
    float i = LoadCell.getData();
    static boolean newDataReady = 0;
    const int serialPrintInterval = 0; //increase value to slow down serial print activity
    // check for new data/start next conversion:
    if (LoadCell.update()) newDataReady = true;
    delay(150);

    if (newDataReady) {
      if (millis() > t + serialPrintInterval) {
        float i = LoadCell.getData();
        t = millis();
      }
    }

    //-------------Fuzzy----------------
    fuzzy->setInput(1, distance);
    fuzzy->setInput(2, i);
    fuzzy->fuzzify();
    
    int out_led = fuzzy->defuzzify(1);
    digitalWrite(pin_led , out_led);
    //------------------------------------

    Serial.print("Jarak   :");
    Serial.print(distance);
    Serial.println(" CM");
    Serial.print("Berat   :");
    Serial.print(i);
    Serial.println(" gram");
    Serial.print("Result led  : ");
    Serial.println(out_led);

    if(out_led<40){
      Firebase.set(fbdo, "Alat/Status", "KOSONG");
    }else if(out_led >= 40 && out_led <= 120){
      Firebase.set(fbdo, "Alat/Status", "ADA");
    }else{
      Firebase.set(fbdo ,"Alat/Status", "PENUH");
        //lcd print awal
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("KOTAK PENUH");
        lcd.setCursor(0,1);
        lcd.print("Hubungi Pemilik!");
        lcd.clear();
    }

//-------------------------------------------------Ambil Data di Firebase------------------------------------------    
    String path_resi_alat = "/NomorResi/ResiAlat";
    Firebase.get(fbdo, path_resi_alat);
    String resi_alat = fbdo.stringData();
    String resi_input = fbdo.stringData();
//    Serial.println(resi_alat);
    
    String path_resi_app = "/NomorResi/ResiApp/"+resi_input+"/resi/";
    Firebase.get(ambil_resi_app, path_resi_app);
    String resi_app = ambil_resi_app.stringData();
//    Serial.println(resi_app);

    String path_kondisi = "/Alat/Status";
    Firebase.get(ambil_kondisi, path_kondisi);
    String kondisi = ambil_kondisi.stringData();
//    Serial.println(kondisi);

    String path_input_key = "/Keypad/Password";
    Firebase.get(fbdo, path_input_key);
    String pw = fbdo.stringData();
//    Serial.println(pw);
//-------------------------------------------------END Ambil Data di Firebase------------------------------------------


//--------------------------------------------------------------------Fungsi Sistem Implementasi-------------------------------------------------------------------------
    if(((resi_alat == resi_app) || (pw == "BENAR")) && (kondisi != "PENUH")){
      Serial.println("sukses");
      Firebase.set(fbdo, "SatusValid", 1);
      digitalWrite(RELAY, LOW);
      delay(5000);
      digitalWrite(RELAY, HIGH);
      delay(1000);
      Firebase.set(fbdo, "Riwayat/"+resi_input+"/resi", resi_input);
      Firebase.set(fbdo, "Riwayat/"+resi_input+"/tanggal", tanggal);
      Firebase.set(fbdo, "Riwayat/"+resi_input+"/waktu", waktu);
      Serial.println(fbdo.errorReason());
      delay(1000);
      Firebase.set(fbdo, "SatusValid", 0);
    }else{
      Firebase.set(fbdo, "SatusValid", 0);
      Serial.println("Kotak Penuh");
    }
//------------------------------------------------------------------END Fungsi Sistem Implementasi------------------------------------------------------------------------
}

int getDistance (int initPin, int echoPin){
   digitalWrite(initPin, HIGH);
   delayMicroseconds(10);
   digitalWrite(initPin, LOW);
   unsigned long pulseTime = pulseIn(echoPin, HIGH);
   int distance = pulseTime/58;
   return distance;
}

 void printDistance(int id, int dist){
  Serial.print(id);
  Serial.print("------>");  
  Serial.print(dist, DEC);
  Serial.println(" cm"); 
 }

 void printawal(){
    //lcd print awal
    lcd.setCursor(0,0);
    lcd.print("Scan QR-Code /");
    lcd.setCursor(0,1);
    lcd.print("Hubungi Pemilik!");
    lcd.clear();
 }
