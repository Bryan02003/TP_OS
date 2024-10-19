# Rapport sur l'erreur de compilation

En essayant d'exécuter le code ci-dessous, on a rencontré l'erreur suivante, malgré le fait que le fichier soit bien présent dans le répertoire :

### Erreur rencontrée :
"tosfs.c:13:10: fatal error: fuse_lowlevel.h: No such file or directory
 #include <fuse_lowlevel.h>
          ^~~~~~~~~~~~~~~~~
compilation terminated."

### Explication :

Cette erreur indique que le compilateur ne parvient pas à trouver le fichier d'en-tête `fuse_lowlevel.h`, qui est nécessaire pour le bon fonctionnement de l'application. Bien que le fichier `tosfs.h` soit présent dans le répertoire, le fichier d'en-tête de FUSE semble manquer ou n'est pas accessible.
