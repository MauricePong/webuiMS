1.通过nfs，将conf_upload.cgi放入编码器系统文件路径/www/cgi-bin/中

2.通过nfs，将scanfip 放入编码系统文件中，并配置成开机自启动，或者测试先启动 ./scanfip

3.注意 主机pc端 ip网段与编码器网段不一致，webuiM将无法自动获取编码器IP;

4.注意 默认支持opengl2.0以上gpu渲染webui，支持大部分主流主机设备，
主流系统（win7以上），支持跨平台编译（windows,linux,mac）

5.webuiM 功能:支持浏览器基本功能(访问网址浏览网页,调试web)；

	      定制功能，一键自动获取编码器设备ip和手动修改ip,
	      
	      一键切换编码器设备webui，一键备份当前webui配置文件，
	      
	      一键上传配置文件，一键流媒体视频播放，一键流媒体视频录制，
	      
	      支持打开pdf,html,js，css,txt等文本，支持jpeg，png等格式图片打开，
	      
	      支持打开ts，mp4,flv等大部分格式视频播放。
			
6.源码获取：git clone https://github.com/MauricePong/webuiMS.git

7.发布安装包路径：\\192.168.8.147\Product\webuiM\ver01
