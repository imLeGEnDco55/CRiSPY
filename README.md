# CRiSPY - Nvidia Studio Voice Audio Enhancer

¡Bienvenido a **CRiSPY**! Este es un proyecto de código abierto diseñado para mejorar la calidad del audio de voz utilizando la tecnología de inteligencia artificial **Nvidia Studio Voice NIM** a través de gRPC.

El proyecto está estructurado en tres componentes principales:
1. **Plugin VST3 (JUCE):** Un plugin de audio compatible con DAWs (Digital Audio Workstations) que procesa la señal de audio.
2. **Aplicación Standalone (Win32 & Direct2D):** Una aplicación de escritorio nativa e interactiva de Windows (Win32) construida con Direct2D, que cuenta con una interfaz fluida para arrastrar y soltar (drag & drop) archivos de audio y procesarlos al instante.
3. **Script de Python:** Una herramienta de consola rápida para automatizar el procesamiento de archivos de audio mediante llamadas directas a la API gRPC de NVIDIA.

---

## Características
- **Procesamiento de Voz de Alta Calidad:** Limpieza y mejora de ruido basada en IA utilizando los modelos avanzados de NVIDIA.
- **Doble Interfaz Windows:**
  - Plugin VST3 integrado en DAWs (como Reaper, Ableton Live, FL Studio).
  - Aplicación de escritorio independiente con soporte para arrastrar y soltar.
- **Script de Automatización Python:** Ideal para su uso en terminales, scripts o procesamiento por lotes (batch).
- **Compilación optimizada con CMake:** Soporte multiplataforma y generación automatizada de recursos de compilación.

---

## Estructura del Directorio
- `src/` - Código fuente de C++ (del plugin VST3 de JUCE y de la aplicación standalone Win32).
- `python/` - Herramienta CLI de Python e interfaces gRPC (`process_audio.py`).
- `JUCE/` - SDK de JUCE (incluido como submódulo de Git).
- `CMakeLists.txt` - Configuración global de compilación con CMake.
- `Crispy.png` - Recursos visuales de la aplicación.

---

## Requisitos Previos

### 1. Clave de API de NVIDIA
Para que el procesamiento funcione, necesitas obtener una API Key de NVIDIA Studio Voice NIM:
1. Regístrate en [NVIDIA Build](https://build.nvidia.com/).
2. Busca el modelo **Studio Voice** y genera tu API Key personal.

### 2. Para Compilar en C++ (Plugin y Standalone)
- **Sistema Operativo:** Windows 10/11
- **Compilador:** Visual Studio 2022 (con herramientas C++)
- **Herramienta de compilación:** CMake 3.22 o superior

### 3. Para Ejecutar el Script de Python
- **Python 3.8 o superior**
- Las dependencias principales se instalarán automáticamente al iniciar el script por primera vez (`grpcio`, `soundfile`, `numpy`).

---

## Cómo Compilar el Proyecto (C++)

1. Abre tu terminal de PowerShell en la raíz del proyecto.
2. Genera los archivos de compilación usando CMake:
   ```bash
   cmake -B build
   ```
3. Compila el proyecto:
   ```bash
   cmake --build build --config Release
   ```
   Esto generará los siguientes binarios:
   - El plugin VST3 listo para tu DAW en la carpeta de salida correspondiente.
   - El ejecutable independiente (`NvidiaStudioVoiceEnhancer.exe`).

---

## Cómo usar el script de Python

Puedes ejecutar el limpiador de audio directamente desde tu consola usando Python:

```bash
python python/process_audio.py --input "ruta/a/tu/audio_con_ruido.wav" --output "ruta/a/tu/audio_limpio.wav" --api-key "TU_NVIDIA_API_KEY"
```

El script se encargará de:
1. Normalizar el formato del audio de entrada a 48,000Hz, mono y PCM de 16 bits.
2. Conectarse al servicio gRPC de NVIDIA.
3. Transmitir el audio en bloques y guardar la respuesta limpia en el archivo de salida indicado.

---

## Contribuciones y Licencia

Este proyecto es de código abierto.
- La parte del plugin de audio está construida sobre **JUCE**, por lo tanto, el proyecto está sujeto a la licencia **GNU General Public License v3 (GPLv3)**. Consulta el archivo [LICENSE](LICENSE) para obtener más detalles.
- Si deseas contribuir, por favor abre un Pull Request o crea un Issue para discutir mejoras.

---
*Desarrollado con ❤️ por imLeGEnDco y la comunidad.*
