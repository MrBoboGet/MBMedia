#pragma once
#include "MBAudioDefinitions.h"
#include "DoNothing.h"
#include <stdint.h>
#include <memory>
#include <vector>
#include <string>

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
namespace MBMedia
{
	SampleFormat GetPlanarAudioFormat(SampleFormat FormatToConvert);
	SampleFormat GetInterleavedAudioFormat(SampleFormat FormatToConvert);
	bool FormatIsPlanar(SampleFormat FormatToInspect);
	SampleFormatInfo GetSampleFormatInfo(SampleFormat FormatToInspect);
	size_t GetChannelFrameSize(AudioParameters const& ParametersToInspect);
	size_t GetParametersDataPlanes(AudioParameters const& ParametersToInspect);
	uint8_t** AllocateAudioBuffer(AudioParameters const& BufferParameters, size_t NumberOfSamples);
	void DeallocateAudioBuffer(AudioParameters const& BufferParameters, const uint8_t* const* BufferToDeallocatioe);

	//utiltiy funktion för att verifiera att samplesen inte är fucked, fungerar egentligen bara för floats eftersom 
	//PCM inte har något sätt att avgöra hur om den är invalid. Kanske skulle kunna inkludera att skillnaden mellan 2 individuella
	//samples inte orsaker en "pop" så att säga
	bool VerifySamples(const uint8_t* const* AudioData, AudioParameters const& DataParameters, size_t NumberOfFrames, size_t SampelsOffset = 0);


	//OBS Förutsätter att output datan är har utrymmer den behöver. Låg nivå konvertering. Kanske borde lägga till en Converssion context klass...
	void ConvertSampleData(const uint8_t** InputData, AudioParameters const& InputParameters, uint8_t** OutputBuffer, AudioParameters const& OutputParameters, size_t SamplesToConvert);

	class AudioFIFOBuffer
	{
	private:
		std::vector<std::vector<uint8_t>> m_InternalBuffers = {};
		static constexpr float m_GrowthFactor = 1.5;
		SampleFormatInfo m_InputFormatInfo;
		AudioParameters m_InputParameters;
		size_t m_StoredSamples = 0;
		size_t m_CurrentBuffersOffset = 0;

		bool m_IsInitialized = false;
		void p_ResizeBuffers();
		size_t p_GetChannelFrameSize();
		void Initialize(AudioParameters const& InputParameters, size_t InitialNumberOfSamples);

		mutable uint8_t** m_DataPointers = nullptr;
	public:
		AudioFIFOBuffer() {};
		AudioFIFOBuffer(AudioParameters const& InputParameters, size_t InitialNumberOfSamples);

		//void Initialize(AudioParameters const& InputParameters, size_t InitialNumberOfSamples);

		void InsertData(const uint8_t* const* AudioData, size_t NumberOfSamples);
		void InsertData(const uint8_t* const* AudioData, size_t NumberOfSamples, size_t InputSampleOffset);
		size_t ReadData(uint8_t** OutputBuffers, size_t NumberOfSamplesToRead);
		size_t ReadData(uint8_t** OutputBuffers, size_t NumberOfSamplesToRead, size_t OutputSampleOffset);

		uint8_t** GetBuffer();
		const uint8_t* const* GetBuffer() const;

		void DiscardSamples(size_t SamplesToDiscard);

		size_t AvailableSamples();
	};

	class AudioDataConverter : public AudioStreamFilter
	{
	private:
		std::unique_ptr<void, void (*)(void*)> m_ConversionContext = std::unique_ptr<void, void (*)(void*)>(nullptr, _DoNothing);
		AudioFIFOBuffer m_AudioBuffer;
		AudioParameters m_InputParameters;
		AudioParameters m_OutputParameters;
	public:
		AudioDataConverter(AudioParameters const& InputParameters, AudioParameters const& OutputParameters);
		//finns inte riktigt något sätt att verifiera att input datan är korrekt formatterad efter AudioParamters klassen, men är den ej det är det
		//undefined behaviour
		AudioDataConverter(AudioParameters const& OutputParameters);
		void InitializeInputParameters(AudioParameters const& InputParameters) override;
		AudioParameters GetAudioParameters() override;
		void InsertData(const uint8_t* const* DataToInsert, size_t NumberOfSamples,size_t InputSampleOffset) override;
		size_t AvailableSamples() override;//frames eller samples, vet inte riktigt än...
		size_t GetNextSamples(uint8_t** OutputBuffer, size_t NumberOfSamples,size_t OutputSampleOffset) override;
		//void Flush() override;
		~AudioDataConverter() override {}
	};
	class AsyncrousAudioBuffer : public MBMedia::AudioStream
	{
	private:
		std::unique_ptr<AudioStream> m_StreamToBuffer = nullptr;

		std::thread m_RetrieverThread;

		std::mutex m_AudioBufferMutex;
		AudioFIFOBuffer m_AudioBuffer;
		
