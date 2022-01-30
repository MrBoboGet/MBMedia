#include "MBAudioDefinitions.h"
#include "MBAudioUtility.h"
#include <algorithm>
#include <assert.h>
#include <MBMediaDefinitions.h>
#include <algorithm>
#include <assert.h>
#include <numeric>
#include <cmath>

#include <cstring>
#include "MBMediaInternals.h"
namespace MBMedia
{

	//BEGIN AudioFIFOBuffer
	void AudioFIFOBuffer::Initialize(AudioParameters const& InputParameters, size_t InitialNumberOfSamples)
	{
		m_IsInitialized = true;
		m_InputFormatInfo = MBMedia::GetSampleFormatInfo(InputParameters.AudioFormat);
		m_InputParameters = InputParameters;
		m_InternalBuffers = std::vector<std::vector<uint8_t>>(GetParametersDataPlanes(InputParameters), std::vector<uint8_t>(InitialNumberOfSamples * m_InputFormatInfo.SampleSize, 0));
	}
	AudioFIFOBuffer::AudioFIFOBuffer(AudioParameters const& InputParameters, size_t InitialNumberOfSamples)
	{
		Initialize(InputParameters, InitialNumberOfSamples);
	}
	size_t AudioFIFOBuffer::p_GetChannelFrameSize()
	{
		if (m_InputFormatInfo.Interleaved)
		{
			return(m_InputFormatInfo.SampleSize * m_InputParameters.NumberOfChannels);
		}
		else
		{
			return(m_InputFormatInfo.SampleSize);
		}
	}
	uint8_t** AudioFIFOBuffer::GetBuffer()
	{
		if (m_IsInitialized == false)
		{
			throw std::exception();
		}
		size_t DataPlanesCount = GetParametersDataPlanes(m_InputParameters);
		if (m_DataPointers == nullptr)
		{
			//std::unique_ptr<uint8
			m_DataPointers = new uint8_t * [DataPlanesCount];
		}
		for (size_t i = 0; i < DataPlanesCount; i++)
		{
			m_DataPointers[i] = m_InternalBuffers[i].data() + m_CurrentBuffersOffset;
		}
		return(m_DataPointers);
	}
	const uint8_t* const* AudioFIFOBuffer::GetBuffer() const
	{
		if (m_IsInitialized == false)
		{
			throw std::exception();
		}
		size_t DataPlanesCount = GetParametersDataPlanes(m_InputParameters);
		if (m_DataPointers == nullptr)
		{
			m_DataPointers = new uint8_t * [DataPlanesCount];
		}
		for (size_t i = 0; i < DataPlanesCount; i++)
		{
			m_DataPointers[i] = (uint8_t*)m_InternalBuffers[i].data() + m_CurrentBuffersOffset;
		}
		return(m_DataPointers);
	}
	void AudioFIFOBuffer::InsertData(const uint8_t* const* AudioData, size_t NumberOfSamples)
	{
		InsertData(AudioData, NumberOfSamples, 0);
	}
	void AudioFIFOBuffer::InsertData(const uint8_t* const* AudioData, size_t NumberOfSamples, size_t InputSampleOffset)
	{
		if (!m_IsInitialized)
		{
			throw std::exception();
		}
#ifdef MBMEDIA_VERIFY_AUDIO_DATA
		assert(VerifySamples(AudioData, m_InputParameters, NumberOfSamples, InputSampleOffset));
#endif
		size_t NewSamplesSize = NumberOfSamples * p_GetChannelFrameSize();
		size_t StoredSamplesByteOffset = (m_StoredSamples * p_GetChannelFrameSize()) + m_CurrentBuffersOffset;
		for (size_t i = 0; i < m_InternalBuffers.size(); i++)
		{
			if (m_InternalBuffers[i].size() - StoredSamplesByteOffset < NewSamplesSize)
			{
				m_InternalBuffers[i].resize(m_InternalBuffers[i].size() * double(m_GrowthFactor) + NewSamplesSize, 0);
			}
			std::memcpy(m_InternalBuffers[i].data() + StoredSamplesByteOffset, AudioData[i] + (InputSampleOffset * p_GetChannelFrameSize()), NewSamplesSize);
		}
		m_StoredSamples += NumberOfSamples;
	}
	size_t AudioFIFOBuffer::ReadData(uint8_t** OutputBuffers, size_t NumberOfSamplesToRead)
	{
		return(ReadData(OutputBuffers, NumberOfSamplesToRead, 0));
	}
	size_t AudioFIFOBuffer::ReadData(uint8_t** OutputBuffers, size_t NumberOfSamplesToRead, size_t OutputSampleOffset)
	{
		if (!m_IsInitialized)
		{
			throw std::exception();
		}
		size_t SamplesToExtract = std::min(m_StoredSamples, NumberOfSamplesToRead);
		for (size_t i = 0; i < m_InternalBuffers.size(); i++)
		{
			std::memcpy(OutputBuffers[i] + OutputSampleOffset * p_GetChannelFrameSize(), m_InternalBuffers[i].data() + m_CurrentBuffersOffset, SamplesToExtract * p_GetChannelFrameSize());
		}
		m_StoredSamples -= SamplesToExtract;
		m_CurrentBuffersOffset += SamplesToExtract * p_GetChannelFrameSize();
		p_ResizeBuffers();
#ifdef MBMEDIA_VERIFY_AUDIO_DATA
		assert(VerifySamples(OutputBuffers, m_InputParameters, SamplesToExtract, OutputSampleOffset));
#endif
		return(SamplesToExtract);
	}
	void AudioFIFOBuffer::DiscardSamples(size_t SamplesToDiscard)
	{
		assert(GetParametersDataPlanes(m_InputParameters) == m_InternalBuffers.size());
		if (SamplesToDiscard >= m_StoredSamples)
		{
			for (size_t i = 0; i < GetParametersDataPlanes(m_InputParameters); i++)
			{
				m_InternalBuffers[i].resize(0);
			}
			m_StoredSamples = 0;
			m_CurrentBuffersOffset = 0;
		}
		else
		{
			m_StoredSamples -= SamplesToDiscard;
			m_CurrentBuffersOffset += p_GetChannelFrameSize() * SamplesToDiscard;
			p_ResizeBuffers();
		}
	}
	void AudioFIFOBuffer::p_ResizeBuffers()
	{
		//TODO använder godtycklig heuristic, kanske vill antingen stora dem som en linked lista eller faktiskt undersöka hur man ska göra?
		if (m_CurrentBuffersOffset >= m_InternalBuffers[0].size() / 4)
		{
			std::vector<std::vector<uint8_t>> NewBuffers = std::vector<std::vector<uint8_t>>(m_InternalBuffers.size(), std::vector<uint8_t>(m_StoredSamples * p_GetChannelFrameSize() * 2, 0));
			for (size_t i = 0; i < NewBuffers.size(); i++)
			{
				std::memcpy(NewBuffers[i].data(), m_InternalBuffers[i].data(), m_StoredSamples * p_GetChannelFrameSize());
			}
		}
	}
	size_t AudioFIFOBuffer::AvailableSamples()
	{
		if (!m_IsInitialized)
		{
			throw std::exception();
		}
		return(m_StoredSamples);
	}
	//END AudioFIFOBuffer



