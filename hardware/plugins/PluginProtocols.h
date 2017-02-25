#pragma once

namespace Plugins {

	class CPluginMessage;

	class CPluginProtocol
	{
	protected:
		std::string		m_sRetainedData;

	public:
		virtual void		ProcessInbound(const int HwdID, std::string& ReadData);
		virtual std::string	ProcessOutbound(const CPluginMessage & WriteMessage);
		virtual void		Flush(const int HwdID);
		virtual int			Length() { return m_sRetainedData.length(); };
	};

	class CPluginProtocolLine : CPluginProtocol
	{
		virtual void	ProcessInbound(const int HwdID, std::string& ReadData);
	};

	class CPluginProtocolXML : CPluginProtocol
	{
	private:
		std::string		m_Tag;
	public:
		virtual void	ProcessInbound(const int HwdID, std::string& ReadData);
	};

	class CPluginProtocolJSON : CPluginProtocol
	{
		virtual void	ProcessInbound(const int HwdID, std::string& ReadData);
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
		virtual void		ProcessInbound(const int HwdID, std::string& ReadData);
		virtual std::string	ProcessOutbound(const CPluginMessage & WriteMessage);
		void				AuthenticationDetails(std::string Username, std::string Password)
		{
			m_Username = Username;
			m_Password = Password;
		};
	};

	class CPluginProtocolMQTT : CPluginProtocol {}; // Maybe?

}