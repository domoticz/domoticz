#pragma once

#include <string>
#include "../cereal/archives/portable_binary.hpp"

#define NOARG
#define PDUSTRING(name) std::string m_##name;
#define PDULONG(name) long m_##name;
#define PROXYPDU(name, members) class C##name##_onlymembers { public: members };

#include "proxydef.def"

#undef PDUSTRING
#undef PDULONG
#undef PROXYPDU

#define PDUSTRING(name) ar & CEREAL_NVP(name);
#define PDULONG(name) ar & CEREAL_NVP(name);
#define PROXYPDU(name, members) class C##name## : public C##name##_onlymembers { public: template <class Archive> void Serialize(Archive &ar) { members } };

#include "proxydef.def"

#undef PDUSTRING
#undef PDULONG
#undef PROXYPDU

