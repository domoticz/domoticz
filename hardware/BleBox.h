#pragma once

// implememtation for devices : http://blebox.eu/
// by Fantom (szczukot@poczta.onet.pl)
// rewritten by gadgetmobile (the_gadget_mobile@yahoo.com)

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

#include "../main/RFXNames.h"

#include "../hardware/DomoticzHardware.h" // for base class
#include "blebox/ip.h"

namespace Json {
class Value;
}

namespace blebox {
class box;
class session;
namespace db {
bool migrate(blebox::session *session = 0);
}
} // namespace blebox

class BleBox : public CDomoticzHardwareBase {
public:
  // TODO: make configurable?
  const int DefaultFirmwareTimeout = 2;

  // TODO: each box should have a minimal poll time, so that this could be
  // even every second without worrying about recommendations
  static const int DefaultPollInterval = 30;

  // called by Domoticz
  BleBox(const int hw_id, const std::string &address, const int pollIntervalsec,
         blebox::session *session = nullptr);

  // called by search
  BleBox(::blebox::ip ip, const int hw_id, const int pollIntervalsec,
         blebox::session *session);

  ~BleBox() override;

  int hw_id() const {
    if (!m_HwdID)
      throw std::invalid_argument("incomplete BleBox hardware instance");

    return m_HwdID;
  }

  int poll_interval() const { return _poll_interval; };

  bool WriteToHardware(const char *pdata, const unsigned char length) final;

  // Called from web
  void UpgradeFirmware();

  void FetchAllFeatures(Json::Value &root);
  void AsyncRemoveFeature(int device_record_id);
  void AsyncRemoveAllFeatures();
  void DetectFeatures();
  void RefreshFeatures();
  void UseFeatures();

  static void SearchNetwork(const std::string &pattern);

  // public only for tests
  auto session() -> blebox::session &;

  // Defined only so it can be replaced in tests
  // NOTE: overridable, but probably not worth doing so (inconvenient to use)
  // NOTE: there's a special version of this is in stubs.cpp (for testing)
  // Public, but only used by blebox::widgets::widget
  virtual void InternalDecodeRX(const std::string &default_name,
                                const uint8_t *cmd, const size_t size);

  // Used by search, adding by user and db migration
  static bool AddOrUpdateHardware(const std::string &name, blebox::ip ip,
                                  bool silent, bool migration,
                                  blebox::session *session);

private:
  int _poll_interval;
  std::shared_ptr<std::thread> m_thread;
  std::unique_ptr<blebox::box> _box;
  std::mutex m_mutex;
  std::string _unique_id;
  ::blebox::ip _ip;
  ::blebox::session *_session{nullptr};
  ::blebox::session *_internal_session{nullptr};

  bool StartHardware() final;
  bool StopHardware() final;

  // helper method for thread management
  void Do_Work();

  void SetSettings(const int pollIntervalsec);

  void reset_box();
  auto box() -> blebox::box &;
};
