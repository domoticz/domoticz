#pragma once
#include "ASyncTCP.h"
#include <algorithm>
#include <atomic>
#include <limits>
#include <string>
#include <vector>
#include <cstdint>

enum class WSOpcode : uint8_t {
    Continuation = 0x00,
    Text         = 0x01,
    Binary       = 0x02,
    Close        = 0x08,
    Ping         = 0x09,
    Pong         = 0x0A,
};

class ASyncWebsocket : protected ASyncTCP
{
protected:
    explicit ASyncWebsocket(bool secure = false);
    virtual ~ASyncWebsocket() = default;

    // Connection
    void connectWS(const std::string& host, uint16_t port,
                   const std::string& path = "/",
                   const std::vector<std::string>& extra_headers = {});
    void disconnectWS();
    bool isConnectedWS() const { return m_wsState.load() == WSState::Connected; }

    // Upper bound for one inbound frame payload and for an accumulated fragmented message. A peer that exceeds it is
    // disconnected. The default is generous enough for large bridge dumps; clients of small devices should lower it.
    // The value is clamped so that adding a frame header or an accumulated fragment to it cannot overflow.
    void SetMaxInboundMessageSize(size_t bytes) { m_maxInboundBytes = std::min(bytes, std::numeric_limits<size_t>::max() / 2); }

    // Send API
    void SendWebsocketMessage(const std::string& msg, bool binary = false);
    void SendWebsocketMessage(const uint8_t* pData, size_t length, bool binary = false);
    void SendWebsocketPing(const std::string& payload = "");
    void SendWebsocketClose(uint16_t code = 1000, const std::string& reason = "");

    // Virtual API for concrete drivers
    virtual void OnWebsocketConnected() = 0;
    virtual void OnWebsocketMessage(WSOpcode opcode,
                                    const std::vector<uint8_t>& payload) = 0;
    // The TCP connection is up but the upgrade was refused or never completed, so OnWebsocketConnected() will not
    // follow. OnDisconnect() is not called for this. Default: nothing.
    virtual void OnWebsocketUpgradeFailed() {}

private:
    // Intercept ASyncTCP — declared final so subclasses cannot bypass the protocol
    void OnConnect() final;
    void OnData(const uint8_t* pData, size_t length) final;
    // A frame whose write failed must not be replayed ahead of the next upgrade handshake.
    bool DropFailedWriteOnError() const final { return true; }

    // Closes a connection that this layer gave up on. Once the upgrade had completed the subclass is told through
    // OnDisconnect(), like for a peer close; before that, OnWebsocketUpgradeFailed() is called instead.
    // It does not arm ASyncTCP's reconnect timer: a subclass that wants to reconnect must do so itself,
    // typically from OnDisconnect().
    void dropConnection();

    // Handshake helpers
    std::string BuildUpgradeRequest() const;
    bool        ParseUpgradeResponse(const std::string& response);
    static std::string GenerateWebSocketKey();
    static std::string ComputeAcceptKey(const std::string& key);

    // Frame codec
    static std::vector<uint8_t> EncodeFrame(WSOpcode opcode,
                                            const uint8_t* payload, size_t length);
    static uint32_t GenerateMaskingKey();
    void ProcessInboundData(const uint8_t* pData, size_t length);
    bool ProcessFrame(size_t& startOffset);

    // State
    enum class WSState { Disconnected, TCPConnected, Connected, Closing };
    // Written on the I/O thread and by the caller's thread (connectWS/disconnectWS), read from both.
    std::atomic<WSState> m_wsState{ WSState::Disconnected };
    std::atomic<bool>    m_bWSHandshakeDone{ false };

    static constexpr size_t DEFAULT_MAX_INBOUND_BYTES = 64 * 1024 * 1024;
    static constexpr size_t MAX_UPGRADE_RESPONSE_BYTES = 64 * 1024;
    size_t m_maxInboundBytes = DEFAULT_MAX_INBOUND_BYTES;

    const bool               m_wsSecure;
    uint16_t                 m_wsPort = 0;
    std::string              m_sHost;
    std::string              m_sPath;
    std::vector<std::string> m_extraHeaders;
    std::string              m_sWSKey;

    std::string          m_httpBuffer;
    std::vector<uint8_t> m_rxBuffer;
    std::vector<uint8_t> m_fragmentBuffer;
    int                  m_fragmentOpCode = -1;
};
