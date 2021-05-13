#include <stdio.h>
#include "record.h"
#include <chrono>
#include <thread>
#include <Windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

using namespace std;

#pragma comment(lib, "winmm.lib")

const char* EVT_START = "start";
const char* EVT_STOP = "stop";
const char* EVT_LOCAL_AUDIO = "localaudio";
const char* EVT_REMOTE_AUDIO = "remoteaudio";
const char* EVT_ERROR = "error";


// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres) { if (FAILED(hres)) { goto Exit; } }     
#define SAFE_RELEASE(punk) { if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; } }
                
const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);


NAUV_WORK_CB(OnSend) {
	Record* record = (Record*) async->data;
	record->HandleSend();
}

void OnClose(uv_handle_t* handle) {
	printf("[%ld]OnClose\n", GetCurrentThreadId());
	uv_async_t* m_async = (uv_async_t*) handle;
	delete m_async;
}

void RunThreadI(void* arg) {
	Record* record = (Record*) arg;
	record->Run(Wave_In);
}

void RunThreadO(void* arg) {
	Record* record = (Record*) arg;
	record->Run(Wave_Out);
}

Nan::Persistent<Function> Record::constructor;

Record::Record(Nan::Callback* callback, int audio_format, int sample_rate, int sample_bit, int channel) {
	printf("[%ld]Record::Record\n", GetCurrentThreadId());

	//check audio_format
	if(audio_format != 1 && audio_format != 2){ // 1pcm,2silk
		Nan::ThrowError("unsupported audio format, only support 1:pcm/2:silk currently");
	}

	//check sample_rate
	if(sample_rate != 8000 && sample_rate != 16000 && sample_rate != 24000 && 
		sample_rate != 32000 && sample_rate != 44100 && sample_rate != 48000){
		Nan::ThrowError("invalid sample rate");
	}

	// check sample_bit
	if(sample_bit != 8 && sample_bit != 16 && sample_bit != 24 && sample_bit != 32){
		Nan::ThrowError("invalid sample bit");
	}

	// check channel
	if(channel != 1 && channel != 2){
		Nan::ThrowError("invalid channel number");
	}

	m_audio_format = audio_format;
	m_sample_rate = sample_rate;
	m_sample_bit = sample_bit;
	m_channel = channel;
	printf("Record::Record audio_format:%d, sample_rate:%d, sample_bit:%d, channel:%d \n", 
	m_audio_format, m_sample_rate, m_sample_bit, m_channel);

	m_event_callback = callback;
	m_async_resource = new Nan::AsyncResource("win-record:Record");
	m_stopped = false;

	uv_mutex_init(&m_lock_events);

	m_async = new uv_async_t;
	m_async->data = this;
	uv_async_init(uv_default_loop(), m_async, OnSend);

	uv_thread_create(&m_record_thread_i, RunThreadI, this);
	uv_thread_create(&m_record_thread_o, RunThreadO, this);
}

Record::~Record() {
	printf("[%ld]Record::~Record\n", GetCurrentThreadId());
	Stop();

	delete m_event_callback;

	// HACK: Sometimes deleting async resource segfaults.
	// Probably related to https://github.com/nodejs/nan/issues/772
	if (!Nan::GetCurrentContext().IsEmpty()) {
		delete m_async_resource;
	}
}

void Record::Initialize(Local<Object> exports, Local<Value> module, Local<Context> context) {
	printf("[%ld]Record::Initialize\n", GetCurrentThreadId());
	Nan::HandleScope scope;

	Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(Record::New);
	tpl->SetClassName(Nan::New<String>("Record").ToLocalChecked());
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	Nan::SetPrototypeMethod(tpl, "destroy", Record::Destroy);
	Nan::SetPrototypeMethod(tpl, "ref", Record::AddRef);
	Nan::SetPrototypeMethod(tpl, "unref", Record::RemoveRef);

	Record::constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
	exports->Set(context,
		Nan::New("Record").ToLocalChecked(),
		Nan::GetFunction(tpl).ToLocalChecked());
}

