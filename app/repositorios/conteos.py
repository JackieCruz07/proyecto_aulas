from ..db import obtener_db


def insertar(aula_id, personas, fecha):
    obtener_db().execute(
        "INSERT INTO conteos (aula_id, personas, fecha) VALUES (?, ?, ?)", (aula_id, personas, fecha)
    )


def listar_desde(aula_id, desde, limite=5000):
    return obtener_db().execute(
        "SELECT fecha, personas FROM conteos WHERE aula_id = ? AND fecha >= ? ORDER BY fecha LIMIT ?",
        (aula_id, desde, limite),
    ).fetchall()


def eliminar_de_aula(aula_id):
    obtener_db().execute("DELETE FROM conteos WHERE aula_id = ?", (aula_id,))
