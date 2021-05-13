#ifndef _RECORD_H
#define _RECORD_H

#include <nan.h>
#include <string>
#include <vector>

using namespace v8;

struct RecordEvent {
	std::string type;
	std::string value;
};

enum WaveSource
{
	Wave_Microphone = 1,
	Wave_Loopback = 2
};

class Record : public Nan::ObjectWrap {
	public:
		static void Initialize(Local<Object> exports, Local<Value> module, Local<Context> context);
		static Nan::Persistent<Function> constructor;

		void Stop();
		void AddEvent(const RecordEvent&);
		void HandleSend();
		void Run(WaveSource ws);

	private:
		explicit Record(Nan::Callback*, int, int, int, int);
		~Record();

		static NAN_METHOD(New);
		static NAN_METHOD(Destroy);
		static NAN_METHOD(AddRef);
		static NAN_METHOD(RemoveRef);

		uv_mutex_t m_lock_events;
		std::vector<RecordEvent> m_events;
		Nan::Callback* m_event_callback;
		Nan::AsyncResource* m_async_resource;
		uv_async_t* m_async;
		volatile bool m_stopped;
		uv_thread_t m_record_thread_i; // record input device audio
		uv_thread_t m_record_thread_o; // record output device audio

		// record audio format and params
		int m_audio_format;
		int m_sample_rate;
		int m_sample_bit;
		int m_channel;
		int m_bitrate; // effect only silk
};

#endif
