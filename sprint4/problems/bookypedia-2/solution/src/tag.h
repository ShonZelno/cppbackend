#pragma once
#include "tagged.h"
#include <string>
#include <vector>

namespace domain {

namespace detail {
struct TagTag {};
} // namespace detail

using TagValue = util::Tagged<std::string, detail::TagTag>;

class Tag {
public:
  explicit Tag(std::string value) : value_(normalize(std::move(value))) {}

  const std::string &GetValue() const noexcept { return *value_; }

  static std::string normalize(std::string tag) {
    // Удаляем пробелы в начале и конце
    size_t start = tag.find_first_not_of(" \t");
    if (start == std::string::npos) {
      return "";
    }
    size_t end = tag.find_last_not_of(" \t");
    tag = tag.substr(start, end - start + 1);

    // Заменяем множественные пробелы на один
    std::string result;
    bool prev_space = false;
    for (char c : tag) {
      if (c == ' ' || c == '\t') {
        if (!prev_space) {
          result += ' ';
          prev_space = true;
        }
      } else {
        result += c;
        prev_space = false;
      }
    }
    return result;
  }

private:
  TagValue value_;
};

class TagRepository {
public:
  virtual void Save(const std::string &book_id,
                    const std::vector<Tag> &tags) = 0;
  virtual void Update(const std::string &book_id,
                      const std::vector<Tag> &tags) = 0;
  virtual std::vector<Tag> GetByBookId(const std::string &book_id) = 0;
  virtual void DeleteByBookId(const std::string &book_id) = 0;

protected:
  ~TagRepository() = default;
};

} // namespace domain