var record = require('../')
var fs = require('fs');

var r = record()

var audio_format = 'silk'  // pcm / silk

// recording system input audio (microphone) started
r.on('in_start', function () {
  console.log('in_start')

  if(audio_format == 'pcm'){
    fs.open('in_audio.pcm', 'w', function (err, file) {
      if (err) throw err;
      console.log('in_audio file open ok!');
    });
  }

  else if(audio_format == 'silk'){
    fs.writeFile('in_audio.silk', "#!SILK_V3", function (err) { // write silk file header
      if (err) throw err;
      console.log('in_audio file open ok!');
    });
  }
})

// recording system output audio (speaker) started
r.on('out_start', function () {
  console.log('out_start')

  if(audio_format == 'pcm'){
    fs.open('out_audio.pcm', 'w', function (err, file) {
      if (err) throw err;
      console.log('out_audio file open ok!');
    });
  }

  else if(audio_format == 'silk'){
    fs.writeFile('out_audio.silk', "#!SILK_V3", function (err) { // write silk file header
      if (err) throw err;
      console.log('out_audio file open ok!');
    });
  }

})

r.on('in_stop', function () {
  console.log('in_stop')
})

r.on('out_stop', function () {
  console.log('out_stop')
})

// system input audio (microphone) data
r.on('in_audio', function (data) {
  //console.log('in_audio: ', typeof 'data')
  var filename;
  if(audio_format == 'pcm') filename = "in_audio.pcm";
  else if (audio_format == 'silk') filename = "in_audio.silk";

  fs.appendFile(filename, data, function (err) {
    if (err) throw err;
    console.log('in_audio save ok, size:', data.length);
  });
})

// system output audio (microphone) data
r.on('out_audio', function (data) {
  //console.log('out_audio: ', typeof 'data')
  var filename;
  if(audio_format == 'pcm') filename = "out_audio.pcm";
  else if (audio_format == 'silk') filename = "out_audio.silk";

  fs.appendFile(filename, data, function (err) {
    if (err) throw err;
    console.log('out_audio save ok, size:', data.length);
  });
})

r.on('error', function (msg) {
  console.log('error', msg)
})

// start recording
//r.start(audio_format, 8000, 16, 1); // audio_format (pcm/silk), sample_rate, sample_bit, channel
r.start(audio_format, 8000, 16, 1, "only_output");// audio_format, sample_rate, sample_bit, channel, only_input/only_output

setTimeout(function () {
  r.destroy()
  console.log('destroy')
}, 10000)