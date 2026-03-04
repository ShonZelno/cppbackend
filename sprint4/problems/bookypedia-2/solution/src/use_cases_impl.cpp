#include "author.h"
#include "book.h"
#include "tag.h"
#include "use_cases_impl.h"

#include <algorithm>
#include <optional>
#include <ranges>
#include <set>
#include <sstream>

namespace app {
using namespace domain;

std::vector<std::string> parseTags(const std::string &tags_input) {
  std::vector<std::string> result;
  std::stringstream ss(tags_input);
  std::string tag;
  std::set<std::string> unique_tags;

  while (std::getline(ss, tag, ',')) {
    std::string normalized = domain::Tag::normalize(tag);
    if (!normalized.empty() && normalized.length() <= 30) {
      unique_tags.insert(std::move(normalized));
    }
  }

  result.assign(unique_tags.begin(), unique_tags.end());
  return result;
}

void UseCasesImpl::AddAuthor(const std::string &name) {
  authors_.Save({AuthorId::New(), name});
}

std::vector<std::string> UseCasesImpl::GetAllAuthors() {
  std::vector<std::string> list_of_authors;
  std::ranges::transform(
      authors_.GetAllAuthors(), std::back_inserter(list_of_authors),
      [](auto &author) -> std::string { return author.GetName(); });
  return list_of_authors;
}

std::optional<std::string>
UseCasesImpl::GetAuthorIdBy(const std::string &author_name) {
  auto author = authors_.GetAuthorBy(author_name);
  if (author) {
    return author.value().GetId().ToString();
  }
  return std::nullopt;
};

void UseCasesImpl::AddBook(const std::string &author_id,
                           const std::string &title, int year) {
  books_.Save({BookId::New(), AuthorId::FromString(author_id), title, year});
};

std::vector<std::string> UseCasesImpl::GetAllBooks() {
  std::vector<std::string> list_of_books;
  std::ranges::transform(
      books_.GetAllBooks(), std::back_inserter(list_of_books),
      [](auto &book) -> std::string {
        std::stringstream ss;
        ss << book.GetTitle() << ", " << book.GetPublicationYear();
        return ss.str();
      });
  return list_of_books;
};

std::vector<std::string>
UseCasesImpl::GetBooksBy(const std::string &author_name) {
  // Сначала получаем автора по имени
  auto author = authors_.GetAuthorBy(author_name);
  if (!author) {
    return {}; // Автор не найден - пустой список
  }

  // Получаем все книги
  auto all_books = books_.GetAllBooks();
  std::vector<std::string> list_of_books;

  // Фильтруем книги по ID автора
  for (const auto &book : all_books) {
    if (book.GetAuthorId().ToString() == author->GetId().ToString()) {
      std::stringstream ss;
      ss << book.GetTitle() << ", " << book.GetPublicationYear();
      list_of_books.push_back(ss.str());
    }
  }

  // Сортируем по году и названию
  std::sort(list_of_books.begin(), list_of_books.end());

  return list_of_books;
}

void UseCasesImpl::DeleteAuthor(const std::string &author_name) {
  auto author = authors_.GetAuthorBy(author_name);
  if (!author) {
    throw std::runtime_error("Author not found");
  }

  // Получаем все книги автора
  auto books = books_.GetBooksBy(author_name);

  // Удаляем теги для каждой книги
  for (const auto &book : books) {
    tags_.DeleteByBookId(book.GetId().ToString());
  }

  for (const auto &book : books) {
    books_.Delete(book.GetId().ToString());
  }

  authors_.Delete(author_name);
}

void UseCasesImpl::EditAuthor(const std::string &old_name,
                              const std::string &new_name) {
  auto author = authors_.GetAuthorBy(old_name);
  if (!author) {
    throw std::runtime_error("Author not found");
  }

  // Проверяем, не существует ли уже автор с таким именем
  auto existing_author = authors_.GetAuthorBy(new_name);
  if (existing_author && existing_author->GetId() != author->GetId()) {
    throw std::runtime_error("Author name already exists");
  }

  // Просто обновляем имя, ID остается тем же
  domain::Author updated_author(author->GetId(), new_name);
  authors_.Save(updated_author);

  // Книги остаются привязанными к тому же author_id
}

void UseCasesImpl::DeleteBook(const std::string &title,
                              const std::optional<std::string> &author_name) {
  std::vector<domain::Book> books_to_delete;

  if (author_name) {
    books_to_delete = books_.GetBooksBy(*author_name);
  } else {
    books_to_delete = books_.GetBooksByTitle(title);
  }

  if (books_to_delete.empty()) {
    throw std::runtime_error("Book not found");
  }

  for (const auto &book : books_to_delete) {
    if (!author_name || book.GetTitle() == title) {
      tags_.DeleteByBookId(book.GetId().ToString());
      books_.Delete(book.GetId().ToString());
    }
  }
}

void UseCasesImpl::DeleteBookById(const std::string &book_id) {
  tags_.DeleteByBookId(book_id);
  books_.Delete(book_id);
}

void UseCasesImpl::EditBook(const std::string &old_title,
                            const std::string &author_name,
                            const std::optional<std::string> &new_title,
                            const std::optional<int> &new_year,
                            const std::vector<std::string> &new_tags) {
  auto books = books_.GetBooksBy(author_name);
  domain::Book *book_to_edit = nullptr;

  for (auto &book : books) {
    if (book.GetTitle() == old_title) {
      book_to_edit = &book;
      break;
    }
  }

  if (!book_to_edit) {
    throw std::runtime_error("Book not found");
  }

  domain::Book updated_book(
      book_to_edit->GetId(), book_to_edit->GetAuthorId(),
      new_title.value_or(book_to_edit->GetTitle()),
      new_year.value_or(book_to_edit->GetPublicationYear()));

  books_.Save(updated_book);

  // Обновляем теги
  if (!new_tags.empty()) {
    std::vector<domain::Tag> tags;
    for (const auto &tag_value : new_tags) {
      tags.emplace_back(tag_value);
    }
    tags_.Update(updated_book.GetId().ToString(), tags);
  }
  // Если новые теги не предоставлены, оставляем старые (ничего не делаем)
  // Старые теги уже есть в базе и не меняются
}

std::vector<std::pair<std::string, std::string>>
UseCasesImpl::GetAllBooksWithAuthors() {
  std::vector<std::pair<std::string, std::string>> result;
  auto books = books_.GetAllBooks();

  for (const auto &book : books) {
    auto author = authors_.GetAuthorById(book.GetAuthorId().ToString());
    if (author) {
      std::stringstream ss;
      ss << book.GetTitle() << " by " << author->GetName() << ", "
         << book.GetPublicationYear();
      result.emplace_back(book.GetId().ToString(), ss.str());
    }
  }

  std::sort(result.begin(), result.end(),
            [](const auto &a, const auto &b) { return a.second < b.second; });

  return result;
}

std::vector<std::pair<domain::Book, std::string>>
UseCasesImpl::GetBooksByTitle(const std::string &title) {
  std::vector<std::pair<domain::Book, std::string>> result;
  auto books = books_.GetBooksByTitle(title);

  for (const auto &book : books) {
    auto author = authors_.GetAuthorById(book.GetAuthorId().ToString());
    if (author) {
      result.emplace_back(book, author->GetName());
    }
  }

  return result;
}

std::optional<std::pair<domain::Book, std::string>>
UseCasesImpl::GetBookById(const std::string &book_id) {
  auto book = books_.GetBookById(book_id);
  if (!book) {
    return std::nullopt;
  }

  auto author = authors_.GetAuthorById(book->GetAuthorId().ToString());
  if (!author) {
    return std::nullopt;
  }

  return std::make_pair(*book, author->GetName());
}

std::vector<std::string> UseCasesImpl::GetBookTags(const std::string &book_id) {
  auto tags = tags_.GetByBookId(book_id);
  std::vector<std::string> result;
  for (const auto &tag : tags) {
    result.push_back(tag.GetValue());
  }
  return result;
}

void UseCasesImpl::AddBookWithTags(const std::string &author_id,
                                   const std::string &title, int year,
                                   const std::vector<std::string> &tag_values) {
  domain::Book book(domain::BookId::New(),
                    domain::AuthorId::FromString(author_id), title, year);
  books_.Save(book);

  if (!tag_values.empty()) {
    std::vector<domain::Tag> tags;
    for (const auto &t : tag_values) {
      tags.emplace_back(t);
    }
    tags_.Save(book.GetId().ToString(), tags);
  }
}

} // namespace app