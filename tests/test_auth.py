from conftest import CLAVE_PANEL_PRUEBA


def test_panel_sin_sesion_redirige_al_login(app):
    r = app.test_client().get("/")
    assert r.status_code == 302 and r.headers["Location"].endswith("/login")


def test_api_del_panel_sin_sesion_da_401(app):
    assert app.test_client().get("/api/estado").status_code == 401


def test_login_incorrecto(app):
    assert app.test_client().post("/login", data={"clave": "mal"}).status_code == 401


def test_login_correcto_da_acceso(app):
    cliente = app.test_client()
    assert cliente.post("/login", data={"clave": CLAVE_PANEL_PRUEBA}).status_code == 302
    assert cliente.get("/").status_code == 200


def test_logout_cierra_la_sesion(panel):
    panel.get("/logout")
    assert panel.get("/api/estado").status_code == 401


def test_login_carga_sus_estilos_sin_sesion(app):
    assert app.test_client().get("/static/css/login.css").status_code == 200


def test_esp32_sin_clave_de_api_es_rechazado(app):
    r = app.test_client().post("/api/conteo", json={"aula": "D103", "personas": 3})
    assert r.status_code == 401
