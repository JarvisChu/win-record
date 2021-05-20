#ifndef _RECORD_H
#define _RECORD_H

#include <nan.h>
#include <string>
#include <vector>
#include "audio_processor.h"

using namespace v8;

struct RecordEvent {
	std::string type;
	std::string value;
};

enum RecordFlag {
	RF_BOTH_IO = 0,
	RF_ONLY_INPUT = 1,
	RF_ONLY_OUTPUT = 2,
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
		explicit Record(Nan::NAN_METHOD_ARGS_TYPE info);
		~Record();

		static NAN_METHOD(New);
		static NAN_METHOD(Destroy);

		uv_mutex_t m_lock_events;
		std::vector<RecordEvent> m_events;
		Nan::Callback* m_event_callback;
		Nan::AsyncResource* m_async_resource;
		uv_async_t* m_async;
		
		volatile bool m_uv_closed;
		volatile bool m_need_stop;
		volatile bool m_thread_i_running;
		volatile bool m_thread_o_running;

		uv_thread_t m_record_thread_i; // recording input device audio. Wave_In
		uv_thread_t m_record_thread_o; // recording output device audio. Wave_Out
		AudioProcessor m_ap_i;
		AudioProcessor m_ap_o;
};

#endif
