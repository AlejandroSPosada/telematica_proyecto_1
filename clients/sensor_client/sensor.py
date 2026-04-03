import socket
import time
import threading
from datetime import datetime, timezone
from config import SERVER_HOST, SERVER_PORT, SEND_INTERVAL, BATCH_SIZE


class Sensor:
    def __init__(self, sensor_type):
        self.sensor_type = sensor_type
        self.sensor_id = None
        self.sock = None
        self.state = "UNIDENTIFIED"  # UNIDENTIFIED → CONNECTED → IDLE → OFFLINE
        self._lock = threading.Lock()

    # ── Conexión y registro ──────────────────────────────────────────

    def connect(self):
        """Conecta al servidor y registra el sensor. Reintenta si falla."""
        while True:
            try:
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.sock.connect((SERVER_HOST, SERVER_PORT))

                if self.sensor_id is None:
                    # Primera vez — registrar sensor nuevo
                    self._register()
                else:
                    # Reconexión — conectar sensor existente
                    self._reconnect()
                return

            except socket.error as e:
                print(f"[{self.sensor_type}] Error de conexión: {e}. Reintentando en 5s...")
                self.state = "OFFLINE"
                time.sleep(5)

    def _register(self):
        """Envía REGISTER y espera respuesta con el sensor_id asignado."""
        msg = f"REGISTER {self.sensor_type}\n"
        self.sock.sendall(msg.encode())
        response = self._recv_line()
        print(f"[{self.sensor_type}] REGISTER → {response}")

        # Formato esperado: RESPONSE 200 <sensor_id> Sensor registered correctly
        parts = response.strip().split(" ")
        if len(parts) >= 3 and parts[0] == "RESPONSE" and parts[1] == "200":
            self.sensor_id = parts[2]
            self.state = "CONNECTED"
            print(f"[{self.sensor_type}] Registrado con ID: {self.sensor_id}")
        else:
            raise Exception(f"Registro fallido: {response}")

    def _reconnect(self):
        """Envía CONNECT para reconectar un sensor existente."""
        msg = f"CONNECT {self.sensor_id} {self.sensor_type}\n"
        self.sock.sendall(msg.encode())
        response = self._recv_line()
        print(f"[{self.sensor_type}:{self.sensor_id}] CONNECT → {response}")

        parts = response.strip().split(" ")
        if len(parts) >= 2 and parts[0] == "RESPONSE" and parts[1] == "200":
            self.state = "CONNECTED"
        else:
            # Si falla reconexión, registrar como nuevo
            self.sensor_id = None
            self._register()

    # ── Envío de mediciones ──────────────────────────────────────────

    def send_batch(self, readings):
        """Envía un batch de lecturas según el protocolo READ_BATCH."""
        timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%S")
        count = len(readings)
        readings_str = " ".join(f"{r:.2f}" for r in readings)
        msg = f"READ_BATCH {self.sensor_id} {timestamp} {count} {readings_str}\n"

        try:
            with self._lock:
                self.sock.sendall(msg.encode())
                response = self._recv_line()
            print(f"[{self.sensor_type}:{self.sensor_id}] Batch enviado → {response}")
            self.state = "CONNECTED"
        except socket.error as e:
            print(f"[{self.sensor_type}:{self.sensor_id}] Error al enviar: {e}")
            self.state = "OFFLINE"
            self.connect()  # Reconectar automáticamente

    def disconnect(self):
        """Envía DISCONNECT limpio al servidor."""
        try:
            self.sock.sendall(b"DISCONNECT\n")
            response = self._recv_line()
            print(f"[{self.sensor_type}:{self.sensor_id}] DISCONNECT → {response}")
        except:
            pass
        finally:
            self.state = "OFFLINE"
            try:
                self.sock.close()
            except:
                pass

    # ── Loop principal ───────────────────────────────────────────────

    def generate_value(self):
        """Cada subclase implementa este método."""
        raise NotImplementedError

    def run(self):
        """Loop principal del sensor."""
        self.connect()
        try:
            while self.state != "OFFLINE":
                readings = [self.generate_value() for _ in range(BATCH_SIZE)]
                self.send_batch(readings)
                time.sleep(SEND_INTERVAL)
        except KeyboardInterrupt:
            self.disconnect()

    # ── Utilidades ───────────────────────────────────────────────────

    def _recv_line(self):
        """Lee una línea completa del socket (hasta \\n)."""
        data = b""
        while True:
            chunk = self.sock.recv(1)
            if not chunk or chunk == b"\n":
                break
            data += chunk
        return data.decode().strip()