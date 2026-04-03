# Configuración del servidor — sin IPs hardcodeadas
SERVER_HOST = "localhost"   # En AWS se cambia por el nombre DNS
SERVER_PORT = 8080

# Intervalo de envío de batches (segundos)
SEND_INTERVAL = 5

# Tamaño del batch (lecturas por envío)
BATCH_SIZE = 3