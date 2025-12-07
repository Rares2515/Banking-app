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
sql::Connection* connectDB() {
    try {
        sql::Driver* driver = get_driver_instance();
        // FIX STABILITATE: Revenim la IP-ul direct 127.0.0.1
        sql::Connection* con = driver->connect("tcp://127.0.0.1:3306", "admin", "admin");
        return con;
    } catch (sql::SQLException &e) {
        // Dacă primești această eroare, serverul MySQL nu rulează sau e blocat.
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
    
    // --- 4. RUTA TRANSFER (Implementează Tranzacția SQL) ---
    CROW_ROUTE(app, "/api/transfer").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body);
        int user_id = body.count("user_id") ? body["user_id"].get<int>() : 0;
        string sursa_iban = body.count("sursa_iban") ? body["sursa_iban"].get<string>() : "";
        string destinatie_iban = body.count("destinatie_iban") ? body["destinatie_iban"].get<string>() : "";
        double suma = body.count("suma") ? body["suma"].get<double>() : 0.0;
        string detalii = body.count("detalii") ? body["detalii"].get<string>() : "Transfer Bancar";

        if (suma <= 0 || sursa_iban == destinatie_iban) {
            return crow::response(400, "{\"status\": \"error\", \"message\": \"Suma invalida sau IBAN identic.\"}");
        }

        sql::Connection* con = connectDB();
        if (!con) return crow::response(500, "Eroare conexiune SQL.");

        con->setSchema("sistem_bancar");
        
        // ** INCEPUTUL TRANZACTIEI SQL **
        con->setAutoCommit(false); 
        sql::PreparedStatement* pstmt = nullptr;
        sql::ResultSet* res = nullptr;

        try {
            // A. Verificarea Soldului (Contul Sursă)
            pstmt = con->prepareStatement(
                "SELECT sold FROM accounts WHERE iban = ? AND user_id = ? FOR UPDATE"
            );
            pstmt->setString(1, sursa_iban);
            pstmt->setInt(2, user_id);
            res = pstmt->executeQuery();
            
            if (!res->next()) {
                con->rollback(); delete con; delete res; delete pstmt;
                return crow::response(400, "{\"status\": \"error\", \"message\": \"Contul sursă nu a fost găsit.\"}");
            }
            double sold_curent = res->getDouble("sold");
            delete res; delete pstmt;
            
            if (sold_curent < suma) {
                con->rollback(); delete con;
                return crow::response(400, "{\"status\": \"error\", \"message\": \"Fonduri insuficiente.\"}");
            }

            // B. Debitarea (Scăderea din Sursă)
            pstmt = con->prepareStatement(
                "UPDATE accounts SET sold = sold - ? WHERE iban = ?"
            );
            pstmt->setDouble(1, suma);
            pstmt->setString(2, sursa_iban);
            pstmt->executeUpdate();
            delete pstmt;

            // C. Creditarea (Adunarea în Destinație)
            // Verificăm dacă IBAN-ul destinație există
            pstmt = con->prepareStatement(
                "UPDATE accounts SET sold = sold + ? WHERE iban = ?"
            );
            pstmt->setDouble(1, suma);
            pstmt->setString(2, destinatie_iban);
            int rows_affected = pstmt->executeUpdate();
            delete pstmt;

            if (rows_affected == 0) {
                 // Daca IBAN-ul destinatie nu exista, dam rollback la debitare
                con->rollback(); delete con;
                return crow::response(400, "{\"status\": \"error\", \"message\": \"IBAN destinatar invalid.\"}");
            }

            // D. Înregistrarea Tranzacției
            pstmt = con->prepareStatement(
                "INSERT INTO transactions (sursa_iban, destinatie_iban, suma, detalii) VALUES (?, ?, ?, ?)"
            );
            pstmt->setString(1, sursa_iban);
            pstmt->setString(2, destinatie_iban);
            pstmt->setDouble(3, suma);
            pstmt->setString(4, detalii);
            pstmt->execute();
            delete pstmt;

            // ** FINALUL TRANZACTIEI SQL **
            con->commit(); // Confirmăm toate operațiunile
            delete con; 
            return crow::response(200, "{\"status\": \"success\", \"message\": \"Transfer realizat cu succes!\"}");

        } catch (sql::SQLException &e) {
            con->rollback(); // Anulăm tot dacă apare orice eroare
            if (con) delete con;
            std::cout << "[EROARE SQL TRANSFER]: " << e.what() << std::endl;
            return crow::response(500, "{\"status\": \"error\", \"message\": \"Eroare la procesarea tranzacției.\"}" );
        }
    });

    // app.cpp - ADĂUGAREA RUTEI ADD FUNDS

    // --- 5. RUTA ADMIN ADD FUNDS (ADMINISTRATIVE TRANSFER) ---
    CROW_ROUTE(app, "/api/admin/add_funds").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body);
        int client_id = body.count("client_id") ? body["client_id"].get<int>() : 0;
        double suma = body.count("suma") ? body["suma"].get<double>() : 0.0;

        // Validare de bază: suma > 0 și ID-ul clientului trebuie să fie > 1 
        // (ID 1 este Fondul Băncii - nu se poate adăuga la el prin acest flow)
        if (suma <= 0 || client_id <= 1) { 
            return crow::response(400, "{\"status\": \"error\", \"message\": \"Suma sau ID client invalide.\"}");
        }

        sql::Connection* con = connectDB();
        if (!con) return crow::response(500, "Eroare conexiune SQL.");

        con->setSchema("sistem_bancar");
        con->setAutoCommit(false); // Începem tranzacția SQL
        sql::PreparedStatement* pstmt = nullptr;
        sql::ResultSet* res = nullptr;
        const string sursa_iban = "RO00FONDUR00BANCA0000"; // IBAN-ul sistemului (Fondul Băncii)

        try {
            // 1. Obtine IBAN-ul clientului (Destinație)
            pstmt = con->prepareStatement("SELECT iban FROM accounts WHERE user_id = ?");
            pstmt->setInt(1, client_id);
            res = pstmt->executeQuery();
            if (!res->next()) { 
                con->rollback(); delete con; 
                return crow::response(400, "{\"status\": \"error\", \"message\": \"Cont client inexistent.\"}"); 
            }
            string destinatie_iban = res->getString("iban");
            delete res; delete pstmt;

            // 2. Debitarea (Scăderea din Sursă - Fondul Băncii)
            // Presupunem că Fondul Băncii are întotdeauna bani.
            pstmt = con->prepareStatement("UPDATE accounts SET sold = sold - ? WHERE iban = ?");
            pstmt->setDouble(1, suma);
            pstmt->setString(2, sursa_iban);
            pstmt->executeUpdate();
            delete pstmt;

            // 3. Creditarea (Adunarea la Destinație - Client)
            pstmt = con->prepareStatement("UPDATE accounts SET sold = sold + ? WHERE iban = ?");
            pstmt->setDouble(1, suma);
            pstmt->setString(2, destinatie_iban);
            pstmt->executeUpdate();
            delete pstmt;

            // 4. Înregistrarea Tranzacției (Sistem către Client)
            pstmt = con->prepareStatement(
                "INSERT INTO transactions (sursa_iban, destinatie_iban, suma, detalii) VALUES (?, ?, ?, 'Incarcare Admin')"
            );
            pstmt->setString(1, sursa_iban);
            pstmt->setString(2, destinatie_iban);
            pstmt->setDouble(3, suma);
            pstmt->execute();
            delete pstmt;

            con->commit(); // Finalizăm tranzacția
            delete con; 
            return crow::response(200, "{\"status\": \"success\", \"message\": \"Fonduri adăugate cu succes!\"}");

        } catch (sql::SQLException &e) {
            con->rollback(); 
            if (con) delete con;
            std::cout << "[EROARE SQL ADD FUNDS]: " << e.what() << std::endl;
            return crow::response(500, "{\"status\": \"error\", \"message\": \"Eroare internă la transfer.\"}" );
        }
    });

    // --- 6. RUTA PENTRU REFRESH DATE UTILIZATOR (Dashboard) ---
    CROW_ROUTE(app, "/api/user/details").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body);
        string email = body.count("email") ? body["email"].get<string>() : "";

        if (email.empty()) {
            return crow::response(400, "{\"status\": \"error\", \"message\": \"Email lipsa.\"}");
        }

        sql::Connection* con = connectDB();
        if (!con) return crow::response(500, "Eroare conexiune SQL.");

        con->setSchema("sistem_bancar");
        sql::PreparedStatement* pstmt = nullptr;
        sql::ResultSet* res = nullptr;
        
        try {
            // Selecteaza datele clientului si soldul curent pe baza de email
            pstmt = con->prepareStatement(
                "SELECT u.id, u.nume, u.prenume, a.iban, a.sold "
                "FROM users u JOIN accounts a ON u.id = a.user_id "
                "WHERE u.email = ?"
            );
            pstmt->setString(1, email);
            res = pstmt->executeQuery();

            if (!res->next()) { 
                delete res; delete pstmt; delete con;
                return crow::response(404, "{\"status\": \"error\", \"message\": \"Utilizator negasit.\"}"); 
            }
            
            json user_data;
            user_data["status"] = "success";
            user_data["id"] = res->getInt("id");
            user_data["nume"] = res->getString("nume");
            user_data["prenume"] = res->getString("prenume");
            user_data["iban"] = res->getString("iban");
            user_data["sold"] = res->getDouble("sold");

            delete res; delete pstmt; delete con;
            return crow::response(200, user_data.dump());

        } catch (sql::SQLException &e) {
            if (con) delete con;
            std::cout << "[EROARE SQL USER DETAILS]: " << e.what() << std::endl;
            return crow::response(500, "{\"status\": \"error\", \"message\": \"Eroare interna server.\"}" );
        }
    });

   // --- 7. RUTA PENTRU ULTIMA TRANZACȚIE A UNUI UTILIZATOR ---
