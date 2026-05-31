#include "stdafx.h"
#include "ASyncWebsocket.h"
#include "../main/Logger.h"
#include "../extern/libwebem/src/sha1.h"
#include "../extern/libwebem/include/libwebem/Base64.h"
#include <random>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <mutex>
#include <array>

ASyncWebsocket::ASyncWebsocket(bool secure)
    : ASyncTCP(secure)
{
}

void ASyncWebsocket::connectWS(const std::string& host, uint16_t port,
                                const std::string& path,
                                const std::vector<std::string>& extra_headers)
{
    m_sHost            = host;
    m_sPath            = path;
    m_extraHeaders     = extra_headers;
    m_wsState          = WSState::Disconnected;
    m_bWSHandshakeDone = false;
    connect(host, port);
}

void ASyncWebsocket::disconnectWS()
{
    m_bWSHandshakeDone = false;
    m_wsState          = WSState::Disconnected;
    disconnect();
}

std::string ASyncWebsocket::GenerateWebSocketKey()
{
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<int> dist(0, 255);
    static std::mutex mtx;
    std::lock_guard<std::mutex> lk(mtx);

    std::array<uint8_t, 16> nonce{};
    for (auto& b : nonce) b = static_cast<uint8_t>(dist(rng));
    return base64_encode_buf(nonce.data(), 16);
}

std::string ASyncWebsocket::ComputeAcceptKey(const std::string& key)
{
    const std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = key + magic;
    uint8_t hash[20]{};
    sha1::calc(combined.data(), static_cast<int>(combined.size()), hash);
    return base64_encode_buf(hash, 20);
}

std::string ASyncWebsocket::BuildUpgradeRequest() const
{
    std::ostringstream ss;
    ss << "GET " << m_sPath << " HTTP/1.1\r\n"
       << "Host: " << m_sHost << "\r\n"
       << "Upgrade: websocket\r\n"
       << "Connection: Upgrade\r\n"
       << "Sec-WebSocket-Key: " << m_sWSKey << "\r\n"
       << "Sec-WebSocket-Version: 13\r\n";
    for (const auto& h : m_extraHeaders)
        ss << h << "\r\n";
    ss << "\r\n";
    return ss.str();
}

bool ASyncWebsocket::ParseUpgradeResponse(const std::string& response)
{
    if (response.find("101") == std::string::npos) return false;

    auto hasHeader = [&](const std::string& name, const std::string& value) {
        std::string lower = response;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::string lname = name, lval = value;
        std::transform(lname.begin(), lname.end(), lname.begin(), ::tolower);
        std::transform(lval.begin(),  lval.end(),  lval.begin(),  ::tolower);
        auto pos = lower.find(lname);
        if (pos == std::string::npos) return false;
        return lower.find(lval, pos) != std::string::npos;
    };

    if (!hasHeader("upgrade", "websocket")) return false;
    if (!hasHeader("connection", "upgrade")) return false;

    std::string expected = ComputeAcceptKey(m_sWSKey);
    if (response.find(expected) == std::string::npos) {
        _log.Log(LOG_ERROR, "ASyncWebsocket: Sec-WebSocket-Accept mismatch");
        return false;
    }

    return true;
}

void ASyncWebsocket::OnConnect()
{
    m_wsState = WSState::TCPConnected;
    m_httpBuffer.clear();
    m_rxBuffer.clear();
    m_fragmentBuffer.clear();
    m_fragmentOpCode = -1;
    m_sWSKey = GenerateWebSocketKey();
    const std::string req = BuildUpgradeRequest();
    write(reinterpret_cast<const uint8_t*>(req.data()), req.size());
}

uint32_t ASyncWebsocket::GenerateMaskingKey()
{
    static std::mt19937 rng(std::random_device{}());
    static std::mutex mtx;
    std::lock_guard<std::mutex> lk(mtx);
    return static_cast<uint32_t>(rng());
}

