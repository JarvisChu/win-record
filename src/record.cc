#include <stdio.h>
#include "record.h"
#include <chrono>
#include <thread>
#include <iostream>

using namespace std;

const char* EVT_START = "start";
const char* EVT_STOP = "stop";
const char* EVT_LOCAL_AUDIO = "localaudio";
const char* EVT_REMOTE_AUDIO = "remoteaudio";

NAUV_WORK_CB(OnSend) {
	Record* record = (Record*) async->data;
	record->HandleSend();
}

void OnClose(uv_handle_t* handle) {
	printf("onClose\n");
	uv_async_t* async = (uv_async_t*) handle;
	delete async;
}

void RunThread(void* arg) {
	Record* record = (Record*) arg;
	record->Run();
}

Nan::Persistent<Function> Record::constructor;

Record::Record(Nan::Callback* callback, int audio_format, int sample_rate, int sample_bit, int channel) {
	printf("Record::Record\n");

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

	for (size_t i = 0; i < BUFFER_SIZE; i++) {
		eventBuffer[i] = new RecordEvent();
	}

	readIndex = 0;
	writeIndex = 0;

	event_callback = callback;
	async_resource = new Nan::AsyncResource("win-record:Record");
	stopped = false;

	uv_mutex_init(&lock);
	uv_mutex_init(&init_lock);
	uv_cond_init(&init_cond);

	async = new uv_async_t;
	async->data = this;
	uv_async_init(uv_default_loop(), async, OnSend);

	uv_thread_create(&thread, RunThread, this);
}


Record::~Record() {
	printf("Record::~Record\n");
	Stop();

	uv_mutex_destroy(&init_lock);
	uv_cond_destroy(&init_cond);

	uv_mutex_destroy(&lock);
	delete event_callback;

	// HACK: Sometimes deleting async resource segfaults.
	// Probably related to https://github.com/nodejs/nan/issues/772
	if (!Nan::GetCurrentContext().IsEmpty()) {
		delete async_resource;
	}

	for (size_t i = 0; i < BUFFER_SIZE; i++) {
		delete eventBuffer[i];
	}
}

void Record::Initialize(Local<Object> exports, Local<Value> module, Local<Context> context) {
	Nan::HandleScope scope;

	Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(Record::New);
	tpl->SetClassName(Nan::New<String>("Record").ToLocalChecked());
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	Nan::SetPrototypeMethod(tpl, "destroy", Record::Destroy);
	Nan::SetPrototypeMethod(tpl, "ref", Record::AddRef);
	Nan::SetPrototypeMethod(tpl, "unref", Record::RemoveRef);
	Nan::SetPrototypeMethod(tpl, "setparam", Record::SetParam);

	Record::constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
	exports->Set(context,
		Nan::New("Record").ToLocalChecked(),
		Nan::GetFunction(tpl).ToLocalChecked());
}

void Record::Run()
{
	std::thread::id this_id = std::this_thread::get_id();
	std::cout<<"Record::Run, threadid: "<<this_id<<std::endl; 

    RecordEvent evtStart = { EVT_START, "" };
	this->AddEvent(evtStart);

	while(!stopped) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
	  RecordEvent evtData;
	  evtData.type = EVT_LOCAL_AUDIO;
	  evtData.value = "data001";
	  this->AddEvent(evtData);
  	}

	RecordEvent evtStop = { EVT_STOP, "" };
	this->AddEvent(evtStop);
}


void Record::Stop() {
	std::thread::id this_id = std::this_thread::get_id();
	std::cout<<"Record::Stop, threadid: "<<this_id<<std::endl; 
	if (!stopped) {
		uv_mutex_lock(&lock);
		stopped = true;
		uv_mutex_unlock(&lock);
	}
	
}

void Record::AddEvent(const RecordEvent& evt){
	printf("Record::AddEvent, type:%s\n", evt.type.c_str());
	
	uv_mutex_lock(&lock);
	eventBuffer[writeIndex]->value = evt.value;
	eventBuffer[writeIndex]->type = evt.type;
	writeIndex = (writeIndex + 1) % BUFFER_SIZE;
	uv_async_send(async);
	uv_mutex_unlock(&lock);
}

