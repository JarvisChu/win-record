#include <stdio.h>
#include "record.h"
#include <chrono>
#include <thread>
#include <Windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <time.h>
#include <sstream>
#include <excpt.h>

using namespace std;

#pragma comment(lib, "winmm.lib")

const char* EVT_IN_START = "in_start";
const char* EVT_OUT_START = "out_start";
const char* EVT_IN_STOP = "in_stop";
const char* EVT_OUT_STOP = "out_stop";
const char* EVT_IN_AUDIO = "in_audio";
const char* EVT_OUT_AUDIO = "out_audio";
const char* EVT_ERROR = "error";


// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres, msg) { \
	if (FAILED(hres)) { \
		bHasError = true; \
		std::ostringstream stream; \
		stream << ", hr=0x" << hex << hres; \
		strErrMsg = msg + stream.str(); \
		if(bCoInitializeExSucc && !bCrashed){ \
			if(pwfx) CoTaskMemFree(pwfx); \
			SAFE_RELEASE(pEnumerator) \
			SAFE_RELEASE(pDevice) \
			SAFE_RELEASE(pAudioClient) \
			SAFE_RELEASE(pCaptureClient) \
			CoUninitialize(); \
		} \
		goto Exit; \
	} \
}

#define SAFE_RELEASE(punk) { if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; } }
                
const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

void OnSend(uv_async_t *async){
	Record* record = (Record*) async->data;
	record->HandleSend();
}

void OnClose(uv_handle_t* handle) {
	printf("[%ld]OnClose\n", GetCurrentThreadId());
	uv_async_t* m_async = (uv_async_t*) handle;
	if(m_async) {
		delete m_async;
		m_async = nullptr;
	}
}

void RunThreadI(void* arg) {
	Record* record = (Record*) arg;
	record->Run(Wave_In);
}

void RunThreadO(void* arg) {
	Record* record = (Record*) arg;
	record->Run(Wave_Out);
}

std::string Record::GetCurrentTime(){
	time_t t = time(NULL);
	char ch[64] = { 0 };
	strftime(ch, sizeof(ch) - 1, "%Y_%m_%d_%H_%M_%S", localtime(&t));
	return std::string(ch);
}

