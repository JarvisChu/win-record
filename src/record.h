#ifndef _RECORD_H
#define _RECORD_H

#include <napi.h>

class Record: public Napi::ObjectWrap<Record> {
public:
    //Init function for setting the export key to JS
    static Napi::Object Init(Napi::Env env, Napi::Object exports);

    //Constructor to initialise
    Record(const Napi::CallbackInfo& info); 

private:
    //Define FunctionReference to avoid gc
    static Napi::FunctionReference constructor; 
    Napi::Value Add(const Napi::CallbackInfo& info);
};

#endif