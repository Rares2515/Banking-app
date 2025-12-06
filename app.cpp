/* BancaServer.cpp - VERSIUNEA STABILA (FINAL FIX) */
#include "crow_all.h"
#include "json.hpp"
#include <iostream>
#include <string>
#include <random>
#include <ctime> // <-- NEGRU: ADAUGAT PENTRU RAND

// MySQL Headers
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

using json = nlohmann::json;
using namespace std;

// --- MIDDLEWARE PENTRU CORS ---
struct CORSMiddleware {
    struct context {};
    void before_handle(crow::request& req, crow::response& res, context& ctx) {}
    void after_handle(crow::request& req, crow::response& res, context& ctx) {
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    }
};

// Functie helper pentru conexiune
// Functie helper pentru conexiune
sql::Connection* connectDB() {
    try {
        sql::Driver* driver = get_driver_instance();
        // FIX FINAL SOCKET/IP: Schimbăm 127.0.0.1 în localhost
        sql::Connection* con = driver->connect("tcp://localhost:3306", "admin", "admin");
        return con;
    } catch (sql::SQLException &e) {
        cout << "[EROARE FATALA] Nu ma pot conecta la MySQL: " << e.what() << endl;
        return nullptr;
    }
}

string genereazaIBAN() {
    return "RO99BANC" + to_string(rand() % 900000000 + 100000000) + to_string(rand() % 9000 + 1000);
}

int main()
{
    // FIX: Initializare Rand (pentru IBAN unic la fiecare restart)
    srand(time(0)); 

    crow::App<CORSMiddleware> app;

    // RUTA OPTIONS (PREFLIGHT)
    CROW_ROUTE(app, "/api/<path>").methods(crow::HTTPMethod::OPTIONS)
    ([](const crow::request& req, crow::response& res, string path){
        res.code = 200;
        res.end();
    });

// --- 1. RUTA REGISTER (FIX-ul FINAL pentru NULL) ---
    CROW_ROUTE(app, "/api/register").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body);
        
        // 1. Prelucrarea datelor (cu verificare de existenta)
        // Ne asiguram ca variabilele sunt luate corect. Folosim .get<string>() pentru a fi stricti.
        string nume = body.count("nume") ? body["nume"].get<string>() : "";
        string prenume = body.count("prenume") ? body["prenume"].get<string>() : "";
        string email = body.count("email") ? body["email"].get<string>() : "";
        string pass = body.count("parola") ? body["parola"].get<string>() : "";
        
        // Verificare rapidă de bază
        if (nume.empty() || email.empty() || pass.empty()) {
            return crow::response(400, "{\"status\": \"error\", \"message\": \"Date incomplete\"}");
        }

        sql::Connection* con = connectDB();
        if(!con) return crow::response(500, "Eroare conexiune SQL");

        // *** FIX CRITIC: Specificăm schema pentru a evita NULL ***
        con->setSchema("sistem_bancar"); 

        try {
            // 2. INSERT INTO users (Verificati sa fie 4 '?' pentru 4 setString)
            sql::PreparedStatement* pstmt = con->prepareStatement(
                "INSERT INTO users(nume, prenume, email, parola_hash) VALUES(?,?,?,?)"
            );
            pstmt->setString(1, nume);
            pstmt->setString(2, prenume);
            pstmt->setString(3, email);
            pstmt->setString(4, pass); // 'pass' este valoarea pentru parola_hash
            pstmt->execute();
            delete pstmt;

            // 3. Obtinem ID-ul nou inserat (new_id)

            // 4. INSERT INTO accounts (utilizeaza new_id)
            // ... (Restul codului de inserare in accounts care este OK)
            
            sql::Statement* stmt = con->createStatement();
            sql::ResultSet* res_id = stmt->executeQuery("SELECT LAST_INSERT_ID() AS id");
            res_id->next();
            int new_id = res_id->getInt("id");
            delete res_id; delete stmt;

            pstmt = con->prepareStatement(
                "INSERT INTO accounts(user_id, iban, sold, tip_pachet) VALUES(?,?, 0.00, 'Standard')"
            );
            pstmt->setInt(1, new_id);
            pstmt->setString(2, genereazaIBAN());
            pstmt->execute();
            delete pstmt;
            delete con;
            
            return crow::response(200, "{\"status\": \"success\", \"message\": \"Cont creat cu succes\"}");
        } catch (sql::SQLException &e) {
            if (con) delete con;
            std::cout << "[EROARE SQL REGISTER]: " << e.what() << std::endl;
            return crow::response(400, "{\"status\": \"error\", \"message\": \"Date duplicate (Email existent) sau eroare SQL\"}");
        }
    });
    // --- 2. RUTA LOGIN (FIX: setSchema adaugat) ---
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

        // FIX NULL: Specificăm schema aici
        con->setSchema("sistem_bancar");

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
                j["status"] = "success"; j["role"] = "client";
                j["user_id"] = r->getInt("id"); j["nume"] = r->getString("nume") + " " + r->getString("prenume");
                j["iban"] = r->getString("iban"); j["sold"] = r->getDouble("sold");
            } else {
                j["status"] = "error"; j["message"] = "Date greșite";
            }
            delete r; delete pstmt; delete con;
            return crow::response(200, j.dump());
        } catch (sql::SQLException &e) {
            if(con) delete con;
            return crow::response(500, "Eroare server");
        }
    });

    // --- 3. RUTA ADMIN (FIX: setSchema adaugat) ---
    CROW_ROUTE(app, "/api/admin/users")
    ([](){
        sql::Connection* con = connectDB();
        if(!con) return crow::response(500, "Err SQL");

        // FIX NULL: Specificăm schema aici
        con->setSchema("sistem_bancar");
        
        try {
            sql::Statement* stmt = con->createStatement();
            
            // ATENTIE: AICI TREBUIE SA FIE "JOIN", nu "LEFT JOIN"
            sql::ResultSet* r = stmt->executeQuery(
                "SELECT u.id, u.nume, u.prenume, u.email, u.parola_hash, a.iban, a.sold "
                "FROM users u JOIN accounts a ON u.id = a.user_id"
            );

            json lista = json::array();
            while(r->next()) {
                json u;
                u["id"] = r->getInt("id"); u["nume"] = r->getString("nume");
                u["prenume"] = r->getString("prenume"); u["email"] = r->getString("email");
                u["parola"] = r->getString("parola_hash"); u["iban"] = r->getString("iban");
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