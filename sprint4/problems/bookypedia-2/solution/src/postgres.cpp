#include "postgres.h"

#include <pqxx/pqxx>
#include <pqxx/zview.hxx>
#include <string>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author &author) {
  pqxx::work work_{connection_};
  work_.exec_params(R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
                    author.GetId().ToString(), author.GetName());
  work_.commit();
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAllAuthors() {
  std::vector<domain::Author> authors;
  pqxx::read_transaction read_transaction_{connection_};
  auto query_text = "SELECT * FROM authors ORDER BY name ASC"_zv;
  for (auto [id, name] :
       read_transaction_.query<std::string, std::string>(query_text)) {
    authors.emplace_back(domain::AuthorId::FromString(id), name);
  }
  return authors;
}

std::optional<domain::Author>
AuthorRepositoryImpl::GetAuthorBy(const std::string &author_name) {
  pqxx::read_transaction read_transaction_{connection_};
  auto query_text = "SELECT * FROM authors WHERE name=" +
                    read_transaction_.quote(author_name);
  std::optional tmp_author =
      read_transaction_.query01<std::string, std::string>(query_text);
  if (tmp_author) {
    auto [id, name] = *tmp_author;
    return domain::Author(domain::AuthorId::FromString(id), name);
  };
  return std::nullopt;
};

std::optional<domain::Author>
AuthorRepositoryImpl::GetAuthorById(const std::string &id) {
  pqxx::read_transaction read_transaction_{connection_};
  auto query_text =
      "SELECT name FROM authors WHERE id=" + read_transaction_.quote(id);
  auto result = read_transaction_.query01<std::string>(query_text);
  if (result) {
    return domain::Author(domain::AuthorId::FromString(id),
                          std::get<0>(*result));
  }
  return std::nullopt;
}

void AuthorRepositoryImpl::Delete(const std::string &author_name) {
  pqxx::work work_{connection_};
  work_.exec_params("DELETE FROM authors WHERE name=$1", author_name);
  work_.commit();
}

void BookRepositoryImpl::Save(const domain::Book &book) {
  pqxx::work work_{connection_};
  std::string title = book.GetTitle();
  work_.exec_params(R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;
)"_zv,
                    book.GetId().ToString(), book.GetAuthorId().ToString(),
                    book.GetTitle(), book.GetPublicationYear());
  work_.commit();
}

std::vector<domain::Book> BookRepositoryImpl::GetAllBooks() {
  std::vector<domain::Book> books;
  pqxx::read_transaction read_transaction_{connection_};
  auto query_text =
      "SELECT id, author_id, title, publication_year FROM books ORDER BY title ASC"_zv;
  for (auto [id, author_id, title, year] :
       read_transaction_.query<std::string, std::string, std::string, int>(
           query_text)) {
    books.emplace_back(domain::BookId::FromString(id),
                       domain::AuthorId::FromString(author_id), title, year);
  }
  return books;
}

std::vector<domain::Book>
BookRepositoryImpl::GetBooksBy(const std::string &author_name) {
  std::vector<domain::Book> books;
  pqxx::read_transaction read_transaction_{connection_};
  auto query_text = "SELECT id, author_id, title, publication_year FROM books "
                    "WHERE author_id=(SELECT id FROM authors WHERE name=" +
                    read_transaction_.quote(author_name) +
                    " LIMIT 1) ORDER BY publication_year ASC, title ASC";
  for (auto [id, author_id, title, year] :
       read_transaction_.query<std::string, std::string, std::string, int>(
           query_text)) {
    books.emplace_back(domain::BookId::FromString(id),
                       domain::AuthorId::FromString(author_id), title, year);
  }
  return books;
}

std::optional<domain::Book>
BookRepositoryImpl::GetBookById(const std::string &id) {
  pqxx::read_transaction read_transaction_{connection_};
  auto query_text =
      "SELECT author_id, title, publication_year FROM books WHERE id=" +
      read_transaction_.quote(id);
  auto result =
      read_transaction_.query01<std::string, std::string, int>(query_text);
  if (result) {
    auto [author_id, title, year] = *result;
    return domain::Book(domain::BookId::FromString(id),
                        domain::AuthorId::FromString(author_id), title, year);
  }
  return std::nullopt;
}

