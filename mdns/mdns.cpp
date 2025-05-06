#include "mdns.hpp"

#include "../main/Logger.h"
#include "../main/Helper.h"

#include "include/mdns.h"

namespace domoticz_mdns {
    mDNS::~mDNS() { stopService(); }

    void mDNS::startService(const bool dumpMode) {
      dumpMode_ = dumpMode;
      if (running_) {
        stopService();
      }

      running_ = true;
      mdns_worker_thread_ = std::thread([this]() { this->runMainLoop(); });
      _log.Log(LOG_STATUS, "mDNS service started");
    }

    void mDNS::stopService() {
        running_ = false;
        if (mdns_worker_thread_.joinable()) {
          mdns_worker_thread_.join();
        }
    }
    
    bool mDNS::isServiceRunning() { return running_; } 
      
    void mDNS::setServiceHostname(const std::string &hostname) { hostname_ = hostname; }

    void mDNS::setServicePort(std::uint16_t port) { port_ = port; }

    void mDNS::setServiceName(const std::string &name) { name_ = name; }

    void mDNS::setServiceTxtRecord(const std::string &txt_record) { txt_record_ = txt_record; }

}
