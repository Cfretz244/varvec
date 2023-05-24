#include <catch2/catch_test_macros.hpp>
#include "../varvec.hpp"

using trivial_vector = varvec::static_vector<32, 8, bool, int, float>;

static_assert(
  sizeof(trivial_vector) == 32 + (8 * 2) + 4
);

static_assert(
  std::is_trivially_destructible_v<trivial_vector>
);

using copyable_vector = varvec::static_vector<128, 4, bool, int, float, std::string>;

static_assert(
  !std::is_trivially_destructible_v<copyable_vector>
  &&
  std::copyable<copyable_vector>
  &&
  std::movable<copyable_vector>
);

using movable_vector = varvec::static_vector<128, 4, bool, int, float, std::string, std::unique_ptr<double>>;

static_assert(
  !std::is_trivially_destructible_v<movable_vector>
  &&
  !std::copyable<movable_vector>
  &&
  std::movable<movable_vector>
);

using dynamic_vector = varvec::vector<bool, int, float, std::string>;

static_assert(
  !std::is_trivially_destructible_v<dynamic_vector>
  &&
  std::copyable<dynamic_vector>
  &&
  std::movable<dynamic_vector>
);

using dynamic_movable_vector = varvec::vector<bool, int, float, std::string, std::unique_ptr<double>>;

static_assert(
  !std::is_trivially_destructible_v<dynamic_movable_vector>
  &&
  !std::copyable<dynamic_movable_vector>
  &&
  std::movable<dynamic_movable_vector>
);

TEST_CASE("construction properties", "[varvec tests]") {
  auto asserts = [] <class V> (varvec::meta::identity<V>) {
    V vec;
    REQUIRE(vec.size() == 0);
    REQUIRE(vec.empty());
    REQUIRE(vec.used_bytes() > 0);
    REQUIRE(vec.begin() == vec.end());

    if constexpr (std::copyable<V>) {
      auto copy = vec;
      REQUIRE(copy.size() == 0);
      REQUIRE(copy.empty());
      REQUIRE(copy.used_bytes() > 0);
      REQUIRE(copy.begin() == copy.end());
      REQUIRE(copy == vec);
    }
  };
  asserts(varvec::meta::identity<trivial_vector> {});
  asserts(varvec::meta::identity<copyable_vector> {});
  asserts(varvec::meta::identity<movable_vector> {});
  asserts(varvec::meta::identity<dynamic_vector> {});
  asserts(varvec::meta::identity<dynamic_movable_vector> {});
}

TEST_CASE("container properties", "[varvec tests]") {
  auto asserts = [] <class V> (varvec::meta::identity<V>) {
    using val = typename V::value_type;
    V vec;
    vec.push_back(true);
    vec.push_back(5);
    vec.push_back((float) 3.5);
    vec.push_back("hello world");

    auto validate = [] (auto& v) {
      auto it = v.begin();
      REQUIRE(v[0] == val {true});
      REQUIRE(*it++ == val {true});
      v.visit_at(0, varvec::meta::overload {
        [] (bool& val) { REQUIRE(val == true); },
        [] (auto&) { REQUIRE(false); }
      });
      REQUIRE(v[1] == val {5});
      REQUIRE(*it++ == val {5});
      v.visit_at(1, varvec::meta::overload {
        [] (int& val) { REQUIRE(val == 5); },
        [] (auto&) { REQUIRE(false); }
      });
      REQUIRE(v[2] == val {(float) 3.5});
      REQUIRE(*it++ == val {(float) 3.5});
      v.visit_at(2, varvec::meta::overload {
        [] (float& val) { REQUIRE(val == 3.5); },
        [] (auto&) { REQUIRE(false); }
      });
      REQUIRE(v[3] == val {"hello world"});
      REQUIRE(*it++ == val {"hello world"});
      v.visit_at(3, varvec::meta::overload {
        [] (std::string& val) { REQUIRE(val == "hello world"); },
        [] (auto&) { REQUIRE(false); }
      });
      REQUIRE(it == v.end());

      REQUIRE(v.begin() + 4 == v.end());
      REQUIRE(4 + v.begin() == v.end());
      REQUIRE(v.begin() == v.end() - 4);
      REQUIRE(v.begin() < v.end());
      REQUIRE(v.begin() <= v.end());
      REQUIRE(v.end() > v.begin());
      REQUIRE(v.end() >= v.begin());
    };

    validate(vec);
    if constexpr (std::copyable<V>) {
      auto copy = vec;
      validate(copy);
    }
  };
  asserts(varvec::meta::identity<copyable_vector> {});
  asserts(varvec::meta::identity<movable_vector> {});
  asserts(varvec::meta::identity<dynamic_vector> {});
  asserts(varvec::meta::identity<dynamic_movable_vector> {});
}

TEST_CASE("move-only properties", "varvec tests") {
  auto asserts = [] <class V> (varvec::meta::identity<V>) {
    using val = typename V::value_type;

    V vec;
    vec.push_back(true);
    vec.push_back(1337);
    vec.push_back("hello world");
    vec.push_back(std::make_unique<double>(3.14159));

    auto validate = [] (auto& v) {
      auto it = v.begin();
      REQUIRE(v[0] == val {true});
      REQUIRE(*it++ == val {true});
      REQUIRE(v[1] == val {1337});
      REQUIRE(*it++ == val {1337});
      REQUIRE(v[2] == val {"hello world"});
      REQUIRE(*it++ == val {"hello world"});

      std::visit(varvec::meta::overload {
        [] (std::unique_ptr<double>* doubleptr) { REQUIRE(**doubleptr == 3.14159); },
        [] (auto&&) { REQUIRE(false); }
      }, v[3]);
      std::visit(varvec::meta::overload {
        [] (std::unique_ptr<double>* doubleptr) { REQUIRE(**doubleptr == 3.14159); },
        [] (auto&&) { REQUIRE(false); }
      }, *it++);

      v.visit_at(3, varvec::meta::overload {
        [] (std::unique_ptr<double>& ptr) { REQUIRE(*ptr == 3.14159); },
        [] (auto&&) { REQUIRE(false); }
      });
      REQUIRE(it == v.end());
    };
    validate(vec);

    auto moved = std::move(vec);
    validate(moved);
  };
  asserts(varvec::meta::identity<movable_vector> {});
  asserts(varvec::meta::identity<dynamic_movable_vector> {});
}