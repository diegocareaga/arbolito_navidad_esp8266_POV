#include <Arduino.h>
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#else
  #include <ESP8266WiFi.h>  //https://github.com/esp8266/Arduino
  #include <PubSubClient.h> //https://github.com/knolleary/pubsubclient
  #include <Arduino_JSON.h> //https://github.com/arduino-libraries/Arduino_JSON
#endif
#include <Adafruit_NeoPixel.h>

// Pines utilizados
#define PIN        4 
#define SENSOR     14

// RGB Values
int myRGBColor[] = {255, 0, 0};

//Cantidad de leds
#define NUMPIXELS 29

//Para hacer pausas en el program, no lo ocupe al final
#define delay_1    900

// Variables y constantes

volatile boolean sensorStatus = false;
String mensaje = "hola";
String mensajeScroll = "";

long timerNextChar = 0;

//Topics MQTT
const char* topic_message = "tutopic/chat";
const char* topic_color = "tutopic/color";

// Constantes de configuracion
const char* ssid = "TuWiFiName";        //Remplaza con EL NOMBRE DE TU RED WIFI
const char* password = "TuWiFiPassword";  //Remplaza con LA CONTRASENA DE TU RED WIFI
const char* mqtt_server = "mqtt.mikrodash.com"; //Servidor MQTT
const int mqtt_port = 1883;                     //Puerto MQTT
const int qos_level=1;                          //Calidad del servicio MQTT

//Variables para la conexión y envio de mensajes MQTT
WiFiClient espClient;
PubSubClient client(espClient);

//Conexion Wi-Fi
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi Conectado");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
}

// Funcion para reconectar
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando al broker MQTT...");
    // Se genera un ID aleatorio con
    String clientId = "MikroDashWiFiClient-";
    clientId += String(random(0xffff), HEX);

    //Cuando se conecta
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado");

      //Topics a los que se suscribira para recibir cambios
      //client.subscribe(topic_pwm,qos_level);
      client.subscribe(topic_message,qos_level);
      client.subscribe(topic_color,qos_level);
    } else {
      Serial.print("Error al conectar: ");
      Serial.print(client.state());
      Serial.println(" Reintentando en 5 segundos...");
      delay(5000);
    }
  }
}

String lastMessage = "";
int contador = 0;
//Callback para la recepcion de datos en los topic suscrito
void callback(char* topic, byte* payload, unsigned int length) {
  String myString = ""; 
  for (int i = 0; i < length; i++) {
    myString = myString + (char)payload[i]; //Se convierte el payload de tipo bytes a una cadena de texto (String)
  }
  JSONVar myObject = JSON.parse(myString); // Se convierte la respuesta a tipo Object para acceder a los valores por el nombre.
  if (JSON.typeof(myObject) == "undefined") { // Si el archivo no tiene información o no es JSON se sale del callback
    Serial.println("[JSON] JSON NO VALIDO"); 
    return;
  }else{                                      //Si el archivo es JSON y contiene info entra aqui
    const char* from = myObject["from"];
    if(strcmp(topic,topic_message)==0 && strcmp(from,"app")==0){ //Si el topic es topic_messages y lo envia https://app.mikrodash.com
      String temporalStr = JSON.stringify(myObject["value"]);
      temporalStr.replace("\"", "");
      if(temporalStr.length() > 0 && temporalStr != "" && !lastMessage.equals(temporalStr)){
        contador++;
        lastMessage = temporalStr;
        mensaje = temporalStr;
        mensajeScroll = mensaje;
        timerNextChar = millis() + 2000;
        if(contador == 2){
          client.publish(topic_message, "{\"from\":\"device\",\"message\":\"Arbolito Navidad\",\"value\": \" Arbolito: Recibí el Mensaje\"}", false);
          contador = 0;
        }
      }
    }else if(strcmp(topic,topic_color)==0 && strcmp(from,"app")==0){ //Si el topic es topic_messages y lo envia https://app.mikrodash.com
      String colorString = JSON.stringify(myObject["value"]["rgb"]);
      if(colorString.length() > 0){
        JSONVar colorObject = JSON.parse(colorString);
        myRGBColor[0] = (int) colorObject["r"];
        myRGBColor[1] = (int) colorObject["g"];
        myRGBColor[2] = (int) colorObject["b"];
      }
    }
  }

}

/*
 * Aqui estan algunos caracteres del ascii, ustedes pueden agregar mas con el Excel de este proyecto
 */
