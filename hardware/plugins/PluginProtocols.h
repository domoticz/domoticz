#pragma once

namespace Plugins {

	class CPluginMessage;

	class CPluginProtocol
	{
	protected:
		std::vector<byte>	m_sRetainedData;

	public:
		virtual void				ProcessInbound(const ReadMessage* Message);
		virtual std::vector<byte>	ProcessOutbound(const WriteDirective* WriteMessage);
		virtual void				Flush(const int HwdID);
		virtual int					Length() { return m_sRetainedData.size(); };
	};

	class CPluginProtocolLine : CPluginProtocol
	{
		virtual void	ProcessInbound(const ReadMessage* Message);
	};

	class CPluginProtocolXML : CPluginProtocol
	{
	private:
		std::string		m_Tag;
	public:
		virtual void	ProcessInbound(const ReadMessage* Message);
	};

	class CPluginProtocolJSON : CPluginProtocol
	{
		virtual void	ProcessInbound(const ReadMessage* Message);
	};

	class CPluginProtocolHTTP : CPluginProtocol
	{
	private:
		int				m_Status;
		int				m_ContentLength;
		void*			m_Headers;
		std::string		m_Username;
		std::string		m_Password;
		bool			m_Chunked;
		size_t			m_RemainingChunk;
	public:
		CPluginProtocolHTTP() : m_Status(0), m_ContentLength(0), m_Headers(NULL), m_Chunked(false) {};
		virtual void				ProcessInbound(const ReadMessage* Message);
		virtual std::vector<byte>	ProcessOutbound(const WriteDirective* WriteMessage);
		void						AuthenticationDetails(std::string Username, std::string Password)
		{
			m_Username = Username;
			m_Password = Password;
		};
	};

	class CPluginProtocolMQTT : CPluginProtocol {}; // Maybe?

}