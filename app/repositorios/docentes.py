from ..db import obtener_db


def crear(nombre, uid):
    obtener_db().execute("INSERT INTO docentes (nombre, uid) VALUES (?, ?)", (nombre, uid))


def por_uid(uid):
    return obtener_db().execute("SELECT * FROM docentes WHERE uid = ?", (uid,)).fetchone()


def existe(docente_id):
    return obtener_db().execute("SELECT 1 FROM docentes WHERE id = ?", (docente_id,)).fetchone() is not None


def actualizar(docente_id, nombre, uid):
    obtener_db().execute("UPDATE docentes SET nombre = ?, uid = ? WHERE id = ?", (nombre, uid, docente_id))


def eliminar(docente_id):
    obtener_db().execute("DELETE FROM docentes WHERE id = ?", (docente_id,))


def listar_con_ubicacion():
    """Cada docente con el aula donde está ahora (si está en alguna)."""
    return obtener_db().execute(
        """SELECT d.id, d.nombre, d.uid,
                  (SELECT r.aula_id FROM registros_docente r
                    WHERE r.docente_id = d.id AND r.salida IS NULL ORDER BY r.id DESC LIMIT 1) AS aula_actual,
                  (SELECT r.entrada FROM registros_docente r
                    WHERE r.docente_id = d.id AND r.salida IS NULL ORDER BY r.id DESC LIMIT 1) AS desde
           FROM docentes d ORDER BY d.nombre COLLATE NOCASE"""
    ).fetchall()
