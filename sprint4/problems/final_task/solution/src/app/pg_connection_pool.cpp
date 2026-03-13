#include "pg_connection_pool.h"

#include <stdexcept>

PgConnectionPool::PgConnectionPool(const std::string &conn_str,
                                   size_t pool_size)
    : conn_str_(conn_str) {
  for (size_t i = 0; i < pool_size; ++i) {
    auto conn = std::make_unique<pqxx::connection>(conn_str_);
    if (!conn->is_open()) {
      throw std::runtime_error("Failed to open PostgreSQL connection");
    }
    connections_.push(std::move(conn));
  }
}

std::unique_ptr<pqxx::connection> PgConnectionPool::GetConnection() {
  std::unique_lock lock(mutex_);
  cv_.wait(lock, [this] { return !connections_.empty(); });
  auto conn = std::move(connections_.front());
  connections_.pop();
  return conn;
}

void PgConnectionPool::ReturnConnection(
    std::unique_ptr<pqxx::connection> conn) {
  {
    std::lock_guard lock(mutex_);
    connections_.push(std::move(conn));
  }
  cv_.notify_one();
}