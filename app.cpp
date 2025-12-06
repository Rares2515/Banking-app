#include "crow.h"
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

int main()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/users")([](){
        crow::json::wvalue result;
        
        try {
            sql::mysql::MySQL_Driver *driver;
            sql::Connection *con;

            // 1. Inițializează driverul
            driver = sql::mysql::get_mysql_driver_instance();
            
            // 2. Conectează-te la baza de date (schimbă cu datele tale)
            con = driver->connect("tcp://127.0.0.1:3306", "user", "parola");
            
            // 3. Selectează baza de date specifică
            con->setSchema("nume_baza_de_date");

            sql::Statement *stmt;
            sql::ResultSet *res;

            stmt = con->createStatement();
            // 4. Execută query-ul
            res = stmt->executeQuery("SELECT id, nume FROM users LIMIT 1");

            while (res->next()) {
                // Accesăm datele și le punem în JSON-ul de răspuns Crow
                result["id"] = res->getInt("id");
                result["nume"] = res->getString("nume");
            }

            delete res;
            delete stmt;
            delete con;

        } catch (sql::SQLException &e) {
            return crow::response(500, "Eroare la baza de date");
        }

        return crow::response(result);
    });

    app.port(18080).multithreaded().run();
}