tad\_fat.c /.h

Contiene la definicion del tipo de dato estructura de una tabla FAT y las operaciones que se peuden realizar sobre este.

Funciones:

// Obtiene la cadena de clusters que le sigue al cluster pasado
cluster\_node**FAT\_getClusterChain(FAT\_struct**fat,uint32\_t first\_cluster);

// Obtiene una lista de clusters libres
cluster\_node**FAT\_getFreeClusters(FAT\_struct** FAT);