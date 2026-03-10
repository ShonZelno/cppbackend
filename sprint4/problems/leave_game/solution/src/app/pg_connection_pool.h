#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <queue>
#include <vector>

class PgConnectionPool {
public:
  explicit PgConnectionPool(const std::string &conn_str, size_t pool_size);
  std::unique_ptr<pqxx::connection> GetConnection();
  void ReturnConnection(std::unique_ptr<pqxx::connection> conn);

private:
  std::string conn_str_;
  std::queue<std::unique_ptr<pqxx::connection>> connections_;
  std::mutex mutex_;
  std::condition_variable cv_;
};