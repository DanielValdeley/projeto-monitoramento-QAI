#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cScd30.h>
#include <SensirionI2CSen5x.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Manter compatibilidade sensirion
#define NO_ERROR 0

// Definição dos pinos do PRIMEIRO barramento I2C (LCD)
#define SDA_1 21  // Verde
#define SCL_1 22  // Amarelo

// Definição dos pinos do SEGUNDO barramento I2C (Sensor SCD30)
#define SDA_2 33  // Verde
#define SCL_2 32  // Amarelo

// Definição do pino da chave que habilita gravação
#define CHAVE 14

// Definição do pino do led
#define LED 12

// Configuração do display LCD
int lcdColumns = 20;
int lcdRows = 4;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
// Variável para controlar a limpeza do LCD em um loop (evita do LCD piscar)
int controleLCD = 0;

// Criando instância para o segundo barramento I2C
TwoWire I2Ctwo = TwoWire(1);

// Criando instância do sensores
SensirionI2cScd30 sensorSCD30;
SensirionI2CSen5x sensorSEN55;

// Criando instância do TinyRTC
RTC_DS1307 rtc;

// Struct para o SCD30
struct SCD30Data {
  float co2;
  float temperature;
  float humidity;
};

SCD30Data dadosSCD30;

// Struct para o SEN55
struct SEN55Data {
  float massConcentrationPm1p0;
  float massConcentrationPm2p5;
  float massConcentrationPm4p0;
  float massConcentrationPm10p0;
  float ambientHumidity;
  float ambientTemperature;
  float vocIndex;
  float noxIndex;
};

SEN55Data dadosSEN55;

// Dias da semana
char daysOfTheWeek[7][14] = {
  "Domingo", "Segunda-feira", "Terca-feira",
  "Quarta-feira", "Quinta-feira", "Sexta-feira", "Sábado"
};

// Armazenar erros
static char errorMessage[256];
static int16_t error;

// Variável para armazenar o nome do arquivo
String arquivo = "";

// Determina o tempo entre as medidas (milisegundos)
const int intervalo = 30;  // Intervalo desejado em segundos
unsigned long ultimoTempoExecutado = 0;
unsigned int intervaloPorTela = 4000;

// #########################################################
// #                        Funções                        #
// #########################################################

void acendeLed() {
  // Acende o LED
  digitalWrite(LED, HIGH);
  // Intervalo 500ms
  delay(500);
  // Apaga o LED
  digitalWrite(LED, LOW);
}

// Limpar o display
void limparLinhaLCD(int linha) {
  if (linha >= 0 && linha <= 3) {
    lcd.setCursor(0, linha);
    lcd.print("                ");
  } else {
    lcd.clear();
  }
}

// Substitui o ponto por vírgula como separador de casas decimais
String numeroComVirgula(float valor, int casasDecimais = 2) {
  String resultado = String(valor, casasDecimais);
  resultado.replace('.', ',');
  return resultado;
}

