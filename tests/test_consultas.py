def _preparar(esp32, panel):
    panel.post("/api/docentes", json={"nombre": "Ana Pérez", "uid": "AA11"})
    panel.post("/api/docentes", json={"nombre": "Luis Gómez", "uid": "BB22"})
    esp32.post("/api/rfid", json={"aula": "D101", "uid": "AA11"})
    esp32.post("/api/rfid", json={"aula": "D102", "uid": "BB22"})


def test_registros_con_filtros(esp32, panel):
    _preparar(esp32, panel)
    assert len(panel.get("/api/registros").json) == 2
    assert len(panel.get("/api/registros?aula=d101").json) == 1
    assert len(panel.get("/api/registros?docente_id=2").json) == 1
    assert panel.get("/api/registros?desde=2099-01-01").json == []
    assert len(panel.get("/api/registros?desde=2020-01-01&hasta=2099-12-31").json) == 2
    assert len(panel.get("/api/registros?desde=no-es-fecha").json) == 2   # fecha inválida: se ignora


def test_exportar_csv(esp32, panel):
    _preparar(esp32, panel)
    r = panel.get("/api/registros.csv")
    assert r.status_code == 200 and r.mimetype == "text/csv"
    texto = r.get_data(as_text=True)
    assert texto.startswith("\ufeffDocente,Aula,Entrada,Salida") and "Ana Pérez" in texto


def test_aula_sin_reportar_aparece_sin_conexion(app, panel):
    panel.post("/api/aulas", json={"id": "D104"})
    aula = panel.get("/api/estado").json[0]
    assert aula["en_linea"] is False and aula["ultima_conexion"] is None
