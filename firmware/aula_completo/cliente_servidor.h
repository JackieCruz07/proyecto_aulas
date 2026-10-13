// WiFi y comunicación con el servidor (conteo y tarjetas)
#pragma once

namespace ClienteServidor {
  void iniciar();                       // conecta al WiFi y lanza la tarea de envío
  bool hayWifi();
  void enviarTarjeta(const char* uid);  // la pone en cola; se envía en segundo plano
}
