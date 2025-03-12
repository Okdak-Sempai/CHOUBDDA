# CHOUBDDA - Mini-SGBD en C/C++

Ce projet implémente un mini-système de gestion de base de données (SGBD) en C/C++ qui se focalise sur la gestion des enregistrements, la mémoire tampon et l’exécution de commandes SQL basiques.

## Fonctionnalités

- **Configuration de la base de données**  
  Gestion des paramètres essentiels via le module `DBConfig`.

- **Gestion de la mémoire tampon**  
  Le module `BufferManager` gère l’allocation et la gestion des tampons pour optimiser les accès disque.

- **Accès disque**  
  Le module `DiskManager` fournit une abstraction pour les opérations de lecture/écriture sur disque.

- **Stockage des enregistrements (HeapFile)**  
  Implémentation du stockage non trié des enregistrements dans des fichiers Heap.

- **Gestion des pages**  
  Le module `PageId` gère les identifiants des pages, essentiel pour la gestion des tampons et des accès disque.

- **Gestion des enregistrements et des relations**  
  Les modules `Record` et `Relation` permettent de manipuler les données et de définir les relations entre entités.

- **Cœur du SGBD**  
  Le module `SGBD` orchestre l’ensemble des opérations du système.

- **Exécution des commandes SQL**  
  Le module `SelectCommand` traite les requêtes SQL de type `SELECT`.

- **Tests et outils utilitaires**  
  Les modules `TestsProcedures` et `Tools_L` offrent des procédures de test et des fonctions utilitaires.

## Structure du Projet

