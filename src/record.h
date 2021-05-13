#ifndef _RECORD_H
#define _RECORD_H

#include <nan.h>
#include "mouse_hook.h"
#include <string>

using namespace v8;

struct RecordEvent {
	std::string type;
	std::string value;
};

const unsigned int BUFFER_SIZE = 10;

class Record : public Nan::ObjectWrap {
	public:
		static void Initialize(Local<Object> exports, Local<Value> module, Local<Context> context);
		static Nan::Persistent<Function> constructor;

		void Stop();
		void HandleEvent(WPARAM, POINT);
		void AddEvent(const RecordEvent&);
		void HandleSend();
		void Run();

	private:
		MouseHookRef hook_ref;
		RecordEvent* eventBuffer[BUFFER_SIZE];
		unsigned int readIndex;
		unsigned int writeIndex;
		Nan::Callback* event_callback;
		Nan::AsyncResource* async_resource;
		uv_async_t* async;
		uv_mutex_t lock;
		volatile bool stopped;

		DWORD thread_id;
		uv_thread_t thread;
		uv_mutex_t init_lock;
		uv_cond_t init_cond;

		explicit Record(Nan::Callback*, int, int, int, int);
		~Record();

		static NAN_METHOD(New);
		static NAN_METHOD(Destroy);
		static NAN_METHOD(AddRef);
		static NAN_METHOD(RemoveRef);
		static NAN_METHOD(SetParam);
public:
		// record audio format and params
		int m_audio_format;
		int m_sample_rate;
		int m_sample_bit;
		int m_channel;
		int m_bitrate; // effect only silk
};

#endif
