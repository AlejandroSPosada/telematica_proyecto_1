# Guía de Despliegue - Sistema IoT de Monitoreo

## Información General

Este documento describe el procedimiento para desplegar el sistema completo de monitoreo de sensores IoT en AWS.

**Sistema Desplegado:**
- Servidor TCP (puerto 5000)
- Servidor HTTP (puerto 8081)
- Servicio de autenticación (puerto 9090, interno)
- Dominio: `telematica-iot-monitoring.duckdns.org`
- IP Pública: `52.21.234.109`

## Requisitos Previos

1. **Cuenta AWS activa** (con acceso a EC2, VPC, Security Groups)
2. **Docker instalado** en la máquina local (para compilar imágenes)
3. **Docker Compose** instalado
4. **Git** para clonar el repositorio
5. **Duck DNS** (para resolución de nombres)

## Arquitectura de Despliegue

```
┌─────────────────────────────────────────────┐
│         Instancia EC2 (52.21.234.109)       │
│  (ubuntu:22.04, 1 vCPU, 1GB RAM)            │
├─────────────────────────────────────────────┤
│  Docker Container Stack (docker-compose)    │
│  ├─ iot-auth-service (puerto 9090 interno)  │
│  ├─ iot-monitor-server (5000 TCP, 8081 HTTP)│
│  └─ iot-sensor-client (simulado)            │
└─────────────────────────────────────────────┘
         ↓↑
    Duck DNS
  (telematica-iot-monitoring.duckdns.org)
```

## Paso 1: Preparación en AWS

### 1.1 Crear instancia EC2

1. Ve a **EC2 Dashboard** → **Launch Instances**
2. Selecciona: **Ubuntu 22.04 LTS** (t2.micro, elegible para free tier)
3. Configuración recomendada:
   - **Tipo de instancia:** t2.micro
   - **Storage:** 20 GB (gp2)
   - **VPC/Subnet:** Default
4. **Key Pair:** Crea o importa una llave SSH
5. **Security Group:** Crea uno nuevo con las siguientes reglas:

```
Inbound Rules:
  - SSH (22): 0.0.0.0/0
  - TCP (5000): 0.0.0.0/0  (Servidor TCP IoT)
  - TCP (8081): 0.0.0.0/0  (Servidor HTTP)
  - TCP (9090): 0.0.0.0/0  (Auth service - opcional, solo si accedes externamente)

Outbound Rules:
  - Todos permitidos (default)
```

6. Lanza la instancia y anota la **IP pública** (en este caso: `52.21.234.109`)

### 1.2 Configurar Route 53 (Hosted Zone)

⚠️ **Nota:** En el lab voclabs no es posible registrar dominios globalmente por restricciones de permisos. Sin embargo, se configuró una Hosted Zone como demostración del requisito.

1. Ve a **Route 53** → **Hosted zones**
2. Crea una Hosted Zone para `telematica-iot-monitoring.com`
3. Crea un record A apuntando a la IP: `52.21.234.109`

**Alternativa usada: Duck DNS**
- Dominio: `telematica-iot-monitoring.duckdns.org`
- IP: `52.21.234.109`
- Actualizable en: https://www.duckdns.org

## Paso 2: Conectar a la instancia EC2

```bash
# Conectarse vía SSH (reemplaza la llave y la IP)
ssh -i tu-llave.pem ubuntu@52.21.234.109
```

## Paso 3: Instalar Docker

```bash
# Actualizar paquetes
sudo apt update
sudo apt install -y docker.io docker-compose

# Agregar usuario actual al grupo docker (sin sudo)
sudo usermod -aG docker $USER
newgrp docker
```

## Paso 4: Clonar el repositorio

```bash
cd ~
git clone <URL-del-repositorio-privado> proyecto-iot
cd proyecto-iot
```

## Paso 5: Construir e iniciar contenedores

```bash
# Construir las imágenes Docker
docker compose build

# Iniciar los contenedores (en background)
docker compose up -d

# Verificar que estén corriendo
docker compose ps
```

**Salida esperada:**
```
CONTAINER ID   IMAGE                      STATUS
abc123def456   proyecto-iot_auth-service  Up 2 minutes
def456ghi789   proyecto-iot_monitor-server Up 2 minutes
ghi789jkl012   proyecto-iot_sensor-client Up 2 minutes
```

## Paso 6: Verificar que funcionan los servicios

### 6.1 Verificar servidor HTTP (Web)

```bash
# Desde tu máquina local
curl -u admin:admin123 http://52.21.234.109:8081/
```

O accede desde el navegador:
```
http://52.21.234.109:8081
# (o http://telematica-iot-monitoring.duckdns.org:8081)
```

### 6.2 Verificar servidor TCP (Protocolo IoT)

