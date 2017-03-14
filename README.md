# BlackSiren

BlackSiren是一个针对嵌入式设备环境的前端降噪模块，这个模块目标将支持Android/Linux/Windows/Mac等多种嵌入式或桌面环境。BlackSiren现在正在开发状态，第一个版本将支持Android NDK/AOSP构建，请参考open-voice中的sample环节进行集成。

BlackSiren包括了NS(noise suppression)，AEC(acoustic echo cancellation)，BF(beam forming)，SL(sound location)，VAD(voice activity detection)，VT(voice triggler)等多种算法，支持数据流输出和语音流输出两种形式，其中数据流输出适合仅需要降噪不需要后续语音识别算法处理的场合，而语音流则是对应open-voice的后续语音处理，open-voice的speech模块需要使用opus编码后的语音流数据作为语音识别的输入。

BlackSiren支持2，4，6，8，10等多麦克风，16K-448000采样率，16bit，32bit的音频输入，数据流和语音流皆输出1路，16K，16bit PCM语音。BlackSiren提供一个open-voice专用的libopus库进行opus编码，具体例子可以参考open-voice的sample部分。

BlackSiren目前使用Android AOSP的集成方式，其驱动进程建议作为一个单独的native service注册在init.rc中。