		std::mutex m_BufferFullMutex;
		std::condition_variable m_BufferFullConditional;

		std::atomic<bool> m_StreamFinished{ false };
		std::atomic<float> m_TargetBufferSize{ 5 };
		std::atomic<bool> m_Finishing{ true };

		bool m_Initialised = false;
		//Read only förutom vid initialisering
		AudioParameters m_AudioParameters;
		//


		static constexpr size_t m_SamplesPerLoad = 4096;

		static void p_RetrieveAudioData(AsyncrousAudioBuffer* AssociatedObject);
	public:
		AsyncrousAudioBuffer();
		AsyncrousAudioBuffer(std::unique_ptr<AudioStream> m_StreamToBuffer);
		
		void SetStreamToBuffer(std::unique_ptr<AudioStream> m_StreamToBuffer);



		virtual MBMedia::AudioParameters GetAudioParameters() override;
		virtual size_t GetNextSamples(uint8_t** DataBuffer, size_t NumberOfSamples,size_t OutputSampleOffset) override;
		virtual bool IsFinished() override;
	
		~AsyncrousAudioBuffer() override;
	};
	class AudioBuffer
	{
	private:
		friend void swap(AudioBuffer& LeftBuffer,AudioBuffer& RightBuffer);
		uint8_t** m_InternalBuffer = nullptr;
		AudioParameters m_AudioParameters;
		size_t m_StoredSamples = 0;

	public:
		AudioBuffer();
		
		AudioBuffer(AudioParameters const& AudioParameters, size_t NumberOfSamples);
		AudioBuffer(AudioBuffer const& BufferToCopy);
		AudioBuffer(AudioBuffer&& BufferToSteal) noexcept;
		AudioBuffer& operator=(AudioBuffer BufferToSteal);

		uint8_t** GetData();
		size_t GetSamplesCount();
		size_t GetNumberOfPlanes();
		size_t GetPlaneSize();

		~AudioBuffer();
	};
	class AudioPipeline : public AudioStream
	{
	private:
		std::unique_ptr<AudioStream> m_InitialSource = nullptr;
		std::vector<std::unique_ptr<AudioStreamFilter>> m_IntermediaryFilters = {};

		bool m_FiltersInitialized = false;

		//void p_InitializeFilters();

		size_t p_ExtractFilterSamples(uint8_t** ByteBuffer, size_t FilterIndex, size_t NumberOfSamples, size_t OutputSampleOffset);
	public:
		virtual MBMedia::AudioParameters GetAudioParameters() override;
		virtual size_t GetNextSamples(uint8_t** DataBuffer, size_t NumberOfSamples,size_t OutputSampleOffset) override;
		virtual bool IsFinished() override;
		virtual ~AudioPipeline() override {};


		void AddFilter(std::unique_ptr<AudioStreamFilter> FilterToAdd);

		AudioPipeline(AudioStream* InitialStream);
		AudioPipeline(std::unique_ptr<AudioStream> InitialStream);

		friend std::unique_ptr<AudioPipeline> operator>>(std::unique_ptr<AudioPipeline> Pipeline,AudioStreamFilter* NewStream)
		{
			Pipeline->AddFilter(std::unique_ptr<AudioStreamFilter>(NewStream));
			return(std::move(Pipeline));
		}
		friend std::unique_ptr<AudioPipeline> operator>>(std::unique_ptr<AudioPipeline> Pipeline, AsyncrousAudioBuffer* NewBuffer)
		{
			NewBuffer->SetStreamToBuffer(std::move(Pipeline));
			std::unique_ptr<AudioPipeline> NewPipeline = std::unique_ptr<AudioPipeline>(new AudioPipeline(NewBuffer));
			return(NewPipeline);
		}
	};
	class AudioMixer : public AudioStream
	{
	private:
		//std::vector<std::vector<std::string>> m_StoredSamples = {};
		std::vector<MBMedia::AudioFIFOBuffer> m_StoredSamples = {};
		size_t m_StoredSamplesCount = 0;
		std::vector<std::unique_ptr<AudioStream>> m_InputSources = {};
		MBMedia::AudioParameters m_OutputParameters;

		std::vector<std::string> p_GetSourceData(size_t SourceIndex, size_t NumberOfSamples, size_t* OutRecievedSamples);
		void p_MixInputSources(std::vector<std::vector<std::string>> const& InputData, uint8_t** OutputData, size_t NumberOfSamples);
	public:
		void AddAudioSource(std::unique_ptr<AudioStream> NewAudioSource);
		void SetOutputParameters(MBMedia::AudioParameters const& NewParameters);

		virtual MBMedia::AudioParameters GetAudioParameters();
		virtual size_t GetNextSamples(uint8_t** DataBuffer, size_t NumberOfSamples);
		virtual bool IsFinished();
		virtual ~AudioMixer() override {};
	};
};