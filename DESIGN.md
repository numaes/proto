# Arquitectura de la Biblioteca Proto

Esta biblioteca implementa un runtime para lenguajes de programación, con un fuerte enfoque en estructuras de datos inmutables y un sistema de gestión de memoria concurrente de alto rendimiento.

## Principios de Diseño

El núcleo de la biblioteca se basa en los siguientes principios:

1.  **Inmutabilidad:** Todas las estructuras de datos fundamentales son inmutables. Las modificaciones no alteran las estructuras existentes, sino que crean nuevas versiones con los cambios aplicados (copy-on-write). Esto simplifica enormemente la programación concurrente al eliminar la necesidad de locks complejos para proteger los datos.
2.  **Gestión de Memoria Basada en Celdas:** Toda la memoria para los objetos del runtime se gestiona a través de celdas de tamaño fijo (64 bytes). Este enfoque elimina la fragmentación de memoria y permite el uso de un asignador (allocator) extremadamente simple y rápido.
3.  **Alto Rendimiento y Concurrencia:** El diseño está optimizado para sistemas multi-core, minimizando la contención de locks y maximizando el paralelismo.

## Gestión de Memoria y Garbage Collection (GC)

El sistema de gestión de memoria es uno de los componentes más críticos y sofisticados de la biblioteca.

### Asignador de Memoria (Allocator)

-   **Pool por Thread:** Cada thread del sistema operativo posee su propio pool de celdas de memoria libres. Cuando un thread necesita crear un nuevo objeto, toma una celda de su pool local.
-   **Asignación Rápida:** Este diseño permite una asignación de memoria casi instantánea, ya que no requiere un lock global. La contención se minimiza, permitiendo que los threads se ejecuten en paralelo sin bloquearse mutuamente durante la creación de objetos.
-   **Obtención de Bloques:** Si el pool local de un thread se agota, solicita un nuevo bloque de celdas al `ProtoSpace` global, que gestiona el heap principal.

### Garbage Collector (GC) Concurrente

El GC está diseñado para minimizar las pausas y el impacto en el rendimiento de la aplicación.

-   **Thread Dedicado:** El GC se ejecuta en su propio thread, operando en paralelo con los threads de la aplicación.
-   **GC Híbrido (Stop-the-World Parcial):** El GC no detiene el mundo por completo para todo su ciclo. La fase de "stop-the-world" es muy breve y se utiliza únicamente para recolectar de forma segura las **raíces (roots)** del sistema. Las raíces incluyen los stacks de todos los threads activos y las referencias globales (como los objetos mutables).
-   **Fases Concurrentes de Mark and Sweep:** Una vez que las raíces han sido recolectadas, las fases de marcado (`mark`) y limpieza (`sweep`) se ejecutan de forma concurrente mientras los threads de la aplicación continúan su ejecución. La inmutabilidad de los datos es clave aquí, ya que garantiza que las referencias entre objetos no cambiarán mientras el GC está trabajando.

### Ciclo de Vida de los Objetos y Limpieza por Ámbito

-   **Objetos de Corta Duración:** La biblioteca implementa una optimización para objetos de corta duración. Cuando un método o función finaliza, todas las celdas de memoria que fueron asignadas dentro de su ámbito y que no son parte del valor de retorno explícito, se devuelven a un "pool de análisis".
-   **Análisis Asíncrono:** Los objetos en este pool son candidatos para ser liberados. El GC analiza de forma asíncrona estas celdas para asegurarse de que no haya ninguna referencia viva a ellas desde las raíces del sistema. Si no hay referencias, la celda se devuelve al pool global de celdas libres, lista para ser reutilizada.
-   **Eficiencia:** Este mecanismo actúa como una forma de recolección generacional, permitiendo que la mayoría de los objetos (que suelen tener una vida corta) se recolecten de manera muy eficiente sin necesidad de un ciclo completo de mark-and-sweep sobre todo el heap.
