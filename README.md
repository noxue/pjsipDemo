# pjsipDemo
利用pjsip实现拨打电话


## 运行环境
centos6.9 gcc5.3 cmake3.0以上

## 制作过程中遇到的问题。
1. 获取rtp流。
2. 把rtp流取出头之后保存到文件 以为直接就是语音了。（搜资料加尝试一周之后发现 收到的pcma  或者pcmu 直接保存不能播放，需要转换一下，具体转换可以看 rtp.c 文件中的 on_rx_rtp函数  里面就调用了一个函数 pjmedia_alaw2linear）

欢迎同行交流，我也是个新手 QQ：173126019


