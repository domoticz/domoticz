#pragma once

namespace Plugins {

	class CPluginMessage;

	class CPluginProtocol
	{
	protected:
		std::vector<byte>	m_sRetainedData;
		bool m_Secure{ false };

	      public:
		CPluginProtocol() = default;
		virtual void				ProcessInbound(const ReadEvent* Message);
		virtual std::vector<byte>	ProcessOutbound(const WriteDirective* WriteMessage);
		virtual void				Flush(CPlugin* pPlugin, PyObject* pConnection);
		virtual int					Length() { return m_sRetainedData.size(); };
		virtual bool				Secure() { return m_Secure; };

		static CPluginProtocol *Create(const std::string &sProtocol);
	};

	class CPluginProtocolLine : CPluginProtocol
	{
		void ProcessInbound(const ReadEvent *Message) override;
	};

	class CPluginProtocolXML : CPluginProtocol
	{
	private:
		std::string		m_Tag;
	public:
	  void ProcessInbound(const ReadEvent *Message) override;
	};

	class CPluginProtocolJSON : CPluginProtocol
	{
	protected:
		PyObject* JSONtoPython(Json::Value* pJSON);
	public:
	  PyObject *JSONtoPython(const std::string &sJSON);
	  std::string PythontoJSON(PyObject *pDict);
	  void ProcessInbound(const ReadEvent *Message) override;
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
		void Flush(CPlugin *pPlugin, PyObject *pConnection) override;

	      public:
		CPluginProtocolHTTP(bool Secure)
			: m_ContentLength(0)
			, m_Headers(nullptr)
			, m_Chunked(false)
			, m_RemainingChunk(0)
		{
			m_Secure = Secure;
		};
		void ProcessInbound(const ReadEvent *Message) override;
		std::vector<byte> ProcessOutbound(const WriteDirective *WriteMessage) override;
	};

	class CPluginProtocolWS : public CPluginProtocolHTTP
	{
	private:
		bool	ProcessWholeMessage(std::vector<byte> &vMessage, const ReadEvent * Message);
	public:
		CPluginProtocolWS(bool Secure) : CPluginProtocolHTTP(Secure) {};
		void ProcessInbound(const ReadEvent *Message) override;
		std::vector<byte> ProcessOutbound(const WriteDirective *WriteMessage) override;
	};

	class CPluginProtocolICMP : CPluginProtocol
	{
		void ProcessInbound(const ReadEvent *Message) override;
	};

	class CPluginProtocolMQTT : CPluginProtocol
	{
	private:
		int				m_PacketID;
		bool			m_bErrored;
	public:
		CPluginProtocolMQTT(bool Secure) : m_PacketID(1), m_bErrored(false) { m_Secure = Secure; };
		void ProcessInbound(const ReadEvent *Message) override;
		std::vector<byte> ProcessOutbound(const WriteDirective *WriteMessage) override;
	};
} // namespace Plugins
