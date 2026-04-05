# Seguimiento Fase 9 - Paso a Paso Completo

Objetivo:
Desplegar este proyecto en EC2, dejarlo accesible desde Internet y documentar evidencia para sustentacion.

Importante:
Ejecuta cada paso en orden. No saltes validaciones.

---

## 0. Datos que debes tener antes de empezar

- [ ] Cuenta AWS activa
- [ ] Llave SSH (.pem)
- [ ] URL del repositorio Git (GitHub/GitLab): https://github.com/AlejandroSPosada/telematica_proyecto_1
- [ ] Tu IP publica actual (para restringir SSH): 38.156.230.20

Completar:

- Region AWS: ____________________
- Repo URL: ____________________
- Rama a desplegar: main

---

## 1. Crear EC2

1. En AWS EC2 -> Launch instance.
2. Nombre sugerido: iot-monitor-fase9.
3. AMI: Ubuntu Server 22.04 LTS.
4. Instance type: t3.small o t3.medium.
5. Key pair: crear o seleccionar una existente.
6. Storage: 20 GB recomendado.
7. Launch instance.

Checklist:

- [ ] Instancia creada
- [ ] Estado running
- [ ] Public IPv4 asignada

Evidencia:

- Instance ID: ____________________
- Public IPv4: ____________________
- Elastic IP (si asignaste): ____________________

---

## 2. Configurar Security Group

Inbound rules:

1. SSH -> TCP 22 -> Source: tu IP/32
2. Custom TCP -> 8081 -> Source: 0.0.0.0/0
3. Custom TCP -> 5000 -> Source: 0.0.0.0/0

Notas:

- No abrir 9090 públicamente (auth-service es interno entre contenedores).
- Si el profe pide más seguridad, restringe 5000 a IPs del equipo.

Checklist:

- [ ] Puerto 22 OK
- [ ] Puerto 8081 OK
- [ ] Puerto 5000 OK

Evidencia:

- Security Group ID: ____________________

---

## 3. Conectarte por SSH desde tu PC

En PowerShell (Windows):

```bash
ssh -i "C:/ruta/tu-key.pem" ubuntu@IP_PUBLICA_EC2
```

Si sale warning de host key, aceptar yes.

Checklist:

- [ ] Login SSH exitoso

---

## 4. Preparar EC2 (Docker + Git)

Ya dentro de Ubuntu EC2:

```bash
sudo apt update
sudo apt install -y git docker.io docker-compose-plugin
sudo usermod -aG docker ubuntu
newgrp docker
docker --version
docker compose version
```

Checklist:

- [ ] Git instalado
- [ ] Docker instalado
- [ ] Docker Compose disponible

Evidencia:

- docker --version: ____________________
- docker compose version: ____________________

---

## 5. Traer codigo en EC2

Primera vez:

```bash
git clone <REPO_URL> proyecto1
cd proyecto1
git checkout main
```

Si ya existe carpeta:

```bash
cd proyecto1
git pull origin main
```

Checklist:

- [ ] Codigo descargado
- [ ] Rama main activa
- [ ] Ultimos commits aplicados

---

## 6. Levantar stack en EC2

En la raiz del repo (donde está docker-compose.yml):

```bash
docker compose up -d --build
docker compose ps
```

Verificar que estén Up:

- auth-service
- monitor-server
- sensor-client

Si algo falla:

```bash
docker compose logs -f monitor-server
docker compose logs -f auth-service
docker compose logs -f sensor-client
```

Checklist:

- [ ] Stack arriba
- [ ] Sin crash loop

---

## 7. Pruebas funcionales en EC2

Desde EC2 (prueba local):

```bash
curl -i http://localhost:8081
```

Esperado:

- Respuesta 401 Unauthorized (porque dashboard usa Basic Auth)

Prueba con credenciales:

```bash
curl -i -u admin:admin123 http://localhost:8081
```

Esperado:

- Respuesta 200 OK

Checklist:

- [ ] HTTP responde en localhost
- [ ] Auth HTTP funciona

---

## 8. Pruebas desde tu computador (acceso Internet)

En tu navegador:

```text
http://IP_PUBLICA_EC2:8081
```

Debe pedir credenciales (Basic Auth):

- user: admin
- pass: admin123

Operador Java desde tu PC:

1. Host: IP_PUBLICA_EC2
2. Puerto: 5000
3. Usuario/Password: admin / admin123
4. Conectar / Autenticar

Esperado:

- Lista de sensores visible
- Alertas push en tiempo real

Checklist:

- [ ] Dashboard accesible desde Internet
- [ ] Operador conecta por TCP 5000
- [ ] Sensores visibles
- [ ] Alertas push visibles

---

## 9. DNS con Route 53 (requisito de fase)

1. Obtener o usar dominio.
2. Crear Hosted Zone en Route 53.
3. Crear registro A apuntando a Elastic IP de EC2.
4. Esperar propagacion (normalmente minutos).

Pruebas:

```bash
nslookup tu-dominio.com
```

Luego probar:

- http://tu-dominio.com:8081
- operador Java host = tu-dominio.com, puerto = 5000

Checklist:

- [ ] Dominio resuelve a EC2
- [ ] Dashboard por dominio funciona
- [ ] Operador por dominio funciona

Evidencia:

- Dominio: ____________________
- Registro A: ____________________

---

## 10. Comandos de operacion diaria en EC2

Actualizar a últimos cambios:

```bash
cd ~/proyecto1
git pull origin main
docker compose up -d --build
docker compose ps
```

Ver logs:

```bash
docker compose logs -f monitor-server
```

Detener:

```bash
docker compose down
```

---

## 11. Evidencias para sustentacion

Capturas recomendadas:

- [ ] EC2 running con IP
- [ ] Security Group inbound rules
- [ ] docker compose ps en EC2
- [ ] docker compose logs monitor-server
- [ ] Dashboard por IP o dominio
- [ ] Operador Java mostrando sensores + alertas

---

## 12. Resultado final

- Estado fase 9: ____________________
- Fecha cierre: ____________________
- Bloqueos encontrados: ____________________
- Soluciones aplicadas: ____________________



