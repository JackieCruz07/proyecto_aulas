import pytest

from app import crear_app
from config import Config

CLAVE_API_PRUEBA = "clave-api-de-prueba"
CLAVE_PANEL_PRUEBA = "clave-panel-de-prueba"


@pytest.fixture
def app(tmp_path):
    class ConfigPrueba(Config):
        TESTING = True
        BASE_DATOS = str(tmp_path / "prueba.db")
        API_KEY = CLAVE_API_PRUEBA
        CLAVE_PANEL = CLAVE_PANEL_PRUEBA
        SEGUNDOS_ANTIREBOTE = 0   # en las pruebas no queremos esperar entre tarjetas

    return crear_app(ConfigPrueba)


@pytest.fixture
def esp32(app):
    """Cliente que se identifica como un ESP32 (clave de API, sin sesión)."""
    cliente = app.test_client()
    cliente.environ_base["HTTP_X_API_KEY"] = CLAVE_API_PRUEBA
    return cliente


@pytest.fixture
def panel(app):
    """Cliente que ya inició sesión en el panel."""
    cliente = app.test_client()
    cliente.post("/login", data={"clave": CLAVE_PANEL_PRUEBA})
    return cliente
