单层Cache模拟器
简介：
C++编写的cache模拟器。能够模拟cache的访问时间，miss次数等。

make：
命令行进入文件夹目录，输入make命令

使用方法：
1. --help进入帮助目录，可以查看详细帮助信息。
2. --name配置模拟文件的名字。例如--name=./1.trace。默认为1.trace。
3. --s配置cache的Sets值。默认--s=5.
4. --e配置cache的组相连程度。默认--e=3.
5. --b配置cache一个block内部的bytes数。默认--b=6.
6. --TF配置Write Through策略。--BF配置Write Back策略。默认Write Back策略。
7. --AF配置Write Allocate策略。--NF配置Write Non-allocate策略。默认Write Allocate策略。
8. --SF进入单步调试。默认不进入。

