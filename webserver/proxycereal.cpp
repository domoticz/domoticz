// for doing the actual serialization
#include "stdafx.h"
#include "proxycereal.h"

#define PDUSTRING(name)
#define PDULONG(name)
#define PROXYPDU(name, members) case ePDU_##name##: { /* start a new scope */ std::shared_ptr<ProxyPdu_##name> pdu(new ProxyPdu_##name()); iarchive(pdu); base = pdu; } break;
std::shared_ptr<CProxyPduBase> FromString(const std::string &str) {
	std::stringstream stream(str);
	cereal::PortableBinaryInputArchive iarchive(stream);
	std::shared_ptr<CProxyPduBase> base;
	pdu_enum theEnum;
	iarchive(theEnum);
	switch (theEnum) {
#include "proxydef.def"
	default:
		// pdu type not found
		base = NULL;
		break;
	}
	return base;
};
#undef PDUSTRING
#undef PDULONG
#undef PROXYPDU

void a() {
	ProxyPdu_ASSIGNKEY b;
	b.m_newapi = "123";
	std::string s = b.ToBinary();
	std::string json = b.ToJson();
	int l = s.length();
	std::shared_ptr<CProxyPduBase> base2 = FromString(s);
	std::string r = base2->pdu_name();
	CPduExec exec(base2);
	exec.Exec();
}

void CPduExec::Exec()
{
	switch (m_pdu->pdu_type()) {
#define PDUSTRING(name)
#define PDULONG(name)
#define PROXYPDU(name, members) case ePDU_##name##: OnPduReceived(std::dynamic_pointer_cast<ProxyPdu_##name##>(m_pdu)); break;
#include "proxydef.def"
	default:
		// pdu enum not found
		break;
	}
#undef PDUSTRING
#undef PDULONG
#undef PROXYPDU
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_NONE> pdu)
{
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_AUTHENTICATE> pdu)
{
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_AUTHRESP> pdu)
{
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_ASSIGNKEY> pdu)
{
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_ENQUIRE> pdu)
{
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_REQUEST> pdu)
{
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_RESPONSE> pdu)
{
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_SERV_CONNECT> pdu)
{
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_SERV_CONNECTRESP> pdu)
{
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_SERV_RECEIVE> pdu)
{
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_SERV_SEND> pdu)
{
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_SERV_DISCONNECT> pdu)
{
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_SERV_ROSTERIND> pdu)
{
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_WS_OPEN> pdu)
{
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_WS_CLOSE> pdu)
{
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_WS_SEND> pdu)
{
}

void CPduExec::OnPduReceived(std::shared_ptr<ProxyPdu_WS_RECEIVE> pdu)
{
}