int ascii[135][11]  = {
    {1,1,1,1,1,1,1,1,1,0,0},{0,0,0,0,1,0,0,0,0,1,1},{0,0,0,0,1,0,0,0,0,1,1},{0,0,0,0,1,0,0,0,0,1,1},{1,1,1,1,1,1,1,1,1,0,0},  //A 0-4
    {1,1,1,1,1,1,1,1,1,1,1},{1,0,0,0,0,1,0,0,0,0,1},{1,0,0,0,0,1,0,0,0,0,1},{1,0,0,0,0,1,0,0,0,0,1},{0,1,1,1,1,0,1,1,1,1,0},  //B 5-9
    {0,1,1,1,1,1,1,1,1,1,0},{1,1,0,0,0,0,0,0,0,1,1},{1,1,0,0,0,0,0,0,0,1,1},{1,1,0,0,0,0,0,0,0,1,1},{0,0,0,0,0,0,0,0,0,0,0},  //C 10-14
    {1,1,1,1,1,1,1,1,1,1,1},{1,1,0,0,0,0,0,0,0,1,1},{1,1,0,0,0,0,0,0,0,1,1},{0,1,0,0,0,0,0,0,0,1,0},{0,0,1,1,1,1,1,1,1,0,0},  //D 15-19
    {1,1,1,1,1,1,1,1,1,1,1},{1,1,0,0,0,1,1,0,0,1,1},{1,1,0,0,0,1,1,0,0,1,1},{1,1,0,0,0,0,0,0,0,1,1},{0,0,0,0,0,0,0,0,0,0,0},  //E 20-24
    {1,1,1,1,1,1,1,1,1,1,1},{0,0,0,0,0,1,1,0,0,1,1},{0,0,0,0,0,1,1,0,0,1,1},{0,0,0,0,0,0,0,0,0,1,1},{0,0,0,0,0,0,0,0,0,0,0},  //F 25-29
    {1,1,1,1,1,1,1,1,1,1,1},{1,0,0,0,0,0,0,0,0,1,1},{1,0,0,0,1,0,0,0,0,1,1},{1,0,0,0,1,0,0,0,0,1,1},{1,1,1,1,1,0,0,0,0,1,1},  //G 30-34
    {1,1,1,1,1,1,1,1,1,1,1},{0,0,0,0,1,1,0,0,0,0,0},{0,0,0,0,1,1,0,0,0,0,0},{0,0,0,0,1,1,0,0,0,0,0},{1,1,1,1,1,1,1,1,1,1,1},  //H 35-39
    {1,0,0,0,0,0,0,0,0,0,1},{1,0,0,0,0,0,0,0,0,0,1},{1,1,1,1,1,1,1,1,1,1,1},{1,0,0,0,0,0,0,0,0,0,1},{1,0,0,0,0,0,0,0,0,0,1},  //I 40-44
    {1,1,0,0,0,0,0,0,0,0,1},{1,0,0,0,0,0,0,0,0,0,1},{1,1,1,1,1,1,1,1,1,1,1},{0,0,0,0,0,0,0,0,0,0,1},{0,0,0,0,0,0,0,0,0,0,1},  //J 45-49
    {1,1,1,1,1,1,1,1,1,1,1},{0,0,0,0,0,1,1,0,0,0,0},{0,0,0,0,1,1,1,0,0,0,0},{0,0,0,1,1,0,0,1,1,0,0},{1,1,1,1,0,0,0,0,1,1,1},  //K 50-54
    {1,1,1,1,1,1,1,1,1,1,1},{1,0,0,0,0,0,0,0,0,0,0},{1,0,0,0,0,0,0,0,0,0,0},{1,1,1,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0},  //L 55-59
    {1,1,1,1,1,1,1,1,1,1,1},{0,0,0,0,0,0,0,0,1,1,0},{0,0,0,0,0,0,1,1,0,0,0},{0,0,0,0,0,0,0,0,1,1,0},{1,1,1,1,1,1,1,1,1,1,1},  //M 60-64
    {1,1,1,1,1,1,1,1,1,1,1},{0,0,0,0,0,0,0,0,1,1,0},{0,0,0,0,0,0,1,1,0,0,0},{0,0,0,0,1,1,0,0,0,0,0},{1,1,1,1,1,1,1,1,1,1,1},  //N 65-69
    {0,1,1,1,1,1,1,1,1,1,0},{1,0,0,0,0,0,0,0,0,0,1},{1,0,0,0,0,0,0,0,0,0,1},{1,0,0,0,0,0,0,0,0,0,1},{0,1,1,1,1,1,1,1,1,1,0},  //O 70-74
    {1,1,1,1,1,1,1,1,1,1,1},{0,0,0,0,0,1,0,0,0,0,1},{0,0,0,0,0,1,0,0,0,0,1},{0,0,0,0,0,1,0,0,0,0,1},{0,0,0,0,0,0,1,1,1,1,0},  //P 75-79
    {0,1,1,1,1,1,1,1,1,1,0},{1,0,0,0,0,0,0,0,0,0,1},{1,0,1,0,0,0,0,0,0,0,1},{1,1,0,0,0,0,0,0,0,0,1},{1,1,1,1,1,1,1,1,1,1,0},  //Q 80-84
    {1,1,1,1,1,1,1,1,1,1,1},{0,0,0,1,1,1,0,0,0,0,1},{0,0,1,1,0,1,0,0,0,0,1},{0,1,1,0,0,1,0,0,0,0,1},{1,1,0,0,0,0,1,1,1,1,0},  //R 85-89
    {0,1,0,0,0,0,0,1,1,1,0},{1,0,0,0,0,0,1,0,0,0,1},{1,0,0,0,0,0,1,0,0,0,1},{1,0,0,0,0,0,1,0,0,0,1},{0,1,1,1,1,1,0,0,0,1,0},  //S 90-94
    {0,0,0,0,0,0,0,0,0,1,1},{0,0,0,0,0,0,0,0,0,0,1},{1,1,1,1,1,1,1,1,1,1,1},{0,0,0,0,0,0,0,0,0,0,1},{0,0,0,0,0,0,0,0,0,1,1},  //T 95-99
    {0,1,1,1,1,1,1,1,1,1,1},{1,0,0,0,0,0,0,0,0,0,0},{1,0,0,0,0,0,0,0,0,0,0},{1,0,0,0,0,0,0,0,0,0,0},{0,1,1,1,1,1,1,1,1,1,1},  //U 100-104
    {0,0,1,1,1,1,1,1,1,1,1},{0,1,1,0,0,0,0,0,0,0,0},{1,1,0,0,0,0,0,0,0,0,0},{0,1,1,0,0,0,0,0,0,0,0},{0,0,1,1,1,1,1,1,1,1,1},  //V 105-109
    {1,1,1,1,1,1,1,1,1,1,1},{0,1,0,0,0,0,0,0,0,0,0},{0,0,1,1,0,0,0,0,0,0,0},{0,1,0,0,0,0,0,0,0,0,0},{1,1,1,1,1,1,1,1,1,1,1},  //W 110-114
    {1,1,0,0,0,0,0,0,1,1,1},{0,0,1,1,0,0,1,1,0,0,0},{0,0,0,0,1,1,0,0,0,0,0},{0,0,1,1,0,0,1,1,0,0,0},{1,1,0,0,0,0,0,0,1,1,1},  //X 115-119
    {0,0,0,0,0,0,0,1,1,1,1},{0,0,0,0,0,0,1,1,0,0,0},{1,1,1,1,1,1,1,0,0,0,0},{0,0,0,0,0,0,1,1,0,0,0},{0,0,0,0,0,0,0,0,1,1,1},  //Y 120-124
    {1,1,1,1,0,0,0,0,0,0,1},{1,0,0,1,1,0,0,0,0,0,1},{1,0,0,0,1,1,1,0,0,0,1},{1,0,0,0,0,0,0,1,1,0,1},{1,0,0,0,0,0,0,0,1,1,1},  //Z 125-129
    {0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0}   //ESPACIO 130-134
  };

