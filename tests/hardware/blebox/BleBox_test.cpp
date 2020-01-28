#include <gmock/gmock.h>

#include "../../../hardware/BleBox.h"
#include "../../../hardware/blebox/errors.h"
#include "../../../hardware/blebox/session.h"

TEST(BleBox, Construction) {
  using namespace blebox;

  // TODO: ports not supported yet
  ASSERT_THROW(BleBox(123, "192.168.2.3::", 13, nullptr), errors::bad_ip);

  ASSERT_THROW(BleBox(123, "0.0.0.0", 13, nullptr), errors::bad_ip);

  ASSERT_THROW(BleBox(123, "1:192.168.2.3", 13, nullptr), errors::bad_mac);

  ASSERT_THROW(BleBox(123, "xxxxxx123456:192.168.2.3", 13, nullptr),
               errors::bad_mac);

  {
    BleBox bb(123, "abcdef123456:192.168.2.3", 13, nullptr);
    ASSERT_EQ(123, bb.hw_id());
    ASSERT_EQ(make_address_v4("192.168.2.3"), bb.session().ip());
  }

  {
    BleBox bb(123, "192.168.2.3", 13, nullptr);
    ASSERT_EQ(123, bb.hw_id());
    ASSERT_EQ(make_address_v4("192.168.2.3"), bb.session().ip());
  }

  {
    session session(make_address_v4("192.168.2.7"), 1, 2);
    BleBox bb(123, "192.168.2.3", 13, &session);
    ASSERT_EQ(123, bb.hw_id());
    ASSERT_EQ(make_address_v4("192.168.2.7"), bb.session().ip());
  }
}

TEST(BleBox, AlternativeConstruction) {
  using namespace blebox;

  ASSERT_THROW(BleBox bb(make_address_v4("0.0.0.0"), 123, 13, nullptr),
               errors::bad_ip);

  {
    BleBox bb(make_address_v4("192.168.2.3"), 123, 13, nullptr);
    ASSERT_EQ(123, bb.hw_id());
    ASSERT_EQ(13, bb.poll_interval());
    ASSERT_EQ(make_address_v4("192.168.2.3"), bb.session().ip());
  }

  {
    session session(make_address_v4("192.168.2.7"), 1, 2);
    BleBox bb(make_address_v4("192.168.2.3"), 123, 13, &session);
    ASSERT_EQ(123, bb.hw_id());
    ASSERT_EQ(make_address_v4("192.168.2.7"), bb.session().ip());
  }
}
