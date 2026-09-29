# Ensamble del Dron

Guía para el armado, conexión, configuración y carga de firmware de un dron que viene **preensamblado de fábrica**.

> **Nota:** Este procedimiento está pensado para un modelo en el que la estructura, placa electrónica, cableado y demás componentes principales ya vienen instalados. El montaje requerido se limita principalmente a los motores y las hélices, seguido de la programación y las pruebas de funcionamiento.

---

## 1. Componentes necesarios

### Componentes principales

* Chasis del dron preensamblado
* Placa controladora de vuelo (ESP-32)
* 4 × motores
* 4 × hélices
* Batería LiPo
* Cable/conector de alimentación
* Cable USB para carga de bateria lipo

### Herramientas

* Destornilladores
* Pinzas
* Tornillos y accesorios incluidos con el dron
* Computadora
* Cable USB
* Software/IDE (Arduino IDE)

### Opcional

* Multímetro
* Fuente de alimentación regulable
* Cargador balanceador para LiPo
* Probador de baterías

---

# 2. Inspección inicial

Antes de instalar cualquier componente, realizar una inspección visual.

### Revisar:

* [ ] Chasis sin grietas o daños.
* [ ] Placa controladora correctamente instalada.
* [ ] Cables correctamente conectados.
* [ ] Conectores sin pines doblados.
* [ ] Soldaduras aparentemente correctas.
* [ ] Conector de batería en buen estado.
* [ ] No existen cables pelados o en corto.
* [ ] Los motores corresponden al modelo del dron.
* [ ] Las hélices no presentan daños.

>  **No conectar la batería todavía.**

---

# 3. Instalación de los motores

El dron utiliza cuatro motores, uno en cada brazo del chasis.

La posición normalmente se identifica como:

```text
             PARTE FRONTAL
                  ↑

             M1          M2
              ↻          ↺

             M3          M4
              ↺          ↻

                  ↓
             PARTE TRASERA
```

> **Importante:** La numeración y dirección de giro pueden variar dependiendo del firmware y de la controladora. Antes de fijar definitivamente los motores, comprobar la documentación o configuración utilizada por el firmware.

## Procedimiento

1. Identificar los cuatro brazos.
2. Colocar un motor en cada extremo.
3. Alinear los orificios de montaje.
4. Colocar los tornillos correspondientes.
5. Apretar los tornillos firmemente, pero sin ejercer fuerza excesiva.
6. Comprobar que cada motor pueda girar libremente.
7. Revisar que ningún cable quede atrapado entre el motor y el chasis.

---

# 4. Conexión de los motores

Conectar cada motor a la salida correspondiente de la controladora.

Ejemplo:

```text
Motor 1 ───────► M1
Motor 2 ───────► M2
Motor 3 ───────► M3
Motor 4 ───────► M4
```

Si los motores utilizan conectores, verificar que estén completamente insertados.

Si utilizan soldadura:

* Comprobar polaridad cuando corresponda.
* Revisar que no existan puentes de soldadura.
* Evitar cables excesivamente largos.
* Asegurar los cables al chasis.

---

# 5. Instalación de las hélices

Las hélices deben instalarse respetando la configuración de giro del dron.

Un cuadricóptero normalmente utiliza:

* 2 hélices que giran en sentido horario (CW)
* 2 hélices que giran en sentido antihorario (CCW)

Ejemplo:

```text
             PARTE FRONTAL
                  ↑

             CW          CCW
              ↻           ↺

             CCW         CW
              ↺           ↻
```

## Procedimiento

1. Identificar las hélices CW y CCW.
2. Identificar qué motor debe girar en cada dirección.
3. Colocar cada hélice sobre su motor correspondiente.
4. Verificar que la hélice esté orientada correctamente.
5. Asegurar la hélice según el sistema de fijación utilizado.

> **No instalar las hélices durante las primeras pruebas de los motores.**

---

# 6. Verificación de la alimentación

Antes de conectar la batería:

1. Inspeccionar el conector.
2. Verificar que no existan cortocircuitos visibles.
3. Comprobar la tensión de la batería con un multímetro.
4. Confirmar que el voltaje sea compatible con el dron.

### LiPo de una celda

Una batería LiPo 1S normalmente trabaja aproximadamente entre:

```text
Carga completa       ≈ 4.2 V
Voltaje nominal      ≈ 3.7 V
Descarga profunda    ≈ 3.0 V
```

> No asumir que una batería de mayor voltaje es compatible solamente porque el dron enciende. La tensión máxima permitida debe coincidir con la electrónica y los motores.

---

# 7. Preparación para programar

Conectar la controladora al ordenador mediante USB cuando el diseño de la placa lo permita.

