"""
Mock Server — Para probar los sensores sin el servidor C real.
Implementa el protocolo del equipo según PROTOCOLO.md.
Puerto: 8080
"""

import socket
import threading

HOST = "localhost"
PORT = 8080

sensor_registry = {}  # sensor_id -> {type, readings}
sensor_counter   = [1]
operator_registry = {}


def handle_client(conn, addr):
    ip, port = addr
    print(f"[MOCK] Nueva conexión: {ip}:{port}")
    state = "UNIDENTIFIED"

    try:
        buffer = ""
        while True:
            chunk = conn.recv(1024).decode(errors="ignore")
            if not chunk:
                break
            buffer += chunk

            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip()
                if not line:
                    continue

                print(f"[MOCK] Recibido de {ip}:{port} → {line}")
                parts = line.split(" ")
                verb  = parts[0].upper()

                # ── REGISTER ──
                if verb == "REGISTER" and state == "UNIDENTIFIED":
                    if len(parts) < 2:
                        resp = "RESPONSE 400 - Bad request\n"
                    else:
                        sensor_type = parts[1]
                        sid = str(sensor_counter[0])
                        sensor_counter[0] += 1
                        sensor_registry[sid] = {"type": sensor_type, "readings": []}
                        state = "CONNECTED"
                        resp = f"RESPONSE 200 {sid} Sensor registered correctly\n"

                # ── CONNECT ──
                elif verb == "CONNECT" and state == "UNIDENTIFIED":
                    if len(parts) < 3:
                        resp = "RESPONSE 400 - Bad request\n"
                    else:
                        sid         = parts[1]
                        sensor_type = parts[2]
                        if sid in sensor_registry:
                            state = "CONNECTED"
                            resp  = f"RESPONSE 200 {sid} Connection established\n"
                        else:
                            sensor_registry[sid] = {"type": sensor_type, "readings": []}
                            state = "CONNECTED"
                            resp  = f"RESPONSE 200 {sid} Sensor registered correctly\n"

                # ── AUTH (operador) ──
                elif verb == "AUTH" and state == "UNIDENTIFIED":
                    if len(parts) < 3:
                        resp = "RESPONSE 400 - Bad request\n"
                    else:
                        username = parts[1]
                        state    = "CONNECTED"
                        resp     = f"RESPONSE 200 {username} User authenticated\n"

                # ── READ_BATCH ──
                elif verb == "READ_BATCH" and state == "CONNECTED":
                    # READ_BATCH sensor_id timestamp count r1 r2 r3...
                    if len(parts) < 5:
                        resp = "RESPONSE 400 - Bad request\n"
                    else:
                        sid       = parts[1]
                        timestamp = parts[2]
                        count     = int(parts[3])
                        readings  = parts[4:4 + count]
                        if sid in sensor_registry:
                            sensor_registry[sid]["readings"].extend(readings)
                            # Mantener solo las últimas 10
                            sensor_registry[sid]["readings"] = \
                                sensor_registry[sid]["readings"][-10:]
                        resp = f"RESPONSE 200 {sid} Batch received\n"

                # ── LIST ──
                elif verb == "LIST" and state == "CONNECTED":
                    filter_type = parts[1] if len(parts) > 1 and parts[1] != "-" else None
                    items = []
                    for sid, info in sensor_registry.items():
                        if filter_type is None or info["type"] == filter_type:
                            last = info["readings"][-1] if info["readings"] else "N/A"
                            items.append(f"{sid} {info['type']} {last}")
                    count = len(items)
                    body  = " ".join(items)
                    resp  = f"RESPONSE 200 {count} {body}\n"

                # ── HISTORY ──
                elif verb == "HISTORY" and state == "CONNECTED":
                    if len(parts) < 2:
                        resp = "RESPONSE 400 - Bad request\n"
                    else:
                        sid = parts[1]
                        if sid in sensor_registry:
                            readings = sensor_registry[sid]["readings"]
                            count    = len(readings)
                            body     = " ".join(readings)
                            resp     = f"RESPONSE 200 {count} {body}\n"
                        else:
                            resp = "RESPONSE 404 - Sensor not found\n"

                # ── ALERTS ──
                elif verb == "ALERTS" and state == "CONNECTED":
                    resp = "RESPONSE 200 0 No alerts\n"

                # ── DISCONNECT ──
                elif verb == "DISCONNECT":
                    resp  = "RESPONSE 200 - Connection terminated\n"
                    state = "OFFLINE"
                    conn.sendall(resp.encode())
                    print(f"[MOCK] {ip}:{port} desconectado limpiamente")
                    return

                # ── Mensaje inválido ──
                else:
                    resp = "RESPONSE 400 - Bad request\n"

                print(f"[MOCK] Enviando → {resp.strip()}")
                conn.sendall(resp.encode())

    except Exception as e:
        print(f"[MOCK] Error con {ip}:{port}: {e}")
    finally:
        conn.close()
        print(f"[MOCK] Conexión cerrada: {ip}:{port}")


def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen(20)
    print(f"[MOCK] Servidor mock corriendo en {HOST}:{PORT}")
    print(f"[MOCK] Esperando sensores y operadores...")

    while True:
        conn, addr = server.accept()
        threading.Thread(target=handle_client, args=(conn, addr), daemon=True).start()


if __name__ == "__main__":
    main()