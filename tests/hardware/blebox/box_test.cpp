#include <gmock/gmock.h>

#include "../../test_helper.h"

#include "../../../hardware/blebox/box.h"

TEST(Box, JsonPaths) {
  using namespace std::literals;
  using box = blebox::box;
  using namespace test_helper;

  ASSERT_EQ("foo"s, box::follow(json_parse(R"(["foo"])"), "[0]").asString());
  ASSERT_EQ("4"s, box::follow(json_parse(R"([{"foo":"3", "value":4}])"),
                              "[foo='3']/value")
                      .asString());
  ASSERT_EQ("4"s, box::follow(json_parse(R"([{"foo":3, "value":4}])"),
                              "[foo=3]/value")
                      .asString());
}

template <typename T>
static std::string prepare_form(const std::string &path, const T &value) {
  Json::Value form;
  blebox::box::set_form_field(form, path, value);
  return test_helper::compact_json(form);
}

TEST(Box, FollowAndSet) {
  using namespace test_helper;
  using box = blebox::box;

  ASSERT_EQ(rejson(R"({"foo":123})"), prepare_form("foo", 123));
  ASSERT_EQ(rejson(R"({"foo":"a"})"), prepare_form("foo", "a"));
  ASSERT_EQ(rejson(R"({"foo":{"bar":7}})"), prepare_form("foo/bar", 7));

  ASSERT_EQ(rejson(R"({"foo":{"bar":{"baz":"abc"}}})"),
            prepare_form("foo/bar/baz", "abc"));

  Json::Value form;
  box::set_form_field(form, "foo/bar/baz", "abc");
  box::set_form_field(form, "foo/bar/bax", 123);
  ASSERT_EQ(rejson(R"({"foo":{"bar":{"baz":"abc","bax":123}}})"),
            compact_json(form));

  // TODO: ("foo/[1]" = 5) isn't defined (prepare empty array? [0,5] ? error?)
  ASSERT_EQ(rejson(R"({"foo":{"bar":[9]}})"), prepare_form("foo/bar/[0]", 9));
}

TEST(Box, FollowAndSetMemberByMemberValue) {
  using namespace test_helper;

  ASSERT_EQ(rejson(R"({"foo":[{"bar":3, "baz":9}]})"),
            prepare_form("foo/[bar=3]/baz", 9));
}

TEST(Box, FollowAndAddEntryWithMemberValue) {
  using namespace test_helper;
  using box = blebox::box;

  Json::Value form;
  box::set_form_field(form, "foo/[bar=0]/baz", "abc");
  box::set_form_field(form, "foo/[bar=1]/bax", 123);

  const char *expected = R"({
    "foo":[
      {"bar": 0, "baz":"abc"},
      {"bar": 1, "bax": 123}
    ]
  })";
  ASSERT_EQ(rejson(expected), compact_json(form));
}

TEST(Box, FollowAndUpdateEntryWithMemberValue) {
  using namespace test_helper;
  using box = blebox::box;

  Json::Value form;
  box::set_form_field(form, "foo/[bar=0]/baz", "abc");
  box::set_form_field(form, "foo/[bar=0]/bax", 123);

  const char *expected = R"({
    "foo":[
      {"bar": 0, "baz":"abc", "bax": 123}
    ]
  })";
  ASSERT_EQ(rejson(expected), compact_json(form));
}

TEST(Box, FollowAndAppendEntryWithMemberValue) {
  using namespace test_helper;
  using box = blebox::box;

  Json::Value array(Json::arrayValue);
  Json::Value form;
  form["foo"] = array;
  box::set_form_field(form, "foo/[bar=0]/baz", "abc");

  const char *expected = R"({
    "foo":[
      {"bar": 0, "baz":"abc"}
    ]
  })";
  ASSERT_EQ(rejson(expected), compact_json(form));
}
