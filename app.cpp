/* BancaServer.cpp - SERVER FINAL */
#include "crow_all.h"
#include "json.hpp"
#include <iostream>
#include <string>
#include <random>

// MySQL Headers
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

using json = nlohmann::json;
using namespace std;

// Functie helper pentru conexiune
sql::Connection* connectDB() {
    try {
        sql::Driver* driver = get_driver_instance();
        // ATENTIE: SCHIMBATI PAROLA AICI DACA E CAZUL
        sql::Connection* con = driver->connect("tcp://127.0.0.1:3306", "root", "parola_sql_aici"); // <--- PAROLA!
        con->setSchema("sistem_bancar");
        return con;
    } catch (sql::SQLException &e) {
        return nullptr;
    }
}

// Functie generare IBAN
string genereazaIBAN() {
    return "RO99BANC" + to_string(rand() % 900000000 + 100000000) + to_string(rand() % 9000 + 1000);
}

int main()
{
    crow::SimpleApp app;

    // --- MIDDLEWARE CORS (OBLIGATORIU PENTRU BROWSER) ---
    CROW_ROUTE(app, "/api/<path>")
    .methods(crow::HTTPMethod::OPTIONS)
    ([](const crow::request& req, crow::response& res, string path){
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        res.end();
    });

    // --- 1. RUTA REGISTER ---
    CROW_ROUTE(app, "/api/register").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        crow::response res;
        res.add_header("Access-Control-Allow-Origin", "*"); // CORS pt raspuns
        
        auto body = json::parse(req.body);
        string nume = body["nume"];
        string prenume = body["prenume"];
        string email = body["email"];
        string pass = body["parola"];

        sql::Connection* con = connectDB();
        if(!con) return crow::response(500, "Eroare conexiune SQL");

        try {
            // A. Inseram Userul
            sql::PreparedStatement* pstmt = con->prepareStatement("INSERT INTO users(nume, prenume, email, parola_hash) VALUES(?,?,?,?)");
            pstmt->setString(1, nume);
            pstmt->setString(2, prenume);
            pstmt->setString(3, email);
            pstmt->setString(4, pass);
            pstmt->execute();
            delete pstmt;

            // B. Luam ID-ul noului user (pentru a-i face cont bancar)
            sql::Statement* stmt = con->createStatement();
            sql::ResultSet* res_id = stmt->executeQuery("SELECT LAST_INSERT_ID() AS id");
            res_id->next();
            int new_id = res_id->getInt("id");
            delete res_id;
            delete stmt;

            // C. Cream contul bancar (Sold 0)
            pstmt = con->prepareStatement("INSERT INTO accounts(user_id, iban, sold, tip_pachet) VALUES(?,?, 0.00, 'Standard')");
            pstmt->setInt(1, new_id);
            pstmt->setString(2, genereazaIBAN());
            pstmt->execute();
            delete pstmt;

            delete con;
            return crow::response(200, "{\"status\": \"success\"}");

        } catch (sql::SQLException &e) {
            delete con;
            cout << "Eroare SQL: " << e.what() << endl;
            return crow::response(400, "{\"status\": \"error\", \"message\": \"Email existent sau eroare SQL\"}");
        }
    });

    // --- 2. RUTA LOGIN ---
    CROW_ROUTE(app, "/api/login").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        crow::response res;
        res.add_header("Access-Control-Allow-Origin", "*");

        auto body = json::parse(req.body);
        string email = body["email"];
        string pass = body["parola"];

        // Backdoor pentru Admin (Hardcoded pt simplitate)
        if(email == "admin" && pass == "admin123") {
            json j;
            j["status"] = "success";
            j["role"] = "admin";
            return crow::response(200, j.dump());
        }

        sql::Connection* con = connectDB();
        try {
            // Cautam userul si contul lui
            sql::PreparedStatement* pstmt = con->prepareStatement(
                "SELECT u.id, u.nume, u.prenume, a.iban, a.sold FROM users u "
                "JOIN accounts a ON u.id = a.user_id "
                "WHERE u.email = ? AND u.parola_hash = ?"
            );
            pstmt->setString(1, email);
            pstmt->setString(2, pass);
            sql::ResultSet* r = pstmt->executeQuery();

            json j;
            if(r->next()) {
                j["status"] = "success";
                j["role"] = "client";
                j["user_id"] = r->getInt("id");
                j["nume"] = r->getString("nume") + " " + r->getString("prenume");
                j["iban"] = r->getString("iban");
                j["sold"] = r->getDouble("sold");
            } else {
                j["status"] = "error";
                j["message"] = "Date incorecte";
            }
            
            delete r; delete pstmt; delete con;
            return crow::response(200, j.dump());

        } catch (sql::SQLException &e) {
            if(con) delete con;
            return crow::response(500, "Eroare server");
        }
    });

    // --- 3. RUTA ADMIN DATA (Pentru tabel) ---
    CROW_ROUTE(app, "/api/admin/users")
    ([](){
        crow::response res;
        res.add_header("Access-Control-Allow-Origin", "*");

        sql::Connection* con = connectDB();
        try {
            sql::Statement* stmt = con->createStatement();
            // Luam toate datele
            sql::ResultSet* r = stmt->executeQuery(
                "SELECT u.id, u.nume, u.prenume, u.email, u.parola_hash, a.iban, a.sold "
                "FROM users u LEFT JOIN accounts a ON u.id = a.user_id"
            );

            json lista_useri = json::array();
            while(r->next()) {
                json user;
                user["id"] = r->getInt("id");
                user["nume"] = r->getString("nume");
                user["prenume"] = r->getString("prenume");
                user["email"] = r->getString("email");
                user["parola"] = r->getString("parola_hash"); // Afisam parola pt demo
                user["iban"] = r->getString("iban");
                user["sold"] = r->getDouble("sold");
                lista_useri.push_back(user);
            }

            delete r; delete stmt; delete con;
            return crow::response(200, lista_useri.dump());

        } catch(...) {
             if(con) delete con;
             return crow::response(500, "Err");
        }
    });

    app.port(18080).multithreaded().run();
}