	//BEGIN AudioMixer
	void AudioMixer::AddAudioSource(std::unique_ptr<AudioStream> NewAudioSource)
	{
		//m_StoredSamples.push_back(std::vector<std::string>(NewAudioSource->GetAudioParameters().NumberOfChannels, std::string()));
		//m_StoredSamples.push_back(MBMedia::AudioFIFOBuffer(NewAudioSource->GetAudioParameters(), 4096));//totalt godtycklig
		if (NewAudioSource->GetAudioParameters() != m_OutputParameters)
		{
			//NewAudioSource = std::unique_ptr<AudioStream>(new AudioDataConverter( std::move()
			NewAudioSource = std::unique_ptr<AudioStream>(new AudioInputConverter(std::move(NewAudioSource), m_OutputParameters));
		}
		m_InputSources.push_back(std::move(NewAudioSource));
	}
	void AudioMixer::SetOutputParameters(MBMedia::AudioParameters const& NewParameters)
	{
		m_OutputParameters = NewParameters;
	}
	MBMedia::AudioParameters AudioMixer::GetAudioParameters()
	{
		return(m_OutputParameters);
	}
	size_t AudioMixer::GetNumberOfSources() const
	{
		return(m_InputSources.size());
	}
	AudioStream& AudioMixer::GetAudioSource(size_t AudioSourceIndex)
	{
		if (AudioSourceIndex >= m_InputSources.size())
		{
			throw std::runtime_error("AudioSourceIndex out of range!");
		}
		return(*m_InputSources[AudioSourceIndex]);
	}
	void AudioMixer::RemoveIndex(size_t IndexToRemove)
	{
		if (IndexToRemove >= m_InputSources.size())
		{
			throw std::runtime_error("AudioSourceIndex out of range!");
		}
		//TODO kan optimeras, själva interfacen med
		m_InputSources.erase(m_InputSources.begin() + IndexToRemove);
		//m_AudioConverters.erase(m_AudioConverters.begin() + IndexToRemove);
	}
	AudioBuffer AudioMixer::p_GetSourceData(size_t SourceIndex, size_t NumberOfSamples, size_t* OutRecievedSamples)
	{
		AudioBuffer ReturnValue = AudioBuffer(m_OutputParameters, NumberOfSamples);
		size_t RecievedSamples = m_InputSources[SourceIndex]->GetNextSamples(ReturnValue.GetData(), NumberOfSamples, 0);
		//TODO kanske redundnat, inte säker på om det ska krävas att man alltid har 0:at resten av inputen...
		if (RecievedSamples < NumberOfSamples)
		{
			for (size_t i = 0; i < GetParametersDataPlanes(m_OutputParameters); i++)
			{
				size_t ByteOffset = RecievedSamples * GetChannelFrameSize(m_OutputParameters);
				size_t TotalSize = NumberOfSamples * GetChannelFrameSize(m_OutputParameters);
				std::memset(ReturnValue.GetData()[i] + ByteOffset, 0, TotalSize - ByteOffset);
			}
		}
		*OutRecievedSamples = RecievedSamples;
		return(ReturnValue);
	}
	size_t AudioMixer::GetNextSamples(uint8_t** DataBuffer, size_t NumberOfSamples,size_t BufferSampleOffset)
	{
		std::vector<AudioBuffer> TotalInputData = {};

		size_t MaxRecievedSamples = 0;
		for (size_t i = 0; i < m_InputSources.size(); i++)
		{
			if (m_InputSources[i]->IsFinished())
			{
				continue;
			}
			size_t RecievedSamples = 0;
			TotalInputData.push_back(p_GetSourceData(i, NumberOfSamples, &RecievedSamples));
			if (RecievedSamples > MaxRecievedSamples)
			{
				MaxRecievedSamples = RecievedSamples;
			}
		}
		if (TotalInputData.size() == 0)
		{
			//int fått någon data, skriver bara 0
			for (size_t i = 0; i < MBMedia::GetParametersDataPlanes(m_OutputParameters); i++)
			{
				std::memset(((uint8_t*)DataBuffer[i])+MBMedia::GetChannelFrameSize(m_OutputParameters)*BufferSampleOffset, 0, MBMedia::GetChannelFrameSize(m_OutputParameters) * NumberOfSamples);
			}
			return(0);
		}
		p_MixInputSources(TotalInputData, DataBuffer, NumberOfSamples,BufferSampleOffset);
#ifdef MBMEDIA_VERIFY_AUDIO_DATA
		assert(MBMedia::VerifySamples(DataBuffer, GetAudioParameters(), MaxRecievedSamples, BufferSampleOffset));
#endif // MBAE_VERIFY_AUDIO_DATA
		//float* TestPointer = (float*)TotalInputData[0][0].data();
		return(MaxRecievedSamples);
	}
	template<typename T> 
	void h_AddBuffers(const void** InBuffers_, void* OutBuffer_,size_t NumberOfBuffers,size_t BufferLength)
	{
		const T* const* InBuffers = (const T* const*)InBuffers_;
		T* OutBuffer = (T*)OutBuffer_;

		for (size_t i = 0; i < BufferLength; i++)
		{
			T Result = 0;
			for(size_t j = 0; j < NumberOfBuffers;j++)
			{
				Result += InBuffers[j][i];
			}
			OutBuffer[i] = Result;
		}
	}

