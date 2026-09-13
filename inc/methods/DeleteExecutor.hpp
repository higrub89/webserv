#ifndef DELETEEXECUTOR_HPP_
#define DELETEEXECUTOR_HPP_

#include "IMethodExecutor.hpp"

/**
 * @class DeleteExecutor
 * @brief Concrete implementation of IMethodExecutor for handling HTTP DELETE requests.
 *
 * Checks for file existence, validates permissions, and removes the specified file resource.
 */
class DeleteExecutor : public IMethodExecutor {
public:
  DeleteExecutor();
  virtual ~DeleteExecutor();

  virtual void handle(const HttpRequest& req, HttpResponse& res, ClientHandler* client, const LocationConfig& location);
};

#endif  // DELETEEXECUTOR_HPP_
