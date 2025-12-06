cmake_minimum_required(VERSION 3.10)
project(ServerCrowMysql)

set(CMAKE_CXX_STANDARD 17) # Crow cere minim C++17 (sau C++14 in versiuni mai vechi)

# 1. Găsim pachetele necesare
# Găsește Crow (dacă l-ai instalat în sistem) sau include folderul manual
find_package(Crow QUIET) 

# Găsește MySQL Connector
# Notă: Pe Linux, uneori pachetul se numește doar 'mysqlcppconn'
find_library(MYSQL_LIB mysqlcppconn) 

# 2. Definim executabilul tău (sursa ta este main.cpp)
add_executable(ServerApp app.cpp)

# 3. Includem directoarele necesare (opțional, dacă compilatorul nu le vede)
# include_directories(/usr/include/mysql-cppconn-8) # Decomentează dacă ai erori de "file not found"

# 4. Partea CRITICĂ: Legăm (Link) bibliotecile de aplicația ta
if(Crow_FOUND)
    target_link_libraries(ServerApp PRIVATE Crow::Crow ${MYSQL_LIB})
else()
    # Dacă Crow e doar copiat ca fișier header (crow_all.h), trebuie doar să linkezi pthread
    target_link_libraries(ServerApp PRIVATE pthread ${MYSQL_LIB})
endif()