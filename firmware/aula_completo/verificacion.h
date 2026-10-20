// Fusión radar + ToF: decide el conteo oficial de personas.
//
//   ENTRADA: la propone el ToF (A->B en la puerta) y la confirma el radar
//            (la persona se aleja del radar: cerca -> lejos).
//   SALIDA:  la propone el radar (se acerca al radar: lejos -> cerca) y la
//            confirma el ToF (cruza la puerta: B -> A).
//
// Lo que no confirma el otro sensor se descarta y se anota (sin_confirmar):
// alguien que solo se asomó a la puerta o que se acercó al radar y regresó
// no cuenta. Si un sensor se cae, el otro cuenta solo (con aviso).
#pragma once

namespace Verificacion {
  void actualizar();                 // llamar en cada vuelta de loop()
  void reiniciar();                  // pone todos los contadores en 0
  int  personas();                   // conteo oficial (fusionado)
  unsigned long confirmados();       // cruces validados por los dos sensores
  unsigned long radarSolo();         // cruces que solo vio el radar (descartados o sin ToF)
  unsigned long tofSolo();           // cruces que solo vio el ToF (descartados o sin radar)
  unsigned long sinConfirmar();      // radarSolo + tofSolo
  const char* modo();                // "radar+tof", "solo tof (radar caido)" o "solo radar (sin tof)"
  bool hayCambioPendiente();         // true si el conteo oficial o sin_confirmar cambió y aún no se envía
  void limpiarCambioPendiente();
}
