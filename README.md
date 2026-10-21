# EuroSky Connect

Comparación de rutas aéreas con algoritmos de grafos, restricciones operativas y una interfaz web.

[Ficha y evidencia en el portafolio](https://juan-coronado.github.io/projects/eurosky-connect/)

| Periodo original | Dificultad académica | Estado de esta versión |
| --- | --- | --- |
| Junio de 2026 | Avanzada | Comprobado localmente |

## Funcionalidades

- BFS, DFS, Dijkstra, Bellman-Ford y programación dinámica.
- Comparación de rutas y simulación Monte Carlo.
- Servidor HTTP C++, datos CSV y gestión de aeronaves.

## Ejecución

Windows con las herramientas C++ de Visual Studio. Abrir Developer PowerShell en la raíz del repositorio. El servidor escucha solo en localhost; el mapa es opcional y requiere una clave propia en GOOGLE_MAPS_API_KEY.

```powershell
cl /nologo /std:c++17 /EHsc /utf-8 programa/EuroSkyConnect/backend.cpp ws2_32.lib /Fe:programa/EuroSkyConnect/EuroSkyConnectWeb.exe
Set-Location programa/EuroSkyConnect
.\EuroSkyConnectWeb.exe
```

Abrir http://127.0.0.1:8080. EUROSKY_PORT permite elegir otro puerto. Bootstrap y el exportador PDF originales usan bibliotecas CDN. No exponer este servidor educativo directamente a Internet.

## Comprobación disponible

Compilado y ejecutado en Windows. Pruebas de algoritmos y de API; resultados reales visibles en el navegador.

Ejecutar desde la raíz del repositorio.

```powershell
cl /nologo /std:c++17 /EHsc /utf-8 tests/algoritmos.cpp ws2_32.lib /Fe:pruebas.exe
.\pruebas.exe
```

[Registro de ejecución](evidencia/ejecucion.txt)

![Captura real de EuroSky calculando rutas con el mapa opcional sin configurar.](evidencia/aplicacion.jpg)

Las comprobaciones anteriores corresponden a la recuperación de octubre de 2026; esta publicación conserva sus registros.

## Aprendizaje

- Comparar estrategias de exploración y optimización.
- Integrar un motor algorítmico C++ con una interfaz web.

## Revisión para el portafolio

- Retirada la clave Maps incrustada y separada la configuración del código.
- Validación de entradas, POST para cambios y guardado CSV atómico.

## Alcance

Datos académicos sin tarifas comerciales en vivo. El mapa Google es opcional y no se comprobó con una clave activa. Servidor local educativo, sin autenticación de producción.

## Contexto académico y procedencia

Este repositorio conserva el alcance académico y los créditos disponibles en el material original. La recuperación, las correcciones y las reconstrucciones de octubre de 2026 se realizaron con asistencia de Codex/IA y se distinguen del trabajo original. La contribución individual de cada integrante no está desglosada aquí.
