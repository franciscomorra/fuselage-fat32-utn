tad\_direntry.h /.c

Contiene el tipo de dato estructura que representa una Entrada de Directorio y los Long File Name Entries asi como todas las operaciones posibles sobre estos.

Tipos de Datos:

- file\_attr
- time\_bytes
- date\_bytes
- directory\_entry
- long\_filename\_entry
- file\_node
- lfn\_sequence\_number

Funciones:

- uint32\_t DIRENTRY\_getClusterNumber(directory\_entry 