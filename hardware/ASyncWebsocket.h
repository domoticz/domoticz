#pragma once
#include "ASyncTCP.h"
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
    bool isConnectedWS() const { return m_wsState == WSState::Connected; }

    // Send API
    void SendWebsocketMessage(const std::string& msg, bool binary = false);
    void SendWebsocketMessage(const uint8_t* pData, size_t length, bool binary = false);
    void SendWebsocketPing(const std::string& payload = "");
    void SendWebsocketClose(uint16_t code = 1000, const std::string& reason = "");

    // Virtual API for concrete drivers
    virtual void OnWebsocketConnected() = 0;
    virtual void OnWebsocketMessage(WSOpcode opcode,
                                    const std::vector<uint8_t>& payload) = 0;

private:
    // Intercept ASyncTCP — declared final so subclasses cannot bypass the protocol
    void OnConnect() final;
    void OnData(const uint8_t* pData, size_t length) final;

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
    WSState m_wsState          = WSState::Disconnected;
    bool    m_bWSHandshakeDone = false;

    std::string              m_sHost;
    std::string              m_sPath;
    std::vector<std::string> m_extraHeaders;
    std::string              m_sWSKey;

    std::string          m_httpBuffer;
    std::vector<uint8_t> m_rxBuffer;
    std::vector<uint8_t> m_fragmentBuffer;
    int                  m_fragmentOpCode = -1;
};
