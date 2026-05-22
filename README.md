<div align="center">

# ARGOS
### Sistema Autónomo Resiliente de Operaciones en Vuelo

*Software de misión autónomo embarcado para vehículos aéreos no tripulados*

![Estado](https://img.shields.io/badge/estado-definición%20de%20arquitectura-blue)
![ROS2](https://img.shields.io/badge/ROS2-Humble%20%2F%20Iron-informational)
![Lenguaje](https://img.shields.io/badge/lenguaje-C%2B%2B17-informational)
![Licencia](https://img.shields.io/badge/licencia-Propietaria-red)

</div>

---

## El nombre

En la mitología griega, **Argos Panoptes** (Ἄργος Πανόπτης — *el que todo lo ve*)
era un gigante con cien ojos que jamás dormía. Su función era vigilar, proteger y
nunca fallar en su guardia — sin importar lo que ocurriera a su alrededor.

El nombre describe al sistema con precisión.

ARGOS nunca pierde de vista el estado del vehículo. Observa cada subsistema,
monitorea cada sensor, detecta cada falla y mantiene la misión en curso — incluso
cuando el operador en tierra guarda silencio. No espera instrucciones. Actúa.

*Todo lo ve. Siempre activo. Siempre resiliente.*

---

## Qué es ARGOS

ARGOS es una **capa de software de ejecución autónoma de misiones** que corre en
un companion computer a bordo de un UAV. Se ubica entre el flight controller (PX4)
y la Ground Control Station (GCS), actuando como el cerebro de misión local del
vehículo.
┌─────────────────────────────────┐
│         GCS  (tierra)           │
└─────────────────┬───────────────┘
│  enlace cifrado
┌─────────────────▼───────────────┐
│     ARGOS  (companion computer) │
└─────────────────┬───────────────┘
│
┌─────────────────▼───────────────┐
│     PX4  (flight controller)    │
└─────────────────────────────────┘

Si PX4 es el piloto automático que mantiene el vehículo estable, ARGOS es el
copiloto inteligente que decide **qué hacer**, **cuándo hacerlo** y **qué hacer
cuando algo sale mal** — sin requerir una conexión activa con tierra.

---

## El problema que resuelve

Los fabricantes nacionales de UAVs construyen airframes capaces e integran flight
controllers robustos. Lo que no tienen es una **capa de software de misión local,
auditable y mantenible**. Hoy, la lógica de misión depende de software importado
que no fue diseñado para operar de forma completamente autónoma en campo — software
que no puede auditarse, certificarse ni adaptarse a requisitos de defensa nacionales.

La pérdida del enlace con la GCS puede resultar en comportamiento indefinido.
El código fuente no es propiedad de quien lo opera. No existe un producto nacional
que resuelva esto.

ARGOS está diseñado para llenar ese vacío.

---

## Qué hace

- Ejecuta misiones precargadas de forma autónoma, sin requerir conexión activa con la GCS
- Monitorea cada subsistema crítico en tiempo real y responde a fallas de forma automática
- Gestiona el ciclo de vida de vuelo completo a través de una máquina de estados determinista
- Mantiene un canal de comunicación cifrado y autenticado con la estación en tierra
- Genera logs estructurados y reportes de misión para análisis post-vuelo

---

## Tecnología

Construido sobre **ROS2** en **C++17**, corriendo en un companion computer junto a
un flight controller PX4. Diseñado para deployment en hardware embebido como
Raspberry Pi 4 o NVIDIA Jetson Nano.

---

## Referentes internacionales

ARGOS pertenece a la misma familia arquitectónica que Auterion AuterionOS, NASA cFS,
Anduril Lattice y Shield AI Hivemind. Ninguno de esos sistemas es auditable,
certificable ni adaptable por la industria de defensa argentina. ARGOS lo es.

---

## Estado del proyecto

El proyecto se encuentra actualmente en la **fase de definición de arquitectura**.
La implementación sigue la finalización de un plan de formación estructurado en
sistemas de integración con ROS2.

| Fase | Descripción | Estado |
|------|-------------|--------|
| Definición de arquitectura | Nodos, máquinas de estado, interfaces, schemas de mensajes | ✅ Completo |
| Prueba de concepto | Nodos principales funcionales en simulación | 🔲 En curso |
| Demostrador SIL | Misión completa en PX4 SITL + Gazebo | 🔲 Planificado |
| v2.0 | Autonomía extendida, redundancia de sensores, interfaz GCS | 🔲 Planificado |
| v3.0 | Integración en hardware real + verificación formal | 🔲 Planificado |

---

## Licencia

ARGOS es software propietario. Este repositorio es público únicamente con fines de
referencia. No se otorga ningún derecho de uso, copia, modificación o distribución
de ninguna parte de este software sin autorización escrita explícita del autor.

Ver [LICENSE](./LICENSE) para más detalles.

---

## Autor

**Santiago Flores**  
Ingeniería en Informática — Universidad Blas Pascal, Córdoba, Argentina  
[github.com/SantiFlores06](https://github.com/SantiFlores06)
