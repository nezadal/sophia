#include <DHT.h>
#include <SoftwareSerial.h>     
#include "HX711.h"
#include "LowPower.h"
#include <GSMSimHTTP.h>
                       
#define interval 75 // interval pro čtení hodnot čidel a jejich odesílání v násobcích 8s. Bezplatná verze ThingSpeak umožňuje minimální interval 15 000 ms.  
// 375 - 60 minut
// 187 - 30 minut
// 75 - 12 minut   
// 8 - cca minuta    

/////// Měření hmotnosti  ////////////////////////////////////////////////
// konstanty pro můstkovou váhu
//#define calibration_factor -25500.0 //This value is obtained using the SparkFun_HX711_Calibration sketch
//#define offset 8.45
//#define tempcorr 0.0134
//  2x loadcell
///#define calibration_factor -26880.0 //This value is obtained using the SparkFun_HX711_Calibration sketc
//////#define calibration_factor -25500.0 //This value is obtained using the SparkFun_HX711_Calibration sketch - Poloviční můstek

// Červená váha, 4x load cell - E+ fialová, E- žlutá, A+ šedá, A- modrá
#define calibration_factor -20600
#define offset -30.6
#define tempcorr 0.0205
#define tempcal 20


// Modrá váha - E+ zelená, E- červená, A+ černá, A- bílá
//#define calibration_factor -25300 
//#define offset 8.7
//#define tempcorr 0.0095
//#define tempcal 20

 
 

// teplota, při které byla měřena kalibrace

#define DOUT  3
#define CLK  2
HX711 scale;
float weight;

/////// Měření teploty a vlhkosti ////////////////////////////////////////////////
#define DHTPIN 9          // pin pro připojení teplotního a vlhkostního sensoru
#define DHTTYPE DHT22     // typ teplotně-vlhkostního čidla - může být DHT22
DHT dht(DHTPIN, DHTTYPE); 
float hum;                                    
float temp;  

/////// SIM karta, připojení ////////////////////////////////////////////////////
#define RESET_PIN 8 // you can use any pin.
#define SIMPWR_PIN 6 // červená, stříbrná váha
//#define SIMPWR_PIN 5 // modrá váha
//static volatile int num = 0;
GSMSimHTTP http(Serial, RESET_PIN); // GSMSimHíTTP inherit from GSMSimGPRS. You can use GSMSim and GSMSimGPRS methods with it.
//// nastavení jména serveru, ke kterému se chceme připojit
String gprsServer = "api.thingspeak.com";
//// nastavení ukládacího API pro ThingSpeak
String API = "PZFG108LGVQHWH3F"; // stříbrná váha
////String API = "0MI5IBLE4137224H"; // červená váha
////String API = "F5DWVQP4W90CFYUY"; // modrá váha


//// Počet pokusů o připojení k GPRS před resetem
int Ecount = 3;  

  


/////// Debug ////////////////////////////////////////////////////
SoftwareSerial debug(10, 11);


/////// Měření napětí baterie ////////////////////////////////////////////////////
int voltageinput = A1;
float vout = 0.0;
float vin = 0.0;
float R1 = 46600.0;  
float R2 = 6720.0; 
 

int voltage = 0;

boolean alarmsent;        // byl poslán alarm? Poslán = true, neposlán = false.                    

void setup()
{
  pinMode(SIMPWR_PIN, OUTPUT);  
  digitalWrite(SIMPWR_PIN, HIGH);
  delay(500);
  debug.begin(57600); 
  alarmsent = false;    
    
  
  /// Teplota a vlhkost
  dht.begin();    
  
  /// Hmotnost
  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor); //This value is obtained by using the SparkFun_HX711_Calibration sketch
//  scale.tare(); //Assuming there is no weight on the scale at start up, reset the scale to 0
//  debug.println("Váha nulována, OK");   

  /// Měření napětí
  analogReference(INTERNAL); 
  pinMode(voltageinput, INPUT);   
  
             
}

void AlarmCheck()
// Zkontroluje magnetický senzor. 
// Pokud je rozpojený, a alarmsent je false, odešle SMS na udané číslo. 
//      Pokud je odeslání v pořádku, nastaví globální proměnnou alarmsent na true.
// Pokud je spojený nastaví alarmsent na false.
{
  
}



void ReadTemperatureHumidity()
// načte do globálních float hum a temp vlhkost a teplotu
{

   hum = dht.readHumidity();                 
   temp = dht.readTemperature();
      
   if (isnan(hum) || isnan(temp))    
   // Pokud je chyba čtení, vypíše na sériovou linku hlášení, počká 2,5 s a zopakuje čtení.          
      {
         debug.println("Chyba cteni!"); 
         delay(2500);   
         hum = dht.readHumidity();                 
         temp = dht.readTemperature();
     
       }
  // pokud jsou obě hodnoty v pořádku, tak:
     else                                       
   {
    debug.print("Vlhkost: ");                
    debug.print(hum);
    debug.print(" %, Teplota: ");
    debug.print(temp);
    debug.println(" Celsius");

  }
}  
  
