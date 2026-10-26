#ifndef SUPABASE_H
#define SUPABASE_H
 
#include <string>
#include "crow.h" 
// Connection settings for the Supabase REST API
struct SupabaseConfig
{
    std::string url;
    std::string anon_key;
};
 
// Sends a GET to the Supabase REST API using the anon key.
// Returns the HTTP status (0 if unreachable).
long supabase_get(const SupabaseConfig& cfg, const std::string& path_and_query, std::string& body);
 
// Sends a POST (an insert) to the Supabase REST API using the anon key.
// Returns the HTTP status (0 if unreachable).
long supabase_post(const SupabaseConfig& cfg, const std::string& path, const std::string& json_body, std::string& response);
 
// Builds a JSON error response such as {"error":"Class not found"}
inline crow::response json_error(int code, const std::string& message)
{
    crow::json::wvalue body;
    body["error"] = message;   // wvalue escapes quotes/special characters
 
    crow::response res;
    res.code = code;
    res.set_header("Content-Type", "application/json");
    res.body = body.dump();
    return res;
}
 
#endif // SUPABASE_H