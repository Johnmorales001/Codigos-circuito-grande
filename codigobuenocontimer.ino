#include <Keypad.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// Definimos el ancho y alto de la pantalla OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Pin de datos del sensor DHT11
#define DHT_PIN 4
#define DHT_TYPE DHT11

// Creamos un objeto de pantalla OLED con las dimensiones especificadas y la dirección I2C 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHT dht(DHT_PIN, DHT_TYPE);

int cnt1 = 7 * 3600;  // Comienza en 7 horas (en segundos)
int cnt2 = 0;
int cnt3 = 0;
int cnt4 = 0;
int cnt5 = 0;

char keys[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pin_rows[4] = {32, 33, 25, 26};
byte pin_cols[4] = {27, 14, 12, 13};

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_cols, 4, 4);

String password = "159";
String enteredPassword = "";
bool isPasswordCorrect = false;
bool isCounting = false;  // Variable para controlar el contador

bool isReadingSensor = false;  // Control de lectura del sensor, comienza inactivo

// Bandera para manejar interrupciones
volatile bool flag_isr1 = false;
volatile bool flag_isr2 = false;
volatile bool flag_isr3 = false;
volatile bool flag_isr4 = false;

// Banderas para manejar tareas desde las ISR
volatile bool counterFlag = false;
volatile bool sensorFlag = false;

// Temporizadores de hardware
hw_timer_t* counterTimer = NULL;
hw_timer_t* sensorTimer = NULL;

// Declaración de funciones
void IRAM_ATTR handleCounter();
void IRAM_ATTR handleSensorReading();
void IRAM_ATTR isr1();
void IRAM_ATTR isr2();
void IRAM_ATTR isr3();
void IRAM_ATTR isr4();

void IRAM_ATTR handleCounter() {
  counterFlag = true; // Marca una bandera
}

void IRAM_ATTR handleSensorReading() {
  if (isReadingSensor) {
    sensorFlag = true; // Marca una bandera solo si el sensor está habilitado
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Hello, ESP32!");
  pinMode(18, INPUT_PULLUP);
  pinMode(19, INPUT_PULLUP);
  pinMode(21, INPUT_PULLUP);
  pinMode(22, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(18), isr1, FALLING);
  attachInterrupt(digitalPinToInterrupt(19), isr2, FALLING);
  attachInterrupt(digitalPinToInterrupt(21), isr3, FALLING);
  attachInterrupt(digitalPinToInterrupt(22), isr4, FALLING);

  // Inicializamos la comunicación I2C con los pines especificados
  Wire.begin(23, 15);

  // Inicializamos la pantalla OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("No se encontró la pantalla OLED"));
    for (;;);
  }

  // Inicializamos el sensor DHT
  dht.begin();

  // Limpiamos la pantalla
  display.clearDisplay();

  // Definimos el tamaño del texto y el color
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Configurar temporizadores de hardware
  counterTimer = timerBegin(0, 80, true);  // Timer 0, prescaler 80 para contar microsegundos
  timerAttachInterrupt(counterTimer, &handleCounter, true);
  timerAlarmWrite(counterTimer, 1000000, true);  // Llama cada 1 segundo
  timerAlarmEnable(counterTimer);

  sensorTimer = timerBegin(1, 80, true);  // Timer 1
  timerAttachInterrupt(sensorTimer, &handleSensorReading, true);
  timerAlarmWrite(sensorTimer, 300000000, true);  // Llama cada 3 segundos (3,000,000 microsegundos)
  timerAlarmEnable(sensorTimer);
}

