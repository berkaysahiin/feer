#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <feer/result.hpp>

#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace {

struct MoveOnly {
    explicit MoveOnly(int in_payload) : payload(in_payload) {}
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) = default;
    MoveOnly& operator=(MoveOnly&&) = default;

    int payload;
};

feer::Result<int> always_ok() {
    return 123;
}

feer::Result<int> always_err() {
    return feer::Err{"nope"};
}

}  // namespace

using namespace feer;

TEST_CASE("Result<T> supports value and error states") {
    SUBCASE("ok state stores and returns value") {
        Result<int> result = 42;

        CHECK(result.is_ok());
        CHECK_FALSE(result.is_err());
        CHECK(static_cast<bool>(result));
        CHECK(result.value() == 42);
    }

    SUBCASE("err state stores and returns error") {
        Result<int> result = Err("boom");

        CHECK(result.is_err());
        CHECK_FALSE(result.is_ok());
        CHECK_FALSE(static_cast<bool>(result));
        CHECK(result.error().message == "boom");
    }

    SUBCASE("accessing wrong alternative throws") {
        Result<int> ok_result = 7;
        Result<int> err_result = Err{"bad"};

        CHECK_THROWS_AS(static_cast<void>(ok_result.error()), std::bad_variant_access);
        CHECK_THROWS_AS(static_cast<void>(err_result.value()), std::bad_variant_access);
    }
}

TEST_CASE("Result<T> works in if initializer style") {
    int ok_path = 0;
    if (auto result = always_ok()) {
        ok_path = result.value();
    }
    CHECK(ok_path == 123);

    int err_path = 0;
    if (auto result = always_err()) {
        err_path = 1;
    }
    CHECK(err_path == 0);
}

TEST_CASE("Result<T> value_or returns value or fallback") {
    Result<int> ok_result = 42;
    Result<int> err_result = Err{"fallback-needed"};

    CHECK(ok_result.value_or(7) == 42);
    CHECK(err_result.value_or(7) == 7);

    Result<std::string> ok_string = std::string{"feer"};
    Result<std::string> err_string = Err{"no-string"};
    CHECK(ok_string.value_or("default") == "feer");
    CHECK(err_string.value_or("default") == "default");
}

TEST_CASE("Result<T> match selects ok branch") {
    Result<int> result = 21;

    int out = result.match(
        [](int value) {
            return value * 2;
        },
        [](const Err&) {
            return -1;
        });

    CHECK(out == 42);
}

TEST_CASE("Result<T> match selects err branch") {
    Result<int> result = Err{"match-failed"};

    int out = result.match(
        [](int) {
            return 0;
        },
        [](const Err& err) {
            return err.message == "match-failed" ? -1 : -2;
        });

    CHECK(out == -1);
}

TEST_CASE("Result<T> match supports rvalue overload") {
    Result<std::string> result = std::string{"feer"};

	auto ok = [](std::string&& val) {
		return val.size();
	};

	auto err = [](Err&& err) {
		return err.message.size();
	};

    std::size_t out = std::move(result).match(ok, err);

    CHECK(out == 4);
}

TEST_CASE("Result<T> supports move-only payloads") {
    Result<MoveOnly> result = MoveOnly{99};

    CHECK(result.is_ok());
    CHECK(result.value().payload == 99);
}

TEST_CASE("Result<T> provides correct value() reference categories") {
    static_assert(std::is_same_v<decltype(std::declval<Result<int>&>().value()), int&>);
    static_assert(std::is_same_v<decltype(std::declval<const Result<int>&>().value()), const int&>);
    static_assert(std::is_same_v<decltype(std::declval<Result<int>&&>().value()), int&&>);

    Result<std::string> result = std::string{"feer"};
    std::string moved = std::move(result).value();
    CHECK(moved == "feer");
}

