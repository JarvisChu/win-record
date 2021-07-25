#ifndef _RECORD_H
#define _RECORD_H

#include <napi.h>
#include <string>
#include <vector>
#include "audio_processor.h"

struct RecordEvent {
	std::string type;
	std::string value;
};

enum RecordFlag {
	RF_BOTH_IO = 0,
	RF_ONLY_INPUT = 1,
	RF_ONLY_OUTPUT = 2,
};

class Record : public Napi::ObjectWrap<Record> {
	public:
		static Napi::Object Init(Napi::Env env, Napi::Object exports);
		explicit Record(const Napi::CallbackInfo& info);
		~Record();

		void Stop();
		void Destroy(const Napi::CallbackInfo& info);
		void AddEvent(const RecordEvent&);
		void HandleSend();
		void Run(WaveSource ws);
	private:
		std::string GetCurrentTime();

	private:
		napi_env m_env;
    	Napi::FunctionReference m_callback;

		uv_mutex_t m_lock_events;
		std::vector<RecordEvent> m_events;
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