Record::Record(const Napi::CallbackInfo& info): Napi::ObjectWrap<Record>(info) {
	Napi::Env env = info.Env();

	printf("[%ld]Record::Record\n", GetCurrentThreadId());

	if(info.Length() < 5 ){
		Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
		return;
	}

	if(!info[1].IsString() || !info[2].IsNumber() || !info[3].IsNumber() || !info[4].IsNumber()){
		Napi::TypeError::New(env, "invalid type of arguments").ThrowAsJavaScriptException();
		return;
	}

	Napi::Function callback = info[0].As<Napi::Function>();
	std::string str_audio_format = (std::string) info[1].ToString();
  	int sample_rate = info[2].As<Napi::Number>().Int32Value();
  	int sample_bits = info[3].As<Napi::Number>().Int32Value();
  	int channel = info[4].As<Napi::Number>().Int32Value();

	RecordFlag record_flag = RF_BOTH_IO;// 0 recording both io audio; 1 recording input audio only; 2 recording output audio only
	bool hasRecordFlag = false;
	if(info.Length() >= 6 && info[5].IsString()) {
		std::string flag = (std::string) info[5].ToString();
		if(flag == "only_input") {
			record_flag = RF_ONLY_INPUT;
			hasRecordFlag = true;
		}else if(flag == "only_output") {
			record_flag = RF_ONLY_OUTPUT;
			hasRecordFlag = true;
		}
	}

	// record.start(audio_format, sample_rate, sample_bits, channel, record_flag, cache_dir, cache_key)
	std::string cache_dir;
	std::string cache_key;
	if(hasRecordFlag){
		if(info.Length() >= 7 && info[6].IsString()){
			cache_dir = (std::string) info[6].ToString();
			if(info.Length() >= 8 && info[7].IsString()){
				cache_key = (std::string) info[7].ToString();
			}
		}
	}

	// record.start(audio_format, sample_rate, sample_bits, channel, cache_dir, cache_key)
	// if cache_key set, it will be a part of the cached audio file's name.
	else {
		if(info.Length() >= 6 && info[5].IsString()){
			cache_dir = (std::string) info[5].ToString();
			if(info.Length() >= 7 && info[6].IsString()){
				cache_key = (std::string) info[6].ToString();
			}
		}
	}

	//check audio_format
	AudioFormat audio_format;
	if(str_audio_format == "pcm") {
		audio_format = AF_PCM;
	}else if(str_audio_format == "silk") {
		audio_format = AF_SILK;
	}else{
		Napi::TypeError::New(env, "unsupported audio format, only support pcm/silk currently").ThrowAsJavaScriptException();
		return;
	}

	//check sample_rate
	if(sample_rate != 8000 && sample_rate != 16000 && sample_rate != 24000 && 
		sample_rate != 32000 && sample_rate != 44100 && sample_rate != 48000){
		Napi::TypeError::New(env, "invalid sample rate, only support 8000/16000/24000/32000/44100/48000").ThrowAsJavaScriptException();
		return;
	}

	// check sample_bit
	if(sample_bits != 8 && sample_bits != 16 && sample_bits != 24 && sample_bits != 32){
		Napi::TypeError::New(env, "invalid sample bit, only support 8/16/24/32").ThrowAsJavaScriptException();
		return;
	}

	// check channel
	if(channel != 1 && channel != 2){
		Napi::TypeError::New(env, "invalid channel number, only support 1/2").ThrowAsJavaScriptException();
		return;
	}

	printf("Record::Record audio_format:%s, sample_rate:%d, sample_bit:%d, channel:%d \n", str_audio_format.c_str(), sample_rate, sample_bits, channel);
	printf("Record::Record record_flag:%d, cache_dir:%s, cache_key:%s \n", record_flag, cache_dir.c_str(), cache_key.c_str());

	m_env = callback.Env();
    m_callback = Napi::Persistent(callback);

	m_uv_closed = false;
	m_need_stop = false;

	uv_mutex_init(&m_lock_events);

	m_async = new uv_async_t;
	m_async->data = this;
	uv_async_init(uv_default_loop(), m_async, OnSend);

	// file save path prefix, e.g. C:/path/to/dir/audio_filename
	std::string prefix;
	if(cache_key.size() > 0){
		prefix = cache_dir + "/" + GetCurrentTime();
		if(cache_key.size() > 0) {
			prefix += "_"+ cache_key;
		}
	}

	m_need_record_i = false;
	if(record_flag != RF_ONLY_OUTPUT){
		std::string prefix_in;
		if(prefix.size() > 0) prefix_in = prefix + "_in";
		m_ap_i.SetTgtAudioParam(audio_format, sample_rate, sample_bits, channel, prefix_in);
		m_need_record_i = true;
		uv_thread_create(&m_record_thread_i, RunThreadI, this);
	}

	m_need_record_o = false;
	if(record_flag != RF_ONLY_INPUT){
		std::string prefix_out;
		if(prefix.size() > 0) prefix_out = prefix + "_out";
		m_ap_o.SetTgtAudioParam(audio_format, sample_rate, sample_bits, channel, prefix_out);
		m_need_record_o = true;
		uv_thread_create(&m_record_thread_o, RunThreadO, this);
	}
}

Record::~Record() {
	printf("[%ld]Record::~Record\n", GetCurrentThreadId());
	Stop();
}

Napi::Object Record::Init(Napi::Env env, Napi::Object exports) {
	printf("[%ld]Record::Init\n", GetCurrentThreadId());
	Napi::Function func = DefineClass(env, "Record", {
		InstanceMethod("destroy", &Record::Destroy)
	});

	Napi::FunctionReference* constructor = new Napi::FunctionReference();
	*constructor = Napi::Persistent(func);
	constructor->SuppressDestruct();

	exports.Set("Record", func);
	return exports;
}

void Record::Destroy(const Napi::CallbackInfo& info){
	printf("[%ld]Record::Destroy\n", GetCurrentThreadId());
	Stop();
}

