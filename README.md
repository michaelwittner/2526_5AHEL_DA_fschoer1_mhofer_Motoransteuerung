# 2526_5AHEL_DA_fschoer1_mhofer_Motoransteuerung

Redesign Hardware Motoransteuerung Brushless Motor

Arbeitsbericht [Schörkhuber](Schörkhuber.md) |  [Hofer](Hofer.md)

## Aufgabenbeschreibung
... muss hier noch durch die Schüler ergänzt werden ...

### Theorie
* Wie funktioniert der Brushless, welche Signale müsse mit welchem Timing gesendet werden?

### Hardware
* Blockschaltbild für Motoransteuerung
* Schaltung zur Ansteuerung Brushless-Motor
* Schaltung zur Strommessung
* Bauteilauswahl und Bestellung

### Software

#### Microcontroller
* Generierung von Signalen zur Motoransteuerung mit RT-Threads

#### PC
* Java Datenaustausch über die Serielle Schnittstelle - [jSerialComm](https://fazecast.github.io/jSerialComm/|jSerialComm)
* Datenerfassung/Datenaustausch
* 
##### Serial Commands
Allgemeine Struktur:
``COMMAND <whitespace> PAR1 <whitespace> PAR2 ... <CR>``

* ``LED [RED|GREEN|YELLOW] [ON|OFF]``
* 

# Trello - Aufgabenplanung
[Trello](https://trello.com/b/73xTd0gu/brushless-motoransteuerung)
