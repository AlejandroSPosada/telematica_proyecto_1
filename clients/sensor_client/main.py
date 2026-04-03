import threading
from sensor_types import (
    TemperatureSensor,
    PressureSensor,
    HumiditySensor,
    VibrationSensor,
    EnergySensor
)

# 5 sensores simulados — tipos distintos según el protocolo
sensores = [
    TemperatureSensor(),
    PressureSensor(),
    HumiditySensor(),
    VibrationSensor(),
    EnergySensor(),
]

# Lanzar cada sensor en su propio hilo
hilos = []
for sensor in sensores:
    t = threading.Thread(target=sensor.run, daemon=True)
    t.start()
    hilos.append(t)

print("=" * 50)
print("  Sensores IoT iniciados")
print(f"  Total: {len(sensores)} sensores activos")
print("  Presioná Ctrl+C para detener")
print("=" * 50)

try:
    for t in hilos:
        t.join()
except KeyboardInterrupt:
    print("\nDeteniendo sensores...")
    for sensor in sensores:
        sensor.disconnect()
    print("Sensores detenidos.")