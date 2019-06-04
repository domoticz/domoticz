#include <string>
#include <vector>
#include "Defs.h"

namespace OpenZWave
{
	class SimpleAVCommandItem
	{
	public:
		SimpleAVCommandItem(uint16 const _code, string _name, string _description, uint16 const _version);
		uint16 GetCode();
		string GetName();
		string GetDescription();
		uint16 GetVersion();

		static vector<SimpleAVCommandItem> GetCommands();

	private:
		uint16 m_code;
		string m_name;
		string m_description;
		uint16 m_version;
	};
}

