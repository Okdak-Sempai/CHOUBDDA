Pour compiler:

./launch

Ensuite dans le dossier DamienBDD, pour executer:

./SGDB -fichier_config.txt

====
fichier_config.txt:

db_path=absolute_path_folder
page_size=4096
dm_maxfilesize=12288
dm_buffercount=1[A partir de 1]
dm_policy=LRU OR MRU

====
Notes:
Utilise gcc 11.4.0, g++ 11.4.0

Un fichier_config.txt est donné a titre d'exemple, il est editable.

Chemin de bulk insert si il est relatif cèest relatif au binaire.