import random
from sensor import Sensor


class TemperatureSensor(Sensor):
    """Sensor de temperatura — rango normal: 15.0 a 35.0 °C"""
    def __init__(self):
        super().__init__("temperature")

    def generate_value(self):
        # Ocasionalmente genera valores anómalos para disparar alertas
        if random.random() < 0.1:
            return round(random.uniform(50.0, 80.0), 2)
        return round(random.uniform(15.0, 35.0), 2)


class PressureSensor(Sensor):
    """Sensor de presión — rango normal: 950 a 1050 hPa"""
    def __init__(self):
        super().__init__("pressure")

    def generate_value(self):
        if random.random() < 0.1:
            return round(random.uniform(1100.0, 1200.0), 2)
        return round(random.uniform(950.0, 1050.0), 2)


class HumiditySensor(Sensor):
    """Sensor de humedad — rango normal: 30 a 70 %"""
    def __init__(self):
        super().__init__("humidity")

    def generate_value(self):
        if random.random() < 0.1:
            return round(random.uniform(85.0, 100.0), 2)
        return round(random.uniform(30.0, 70.0), 2)


class VibrationSensor(Sensor):
    """Sensor de vibración — rango normal: 0.0 a 5.0 mm/s"""
    def __init__(self):
        super().__init__("vibration")

    def generate_value(self):
        if random.random() < 0.1:
            return round(random.uniform(8.0, 15.0), 2)
        return round(random.uniform(0.0, 5.0), 2)


class EnergySensor(Sensor):
    """Sensor de consumo energético — rango normal: 100 a 400 W"""
    def __init__(self):
        super().__init__("energy")

    def generate_value(self):
        if random.random() < 0.1:
            return round(random.uniform(600.0, 900.0), 2)
        return round(random.uniform(100.0, 400.0), 2)