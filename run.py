"""Punto de entrada: python run.py"""
from app import crear_app

app = crear_app()

if __name__ == "__main__":
    # host 0.0.0.0 permite que los ESP32 de la red lleguen a este servidor
    app.run(host="0.0.0.0", port=5000)