void Record::Run(WaveSource ws){
	printf("[%ld]Record::Run, ws:%d\n", GetCurrentThreadId(), ws);
	this->AddEvent( RecordEvent{EVT_START, ""} );

	CoInitializeEx(NULL, 0);
	IMMDeviceEnumerator *pEnumerator = NULL;
	HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL,CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
	EXIT_ON_ERROR(hr);

	EDataFlow device = eCapture;
	if (ws == Wave_Out) {
		device = eRender;
	}
	
	IMMDevice *pDevice = NULL;
	hr = pEnumerator->GetDefaultAudioEndpoint(device, eConsole, &pDevice);
	EXIT_ON_ERROR(hr);

	IAudioClient *pAudioClient = NULL;
	hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
	EXIT_ON_ERROR(hr);

	WAVEFORMATEX *pwfx = NULL;
	hr = pAudioClient->GetMixFormat(&pwfx);
	EXIT_ON_ERROR(hr);

	int nBlockAlign = 0;
	// coerce int-XX wave format (like int-16 or int-32)
	// can do this in-place since we're not changing the size of the format
	// also, the engine will auto-convert from float to int for us
	switch (pwfx->wFormatTag) {
	case WAVE_FORMAT_EXTENSIBLE: // 65534
	{
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
		}
		else {
			goto Exit;
		}
	}
	break;
	default:
			//ShowOutput("Don't know how to coerce WAVEFORMATEX with wFormatTag = 0x%08x to int-16\n", pwfx->wFormatTag);
			CoTaskMemFree(pwfx);
			pAudioClient->Release();
			return;
			//return E_UNEXPECTED;
		}
		
	EXIT_ON_ERROR(hr);

	DWORD StreamFlags = 0;
	if (ws == Wave_Out) {
		StreamFlags |= AUDCLNT_STREAMFLAGS_LOOPBACK;
	}

	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,StreamFlags,hnsRequestedDuration,0,pwfx,NULL);
	EXIT_ON_ERROR(hr);

	// Get the size of the allocated buffer.
	UINT32 bufferFrameCount;
	hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	EXIT_ON_ERROR(hr);

	IAudioCaptureClient *pCaptureClient = NULL;
	hr = pAudioClient->GetService(IID_IAudioCaptureClient,(void**)&pCaptureClient);
	EXIT_ON_ERROR(hr);

	// Notify the audio sink which format to use.
	//hr = pMySink->SetFormat(pwfx);
	//EXIT_ON_ERROR(hr)

	// Calculate the actual duration of the allocated buffer.
	REFERENCE_TIME hnsActualDuration = (double)REFTIMES_PER_SEC * bufferFrameCount / pwfx->nSamplesPerSec;

	hr = pAudioClient->Start();  // Start recording.
	EXIT_ON_ERROR(hr);

	// Each loop fills about half of the shared buffer.
	while (!m_stopped)
	{
		// Sleep for half the buffer duration.
		Sleep(hnsActualDuration / REFTIMES_PER_MILLISEC / 2);

		UINT32 packetLength = 0;
		hr = pCaptureClient->GetNextPacketSize(&packetLength);
		EXIT_ON_ERROR(hr);

		while (!m_stopped && packetLength != 0)
		{
			// Get the available data in the shared buffer.
			BYTE *pData;
			DWORD flags;
			UINT32 numFramesAvailable;
			hr = pCaptureClient->GetBuffer(&pData,&numFramesAvailable,&flags, NULL, NULL);
			EXIT_ON_ERROR(hr);

			if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
			{
				pData = NULL;  // Tell CopyData to write silence.
			}

			// Copy the available capture data to the audio sink.
			//hr = pMySink->CopyData(pData, nBlockAlign *numFramesAvailable, &bDone);
			if(ws == Wave_In){
				m_ap_i.OnAudioData(pData, nBlockAlign *numFramesAvailable);
				this->AddEvent(RecordEvent{EVT_LOCAL_AUDIO, std::to_string(nBlockAlign *numFramesAvailable) });
			}else if(ws == Wave_Out){
				m_ap_o.OnAudioData(pData, nBlockAlign *numFramesAvailable);
				this->AddEvent(RecordEvent{EVT_REMOTE_AUDIO, std::to_string(nBlockAlign *numFramesAvailable) });
			}

			EXIT_ON_ERROR(hr);

			hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
			EXIT_ON_ERROR(hr);

			hr = pCaptureClient->GetNextPacketSize(&packetLength);
			EXIT_ON_ERROR(hr);
		}
	}

	hr = pAudioClient->Stop();  // Stop recording.
	EXIT_ON_ERROR(hr);

	Exit:
	CoTaskMemFree(pwfx);
	SAFE_RELEASE(pEnumerator)
	SAFE_RELEASE(pDevice)
	SAFE_RELEASE(pAudioClient)
	SAFE_RELEASE(pCaptureClient)

	this->AddEvent(RecordEvent{EVT_STOP, ""});
}

void Record::Stop() {
	printf("[%ld]Record::Stop\n", GetCurrentThreadId());
	if (!m_stopped) {
		m_stopped = true;
	}
}

void Record::AddEvent(const RecordEvent& evt){
	printf("[%ld]Record::AddEvent, type:%s\n", GetCurrentThreadId(), evt.type.c_str());
	uv_mutex_lock(&m_lock_events);
	m_events.push_back(evt);
	uv_async_send(m_async);
	uv_mutex_unlock(&m_lock_events);
}

void Record::HandleSend() {
	printf("[%ld]Record::HandleSend\n", GetCurrentThreadId());	
	Nan::HandleScope scope;

	uv_mutex_lock(&m_lock_events); 
	for(int i=0; i < m_events.size(); i ++){
		Local<Value> argv[] = {
			Nan::New<String>(m_events[i].type).ToLocalChecked(),
			Nan::New<String>(m_events[i].value).ToLocalChecked()
		};
		m_event_callback->Call(2, argv, m_async_resource);
	}
	m_events.clear();
	uv_mutex_unlock(&m_lock_events);

	if(m_stopped) {
		uv_close((uv_handle_t*) m_async, OnClose);
	}
}

NAN_METHOD(Record::New) {
	printf("[%ld]Record::New\n", GetCurrentThreadId());
	Nan::Callback* callback = new Nan::Callback(info[0].As<Function>());

	v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();
  	int audio_format = info[1]->NumberValue(context).FromJust();
  	int sample_rate = info[2]->NumberValue(context).FromJust();
  	int sample_bit = info[3]->NumberValue(context).FromJust();
  	int channel = info[4]->NumberValue(context).FromJust();
	Record* record = new Record(callback, audio_format, sample_rate, sample_bit, channel);
	record->Wrap(info.This());

	info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Record::Destroy) {	
	printf("[%ld]Record::Destroy\n", GetCurrentThreadId());
	Record* record = Nan::ObjectWrap::Unwrap<Record>(info.Holder());
	record->Stop();

	info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Record::AddRef) {
	printf("[%ld]Record::AddRef\n", GetCurrentThreadId());
	Record* record = ObjectWrap::Unwrap<Record>(info.Holder());
	uv_ref((uv_handle_t*) record->m_async);

	info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Record::RemoveRef) {
	printf("[%ld]Record::RemoveRef\n", GetCurrentThreadId());
	Record* record = ObjectWrap::Unwrap<Record>(info.Holder());
	uv_unref((uv_handle_t*) record->m_async);

	info.GetReturnValue().SetUndefined();
}