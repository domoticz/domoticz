#pragma once
#ifndef NOCLOUD
#include <string>
#include <sstream>
// type support
#include "../cereal/types/string.hpp"
#include "../cereal/types/memory.hpp"
// the archiver
#include "../cereal/archives/portable_binary.hpp"
#include "../cereal/archives/json.hpp"
#include <string>

#define SUBSYSTEM_HTTP 0x01
#define SUBSYSTEM_SHAREDDOMOTICZ 0x02
#define SUBSYSTEM_APPS 0x04

/* typedef proxy pdu numbers. These numbers align with mydomoticz. */
#define NOARG
#define PDUSTRING(name)
#define PDULONG(name)
#define PDULONGLONG(name)
#define PROXYPDU(name, members) ePDU_##name,
typedef enum enum_pdu {
#include "proxydef.def"
	ePDU_END
} pdu_enum;
#undef PDUSTRING
#undef PDULONG
#undef PDULONGLONG
#undef PROXYPDU

/* base pdu class */
class CProxyPduBase {
public:
	pdu_enum mPduEnum;
	CProxyPduBase() { mPduEnum = ePDU_NONE; };
	virtual pdu_enum pdu_type() { return mPduEnum; };
	virtual std::string pdu_name() { return "NONE"; };
	template <class Archive> void serialize(Archive &ar, std::uint32_t const version) { ar & CEREAL_NVP(mPduEnum); };
	static std::shared_ptr<CProxyPduBase> FromString(const std::string &str);
};

/* base classes with the pdu member variables */
#define PDUSTRING(name) std::string m_##name = "";
#define PDULONG(name) int32_t m_##name = 0;
#define PDULONGLONG(name) uint64_t m_##name = 0;
#define PROXYPDU(name, members) class ProxyPdu_##name##_onlymembers : public CProxyPduBase { public: members };
#include "proxydef.def"
#undef PDUSTRING
#undef PDULONG
#undef PDULONGLONG
#undef PROXYPDU

/* ProxuPdu_* classes */
#define PDUSTRING(name) ar & CEREAL_NVP(m_##name);
#define PDULONG(name) ar & CEREAL_NVP(m_##name);
#define PDULONGLONG(name) ar & CEREAL_NVP(m_##name);
#define PROXYPDU(name, members) class ProxyPdu_##name : public ProxyPdu_##name##_onlymembers { public: \
	ProxyPdu_##name() { mPduEnum = ePDU_##name ; }; \
	virtual pdu_enum pdu_type() { return ePDU_##name; }; \
	virtual std::string pdu_name() { return #name; }; \
	template <class Archive> void serialize(Archive &ar, std::uint32_t const version) { members }; \
	std::string ToBinary() { \
		std::stringstream os; { /* start a new scope */ \
		cereal::PortableBinaryOutputArchive oarchive(os); \
		std::shared_ptr< ProxyPdu_##name > pt(std::shared_ptr< ProxyPdu_##name >(this, [](ProxyPdu_##name *) {})); \
		oarchive(mPduEnum); \
		oarchive(pt); } \
		return os.str(); \
	}; \
	std::string ToJson() { \
		std::stringstream os; { /* start a new scope */ \
		cereal::JSONOutputArchive oarchive(os); \
		std::shared_ptr< ProxyPdu_##name > pt(std::shared_ptr< ProxyPdu_##name >(this, [](ProxyPdu_##name *) {})); \
		oarchive(mPduEnum); \
		oarchive(pt); } \
		return os.str(); \
	}; \
};
#include "proxydef.def"
#undef PDUSTRING
#undef PDULONG
#undef PDULONGLONG
#undef PROXYPDU

#endif
