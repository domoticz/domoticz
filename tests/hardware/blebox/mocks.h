#pragma once

#include <gmock/gmock.h>

#include "../../../json/value.h"

#include "../../test_helper.h"

#include "../../../hardware/BleBox.h"
#include "../../../hardware/blebox/box.h"
#include "../../../hardware/blebox/ip.h"
#include "../../../hardware/blebox/session.h"

#include "../../memdb.h"
#include "../../worker_helper.h"

#define EXPECT_POST(path, form, response)                                      \
  EXPECT_CALL(_session, post_raw((path), (form)))                              \
      .WillOnce(::testing::Return((response)))

class GMockSession : public blebox::session {
public:
  GMockSession(const blebox::ip &ip, const int socket_timeout,
               const int read_timeout)
      : blebox::session(ip, socket_timeout, read_timeout) {}

  MOCK_METHOD(std::string, fetch_raw, (const std::string &), (const));
  MOCK_METHOD(std::string, post_raw, (const std::string &, const std::string &),
              (const));
};

class IdentifiedBox : public ::testing::Test {
protected:
  IdentifiedBox();
  void SetUp() override;

  virtual std::string default_status() const = 0;

  typedef ::testing::StrictMock<GMockSession> session_type;
  session_type _session;

  std::unique_ptr<blebox::box> box;
};

class WorkerScenario : public ::testing::Test {
protected:
  WorkerScenario();
  void SetUp() override;
  void TearDown() override;

  int idx(std::string node_id, int unit = 0xFF) const;

  typedef ::testing::StrictMock<GMockSession> session_type;
  session_type _session;

  worker_helper worker;
  std::unique_ptr<BleBox> bb;
  std::set<test_helper::device_status> _features;

  void add_identify_expectations(session_type &session);

  // override this always (worker scenarios need an identifiable device anyway)
  virtual std::string default_status() const = 0;

  // override this with empty if device returns state in identify() step
  // otherwise, override it with the tested state response
  virtual void add_state_fetch_expectations(session_type &session) = 0;

private:
  MemDBSession mem_session;
};
