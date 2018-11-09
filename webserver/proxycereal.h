#pragma once

#include <string>
#include <sstream>
// type support
#include "../cereal/types/common.hpp"
#include "../cereal/types/map.hpp"
#include "../cereal/types/vector.hpp"
#include "../cereal/types/string.hpp"
#include "../cereal/types/map.hpp"
#include "../cereal/types/complex.hpp"
#include "../cereal/types/memory.hpp"
// Include the polymorphic serialization and registration mechanisms
#include "../cereal/types/polymorphic.hpp"
// the archiver
#include "../cereal/archives/portable_binary.hpp"
#include <string>
#include <memory>

#define NOARG
#define PDUSTRING(name)
#define PDULONG(name)
#define PROXYPDU(name, members) ePDU_##name,
typedef enum enum_pdu {
#include "proxydef.def"
ePDU_END
} pdu_enum;
#undef PDUSTRING
#undef PDULONG
#undef PROXYPDU

class CProxyPduBase {
public:
	pdu_enum myEnum;
	CProxyPduBase() { myEnum = ePDU_NONE; };
	virtual pdu_enum pdu_type() { return myEnum; };
	virtual std::string pdu_name() { return "NONE"; };
	template <class Archive> void serialize(Archive &ar, std::uint32_t const version) { };
};

#define PDUSTRING(name) std::string m_##name = "";
#define PDULONG(name) long m_##name = 0;
#define PROXYPDU(name, members) class ProxyPdu_##name##_onlymembers : public CProxyPduBase { public: members };
#include "proxydef.def"
#undef PDUSTRING
#undef PDULONG
#undef PROXYPDU

#define PDUSTRING(name) ar & CEREAL_NVP(m_##name);
#define PDULONG(name) ar & CEREAL_NVP(m_##name);
#define PROXYPDU(name, members) class ProxyPdu_##name : public ProxyPdu_##name##_onlymembers { public: \
    ProxyPdu_##name() { myEnum = ePDU_##name ; }; \
	virtual pdu_enum pdu_type() { return ePDU_##name; }; \
	virtual std::string pdu_name() { return #name; }; \
	template <class Archive> void serialize(Archive &ar, std::uint32_t const version) { members }; \
	std::string ToString() { \
		std::stringstream os; { /* start a new scope */ \
		cereal::PortableBinaryOutputArchive oarchive(os); \
		std::shared_ptr<ProxyPdu_##name##> pt(std::shared_ptr<ProxyPdu_##name##>(this, [](ProxyPdu_##name## *) {})); \
		oarchive(myEnum); \
		oarchive(pt); } \
		return os.str(); \
	}; \
};

#include "proxydef.def"

#undef PDUSTRING
#undef PDULONG
#undef PROXYPDU
