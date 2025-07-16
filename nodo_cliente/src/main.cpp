#include "config.h"
#define MQTT_MAX_PACKET_SIZE 512
#include <PubSubClient.h>
#include <WiFi.h>
#include <tinycbor.h>

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
const int MSG_INTERVAL = 2000;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect(ESTACION_ID)) {
      Serial.println("conectado");
      bool subOk = client.subscribe("/datos/resumen");
      if (subOk) {
        Serial.println("✅ Suscripción exitosa a /datos/resumen");
      } else {
        Serial.println("❌ Falló la suscripción");
      }
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

uint8_t buffer[512]; // CBOR buffer
CborEncoder encoder, mapEncoder, locEncoder;
void publicarDatosSensor() {
  float temperatura = random(10, 40);
  float humedad = random(0, 100);
  float viento = random(0, 40);
  float sensacionTermica = temperatura - (viento * 0.2);
  float presion = random(980, 1050);
  float lluvia = random(0, 10);
  float radiacion = random(0, 1000);
  float calidadAire = random(0, 500);

  cbor_encoder_init(&encoder, buffer, sizeof(buffer), 0);
  cbor_encoder_create_map(&encoder, &mapEncoder, CborIndefiniteLength);

  cbor_encode_text_stringz(&mapEncoder, "station_id");
  cbor_encode_text_stringz(&mapEncoder, ESTACION_ID);

  if (ENVIAR_TEMPERATURA) {
    cbor_encode_text_stringz(&mapEncoder, "temperatura");
    cbor_encode_float(&mapEncoder, temperatura);
  }
  if (ENVIAR_HUMEDAD) {
    cbor_encode_text_stringz(&mapEncoder, "humedad");
    cbor_encode_float(&mapEncoder, humedad);
  }
  if (ENVIAR_SENSACION_TERMICA) {
    cbor_encode_text_stringz(&mapEncoder, "sensacion_termica");
    cbor_encode_float(&mapEncoder, sensacionTermica);
  }
  if (ENVIAR_PRESION) {
    cbor_encode_text_stringz(&mapEncoder, "presion");
    cbor_encode_float(&mapEncoder, presion);
  }
  if (ENVIAR_LLUVIA) {
    cbor_encode_text_stringz(&mapEncoder, "lluvia");
    cbor_encode_float(&mapEncoder, lluvia);
  }
  if (ENVIAR_VIENTO) {
    cbor_encode_text_stringz(&mapEncoder, "viento");
    cbor_encode_float(&mapEncoder, viento);
  }
  if (ENVIAR_RADIACION_SOLAR) {
    cbor_encode_text_stringz(&mapEncoder, "radiacion_solar");
    cbor_encode_float(&mapEncoder, radiacion);
  }
  if (ENVIAR_CALIDAD_AIRE) {
    cbor_encode_text_stringz(&mapEncoder, "calidad_aire");
    cbor_encode_float(&mapEncoder, calidadAire);
  }

  cbor_encode_text_stringz(&mapEncoder, "localidad");
  cbor_encode_text_stringz(&mapEncoder, LOCALIDAD);

  cbor_encode_text_stringz(&mapEncoder, "ubicacion");
  cbor_encoder_create_map(&mapEncoder, &locEncoder, 2);
  cbor_encode_text_stringz(&locEncoder, "lat");
  cbor_encode_float(&locEncoder, LATITUD);
  cbor_encode_text_stringz(&locEncoder, "lon");
  cbor_encode_float(&locEncoder, LONGITUD);
  cbor_encoder_close_container(&mapEncoder, &locEncoder);

  cbor_encoder_close_container(&encoder, &mapEncoder);

  size_t encodedLength = cbor_encoder_get_buffer_size(&encoder, buffer);

  String topic = "barrio/estaciones/" + String(CLAVE);

  Serial.print("Publicando en el canal: ");
  Serial.println(topic);

  client.publish(topic.c_str(), buffer, encodedLength);
}

// Umbrales para activar sistemas
#define TEMP_THRESHOLD 20.0 // Temperatura menor a 20°C activa calefacción
#define HUMIDITY_THRESHOLD 40.0 // Humedad menor a 40% activa humidificador

// Función auxiliar para parsear datos CBOR
void parseCborWeatherData(byte *payload, unsigned int length) {
  CborParser parser;
  CborValue it;
  CborError err = cbor_parser_init(payload, length, 0, &parser, &it);
  if (err != CborNoError || !cbor_value_is_map(&it)) {
    Serial.printf("❌ Error al inicializar el parser CBOR: %d\n", err);
    return;
  }

  CborValue mapIt;
  err = cbor_value_enter_container(&it, &mapIt);
  if (err != CborNoError) {
    Serial.printf("❌ Error al entrar al mapa de localidades: %d\n", err);
    return;
  }

  while (cbor_value_is_valid(&mapIt) && !cbor_value_at_end(&mapIt)) {
    // --- Leer clave de localidad ---
    if (!cbor_value_is_text_string(&mapIt)) {
      Serial.println("❌ Se esperaba una clave string (localidad).");
      cbor_value_advance(&mapIt);
      continue;
    }

    char localidad[64];
    size_t locLen = sizeof(localidad);
    err = cbor_value_copy_text_string(&mapIt, localidad, &locLen, &mapIt);
    if (err != CborNoError) {
      Serial.printf("❌ Error al leer la clave de localidad: %d\n", err);
      cbor_value_advance(&mapIt);
      continue;
    }

    Serial.print("🌍 Localidad: ");
    Serial.println(localidad);

    // --- Verificar que el valor es un mapa ---
    if (!cbor_value_is_map(&mapIt)) {
      Serial.println("❌ Se esperaba un objeto/mapa como valor de localidad.");
      cbor_value_advance(&mapIt);
      continue;
    }

    // --- Procesar datos meteorológicos ---
    CborValue dataIt;
    err = cbor_value_enter_container(&mapIt, &dataIt);
    if (err != CborNoError) {
      Serial.printf("❌ Error al entrar al mapa de datos: %d\n", err);
      cbor_value_advance(&mapIt);
      continue;
    }

    double temperatura = 0.0;
    double humedad = 0.0;
    bool has_temp = false, has_humidity = false;

    while (cbor_value_is_valid(&dataIt) && !cbor_value_at_end(&dataIt)) {
      if (!cbor_value_is_text_string(&dataIt)) {
        Serial.println("❌ Se esperaba clave de campo tipo string.");
        cbor_value_advance(&dataIt);
        continue;
      }

      char campo[64];
      size_t campoLen = sizeof(campo);
      err = cbor_value_copy_text_string(&dataIt, campo, &campoLen, &dataIt);
      if (err != CborNoError) {
        Serial.printf("❌ Error al leer nombre de campo: %d\n", err);
        cbor_value_advance(&dataIt);
        continue;
      }

      // --- Leer valor ---
      if (cbor_value_is_double(&dataIt) || cbor_value_is_float(&dataIt) || cbor_value_is_half_float(&dataIt)) {
        double val;
        err = cbor_value_get_double(&dataIt, &val);
        if (err == CborNoError) {
          Serial.printf("  📈 %s: %.2f\n", campo, val);
          // Guardar temperatura y humedad para verificación de umbrales
          if (strcmp(campo, "temperatura") == 0) {
            temperatura = val;
            has_temp = true;
          } else if (strcmp(campo, "humedad") == 0) {
            humedad = val;
            has_humidity = true;
          }
        } else {
          Serial.printf("  ⚠️ Error al leer valor double para %s: %d\n", campo, err);
        }
      } else if (cbor_value_is_integer(&dataIt)) {
        int valInt;
        err = cbor_value_get_int(&dataIt, &valInt);
        if (err == CborNoError) {
          Serial.printf("  🔢 %s: %d\n", campo, valInt);
        } else {
          Serial.printf("  ⚠️ Error al leer valor entero para %s: %d\n", campo, err);
        }
      } else {
        Serial.printf("  ⚠️ Campo no reconocido o tipo no soportado: %s\n", campo);
      }

      cbor_value_advance(&dataIt);
    }

    err = cbor_value_leave_container(&mapIt, &dataIt);
    if (err != CborNoError) {
      Serial.printf("❌ Error al salir del mapa de datos: %d\n", err);
      cbor_value_advance(&mapIt);
      continue;
    }

    // --- Verificar umbrales ---
    if (has_temp && temperatura < TEMP_THRESHOLD) {
      Serial.println("🔥 Prendiendo sistema de calefacción");
    }
    if (has_humidity && humedad < HUMIDITY_THRESHOLD) {
      Serial.println("💧 Prendiendo humidificador");
    }
  }

  err = cbor_value_leave_container(&it, &mapIt);
  if (err != CborNoError) {
    Serial.printf("❌ Error al salir del mapa de localidades: %d\n", err);
  }
}

// Callback principal
void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("📥 Mensaje recibido en el tópico: ");
  Serial.println(topic);

  // Filtrar por tópico
  if (String(topic) == "/datos/resumen") {
    parseCborWeatherData(payload, length);
  }
}




void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > MSG_INTERVAL) {
    lastMsg = now;
    publicarDatosSensor();
  }
}
