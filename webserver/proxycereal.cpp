// for doing the actual serialization
#include "stdafx.h"
#include "proxycereal.h"

#define PDUSTRING(name)
#define PDULONG(name)
#define PROXYPDU(name, members) case ePDU_##name: { /* start a new scope */ std::shared_ptr<ProxyPdu_##name> pdu(new ProxyPdu_##name()); iarchive(pdu); base = pdu; } break;
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
	std::string s = b.ToString();
	int l = s.length();
	std::shared_ptr<CProxyPduBase> base2 = FromString(s);
	std::string r = base2->pdu_name();
	std::shared_ptr<CProxyPduBase> base = FromString(s);
	std::string t = base->pdu_name();
	ProxyPdu_ASSIGNKEY *key = dynamic_cast<ProxyPdu_ASSIGNKEY *>(base.get());
	std::string q = key->m_newapi;
	int c = 1;
}
