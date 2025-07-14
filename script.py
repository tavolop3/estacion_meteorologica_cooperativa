import paho.mqtt.client as mqtt
import cbor2
import time
import random

MQTT_BROKER = "localhost"  # Cambia esto por la dirección de tu broker MQTT
MQTT_PORT = 1883
MQTT_TOPIC = "barrio/estaciones/datos"
ESTACION_ID = "simulacion"
MSG_INTERVAL_SECONDS = 2  
LOCALIDAD = "TOLOSA"

BASE_LAT = -34.9214
BASE_LON = -57.9545

def on_connect(client, userdata, flags, rc):
    """Función que se ejecuta al conectar al broker."""
    if rc == 0:
        print(f"✅ Conectado exitosamente al broker MQTT en {MQTT_BROKER}")
    else:
        print(f"❌ Falló la conexión, código de retorno: {rc}")

def create_cbor_payload() -> bytes:
    """
    Crea el payload de datos en formato CBOR, imitando la estructura del ESP32.
    """
    temperatura = round(random.uniform(10.0, 40.0), 2)
    humedad = round(random.uniform(0.0, 100.0), 2)
    
    latitud_variada = BASE_LAT + random.uniform(-0.005, 0.005)
    longitud_variada = BASE_LON + random.uniform(-0.005, 0.005)

    data_dict = {
        "station_id": ESTACION_ID,
        "temperatura": temperatura,
        "humedad": humedad,
        "localidad": LOCALIDAD,
        "ubicacion": {
            "lat": round(latitud_variada, 6),
            "lon": round(longitud_variada, 6)
        }
    }
    
    return cbor2.dumps(data_dict)

def main():
    """Función principal para inicializar el cliente y publicar datos."""
    client = mqtt.Client(client_id=ESTACION_ID)
    client.on_connect = on_connect

    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
    except ConnectionRefusedError:
        print("❌ Error: Conexión rechazada. ¿Está el broker MQTT funcionando?")
        return
    except Exception as e:
        print(f"❌ Ocurrió un error al conectar: {e}")
        return

    client.loop_start() 

    try:
        while True:
            payload = create_cbor_payload()
            
            result = client.publish(MQTT_TOPIC, payload)
            result.wait_for_publish() 

            if result.is_published():
                 print(f"🛰️  Mensaje enviado al tópico '{MQTT_TOPIC}'")
            else:
                 print(f"⚠️  El mensaje no pudo ser enviado.")

            time.sleep(MSG_INTERVAL_SECONDS)

    except KeyboardInterrupt:
        print("\n🛑 Simulación detenida por el usuario.")
    finally:
        client.loop_stop()
        client.disconnect()
        print("🔌 Cliente desconectado.")


if __name__ == "__main__":
    main()