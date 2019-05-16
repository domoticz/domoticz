// for doing the actual serialization
#include "stdafx.h"
#ifndef NOCLOUD
#include "proxycereal.hpp"

#define PDUSTRING(name)
#define PDULONG(name)
#define PDULONGLONG(name)
#define PROXYPDU(name, members) case ePDU_##name: { /* start a new scope */ std::shared_ptr<ProxyPdu_##name> pdu(new ProxyPdu_##name()); iarchive(pdu); base = pdu; } break;
std::shared_ptr<CProxyPduBase> CProxyPduBase::FromString(const std::string &str) {
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
#undef PDULONGLONG
#undef PROXYPDU

#endif
