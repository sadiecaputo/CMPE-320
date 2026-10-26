#include "crow.h"
#include <curl/curl.h>
#include <cstdio>
#include <cstdlib>
#include <string>
 
#include "supabase.h"
#include "classes.h"
#include "resources.h"
 
int main()
{
    const char* url = std::getenv("SUPABASE_URL");
    const char* key = std::getenv("SUPABASE_ANON_KEY");
 
    if (!url || !key) {
        fprintf(stderr, "Set SUPABASE_URL and SUPABASE_ANON_KEY first.\n");
        return 1;
    }
    SupabaseConfig cfg{url, key};
 
    curl_global_init(CURL_GLOBAL_DEFAULT);
 
    crow::SimpleApp app;
 
    CROW_ROUTE(app, "/")([]() {
        return "Hello from CMPE-320!";
    });
 
    // GET /classes, GET /classes/<id>   (search options live in classes.cpp)
    register_class_routes(app, cfg);
 
    // GET and POST /classes/<id>/resources   (see resources.cpp)
    register_resource_routes(app, cfg);
 
    // Runs on PORT if set, else 8080
    const char* port_env = std::getenv("PORT");
    int port = port_env ? std::stoi(port_env) : 8080;
 
    app.port(port).multithreaded().run();
 
    curl_global_cleanup();
    return 0;
}
 