std::vector<uint8_t> ASyncWebsocket::EncodeFrame(WSOpcode opcode,
                                                   const uint8_t* payload,
                                                   size_t length)
{
    std::vector<uint8_t> frame;
    frame.reserve(length + 14);

    // RFC 6455: 64-bit length MSB must be 0
    if (length > 0x7FFFFFFFFFFFFFFF) {
        _log.Log(LOG_ERROR, "ASyncWebsocket: payload exceeds RFC 6455 maximum");
        return {};
    }

    frame.push_back(0x80 | static_cast<uint8_t>(opcode));

    if (length < 126) {
        frame.push_back(0x80 | static_cast<uint8_t>(length));
    } else if (length <= 0xFFFF) {
        frame.push_back(0x80 | 126);
        frame.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
        frame.push_back(static_cast<uint8_t>( length       & 0xFF));
    } else {
        frame.push_back(0x80 | 127);
        for (int i = 7; i >= 0; --i)
            frame.push_back(static_cast<uint8_t>((length >> (i * 8)) & 0xFF));
    }

    uint32_t maskKey = GenerateMaskingKey();
    frame.push_back(static_cast<uint8_t>((maskKey >> 24) & 0xFF));
    frame.push_back(static_cast<uint8_t>((maskKey >> 16) & 0xFF));
    frame.push_back(static_cast<uint8_t>((maskKey >>  8) & 0xFF));
    frame.push_back(static_cast<uint8_t>( maskKey        & 0xFF));
    const uint8_t* pMask = &frame[frame.size() - 4];

    for (size_t i = 0; i < length; ++i)
        frame.push_back(payload[i] ^ pMask[i % 4]);

    return frame;
}

void ASyncWebsocket::SendWebsocketMessage(const std::string& msg, bool binary)
{
    if (!m_bWSHandshakeDone) return;
    auto frame = EncodeFrame(binary ? WSOpcode::Binary : WSOpcode::Text,
                             reinterpret_cast<const uint8_t*>(msg.data()),
                             msg.size());
    write(frame.data(), frame.size());
}

void ASyncWebsocket::SendWebsocketMessage(const uint8_t* pData, size_t length, bool binary)
{
    if (!m_bWSHandshakeDone) return;
    auto frame = EncodeFrame(binary ? WSOpcode::Binary : WSOpcode::Text, pData, length);
    write(frame.data(), frame.size());
}

void ASyncWebsocket::SendWebsocketPing(const std::string& payload)
{
    if (!m_bWSHandshakeDone) return;
    auto frame = EncodeFrame(WSOpcode::Ping,
                             reinterpret_cast<const uint8_t*>(payload.data()),
                             payload.size());
    write(frame.data(), frame.size());
}

void ASyncWebsocket::SendWebsocketClose(uint16_t code, const std::string& reason)
{
    if (!m_bWSHandshakeDone) return;
    m_wsState = WSState::Closing;
    std::vector<uint8_t> payload;
    payload.push_back(static_cast<uint8_t>((code >> 8) & 0xFF));
    payload.push_back(static_cast<uint8_t>( code       & 0xFF));
    size_t maxReasonLen = std::min(static_cast<size_t>(123), reason.size());
    for (size_t i = 0; i < maxReasonLen; ++i)
        payload.push_back(static_cast<uint8_t>(reason[i]));
    auto frame = EncodeFrame(WSOpcode::Close, payload.data(), payload.size());
    write(frame.data(), frame.size());
}

void ASyncWebsocket::ProcessInboundData(const uint8_t* pData, size_t length)
{
    m_rxBuffer.insert(m_rxBuffer.end(), pData, pData + length);
    size_t startOffset = 0;
    while (ProcessFrame(startOffset)) { }
    if (startOffset > 0)
        m_rxBuffer.erase(m_rxBuffer.begin(),
                         m_rxBuffer.begin() + static_cast<std::ptrdiff_t>(startOffset));
}

