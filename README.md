# win-record

Audio recording for Windows.
Recording audio data from system IO device (speaker/microphone), supporting audio formats like PCM/SILK.

# Usage

```
npm install win-record
```

# Build

```
export NODE_PATH=$(npm root -g)
node-gyp configure
node-gyp build
```

# Project Detail

```
sudo npm install -g yarn
yarn global add node-gyp 

cd win-record && yarn init -y # # init project (will generate package.json)

npm i -g nan
export NODE_PATH=$(npm root -g)

# manual create binding.gyp, index.js, src/*

node-gyp configure
node-gyp build #node-gyp build --debug
node-gyp rebuild
npm publish
```

# References
> https://juejin.cn/post/6844903971220357134
>
> https://juejin.cn/post/6844904030162911240
>
> github.com/kapetan/win-mouse