void Record::Run(WaveSource ws){
	printf("[%ld]Record::Run, ws:%d\n", GetCurrentThreadId(), ws);

	if(ws == Wave_In) {
		this->AddEvent( RecordEvent{EVT_IN_START, ""} );
	}else{
		this->AddEvent( RecordEvent{EVT_OUT_START, ""} );
	}

	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDevice *pDevice = NULL;
	IAudioClient *pAudioClient = NULL;
	WAVEFORMATEX *pwfx = NULL;
	IAudioCaptureClient *pCaptureClient = NULL;
	bool bHasError = false;
	std::string strErrMsg;
	bool bCoInitializeExSucc = false;
	bool bCrashed = false;
	int nRetryCnt = 0;

Recording:

	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	EXIT_ON_ERROR(hr, "CoInitializeEx failed");
	bCoInitializeExSucc = true;

	__try{
		hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL,CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
		EXIT_ON_ERROR(hr, "CoCreateInstance failed");

		EDataFlow device = eCapture;
		if (ws == Wave_Out) {
			device = eRender;
		}
		
		hr = pEnumerator->GetDefaultAudioEndpoint(device, eConsole, &pDevice);
		EXIT_ON_ERROR(hr, "GetDefaultAudioEndpoint failed");

		hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
		EXIT_ON_ERROR(hr, "Activate failed");
		
		hr = pAudioClient->GetMixFormat(&pwfx);
		EXIT_ON_ERROR(hr, "GetMixFormat failed");

		int nBlockAlign = 0;
		// coerce int-XX wave format (like int-16 or int-32)
		// can do this in-place since we're not changing the size of the format
		// also, the engine will auto-convert from float to int for us
		if(pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE){
			EXIT_ON_ERROR(E_UNEXPECTED, "Unsupported Wave format");
		}

		// naked scope for case-local variable
		PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
		if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat)) {
			// WE GET HERE!
			pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
			// convert it to PCM, but let it keep as many bits of precision as it has initially...though it always seems to be 32
			// comment this out and set wBitsPerSample to  pwfex->wBitsPerSample = getBitsPerSample(); to get an arguably "better" quality 32 bit pcm
			// unfortunately flash media live encoder basically rejects 32 bit pcm, and it's not a huge gain sound quality-wise, so disabled for now.
			pwfx->wBitsPerSample = 16;
			pEx->Samples.wValidBitsPerSample = pwfx->wBitsPerSample;
			pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
			pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
			nBlockAlign = pwfx->nBlockAlign;
			// see also setupPwfex method
		}else {
			EXIT_ON_ERROR(E_UNEXPECTED, "Invalid Wave format");
		}

		if (ws == Wave_In) {
			m_ap_i.SetOrgAudioParam(AF_PCM, pwfx->nSamplesPerSec, pwfx->wBitsPerSample, pwfx->nChannels);
		}else{
			m_ap_o.SetOrgAudioParam(AF_PCM, pwfx->nSamplesPerSec, pwfx->wBitsPerSample, pwfx->nChannels);
		}

		DWORD StreamFlags = 0;
		if (ws == Wave_Out) {
			StreamFlags |= AUDCLNT_STREAMFLAGS_LOOPBACK;
		}

		REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
		hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,StreamFlags,hnsRequestedDuration,0,pwfx,NULL);
		EXIT_ON_ERROR(hr, "AudioClient Initialize failed");

		// Get the size of the allocated buffer.
		UINT32 bufferFrameCount;
		hr = pAudioClient->GetBufferSize(&bufferFrameCount);
		EXIT_ON_ERROR(hr, "AudioClient GetBufferSize failed");

		hr = pAudioClient->GetService(IID_IAudioCaptureClient,(void**)&pCaptureClient);
		EXIT_ON_ERROR(hr, "AudioClient GetService failed");


		// Calculate the actual duration of the allocated buffer.
		REFERENCE_TIME hnsActualDuration = (double)REFTIMES_PER_SEC * bufferFrameCount / pwfx->nSamplesPerSec;

		hr = pAudioClient->Start();  // Start recording.
		EXIT_ON_ERROR(hr, "AudioClient Start failed");

		// Each loop fills about half of the shared buffer.
		while (!m_need_stop)
		{
			// Sleep for half the buffer duration.
			Sleep(hnsActualDuration / REFTIMES_PER_MILLISEC / 2);

			UINT32 packetLength = 0;
			hr = pCaptureClient->GetNextPacketSize(&packetLength);
			EXIT_ON_ERROR(hr, "GetNextPacketSize failed");

			while (!m_need_stop && packetLength != 0)
			{
				// Get the available data in the shared buffer.
				BYTE *pData;
				DWORD flags;
				UINT32 numFramesAvailable;
				hr = pCaptureClient->GetBuffer(&pData,&numFramesAvailable,&flags, NULL, NULL);
				EXIT_ON_ERROR(hr, "GetBuffer failed");

				if (flags & AUDCLNT_BUFFERFLAGS_SILENT){
					pData = NULL;  // Tell CopyData to write silence.
				}

				// Copy the available capture data to the audio sink.
				DWORD now = ::GetTickCount();
				if(ws == Wave_In){
					m_ap_i.OnAudioData(pData, nBlockAlign *numFramesAvailable);
					if( now - m_last_time_i > 200 ){
						this->AddEvent(RecordEvent{EVT_IN_AUDIO,"" });
						m_last_time_i = now;
					}	
				}else if(ws == Wave_Out){
					m_ap_o.OnAudioData(pData, nBlockAlign *numFramesAvailable);
					if( now - m_last_time_o > 200 ){
						this->AddEvent(RecordEvent{EVT_OUT_AUDIO, ""});
						m_last_time_o = now;
					}	
				}

				hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
				EXIT_ON_ERROR(hr, "ReleaseBuffer failed");

				hr = pCaptureClient->GetNextPacketSize(&packetLength);
				EXIT_ON_ERROR(hr, "GetNextPacketSize1 failed");
			}
		}

		hr = pAudioClient->Stop();  // Stop recording.
		EXIT_ON_ERROR(hr, "AudioClient Stop failed");

		if(bCoInitializeExSucc){
			if(pwfx) CoTaskMemFree(pwfx);
			SAFE_RELEASE(pEnumerator)
			SAFE_RELEASE(pDevice)
			SAFE_RELEASE(pAudioClient)
			SAFE_RELEASE(pCaptureClient)
			CoUninitialize();
		}
	}

	__except(EXCEPTION_EXECUTE_HANDLER ){
		bCrashed = true;
		std::ostringstream stream;
		stream << "crashed, exception code = 0x" << hex << GetExceptionCode();
		strErrMsg = stream.str();
	}

