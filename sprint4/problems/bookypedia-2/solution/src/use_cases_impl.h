#pragma once
#include "author_fwd.h"
#include "book_fwd.h"
#include "tag_fwd.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
  // Конструктор без тегов (для обратной совместимости)
  explicit UseCasesImpl(domain::AuthorRepository &authors,
                        domain::BookRepository &books)
      : authors_{authors}, books_{books}, tags_(GetNullTagRepository()) {}

  // Основной конструктор с тегами
  explicit UseCasesImpl(domain::AuthorRepository &authors,
                        domain::BookRepository &books,
                        domain::TagRepository &tags)
      : authors_{authors}, books_{books}, tags_{tags} {}

  void AddAuthor(const std::string &name) override;
  std::vector<std::string> GetAllAuthors() override;
  std::optional<std::string>
  GetAuthorIdBy(const std::string &author_name) override;
  void AddBook(const std::string &author_id, const std::string &title,
               int year) override;
  std::vector<std::string> GetAllBooks() override;
  std::vector<std::string> GetBooksBy(const std::string &author_name) override;

  // Новые методы
  void DeleteAuthor(const std::string &author_name) override;
  void EditAuthor(const std::string &old_name,
                  const std::string &new_name) override;
  void DeleteBook(
      const std::string &title,
      const std::optional<std::string> &author_name = std::nullopt) override;
  void DeleteBookById(const std::string &book_id) override;
  void EditBook(const std::string &old_title, const std::string &author_name,
                const std::optional<std::string> &new_title,
                const std::optional<int> &new_year,
                const std::vector<std::string> &new_tags) override;
  std::vector<std::pair<std::string, std::string>>
  GetAllBooksWithAuthors() override;
  std::vector<std::pair<domain::Book, std::string>>
  GetBooksByTitle(const std::string &title) override;
  std::optional<std::pair<domain::Book, std::string>>
  GetBookById(const std::string &book_id) override;
  std::vector<std::string> GetBookTags(const std::string &book_id) override;
  void AddBookWithTags(const std::string &author_id, const std::string &title,
                       int year, const std::vector<std::string> &tags) override;

private:
  // Вспомогательный класс-заглушка для случаев без тегов
  class NullTagRepository : public domain::TagRepository {
  public:
    void Save(const std::string &, const std::vector<domain::Tag> &) override {}
    void Update(const std::string &,
                const std::vector<domain::Tag> &) override {}
    std::vector<domain::Tag> GetByBookId(const std::string &) override {
      return {};
    }
    void DeleteByBookId(const std::string &) override {}
  };

  static domain::TagRepository &GetNullTagRepository() {
    static NullTagRepository null_repo;
    return null_repo;
  }

  domain::AuthorRepository &authors_;
  domain::BookRepository &books_;
  domain::TagRepository &tags_;
};

} // namespace app