DROP DATABASE IF EXISTS sistem_bancar;
CREATE DATABASE sistem_bancar;
USE sistem_bancar;

CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    nume VARCHAR(50) NOT NULL,
    prenume VARCHAR(50) NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    telefon VARCHAR(15),
    parola_hash VARCHAR(255) NOT NULL,
    data_inregistrare DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE accounts (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT,
    iban VARCHAR(34) UNIQUE NOT NULL,
    sold DECIMAL(15, 2) DEFAULT 0.00,
    moneda VARCHAR(3) DEFAULT 'RON',
    tip_pachet VARCHAR(20) DEFAULT 'Standard',
    FOREIGN KEY (user_id) REFERENCES users(id)
);

CREATE TABLE transactions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    sursa_iban VARCHAR(34),
    destinatie_iban VARCHAR(34),
    suma DECIMAL(15, 2) NOT NULL,
    detalii VARCHAR(100),
    data_tranzactie DATETIME DEFAULT CURRENT_TIMESTAMP
);
