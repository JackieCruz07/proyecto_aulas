from ..db import obtener_db


def ultimo_evento(docente_id):
    """Fecha de su última entrada o salida (None si nunca ha pasado la tarjeta)."""
    return obtener_db().execute(
        "SELECT MAX(COALESCE(salida, entrada)) AS t FROM registros_docente WHERE docente_id = ?",
        (docente_id,),
    ).fetchone()["t"]


def abierto_de_docente(docente_id):
    """Su registro de entrada sin salida, si lo tiene."""
    return obtener_db().execute(
        "SELECT * FROM registros_docente WHERE docente_id = ? AND salida IS NULL ORDER BY id DESC LIMIT 1",
        (docente_id,),
    ).fetchone()


def abiertos_de_aula(aula_id):
    """Docentes que están dentro del aula en este momento."""
    return obtener_db().execute(
        """SELECT r.id AS registro_id, r.docente_id,
                  COALESCE(d.nombre, 'Docente eliminado') AS nombre, r.entrada
           FROM registros_docente r
           LEFT JOIN docentes d ON d.id = r.docente_id
           WHERE r.aula_id = ? AND r.salida IS NULL ORDER BY r.entrada""",
        (aula_id,),
    ).fetchall()


def abrir(aula_id, docente_id, fecha):
    obtener_db().execute(
        "INSERT INTO registros_docente (aula_id, docente_id, entrada) VALUES (?, ?, ?)",
        (aula_id, docente_id, fecha),
    )


def cerrar(registro_id, fecha):
    """Marca la salida. Devuelve False si el registro no existe o ya estaba cerrado."""
    cursor = obtener_db().execute(
        "UPDATE registros_docente SET salida = ? WHERE id = ? AND salida IS NULL", (fecha, registro_id)
    )
    return cursor.rowcount > 0


def cerrar_abiertos_de_docente(docente_id, fecha):
    obtener_db().execute(
        "UPDATE registros_docente SET salida = ? WHERE docente_id = ? AND salida IS NULL",
        (fecha, docente_id),
    )


def eliminar_de_aula(aula_id):
    obtener_db().execute("DELETE FROM registros_docente WHERE aula_id = ?", (aula_id,))


def buscar(filtros, limite):
    """filtros: dict con aula, docente_id, desde, hasta (los que no se usen, None)."""
    condiciones, parametros = [], []
    if filtros.get("aula"):
        condiciones.append("r.aula_id = ?")
        parametros.append(filtros["aula"])
    if filtros.get("docente_id"):
        condiciones.append("r.docente_id = ?")
        parametros.append(filtros["docente_id"])
    if filtros.get("desde"):
        condiciones.append("r.entrada >= ?")
        parametros.append(filtros["desde"] + " 00:00:00")
    if filtros.get("hasta"):
        condiciones.append("r.entrada <= ?")
        parametros.append(filtros["hasta"] + " 23:59:59")
    donde = (" WHERE " + " AND ".join(condiciones)) if condiciones else ""
    return obtener_db().execute(
        """SELECT r.id, r.aula_id, r.docente_id,
                  COALESCE(d.nombre, 'Docente eliminado') AS nombre, r.entrada, r.salida
           FROM registros_docente r
           LEFT JOIN docentes d ON d.id = r.docente_id"""
        + donde + " ORDER BY r.id DESC LIMIT ?",
        parametros + [limite],
    ).fetchall()
