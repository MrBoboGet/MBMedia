#pragma once
#define NOMINMAX

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

	//utiltiy funktion f�r att verifiera att samplesen inte �r fucked, fungerar egentligen bara f�r floats eftersom 
	//PCM inte har n�got s�tt att avg�ra hur om den �r invalid. Kanske skulle kunna inkludera att skillnaden mellan 2 individuella
	//samples inte orsaker en "pop" s� att s�ga
	bool VerifySamples(const uint8_t* const* AudioData, AudioParameters const& DataParameters, size_t NumberOfFrames, size_t SampelsOffset = 0);


	//OBS F�ruts�tter att output datan �r har utrymmer den beh�ver. L�g niv� konvertering. Kanske borde l�gga till en Converssion context klass...
	void ConvertSampleData(const uint8_t** InputData, AudioParameters const& InputParameters, uint8_t** OutputBuffer, AudioParameters const& OutputParameters, size_t SamplesToConvert);

	class AudioFIFOBuffer
	{
	private:
		std::unique_ptr<void, void(*)(void*)> m_InternalData = std::unique_ptr<void*, void(*)(void*)>(nullptr, _DoNothing);

		bool m_IsInitialized = false;
		AudioParameters m_StoredAudioParameters;
		//void p_ResizeBuffers();
		//size_t p_GetChannelFrameSize();
		void Initialize(AudioParameters const& InputParameters, size_t InitialNumberOfSamples);
	public:
		AudioFIFOBuffer() {};
		AudioFIFOBuffer(AudioParameters const& InputParameters, size_t InitialNumberOfSamples);

		//void Initialize(AudioParameters const& InputParameters, size_t InitialNumberOfSamples);

		void InsertData(const uint8_t* const* AudioData, size_t NumberOfSamples);
		void InsertData(const uint8_t* const* AudioData, size_t NumberOfSamples, size_t InputSampleOffset);
		size_t ReadData(uint8_t* const* OutputBuffers, size_t NumberOfSamplesToRead);
		size_t ReadData(uint8_t* const* OutputBuffers, size_t NumberOfSamplesToRead, size_t OutputSampleOffset);

		//uint8_t* const* GetBuffer();
		//uint8_t const* const* GetBuffer() const;

		void DiscardSamples(size_t SamplesToDiscard);

		size_t AvailableSamples() const;
	};


	//Kommer nog reworka denna klass
	class AudioDataConverter : public AudioStreamFilter
	{
	private:
		std::unique_ptr<void, void (*)(void*)> m_ConversionContext = std::unique_ptr<void, void (*)(void*)>(nullptr, _DoNothing);
		AudioFIFOBuffer m_AudioBuffer;
		AudioParameters m_InputParameters;
		AudioParameters m_OutputParameters;

		bool m_Finished = false;
	public:
		AudioDataConverter(AudioParameters const& InputParameters, AudioParameters const& OutputParameters);
		//finns inte riktigt n�got s�tt att verifiera att input datan �r korrekt formatterad efter AudioParamters klassen, men �r den ej det �r det
		//undefined behaviour
		AudioDataConverter(AudioParameters const& OutputParameters);
		void InitializeInputParameters(AudioParameters const& InputParameters) override;
		AudioParameters GetAudioParameters() override;
		void InsertData(const uint8_t* const* DataToInsert, size_t NumberOfSamples,size_t InputSampleOffset) override;
		size_t AvailableSamples() override;//frames eller samples, vet inte riktigt �n...
		size_t GetNextSamples(uint8_t* const* OutputBuffer, size_t NumberOfSamples,size_t OutputSampleOffset) override;
		//void Flush() override;
		~AudioDataConverter() override {}
	};

	class AudioInputConverter : public AudioStream
	{
	private:
		std::unique_ptr<AudioStream> m_InternalStream = nullptr;
		AudioParameters m_OutputParameters;
		AudioDataConverter m_InternalConverter;
	public:
		AudioParameters GetAudioParameters() override;
		size_t GetNextSamples(uint8_t* const* OutputBuffer, size_t NumberOfSamples, size_t OutputSampleOffset) override;
		bool IsFinished() override;

		AudioInputConverter(std::unique_ptr<AudioStream> StreamToConvert, AudioParameters const& NewParameters);

		//void Flush() override;
		~AudioInputConverter() override {}
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
		//Read only f�rutom vid initialisering
		AudioParameters m_AudioParameters;
		//


		static constexpr size_t m_SamplesPerLoad = 4096;

		static void p_RetrieveAudioData(AsyncrousAudioBuffer* AssociatedObject);
	public:
		AsyncrousAudioBuffer();
		AsyncrousAudioBuffer(std::unique_ptr<AudioStream> m_StreamToBuffer);
		
		void SetStreamToBuffer(std::unique_ptr<AudioStream> m_StreamToBuffer);



		virtual MBMedia::AudioParameters GetAudioParameters() override;
		virtual size_t GetNextSamples(uint8_t* const* DataBuffer, size_t NumberOfSamples,size_t OutputSampleOffset) override;
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
		size_t m_SampleCapacity = 0;

		static constexpr double i_BufferGrowthSize = 1.5;
	public:
		AudioBuffer();
		
		AudioBuffer(AudioParameters const& AudioParameters, size_t NumberOfSamples);
		AudioBuffer(AudioBuffer const& BufferToCopy);
		AudioBuffer(AudioBuffer&& BufferToSteal) noexcept;
		AudioBuffer& operator=(AudioBuffer BufferToSteal);

		uint8_t* const* GetData();
		uint8_t const* const* GetData() const;
		size_t GetSamplesCount() const;
		size_t GetNumberOfPlanes() const;
		size_t GetPlaneSize() const;
		AudioParameters GetAudioParameters() const;

		size_t CopyData(uint8_t* const* OutputBuffer, size_t OutputOffset, size_t InputOffset,size_t SamplesToCopy) const;
		size_t CopyData(AudioBuffer& OutputBuffer, size_t OutputOffset, size_t InputOffset,size_t SamplesToCopy) const;

		void Resize(size_t TotalSize);
		void Reserve(size_t TotalSize);

		friend AudioBuffer operator+(AudioBuffer LeftBuffer, AudioBuffer const& RightBuffer);
		AudioBuffer& operator+=(AudioBuffer const& BufferToAdd);
		~AudioBuffer();
	};
	class AudioPipeline : public AudioStream
	{
	private:
		std::unique_ptr<AudioStream> m_InitialSource = nullptr;
		std::vector<std::unique_ptr<AudioStreamFilter>> m_IntermediaryFilters = {};

		bool m_FiltersInitialized = false;

		//void p_InitializeFilters();

		size_t p_ExtractFilterSamples(uint8_t* const* ByteBuffer, size_t FilterIndex, size_t NumberOfSamples, size_t OutputSampleOffset);
	public:
		virtual MBMedia::AudioParameters GetAudioParameters() override;
		virtual size_t GetNextSamples(uint8_t* const* DataBuffer, size_t NumberOfSamples,size_t OutputSampleOffset) override;
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
		size_t m_StoredSamplesCount = 0;
		std::vector<std::unique_ptr<AudioStream>> m_InputSources = {};
		std::vector<float> m_InputVolumes = {};
		MBMedia::AudioParameters m_OutputParameters;

		AudioBuffer p_GetSourceData(size_t SourceIndex, size_t NumberOfSamples, size_t* OutRecievedSamples);
		void p_MixInputSources(std::vector<AudioBuffer> const& InputData, uint8_t* const* OutputData, size_t NumberOfSamples,size_t OutputSampleOffset);
	public:
		void AddAudioSource(std::unique_ptr<AudioStream> NewAudioSource);
		void AddAudioSource(std::unique_ptr<AudioStream> NewAudioSource,float VolumeCoefficient);
		void SetOutputParameters(MBMedia::AudioParameters const& NewParameters);

		size_t GetNumberOfSources() const;
		AudioStream& GetAudioSource(size_t AudioSourceIndex);
		void RemoveIndex(size_t IndexToRemove); 

		virtual MBMedia::AudioParameters GetAudioParameters();
		virtual size_t GetNextSamples(uint8_t* const* DataBuffer, size_t NumberOfSamples,size_t BufferSampleOffset) override;
		virtual bool IsFinished();
		virtual ~AudioMixer() override {};
	};
};