// for doing the actual serialization
#include "stdafx.h"
#ifndef NOCLOUD
#include "proxycereal.hpp"

#define PDUSTRING(name)
#define PDULONG(name)
#define PROXYPDU(name, members) case ePDU_##name##: pdu.reset(new ProxyPdu_##name()); iarchive(pdu); break;
std::shared_ptr<CProxyPduBase> CProxyPduBase::FromString(const std::string &str) {
	std::stringstream stream(str);
	cereal::PortableBinaryInputArchive iarchive(stream);
	std::shared_ptr<CProxyPduBase> pdu;
	pdu_enum theEnum;
	iarchive(theEnum);
	switch (theEnum) {
#include "proxydef.def"
	default:
		// pdu type not found
		pdu.reset();
		break;
	}
	return pdu;
};
#undef PDUSTRING
#undef PDULONG
#undef PROXYPDU

#endif