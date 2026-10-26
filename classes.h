#ifndef CLASSES_H
#define CLASSES_H
 
#include "crow.h"
#include "supabase.h"
 
// Adds the class routes to the app:
//   GET /classes        -> list/search classes (see SEARCH_FIELDS in classes.cpp)
//   GET /classes/<id>   -> one class
void register_class_routes(crow::SimpleApp& app, const SupabaseConfig& cfg);
 
#endif // CLASSES_H