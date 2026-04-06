# TODO - Proyecto 1

Checklist operativo para desarrollar el proyecto paso a paso segun [PROYECTO.md](PROYECTO.md).

## Fase 0 - Arranque y organizacion

- [x] Definir arquitectura base: sensores, operadores, servidor central.
- [x] Definir tecnologias por componente (recordar: servidor central obligatorio en C).
- [x] Crear estructura de carpetas real del repositorio (`server`, `clients`, `web`, `docker`, `docs`).
- [ ] Definir convenciones de ramas y mensajes de commit.
- [ ] Crear matriz de responsabilidades por integrante.

## Fase 1 - Protocolo de aplicacion (texto)

- [x] Definir formato de mensaje (comando, campos, separador, fin de linea).
- [ ] Definir operaciones minimas:
	- [x] Registro de sensor.
	- [x] Envio de medicion.
	- [x] Consulta de estado del sistema.
	- [x] Notificacion de alertas a operadores.
- [x] Definir codigos de respuesta y errores de protocolo.
- [x] Definir ejemplos de intercambio completos (sensor-servidor y operador-servidor).
- [x] Escribir especificacion en `PROTOCOLO.md`.

## Fase 2 - Servidor central (C + Berkeley Sockets)

- [ ] Crear servidor en C con parametros de ejecucion: `./server puerto archivoDeLogs`.
- [x] Implementar socket principal y `bind/listen/accept` (si usan TCP para control principal).
- [x] Implementar concurrencia para multiples clientes (hilos o estrategia equivalente).
- [x] Implementar parser del protocolo de texto.
- [x] Implementar estado interno de sensores activos y ultimas mediciones.
- [x] Implementar deteccion basica de anomalias (reglas por umbral).
- [x] Implementar notificaciones a operadores conectados.
- [x] Implementar manejo de errores de red sin terminar el proceso abruptamente.

## Fase 3 - Logging obligatorio del servidor

- [x] Registrar peticiones entrantes.
- [x] Registrar respuestas enviadas.
- [ ] Registrar errores ocurridos.
- [x] Incluir en cada log: IP, puerto, mensaje recibido y respuesta enviada.
- [x] Imprimir logs en consola y persistirlos en archivo.

## Fase 4 - Cliente de sensores (`clients/sensor_client`)

- [x] Implementar cliente sensor que se registre ante el servidor.
- [x] Simular al menos 5 sensores con tipos distintos.
- [x] Enviar mediciones periodicas por sensor.
- [x] Incluir reconexion o reintentos ante fallos de red.
- [x] Verificar que el servidor reciba y procese las mediciones simultaneas.

## Fase 5 - Cliente operador (`clients/operator_client`)

- [x] Implementar autenticacion via servicio externo (sin usuarios locales en servidor central).
- [x] Implementar consulta de sensores activos.
- [x] Implementar visualizacion de mediciones recientes.
- [x] Implementar recepcion de alertas en tiempo real.
- [x] Construir interfaz grafica sencilla para operador.

## Fase 6 - Servicio web HTTP basico (`web`)

- [x] Implementar servidor HTTP basico.
- [x] Soportar peticiones GET.
- [x] Interpretar cabeceras HTTP correctamente.
- [x] Responder con codigos de estado HTTP adecuados.
- [ ] Exponer vistas minimas: login, estado general, sensores activos.

## Fase 7 - Resolucion de nombres y autenticacion externa

- [x] Remover cualquier IP hardcodeada del codigo.
- [x] Configurar consumo de servicios por nombre de dominio.
- [x] Manejar excepciones de resolucion DNS sin caida del sistema.
- [x] Configurar o simular servicio de identidad externo para roles de usuario.

## Fase 8 - Docker

- [x] Crear `Dockerfile` funcional del servidor.
- [x] Verificar build local de imagen.
- [x] Verificar ejecucion local del contenedor con puertos publicados.
- [x] Documentar comando de build y run utilizados en sustentacion.

## Fase 9 - AWS y acceso desde Internet

- [ ] Crear/usar instancia EC2.
- [ ] Configurar reglas de seguridad y puertos requeridos.
- [ ] Desplegar contenedor en EC2.
- [ ] Configurar DNS (idealmente Route 53) para acceso por dominio.
- [ ] Validar conexion desde clientes externos reales.

## Fase 10 - Pruebas integrales y estabilidad

- [ ] Probar carga con multiples sensores y operadores simultaneos.
- [ ] Probar desconexiones y reconexiones de clientes.
- [ ] Probar caidas parciales (DNS o servicio externo) y recuperacion.
- [ ] Verificar coordinacion correcta de mensajes entre entidades.
- [ ] Corregir cuellos de botella y errores de concurrencia.

## Fase 11 - Documentacion y entrega

- [x] Actualizar `README.md` con arquitectura final, ejecucion y despliegue.
- [x] Completar `PROTOCOLO.md` con especificacion final y ejemplos.
- [ ] Escribir guia de despliegue AWS paso a paso.
- [x] Verificar que el repositorio incluya: codigo fuente, `Dockerfile`, protocolo e instrucciones.
- [ ] Validar checklist de entrega:
	- [ ] Enlace al repositorio privado.
	- [x] Codigo fuente completo.
	- [x] Documentacion del protocolo.
	- [x] Archivo `Dockerfile`.
	- [ ] Instrucciones de despliegue en AWS.
	- [ ] Configuracion de dominio o DNS usado.
- [ ] Congelar cambios antes de la fecha limite (sin commits posteriores).

## Checklist de sustentacion (ensayo final)

- [x] Mostrar arquitectura implementada.
- [x] Demostrar servidor en C funcionando con multiples clientes.
- [x] Demostrar simulacion de sensores y alertas en tiempo real.
- [x] Demostrar build de imagen Docker en vivo.
- [ ] Demostrar ejecucion de contenedor en AWS.
- [ ] Demostrar acceso al sistema desde cliente externo por dominio.