```bash
# Conectar y probar AUTH
(echo "AUTH admin admin123"; sleep 1) | nc 52.21.234.109 5000
```

Deberías recibir:
```
RESPONSE 200 <id> User authenticated
```

### 6.3 Ver logs del servidor

```bash
# Ver logs en vivo
docker compose logs -f iot-monitor-server

# O en archivo dentro del contenedor
docker exec iot-monitor-server cat /app/server.log
```

## Paso 7: Conectar clientes

### Cliente Operador (Java)

1. En tu máquina local, ve a `clients/operador_client/`
2. Compila: `javac Main.java`
3. Ejecuta: `java Main`
4. En la UI, ingresa:
   - **Host:** `telematica-iot-monitoring.duckdns.org` (o `52.21.234.109`)
   - **Puerto:** `5000`
   - **Usuario:** `admin`
   - **Contraseña:** `admin123`
5. Pulsa **"Conectar / Autenticar"**

### Clientes Sensores

Los sensores se inician automáticamente dentro del contenedor `iot-sensor-client`:
```bash
docker compose logs iot-sensor-client
```

## Resolución de Nombres - Duck DNS

### Configuración inicial

1. Regístrate en https://www.duckdns.org
2. Crea un dominio: `telematica-iot-monitoring`
3. Actualiza la IP manualmente o usa el token API

### Script de auto-actualización (Opcional)

En la instancia EC2, crea `/home/ubuntu/update-duckdns.sh`:

```bash
#!/bin/bash
DUCKDNS_TOKEN="tu-token-aqui"
DOMAIN="telematica-iot-monitoring"

while true; do
  IP=$(curl -s https://checkip.amazonaws.com)
  curl "https://www.duckdns.org/update?domains=$DOMAIN&token=$DUCKDNS_TOKEN&ip=$IP"
  sleep 3600  # Actualizar cada hora
done
```

Hazlo ejecutable y añádelo a cron:
```bash
sudo chmod +x /home/ubuntu/update-duckdns.sh
crontab -e
# Añade: @reboot /home/ubuntu/update-duckdns.sh &
```

## Entorno - Variables de Configuración

Las variables de entorno se definen en `docker-compose.yml`:

```yaml
environment:
  AUTH_SERVICE_HOST: auth-service      # No hardcodeado
  AUTH_SERVICE_PORT: "9090"
  SERVER_HOST: monitor-server
  SERVER_PORT: "5000"
```

**Importante:** El código no contiene IPs hardcodeadas, usando nombres de servicio y variables de entorno.

## Detener y limpiar

```bash
# Detener contenedores
docker compose down

# Eliminar volúmenes (cuidado, borra datos)
docker compose down -v
```

## Troubleshooting

| Problema | Solución |
|----------|----------|
| "Connection refused" en puerto 5000 | Verifica Security Group allows puerto 5000; comprueba `docker compose ps` |
| Auth service no responde | Asegúrate que `iot-auth-service` esté corriendo: `docker compose logs auth-service` |
| DNS no resuelve | Actualiza IP en Duck DNS manualmente o verifica propagación |
| Contenedor se crashes | Ver logs: `docker compose logs iot-monitor-server` |
| "Permission denied" con Docker | Ejecuta `newgrp docker` o usa `sudo docker compose` |

## Verificación del Despliegue Completo

Checklist de validación:

- [ ] Instancia EC2 corriendo
- [ ] Security Group configurado con puertos 5000, 8081
- [ ] Docker y docker-compose instalados
- [ ] Contenedores construidos: `docker compose build`
- [ ] Contenedores levantados: `docker compose up -d`
- [ ] Servidor HTTP accesible: `curl http://IP:8081`
- [ ] Servidor TCP responde: `nc -zv IP 5000`
- [ ] Duck DNS apunta a IP correcta
- [ ] Cliente Java conecta exitosamente
- [ ] Sensores envían datos (ver logs)
- [ ] Alertas se reciben en cliente

## Documentación Técnica

- **Protocolo de aplicación:** Ver `PROTOCOLO.md`
- **Especificación del proyecto:** Ver `PROYECTO.md`
- **Cliente Operador:** Ver `clients/operador_client/README.md`
- **Sensor Client:** Ver `clients/sensor_client/README.md`

## Soporte y Desarrollo

Para cambios durante el despliegue:

```bash
# Recompilar e iniciar (preserva datos)
docker compose down
docker compose build --no-cache
docker compose up -d

# Acceder a un contenedor interactivamente
docker compose exec iot-monitor-server bash

# Ver todas las redes y servicios
docker network ls
docker compose config
```

---

**Última actualización:** Abril 5, 2026  
**Responsables:** Equipo de Desarrollo  
**Institución:** EAFIT - Internet, Arquitectura y Protocolos