std::vector<domain::Book>
BookRepositoryImpl::GetBooksByTitle(const std::string &title) {
  std::vector<domain::Book> books;
  pqxx::read_transaction read_transaction_{connection_};
  auto query_text =
      "SELECT id, author_id, publication_year FROM books WHERE title=" +
      read_transaction_.quote(title) + " ORDER BY title ASC";
  for (auto [id, author_id, year] :
       read_transaction_.query<std::string, std::string, int>(query_text)) {
    books.emplace_back(domain::BookId::FromString(id),
                       domain::AuthorId::FromString(author_id), title, year);
  }
  return books;
}

void BookRepositoryImpl::Delete(const std::string &book_id) {
  pqxx::work work_{connection_};
  work_.exec_params("DELETE FROM books WHERE id=$1", book_id);
  work_.commit();
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
  pqxx::work work_{connection_};
  work_.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);

  work_.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
    author_id UUID NOT NULL,
    title varchar(100) NOT NULL,
    publication_year int NOT NULL
);
)"_zv);

  work_.exec(R"(
CREATE TABLE IF NOT EXISTS book_tags (
    book_id UUID REFERENCES books(id) ON DELETE CASCADE,
    tag varchar(30) NOT NULL,
    PRIMARY KEY (book_id, tag)
);
)"_zv);

  // коммитим изменения
  work_.commit();
}

void TagRepositoryImpl::Save(const std::string &book_id,
                             const std::vector<domain::Tag> &tags) {
  if (tags.empty()) {
    return;
  }

  pqxx::work work_{connection_};
  for (const auto &tag : tags) {
    work_.exec_params(R"(
INSERT INTO book_tags (book_id, tag) VALUES ($1, $2)
ON CONFLICT DO NOTHING;
)"_zv,
                      book_id, tag.GetValue());
  }
  work_.commit();
}

void TagRepositoryImpl::Update(const std::string &book_id,
                               const std::vector<domain::Tag> &tags) {
  pqxx::work work_{connection_};
  // Удаляем все старые теги
  work_.exec_params("DELETE FROM book_tags WHERE book_id=$1"_zv, book_id);
  // Добавляем новые
  for (const auto &tag : tags) {
    work_.exec_params(R"(
INSERT INTO book_tags (book_id, tag) VALUES ($1, $2)
ON CONFLICT DO NOTHING;
)"_zv,
                      book_id, tag.GetValue());
  }
  work_.commit();
}

std::vector<domain::Tag>
TagRepositoryImpl::GetByBookId(const std::string &book_id) {
  std::vector<domain::Tag> tags;
  pqxx::read_transaction read_transaction_{connection_};
  auto result = read_transaction_.exec_params(
      "SELECT tag FROM book_tags WHERE book_id=$1 ORDER BY tag", book_id);
  for (const auto &row : result) {
    tags.emplace_back(row[0].as<std::string>());
  }
  return tags;
}

void TagRepositoryImpl::DeleteByBookId(const std::string &book_id) {
  pqxx::work work_{connection_};
  work_.exec_params("DELETE FROM book_tags WHERE book_id=$1"_zv, book_id);
  work_.commit();
}

std::vector<domain::Book>
BookRepositoryImpl::GetBooksByAuthorId(const std::string &author_id) {
  std::vector<domain::Book> books;
  pqxx::read_transaction read_transaction_{connection_};
  auto query_text =
      "SELECT id, title, publication_year FROM books WHERE author_id=" +
      read_transaction_.quote(author_id) +
      " ORDER BY publication_year ASC, title ASC";
  for (auto [id, title, year] :
       read_transaction_.query<std::string, std::string, int>(query_text)) {
    books.emplace_back(domain::BookId::FromString(id),
                       domain::AuthorId::FromString(author_id), title, year);
  }
  return books;
}

} // namespace postgres