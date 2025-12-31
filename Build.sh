### aide快捷编译工具
## 开发者信息
# *思路来源 FuckError11(@FuckError11)
# *更新优化 FuckError11(@FuckError11)
# *上一版本 雪屹(@C_XueYi)
##优化更新内容
# *自动遍历ndk-build(比上一个版本准确)
# *修改编译后内容为`MakeSuc`
##使用说明
# *若无法遍历到你的ndk-build，请如此操作 >> 
##发布声明
# *官方Telegram频道 @ErrorKernel
# *开发者TG @FuckError11 & @C_XueYi
# *修改/转发请保留开发者信息！
# *修改/删除开发者信息全家死光！
## 2024 09 21 13:35
### The following is the official script
[[ "$(id -u)" != "2000" && "$(id -u)" != "0" ]] && echo "\e[31m需求ROOT\e[0m" && exit

Pns=("com.idragoncheats.studio" "com.idragoncheats.studiopro" "com.goxome.aidestudio")  # 全部包名
Fd=()  # 找到的包名

for Pn in "${Pns[@]}"
do
    [ -e "/data/user/0/${Pn}/no_backup/ndksupport-1710240003/android-ndk-aide/ndk-build" ] && Fd+=("${Pn}")
done
[ ${#Fd[@]} -eq 0 ] && echo "\e[31m未找到ndk-build\e[0m" && exit 1
if [ ${#Fd[@]} != 1 ] ; then
    echo "\e[31m找到多个ndk-build，输入序号使用\e[0m"
    for i in "${!Fd[@]}"; do
        echo "\e[33m[$((i+1))]\e[90m ->\e[37m ${Fd[i]}\e[0m"
    done
    read Input
    if [[ $Input -ge 1 && $Input -le ${#Fd[@]} ]]; then
        UsePN="${Fd[$((Input-1))]}"
    else
        echo "\e[31m请输入1~${#Fd[@]}\e[0m"
        exit 1
    fi
else
    UsePN="${Fd[0]}"
fi

echo "\e[33m开始编译\e[0m"
/data/user/0/${UsePN}/no_backup/ndksupport-1710240003/android-ndk-aide/ndk-build 2>&1

[ $? -eq 0 ] && echo "\e[32mMake Success\e[0m" || { echo "\e[31mMake Failed\e[0m" && exit 1; }

tmp=/data/local/tmp
rm -rf ${tmp}/MakeSuc && mv ./libs/**/* ${tmp}/MakeSuc && chmod +x ${tmp}/MakeSuc
echo "\n\e[33m开始运行\e[0m\n"
${tmp}/MakeSuc