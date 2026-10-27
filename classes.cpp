#include "classes.h"
#include <curl/curl.h>
#include <cctype>
#include <cstring>
#include <string>
 
// ============================================================================
//  SEARCH CONFIGURATION  --  this is the part you edit to change what's searchable
// ============================================================================
//
// To add a new search option, add ONE line to SEARCH_FIELDS below.
// Example: to search by instructor you only need   GET /classes?instructor=smith
// To search a new column (e.g. "requirements"), first make sure the column
// exists in your Supabase "classes" table, then add a line for it.
 
enum class Match {
    Contains,    // *term*  -> term appears anywhere
    StartsWith,  // term*   -> value begins with term
    Exact        // term    -> whole value equals term (case-insensitive)
};
 
struct SearchField {
    const char* param;        // the URL parameter:   /classes?<param>=...
    const char* column;       // the column in the Supabase table
    Match       match;        // how the term is matched
    size_t      max_len;      // longest term accepted
    const char* extra_chars;  // characters allowed besides letters and digits
};
 
// Every entry here becomes a filter. If several are given in one request,
// ALL of them must match (AND), e.g.  /classes?instructor=smith&term=fall
static const SearchField SEARCH_FIELDS[] = {
    //  param          column         match              max  extra chars
    {  "name",        "name",        Match::Contains,    100, " -"    },
    {  "code",        "code",        Match::StartsWith,   20, "-"     },
    {  "instructor",  "instructor",  Match::Contains,    100, " -.'"  },
    {  "term",        "term",        Match::Contains,     40, " -"    },
    // {  "requirements", "requirements", Match::Contains, 100, " -,." },   // <- example of a new one
};
 
// Quick-search box: /classes?q=320 matches if ANY of these columns contains "320" (OR).
static const char* const QUICK_SEARCH_COLUMNS[] = { "name", "code", "instructor" };
static const SearchField QUICK_SEARCH = { "q", nullptr, Match::Contains, 100, " -" };
 
// Columns returned by the list endpoint (add new columns here to show them in results)
static const char* const LIST_COLUMNS = "id,code,name,instructor,term";
 
// Max rows returned from one search
static const int RESULT_LIMIT = 100;
 
// NOTE on safety: the allowed-character rule below is what keeps users from
// injecting PostgREST operators or LIKE wildcards. If you ever allow '%', '_',
// ',', '(' or ')' in extra_chars, you'll need to handle them specially.
 
//  Helpers 
static std::string trim_spaces(const std::string& s)
{
    auto first = s.find_first_not_of(' ');
    if (first == std::string::npos) return "";
    return s.substr(first, s.find_last_not_of(' ') - first + 1);
}
 
// Trims `value` in place. Returns "" if it's acceptable, otherwise an error message.
static std::string validate_term(const SearchField& f, std::string& value)
{
    value = trim_spaces(value);
    if (value.size() > f.max_len) {
        return std::string("'") + f.param + "' must be at most " + std::to_string(f.max_len) + " characters";
    }
    for (char ch : value) {
        unsigned char c = static_cast<unsigned char>(ch);
        if (!(std::isalnum(c) || (c != 0 && std::strchr(f.extra_chars, c)))) {
            return std::string("'") + f.param + "' contains a character that isn't allowed";
        }
    }
    return "";
}
 
static std::string make_pattern(Match m, const std::string& v)
{
    switch (m) {
        case Match::Contains:   return "*" + v + "*";   // PostgREST uses * as the wildcard
        case Match::StartsWith: return v + "*";
        case Match::Exact:      return v;
    }
    return v;
}
 
// URL-encodes a string. Returns false if encoding failed.
static bool url_encode(const std::string& s, std::string& out)
{
    char* enc = curl_easy_escape(nullptr, s.c_str(), static_cast<int>(s.size()));
    if (!enc) return false;
    out = enc;
    curl_free(enc);
    return true;
}
 
// Forwards Supabase's answer to the caller
static crow::response pass_through(long status, const std::string& body)
{
    crow::response res;
    res.code = static_cast<int>(status);
    res.set_header("Content-Type", "application/json");
    res.body = body;
    return res;
}
 
//  Routes 
void register_class_routes(crow::SimpleApp& app, const SupabaseConfig& cfg)
{
    // GET /classes                      -> all classes
    // GET /classes?name=calc            -> name contains "calc"
    // GET /classes?code=MTH             -> code starts with "MTH"
    // GET /classes?instructor=smith     -> instructor contains "smith"
    // GET /classes?q=320                -> name OR code OR instructor contains "320"
    // Any combination works:  /classes?code=CMPE&term=fall
    CROW_ROUTE(app, "/classes")([&cfg](const crow::request& req) {
        std::string path = std::string("/rest/v1/classes?select=") + LIST_COLUMNS
                         + "&order=name.asc&limit=" + std::to_string(RESULT_LIMIT);
 
        // One filter per entry in SEARCH_FIELDS
        for (const SearchField& f : SEARCH_FIELDS) {
            const char* raw = req.url_params.get(f.param);
            if (!raw) continue;
 
            std::string value = raw;
            std::string problem = validate_term(f, value);
            if (!problem.empty()) return json_error(400, problem);
            if (value.empty()) continue;
 
            std::string enc;
            if (!url_encode(make_pattern(f.match, value), enc)) {
                return json_error(500, "Could not encode search");
            }
            path += std::string("&") + f.column + "=ilike." + enc;
        }
 
        // Quick-search box: match ANY of QUICK_SEARCH_COLUMNS
        if (const char* raw = req.url_params.get(QUICK_SEARCH.param)) {
            std::string value = raw;
            std::string problem = validate_term(QUICK_SEARCH, value);
            if (!problem.empty()) return json_error(400, problem);
 
            if (!value.empty()) {
                std::string enc;
                if (!url_encode(make_pattern(QUICK_SEARCH.match, value), enc)) {
                    return json_error(500, "Could not encode search");
                }
                // PostgREST "or" group:  or=(name.ilike.*x*,code.ilike.*x*,...)
                std::string group;
                for (const char* col : QUICK_SEARCH_COLUMNS) {
                    if (!group.empty()) group += ",";
                    group += std::string(col) + ".ilike." + enc;
                }
                path += "&or=(" + group + ")";
            }
        }
 
        std::string body;
        long status = supabase_get(cfg, path, body);
        if (status == 0) {
            return json_error(502, "Could not reach database");
        }
        return pass_through(status, body);
    });
 
    // GET /classes/:id  -> one class, with every column
    CROW_ROUTE(app, "/classes/<int>")([&cfg](int id) {
        std::string path = "/rest/v1/classes?select=*&id=eq." + std::to_string(id);
 
        std::string body;
        long status = supabase_get(cfg, path, body);
        if (status == 0) {
            return json_error(502, "Could not reach database");
        }
        // PostgREST returns a list, so a missing class is an empty list
        if (status == 200 && body == "[]") {
            return json_error(404, "Class not found");
        }
        return pass_through(status, body);
    });
}