#pragma once

namespace Plugins {

	class CPluginMessage;

	class CPluginProtocol
	{
	protected:
		std::vector<byte>	m_sRetainedData;
		bool				m_Secure;

	public:
		CPluginProtocol() : m_Secure(false) {};
		virtual void				ProcessInbound(const ReadEvent* Message);
		virtual std::vector<byte>	ProcessOutbound(const WriteDirective* WriteMessage);
		virtual void				Flush(CPlugin* pPlugin, PyObject* pConnection);
		virtual int					Length() { return m_sRetainedData.size(); };
		virtual bool				Secure() { return m_Secure; };

		static CPluginProtocol*		Create(std::string sProtocol);
	};

	class CPluginProtocolLine : CPluginProtocol
	{
		virtual void	ProcessInbound(const ReadEvent* Message);
	};

	class CPluginProtocolXML : CPluginProtocol
	{
	private:
		std::string		m_Tag;
	public:
		virtual void	ProcessInbound(const ReadEvent* Message);
	};

	class CPluginProtocolJSON : CPluginProtocol
	{
	protected:
		PyObject* JSONtoPython(Json::Value* pJSON);
	public:
		PyObject * JSONtoPython(std::string sJSON);
		std::string PythontoJSON(PyObject * pDict);
		virtual void	ProcessInbound(const ReadEvent* Message);
	};

	class CPluginProtocolHTTP : public CPluginProtocol
	{
	private:
		std::string		m_Status;
		int				m_ContentLength;
		void*			m_Headers;
		bool			m_Chunked;
		size_t			m_RemainingChunk;
	protected:
		void			ExtractHeaders(std::string*	pData);
	public:
		CPluginProtocolHTTP(bool Secure) : m_ContentLength(0), m_Headers(NULL), m_Chunked(false), m_RemainingChunk(0) { m_Secure = Secure; };
		virtual void				ProcessInbound(const ReadEvent* Message);
		virtual std::vector<byte>	ProcessOutbound(const WriteDirective* WriteMessage);
	};

	class CPluginProtocolWS : public CPluginProtocolHTTP
	{
	private:
		bool	ProcessWholeMessage(std::vector<byte> &vMessage, const ReadEvent * Message);
	public:
		CPluginProtocolWS(bool Secure) : CPluginProtocolHTTP(Secure) {};
		virtual void				ProcessInbound(const ReadEvent* Message);
		virtual std::vector<byte>	ProcessOutbound(const WriteDirective* WriteMessage);
	};

	class CPluginProtocolICMP : CPluginProtocol
	{
		virtual void	ProcessInbound(const ReadEvent* Message);
	};

	class CPluginProtocolMQTT : CPluginProtocol
	{
	private:
		int				m_PacketID;
		bool			m_bErrored;
	public:
		CPluginProtocolMQTT(bool Secure) : m_PacketID(1), m_bErrored(false) { m_Secure = Secure; };
		virtual void				ProcessInbound(const ReadEvent* Message);
		virtual std::vector<byte>	ProcessOutbound(const WriteDirective* WriteMessage);
	};
}
