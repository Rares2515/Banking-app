# PURPL3 Banking System 💜

Un sistem bancar modern, simplificat, dezvoltat folosind un backend robust în **C++** și o interfață web dinamică (HTML/CSS/JS). Aplicația permite crearea de conturi, transferuri bancare în timp real, vizualizarea istoricului și administrarea utilizatorilor.

## 🚀 Tehnologii Utilizate

* **Backend:** C++ (folosind framework-ul [Crow](https://crowcpp.org/) pentru API REST).
* **Database:** MySQL (gestionată prin `mysql-connector-c++`).
* **Frontend:** HTML5, CSS3 (Glassmorphism design), JavaScript (Vanilla).
* **Format Date:** JSON (folosind `nlohmann/json`).

## 📋 Funcționalități

* **Autentificare & Înregistrare:** Sistem securizat de login și creare conturi.
* **Dashboard Client:** Vizualizare sold, IBAN generat automat și istoric tranzacții.
* **Transferuri:** Transferuri de bani între conturi pe baza IBAN-ului.
* **Admin Panel:** Panou dedicat administratorilor pentru a vedea toți utilizatorii și a alimenta conturile cu fonduri.
* **Statistici:** Grafice (textuale) pentru încasări și cheltuieli.

---

## 🛠️ Instrucțiuni de Instalare și Rulare

### 1. Pre-rechizite

Asigură-te că ai instalate următoarele pe sistemul tău (Linux/WSL recomandat):

* Compilator C++ (g++)
* MySQL Server
* Librăria de dezvoltare MySQL Connector (`libmysqlcppconn-dev`)

```bash
# Exemplu instalare pe Ubuntu/Debian:
sudo apt update
sudo apt install build-essential libmysqlcppconn-dev mysql-server
```
### 2. Configurarea Bazei de Date
Serverul C++ este configurat să se conecteze la baza de date folosind utilizatorul `admin` și parola `admin`.

Deschide terminalul MySQL:
```bash
sudo mysql
```

Creează utilizatorul și drepturile (rulează comenzile una câte una):
```sql
CREATE USER 'admin'@'localhost' IDENTIFIED BY 'admin';
GRANT ALL PRIVILEGES ON *.* TO 'admin'@'localhost';
FLUSH PRIVILEGES;
EXIT;
```
Importă structura bazei de date din fișierul SQL inclus:

```Bash

mysql -u admin -p < banca_sql.sql
```
(Introdu parola 'admin' când este cerută).

## 3. Pregătirea fișierelor C++
Asigură-te că în folderul proiectului ai fișierele header necesare (care nu sunt incluse standard în C++):

crow_all.h (Descarcă de pe repo-ul Crow)

json.hpp (Descarcă de pe repo-ul nlohmann/json)

mysql_connection.h (Vine de obicei cu instalarea libmysqlcppconn-dev)

## 4. Compilare
Compilează serverul folosind g++. Asigură-te că linkezi librăria MySQL și pthread.

```Bash
g++ app.cpp -o server_banca -lmysqlcppconn -lpthread
```

## 5. Rulare
Pornește serverul:

```Bash
./server_banca
```
Dacă totul este OK, vei vedea un mesaj în consolă, iar serverul va rula pe portul 18080.

## 💻 Utilizare
Deschide browserul și accesează: 👉 http://localhost:18080

## Credențiale
Client Nou: Poți folosi pagina Register pentru a crea un cont nou.

## Administrator:

Email: admin

Parolă: admin123

Acces la panoul de administrare pentru a vedea baza de date și a adăuga bani în conturile clienților.

## 📂 Structura Proiectului
app.cpp - Codul sursă principal al serverului (rute, logică bancară, conexiune DB).

banca_sql.sql - Scriptul de creare a tabelelor și a datelor inițiale.

*.html - Pagini web (Index, Login, Register, Dashboard, Admin).

style.css - Stiluri CSS moderne.

app.js - Logica frontend (fetch API requests către C++).

## ⚠️ Troubleshooting
Eroare: "Nu ma pot conecta la MySQL": Verifică dacă serviciul MySQL rulează (sudo service mysql start) și dacă ai creat utilizatorul admin cu parola admin conform pasului 2.

Eroare la compilare mysql_connection.h not found: Asigură-te că ai instalat libmysqlcppconn-dev.

CORS Error: Dacă deschizi fișierul HTML direct din folder (file://), nu va merge. Trebuie să accesezi mereu prin http://localhost:18080.


---
<div align="center">

**With love de la developeri: Chiculiță Rareș-Andrei & Toderașc Octavian-Gabriel** 💜

</div>