	void AudioMixer::p_MixInputSources(std::vector<AudioBuffer> const& InputData, uint8_t** OutputData, size_t NumberOfSamples,size_t OutputSampleOffset)
	{
		if (InputData.size() == 0)
		{
			return;
		}
#ifdef MBMEDIA_VERIFY_AUDIO_DATA
		size_t FirstBufferSize = InputData[0].GetSamplesCount();
		for (size_t i = 1; i < InputData.size(); i++)
		{
			assert(InputData[i].GetSamplesCount() == FirstBufferSize);
		}
#endif // MBMEDIA_VERIFY_AUDIO_DATA
		
		//for (size_t i = 0; i < MBMedia::GetParametersDataPlanes(m_OutputParameters); i++)
		//{
		//	std::memcpy(OutputData[i]+ MBMedia::GetChannelFrameSize(m_OutputParameters)*OutputSampleOffset, InputData[0][i].data(), NumberOfSamples * MBMedia::GetChannelFrameSize(m_OutputParameters));
		//}

		size_t SamplesPerPlane = NumberOfSamples;
		if (!MBMedia::FormatIsPlanar(m_OutputParameters.AudioFormat))
		{
			SamplesPerPlane *= m_OutputParameters.NumberOfChannels;
		}
		for (size_t i = 0; i < MBMedia::GetParametersDataPlanes(m_OutputParameters); i++)
		{
			const uint8_t** DataPlanes =(const uint8_t **) new uint8_t * [InputData.size()];
			for (size_t j = 0; j < InputData.size(); j++)
			{
				DataPlanes[j] = InputData[j].GetData()[i];
			}
			if (m_OutputParameters.AudioFormat == SampleFormat::DBL || m_OutputParameters.AudioFormat == SampleFormat::DBLP)
			{
				h_AddBuffers<double>((const void**)DataPlanes, (void*)(OutputData[i]+GetChannelFrameSize(m_OutputParameters) * OutputSampleOffset), InputData.size(), SamplesPerPlane);
			}
			else if (m_OutputParameters.AudioFormat == SampleFormat::FLTP || m_OutputParameters.AudioFormat == SampleFormat::FLT)
			{
				h_AddBuffers<float>((const void**)DataPlanes, (void*)(OutputData[i] + GetChannelFrameSize(m_OutputParameters) * OutputSampleOffset), InputData.size(), SamplesPerPlane);
			}
			else if (m_OutputParameters.AudioFormat == SampleFormat::S16 || m_OutputParameters.AudioFormat == SampleFormat::S16)
			{
				h_AddBuffers<int16_t>((const void**)DataPlanes, (void*)(OutputData[i] + GetChannelFrameSize(m_OutputParameters) * OutputSampleOffset), InputData.size(), SamplesPerPlane);
			}
			else if (m_OutputParameters.AudioFormat == SampleFormat::S32 || m_OutputParameters.AudioFormat == SampleFormat::S32)
			{
				h_AddBuffers<int32_t>((const void**)DataPlanes, (void*)(OutputData[i] + GetChannelFrameSize(m_OutputParameters) * OutputSampleOffset), InputData.size(), SamplesPerPlane);
			}
			else if (m_OutputParameters.AudioFormat == SampleFormat::U8 || m_OutputParameters.AudioFormat == SampleFormat::U8)
			{
				h_AddBuffers<uint8_t>((const void**)DataPlanes, (void*)(OutputData[i] + GetChannelFrameSize(m_OutputParameters) * OutputSampleOffset), InputData.size(), SamplesPerPlane);
			}
			delete[] DataPlanes;
		}


#ifdef MBMEDIA_VERIFY_AUDIO_DATA
		assert(VerifySamples(OutputData, m_OutputParameters, NumberOfSamples));
#endif // MBMEDIA_VERIFY_AUDIO_DATA
	}
	bool AudioMixer::IsFinished()
	{
		bool Finished = true;
		for (size_t i = 0; i < m_InputSources.size(); i++)
		{
			if (m_InputSources[i]->IsFinished() == false)
			{
				Finished = false;
				break;
			}
		}
		return(Finished);
	}
	//END AudioMixer

