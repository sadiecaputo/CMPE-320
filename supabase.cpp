#include "supabase.h"
#include <curl/curl.h>
 
static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}
 
// The two headers Supabase needs on every request
static struct curl_slist* auth_headers(const SupabaseConfig& cfg)
{
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("apikey: " + cfg.anon_key).c_str());
    headers = curl_slist_append(headers, ("Authorization: Bearer " + cfg.anon_key).c_str());
    return headers;
}
 
// Sends a GET to the Supabase REST API. Returns the HTTP status (0 if unreachable).
long supabase_get(const SupabaseConfig& cfg, const std::string& path_and_query, std::string& body)
{
    CURL* curl = curl_easy_init();
    if (!curl) return 0;
 
    struct curl_slist* headers = auth_headers(cfg);
 
    std::string url = cfg.url + path_and_query;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
 
    long status = 0;
    if (curl_easy_perform(curl) == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    }
 
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return status;
}
 
// Sends a POST (an insert) to the Supabase REST API. Returns the HTTP status (0 if unreachable).
long supabase_post(const SupabaseConfig& cfg, const std::string& path, const std::string& json_body, std::string& response)
{
    CURL* curl = curl_easy_init();
    if (!curl) return 0;
 
    struct curl_slist* headers = auth_headers(cfg);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Prefer: return=minimal");
 
    std::string url = cfg.url + path;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(json_body.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
 
    long status = 0;
    if (curl_easy_perform(curl) == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    }
 
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return status;
}