#include "command_classes/CommandClass.h"

namespace OpenZWave
{
	/** Implements COMMAND_CLASS_SIMPLE_AV_CONTROL (0x94), a Z-Wave device command class. */
	class SimpleAV : public CommandClass
	{
	public:
		static CommandClass* Create(uint32 const _homeId, uint8 const _nodeId) { return new SimpleAV(_homeId, _nodeId); }
		virtual ~SimpleAV() {}

		static uint8 const StaticGetCommandClassId() { return 0x94; }
		static string const StaticGetCommandClassName() { return "COMMAND_CLASS_SIMPLE_AV_CONTROL"; }

		// From CommandClass
		virtual uint8 const GetCommandClassId() const override { return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName() const override { return StaticGetCommandClassName(); }
		virtual bool HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance = 1) override;
		virtual bool SetValue(Value const& _value) override;

	protected:
		virtual void CreateVars(uint8 const _instance) override;
		
	private:
		SimpleAV(uint32 const _homeId, uint8 const _nodeId) : CommandClass(_homeId, _nodeId), m_sequence(0) {}
		uint8 m_sequence;
	};

} // namespace OpenZWave
