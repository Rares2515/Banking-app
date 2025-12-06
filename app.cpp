/* BancaServer.cpp - VERSIUNEA CU MIDDLEWARE (CORS GARANTAT) */
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

// --- AICI ESTE SECRETUL: MIDDLEWARE PENTRU CORS ---
struct CORSMiddleware {
    struct context {};

    void before_handle(crow::request& req, crow::response& res, context& ctx) {
        // Nu facem nimic inainte
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx) {
        // Fortam aceste headere pe ORICE raspuns
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    }
};

// Functie helper pentru conexiune
sql::Connection* connectDB() {
    try {
        sql::Driver* driver = get_driver_instance();
        // ATENTIE: Verificati userul si parola (root sau admin)
        sql::Connection* con = driver->connect("tcp://127.0.0.1:3306", "admin", "admin");
        con->setSchema("sistem_bancar");
        return con;
    } catch (sql::SQLException &e) {
        cout << "Eroare SQL Connection: " << e.what() << endl;
        return nullptr;
    }
}

string genereazaIBAN() {
    return "RO99BANC" + to_string(rand() % 900000000 + 100000000) + to_string(rand() % 9000 + 1000);
}

int main()
{
    // IMPORTANT: Activam Middleware-ul aici
    crow::App<CORSMiddleware> app;

    // --- RUTA SPECIALA PENTRU PREFLIGHT (OPTIONS) ---
    // Aceasta prinde orice cerere de tip "Verificare" de la browser
    CROW_ROUTE(app, "/api/<path>")
    .methods(crow::HTTPMethod::OPTIONS)
    ([](const crow::request& req, crow::response& res, string path){
        res.code = 200;
        res.end(); // Middleware-ul va adauga headerele automat
    });

    // --- 1. RUTA REGISTER ---
    CROW_ROUTE(app, "/api/register").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body);
        string nume = body["nume"];
        string prenume = body["prenume"];
        string email = body["email"];
        string pass = body["parola"];

        sql::Connection* con = connectDB();
        if(!con) return crow::response(500, "Eroare conexiune SQL");

        try {
            sql::PreparedStatement* pstmt = con->prepareStatement("INSERT INTO users(nume, prenume, email, parola_hash) VALUES(?,?,?,?)");
            pstmt->setString(1, nume);
            pstmt->setString(2, prenume);
            pstmt->setString(3, email);
            pstmt->setString(4, pass);
            pstmt->execute();
            delete pstmt;

            sql::Statement* stmt = con->createStatement();
            sql::ResultSet* res_id = stmt->executeQuery("SELECT LAST_INSERT_ID() AS id");
            res_id->next();
            int new_id = res_id->getInt("id");
            delete res_id; delete stmt;

            pstmt = con->prepareStatement("INSERT INTO accounts(user_id, iban, sold, tip_pachet) VALUES(?,?, 0.00, 'Standard')");
            pstmt->setInt(1, new_id);
            pstmt->setString(2, genereazaIBAN());
            pstmt->execute();
            delete pstmt;
            delete con;

            return crow::response(200, "{\"status\": \"success\"}");
        } catch (sql::SQLException &e) {
            delete con;
            return crow::response(400, "{\"status\": \"error\", \"message\": \"Date invalide sau email duplicat\"}");
        }
    });

    // --- 2. RUTA LOGIN ---
    CROW_ROUTE(app, "/api/login").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body);
        string email = body["email"];
        string pass = body["parola"];

        if(email == "admin" && pass == "admin123") {
            return crow::response(200, "{\"status\": \"success\", \"role\": \"admin\"}");
        }

        sql::Connection* con = connectDB();
        if(!con) return crow::response(500, "Eroare conexiune SQL");

        try {
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
                j["message"] = "Date greșite";
            }
            delete r; delete pstmt; delete con;
            return crow::response(200, j.dump());
        } catch (sql::SQLException &e) {
            if(con) delete con;
            return crow::response(500, "Eroare server");
        }
    });

    // --- 3. RUTA ADMIN ---
    CROW_ROUTE(app, "/api/admin/users")
    ([](){
        sql::Connection* con = connectDB();
        if(!con) return crow::response(500, "Err SQL");
        
        try {
            sql::Statement* stmt = con->createStatement();
            sql::ResultSet* r = stmt->executeQuery(
                "SELECT u.id, u.nume, u.prenume, u.email, u.parola_hash, a.iban, a.sold "
                "FROM users u LEFT JOIN accounts a ON u.id = a.user_id"
            );

            json lista = json::array();
            while(r->next()) {
                json u;
                u["id"] = r->getInt("id");
                u["nume"] = r->getString("nume");
                u["prenume"] = r->getString("prenume");
                u["email"] = r->getString("email");
                u["parola"] = r->getString("parola_hash");
                u["iban"] = r->getString("iban");
                u["sold"] = r->getDouble("sold");
                lista.push_back(u);
            }
            delete r; delete stmt; delete con;
            return crow::response(200, lista.dump());
        } catch(...) {
            if(con) delete con;
            return crow::response(500, "Err");
        }
    });

    app.port(18080).multithreaded().run();
}