	//BEGIN AudioDataConverter
	AudioDataConverter::AudioDataConverter(AudioParameters const& InputParameters, AudioParameters const& OutputParameters)
	{
		m_InputParameters = InputParameters;
		m_OutputParameters = OutputParameters;
		if (int64_t(InputParameters.Layout) == 0 && InputParameters.NumberOfChannels == 1)
		{
			m_InputParameters.Layout = h_FFMPEGLayoutToMBLayout(AV_CH_LAYOUT_MONO);
		}
		SwrContext* ConversionContext = swr_alloc_set_opts(NULL,
			h_MBLayoutToFFMPEGLayout(m_OutputParameters.Layout),
			h_MBSampleFormatToFFMPEGSampleFormat(m_OutputParameters.AudioFormat),
			m_OutputParameters.SampleRate,
			h_MBLayoutToFFMPEGLayout(m_InputParameters.Layout),
			h_MBSampleFormatToFFMPEGSampleFormat(m_InputParameters.AudioFormat),
			m_InputParameters.SampleRate,
			0,
			NULL);
		FFMPEGCall(swr_init(ConversionContext));
		m_AudioBuffer = AudioFIFOBuffer(InputParameters, 2048 * 4);//helt godtyckligt
		m_ConversionContext = std::unique_ptr<void, void (*)(void*)>(ConversionContext, _FreeSwrContext);
	}
	AudioDataConverter::AudioDataConverter(AudioParameters const& OutputParameters)
	{
		m_OutputParameters = OutputParameters;
	}
	void AudioDataConverter::InitializeInputParameters(AudioParameters const& InputParameters)
	{
		m_InputParameters = InputParameters;
		if (int64_t(InputParameters.Layout) == 0 && InputParameters.NumberOfChannels == 1)
		{
			m_InputParameters.Layout = h_FFMPEGLayoutToMBLayout(AV_CH_LAYOUT_MONO);
		}
		SwrContext* ConversionContext = swr_alloc_set_opts(NULL,
			h_MBLayoutToFFMPEGLayout(m_OutputParameters.Layout),
			h_MBSampleFormatToFFMPEGSampleFormat(m_OutputParameters.AudioFormat),
			m_OutputParameters.SampleRate,
			h_MBLayoutToFFMPEGLayout(m_InputParameters.Layout),
			h_MBSampleFormatToFFMPEGSampleFormat(m_InputParameters.AudioFormat),
			m_InputParameters.SampleRate,
			0,
			NULL);
		FFMPEGCall(swr_init(ConversionContext));
		m_AudioBuffer = AudioFIFOBuffer(InputParameters, 2048 * 4);//helt godtyckligt
		m_ConversionContext = std::unique_ptr<void, void (*)(void*)>(ConversionContext, _FreeSwrContext);
	}
	AudioParameters AudioDataConverter::GetAudioParameters()
	{
		return(m_OutputParameters);
	}
	void AudioDataConverter::InsertData(const uint8_t* const* DataToInsert, size_t NumberOfSamples,size_t InputSamplesOffset)
	{
#ifdef MBMEDIA_VERIFY_AUDIO_DATA
		assert(VerifySamples(DataToInsert, m_InputParameters, NumberOfSamples, 0));
#endif
		m_AudioBuffer.InsertData(DataToInsert, NumberOfSamples, InputSamplesOffset);
	}
	size_t AudioDataConverter::AvailableSamples()
	{
		SwrContext* ConversionContext = (SwrContext*)m_ConversionContext.get();
		int ReturnValue = swr_get_out_samples(ConversionContext, m_AudioBuffer.AvailableSamples());
		FFMPEGCall(ReturnValue);
		if (ReturnValue < 0)
		{
			return(-1);
		}
		else
		{
			return(ReturnValue);
		}
	}
	//void AudioDataConverter::Flush()
	//{
	//	//TODO implementera xd
	//}
	size_t AudioDataConverter::GetNextSamples(uint8_t** OutputBuffer, size_t NumberOfSamples,size_t OutputSampleOffset)
	{
		SwrContext* ConversionContext = (SwrContext*)m_ConversionContext.get();
		size_t ConvertedSamples = -1;
		int Result = -1;
		std::vector<uint8_t*> OutputPointers = std::vector<uint8_t*>(GetParametersDataPlanes(m_OutputParameters), nullptr);
		for (size_t i = 0; i < GetParametersDataPlanes(m_OutputParameters); i++)
		{
			OutputPointers[i] = OutputBuffer[i] + (GetChannelFrameSize(m_OutputParameters) * OutputSampleOffset);
		}
		if (m_AudioBuffer.AvailableSamples() > 0)
		{
			Result = swr_convert(ConversionContext, OutputPointers.data(), NumberOfSamples, (const uint8_t**)m_AudioBuffer.GetBuffer(), m_AudioBuffer.AvailableSamples());
		}
		else
		{
			Result = swr_convert(ConversionContext, OutputPointers.data(), NumberOfSamples, nullptr, 0);
		}
		m_AudioBuffer.DiscardSamples(m_AudioBuffer.AvailableSamples());
		FFMPEGCall(Result);
		if (Result < 0)
		{
			return(-1);
		}
		else
		{
#ifdef MBMEDIA_VERIFY_AUDIO_DATA
			assert(VerifySamples(OutputBuffer, m_OutputParameters, Result, 0));
#endif
			return(Result);
		}
	}
	//END AudioDataConverter

