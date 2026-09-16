/*
  ESP32-C3 + DHT11 -> ThingSpeak
  Le temperatura e umidade e envia pro ThingSpeak periodicamente.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

const char* ssid = "SSID_DA_REDE_WIFI_AQUI";
const char* password = "PASSWORD_DA_REDE_WIFI_AQUI";

#define DHTPIN 4       // GPIO onde o pino de dados do DHT11 esta ligado
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ThingSpeak
const char* thingSpeakApiKey = "SUA_WRITE_API_KEY_AQUI";
const char* thingSpeakServer = "http://api.thingspeak.com/update";

unsigned long ultimoEnvio = 0;
const unsigned long intervaloEnvio = 20000; // ThingSpeak (free) exige >= 15s entre updates

void conectarWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm); // fix que resolveu o problema de conexao nessa placa
  WiFi.disconnect();
  delay(100);

  Serial.printf("Conectando em: %s\n", ssid);
  WiFi.begin(ssid, password);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha ao conectar Wi-Fi.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  dht.begin();
  conectarWiFi();
}

void loop() {
  // Reconecta automaticamente se cair no meio do caminho
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi caiu, tentando reconectar...");
    conectarWiFi();
  }

  if (millis() - ultimoEnvio >= intervaloEnvio) {
    float umidade = dht.readHumidity();
    float temperatura = dht.readTemperature();

    if (isnan(umidade) || isnan(temperatura)) {
      Serial.println("Falha ao ler o DHT11! Confira a fiacao.");
    } else {
      Serial.printf("Temperatura: %.1f C  Umidade: %.1f %%\n", temperatura, umidade);

      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = String(thingSpeakServer) + "?api_key=" + thingSpeakApiKey +
                      "&field1=" + String(temperatura) +
                      "&field2=" + String(umidade);

        http.begin(url);
        int httpCode = http.GET();

        if (httpCode > 0) {
          Serial.printf("ThingSpeak respondeu: %d (numero da entrada, 0 = erro)\n", httpCode);
        } else {
          Serial.printf("Erro ao enviar pro ThingSpeak: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
      }
    }

    ultimoEnvio = millis();
  }
}