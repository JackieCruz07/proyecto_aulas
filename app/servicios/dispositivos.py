"""Lo que reportan los ESP32: conteo de personas y tarjetas RFID."""
from flask import current_app

from ..db import obtener_db
from ..errores import ErrorApi
from ..repositorios import aulas, conteos, docentes, registros
from ..utilidades import ahora, limpiar_aula, limpiar_uid, segundos_desde


def registrar_conteo(datos):
    aula = limpiar_aula(datos.get("aula"))
    personas = datos.get("personas")
    if not aula or not isinstance(personas, int) or isinstance(personas, bool) or personas < 0:
        raise ErrorApi("Datos inválidos", 400)

    fecha = ahora()
    aulas.asegurar(aula)
    aulas.registrar_lectura(aula, personas, fecha)
    conteos.insertar(aula, personas, fecha)
    obtener_db().commit()


def registrar_tarjeta(datos):
    """Si el docente estaba dentro del aula registra su salida; si no, su entrada."""
    aula = limpiar_aula(datos.get("aula"))
    uid = limpiar_uid(datos.get("uid"))
    if not aula or not uid:
        raise ErrorApi("Datos inválidos", 400)

    aulas.asegurar(aula)
    docente = docentes.por_uid(uid)
    if docente is None:
        obtener_db().commit()
        raise ErrorApi("Tarjeta no registrada", 404, uid=uid)

    # Antirrebote: una tarjeta que se queda cerca del lector no cuenta dos veces
    ultimo = registros.ultimo_evento(docente["id"])
    if ultimo and segundos_desde(ultimo) < current_app.config["SEGUNDOS_ANTIREBOTE"]:
        return {"accion": "ignorado", "docente": docente["nombre"]}

    fecha = ahora()
    abierto = registros.abierto_de_docente(docente["id"])
    if abierto is not None and abierto["aula_id"] == aula:
        registros.cerrar(abierto["id"], fecha)
        accion = "salida"
    else:
        if abierto is not None:   # se le olvidó registrar la salida en otra aula
            registros.cerrar(abierto["id"], fecha)
        registros.abrir(aula, docente["id"], fecha)
        accion = "entrada"

    obtener_db().commit()
    return {"accion": accion, "docente": docente["nombre"], "hora": fecha}
