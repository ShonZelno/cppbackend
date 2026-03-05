#pragma once
#include <compare>

namespace util {

/**
 * Âñïîìîãàòåëüíûé øàáëîííûé êëàññ "Ìàðêèðîâàííûé òèï".
 * Ñ åãî ïîìîùüþ ìîæíî îïèñàòü ñòðîãèé òèï íà îñíîâå äðóãîãî òèïà.
 * Ïðèìåð:
 *
 *  struct AddressTag{}; // ìåòêà òèïà äëÿ ñòðîêè, õðàíÿùåé àäðåñ
 *  using Address = util::Tagged<std::string, AddressTag>;
 *
 *  struct NameTag{}; // ìåòêà òèïà äëÿ ñòðîêè, õðàíÿùåé èìÿ
 *  using Name = util::Tagged<std::string, NameTag>;
 *
 *  struct Person {
 *      Name name;
 *      Address address;
 *  };
 *
 *  Name name{"Harry Potter"s};
 *  Address address{"4 Privet Drive, Little Whinging, Surrey, England"s};
 *
 * Person p1{name, address}; // OK
 * Person p2{address, name}; // Îøèáêà, Address è Name - ðàçíûå òèïû
 */
template <typename Value, typename Tag>
class Tagged {
public:
    using ValueType = Value;
    using TagType = Tag;

    explicit Tagged(Value&& v)
        : value_(std::move(v)) {
    }
    explicit Tagged(const Value& v)
        : value_(v) {
    }

    const Value& operator*() const {
        return value_;
    }

    Value& operator*() {
        return value_;
    }

    // Òàê â C++20 ìîæíî îáúÿâèòü îïåðàòîð ñðàâíåíèÿ Tagged-òèïîâ
    // Áóäåò ïðîñòî âûçâàí ñîîòâåòñòâóþùèé îïåðàòîð äëÿ ïîëÿ value_
    auto operator<=>(const Tagged<Value, Tag>&) const = default;

private:
    Value value_;
};

// Õåøåð äëÿ Tagged-òèïà, ÷òîáû Tagged-îáúåêòû ìîæíî áûëî õðàíèòü â unordered-êîíòåéíåðàõ
template <typename TaggedValue>
struct TaggedHasher {
    size_t operator()(const TaggedValue& value) const {
        // Âîçâðàùàåò õåø çíà÷åíèÿ, õðàíÿùåãîñÿ âíóòðè value
        return std::hash<typename TaggedValue::ValueType>{}(*value);
    }
};

}  // namespace util