pfs\_fat32.c

Archivo fuente que contiene lo que viene a ser la capa de implementacion de FAT32, en esta se encontrara el manejo especifico de este tipo de Filesystem. Incluye "tad\_fat.h" para el manejo de la FAT.

Hasta el momento contiene las funciones:

// Lee la tabla fat en la estructura FAT\_struct
uint32\_t fat32\_readFAT(FAT\_struct **fat);**

// Lee el boot sector en la estructura BS\_struct
uint32\_t fat32\_readBootSector(BS\_struct **bs);**

// Obtiene los datos guardados en el cluster dado
uint32\_t fat32\_getClusterData(uint32\_t cluster\_no,char buf);