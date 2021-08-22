#if defined(ESP32)
#include <WiFi.h>
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#endif
/* 1. Define the WiFi credentials */
#define WIFI_SSID "First_Blood"
#define WIFI_PASSWORD "First_Blood8"
// Define Firebase host and auth
#define FIREBASE_HOST "https://tugas-akhir-cffaa-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "dDx4iIpPkRuZEtYSi2kSYVbwxQcSdKgpNVF1mlPL"
//Define Firebase Data object
FirebaseData fbdo;

#include <Keypad.h>
String password_1 = "6613"; // change your password here
String password_2 = "3343";
String password_3 = "7138";
String password_4 = "1234";

const int ROW_NUM = 4; //four rows
const int COLUMN_NUM = 3; //three columns

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte pin_rows[ROW_NUM] = {D1, D2, D3, D4}; //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {D5, D6, D7}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );
String input_password;
boolean keterangan = false;

void setup()
{
  Serial.begin(9600);
  input_password.reserve(3);

  
//------------------------------------------------KONEKSI WIFI-------------------------------------------
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
//------------------------------------------------END KONEKSI WIFI-------------------------------------------

  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
}

void loop()
{
  char key = keypad.getKey();
  
//------------------------------------------------KEYPAD-------------------------------------------
  if (key){
    Serial.println(key);

    if(key == '*') {
      input_password = ""; // menghapus key yang pernah dimasukkan
    }else if(key == '#'){
      if(input_password == password_1 || input_password == password_2 || input_password == password_3 || input_password == password_4 ) {
        keterangan = true;
        Serial.println("password benar!");

      }else{
        keterangan= false;
        Serial.println("password salah!");     
      }

      input_password = ""; // mnghapus inputan key
    } else {
      input_password += key; // menambahkan karakter
    }
//------------------------------------------------END INPUT KEYPAD-------------------------------------------


//------------------------------------------------MENGIRIM DATA KE FIREBASE-------------------------------------------
  if(keterangan==true){
    Firebase.set(fbdo, "Keypad/Password","BENAR");
  }else{
    Firebase.set(fbdo, "Keypad/Password","SALAH");
  }
//------------------------------------------------END KIRIM DATA KE FIREBASE-------------------------------------------
    delay(5000);
    Firebase.set(fbdo, "Keypad/Password","SALAH");
  }


  
}
