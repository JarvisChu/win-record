#include "record.h"

Napi::FunctionReference Record::constructor;

Napi::Object Record::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "Record", {
    InstanceMethod("add", &Record::Add),
  });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("Record", func);
  return exports;
}


Record::Record(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Record>(info)  {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);
}


Napi::Value Record::Add(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  if (info.Length() != 2 || !info[0].IsNumber()|| !info[1].IsNumber()) {
    Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
  }

  Napi::Number n0 = info[0].As<Napi::Number>();
  Napi::Number n1 = info[1].As<Napi::Number>();
  double answer = n0.DoubleValue() + n1.DoubleValue();

  return Napi::Number::New(info.Env(), answer);
}