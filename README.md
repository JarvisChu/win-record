# win-record

Audio recording for Windows.
Recording audio data from system IO device (speaker/microphone), supporting audio formats like PCM/SILK.

# Usage

```
npm install win-record
```

# Example

Please refer to `test/test.js`

```javascript
var record = require('win-record')

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
}, 2000)
```

# Build && Test

```
yarn build
yarn test
```

# References
> - https://github.com/kapetan/win-mouse
> - https://github.com/ploverlake/silk
> - https://community.risingstack.com/using-buffers-node-js-c-plus-plus/

