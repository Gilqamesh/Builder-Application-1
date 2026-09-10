#ifndef M03GAGBHSQFSQBLHWVELROU7NC_JSON_API_H
#define M03GAGBHSQFSQBLHWVELROU7NC_JSON_API_H

# include "json.hpp"

/**
 * @file
 * @brief Exposes nlohmann::json for parsing, manipulating, and serializing JSON values.
 *
 * Include this module-qualified api.h from a consumer. The source phase downloads
 * the checksum-pinned json.hpp for nlohmann/json v3.12.0, as specified in
 * builder.cpp; the interface phase publishes both headers beneath the module
 * prefix. A source checkout alone does not contain json.hpp. This integration is
 * header-only and adds no wrapper namespace, initialization, or library to link.
 *
 * The upstream API owns value, reference/iterator invalidation, and exception
 * semantics; see the [v3.12.0 documentation](https://github.com/nlohmann/json/tree/v3.12.0/docs/mkdocs/docs)
 * and [parse reference](https://github.com/nlohmann/json/blob/v3.12.0/docs/mkdocs/docs/api/basic_json/parse.md).
 * parse() constructs an owning, mutable JSON value; the input string need not
 * outlive it. Parsing errors throw by default. at() checks key existence, and
 * get<T>() performs upstream type conversion; failures are not translated here.
 *
 * @code{.cpp}
 * #include <m03gagbhsqfsqblhwvelrou7nc_json/api.h>
 *
 * #include <iostream>
 * #include <string>
 *
 * int main() {
 *     try {
 *         auto json = nlohmann::json::parse(R"({"name":"builder","jobs":2})");
 *         const auto name = json.at("name").get<std::string>();
 *         json["jobs"] = 4;
 *         std::cout << name << ": " << json.dump() << '\n';
 *         return 0;
 *     } catch (const nlohmann::json::exception& exception) {
 *         std::cerr << exception.what() << '\n';
 *         return 1;
 *     }
 * }
 * @endcode
 */

#endif // M03GAGBHSQFSQBLHWVELROU7NC_JSON_API_H
