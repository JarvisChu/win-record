# win-record

Audio recording for Windows.
Recording audio data from system IO device (speaker/microphone), supporting audio formats like PCM/SILK.

# Usage

```
npm install win-record
```

# Build

```
node-gyp configure
node-gyp build
```

# Project Detail

```
# 1. install yarn
sudo npm install -g yarn

# 2. install gyp
yarn global add node-gyp 

# 3. init project (will generate package.json)
cd win-record
yarn init -y # 

# 4. install node-addon-api
yarn add node-addon-api
# or npm i bindings node-addon-api -S # install bingdings and node-addon-api

# 5. manual create binding.gyp

# 6. manual create src/*

# 7. configure
node-gyp configure

# 8. build
node-gyp build
#node-gyp build --debug

# 9. rebuild
node-gyp rebuild

# 10. publish
npm publish


```

# References
> https://juejin.cn/post/6844903971220357134
> https://juejin.cn/post/6844904030162911240
