var events = require('events')
var bindings = require('bindings')
var Record = bindings('win_record_addon').Record

module.exports = function () {
  var that = new events.EventEmitter()
  var record = null
  var left = false
  var right = false
  var audio_format
  var sample_rate
  var sample_bit
  var channel

  that.once('newListener', function () {
    console.info("newListener")
    record = new Record(function (type, x) {
      if(type == 'error') that.emit(type, x);
      else if(type == 'start') that.emit(type);
      else if(type == 'stop') that.emit(type);
      else if(type == 'data') that.emit(type, x);
      else that.emit(type, x);

    }, audio_format, sample_rate, sample_bit, channel)
  })

  that.ref = function () {
    if (record) record.ref()
  }

  that.unref = function () {
    if (record) record.unref()
  }

  that.destroy = function () {
    if (record) record.destroy()
    record = null
  }

  that.setparam = function (audio_format_, sample_rate_, sample_bit_, channel_) {
    console.info("setparam")
    audio_format = audio_format_
    sample_rate = sample_rate_
    sample_bit = sample_bit_
    channel = channel_
    //if (record) record.setparam(audio_format, sample_rate, sample_bit, channel)
  }

  return that
}