/**
 *Se configura la tira de LEDS, Podrias ocupar incluso LEDS diferentes al WS2812 
 *Verifica que este el modelo y remplazalo por el actual.
 */
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
// Argumento 1 = Número de pixeles encadenados
// Argumento 2 = Número del pin de Arduino utilizado con pin de datos
// Argumento 3 = Banderas de tipo de pixel:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)


//Interrupcion del ESP cuando detecta el sensor
ICACHE_RAM_ATTR void evento() {
 sensorStatus = true;
}

//Se inicializa y configura todo
void setup() {
  mensajeScroll = mensaje;
  delay(10);
  Serial.begin(115200);
  delay(10);
  //Se configura la entrada del sensor y la interrupcion
  pinMode(SENSOR, INPUT);
  attachInterrupt(digitalPinToInterrupt(SENSOR), evento, RISING);
  delay(10);
  setup_wifi();               //Se llama la funcición para conectarse al Wi-Fi
  client.setServer(mqtt_server, mqtt_port);  //Se indica el servidor y puerto de MQTT para conectarse
  client.setCallback(callback);  //Se establece el callback, donde se reciben los cambios de las suscripciones MQTT
  delay(10);
  // Inicializamos el objeto "pixeles"
  pixels.begin();
  pixels.setBrightness(40);
  pixels.clear();
  for(int row = 0; row <= NUMPIXELS; row++){
    pixels.setPixelColor(row, pixels.Color(0, 255, 0));
  }
  pixels.show();
  delay(10);
  timerNextChar = millis() + 2000;
  delay(10);
}