CROW_ROUTE(app, "/api/user/last_transaction").methods(crow::HTTPMethod::POST)
([](const crow::request& req){
    sql::Connection* con = nullptr;
    sql::PreparedStatement* stmt = nullptr;
    sql::ResultSet* rs = nullptr;

    try {
        auto body = json::parse(req.body);
        // Verificăm că avem câmpul "iban"
        string iban = body.count("iban") ? body["iban"].get<string>() : "";
        if (iban.empty()) {
            return crow::response(400, "{\"status\":\"error\",\"message\":\"IBAN lipsă\"}");
        }

        con = connectDB();
        if (!con) {
            return crow::response(500, "{\"status\":\"error\",\"message\":\"Eroare conexiune SQL\"}");
        }

        con->setSchema("sistem_bancar");

        stmt = con->prepareStatement(
            "SELECT id, sursa_iban, destinatie_iban, suma, detalii "
            "FROM transactions "
            "WHERE sursa_iban = ? OR destinatie_iban = ? "
            "ORDER BY id DESC LIMIT 1"
        );
        stmt->setString(1, iban);
        stmt->setString(2, iban);

        rs = stmt->executeQuery();

        if (!rs->next()) {
            delete rs; rs = nullptr;
            delete stmt; stmt = nullptr;
            delete con; con = nullptr;
            return crow::response(404, "{\"status\":\"empty\"}");
        }

        json t;
        t["status"] = "success";
        t["sursa_iban"] = rs->getString("sursa_iban");
        t["destinatie_iban"] = rs->getString("destinatie_iban");
        t["suma"] = rs->getDouble("suma");
        t["detalii"] = rs->getString("detalii");

        delete rs; rs = nullptr;
        delete stmt; stmt = nullptr;
        delete con; con = nullptr;

        return crow::response(200, t.dump());
    } catch (const std::exception& e) {
        std::cout << "[EROARE /api/user/last_transaction]: " << e.what() << std::endl;

        if (rs) delete rs;
        if (stmt) delete stmt;
        if (con) delete con;

        return crow::response(500, "{\"status\":\"error\",\"message\":\"Eroare interna server\"}");
    }
});


