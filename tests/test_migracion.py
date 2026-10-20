import sqlite3

from app import crear_app
from config import Config


def test_base_de_datos_antigua_se_actualiza_sin_perder_datos(tmp_path):
    ruta = tmp_path / "vieja.db"
    conexion = sqlite3.connect(ruta)
    conexion.executescript(
        "CREATE TABLE aulas (id TEXT PRIMARY KEY, nombre TEXT, personas INTEGER DEFAULT 0, ultima_conexion TEXT);"
        "INSERT INTO aulas (id, nombre, personas) VALUES ('D101', 'Aula vieja', 3);"
    )
    conexion.commit()
    conexion.close()

    class ConfigVieja(Config):
        TESTING = True
        BASE_DATOS = str(ruta)
        CLAVE_PANEL = "clave"

    app = crear_app(ConfigVieja)
    cliente = app.test_client()
    cliente.post("/login", data={"clave": "clave"})
    aula = cliente.get("/api/estado").json[0]
    assert aula["nombre"] == "Aula vieja" and aula["personas"] == 3
    assert aula["personas_tof"] is None and aula["sin_confirmar"] == 0

    # Arrancar otra vez sobre la misma base no debe fallar (la migración es repetible)
    crear_app(ConfigVieja)
