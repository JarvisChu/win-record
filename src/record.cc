#include <stdio.h>
#include "record.h"
#include <chrono>
#include <thread>
#include <Windows.h>

using namespace std;

const char* EVT_START = "start";
const char* EVT_STOP = "stop";
const char* EVT_LOCAL_AUDIO = "localaudio";
const char* EVT_REMOTE_AUDIO = "remoteaudio";
const char* EVT_ERROR = "error";

NAUV_WORK_CB(OnSend) {
	Record* record = (Record*) async->data;
	record->HandleSend();
}

void OnClose(uv_handle_t* handle) {
	printf("[%ld]OnClose\n", GetCurrentThreadId());
	uv_async_t* m_async = (uv_async_t*) handle;
	delete m_async;
}

void RunThread(void* arg) {
	Record* record = (Record*) arg;
	record->Run();
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

	uv_thread_create(&m_record_thread, RunThread, this);
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

void Record::Run()
{
	printf("[%ld]Record::Run\n", GetCurrentThreadId());

	this->AddEvent( RecordEvent{EVT_START, ""} );

	while(!m_stopped) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
	  RecordEvent evtData;
	  evtData.type = EVT_LOCAL_AUDIO;
	  evtData.value = "data001";
	  this->AddEvent(evtData);
  	}

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