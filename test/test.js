var record = require('../')
var fs = require('fs');

var r = record()
var audio_format = 'silk'  // pcm / silk
var save_in_js = true

// recording system input audio (microphone) started
r.on('in_start', function () {
  console.log('in_start')

  if(save_in_js){
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
  }
})

// recording system output audio (speaker) started
r.on('out_start', function () {
  console.log('out_start')

  if(save_in_js){
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

  if(save_in_js){
    var filename;
    if(audio_format == 'pcm') filename = "in_audio.pcm";
    else if (audio_format == 'silk') filename = "in_audio.silk";

    fs.appendFile(filename, data, function (err) {
      if (err) throw err;
      console.log('in_audio save ok, size:', data.length);
    });
  }
})

// system output audio (microphone) data
r.on('out_audio', function (data) {
  //console.log('out_audio: ', typeof 'data')

  if(save_in_js){
    var filename;
    if(audio_format == 'pcm') filename = "out_audio.pcm";
    else if (audio_format == 'silk') filename = "out_audio.silk";

    fs.appendFile(filename, data, function (err) {
      if (err) throw err;
      console.log('out_audio save ok, size:', data.length);
    });
  }
})

r.on('error', function (msg) {
  console.log('error', msg)
})

// start recording
//r.start(audio_format, 8000, 16, 1); // audio_format (pcm/silk), sample_rate, sample_bit, channel
//r.start(audio_format, 8000, 16, 1, "only_output");// audio_format, sample_rate, sample_bit, channel, only_input/only_output

// with audio file saving, e.g.
// c:\\code\\github\\win-record\\test\\2021_07_25_20_25_17_mykey_in.silk
// c:\\code\\github\\win-record\\test\\2021_07_25_20_25_17_mykey_out.silk
// c:\\code\\github\\win-record\\test\\2021_07_25_20_25_17_mykey_in.wav
// c:\\code\\github\\win-record\\test\\2021_07_25_20_25_17_mykey_out.wav
r.start(audio_format, 8000, 16, 1, "c:\\code\\github\\win-record\\test", "mykey"); // audio_format, sample_rate, sample_bit, channel, cache_dir, cache_key
//r.start(audio_format, 8000, 16, 1, "only_output", "c:\\code\\github\\win-record\\test", "mykey"); // audio_format, sample_rate, sample_bit, channel, only_input/only_output, cache_dir, cache_key

setTimeout(function () {
  r.destroy()
  console.log('destroy')
}, 10000)