#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#define DHTPIN D4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define LED_VERMELHO D5
#define LED_VERDE D6

const char* ssid = "SENAC";
const char* password = "x1y2z3@snc";

const char* mqtt_server = "zfeemqtt.eastus.cloudapp.azure.com";
const int mqtt_port = 41883;
const char* mqtt_user = "Senac";
const char* mqtt_password = "Senac";
const char* mqtt_client_id = "Senac18";

const char* topico_publicacao_temperatura = "sensor/temperatura";
const char* topico_publicacao_umidade = "sensor/umidade";
const char* topico_comando = "sensor/comando";
const char* topico_comando_luz = "sensor/comando/luz";
const char* topico_status = "sensor/status";

float limite_temperatura = 30.0;

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("Mensagem: " + message);

  if (String(topic) == topico_comando) {
    if (message.indexOf("limite") >= 0) {
      String valor_limite = message.substring(message.indexOf(":") + 1);
      limite_temperatura = valor_limite.toFloat();
      Serial.print("Novo limite de temperatura: ");
      Serial.println(limite_temperatura);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
      Serial.println("Conectado");
      client.subscribe(topico_comando);
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float temperatura = dht.readTemperature();

  if (isnan(temperatura)) {
    Serial.println("Falha na leitura do DHT!");
    return;
  }

  if (temperatura > limite_temperatura) {
    digitalWrite(LED_VERMELHO, HIGH);
    digitalWrite(LED_VERDE, LOW);
  } else {
    digitalWrite(LED_VERMELHO, LOW);
    digitalWrite(LED_VERDE, HIGH);
  }

  String jsonPayload = "{\"temperatura\":";
  jsonPayload += temperatura;
  jsonPayload += ", \"limite\":";
  jsonPayload += limite_temperatura;
  jsonPayload += "}";

  client.publish(topico_publicacao_temperatura, jsonPayload.c_str());
  Serial.println(jsonPayload);
  delay(2000);
}