void loop() {
  char key = keypad.getKey();  // Leer tecla presionada

  if (key) {
    Serial.print("Key pressed: ");
    Serial.println(key);

    // Mostrar en la pantalla OLED
    display.clearDisplay();
    display.setCursor(0, 10);
    display.print("Key pressed: ");
    display.print(key);
    display.display();

    if (key == '*') {
      // Verificar la contraseña cuando se presiona '*'
      if (enteredPassword == password) {
        if (isPasswordCorrect && isCounting) {
          // Si la contraseña ya fue correcta y el contador estaba activo, detener el contador
          isCounting = false;
          isReadingSensor = false;  // Detener las lecturas del sensor
          Serial.println("Password entered again. Counting stopped.");
          
          // Mostrar en la pantalla OLED
          display.clearDisplay();
          display.setCursor(0, 10);
          display.print("Password entered again.");
          display.setCursor(0, 20);
          display.print("Counting stopped.");
          display.display();
        } else {
          // Si es la primera vez que se introduce correctamente la contraseña, iniciar el contador y lectura del sensor
          isPasswordCorrect = true;
          isCounting = true;  // Iniciar el contador después de la contraseña correcta
          isReadingSensor = true;  // Iniciar la lectura del sensor DHT11
          Serial.println("Password correct! Access granted.");
          
          // Mostrar en la pantalla OLED
          display.clearDisplay();
          display.setCursor(0, 10);
          display.print("Password correct!");
          display.setCursor(0, 20);
          display.print("Access granted.");
          display.display();
        }
      } else {
        Serial.println("Password incorrect! Try again.");
        
        // Mostrar en la pantalla OLED
        display.clearDisplay();
        display.setCursor(0, 10);
        display.print("Password incorrect!");
        display.setCursor(0, 20);
        display.print("Try again.");
        display.display();
      }
      enteredPassword = "";  // Resetear la entrada de la contraseña
    } else if (key == '#') {
      // Detener el contador cuando se presiona '#'
      isCounting = false;
      isReadingSensor = false;  // Detener la lectura del sensor
      Serial.println("Counting stopped.");
      
      // Mostrar en la pantalla OLED
      display.clearDisplay();
      display.setCursor(0, 10);
      display.print("Counting stopped.");
      display.display();
    } else {
      // Añadir el carácter a la contraseña ingresada
      enteredPassword += key;
    }
  }

  // Verificar si se activó la bandera del contador
  if (counterFlag) {
    counterFlag = false;
    if (isPasswordCorrect && isCounting) {
      if (cnt1 < 21 * 3600) {  // Asegurarnos de que no pase de 21 horas (en segundos)
        cnt1++;  // Incrementar el contador (en segundos)
      }
      
      // Convertir cnt1 de segundos a horas, minutos y segundos
      int hours = cnt1 / 3600;
      int minutes = (cnt1 % 3600) / 60;
      int seconds = cnt1 % 60;

      Serial.print("Hora = "); 
      Serial.print(hours); 
      Serial.print(":"); 
      Serial.print(minutes); 
      Serial.print(":"); 
      Serial.println(seconds);
      
      // Mostrar en la pantalla OLED
      display.clearDisplay();
      display.setCursor(0, 10);
      display.print("Hora: ");
      display.print(hours);
      display.print(":");
      display.print(minutes);
      display.print(":");
      display.println(seconds);
      display.display();
    }
  }

  // Verificar si se activó la bandera del sensor
  if (sensorFlag) {
    sensorFlag = false;
    Serial.println("Reading sensor...");  // Mensaje de depuración
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println("Error leyendo el sensor!");
    } else {
      Serial.print("Humedad: "); Serial.print(h); Serial.print(" %\t");
      Serial.print("Temperatura: "); Serial.print(t); Serial.println(" *C");

      // Mostrar en la pantalla OLED
      display.clearDisplay();
      display.setCursor(0, 20);
      display.print("Humedad: ");
      display.print(h);
      display.print(" %");
      display.setCursor(0, 30);
      display.print("Temperatura: ");
      display.print(t);
      display.print(" *C");
      display.display();
    }
  }

  // Procesar las interrupciones de forma segura fuera de la ISR
  if (flag_isr1) {
    flag_isr1 = false;
    cnt2++;
    Serial.print("Entrada 1 ="); Serial.println(cnt2);
    
    // Mostrar en la pantalla OLED
    display.clearDisplay();
    display.setCursor(0, 10);
    display.print("Ent1=");
    display.print(cnt2);
    display.display();
  }
  if (flag_isr2) {
    flag_isr2 = false;
    cnt3++;
    Serial.print("Salida 1 ="); Serial.println(cnt3);
    
    // Mostrar en la pantalla OLED
    display.clearDisplay();
    display.setCursor(0, 10);
    display.print("Sal1 =");
    display.print(cnt3);
    display.display();
  }
  if (flag_isr3) {
    flag_isr3 = false;
    cnt4++;
    Serial.print("Entrada 2="); Serial.println(cnt4);
    
    // Mostrar en la pantalla OLED
    display.clearDisplay();
    display.setCursor(0, 10);
    display.print("Ent2=");
    display.print(cnt4);
    display.display();
  }
  if (flag_isr4) {
    flag_isr4 = false;
    cnt5++;
    Serial.print("Salida 2 ="); Serial.println(cnt5);
    
    // Mostrar en la pantalla OLED
    display.clearDisplay();
    display.setCursor(0, 10);
    display.print("Sal2=");
    display.print(cnt5);
    display.display();
  }
}

// Funciones de interrupción para aumentar los contadores
void IRAM_ATTR isr1() {
  flag_isr1 = true;
}

void IRAM_ATTR isr2() {
  flag_isr2 = true;
}

void IRAM_ATTR isr3() {
  flag_isr3 = true;
}

void IRAM_ATTR isr4() {
  flag_isr4 = true;
}
