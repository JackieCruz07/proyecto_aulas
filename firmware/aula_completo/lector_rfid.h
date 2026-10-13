// Lector RFID RC522: detecta tarjetas y entrega su UID
#pragma once
#include <stddef.h>

namespace LectorRfid {
  void iniciar();
  // Si hay una tarjeta nueva, escribe su UID en 'uid' (hexadecimal en mayúsculas,
  // ej. "6336D505") y devuelve true. 'tam' debe ser al menos 21.
  bool leerTarjeta(char* uid, size_t tam);
  bool responde();   // true si el chip del lector contesta (para diagnóstico)
}