	//BEGIN AudioInputConverter
	AudioParameters AudioInputConverter::GetAudioParameters() 
	{
		return(m_OutputParameters);
	}
	size_t AudioInputConverter::GetNextSamples(uint8_t** OutputBuffer, size_t NumberOfSamples, size_t OutputSampleOffset)
	{
		const size_t DataFetchChunk = 4096;
		AudioBuffer TempBuffer = AudioBuffer(m_InternalStream->GetAudioParameters(), DataFetchChunk);
		size_t FetchedSamples = 0;
		while (FetchedSamples < NumberOfSamples)
		{
			if(!m_InternalStream->IsFinished())
			{
				size_t InputStreamSamples = m_InternalStream->GetNextSamples(TempBuffer.GetData(), DataFetchChunk, 0);
				m_InternalConverter.InsertData(TempBuffer.GetData(), InputStreamSamples, 0);
			}
			size_t SamplesToFetch = std::min(m_InternalConverter.AvailableSamples(), NumberOfSamples - FetchedSamples);
			if (SamplesToFetch == 0)
			{
				break;
			}
			FetchedSamples += m_InternalConverter.GetNextSamples(OutputBuffer, SamplesToFetch, FetchedSamples + OutputSampleOffset);
		}
		return(FetchedSamples);
	}
	bool AudioInputConverter::IsFinished()
	{
		return(m_InternalConverter.AvailableSamples() == 0 && m_InternalStream->IsFinished());
	}