bool ASyncWebsocket::ProcessFrame(size_t& startOffset)
{
    const auto& buf = m_rxBuffer;
    if (buf.size() - startOffset < 2) return false;

    auto vAt = [&](size_t i) -> uint8_t { return buf[startOffset + i]; };

    size_t iOffset = 0;

    bool    bFin    = (vAt(iOffset) & 0x80) != 0;
    int     iOpCode = (vAt(iOffset) & 0x0F);
    iOffset++;

    bool   bMasked        = (vAt(iOffset) & 0x80) != 0;
    size_t lPayloadLength = (vAt(iOffset) & 0x7F);
    iOffset++;

    if (bMasked)
        _log.Log(LOG_ERROR, "ASyncWebsocket: received masked frame from server (RFC 6455 violation)");

    if (lPayloadLength == 126) {
        if (buf.size() - startOffset < iOffset + 2) return false;
        lPayloadLength = (static_cast<size_t>(vAt(iOffset)) << 8)
                       |  static_cast<size_t>(vAt(iOffset + 1));
        iOffset += 2;
    } else if (lPayloadLength == 127) {
        if (buf.size() - startOffset < iOffset + 8) return false;
        uint64_t u64Len = 0;
        for (int i = 0; i < 8; ++i)
            u64Len = (u64Len << 8) | vAt(iOffset + i);
        if ((u64Len >> 63) || u64Len > SIZE_MAX) {
            _log.Log(LOG_ERROR, "ASyncWebsocket: invalid 64-bit frame length");
            startOffset = buf.size();
            return false;
        }
        lPayloadLength = static_cast<size_t>(u64Len);
        iOffset += 8;
    }

    uint8_t maskKey[4]{};
    if (bMasked) {
        if (buf.size() - startOffset < iOffset + 4) return false;
        for (int i = 0; i < 4; ++i) maskKey[i] = vAt(iOffset + i);
        iOffset += 4;
    }

    if (buf.size() - startOffset < iOffset + lPayloadLength) return false;

    std::vector<uint8_t> vPayload(buf.begin() + startOffset + iOffset,
                                  buf.begin() + startOffset + iOffset + lPayloadLength);
    if (bMasked) {
        for (size_t i = 0; i < vPayload.size(); ++i)
            vPayload[i] ^= maskKey[i % 4];
    }
    startOffset += iOffset + lPayloadLength;

    if (iOpCode == 0x00) {
        if (m_fragmentOpCode < 0) {
            _log.Log(LOG_ERROR, "ASyncWebsocket: Continuation frame without prior fragmented message");
            startOffset = buf.size();
            return false;
        }
        m_fragmentBuffer.insert(m_fragmentBuffer.end(), vPayload.begin(), vPayload.end());
        if (!bFin) return true;
        vPayload         = std::move(m_fragmentBuffer);
        iOpCode          = m_fragmentOpCode;
        m_fragmentBuffer.clear();
        m_fragmentOpCode = -1;
    } else if (!bFin && iOpCode < 0x08) {
        m_fragmentBuffer = vPayload;
        m_fragmentOpCode = iOpCode;
        return true;
    }

    switch (iOpCode) {
    case 0x01:
    case 0x02:
        OnWebsocketMessage(static_cast<WSOpcode>(iOpCode), vPayload);
        break;
    case 0x08:
        {
            uint16_t code = vPayload.size() >= 2
                ? static_cast<uint16_t>((vPayload[0] << 8) | vPayload[1]) : 1000;
            SendWebsocketClose(code);
            m_wsState = WSState::Closing;
        }
        break;
    case 0x09:
        {
            auto pong = EncodeFrame(WSOpcode::Pong, vPayload.data(), vPayload.size());
            write(pong.data(), pong.size());
        }
        break;
    case 0x0A:
        break;
    default:
        _log.Log(LOG_ERROR, "ASyncWebsocket: unknown opcode 0x%02X", iOpCode);
        break;
    }
    return true;
}

void ASyncWebsocket::OnData(const uint8_t* pData, size_t length)
{
    switch (m_wsState)
    {
    case WSState::TCPConnected:
        m_httpBuffer.append(reinterpret_cast<const char*>(pData), length);
        {
            auto pos = m_httpBuffer.find("\r\n\r\n");
            if (pos == std::string::npos) return;

            std::string httpResp = m_httpBuffer.substr(0, pos + 4);
            std::string leftover = m_httpBuffer.substr(pos + 4);
            m_httpBuffer.clear();

            if (!ParseUpgradeResponse(httpResp)) {
                _log.Log(LOG_ERROR, "ASyncWebsocket: WebSocket upgrade failed");
                disconnectWS();
                return;
            }
            m_wsState          = WSState::Connected;
            m_bWSHandshakeDone = true;
            OnWebsocketConnected();

            if (!leftover.empty())
                ProcessInboundData(
                    reinterpret_cast<const uint8_t*>(leftover.data()),
                    leftover.size());
        }
        break;

    case WSState::Connected:
    case WSState::Closing:
        ProcessInboundData(pData, length);
        break;

    default:
        break;
    }
}
