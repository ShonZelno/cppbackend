#pragma once
#include <string>
#include <vector>

#include "author.h"
#include "tag.h"
#include "tagged_uuid.h"

namespace domain {

namespace detail {
struct BookTag {};
} // namespace detail

using BookId = util::TaggedUUID<detail::BookTag>;

class Book {
public:
  Book(BookId id, AuthorId author_id, std::string title, int year,
       std::vector<Tag> tags = {})
      : id_(std::move(id)), author_id_(std::move(author_id)),
        title_(std::move(title)), publication_year_(year),
        tags_(std::move(tags)) {}

  const BookId &GetId() const noexcept { return id_; }

  const AuthorId &GetAuthorId() const noexcept { return author_id_; }

  const std::string &GetTitle() const noexcept { return title_; }

  int GetPublicationYear() const noexcept { return publication_year_; }

  const std::vector<Tag> &GetTags() const noexcept { return tags_; }

  void SetTags(std::vector<Tag> tags) { tags_ = std::move(tags); }

private:
  BookId id_;
  AuthorId author_id_;
  std::string title_;
  int publication_year_;
  std::vector<Tag> tags_;
};

class BookRepository {
public:
  virtual void Save(const Book &book) = 0;
  virtual std::vector<Book> GetAllBooks() = 0;
  virtual std::vector<Book> GetBooksBy(const std::string &author_name) = 0;
  virtual std::optional<Book> GetBookById(const std::string &id) = 0;
  virtual std::vector<Book>
  GetBooksByAuthorId(const std::string &author_id) = 0;
  virtual std::vector<Book> GetBooksByTitle(const std::string &title) = 0;
  virtual void Delete(const std::string &book_id) = 0;

protected:
  ~BookRepository() = default;
};

} // namespace domain
