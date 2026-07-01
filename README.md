# RF-NPI

**RF-NPI** es un dispositivo multiprotocolo de generación de ruido en radiofrecuencia
basado en **ESP32** y tres módulos **nRF24L01+**, pensado para **investigación de
seguridad, pruebas de laboratorio y fines educativos** sobre el comportamiento de
distintos protocolos inalámbricos frente a interferencias.

> Proyecto basado en **[RF-Clown de CiferTech](https://github.com/cifertech/rfclown)**
> (licencia MIT). RF-NPI es una versión adaptada y modificada de ese proyecto.

---

## Aviso legal y uso responsable

Este dispositivo emite señales de radiofrecuencia que pueden **interferir con
comunicaciones inalámbricas legítimas**. En la mayoría de los países la emisión de
interferencias intencionadas está **regulada o directamente prohibida**.

- Úsalo **únicamente** en entornos controlados de tu propiedad, jaulas de Faraday o
  laboratorios aislados, y sobre dispositivos que te pertenezcan.
- **No** lo uses sobre redes, personas o equipos de terceros.
- El propósito de este proyecto es **educativo y de investigación defensiva**
  (entender cómo se comportan los protocolos ante ruido de RF).

El autor y los colaboradores no se hacen responsables del mal uso de este material.

---

## ¿Para qué sirve?

RF-NPI permite estudiar cómo distintas tecnologías inalámbricas responden a la
presencia de ruido de RF en sus bandas de operación. Cubre varias familias de
protocolos que operan en la banda de 2.4 GHz (y canales cercanos), seleccionables
desde un menú en pantalla.

---

## ¿Cómo funciona?

El corazón del dispositivo son **tres radios nRF24L01+** conectadas al ESP32 por un
bus SPI compartido. En lugar de transmitir datos, cada radio se configura para emitir
una **portadora constante** (`startConstCarrier()`) a máxima potencia sobre un
conjunto de canales, saltando entre ellos de forma pseudo-aleatoria.

1. Al encender, el ESP32 inicializa las tres radios, la pantalla OLED y los botones.
2. Con los botones **Izquierdo/Derecho** navegas entre los distintos **modos de
   operación** (cada modo tiene su propia lista de canales objetivo).
3. El **botón central (Selección)** actúa como interruptor de software que alterna
   entre `DEACTIVE` (inactivo) y `ACTIVE` (emitiendo).
4. En estado `ACTIVE`, las radios saltan continuamente entre los canales del modo
   seleccionado; en `DEACTIVE`, las radios entran en `powerDown()`.
5. La pantalla OLED muestra el modo actual, su estado (ACTIVE/DEACTIVE) y una
   interfaz animada tipo tarjeta con indicadores.

> Por seguridad, el dispositivo **siempre arranca en estado `DEACTIVE`**.

---

## Modos de operación

Seleccionables desde el menú en pantalla:

| # | Modo          | Objetivo típico                         |
|---|---------------|-----------------------------------------|
| 1 | WiFi          | Canales de 2.4 GHz WiFi                 |
| 2 | Video TX      | Transmisores de video analógico         |
| 3 | RC            | Radiocontrol                            |
| 4 | BLE           | Bluetooth Low Energy                    |
| 5 | Bluetooth     | Bluetooth clásico                       |
| 6 | USB Wireless  | Periféricos inalámbricos USB            |
| 7 | Zigbee        | Dispositivos Zigbee                     |
| 8 | NRF24         | Otros dispositivos basados en nRF24     |

Cada modo define su propia lista de canales en [`src/config.h`](src/config.h).

---

## Controles

| Control                    | Acción                                                    |
|----------------------------|-----------------------------------------------------------|
| **Botón Izquierdo** (GPIO 27) | Modo anterior                                          |
| **Botón Derecho** (GPIO 25)   | Modo siguiente                                         |
| **Botón Selección** (GPIO 26) | Activa/desactiva la emisión (toggle ACTIVE/DEACTIVE)   |
| **Interruptor de alimentación** | Enciende/apaga todo el dispositivo (hardware físico)  |

---

## Características

### Del proyecto original (RF-Clown)
- Basado en **ESP32** + **3× nRF24L01+** con bus SPI compartido.
- **8 modos de operación** con listas de canales específicas por protocolo.
- **Pantalla OLED SSD1306 128×64** (I2C) con interfaz animada.
- Navegación por menú con **3 botones** e interrupciones por hardware.
- Activación por **toggle de software** (arranque seguro en estado inactivo).

### Modificaciones propias de RF-NPI
- **Migrado de Arduino IDE a [PlatformIO](https://platformio.org/)** — estructura
  de proyecto estándar, dependencias declaradas en `platformio.ini` y compilación
  reproducible. Se separaron las definiciones de variables globales
  (`config.cpp`) de sus declaraciones (`config.h`) para resolver los errores de
  *multiple definition* al enlazar.
- **Interruptor físico de encendido/apagado** mediante módulo de carga **TP4056**
  en la línea de alimentación (batería Li-ion/LiPo → switch → `VIN`). Ver
  [CONEXIONES.md](CONEXIONES.md).
- **NeoPixel deshabilitado** — el código del LED direccionable se dejó comentado
  (no se usa este hardware), reversible en el futuro.
- **Documentación de hardware** completa en [CONEXIONES.md](CONEXIONES.md)
  (diagrama pin a pin de todos los componentes).

---

## Hardware

| Componente         | Detalle                                          |
|--------------------|--------------------------------------------------|
| MCU                | ESP32 DevKit (WROOM-32)                          |
| Radios             | 3× nRF24L01+ (alimentar a **3.3 V**)             |
| Pantalla           | OLED SSD1306 128×64, I2C                          |
| Entradas           | 3 botones (Izquierdo, Derecho, Selección)        |
| Alimentación       | Módulo TP4056 + batería Li-ion/LiPo 1S + switch  |
| LED (opcional)     | NeoPixel WS2812 *(deshabilitado en firmware)*    |

El esquema de conexiones pin a pin está detallado en **[CONEXIONES.md](CONEXIONES.md)**.

---

## Compilación y carga (PlatformIO)

Requiere [PlatformIO Core](https://docs.platformio.org/en/latest/core/) o la
extensión de PlatformIO para VS Code.

```bash
# Compilar
pio run

# Compilar y cargar al ESP32
pio run --target upload

# Monitor serie (115200 baud)
pio device monitor
```

Las dependencias (U8g2, RF24) se instalan automáticamente según lo declarado en
[`platformio.ini`](platformio.ini).

---

## Estructura del proyecto

```
RF-NPI/
├── platformio.ini      # Configuración de PlatformIO y dependencias
├── README.md           # Este archivo
├── CONEXIONES.md       # Diagrama de conexiones pin a pin
└── src/
    ├── main.cpp        # Lógica principal, UI del OLED y control de modos
    ├── config.h        # Pines, enums y declaraciones globales
    ├── config.cpp      # Definiciones de variables globales
    ├── setting.h       # Declaraciones de radios y utilidades
    ├── setting.cpp     # Configuración de las radios nRF24 y OLED
    └── neopixel.cpp    # Control de NeoPixel (deshabilitado)
```

---

## Créditos

- Proyecto original: **[RF-Clown](https://github.com/cifertech/rfclown)** por
  **[CiferTech](https://github.com/cifertech)** — licencia MIT.
- RF-NPI: adaptación a PlatformIO y modificaciones de hardware/firmware.

## Licencia

Distribuido bajo la **licencia MIT**, en concordancia con el proyecto original.