void Record::HandleSend() {
	std::thread::id this_id = std::this_thread::get_id();
	std::cout<<"Record::HandleSend, threadid: "<<this_id<<std::endl; 

	Nan::HandleScope scope;

	uv_mutex_lock(&lock);

	while (readIndex != writeIndex /*&& !stopped*/) {
		printf("Record::HandleSend2 readIndex:%d, writeIndex:%d, stopped:%d\n", readIndex, writeIndex, stopped);	
		RecordEvent e = {
			eventBuffer[readIndex]->type,
			eventBuffer[readIndex]->value
		};
		readIndex = (readIndex + 1) % BUFFER_SIZE;

		Local<Value> argv[] = {
			Nan::New<String>(e.type).ToLocalChecked(),
			Nan::New<String>(e.value).ToLocalChecked()
		};

		event_callback->Call(2, argv, async_resource);
	}

	uv_mutex_unlock(&lock);

	if(stopped) {
		uv_close((uv_handle_t*) async, OnClose);
	}

}

NAN_METHOD(Record::New) {
	printf("Record::New\n");
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
	printf("Record::Destroy\n");
	Record* record = Nan::ObjectWrap::Unwrap<Record>(info.Holder());
	record->Stop();

	info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Record::AddRef) {
	printf("Record::AddRef\n");
	Record* record = ObjectWrap::Unwrap<Record>(info.Holder());
	uv_ref((uv_handle_t*) record->async);

	info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Record::RemoveRef) {
	printf("Record::RemoveRef\n");
	Record* record = ObjectWrap::Unwrap<Record>(info.Holder());
	uv_unref((uv_handle_t*) record->async);

	info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Record::SetParam) {
	printf("SetParam");
	Record* obj = ObjectWrap::Unwrap<Record>(info.Holder());
	v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();

	if (info.Length() < 4 || !info[0]->IsNumber() || !info[1]->IsNumber() || !info[2]->IsNumber() || !info[3]->IsNumber()) {
		Nan::ThrowTypeError("invalid param");
		return;
	}

	int m_audio_format = info[0]->NumberValue(context).FromJust();
	int m_sample_rate = info[1]->NumberValue(context).FromJust();
	int m_sample_bit = info[2]->NumberValue(context).FromJust();
	int m_channel = info[3]->NumberValue(context).FromJust();

	//check audio_format
	if(m_audio_format != 1 && m_audio_format != 2){ // 1pcm,2silk
		Nan::ThrowError("unsupported audio format, only support pcm/silk currently");
		//Napi::Error::New(env, "unsupported audio format, only support pcm/silk currently").ThrowAsJavaScriptException();
	}

	//check sample_rate
	if(m_sample_rate != 8000 && m_sample_rate != 16000 && m_sample_rate != 24000 && 
		m_sample_rate != 32000 && m_sample_rate != 44100 && m_sample_rate != 48000){
		//Napi::Error::New(env, "invalid sample rate").ThrowAsJavaScriptException();
		Nan::ThrowError("invalid sample rate");
	}

	// check sample_bit
	if(m_sample_bit != 8 && m_sample_bit != 16 && m_sample_bit != 24 && m_sample_bit != 32){
		//Napi::Error::New(env, "invalid sample bit").ThrowAsJavaScriptException();
		Nan::ThrowError("invalid sample bit");
	}

	// check channel
	if(m_channel != 1 && m_channel != 2){
		//Napi::Error::New(env, "invalid channel number").ThrowAsJavaScriptException();
		Nan::ThrowError("invalid channel number");
	}

	printf("Record::SetParam audio_format:%d, sample_rate:%d, sample_bit:%d, channel:%d \n", 
	m_audio_format, m_sample_rate, m_sample_bit, m_channel);

	obj->m_audio_format = m_audio_format;
	obj->m_sample_rate = m_sample_rate;
	obj->m_sample_bit = m_sample_bit;
	obj->m_channel = m_channel;

	info.GetReturnValue().Set(true);
}
