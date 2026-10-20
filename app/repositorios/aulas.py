from ..db import obtener_db


def asegurar(aula_id):
    """Crea el aula si todavía no existe."""
    obtener_db().execute("INSERT OR IGNORE INTO aulas (id, nombre) VALUES (?, ?)", (aula_id, aula_id))


def listar():
    return obtener_db().execute("SELECT * FROM aulas ORDER BY id").fetchall()


def registrar_lectura(aula_id, personas, fecha, personas_tof=None, sin_confirmar=None):
    """personas_tof y sin_confirmar son None si el aula no tiene sensores ToF."""
    obtener_db().execute(
        """UPDATE aulas SET personas = ?, ultima_conexion = ?, personas_tof = ?, sin_confirmar = ?
           WHERE id = ?""",
        (personas, fecha, personas_tof, sin_confirmar, aula_id),
    )


def guardar(aula_id, nombre):
    """Crea el aula o, si ya existe, actualiza su nombre."""
    obtener_db().execute(
        "INSERT INTO aulas (id, nombre) VALUES (?, ?) ON CONFLICT(id) DO UPDATE SET nombre = excluded.nombre",
        (aula_id, nombre),
    )


def renombrar(aula_id, nombre):
    cursor = obtener_db().execute("UPDATE aulas SET nombre = ? WHERE id = ?", (nombre, aula_id))
    return cursor.rowcount > 0


def eliminar(aula_id):
    obtener_db().execute("DELETE FROM aulas WHERE id = ?", (aula_id,))