TEST_CASE("Result<T> constructor constraints are intentional") {
    static_assert(std::is_constructible_v<Result<int>, int>);
    static_assert(std::is_constructible_v<Result<int>, const int&>);
    static_assert(std::is_constructible_v<Result<int&>, int&>);
    static_assert(!std::is_constructible_v<Result<int&>, int&&>);
    static_assert(std::is_constructible_v<Result<const int&>, int&>);
    static_assert(std::is_constructible_v<Result<const int&>, const int&>);
}

TEST_CASE("Result<void> supports ok and err states") {
    SUBCASE("default construction is ok") {
        Result<void> result;
        CHECK(result.is_ok());
        CHECK_FALSE(result.is_err());
        CHECK(static_cast<bool>(result));
    }

    SUBCASE("error construction is err") {
        Result<void> result = Err{"failed"};
        CHECK(result.is_err());
        CHECK_FALSE(result.is_ok());
        CHECK_FALSE(static_cast<bool>(result));
        CHECK(result.error().message == "failed");
    }

    SUBCASE("accessing error on ok throws") {
        Result<void> result;
        CHECK_THROWS_AS(static_cast<void>(result.error()), std::bad_variant_access);
    }
}

TEST_CASE("Result<void> match supports both branches") {
    Result<void> ok_result;
    Result<void> err_result = Err{"void-failed"};

    int ok_out = ok_result.match(
        [] {
            return 1;
        },
        [](const Err&) {
            return 0;
        });

    int err_out = err_result.match(
        [] {
            return 1;
        },
        [](const Err& err) {
            return err.message == "void-failed" ? 2 : 0;
        });

    CHECK(ok_out == 1);
    CHECK(err_out == 2);
}

TEST_CASE("Result<T&> aliases mutable value") {
    int source = 7;
    Result<int&> result = source;

    CHECK(result.is_ok());
    CHECK(result.value() == 7);

    result.value() = 11;
    CHECK(source == 11);
    CHECK(&result.value() == &source);
}

TEST_CASE("const Result<T&> still aliases mutable value") {
    int source = 3;
    const Result<int&> result = source;

    result.value() = 9;
    CHECK(source == 9);
}

TEST_CASE("Result<const T&> aliases const value") {
    const std::string source = "feer";
    Result<const std::string&> result = source;

    CHECK(result.is_ok());
    CHECK(result.value() == "feer");
    CHECK(&result.value() == &source);

    static_assert(std::is_same_v<decltype(std::declval<Result<const std::string&>&>().value()), const std::string&>);
}

TEST_CASE("reference results can also hold error state") {
    Result<int&> mutable_ref_error = Err{"mutable-ref-failed"};
    Result<const int&> const_ref_error = Err{"const-ref-failed"};

    CHECK(mutable_ref_error.is_err());
    CHECK(const_ref_error.is_err());
    CHECK(mutable_ref_error.error().message == "mutable-ref-failed");
    CHECK(const_ref_error.error().message == "const-ref-failed");

    CHECK_THROWS_AS(static_cast<void>(mutable_ref_error.value()), std::bad_variant_access);
    CHECK_THROWS_AS(static_cast<void>(const_ref_error.value()), std::bad_variant_access);
}

TEST_CASE("Err state preserves explicit source location") {
    const auto call_site = std::source_location::current();
    const Err err{"explicit", call_site};
    Result<int> result = err;

    CHECK(result.is_err());
    CHECK(result.error().message == "explicit");
    CHECK(result.error().where.line() == call_site.line());
}

TEST_CASE("Err captures source_location") {
    SUBCASE("default where points to construction site") {
        const auto before = std::source_location::current().line();
        const Err err{"location-check"};

        CHECK(err.message == "location-check");
        CHECK(err.where.line() >= before);
        CHECK(err.where.line() <= before + 10);
        CHECK(std::string{err.where.file_name()}.empty() == false);
    }

    SUBCASE("explicit where is preserved") {
        const auto call_site = std::source_location::current();
        const Err err{"explicit-location", call_site};

        CHECK(err.message == "explicit-location");
        CHECK(err.where.line() == call_site.line());
        CHECK(std::string{err.where.file_name()} == call_site.file_name());
    }
}
