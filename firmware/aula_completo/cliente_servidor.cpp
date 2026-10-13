#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"
#include "conteo.h"
#include "cliente_servidor.h"

namespace {

constexpr int LARGO_UID = 24;
QueueHandle_t colaTarjetas;

// Devuelve true si el servidor respondió (aunque sea con un error); false si no hubo conexión
bool enviarTarjetaAlServidor(const char* uid) {
  HTTPClient http;
  http.begin(String(SERVIDOR) + "/api/rfid");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", API_KEY);
  http.setTimeout(3000);
  int codigo = http.POST("{\"aula\":\"" + String(AULA_ID) + "\",\"uid\":\"" + String(uid) + "\"}");
  String respuesta = (codigo > 0) ? http.getString() : "";
  http.end();

  Serial.printf("RFID -> servidor: codigo=%d %s\n", codigo, respuesta.c_str());
  if (codigo == 200) {
    if (respuesta.indexOf("\"entrada\"") >= 0)      Serial.println(">> Docente: ENTRADA registrada");
    else if (respuesta.indexOf("\"salida\"") >= 0)  Serial.println("<< Docente: SALIDA registrada");
    else if (respuesta.indexOf("ignorado") >= 0)    Serial.println("Ignorada: esa tarjeta se paso hace muy poco. Espera unos segundos.");
  }
  if (codigo == 404) Serial.printf("Tarjeta no registrada. Agrega este UID en el panel: %s\n", uid);
  if (codigo == 401) Serial.println("La API_KEY no coincide con la del servidor.");
  return codigo > 0;
}

void enviarConteoAlServidor() {
  int n = Conteo::personas();
  HTTPClient http;
  http.begin(String(SERVIDOR) + "/api/conteo");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", API_KEY);
  http.setTimeout(3000);
  int codigo = http.POST("{\"aula\":\"" + String(AULA_ID) + "\",\"personas\":" + String(n) + "}");
  http.end();
  Serial.printf("Conteo -> servidor: personas=%d, codigo=%d\n", n, codigo);
}

// Tarea aparte: así el radar nunca se queda esperando a la red
void tareaEnvio(void*) {
  unsigned long ultimoConteo = 0;
  int intentosTarjeta = 0;

  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      // 1) Primero las tarjetas, que son lo más urgente
      char uid[LARGO_UID];
      if (xQueuePeek(colaTarjetas, uid, 0) == pdTRUE) {
        if (enviarTarjetaAlServidor(uid)) {
          xQueueReceive(colaTarjetas, uid, 0);   // enviada: se quita de la cola
          intentosTarjeta = 0;
        } else if (++intentosTarjeta >= 3) {
          xQueueReceive(colaTarjetas, uid, 0);   // no hubo manera: se descarta
          Serial.println("No se pudo contactar al servidor: tarjeta descartada.");
          intentosTarjeta = 0;
        } else {
          vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
      }

      // 2) El conteo, cuando cambia o cada minuto
      bool tocaLatido = millis() - ultimoConteo > INTERVALO_LATIDO_MS;
      if (Conteo::hayCambioPendiente() || tocaLatido) {
        Conteo::limpiarCambioPendiente();
        enviarConteoAlServidor();
        ultimoConteo = millis();
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

}  // namespace

namespace ClienteServidor {

void iniciar() {
  colaTarjetas = xQueueCreate(5, LARGO_UID);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_NOMBRE, WIFI_CLAVE);
  Serial.print("Conectando al WiFi");
  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < TIEMPO_WIFI_MS) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nConectado. IP del ESP32: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nSin WiFi por ahora; el conteo sigue y se enviará cuando haya conexión.");
  }

  // En el núcleo 0 (donde vive el WiFi), para no estorbar la lectura del radar y las tarjetas
  xTaskCreatePinnedToCore(tareaEnvio, "envio", 12288, NULL, 1, NULL, 0);
}

bool hayWifi() { return WiFi.status() == WL_CONNECTED; }

void enviarTarjeta(const char* uid) {
  xQueueSend(colaTarjetas, uid, 0);
}

}  // namespace ClienteServidor
