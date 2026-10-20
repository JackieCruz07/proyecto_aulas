def test_conteo_crea_el_aula_y_guarda_historial(esp32, panel):
    assert esp32.post("/api/conteo", json={"aula": "d103", "personas": 25}).json == {"ok": True}
    estado = panel.get("/api/estado").json
    assert estado[0]["id"] == "D103" and estado[0]["personas"] == 25 and estado[0]["en_linea"]
    assert panel.get("/api/historial?aula=D103&horas=1").json[0]["personas"] == 25


def test_conteo_con_datos_invalidos(esp32):
    for cuerpo in ({}, {"aula": "D103"}, {"aula": "D103", "personas": -1},
                   {"aula": "D103", "personas": "5"}, {"aula": "D103", "personas": True}):
        assert esp32.post("/api/conteo", json=cuerpo).status_code == 400


def test_tarjeta_alterna_entrada_y_salida(esp32, panel):
    panel.post("/api/docentes", json={"nombre": "Ana Pérez", "uid": "63 36:d5 05"})
    assert esp32.post("/api/rfid", json={"aula": "D103", "uid": "6336D505"}).json["accion"] == "entrada"
    assert panel.get("/api/estado").json[0]["docentes"][0]["nombre"] == "Ana Pérez"
    assert esp32.post("/api/rfid", json={"aula": "D103", "uid": "6336D505"}).json["accion"] == "salida"
    assert panel.get("/api/estado").json[0]["docentes"] == []


def test_tarjeta_no_registrada(esp32):
    r = esp32.post("/api/rfid", json={"aula": "D103", "uid": "ZZ99"})
    assert r.status_code == 404 and r.json["uid"] == "ZZ99"


def test_docente_que_cambia_de_aula_cierra_el_registro_anterior(esp32, panel):
    panel.post("/api/docentes", json={"nombre": "Ana", "uid": "AA11"})
    esp32.post("/api/rfid", json={"aula": "D101", "uid": "AA11"})
    esp32.post("/api/rfid", json={"aula": "D102", "uid": "AA11"})
    estado = {a["id"]: a for a in panel.get("/api/estado").json}
    assert estado["D101"]["docentes"] == [] and len(estado["D102"]["docentes"]) == 1


def test_antirrebote(app, panel):
    app.config["SEGUNDOS_ANTIREBOTE"] = 60
    cliente = app.test_client()
    cliente.environ_base["HTTP_X_API_KEY"] = app.config["API_KEY"]
    panel.post("/api/docentes", json={"nombre": "Ana", "uid": "AA11"})
    assert cliente.post("/api/rfid", json={"aula": "D103", "uid": "AA11"}).json["accion"] == "entrada"
    assert cliente.post("/api/rfid", json={"aula": "D103", "uid": "AA11"}).json["accion"] == "ignorado"


def test_conteo_con_verificacion_tof(esp32, panel):
    esp32.post("/api/conteo", json={"aula": "D103", "personas": 5, "tof": 4, "sin_confirmar": 1})
    aula = panel.get("/api/estado").json[0]
    assert aula["personas"] == 5 and aula["personas_tof"] == 4 and aula["sin_confirmar"] == 1


def test_aula_sin_sensores_tof_no_tiene_verificacion(esp32, panel):
    esp32.post("/api/conteo", json={"aula": "D103", "personas": 5})
    aula = panel.get("/api/estado").json[0]
    assert aula["personas_tof"] is None and aula["sin_confirmar"] == 0


def test_el_tof_desaparece_si_el_esp32_deja_de_mandarlo(esp32, panel):
    esp32.post("/api/conteo", json={"aula": "D103", "personas": 5, "tof": 5})
    esp32.post("/api/conteo", json={"aula": "D103", "personas": 6})
    assert panel.get("/api/estado").json[0]["personas_tof"] is None


def test_campos_tof_invalidos(esp32):
    for extra in ({"tof": -1}, {"tof": "3"}, {"tof": True}, {"sin_confirmar": -2}):
        assert esp32.post("/api/conteo", json={"aula": "D103", "personas": 1, **extra}).status_code == 400