void ReadWeight()
// načte do globální proměnné float weight hmotnost úlu
{             
  // print the average of 5 readings from the ADC minus tare weight, divided
  // by the SCALE parameter set with set_scale
  weight = (scale.get_units(3));
  weight = weight - offset;
  // korelace na teplotu
  weight = weight + tempcorr*(tempcal-temp);
  debug.print("Hmotnost: ");
  debug.println(weight);

 
}

void ReadBattery()
// načte do globální proměnné float battery voltage baterie vin
{
  int poc;
  int avgvoltage = 0;
  poc = 0;
  voltage = 0;
  // prumer z 5ti mereni s mezerou 500 ms
   for(poc=1;poc<6;poc++){ 
     voltage = analogRead(voltageinput);
     avgvoltage = avgvoltage + voltage;
     delay(500);
   }
   voltage = avgvoltage/5;
   debug.print("voltage ");
   debug.print(voltage);
   vout = (voltage *1.1) / 1024.0; // spravne ma b7t hodnota Vint 1.1V
   debug.print("vout ");
   debug.print(vout); 
   vin = vout / (R2/(R1+R2));
//   vin = vout / 0.2359;
   debug.print("Napětí baterie: ");
   debug.println(vin,3);
}




//
//void BuzzerBeep(int bcount)
//{
//  for (int i = 0; i < bcount; i++){
//  tone(buzzer, 1000, 250);
//  delay(500);
//  
//  }
//}
//
//
//void BuzzerSucc()
//{
//  for (int i = 1; i < 6; i++){
//  tone(buzzer, i*1000, 100);
//  delay(200);
//  
//  }
//}
//
//void BuzzerEndLoop()
//{
//  for (int i = 1; i < 12; i++){
//  tone(buzzer, i*500, 50);
//  delay(100);
//  
//  }
//}
//
//void BuzzerFail()
//{
//  for (int i = 3; i > 0; i--){
//  tone(buzzer, i*1000, 250);
//  delay(500);
//  
//  }
//}

boolean SendDataToThingSpeak() {
 boolean isregistered = 1;
 boolean isconnected = 1;
 debug.println("StartSEnding");
 digitalWrite(SIMPWR_PIN, HIGH);
 delay(2000);
 Serial.begin(57600); 
  while(!Serial) {
    debug.println("Cekam na sběrnici"); // wait for module for connect.
  }

  http.init(); // use for init module. Use it if you dont have any valid reason.
  int poc = 0;
  debug.println("INIT_Set Phone Function...");
  do{
    delay(2000);
    poc++;
  }while (!http.setPhoneFunc(1) == 1);   
  delay(500);
  debug.println(http.setPhoneFunc(1));

  
  debug.println("is Module Registered to Network?");
  poc = 0;
  do{
    delay(2000);
    poc++;
    if (poc > 9) {
      isregistered = 0;
      break;      
    }
  }while (!http.isRegistered() == 1);   
  delay(500);
  if (isregistered) {
    debug.print("registrováno do sítě po ");
    debug.print(poc);
    debug.println("x2 s");
  }
  else {
    debug.println("Neregistrováno do sítě");
    }

  debug.println("Connect GPRS");
  poc = 0;
  do{
    delay(2000);
    poc++;
    debug.println(poc);
    if (poc > 9) {
       isconnected = 0;
       break;
       
    }
  }while (!http.connect() == 1); 
  delay(500);  
  if (isconnected) {
    debug.print("Připojeno k GPRS po  ");
    debug.print(poc);
    debug.println("x2 s");
  }
  else {
    debug.println("Nepřipojeno k GPRS datům");
    }
  
  String zprava = gprsServer; 
  zprava += "/update?api_key=";
  zprava += API;
  zprava += "&field1=";
  zprava += hum;
  zprava += "&field2=";
  zprava += temp;
  zprava += "&field3=";
  zprava += weight;
  zprava += "&field4=";
  zprava += vin;
  debug.println(zprava);
  String odezva = (http.get(zprava));
  debug.println(odezva);
  delay(500);
  debug.print("Close GPRS... ");
  debug.println(http.closeConn());
  
  // Vypnutí
  poc = 0;
  debug.print("Vypnuti SIM modulu... ");
  delay(1000);
  do{
    delay(1000);
    poc++;
  }while (!http.setPhoneFunc(4) == 1);  
  
  debug.print("Vypnuto po ");
  debug.print(poc);
  debug.println(" s");
  digitalWrite(SIMPWR_PIN, LOW);
  
  if (odezva.substring(11,23) == "HTTPCODE:200"){
    return true;
  }
  else {
    return false;
  } 

}


void loop()
{
  //AlarmCheck;
    delay(1000);
    ReadTemperatureHumidity(); 
    ReadWeight();
    ReadBattery();
    
   int poc = 0;
   do {
    poc++;
    if (poc > 3) {
      debug.println("Odeslání po 3 pokusech neúspěšné, usínám...");
      break;
    }
   } while (!SendDataToThingSpeak() == 1); 
   debug.println("Odeslání úspěšné, usínám...");
    for (int i = 0; i <=  interval; i++) {
     LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); 
   }
}


                              
