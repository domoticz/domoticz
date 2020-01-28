#include <gmock/gmock.h>

#include "../../../hardware/blebox/version.h"

TEST(Version, Compatibility) {
  using namespace blebox;

  // ok - same
  EXPECT_TRUE(version("0.0.0").compatible_with(version("0.0.0")));
  EXPECT_TRUE(version("0.0.1").compatible_with(version("0.0.1")));
  EXPECT_TRUE(version("0.1.0").compatible_with(version("0.1.0")));
  EXPECT_TRUE(version("0.1.1").compatible_with(version("0.1.1")));
  EXPECT_TRUE(version("1.0.0").compatible_with(version("1.0.0")));
  EXPECT_TRUE(version("1.0.1").compatible_with(version("1.0.1")));
  EXPECT_TRUE(version("1.1.0").compatible_with(version("1.1.0")));
  EXPECT_TRUE(version("1.1.1").compatible_with(version("1.1.1")));

  // ok - patch level is more
  EXPECT_TRUE(version("0.0.1").compatible_with(version("0.0.0")));

  // ok - minor is more
  EXPECT_TRUE(version("0.1.0").compatible_with(version("0.0.0")));
  EXPECT_TRUE(version("0.1.0").compatible_with(version("0.0.1")));
  EXPECT_TRUE(version("0.1.1").compatible_with(version("0.0.0")));
  EXPECT_TRUE(version("0.1.1").compatible_with(version("0.0.1")));

  // patch not enough
  EXPECT_FALSE(version("0.0.0").compatible_with(version("0.0.1")));
  EXPECT_FALSE(version("0.1.0").compatible_with(version("0.1.1")));
  EXPECT_FALSE(version("1.0.0").compatible_with(version("1.0.1")));
  EXPECT_FALSE(version("1.1.0").compatible_with(version("1.1.1")));

  // minor not enough
  EXPECT_FALSE(version("0.0.0").compatible_with(version("0.1.0")));
  EXPECT_FALSE(version("0.0.1").compatible_with(version("0.1.0")));
  EXPECT_FALSE(version("1.0.0").compatible_with(version("1.1.0")));
  EXPECT_FALSE(version("1.0.1").compatible_with(version("1.1.0")));

  // major not matching (otherwise ok)
  EXPECT_FALSE(version("0.0.0").compatible_with(version("1.0.0")));
  EXPECT_FALSE(version("0.0.1").compatible_with(version("1.0.0")));
  EXPECT_FALSE(version("0.1.1").compatible_with(version("1.0.0")));

  EXPECT_FALSE(version("1.0.0").compatible_with(version("0.0.0")));
  EXPECT_FALSE(version("1.0.0").compatible_with(version("0.0.1")));
  EXPECT_FALSE(version("1.0.0").compatible_with(version("0.1.1")));
}