	AudioInputConverter::AudioInputConverter(std::unique_ptr<AudioStream> StreamToConvert, AudioParameters const& NewParameters)
		: m_InternalConverter(NewParameters)
	{
		m_InternalStream = std::move(StreamToConvert);
		m_InternalConverter.InitializeInputParameters(m_InternalStream->GetAudioParameters());
	}

	//END AudioInputConverter




	//BEGIN AsyncrousAudioBuffer 
	AsyncrousAudioBuffer::AsyncrousAudioBuffer()
	{

	}
	AsyncrousAudioBuffer::AsyncrousAudioBuffer(std::unique_ptr<AudioStream> StreamToBuffer)
	{
		m_Initialised = true;
		m_StreamToBuffer = std::move(StreamToBuffer);
		m_AudioBuffer = MBMedia::AudioFIFOBuffer(m_StreamToBuffer->GetAudioParameters(), 4096);//helt godtyckligt
		m_RetrieverThread = std::thread(AsyncrousAudioBuffer::p_RetrieveAudioData, this);
		
	}
	void AsyncrousAudioBuffer::SetStreamToBuffer(std::unique_ptr<AudioStream> StreamToBuffer)
	{
		if (m_Initialised)
		{
			throw std::exception();
		}
		m_Initialised = true;
		m_StreamToBuffer = std::move(StreamToBuffer);
		m_AudioBuffer = MBMedia::AudioFIFOBuffer(m_StreamToBuffer->GetAudioParameters(), 4096);//helt godtyckligt
	}
	MBMedia::AudioParameters AsyncrousAudioBuffer::GetAudioParameters()
	{
		if (m_StreamToBuffer == nullptr)
		{
			throw std::exception();
		}
		return(m_StreamToBuffer->GetAudioParameters());
	}
	void AsyncrousAudioBuffer::p_RetrieveAudioData(AsyncrousAudioBuffer* AssociatedObject)
	{
		while (AssociatedObject->m_Finishing.load())
		{
			//uint8_t** TempBuffer = AllocateAudioBuffer(AssociatedObject->m_AudioParameters, m_SamplesPerLoad);
			AudioBuffer TempBuffer = AudioBuffer(AssociatedObject->m_AudioParameters, m_SamplesPerLoad);
			size_t ExtractedSamples = AssociatedObject->GetNextSamples(TempBuffer.GetData(), m_SamplesPerLoad,0);
			size_t StoredSamples = 0;
			{
				std::lock_guard<std::mutex> Lock(AssociatedObject->m_AudioBufferMutex);
				AssociatedObject->m_AudioBuffer.InsertData(TempBuffer.GetData(), ExtractedSamples);
				StoredSamples = AssociatedObject->m_AudioBuffer.AvailableSamples();
			}
			//DeallocateAudioBuffer(AssociatedObject->m_AudioParameters, TempBuffer);
			if (ExtractedSamples < m_SamplesPerLoad || AssociatedObject->m_StreamToBuffer->IsFinished())
			{
				break;
			}
			
			if (StoredSamples / double(AssociatedObject->m_AudioParameters.SampleRate) > AssociatedObject->m_TargetBufferSize.load())
			{
				std::unique_lock<std::mutex> Lock(AssociatedObject->m_BufferFullMutex);
				AssociatedObject->m_BufferFullConditional.wait(Lock);
			}
		}
		AssociatedObject->m_StreamFinished.store(true);
	}
	size_t AsyncrousAudioBuffer::GetNextSamples(uint8_t** DataBuffer, size_t NumberOfSamples,size_t OutputSampleoffset)
	{
		//returnar alltid alla samples även om dem inte finns sparade, kanske borde vänta vem vet?
		size_t ReturnValue = 0;
		size_t ExtractedSamples = 0;
		{
			std::lock_guard<std::mutex> Lock(m_AudioBufferMutex);
			ExtractedSamples = m_AudioBuffer.ReadData(DataBuffer,NumberOfSamples,OutputSampleoffset);
		}
		for (size_t i = 0; i < GetParametersDataPlanes(m_AudioParameters); i++)
		{
			size_t BytesToZero = (NumberOfSamples - ExtractedSamples) * GetChannelFrameSize(m_AudioParameters);
			size_t BytesOffset = ExtractedSamples * GetChannelFrameSize(m_AudioParameters);
			std::memset(DataBuffer[i]+BytesOffset, 0, BytesToZero);
		}
		if (!m_StreamFinished.load())
		{
			ReturnValue = NumberOfSamples;
		}
		else
		{
			ReturnValue = ExtractedSamples;
		}
		m_BufferFullConditional.notify_all();
		return(ExtractedSamples);
	}
	bool AsyncrousAudioBuffer::IsFinished()
	{
		bool BytesInBuffer = false;
		{
			std::lock_guard<std::mutex> Lock(m_AudioBufferMutex);
			BytesInBuffer = m_AudioBuffer.AvailableSamples() != 0;
		}
		return(!BytesInBuffer && m_StreamFinished.load());
	}
	AsyncrousAudioBuffer::~AsyncrousAudioBuffer()
	{
		m_Finishing.store(true);
		m_RetrieverThread.join();
	}
	//END AsyncrousAudioBuffer 

