#ifndef HTTPERROR_HPP_
#define HTTPERROR_HPP_

#include "ConfigStructures.hpp"
#include "HttpResponse.hpp"

/**
 * @namespace HttpError
 * @brief Utility functions for populating standardized HTTP error responses.
 */
namespace HttpError {

/**
 * @brief Populates res with custom error_page from server if configured and readable,
 * or standard default HTML error page.
 * @param res The HttpResponse to populate.
 * @param errorCode The HTTP error status code (e.g. 404, 500).
 * @param server The ServerConfig holding the error_pages map.
 */
void populate(HttpResponse& res, int errorCode, const ServerConfig& server);

/**
 * @brief Fallback when no ServerConfig is available (e.g. early 400 Bad Request before server resolution).
 * @param res The HttpResponse to populate.
 * @param errorCode The HTTP error status code.
 */
void populateDefault(HttpResponse& res, int errorCode);

}  // namespace HttpError

#endif  // HTTPERROR_HPP_
