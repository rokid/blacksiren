# Siren模块设计概要

&ensp;
&ensp;
* **简介**

&emsp;&emsp;Siren是一个针对语音识别场景的前端算法模块，其负责从一个麦克风或麦克风阵列中读取pcm语音信号，然后将这些信号根据相关配置进行处理，将处理的结果通过元数据及语音数据的形式提供给用户。Siren由可移植体系无关模块，体系相关的算法模块以及平台相关的驱动模块组成。其中可移植的体系无关模块包括libsiren.so，libsiren\_jni.so，libsiren.jar组成。体系相关的前端音频算法模块包括libr2audio.so，libr2ssp.so，libr2vt.so，libztcodec2.so，libztvad.so。第一版本仅支持Android6.0及4.4版本，这部分模块包括了libsiren\_android.so，siren_android.bin和一个应用demo。


&emsp;&emsp;整个软件栈大概如下图所示：
![](http://i.imgur.com/k6WxgiA.png)



&ensp;
&ensp;

* **Siren核心模块**

&emsp;&emsp;Siren的核心模块由siren，siren jni接口，siren java接口组成，该模块依赖第三方库JSON解析库json-c和辅助库libeasyr2。Siren提供C++和java两个对外接口。  
&emsp;&emsp;Siren主要根据配置文件接收来自用户或麦克风阵列的以10ms为单位的音频PCM流，该PCM流采用的格式必须是配置文件中的InputRawPCMFormat字段所描述的格式。该PCM流将通过一个Record Thread输入到Siren内部，根据配置文件中的PreRawPCMProcess字段的规定来对InputRawPCMStream进行处理，处理后的PreProcessedRawStream将拷贝并转发到Voice Process Thread。之后Record Thread将PreProcessedRawPCMStream通过一个内存池中输出给用户，用户可以根据配置文件中的RawVoicePool字段来配置该内存池。  
&emsp;&emsp;Algorithm Process Thread将调用前端音频算法模块的接口对PreProcessedRawPCMStream进行降噪，波束成形，自身音源降噪，寻向，语音激活判断等处理。经过处理后的语音将得到语音事件VoiceEvent和语音数据VoiceData两个部分。其中VoiceEvent如果包含激活事件，则与此同时会进入WakeArbitration模块进行激活事件的局域网激活仲裁。VoiceEvent会通过Siren的接口回调通知到用户，VoiceData则会放入ProcessedVoiceMemoryPool，并将token和VoiceEvent一起交给用户。   

&emsp;&emsp;下图展示了Siren的整个流程：


![](http://i.imgur.com/wjdwTpz.png)



* **前端音频算法模块**

&emsp;&emsp;Siren的语音算法前端模块又libr2audio.so，libr2ssp.so，libr2vt.so，libztcodec2.so组成，第一版中该模块和具体平台有关，分为arm32和arm64两个版本。


* **Android平台的集成与移植**

&emsp;&emsp;针对Android平台，Siren需要通过native service或者jni来启动，如果通过native service的方式启动，需要在init.<device>.rc中添加：

    service siren /system/bin/siren
    	class main
		user root
		group root root

&emsp;&emsp;上述user和group权限可以根据具体情况进行调整。