	//BEGIN AudioBuffer
	void swap(AudioBuffer& LeftBuffer, AudioBuffer& RightBuffer)
	{
		std::swap(LeftBuffer.m_AudioParameters, RightBuffer.m_AudioParameters);
		std::swap(LeftBuffer.m_InternalBuffer, RightBuffer.m_InternalBuffer);
		std::swap(LeftBuffer.m_StoredSamples, RightBuffer.m_StoredSamples);
	}
	AudioBuffer::AudioBuffer()
	{

	}

	AudioBuffer::AudioBuffer(AudioParameters const& AudioParameters, size_t NumberOfSamples)
	{
		m_AudioParameters = AudioParameters;
		m_StoredSamples = NumberOfSamples;
		m_InternalBuffer = new uint8_t * [GetParametersDataPlanes(AudioParameters)];
		size_t PlaneSize = GetChannelFrameSize(AudioParameters) * NumberOfSamples;
		for (size_t i = 0; i < GetParametersDataPlanes(AudioParameters) ; i++)
		{
			m_InternalBuffer[i] = new uint8_t[PlaneSize];
		}
	}
	AudioBuffer::AudioBuffer(AudioBuffer const& BufferToCopy)
	{
		m_AudioParameters = BufferToCopy.m_AudioParameters;
		m_StoredSamples = BufferToCopy.m_StoredSamples;
		m_InternalBuffer = new uint8_t * [GetParametersDataPlanes(m_AudioParameters)];
		size_t PlaneSize = GetChannelFrameSize(m_AudioParameters) * m_StoredSamples;
		for (size_t i = 0; i < GetParametersDataPlanes(m_AudioParameters); i++)
		{
			m_InternalBuffer[i] = new uint8_t[PlaneSize];
			std::memcpy(m_InternalBuffer[i], BufferToCopy.m_InternalBuffer[i], PlaneSize);
		}
	}
	AudioBuffer::AudioBuffer(AudioBuffer&& BufferToSteal) noexcept
	{
		swap(*this, BufferToSteal);
	}
	AudioBuffer& AudioBuffer::operator=(AudioBuffer BufferToSteal)
	{
		swap(*this, BufferToSteal);
		return(*this);
	}

	uint8_t** AudioBuffer::GetData()
	{
		return(m_InternalBuffer);
	}
	const uint8_t* const* AudioBuffer::GetData() const
	{
		return(m_InternalBuffer);
	}
	size_t AudioBuffer::GetSamplesCount() const
	{
		return(m_StoredSamples);
	}
	size_t AudioBuffer::GetNumberOfPlanes() const
	{
		return(GetParametersDataPlanes(m_AudioParameters));
	}
	size_t AudioBuffer::GetPlaneSize() const
	{
		return(GetChannelFrameSize(m_AudioParameters));
	}
	AudioBuffer::~AudioBuffer()
	{
		if (m_InternalBuffer != nullptr)
		{
			for (size_t i = 0; i < GetParametersDataPlanes(m_AudioParameters); i++)
			{
				delete[] m_InternalBuffer[i];
			}
		}
		delete[] m_InternalBuffer;
	}
	//END AudioBuffer