Verificar que el dispositivo sea reconocido.

Ejemplo:

```text
PC
 │
 │ USB
 ▼
Controladora
 │
 ├── Motor 1
 ├── Motor 2
 ├── Motor 3
 └── Motor 4
```

Instalar los controladores necesarios para el puerto USB/serial si el sistema operativo no reconoce automáticamente la placa.

---

# 8. Configuración del entorno de desarrollo

Instalar el IDE correspondiente al proyecto.

Ejemplos:

* Arduino IDE
* PlatformIO
* ESP-IDF
* Otro entorno compatible con la controladora

Configurar:

```text
Placa:       [modelo de la placa]
Puerto:      [COM correspondiente]
Baudrate:    [valor utilizado por el firmware]
Programador: [si aplica]
```

---

# 9. Carga del firmware

Antes de cargar el código:

* [ ] Seleccionar la placa correcta.
* [ ] Seleccionar el puerto correcto.
* [ ] Verificar las dependencias.
* [ ] Comprobar las librerías utilizadas.
* [ ] Revisar las constantes de configuración.
* [ ] Verificar la asignación de motores.
* [ ] Compilar el proyecto.

Si la compilación es correcta, cargar el firmware:

```text
Compilar
   ↓
Verificar errores
   ↓
Conectar placa
   ↓
Seleccionar puerto
   ↓
Upload / Cargar
   ↓
Reiniciar placa
```

---

# 10. Configuración del firmware

La configuración debe corresponder físicamente al dron.

Revisar principalmente:

### Asignación de motores

```text
M1 → Motor delantero izquierdo
M2 → Motor delantero derecho
M3 → Motor trasero izquierdo
M4 → Motor trasero derecho
```

### Dirección de giro

```text
M1 → CW
M2 → CCW
M3 → CCW
M4 → CW
```

> La distribución anterior es únicamente un ejemplo. Utilizar la asignación definida por el firmware y la controladora específica.

### Otros parámetros

Dependiendo del proyecto:

* Tipo de batería
* Límites de aceleración
* PWM
* Frecuencia PWM
* Velocidad mínima de motor
* Velocidad máxima
* PID
* IMU
* Giroscopio
* Acelerómetro
* Calibración
* Protocolo de comunicación
* Fail-safe
* Control remoto

---

# 11. Primera prueba sin hélices

**Esta prueba debe realizarse sin hélices instaladas.**

Conectar el dron y comprobar:

```text
Encendido
   ↓
Controladora inicia
   ↓
Firmware ejecuta
   ↓
Comunicación disponible
   ↓
Motores responden
```

Activar los motores individualmente.

Por ejemplo:

```text
Prueba 1 → M1
Prueba 2 → M2
Prueba 3 → M3
Prueba 4 → M4
```

Comprobar:

* [ ] M1 funciona.
* [ ] M2 funciona.
* [ ] M3 funciona.
* [ ] M4 funciona.
* [ ] Ningún motor presenta ruido anormal.
* [ ] Ningún motor se calienta excesivamente.
* [ ] La controladora no se reinicia.
* [ ] La alimentación permanece estable.

---

# 12. Verificación del sentido de giro

Sin hélices instaladas, comprobar visualmente el sentido de giro.

```text
             FRENTE
                ↑

          ↻             ↺
         M1             M2

          ↺             ↻
         M3             M4
```

Si un motor gira en sentido contrario:

* Corregir la configuración del firmware, si es posible.
* Si el motor utiliza tres fases y el diseño lo permite, intercambiar dos de las fases/cables del motor.

> No realizar modificaciones eléctricas mientras la batería esté conectada.

---

# 13. Instalación definitiva de las hélices

Una vez comprobado que:

* Los cuatro motores funcionan.
* La dirección de giro es correcta.
* La controladora responde.
* La alimentación es estable.

Instalar las hélices.

### Comprobación

Cada hélice debe:

* Corresponder al motor correcto.
* Tener la orientación correcta.
* Estar correctamente fijada.
* No presentar grietas.
* No tener deformaciones.

---

# 14. Prueba del sistema de control

Encender el sistema de control y verificar:

```text
Control
   ↓
Comunicación
   ↓
Controladora
   ↓
Firmware
   ↓
Motores
```

Comprobar:

* [ ] Conexión estable.
* [ ] Throttle responde correctamente.
* [ ] Roll responde correctamente.
* [ ] Pitch responde correctamente.
* [ ] Yaw responde correctamente.
* [ ] Los comandos corresponden al movimiento esperado.

---

# 15. Primera prueba de vuelo

Realizar la primera prueba en un área:

* Abierta.
* Sin personas cerca.
* Sin objetos frágiles.
* Sin cables u obstáculos.
* Con suficiente espacio alrededor del dron.

### Procedimiento

1. Colocar el dron sobre una superficie nivelada.
2. Encender la controladora.
3. Esperar a que finalice la inicialización.
4. Conectar el control.
5. Comprobar nuevamente la comunicación.
6. Armar el dron.
7. Incrementar gradualmente el throttle.
8. Realizar un despegue corto.
9. Mantenerlo a baja altura.
10. Comprobar estabilidad.
11. Aterrizar inmediatamente si se detecta un comportamiento anormal.

---

# 16. Condiciones para abortar la prueba

Interrumpir inmediatamente la prueba si:

* Un motor no responde.
* Una hélice vibra excesivamente.
* El dron intenta volcarse.
* El dron gira sobre sí mismo sin comando.
* La controladora se reinicia.
* La batería presenta calentamiento anormal.
* Aparece olor a componente quemado.
* Se pierde la comunicación.
* El dron responde de manera inesperada a los controles.

---

# 17. Diagnóstico de problemas

## El dron no enciende

Comprobar:

```text
Batería
   ↓
Conector
   ↓
Protección/BMS
   ↓
Alimentación de la placa
   ↓
Reguladores
```

---

## Un motor no funciona

Comprobar:

1. Conector.
2. Cableado.
3. Salida de la controladora.
4. Configuración del firmware.
5. Motor.
6. Alimentación.

Una prueba útil es intercambiar temporalmente el motor por otro canal, **sin hélices**, para determinar si el problema está en el motor o en la salida de la controladora.

---

## El motor gira en sentido incorrecto

Revisar la configuración de dirección del motor en el firmware.

---

## El dron intenta volcarse

Posibles causas:

* Hélices incorrectas.
* Hélices instaladas al revés.
* Motor conectado a una posición incorrecta.
* Dirección de giro incorrecta.
* Asignación de motores incorrecta.
* IMU mal calibrada.
* Orientación incorrecta de la controladora.
* Configuración PID incorrecta.

---

# 18. Checklist final

## Hardware

* [ ] Chasis inspeccionado.
* [ ] 4 motores instalados.
* [ ] Motores correctamente conectados.
* [ ] Hélices correctas.
* [ ] Hélices correctamente instaladas.
* [ ] Batería compatible.
* [ ] Conectores revisados.


## Pruebas

* [ ] Prueba sin hélices.
* [ ] Los 4 motores funcionan.
* [ ] Sentido de giro correcto.
* [ ] Comunicación estable.
* [ ] Controles funcionan.
* [ ] Prueba de vuelo realizada.

---

# 19. Estructura recomendada del proyecto

```text
DRONE/
│
├── README.md
│
├── firmware/
│   ├── src/
│   ├── include/
│   ├── lib/
│   └── platformio.ini
│
├── hardware/
│   ├── esquemas/
│   ├── conexiones/
│   └── diagramas/
│
├── documentation/
│   ├── pruebas.md
│   ├── configuracion.md
│   └── troubleshooting.md
│
└── images/
    ├── drone.jpg
    ├── motores.jpg
    └── conexiones.jpg
```

---

# 20. Registro de pruebas

| Prueba          | Resultado | Observaciones |
| --------------- | --------- | ------------- |
| Alimentación    | [ ]       |               |
| Comunicación    | [ ]       |               |
| Motor 1         | [ ]       |               |
| Motor 2         | [ ]       |               |
| Motor 3         | [ ]       |               |
| Motor 4         | [ ]       |               |
| Sentido de giro | [ ]       |               |
| IMU             | [ ]       |               |
| Control remoto  | [ ]       |               |
| Despegue        | [ ]       |               |
| Estabilidad     | [ ]       |               |

---

## Resumen del procedimiento

```text
Dron preensamblado
        │
        ▼
Inspección
        │
        ▼
Instalar motores
        │
        ▼
Conectar motores
        │
        ▼
Programar controladora
        │
        ▼
Prueba SIN hélices
        │
        ▼
Verificar motores
        │
        ▼
Verificar sentidos de giro
        │
        ▼
Instalar hélices
        │
        ▼
Configurar control
        │
        ▼
Prueba de vuelo
        │
        ▼
       Dron
```

# Seguridad

Las hélices pueden provocar lesiones y los motores pueden arrancar inesperadamente. Durante la programación, configuración y diagnóstico, **retirar las hélices siempre que sea posible**.

Las baterías LiPo deben manipularse con cuidado y utilizar un cargador compatible. No utilizar una batería únicamente porque físicamente encaje en el conector: verificar siempre su tensión, corriente y compatibilidad con la electrónica del dron.