//Ciclo infinito hasta que se apague
void loop() {
  if (!client.connected()) { //Si se detecta que no hay conexión con el broker 
    reconnect();              //Reintenta la conexión
  }
  client.loop();            //De lo contrario crea un bucle para las suscripciones MQTT
  if(sensorStatus==true)
  {
    if(mensaje.length() > 3 || mensajeScroll.length() <= 3){
      if(timerNextChar <= millis()){
        if(mensajeScroll.length() > 3){
          mensajeScroll.remove(0, 1);
          timerNextChar = millis() + 500;
        }else if(mensajeScroll.length() <= 3){
          mensajeScroll = mensaje;
          timerNextChar = millis() + 1000;
        }
      }
    }
    stringToChar(mensajeScroll.substring(0,4));
    sensorStatus = false;
  }
}

//Muestra el caracter correspondiente
void showChar(int x){
  Serial.println(x);
  for(int column=x;column < x+6; column++){
      pixels.clear();
      for(int row=NUMPIXELS; row>=0; row--) {
        
        if(row <= 25 && row >14){
          if(column <= x+4){
            if(ascii[column][25-row] == 1){
            pixels.setPixelColor(row, pixels.Color(myRGBColor[0], myRGBColor[1], myRGBColor[2]));
            }else{
              pixels.setPixelColor(row, pixels.Color(0, 15, 0));
            }
          }else{
            pixels.setPixelColor(row, pixels.Color(0, 15, 0));
          }
        }else{
          pixels.setPixelColor(row, pixels.Color(0, 255, 0));
        }
        
      }
      pixels.show(); 
      //delayMicroseconds(delay_1); //Habiliten este delay si ven los caracteres muy anchos y jueguen con los valores de delay_1 declarado al inicio
  }
  //pixels.clear();
  //pixels.show();
}

//Convierte un string a un charArray y posteriormente llama a la función ShowChar
void stringToChar(String msg){  
  int len = msg.length()+1;
  char charBuf[len];
  msg.toUpperCase();
  msg.toCharArray(charBuf, len);

  //Serial.println(charBuf);
  for(int i=0;i < len-1; i++){
    switch(charBuf[i]){
      case 'A':
        showChar(0);
        break;
      case 'B':
        showChar(5);
        break;
      case 'C':
        showChar(10);
        break;
      case 'D':
        showChar(15);
        break;
      case 'E':
        showChar(20);
        break;
      case 'F':
        showChar(25);
        break;
      case 'G':
        showChar(30);
        break;
      case 'H':
        showChar(35);
        break;
      case 'I':
        showChar(40);
        break;
      case 'J':
        showChar(45);
        break;
      case 'K':
        showChar(50);
        break;
      case 'L':
        showChar(55);
        break;
      case 'M':
        showChar(60);
        break;
      case 'N':
        showChar(65);
        break;
      case 'O':
        showChar(70);
        break;
      case 'P':
        showChar(75);
        break;
      case 'Q':
        showChar(80);
        break;
      case 'R':
        showChar(85);
        break;
      case 'S':
        showChar(90);
        break;
      case 'T':
        showChar(95);
        break;
        case 'U':
        showChar(100);
        break;
      case 'V':
        showChar(105);
        break;
      case 'W':
        showChar(110);
        break;
      case 'X':
        showChar(115);
        break;
      case 'Y':
        showChar(120);
        break;
      case 'Z':
        showChar(125);
        break;
      case ' ':
        showChar(130);
        break;
    }
  }
}
