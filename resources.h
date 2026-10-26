#ifndef RESOURCES_H
#define RESOURCES_H

#include "crow.h"
#include "supabase.h"

// Adds the link-submission routes to the app:
//   GET  /classes/<id>/resources  -> approved links for a class
//   POST /classes/<id>/resources  -> submit a link (saved as "not approved")
void register_resource_routes(crow::SimpleApp& app, const SupabaseConfig& cfg);

#endif 