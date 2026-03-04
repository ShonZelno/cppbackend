#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "author.h"
#include "book.h"
#include "tag.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
  explicit AuthorRepositoryImpl(pqxx::connection &connection)
      : connection_(connection) {}

  void Save(const domain::Author &author) override;
  std::vector<domain::Author> GetAllAuthors() override;
  std::optional<domain::Author>
  GetAuthorBy(const std::string &author_name) override;
  std::optional<domain::Author> GetAuthorById(const std::string &id) override;
  void Delete(const std::string &author_name) override;

private:
  pqxx::connection &connection_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
  explicit BookRepositoryImpl(pqxx::connection &connection)
      : connection_(connection) {}

  void Save(const domain::Book &book) override;
  std::vector<domain::Book> GetAllBooks() override;
  std::vector<domain::Book> GetBooksBy(const std::string &author_name) override;
  std::optional<domain::Book> GetBookById(const std::string &id) override;
  std::vector<domain::Book> GetBooksByTitle(const std::string &title) override;
  void Delete(const std::string &book_id) override;
  std::vector<domain::Book>
  GetBooksByAuthorId(const std::string &author_id) override;

private:
  pqxx::connection &connection_;
};

class TagRepositoryImpl : public domain::TagRepository {
public:
  explicit TagRepositoryImpl(pqxx::connection &connection)
      : connection_(connection) {}

  void Save(const std::string &book_id,
            const std::vector<domain::Tag> &tags) override;
  void Update(const std::string &book_id,
              const std::vector<domain::Tag> &tags) override;
  std::vector<domain::Tag> GetByBookId(const std::string &book_id) override;
  void DeleteByBookId(const std::string &book_id) override;

private:
  pqxx::connection &connection_;
};

class Database {
public:
  explicit Database(pqxx::connection connection);

  AuthorRepositoryImpl &GetAuthors() & { return authors_; }
  BookRepositoryImpl &GetBooks() & { return books_; }
  TagRepositoryImpl &GetTags() & { return tags_; }

private:
  pqxx::connection connection_;
  AuthorRepositoryImpl authors_{connection_};
  BookRepositoryImpl books_{connection_};
  TagRepositoryImpl tags_{connection_};
};

} // namespace postgres