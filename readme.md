# CHOUBDDA - Mini-DBMS in C/C++

This project implements a mini-database management system (DBMS) in C/C++ that focuses on record management, buffer memory management, and the execution of basic SQL commands.

## Features

- **Database Configuration**  
  Manages essential parameters through the `DBConfig` module.

- **Buffer Memory Management**  
  The `BufferManager` module handles buffer allocation and management to optimize disk access.

- **Disk Access**  
  The `DiskManager` module provides an abstraction for disk read/write operations.

- **Record Storage (HeapFile)**  
  Implements unsorted record storage using heap files.

- **Page Management**  
  The `PageId` module manages page identifiers, which is essential for both buffer and disk access management.

- **Record and Relation Management**  
  The `Record` and `Relation` modules allow for data manipulation and defining relationships between entities.

- **DBMS Core**  
  The `SGBD` module orchestrates the overall operations of the system.

- **SQL Command Execution**  
  The `SelectCommand` module processes SQL `SELECT` queries.

- **Testing and Utility Tools**  
  The `TestsProcedures` and `Tools_L` modules provide testing routines and utility functions.

## Project Structure

CHOUBDDA-main/ ├── .gitignore ├── BufferManager.c & BufferManager.h
├── CMakeLists.txt
├── DBConfig.c / DBConfig.cpp & DBConfig.h
├── DBManager.cpp & DBManager.h
├── DiskManager.c & DiskManager.h
├── HeapFile.c & HeapFile.h
├── PageId.c & PageId.h
├── Record.c & Record.h
├── Relation.c & Relation.h
├── SGBD.cpp & SGBD.h
├── SelectCommand.cpp & SelectCommand.h
├── Structures.h
├── TestsProcedures.c & TestsProcedures.h
├── Tools_L.c & Tools_L.h
├── fichier_config.txt
├── launch
├── main.c / main.cpp
└── readme.txt
