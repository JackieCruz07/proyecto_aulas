"""Altas, cambios y bajas que se hacen desde el panel."""
import sqlite3

from ..db import obtener_db
from ..errores import ErrorApi
from ..repositorios import aulas, conteos, docentes, registros
from ..utilidades import ahora, limpiar_aula, limpiar_uid

UID_REPETIDO = "Ese UID ya está registrado con otro docente."


def _datos_docente(datos):
    nombre = str(datos.get("nombre") or "").strip()
    uid = limpiar_uid(datos.get("uid"))
    if not nombre or not uid:
        raise ErrorApi("Escribe el nombre y el UID de la tarjeta.", 400)
    return nombre, uid


def crear_docente(datos):
    nombre, uid = _datos_docente(datos)
    try:
        docentes.crear(nombre, uid)
        obtener_db().commit()
    except sqlite3.IntegrityError:
        raise ErrorApi(UID_REPETIDO, 409)


def editar_docente(docente_id, datos):
    nombre, uid = _datos_docente(datos)
    if not docentes.existe(docente_id):
        raise ErrorApi("El docente no existe.", 404)
    try:
        docentes.actualizar(docente_id, nombre, uid)
        obtener_db().commit()
    except sqlite3.IntegrityError:
        raise ErrorApi(UID_REPETIDO, 409)


def eliminar_docente(docente_id):
    # Si estaba dentro de un aula se cierra su registro; el historial se conserva
    registros.cerrar_abiertos_de_docente(docente_id, ahora())
    docentes.eliminar(docente_id)
    obtener_db().commit()


def crear_aula(datos):
    aula = limpiar_aula(datos.get("id"))
    nombre = str(datos.get("nombre") or "").strip() or aula
    if not aula:
        raise ErrorApi("Escribe el identificador del aula (por ejemplo D103).", 400)
    aulas.guardar(aula, nombre)
    obtener_db().commit()


def renombrar_aula(aula_id, datos):
    nombre = str(datos.get("nombre") or "").strip()
    if not nombre:
        raise ErrorApi("Escribe un nombre para el aula.", 400)
    if not aulas.renombrar(limpiar_aula(aula_id), nombre):
        raise ErrorApi("El aula no existe.", 404)
    obtener_db().commit()


def eliminar_aula(aula_id):
    aula = limpiar_aula(aula_id)
    conteos.eliminar_de_aula(aula)
    registros.eliminar_de_aula(aula)
    aulas.eliminar(aula)
    obtener_db().commit()


def cerrar_registro(registro_id):
    """Marca la salida de un docente que olvidó pasar su tarjeta."""
    if not registros.cerrar(registro_id, ahora()):
        raise ErrorApi("Ese registro ya estaba cerrado.", 404)
    obtener_db().commit()
