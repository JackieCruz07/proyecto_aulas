def test_docentes_crud(panel):
    assert panel.post("/api/docentes", json={"nombre": "Ana", "uid": "aa11"}).json["ok"]
    assert panel.post("/api/docentes", json={"nombre": "Otra", "uid": "AA11"}).status_code == 409
    assert panel.post("/api/docentes", json={"nombre": "", "uid": "BB22"}).status_code == 400
    docente = panel.get("/api/docentes").json[0]
    assert docente["uid"] == "AA11" and docente["aula_actual"] is None
    assert panel.put(f"/api/docentes/{docente['id']}", json={"nombre": "Ana P.", "uid": "AA11"}).json["ok"]
    assert panel.put("/api/docentes/999", json={"nombre": "X", "uid": "X1"}).status_code == 404
    assert panel.delete(f"/api/docentes/{docente['id']}").json["ok"]
    assert panel.get("/api/docentes").json == []


def test_eliminar_docente_conserva_historial_y_cierra_registro(esp32, panel):
    panel.post("/api/docentes", json={"nombre": "Ana", "uid": "AA11"})
    esp32.post("/api/rfid", json={"aula": "D103", "uid": "AA11"})
    panel.delete(f"/api/docentes/{panel.get('/api/docentes').json[0]['id']}")
    registro = panel.get("/api/registros").json[0]
    assert registro["nombre"] == "Docente eliminado" and registro["salida"] is not None
    assert panel.get("/api/estado").json[0]["docentes"] == []


def test_aulas_crud(esp32, panel):
    assert panel.post("/api/aulas", json={"id": "d102", "nombre": ""}).json["ok"]
    assert panel.post("/api/aulas", json={"id": ""}).status_code == 400
    assert panel.put("/api/aulas/D102", json={"nombre": "Laboratorio"}).json["ok"]
    assert panel.put("/api/aulas/NOEXISTE", json={"nombre": "X"}).status_code == 404
    assert panel.get("/api/estado").json[0]["nombre"] == "Laboratorio"
    esp32.post("/api/conteo", json={"aula": "D102", "personas": 4})
    assert panel.delete("/api/aulas/D102").json["ok"]
    assert panel.get("/api/estado").json == []
    assert panel.get("/api/historial?aula=D102&horas=1").json == []


def test_marcar_salida_manual(esp32, panel):
    panel.post("/api/docentes", json={"nombre": "Ana", "uid": "AA11"})
    esp32.post("/api/rfid", json={"aula": "D103", "uid": "AA11"})
    registro_id = panel.get("/api/estado").json[0]["docentes"][0]["registro_id"]
    assert panel.post(f"/api/registros/{registro_id}/cerrar").json["ok"]
    assert panel.post(f"/api/registros/{registro_id}/cerrar").status_code == 404
