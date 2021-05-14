var record = require('../')

var r = record()

// recording system input audio (microphone) started
r.on('in_start', function () {
  console.log('in_start')
})

// recording system output audio (speaker) started
r.on('out_start', function () {
  console.log('out_start')
})

r.on('in_stop', function () {
  console.log('in_stop')
})

r.on('out_stop', function () {
  console.log('out_stop')
})

// system input audio (microphone) data
r.on('in_audio', function (data) {
  console.log('in_audio: ', data)
})

// system output audio (microphone) data
r.on('out_audio', function (data) {
  console.log('out_audio: ', data)
})

r.on('error', function (msg) {
  console.log('error', msg)
})

// start recording
r.start('pcm', 8000, 16, 1); // audio_format (pcm/silk), sample_rate, sample_bit, channel
//r.start('pcm', 8000, 16, 1, "only_input");// audio_format, sample_rate, sample_bit, channel, only_input/only_output

setTimeout(function () {
  r.destroy()
  console.log('destroy')
}, 2000)