#include "resources.h"
#include <cctype>
#include <cstdio>
#include <string>

//helper code for link submissions -- UPDATE AND CHANGE

static std::string trim_spaces(const std::string& s)
{
    auto first = s.find_first_not_of(' ');
    if (first == std::string::npos) return "";
    return s.substr(first, s.find_last_not_of(' ') - first + 1);
}

static bool has_control_chars(const std::string& s)
{
    for (unsigned char c : s) {
        if (c < 32 || c == 127) return true;
    }
    return false;
}

// Returns "" if the URL is acceptable, otherwise a message explaining why not.
static std::string check_url(const std::string& url)
{
    if (url.empty()) return "url is required";
    if (url.size() > 2048) return "url must be at most 2048 characters";

    // Only http(s) links. This blocks javascript:, data:, file:, and similar.
    std::string start = url.substr(0, 8);
    for (char& c : start) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    size_t scheme_len = 0;
    if (start.rfind("http://", 0) == 0) scheme_len = 7;
    else if (start.rfind("https://", 0) == 0) scheme_len = 8;
    else return "url must start with http:// or https://";

    // No spaces or control characters; anything non-ASCII must be percent-encoded
    for (unsigned char c : url) {
        if (c <= 32 || c >= 127) return "url may not contain spaces or special characters (percent-encode them)";
    }
    // There must be a host name right after the scheme
    if (url.size() == scheme_len || url[scheme_len] == '/' || url[scheme_len] == '?' || url[scheme_len] == '#') {
        return "url must include a host name";
    }
    return "";
}


void register_resource_routes(crow::SimpleApp& app, const SupabaseConfig& cfg)
{
    // GET  /classes/:id/resources  -> approved links attached to a class
    // POST /classes/:id/resources  -> submit a link: {"title":"Lecture notes","url":"https://..."}
    //                                 Saved as "not approved"; an admin approves it in Supabase.
    CROW_ROUTE(app, "/classes/<int>/resources").methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post)
    ([&cfg](const crow::request& req, int id) {
        if (req.method == crow::HTTPMethod::Get) {
            std::string path = "/rest/v1/class_resources?select=id,title,url,created_at&class_id=eq."
                               + std::to_string(id) + "&approved=eq.true&order=created_at.desc";
            std::string body;
            long status = supabase_get(cfg, path, body);
            if (status == 0) {
                return json_error(502, "Could not reach database");
            }

            crow::response res;
            res.code = static_cast<int>(status);
            res.set_header("Content-Type", "application/json");
            res.body = body;
            return res;
        }

        //POST method: submit a link

        //ensure URL is valid
        // if (cfg.secret_key.empty()) {
        //     return json_error(503, "Submissions are not enabled on this server");
        // }
        if (req.body.size() > 10000) {
            return json_error(413, "Request too large");
        }

        auto json = crow::json::load(req.body);
        if (!json || json.t() != crow::json::type::Object) {
            return json_error(400, "Body must be a JSON object");
        }
        if (!json.has("title") || json["title"].t() != crow::json::type::String ||
            !json.has("url") || json["url"].t() != crow::json::type::String) {
            return json_error(400, "Body must include text fields: title and url");
        }

        // Validate the title and URL
        std::string title = trim_spaces(json["title"].s());
        std::string link = trim_spaces(json["url"].s());

        //ensure title is valid
        if (title.empty() || title.size() > 120) {
            return json_error(400, "title must be 1 to 120 characters");
        }
        if (has_control_chars(title)) {
            return json_error(400, "title may not contain control characters");
        }
        std::string url_problem = check_url(link);
        if (!url_problem.empty()) {
            return json_error(400, url_problem);
        }

        // Make sure the class exists before saving anything for it
        std::string class_body;
        long class_status = supabase_get(cfg, "/rest/v1/classes?select=id&id=eq." + std::to_string(id), class_body);
        if (class_status == 0) {
            return json_error(502, "Could not reach database");   
        }
        if (class_status != 200) {
            return json_error(502, "Could not verify class");
        }
        if (class_body == "[]") {
            return json_error(404, "Class not found");
        }

        // wvalue escapes quotes and special characters for us
        crow::json::wvalue row;
        row["class_id"] = id;
        row["title"] = title;
        row["url"] = link;
        // "approved" is left out on purpose: the database defaults it to false


        //error messages
        std::string response;
        long status = supabase_post(cfg, "/rest/v1/class_resources", row.dump(), response);
        if (status == 0) {
            return json_error(502, "Could not reach database");
        }
        if (status != 201) {
            // Log the details privately instead of sending them to the user
            fprintf(stderr, "Insert into class_resources failed (HTTP %ld): %s\n", status, response.c_str());
            return json_error(502, "Could not save submission");
        }

        crow::response res;
        res.code = 202;
        res.set_header("Content-Type", "application/json");
        res.body = "{\"status\":\"pending_review\"}";
        return res;
    });
}
 