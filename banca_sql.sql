DROP DATABASE IF EXISTS sistem_bancar;
CREATE DATABASE sistem_bancar;
USE sistem_bancar;

-- 1. TABELA UTILIZATORI (USERS)
CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    nume VARCHAR(50) NOT NULL,
    prenume VARCHAR(50) NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    telefon VARCHAR(15),
    parola_hash VARCHAR(255) NOT NULL,
    data_inregistrare DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 2. TABELA INSTITUȚII (BANKS)
CREATE TABLE institutions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    nume_banca VARCHAR(100) NOT NULL UNIQUE,
    cod_bancar VARCHAR(10) UNIQUE, 
    adresa VARCHAR(255),
    data_infiintare DATE
);

-- 3. TABELA CONTURI (ACCOUNTS)
CREATE TABLE accounts (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT,
    iban VARCHAR(34) UNIQUE NOT NULL,
    sold DECIMAL(15, 2) DEFAULT 0.00,
    moneda VARCHAR(3) DEFAULT 'RON',
    tip_pachet VARCHAR(20) DEFAULT 'Standard',
    
    -- Cheia străină pentru utilizator
    FOREIGN KEY (user_id) REFERENCES users(id)
);

-- 4. TABELA TRANZACȚII (TRANSACTIONS)
CREATE TABLE transactions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    sursa_iban VARCHAR(34),
    destinatie_iban VARCHAR(34),
    suma DECIMAL(15, 2) NOT NULL,
    detalii VARCHAR(100),
    data_tranzactie DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 5. LEGAREA CONTURILOR LA INSTITUȚII
ALTER TABLE accounts
ADD COLUMN institution_id INT,
ADD FOREIGN KEY (institution_id) REFERENCES institutions(id);

-- ==========================================================
-- POPULARE INIȚIALĂ: CREAREA FONDULUI BĂNCII (ID 1)
-- ==========================================================

-- A. Inserăm Instituția principală (ID 1)
INSERT INTO institutions (id, nume_banca, cod_bancar)
VALUES (
    1, 
    'PURPL3 Bank System', 
    'PURPL3'
);

-- B. Inserăm Utilizatorul Sistem (Fondul Băncii) ca ID 1
INSERT INTO users (id, nume, prenume, email, parola_hash)
VALUES (
    1,
    'PURPL3', 
    'REZERVA', 
    'bank_reserve@purpl3.ro', 
    'SYSTEM_LOCKED'
);

-- C. Inserăm Contul de Rezervă al Băncii (IBAN-ul de Sursă)
INSERT INTO accounts (user_id, iban, sold, moneda, tip_pachet, institution_id)
VALUES (
    1, -- Leaga de user ID 1 (PURPL3 REZERVA)
    'RO99BANKFUND00000000000001', 
    999999999.99, -- Sold mare pentru simulare
    'RON', 
    'System Reserve',
    1 -- Leaga de institution ID 1 (PURPL3 Bank)
);

-- D. Setăm contorul pentru clienții reali să înceapă de la 2
-- Următorul client înregistrat va primi ID 2.
ALTER TABLE users AUTO_INCREMENT = 2;

SELECT 'Schema sistem_bancar și Fondul Băncii (ID 1) au fost create.' AS Status;