// --- 8. RUTA PENTRU STATISTICI TRANZACTII (TOTAL INCASARI/CHELTUIELI) ---
CROW_ROUTE(app, "/api/user/transactions_stats").methods(crow::HTTPMethod::POST)
([](const crow::request& req){
    sql::Connection* con = nullptr;
    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* res = nullptr;

    try {
        auto body = json::parse(req.body);
        std::string iban = body.count("iban") ? body["iban"].get<std::string>() : "";
        if (iban.empty()) {
            return crow::response(400, "{\"status\":\"error\",\"message\":\"IBAN lipsa\"}");
        }

        con = connectDB();
        if (!con) {
            return crow::response(500, "{\"status\":\"error\",\"message\":\"Eroare conexiune SQL\"}");
        }
        con->setSchema("sistem_bancar");

        pstmt = con->prepareStatement(
            "SELECT sursa_iban, destinatie_iban, suma "
            "FROM transactions "
            "WHERE sursa_iban = ? OR destinatie_iban = ?"
        );
        pstmt->setString(1, iban);
        pstmt->setString(2, iban);

        res = pstmt->executeQuery();

        double totalIn = 0.0;
        double totalOut = 0.0;

        while (res->next()) {
            std::string sursa = res->getString("sursa_iban");
            std::string dest  = res->getString("destinatie_iban");
            double suma      = res->getDouble("suma");

            if (dest == iban)  totalIn  += suma;    // banii AU INTRAT în contul clientului
            if (sursa == iban) totalOut += suma;    // banii AU IESIT din contul clientului
        }

        json j;
        j["status"]   = "success";
        j["total_in"] = totalIn;
        j["total_out"]= totalOut;

        delete res;  res = nullptr;
        delete pstmt; pstmt = nullptr;
        delete con;  con = nullptr;

        return crow::response(200, j.dump());
    } catch (const std::exception& e) {
        std::cout << "[EROARE /api/user/transactions_stats]: " << e.what() << std::endl;
        if (res)   delete res;
        if (pstmt) delete pstmt;
        if (con)   delete con;
        return crow::response(500, "{\"status\":\"error\",\"message\":\"Eroare interna server\"}");
    }
});



    // --- RUTA STATICA PENTRU FISIERE HTML ---
    
    // RUTA 1: Pagina principala (index.html)
    CROW_ROUTE(app, "/")
    ([](crow::response& res){
        res.set_static_file_info("index.html");
        res.end();
    });

    // RUTA 2: Pagina Login
    CROW_ROUTE(app, "/login.html")
    ([](crow::response& res){
        res.set_static_file_info("login.html");
        res.end();
    });

    // RUTA 3: Pagina Register
    CROW_ROUTE(app, "/register.html")
    ([](crow::response& res){
        res.set_static_file_info("register.html");
        res.end();
    });

    // RUTA 4: Pagina Dashboard (și celelalte pagini HTML)
    CROW_ROUTE(app, "/<path>")
    ([](const crow::request& req, crow::response& res, std::string path){
        if (path.find(".html") != std::string::npos || path.find(".css") != std::string::npos || path.find(".js") != std::string::npos) {
            res.set_static_file_info(path);
            res.end();
        } else {
            res.code = 404;
            res.end("404 Not Found");
        }
    });

    app.port(18080).multithreaded().run();
}