	//BEGIN AudioPipeline
	//void AudioPipeline::p_InitializeFilters()
	//{
	//	if (m_InitialSource == nullptr)
	//	{
	//		throw std::exception();
	//	}
	//	AudioParameters PreviousParameters = m_InitialSource->GetAudioParameters();
	//	for (size_t i = 0; i < m_IntermediaryFilters.size(); i++)
	//	{
	//		m_IntermediaryFilters[i]->InitializeInputParameters(PreviousParameters);
	//		PreviousParameters = m_IntermediaryFilters[i]->GetAudioParameters();
	//	}
	//}
	MBMedia::AudioParameters AudioPipeline::GetAudioParameters()
	{
		if (m_IntermediaryFilters.size() > 0)
		{
			return(m_IntermediaryFilters.back()->GetAudioParameters());
		}
		else
		{
			if (m_InitialSource != nullptr)
			{
				return(m_InitialSource->GetAudioParameters());
			}
			else
			{
				throw std::exception();
			}
		}
	}
	size_t AudioPipeline::p_ExtractFilterSamples(uint8_t** ByteBuffer, size_t FilterIndex, size_t NumberOfSamples,size_t OutputSampleOffset)
	{
		if (FilterIndex == -1)
		{
			size_t ReturnValue = m_InitialSource->GetNextSamples(ByteBuffer, NumberOfSamples, OutputSampleOffset);
#ifdef MBMEDIA_VERIFY_AUDIO_DATA
			assert(VerifySamples(ByteBuffer, m_InitialSource->GetAudioParameters(), ReturnValue, OutputSampleOffset));
#endif // 

			return(ReturnValue);
		}
		AudioParameters InputParameters;
		if (FilterIndex != 0)
		{
			InputParameters = m_IntermediaryFilters[FilterIndex - 1]->GetAudioParameters();
		}
		else
		{
			InputParameters = m_InitialSource->GetAudioParameters();
		}
		const size_t DataRequestSize = 4096;
		AudioBuffer IntermediaryBuffer = AudioBuffer(InputParameters, 4096);
		size_t ExtractedSamples = 0;
		size_t CurrentOutputOffset = 0;
		bool FiltersFinished = false;
		while (!IsFinished() && ExtractedSamples < NumberOfSamples) 
		{
			if (m_IntermediaryFilters[FilterIndex]->AvailableSamples() > 0)
			{
				size_t RequestSize = std::min(NumberOfSamples - ExtractedSamples, m_IntermediaryFilters[FilterIndex]->AvailableSamples());
				size_t NewSamples = m_IntermediaryFilters[FilterIndex]->GetNextSamples(ByteBuffer, RequestSize, ExtractedSamples);
				ExtractedSamples += NewSamples;
			}
			else
			{
				if (FiltersFinished)
				{
					break;	
				}
				size_t IntermediaryExtractedSamples = p_ExtractFilterSamples(IntermediaryBuffer.GetData(), FilterIndex - 1, DataRequestSize, 0);
				if (IntermediaryExtractedSamples < DataRequestSize)
				{
					FiltersFinished = true;
				}
				m_IntermediaryFilters[FilterIndex]->InsertData(IntermediaryBuffer.GetData(),IntermediaryExtractedSamples,0);
			}
		}
#ifdef MBMEDIA_VERIFY_AUDIO_DATA
		assert(VerifySamples(ByteBuffer, m_IntermediaryFilters[FilterIndex]->GetAudioParameters(), ExtractedSamples, OutputSampleOffset));
#endif // 
		return(ExtractedSamples);
	}
	size_t AudioPipeline::GetNextSamples(uint8_t** DataBuffer, size_t NumberOfSamples,size_t OutputSampleOffseet)
	{
		size_t ExtractedSamples = 0;
		if (m_IntermediaryFilters.size() == 0)
		{
			ExtractedSamples = m_InitialSource->GetNextSamples(DataBuffer, NumberOfSamples,OutputSampleOffseet);
		}
		else
		{
			ExtractedSamples = p_ExtractFilterSamples(DataBuffer, m_IntermediaryFilters.size()-1, NumberOfSamples, OutputSampleOffseet);
		}
#ifdef MBMEDIA_VERIFY_AUDIO_DATA
		assert(VerifySamples(DataBuffer, GetAudioParameters(), ExtractedSamples, OutputSampleOffseet));
#endif // MBMEDIA_VERIFY_AUDIO_DATA

		return(ExtractedSamples);
	}
	bool AudioPipeline::IsFinished()
	{
		bool ReturnValue = m_InitialSource->IsFinished();
		for (size_t i = 0; i < m_IntermediaryFilters.size(); i++)
		{
			if (m_IntermediaryFilters[i]->AvailableSamples() > 0)
			{
				ReturnValue = false;
				break;
			}
		}
		return(ReturnValue);
	}
	void AudioPipeline::AddFilter(std::unique_ptr<AudioStreamFilter> FilterToAdd)
	{
		m_IntermediaryFilters.push_back(std::move(FilterToAdd));
		if (m_IntermediaryFilters.size() == 1)
		{
			m_IntermediaryFilters.back()->InitializeInputParameters(m_InitialSource->GetAudioParameters());
		}
		else
		{
			m_IntermediaryFilters.back()->InitializeInputParameters(m_IntermediaryFilters[m_IntermediaryFilters.size() - 2]->GetAudioParameters());
		}
	}
	AudioPipeline::AudioPipeline(AudioStream* InitialStream)
	{
		m_InitialSource = std::unique_ptr<AudioStream>(InitialStream);
	}
	AudioPipeline::AudioPipeline(std::unique_ptr<AudioStream> InitialStream)
	{
		m_InitialSource = std::move(InitialStream);
	}
	//END AudioPipeline
};