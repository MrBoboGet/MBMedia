#include <memory>
#include <vector>
#include <string>
namespace MBMedia
{
	enum class Codec
	{
		MP4,
		MKV,
		WEBM,
		H264,
		H265,
		VP9,
		Null,
	};
	enum class MediaType
	{
		Video,
		Audio,
		Subtitles,
		Null,
	};
	enum class ContainerFormat
	{
		Null,
	};


	struct StreamInfo
	{
	private:

	public:
		MediaType Type = MediaType::Null;
		Codec StreamCodec = Codec::Null;
	};

	struct VideoDecodeInfo
	{

	};
	struct VideoEncodeInfo
	{

	};
	struct AudioDecodeInfo
	{

	};
	struct AudioEncodeInfo
	{

	};

	class StreamPacket
	{
	private:
		void* m_ImplementationData = nullptr;
		MediaType m_Type = MediaType::Null;
	public:
		MediaType GetType() { return(m_Type); }
		~StreamPacket();
	};
	class StreamFrame
	{
	private:
		MediaType m_MediaType = MediaType::Null;
	public:
		MediaType GetMediaType() { return(m_MediaType); };
	};
	class StreamDecoder
	{
	private:
		//void p_Flush();
	public:
		StreamDecoder(StreamInfo const& StreamToDecode);//implicit antagande här, att decoda en stream kan göras "korrekt", att omvandla eller omtolka datan bör göras från framesen vi får efter

		void InsertPacket(StreamPacket const& PacketToDecode);
		bool DataAvailable();
		StreamFrame GetNextFrame();
		void Flush();
		~StreamDecoder();
	};
	class StreamEncoder
	{
	private:

	public:
		StreamEncoder(Codec StreamType, AudioEncodeInfo const& EncodeInfo);
		StreamEncoder(Codec StreamType, VideoEncodeInfo const& EncodeInfo);
		void InsertFrame(StreamFrame const& FrameToEncode);
		void Flush();
		StreamPacket GetNextPacket();
		~StreamEncoder();
	};
	class ContainerDemuxer
	{
	private:
		std::vector<StreamInfo> m_InputStreams = {};
		bool m_FileEnded = false;
	public:
		size_t NumberOfStreams() { return(m_InputStreams.size()); }
		StreamInfo const& GetStreamInfo(size_t StreamIndex);
		StreamPacket GetNextPacket(size_t* StreamIndex);
		bool EndOfFile();

		ContainerDemuxer(std::string const& InputFile);
		~ContainerDemuxer();
	};
	class OutputContext
	{
	private:
		//std::vector<std::unique_ptr<
		ContainerFormat m_OutputFormat = ContainerFormat::Null;
		std::vector<StreamInfo> m_OutputStreams = {};
	public:
		OutputContext(std::string const& OutputFile);

		void AddOutputStream(StreamEncoder&& Encoder);
		void WriteHeader();
		void InsertFrame(StreamFrame const& PacketToInsert, size_t StreamIndex);
		void Finalize();
		~OutputContext();
	};


	class MediaStreamer
	{
	private:
	public:

	};

	AudioEncodeInfo GetAudioEncodePresets(StreamDecoder const& StreamToCopy);
	VideoEncodeInfo GetVideoEncodePresets(StreamDecoder const& StreamToCopy);
	MediaType GetCodecMediaType(Codec InputCodec);
	void Transcode(std::string const& InputFile, std::string const& OutputFile,Codec NewAudioCodec,Codec NewVideoCodec);
};