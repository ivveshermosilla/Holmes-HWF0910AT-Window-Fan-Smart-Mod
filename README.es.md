# Holmes HWF0910AT Window Fan Smart Mod

![Holmes HWF0910AT limpio y modificado](assets/photos/10-cleaned-front-view.jpg)

Modificación independiente de un ventilador doble de ventana Holmes HWF0910AT
que aún funcionaba. El proyecto reemplaza la electrónica de control original no
aislada por un controlador ESP32-S3, conserva la secuencia del botón físico y
agrega una PWA local instalable para control, termostato, horarios, iluminación,
diagnóstico, configuración Wi-Fi y actualizaciones OTA.

Este repositorio funciona como documentación técnica y como portafolio: registra
el estado inicial incierto, la investigación, los errores encontrados, las
correcciones, las decisiones de seguridad, la integración y las pruebas.

> **Seguridad:** Este prototipo conmuta 120 VAC y no es un producto certificado.
> No debe reproducirse, energizarse ni repararse sin conocimientos de tensión de
> red, aislamiento, fusibles, distancias, encapsulado e instrumentación adecuados.
> Consulta [Seguridad](docs/safety.md).

## Resultado

![PWA móvil](assets/screenshots/mobile-home.png)

- Secuencia original de 13 pasos y memoria volátil por pulsación larga.
- Estado OFF cada vez que se detecta una nueva conexión a VAC.
- Modos HIGH, LOW, termostato y velocidad por ángulo de fase entre 85 y 100%.
- Impulso de arranque de dos segundos a máxima potencia.
- Siete indicadores direccionables sincronizados con la función real.
- Temperatura DS18B20, historial horario de siete días, timer y horario semanal/diario.
- Configuración por AP, Wi-Fi 2.4 GHz, IP fija configurable y OTA separada para
  firmware y LittleFS.
- PWA bilingüe y adaptable a móvil y escritorio.

## Organización

| Ruta | Contenido |
| --- | --- |
| [`firmware/`](firmware/) | Sketch compilable y archivos LittleFS/PWA |
| [`docs/`](docs/) | Arquitectura, hardware, pines, firmware, pruebas y decisiones |
| [`assets/photos/`](assets/photos/) | Fotos seleccionadas, reducidas y sin metadatos |
| [`assets/screenshots/`](assets/screenshots/) | Capturas de la PWA instalada `0.3.10` |
| [`assets/diagrams/`](assets/diagrams/) | Material de diagramas |

La lectura técnica recomendada continúa en [Arquitectura](docs/architecture.md),
[Hardware](docs/hardware.md), [Pinout](docs/pinout.md),
[Desarrollo asistido por IA](docs/ai-assisted-development.md) y
[Galería](docs/gallery.md).

## Desarrollo asistido por IA

ChatGPT y Codex se utilizaron en distintas instancias desde la investigación
inicial hasta el desarrollo, depuración y mantenimiento del firmware y de la
PWA/LittleFS. Sus respuestas se trataron como propuestas que debían cuestionarse
y verificarse, nunca como evidencia física.

Ivves decidió cada paso y contrastó el trabajo con diagramas originales,
datasheets, mediciones de continuidad y voltaje, inspección física y operación
real del ventilador. La IA no soldó, no diseñó la ubicación de componentes, no
midió distancias ni cableado, no manipuló instrumentos y no certificó el equipo.
La instalación, el cableado y el juicio final fueron humanos. Los errores de IA
que aparecieron durante el proceso se corrigieron y forman parte del registro de
ingeniería.

Los nombres y contraseñas de redes domésticas, MAC, rutas locales y GPS de las
fotos fueron eliminados. El código conserva credenciales iniciales genéricas
documentadas; deben cambiarse antes de usar el equipo fuera de una LAN confiable.

Código y documentación original: [licencia MIT](LICENSE). Las fotos y capturas
conservan copyright de Ivves. Holmes pertenece a su respectivo titular; este
proyecto no está afiliado, patrocinado ni respaldado por Holmes.
