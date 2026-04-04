import os

# Configuracion del servidor, sobreescribible por variables de entorno
SERVER_HOST = os.getenv("SERVER_HOST", "localhost")
SERVER_PORT = int(os.getenv("SERVER_PORT", "5000"))

# Intervalo de envio de batches (segundos)
SEND_INTERVAL = int(os.getenv("SEND_INTERVAL", "5"))

# Tamano del batch (lecturas por envio)
BATCH_SIZE = int(os.getenv("BATCH_SIZE", "3"))