// Obter as leituras do sensor SCD30
bool lerSCD30(SCD30Data &dadosSCD30) {
  dadosSCD30.co2 = dadosSCD30.temperature = dadosSCD30.humidity = -1.0;  // Define valores padrão em caso de erro
  error = sensorSCD30.blockingReadMeasurementData(dadosSCD30.co2, dadosSCD30.temperature, dadosSCD30.humidity);
  if (error != NO_ERROR) {
    Serial.print("Erro ao ler SCD30: ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return false;
  } else {
    return true;
  }
}

// Obter as leituras do sensor SEN55
bool lerSEN55(SEN55Data &dadosSEN55) {
  dadosSEN55.massConcentrationPm1p0 = -1;  // Define valores padrão em caso de erro
  dadosSEN55.massConcentrationPm2p5 = -1;
  dadosSEN55.massConcentrationPm4p0 = -1;
  dadosSEN55.massConcentrationPm10p0 = -1;
  dadosSEN55.ambientHumidity = -1;
  dadosSEN55.ambientTemperature = -1;
  dadosSEN55.vocIndex = -1;
  dadosSEN55.noxIndex = -1;

  error = sensorSEN55.readMeasuredValues(
    dadosSEN55.massConcentrationPm1p0, dadosSEN55.massConcentrationPm2p5, dadosSEN55.massConcentrationPm4p0, dadosSEN55.massConcentrationPm10p0,
    dadosSEN55.ambientHumidity, dadosSEN55.ambientTemperature, dadosSEN55.vocIndex, dadosSEN55.noxIndex);

  if (error != NO_ERROR) {
    Serial.print("Erro ao ler SEN55: ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return false;
  } else {
    return true;
  }
}

// Retorna a data e a hora do RTC
String obterDataHora(bool formatoGravacao) {
  DateTime now = rtc.now();

  String yearStr = String(now.year());
  String monthStr = (now.month() < 10 ? "0" : "") + String(now.month());
  String dayStr = (now.day() < 10 ? "0" : "") + String(now.day());
  String hourStr = (now.hour() < 10 ? "0" : "") + String(now.hour());
  String minuteStr = (now.minute() < 10 ? "0" : "") + String(now.minute());
  String secondStr = (now.second() < 10 ? "0" : "") + String(now.second());
  // String dayOfWeek = daysOfTheWeek[now.dayOfTheWeek()];

  String formattedTime = "";

  if (formatoGravacao) {
    formattedTime = yearStr + "/" + monthStr + "/" + dayStr + " " + hourStr + ":" + minuteStr + ":" + secondStr;
  } else {
    formattedTime = dayStr + "/" + monthStr + "/" + yearStr + " " + hourStr + ":" + minuteStr + ":" + secondStr;
  }
  return formattedTime;
}

// Atualizar os dados exibidos no LCD
void atualizaLCD(SCD30Data &dadosSCD30, SEN55Data &dadosSEN55) {
  // Valores do SCD30
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CO2: " + String(dadosSCD30.co2) + " ppm  ");
  lcd.setCursor(0, 1);
  lcd.print("Temp: " + String(dadosSCD30.temperature) + " C ");
  lcd.setCursor(0, 2);
  lcd.print("Umid: " + String(dadosSCD30.humidity) + " % ");
  delay(intervaloPorTela);

  // Valores do SEN55
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PM 1.0: " + String(dadosSEN55.massConcentrationPm1p0));
  lcd.setCursor(0, 1);
  lcd.print("PM 2.5: " + String(dadosSEN55.massConcentrationPm2p5));
  lcd.setCursor(0, 2);
  lcd.print("PM 4.0: " + String(dadosSEN55.massConcentrationPm4p0));
  lcd.setCursor(0, 3);
  lcd.print("PM10.0: " + String(dadosSEN55.massConcentrationPm10p0));
  delay(intervaloPorTela);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: " + String(dadosSEN55.ambientTemperature) + " C ");
  lcd.setCursor(0, 1);
  lcd.print("Umid: " + String(dadosSEN55.ambientHumidity) + " % ");
  lcd.setCursor(0, 2);
  lcd.print("VocIndex: " + String(dadosSEN55.vocIndex));
  lcd.setCursor(0, 3);
  lcd.print("NoxIndex: " + String(dadosSEN55.noxIndex));
  delay(intervaloPorTela);
}

// Atualizar os dados exibidos no terminal
void atualizaTerminal(SCD30Data &dadosSCD30, SEN55Data &dadosSEN55) {
  // Valores do SCD30
  Serial.println("========== SCD30 ==========");
  Serial.print("CO2: ");
  Serial.print(dadosSCD30.co2);
  Serial.print(" ppm\tTemp: ");
  Serial.print(dadosSCD30.temperature);
  Serial.print(" C\tUmidade: ");
  Serial.println(dadosSCD30.humidity);

  // Valores do SEN55
  Serial.println("========== SEN55 ==========");
  Serial.print("Massa PM1.0: ");  // Material Particulado com diâmetro inferior a 1,0 µm
  Serial.print(dadosSEN55.massConcentrationPm1p0);
  Serial.print("\t");
  Serial.print("Massa PM2.5: ");  // Material Particulado com diâmetro inferior a 2,5 µm
  Serial.print(dadosSEN55.massConcentrationPm2p5);
  Serial.print("\t");
  Serial.print("Massa PM4.0: ");  // Material Particulado com diâmetro inferior a 4,0 µm
  Serial.print(dadosSEN55.massConcentrationPm4p0);
  Serial.print("\t");
  Serial.print("Massa PM10.0: ");  // Material Particulado com diâmetro inferior a 10,0 µm
  Serial.print(dadosSEN55.massConcentrationPm10p0);
  Serial.print("\t");
  Serial.print("Umidade:");
  if (isnan(dadosSEN55.ambientHumidity)) {
    Serial.print("n/a");
  } else {
    Serial.print(dadosSEN55.ambientHumidity);
  }
  Serial.print("\t");
  Serial.print("Temperatura:");
  if (isnan(dadosSEN55.ambientTemperature)) {
    Serial.print("n/a");
  } else {
    Serial.print(dadosSEN55.ambientTemperature);
  }
  Serial.print("\t");
  Serial.print("VocIndex:");
  if (isnan(dadosSEN55.vocIndex)) {
    Serial.print("n/a");
  } else {
    Serial.print(dadosSEN55.vocIndex);
  }
  Serial.print("\t");
  Serial.print("NoxIndex:");
  if (isnan(dadosSEN55.noxIndex)) {
    Serial.println("n/a");
  } else {
    Serial.println(dadosSEN55.noxIndex);
  }
}

// Atualiza os dados no arquivo atual
void atualizaArquivo(String arquivo, SCD30Data &dadosSCD30, SEN55Data &dadosSEN55, String dataAtual) {
  // Ordem de valores da string: data;co2;scd30_temp;scd30_umid;pm1.0;pm2.5;pm4.0;pm10.0;sen55_umid;sen55_temp;vocindex;noxindex
  String valores = "";
  valores += dataAtual + ";";
  valores += String(numeroComVirgula(dadosSCD30.co2)) + ";";
  valores += String(numeroComVirgula(dadosSCD30.temperature)) + ";";
  valores += String(numeroComVirgula(dadosSCD30.humidity)) + ";";
  valores += String(numeroComVirgula(dadosSEN55.massConcentrationPm1p0)) + ";";
  valores += String(numeroComVirgula(dadosSEN55.massConcentrationPm2p5)) + ";";
  valores += String(numeroComVirgula(dadosSEN55.massConcentrationPm4p0)) + ";";
  valores += String(numeroComVirgula(dadosSEN55.massConcentrationPm10p0)) + ";";
  valores += String(numeroComVirgula(dadosSEN55.ambientHumidity)) + ";";
  valores += String(numeroComVirgula(dadosSEN55.ambientTemperature)) + ";";
  valores += String(numeroComVirgula(dadosSEN55.vocIndex)) + ";";
  valores += String(numeroComVirgula(dadosSEN55.noxIndex)) + ";";
  valores += "\n";
  appendFile(SD, arquivo.c_str(), valores.c_str());
}

// Criar um novo arquivo no cartão SD
void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// Agregar dados em um arquivo
void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

// Obtém o nome do próximo arquivo
String getNextFileName(fs::FS &fs, const char *prefix, const char *extension) {
  File root = fs.open("/");
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open root directory");
    return "";
  }

  int maxIndex = 0;

  File file = root.openNextFile();
  while (file) {
    String name = file.name();  // exemplo: /medidas-3.csv

    // Remove o '/' inicial se existir
    if (name.startsWith("/")) {
      name = name.substring(1);
    }

    if (name.startsWith(prefix) && name.endsWith(extension)) {
      int dashIndex = name.indexOf('-');
      int dotIndex = name.lastIndexOf('.');

      if (dashIndex != -1 && dotIndex != -1 && dotIndex > dashIndex) {
        String numberStr = name.substring(dashIndex + 1, dotIndex);
        int number = numberStr.toInt();
        if (number > maxIndex) {
          maxIndex = number;
        }
      }
    }

    file = root.openNextFile();
  }

  int nextIndex = maxIndex + 1;
  return String("/") + prefix + "-" + nextIndex + extension;
}

// #########################################################
// #                         Setup                         #
// #########################################################

// Rede wifi
const char* ssid = "Conforto-Termico";
const char* password = "master1466";

// Config MQTT
const char* mqtt_server = "192.168.50.20";
const int   mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void publicarMQTT(SCD30Data &dadosSCD30, SEN55Data &dadosSEN55) {
  StaticJsonDocument<900> doc;

  doc["co2"]         = dadosSCD30.co2;
  doc["temperatura"] = dadosSCD30.temperature;
  doc["umidade"]     = dadosSCD30.humidity;

  doc["pm1.0"]  = dadosSEN55.massConcentrationPm1p0;
  doc["pm2.5"]  = dadosSEN55.massConcentrationPm2p5;
  doc["pm4.0"]  = dadosSEN55.massConcentrationPm4p0;
  doc["pm10.0"] = dadosSEN55.massConcentrationPm10p0;

  if (!isnan(dadosSEN55.vocIndex)) {
    doc["vocIndex"] = dadosSEN55.vocIndex;
  } 

  if (!isnan(dadosSEN55.noxIndex)) {
    doc["noxIndex"] = dadosSEN55.noxIndex;
  }

  char payload[900];
  serializeJson(doc, payload);

  client.publish("sensores/esp32", payload);
}

void mqtt_reconnect() {
  while (!client.connected()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Reconectando MQTT...");
    Serial.print("Reconectando MQTT...");
    delay(1000);

    if (client.connect("ESP32Client", "admin", "paulvandyk11")) {
      Serial.println("Conectado ao MQTT!");
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Conectado ao MQTT!");
      client.subscribe("comandos/esp32");
    } else {
      lcd.clear();
      lcd.print("Falhou, rc=");
      Serial.print("Falhou, rc=");
      lcd.setCursor(0,2);
      lcd.print(client.state());
      Serial.print(client.state());
      lcd.setCursor(0,3);
      lcd.print("Retry in 5 sec...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }

  client.setServer(mqtt_server, mqtt_port);

  // Inicializa a chave para leitura de dados
  pinMode(CHAVE, INPUT_PULLUP);

  // Inicializa o LED indicador de leitura
  pinMode(LED, OUTPUT);

  // Inicializa o primeiro barramento I2C manualmente
  Wire.begin(SDA_1, SCL_1);
  delay(500);

  // Inicializa o segundo barramento I2C
  I2Ctwo.begin(SDA_2, SCL_2);
  delay(500);

  // Inicializa o display LCD no primeiro barramento I2C
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");
  delay(500);

  // Inicializa o TinyRTC no primeiro barramento I2C
  rtc.begin(&Wire);
  delay(600);

  if (!rtc.begin()) {
    Serial.println("Não foi possível encontrar o RTC.");
    lcd.setCursor(0, 1);
    lcd.print("TinyRTC nao encontrado");
    Serial.flush();
    while (1) delay(10);
  } else {
    lcd.setCursor(0, 1);
    lcd.print("                    ");  // 20 espaços
    lcd.setCursor(0, 1);
    lcd.print("TinyRTC OK");
  }

  delay(500);

  if (!rtc.isrunning()) {
    Serial.println("O RTC parado, vamos definir o horário!");
    limparLinhaLCD(1);  // 20 espaços
    lcd.setCursor(0, 1);
    lcd.print("Definindo o horario...");
    delay(500);
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  // Inicializa o sensor SCD30 no segundo barramento I2C ---------------------------
  sensorSCD30.begin(I2Ctwo, SCD30_I2C_ADDR_61);
  sensorSCD30.stopPeriodicMeasurement();
  sensorSCD30.softReset();
  delay(2000);

  error = sensorSCD30.startPeriodicMeasurement(0);
  if (error != NO_ERROR) {
    Serial.print("Erro ao iniciar medição periódica: ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return;
  } else {
    lcd.setCursor(0, 2);
    lcd.print("SCD30 OK");
  }

  // Inicializa o sensor SEN55 no primeiro barramento I2C ---------------------------
  sensorSEN55.begin(Wire);

  error = sensorSEN55.deviceReset();
  if (error) {
    Serial.print("Error trying to execute deviceReset(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
  }

  float tempOffset = 0.0;
  error = sensorSEN55.setTemperatureOffsetSimple(tempOffset);
  if (error) {
    Serial.print("Error trying to execute setTemperatureOffsetSimple(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.print("Temperature Offset set to ");
    Serial.print(tempOffset);
    Serial.println(" deg. Celsius (SEN54/SEN55 only");
    lcd.setCursor(0, 3);
    lcd.print("SEN55 OK");
  }

  // Start Measurement
  error = sensorSEN55.startMeasurement();
  if (error) {
    Serial.print("Error trying to execute startMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  delay(500);
  lcd.clear();

  // Inicializa o módulo SC
  if (!SD.begin(5)) {
    Serial.println("Falha ao montar cartão SD");
    lcd.setCursor(0, 0);
    lcd.print("SD erro");
    return;
  }

  // Verifica se o cartão SD está inserido
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    lcd.setCursor(0, 0);
    lcd.print("SD inexistente");
    return;
  }

  // Verifica o nome do último arquivo e cria o próximo
  arquivo = getNextFileName(SD, "medidas", ".csv");
  writeFile(SD, arquivo.c_str(), "data;co2;scd30_temp;scd30_umid;pm1.0;pm2.5;pm4.0;pm10.0;sen55_umid;sen55_temp;vocindex;noxindex\n");
  Serial.print("Próximo nome de arquivo: ");
  Serial.println(arquivo);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nome do arquivo");
  lcd.setCursor(0, 1);
  lcd.print(String(arquivo));

  Serial.begin(115200);
  delay(1000);

  Serial.println("Conectando wiFi...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando wiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado!");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi conectado!");
  Serial.print("Endereço IP: ");
  lcd.setCursor(0, 1);
  lcd.print("Endereco IP: ");
  lcd.setCursor(0, 2);
  lcd.print(WiFi.localIP());
  Serial.println(WiFi.localIP());

  delay(5000);
}

void loop() {
  // Lê o estado da chave
  int estadoChave = digitalRead(CHAVE);

  if (estadoChave == LOW) {
    DateTime agora = rtc.now();
    unsigned long tempoAtual = agora.unixtime();  // Tempo atual em segundos
    if (tempoAtual - ultimoTempoExecutado >= intervalo) {
      ultimoTempoExecutado = tempoAtual;

      if (!client.connected()) {
        mqtt_reconnect();
      }
      client.loop();

      // Obtém as leituras e atualiza as variáveis
      if (lerSCD30(dadosSCD30) && lerSEN55(dadosSEN55)) {
        // pisca o led indicando que entrou no loop
        acendeLed();
        // atualiza os dados no terminal
        atualizaTerminal(dadosSCD30, dadosSEN55);

        // publica dados no broker mqtt
        publicarMQTT(dadosSCD30, dadosSEN55);

        // atualiza os dados no arquivo atual
        atualizaArquivo(arquivo, dadosSCD30, dadosSEN55, obterDataHora(true));
        // atualiza os dados no LCD
        atualizaLCD(dadosSCD30, dadosSEN55);
        // limpa o LCD e posiciona o cursos
        lcd.clear();
        lcd.setCursor(0, 0);
        // exibe informações do arquivo gravado
        lcd.print("Dados gravados:");
        lcd.setCursor(0, 1);
        lcd.print(String(arquivo));
        delay(intervaloPorTela);
      } else {
        lcd.clear();
        Serial.println("Falha na leitura dos sensores.");
        lcd.setCursor(0, 0);
        lcd.print("Erro na leitura!");
        delay(intervaloPorTela);
      }
      controleLCD = 0;
    } else {
      // Limpa o LCD e evita de piscar ao limpar somente uma vez
      if (controleLCD == 0) {
        lcd.clear();
        controleLCD = 1;
      }
      lcd.setCursor(0, 0);
      lcd.print("Proxima leitura:");
      float tempoProximaLeitura = intervalo - (tempoAtual - ultimoTempoExecutado);
      lcd.setCursor(0, 1);
      lcd.print(String(tempoProximaLeitura, 0) + " segundos");
    }
    delay(500);
  } else {
    // Limpa o LCD e evita de piscar ao limpar somente uma vez
    if (controleLCD == 0) {
      lcd.clear();
      controleLCD = 1;
    }
    String horaAtual = obterDataHora(false);
    Serial.println("Leitura desligada");
    lcd.setCursor(0, 0);
    lcd.print("IFSC Campus Sao Jose");
    lcd.setCursor(0, 1);
    lcd.print("Leitura desligada");
    lcd.setCursor(0, 2);
    lcd.print(String(horaAtual));
    delay(500);
  }
}
