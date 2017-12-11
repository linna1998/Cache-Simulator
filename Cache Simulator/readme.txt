多层Cache模拟器
简介：
C++编写的cache模拟器。能够模拟cache的访问时间，miss次数等。

make：
命令行进入文件夹目录，输入make命令

使用方法：
1. --help进入帮助目录，可以查看详细帮助信息。
2. --name配置模拟文件的名字。例如--name=./01-mcf-gem5-xcg.trace.
	默认为--name=./01-mcf-gem5-xcg.trace.
3. --PFA配置预取算法。默认为Next-line prefetch algorithm.
	--PFA=0 means no prefetch algorithm.
	--PFA=1 means next-line prefetch algorithm.
	--PFA=2 means prefetch 2 lines.
	--PFA=3 means prefetch 4 lines.
4. --RA配置替换算法。默认为LRU.
	--RA=0 means LRU.
	--RA=1 means LIRS
5. --SF进入单步调试。默认不进入。