Exit:
	if(bHasError || bCrashed){
		if(nRetryCnt < 3){
			nRetryCnt ++;

			bHasError = false;
			bCrashed = false;
			bCoInitializeExSucc = false;
			pEnumerator = NULL;
			pDevice = NULL;
			pAudioClient = NULL;
			pwfx = NULL;
			pCaptureClient = NULL;

			Sleep(100);

			printf("retry: %d\n", nRetryCnt);
			goto Recording;
		}

		this->AddEvent( RecordEvent{EVT_ERROR, strErrMsg} );
	}
}

void Record::Stop() {
	printf("[%ld]Record::Stop\n", GetCurrentThreadId());
	if (!m_need_stop) {
		m_ap_i.Stop();
		m_ap_o.Stop();
		m_need_stop = true;

    	if(m_need_record_i) uv_thread_join(&m_record_thread_i);
    	if(m_need_record_o) uv_thread_join(&m_record_thread_o);

    	if(m_need_record_i) m_callback.Call({Napi::String::New(m_env, EVT_IN_STOP), Napi::String::New(m_env, "")});
    	if(m_need_record_o) m_callback.Call({Napi::String::New(m_env, EVT_OUT_STOP), Napi::String::New(m_env, "")});

    	uv_close((uv_handle_t *)m_async, OnClose);
	}
}

void Record::AddEvent(const RecordEvent& evt){
	//printf("[%ld]Record::AddEvent, type:%s\n", GetCurrentThreadId(), evt.type.c_str());
	uv_mutex_lock(&m_lock_events);
	m_events.push_back(evt);
	uv_async_send(m_async);
	uv_mutex_unlock(&m_lock_events);
}

void Record::HandleSend() {
	//printf("[%ld]Record::HandleSend\n", GetCurrentThreadId());
	if(m_need_stop){
		return;
	}

	Napi::HandleScope scope(m_env);

	std::vector<RecordEvent> events;
	uv_mutex_lock(&m_lock_events);
	events.swap(m_events);
	m_events.clear();
	uv_mutex_unlock(&m_lock_events);

	bool bInSended = false;
	bool bOutSended = false;
	for(int i=0; i < (int)events.size(); i ++){

		if(events[i].type == EVT_IN_AUDIO){
			if( bInSended ) continue; // merge same events, trigger only once
			bInSended = true;
			
			std::vector<BYTE> audio_data;
			m_ap_i.GetAudioData(audio_data);
			if(audio_data.size() == 0 ) continue;

			m_callback.Call({
				Napi::String::New(m_env, events[i].type), 
				Napi::Buffer<BYTE>::Copy(m_env, audio_data.data(), audio_data.size())
			});
		}
		
		else if (events[i].type == EVT_OUT_AUDIO){
			if( bOutSended ) continue; // merge same events, trigger only once
			bOutSended = true;

			std::vector<BYTE> audio_data;
			m_ap_o.GetAudioData(audio_data);
			if(audio_data.size() == 0 ) continue;

			m_callback.Call({
				Napi::String::New(m_env, events[i].type), 
				Napi::Buffer<BYTE>::Copy(m_env, audio_data.data(), audio_data.size())
			});
		}

		else{
			m_callback.Call({
				Napi::String::New(m_env, events[i].type), 
				Napi::String::New(m_env, events[i].value)
			});
		}
	}
}