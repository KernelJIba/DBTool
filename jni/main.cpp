#include <iostream>
#include <cstdlib>
#include <string>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

// ========== 全局配置 ==========
const std::string COLOR_RED     = "\033[31m";
const std::string COLOR_GREEN   = "\033[32m";
const std::string COLOR_YELLOW  = "\033[33m";
const std::string COLOR_BLUE    = "\033[34m";
const std::string COLOR_RESET   = "\033[0m";

// ========== 基础工具函数 ==========
void log(const std::string& msg, bool isError = false) {
    if (isError) {
        std::cout << COLOR_RED << "[ERROR] " << msg << COLOR_RESET << std::endl;
    } else {
        std::cout << COLOR_GREEN << "[INFO] " << msg << COLOR_RESET << std::endl;
    }
}

std::string execCmd(const std::string& cmd) {
    char buffer[256];
    std::string result = "";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        log("命令执行失败: " + cmd, true);
        return "";
    }
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

std::string getCurrentTime() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char buf[50];
    sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", 
            1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday,
            ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
    return std::string(buf);
}

void printSeparator(const std::string& title = "") {
    std::cout << COLOR_BLUE << "================================================" << COLOR_RESET << std::endl;
    if (!title.empty()) {
        std::cout << COLOR_YELLOW << "              " << title << "              " << COLOR_RESET << std::endl;
        std::cout << COLOR_BLUE << "===============================================" << COLOR_RESET << std::endl;
    }
}

// ========== VPN 检测核心函数 ==========
bool checkVirtualNet() {
    DIR* dir = opendir("/sys/class/net");
    if (!dir) return false;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string ifname = entry->d_name;
        if (ifname.find("tun") != std::string::npos || 
            ifname.find("tap") != std::string::npos ||
            ifname.find("vpn") != std::string::npos) {
            closedir(dir);
            return true;
        }
    }
    closedir(dir);
    return false;
}

bool checkProcNet() {
    std::ifstream tcp_file("/proc/net/tcp");
    if (!tcp_file.is_open()) return false;

    std::string line;
    getline(tcp_file, line);
    while (getline(tcp_file, line)) {
        if (line.find("0A") == std::string::npos && 
            line.find("AC10") == std::string::npos && 
            line.find("C0A8") == std::string::npos) {
            tcp_file.close();
            return true;
        }
    }
    tcp_file.close();
    return false;
}

bool checkSystemProxy() {
    std::ifstream proxy_file("/data/system/settings_secure.xml");
    if (!proxy_file.is_open()) return false;

    std::string line;
    while (getline(proxy_file, line)) {
        if (line.find("global_http_proxy_host") != std::string::npos &&
            line.find("global_http_proxy_port") != std::string::npos) {
            proxy_file.close();
            return true;
        }
    }
    proxy_file.close();
    return false;
}

void vpnDetect() {
    printSeparator("VPN 状态检测");
    bool is_vpn = false;
    std::string result = "";

    if (checkVirtualNet()) {
        is_vpn = true;
        result = COLOR_RED + "VPN 已连接 (检测到虚拟网卡 tun/tap/vpn)" + COLOR_RESET;
    } else if (checkProcNet()) {
        is_vpn = true;
        result = COLOR_RED + "VPN 已连接 (检测到非内网 TCP 连接)" + COLOR_RESET;
    } else if (checkSystemProxy()) {
        is_vpn = true;
        result = COLOR_RED + "VPN 已连接 (检测到系统代理配置)" + COLOR_RESET;
    } else {
        result = COLOR_GREEN + "未检测到 VPN 连接" + COLOR_RESET;
    }

    std::cout << result << std::endl;
    printSeparator();
}

// ========== 1. 文件管理功能（6个） ==========
void fileCopy(const std::string& src, const std::string& dst) {
    std::string cmd = "cp -rf " + src + " " + dst;
    log("执行复制: " + src + " -> " + dst);
    execCmd(cmd);
    log("复制完成");
}

void fileMove(const std::string& src, const std::string& dst) {
    std::string cmd = "mv -f " + src + " " + dst;
    log("执行移动: " + src + " -> " + dst);
    execCmd(cmd);
    log("移动完成");
} 

void fileDelete(const std::string& path) {
    std::string cmd = "rm -rf " + path;
    log("执行删除: " + path);
    execCmd(cmd);
    log("删除完成");
}

void fileChmod(const std::string& path, const std::string& perm) {
    std::string cmd = "chmod " + perm + " " + path;
    log("修改权限: " + path + " -> " + perm);
    execCmd(cmd);
    log("权限修改完成");
}

void fileChown(const std::string& path, const std::string& user, const std::string& group) {
    std::string cmd = "chown " + user + ":" + group + " " + path;
    log("修改所有者: " + path);
    execCmd(cmd);
    log("所有者修改完成");
}

void fileList(const std::string& path) {
    std::string cmd = "ls -lha " + path;
    printSeparator("目录内容: " + path);
    std::string res = execCmd(cmd);
    if (res.empty()) {
        std::cout << COLOR_RED << "目录为空或不存在" << COLOR_RESET << std::endl;
    } else {
        std::cout << res << std::endl;
    }
    printSeparator();
}

// ========== 2. 分区操作功能（7个） ==========
void partitionList() {
    printSeparator("分区列表");
    std::string res = execCmd("lsblk");
    if (res.empty()) {
        std::cout << COLOR_RED << "获取分区信息失败" << COLOR_RESET << std::endl;
    } else {
        std::cout << res << std::endl;
    }
    printSeparator();
}

void partitionMount(const std::string& dev, const std::string& mountPoint, const std::string& fsType) {
    std::string cmd = "su -c mount -t " + fsType + " " + dev + " " + mountPoint;
    log("挂载分区: " + dev + " -> " + mountPoint);
    execCmd(cmd);
    log("分区挂载完成");
}

void partitionUmount(const std::string& mountPoint) {
    std::string cmd = "su -c umount " + mountPoint;
    log("卸载分区: " + mountPoint);
    execCmd(cmd);
    log("分区卸载完成");
}

void partitionCheck(const std::string& dev) {
    std::string cmd = "su -c fsck -y " + dev;
    log("检查分区完整性: " + dev);
    execCmd(cmd);
    log("分区检查完成");
}

void partitionResize(const std::string& dev, const std::string& size) {
    std::string cmd = "su -c resize2fs " + dev + " " + size;
    log("调整分区大小: " + dev + " -> " + size);
    execCmd(cmd);
    log("分区大小调整完成");
} 

void partitionExtract(const std::string& dev, const std::string& imgPath) {
    std::string cmd = "su -c dd if=" + dev + " of=" + imgPath + " bs=4096 status=progress";
    log("提取分区镜像: " + dev + " -> " + imgPath);
    execCmd(cmd);
    log("分区镜像提取完成");
}

void partitionFlash(const std::string& imgPath, const std::string& dev) {
    std::string cmd = "su -c dd if=" + imgPath + " of=" + dev + " bs=4096 status=progress";
    log("刷入分区镜像: " + imgPath + " -> " + dev);
    execCmd(cmd);
    log("分区镜像刷入完成");
}

// ========== 3. Magisk 模块管理（4个） ==========
void magiskModuleInstall(const std::string& zipPath) {
    std::string cmd = "su -c magisk --install-module " + zipPath;
    log("安装Magisk模块: " + zipPath);
    execCmd(cmd);
    log("模块安装完成，需重启生效");
}

void magiskModuleUninstall(const std::string& moduleName) {
    std::string cmd = "su -c magisk --remove-module " + moduleName;
    log("卸载Magisk模块: " + moduleName);
    execCmd(cmd);
    log("模块卸载完成");
}

void magiskModuleList() {
    printSeparator("已安装Magisk模块");
    std::string res = execCmd("su -c magisk --list-modules");
    if (res.empty()) {
        std::cout << COLOR_YELLOW << "无已安装模块" << COLOR_RESET << std::endl;
    } else {
        std::cout << res << std::endl;
    }
    printSeparator();
}

void magiskModuleToggle(const std::string& moduleName, bool enable) {
    std::string cmd = enable ? 
        "su -c magisk --enable-module " + moduleName : 
        "su -c magisk --disable-module " + moduleName;
    std::string op = enable ? "启用" : "禁用";
    log(op + "Magisk模块: " + moduleName);
    execCmd(cmd);
    log("模块" + op + "完成");
}

// ========== 4. 系统优化功能（5个） ==========
void systemClearCache() {
    std::string cmd = "su -c rm -rf /cache/* /data/dalvik-cache/* /data/cache/*";
    log("清理系统缓存");
    execCmd(cmd);
    log("系统缓存清理完成");
}

void systemOptimizeMemory() {
    std::string cmd = "su -c echo 3 > /proc/sys/vm/drop_caches && echo 2048 > /sys/block/mmcblk0/queue/read_ahead_kb";
    log("优化内存占用");
    execCmd(cmd);
    log("内存优化完成");
}

void systemSetCpuGovernor(const std::string& governor) {
    std::string cmd = "su -c for cpu in /sys/devices/system/cpu/cpu[0-9]*/cpufreq/scaling_governor; do echo " + governor + " > $cpu; done";
    log("设置CPU调频策略为: " + governor);
    execCmd(cmd);
    log("CPU调频策略设置完成");
}

void systemDisableApp(const std::string& pkgName) {
    std::string cmd = "su -c pm disable " + pkgName;
    log("禁用系统应用: " + pkgName);
    execCmd(cmd);
    log("应用禁用完成");
}

void systemEnableApp(const std::string& pkgName) {
    std::string cmd = "su -c pm enable " + pkgName;
    log("启用系统应用: " + pkgName);
    execCmd(cmd);
    log("应用启用完成");
}

// ========== 5. 硬件调试功能（4个） ==========
void hardwareBatteryInfo() {
    printSeparator("电池状态信息");
    std::string res = execCmd("cat /sys/class/power_supply/battery/{capacity,health,status,voltage_now}");
    if (res.empty()) {
        std::cout << COLOR_RED << "获取电池信息失败" << COLOR_RESET << std::endl;
    } else {
        std::cout << res << std::endl;
    }
    printSeparator();
} 

void hardwareCpuTemp() {
    log("获取CPU温度信息");
    std::string temp = execCmd("cat /sys/class/thermal/thermal_zone0/temp");
    if (!temp.empty()) {
        float tempC = stof(temp) / 1000.0f;
        log("当前CPU温度: " + std::to_string(tempC) + "°C");
    } else {
        log("获取CPU温度失败", true);
    }
}

void hardwareTouchTest() {
    std::string cmd = "su -c getevent -lt /dev/input/event2";
    log("开始触控测试（按Ctrl+C退出）");
    execCmd(cmd);
}

void hardwareSetBrightness(int level) {
    if (level < 0 || level > 255) {
        log("亮度值超出范围（0-255）", true);
        return;
    }
    std::string cmd = "su -c echo " + std::to_string(level) + " > /sys/class/backlight/panel0-backlight/brightness";
    log("设置屏幕亮度为: " + std::to_string(level));
    execCmd(cmd);
    log("亮度设置完成");
}

// ========== 6. 备份恢复功能（4个） ==========
void backupAppData(const std::string& pkgName, const std::string& backupPath) {
    std::string cmd = "su -c mkdir -p " + backupPath + " && cp -rf /data/data/" + pkgName + " " + backupPath;
    log("备份应用数据: " + pkgName);
    execCmd(cmd);
    log("备份完成: " + backupPath + "/" + pkgName);
}

void restoreAppData(const std::string& pkgName, const std::string& backupPath) {
    std::string cmd = "su -c cp -rf " + backupPath + "/" + pkgName + " /data/data/";
    log("恢复应用数据: " + pkgName);
    execCmd(cmd);
    log("恢复完成");
}

void backupPartition(const std::string& dev, const std::string& imgPath) {
    std::string cmd = "su -c dd if=" + dev + " of=" + imgPath + " bs=4096 status=progress";
    log("备份分区镜像: " + dev + " -> " + imgPath);
    execCmd(cmd);
    log("分区镜像备份完成");
}

void restorePartition(const std::string& imgPath, const std::string& dev) {
    std::string cmd = "su -c dd if=" + imgPath + " of=" + dev + " bs=4096 status=progress";
    log("恢复分区镜像: " + imgPath + " -> " + dev);
    execCmd(cmd);
    log("分区镜像恢复完成");
}

// ========== 7. 系统属性功能（2个） ==========
void systemGetProp(const std::string& propName) {
    std::string cmd = "getprop " + propName;
    log("获取系统属性: " + propName);
    std::string res = execCmd(cmd);
    log(res.empty() ? "属性不存在" : "属性值: " + res);
}

void systemSetProp(const std::string& propName, const std::string& propValue) {
    std::string cmd = "su -c setprop " + propName + " " + propValue;
    log("设置系统属性: " + propName + "=" + propValue);
    execCmd(cmd);
    log("系统属性设置完成");
}

// ========== 8. 重启系统功能（1个） ==========
void systemReboot(const std::string& mode) {
    std::string cmd = mode == "recovery" ? "su -c reboot recovery" : 
                      mode == "bootloader" ? "su -c reboot bootloader" : "su -c reboot";
    std::string target = mode.empty() ? "正常模式" : mode;
    log("重启系统至: " + target);
    execCmd(cmd);
    log("重启指令已发送");
}

// ========== 9. 新增工具功能（4个） ==========
void systemGetCpuInfo() {
    printSeparator("CPU详细信息");
    std::string res = execCmd("cat /proc/cpuinfo");
    if (res.empty()) {
        std::cout << COLOR_RED << "获取CPU信息失败" << COLOR_RESET << std::endl;
    } else {
        std::cout << res << std::endl;
    }
    printSeparator();
}

void fileFind(const std::string& path, const std::string& fileName) {
    std::string cmd = "find " + path + " -name " + fileName;
    printSeparator("查找结果: " + fileName);
    std::string res = execCmd(cmd);
    if (res.empty()) {
        std::cout << COLOR_YELLOW << "未找到文件" << COLOR_RESET << std::endl;
    } else {
        std::cout << res << std::endl;
    }
    printSeparator();
}

void networkGetIp() {
    printSeparator("网络IP信息");
    std::string res = execCmd("ifconfig | grep inet");
    if (res.empty()) {
        std::cout << COLOR_RED << "获取IP失败" << COLOR_RESET << std::endl;
    } else {
        std::cout << res << std::endl;
    }
    printSeparator();
}

void systemKillProcess(const std::string& processName) {
    std::string cmd = "su -c killall " + processName;
    log("杀死进程: " + processName);
    execCmd(cmd);
    log("进程杀死完成");
}

// ========== 10. VAB 机型识别 + 线刷 ROM 功能（3个） ==========
bool isVabDevice() {
    log("检测VAB机型...");
    std::string res = execCmd("getprop ro.boot.vab");
    bool isVab = res.find("true") != std::string::npos || res.find("1") != std::string::npos;
    log(isVab ? "当前设备为 VAB 机型" : "当前设备为非 VAB 机型");
    return isVab;
}

void vabPartitionInfo() {
    if (!isVabDevice()) {
        log("非VAB机型，无法获取VAB分区信息", true);
        return;
    }
    printSeparator("VAB 分区信息");
    std::string res = execCmd("ls -l /dev/block/by-name/ | grep -E 'vbmeta|boot|system'");
    if (res.empty()) {
        std::cout << COLOR_RED << "获取VAB分区信息失败" << COLOR_RESET << std::endl;
    } else {
        std::cout << res << std::endl;
    }
    printSeparator();
}

void phoneToPhoneFlashRom(const std::string& romPath, const std::string& targetDevice) {
    if (!isVabDevice()) {
        log("非VAB机型不支持手机间线刷", true);
        return;
    }
    log("准备手机间线刷 ROM: " + romPath);
    log("请确保目标设备 " + targetDevice + " 已进入 Fastboot 模式并连接");
    
    std::string fastbootPath = "/data/local/tmp/fastboot";
    std::string checkFastboot = execCmd("test -f " + fastbootPath + " && echo exists");
    if (checkFastboot.empty()) {
        log("未找到 fastboot 二进制文件", true);
        return;
    }
    std::string cmd = "su -c " + fastbootPath + " flash boot " + romPath + "/boot.img && " +
                      fastbootPath + " flash system " + romPath + "/system.img && " +
                      fastbootPath + " flash vbmeta " + romPath + "/vbmeta.img";
    log("开始线刷 ROM 至目标设备...");
    execCmd(cmd);
    log("线刷 ROM 完成，请重启目标设备");
}

// ========== 11. VPN 检测子菜单 ==========
void vpnMenu() {
    vpnDetect();
    std::cout << COLOR_BLUE << "按任意键返回主菜单..." << COLOR_RESET;
    std::cin.ignore();
    std::cin.get();
}

// ========== 美化版菜单函数 ==========
void showMainMenu() {
    system("clear");
    printSeparator("DBTool v3.1 - " + getCurrentTime());
    std::cout << COLOR_YELLOW << "  1. 文件管理 (7)    2. 分区操作 (7)    3. Magisk模块 (4)" << COLOR_RESET << std::endl;
    std::cout << COLOR_YELLOW << "  4. 系统优化 (5)    5. 硬件调试 (4)    6. 备份恢复 (4)" << COLOR_RESET << std::endl;
    std::cout << COLOR_YELLOW << "  7. 系统属性 (2)    8. 重启系统 (1)    9. 新增工具 (4)" << COLOR_RESET << std::endl;
    std::cout << COLOR_YELLOW << "  10. VAB识别+线刷(3)  11. VPN状态检测  0. 退出程序" << COLOR_RESET << std::endl;
    printSeparator();
}

void fileManagerMenu() {
    printSeparator("文件管理子菜单");
    int opt;
    std::string src, dst, perm, user, group;
    std::cout << COLOR_YELLOW << "  1. 复制  2. 移动  3. 删除  4. 改权限  5. 改所有者  6. 列目录  7. 查找文件" << COLOR_RESET << std::endl;
    std::cout << "请选择功能: ";
    std::cin >> opt;
    switch (opt) {
        case 1:
            std::cout << "源路径: ";
            std::cin >> src;
            std::cout << "目标路径: ";
            std::cin >> dst;
            fileCopy(src, dst);
            break;
        case 2:
            std::cout << "源路径: ";
            std::cin >> src;
            std::cout << "目标路径: ";
            std::cin >> dst;
            fileMove(src, dst);
            break;
        case 3:
            std::cout << "文件路径: ";
            std::cin >> src;
            fileDelete(src);
            break;
        case 4:
            std::cout << "文件路径: ";
            std::cin >> src;
            std::cout << "权限(如755): ";
            std::cin >> perm;
            fileChmod(src, perm);
            break;
        case 5:
            std::cout << "文件路径: ";
            std::cin >> src;
            std::cout << "用户: ";
            std::cin >> user;
            std::cout << "组: ";
            std::cin >> group;
            fileChown(src, user, group);
            break;
        case 6:
            std::cout << "目录路径: ";
            std::cin >> src;
            fileList(src);
            break;
        case 7:
            std::cout << "查找根路径: ";
            std::cin >> src;
            std::cout << "文件名: ";
            std::cin >> dst;
            fileFind(src, dst);
            break;
        default:
            log("无效选项", true);
    }
    std::cout << COLOR_BLUE << "按任意键返回主菜单..." << COLOR_RESET;
    std::cin.ignore();
    std::cin.get();
}

void partitionMenu() {
    printSeparator("分区操作子菜单");
    int opt;
    std::string dev, mountPoint, fsType, size, imgPath;
    std::cout << COLOR_YELLOW << "  1. 列分区  2. 挂载  3. 卸载  4. 检查  5. 调整大小  6. 提取镜像  7. 刷入镜像" << COLOR_RESET << std::endl;
    std::cout << "请选择功能: ";
    std::cin >> opt;
    switch (opt) {
        case 1:
            partitionList();
            break;
        case 2:
            std::cout << "设备路径(如/dev/block/sda1): ";
            std::cin >> dev;
            std::cout << "挂载点: ";
            std::cin >> mountPoint;
            std::cout << "文件系统(如ext4): ";
            std::cin >> fsType;
            partitionMount(dev, mountPoint, fsType);
            break;
        case 3:
            std::cout << "挂载点: ";
            std::cin >> mountPoint;
            partitionUmount(mountPoint);
            break;
        case 4:
            std::cout << "设备路径: ";
            std::cin >> dev;
            partitionCheck(dev);
            break;
        case 5:
            std::cout << "设备路径: ";
            std::cin >> dev;
            std::cout << "目标大小(如10G): ";
            std::cin >> size;
            partitionResize(dev, size);
            break;
        case 6:
            std::cout << "设备路径: ";
            std::cin >> dev;
            std::cout << "镜像保存路径: ";
            std::cin >> imgPath;
            partitionExtract(dev, imgPath);
            break;
        case 7:
            std::cout << "镜像路径: ";
            std::cin >> imgPath;
            std::cout << "目标设备路径: ";
            std::cin >> dev;
            partitionFlash(imgPath, dev);
            break;
        default:
            log("无效选项", true);
    }
    std::cout << COLOR_BLUE << "按任意键返回主菜单..." << COLOR_RESET;
    std::cin.ignore();
    std::cin.get();
}

void magiskMenu() {
    printSeparator("Magisk模块子菜单");
    int opt;
    std::string path, name;
    bool enable;
    std::cout << COLOR_YELLOW << "  1. 安装  2. 卸载  3. 列模块  4. 切换状态" << COLOR_RESET << std::endl;
    std::cout << "请选择功能: ";
    std::cin >> opt;
    switch (opt) {
        case 1:
            std::cout << "模块ZIP路径: ";
            std::cin >> path;
            magiskModuleInstall(path);
            break;
        case 2:
            std::cout << "模块名称: ";
            std::cin >> name;
            magiskModuleUninstall(name);
            break;
        case 3:
            magiskModuleList();
            break;
        case 4:
            std::cout << "模块名称: ";
            std::cin >> name;
            std::cout << "启用(1)/禁用(0): ";
            std::cin >> enable;
            magiskModuleToggle(name, enable);
            break;
        default:
            log("无效选项", true);
    }
    std::cout << COLOR_BLUE << "按任意键返回主菜单..." << COLOR_RESET;
    std::cin.ignore();
    std::cin.get();
}

void systemOptimizeMenu() {
    printSeparator("系统优化子菜单");
    int opt;
    std::string pkg, governor;
    std::cout << COLOR_YELLOW << "  1. 清缓存  2. 内存优化  3. 设置CPU调频  4. 禁用应用  5. 启用应用  6. 查看CPU信息  7. 杀死进程" << COLOR_RESET << std::endl;
    std::cout << "请选择功能: ";
    std::cin >> opt;
    switch (opt) {
        case 1:
            systemClearCache();
            break;
        case 2:
            systemOptimizeMemory();
            break;
        case 3:
            std::cout << "调频策略(如performance/powersave): ";
            std::cin >> governor;
            systemSetCpuGovernor(governor);
            break;
        case 4:
            std::cout << "应用包名: ";
            std::cin >> pkg;
            systemDisableApp(pkg);
            break;
        case 5:
            std::cout << "应用包名: ";
            std::cin >> pkg;
            systemEnableApp(pkg);
            break;
        case 6:
            systemGetCpuInfo();
            break;
        case 7:
            std::cout << "进程名: ";
            std::cin >> pkg;
            systemKillProcess(pkg);
            break;
        default:
            log("无效选项", true);
    }
    std::cout << COLOR_BLUE << "按任意键返回主菜单..." << COLOR_RESET;
    std::cin.ignore();
    std::cin.get();
}

void hardwareMenu() {
    printSeparator("硬件调试子菜单");
    int opt, level;
    std::cout << COLOR_YELLOW << "  1. 电池信息  2. CPU温度  3. 触控测试  4. 设置亮度" << COLOR_RESET << std::endl;
    std::cout << "请选择功能: ";
    std::cin >> opt;
    switch (opt) {
        case 1:
            hardwareBatteryInfo();
            break;
        case 2:
            hardwareCpuTemp();
            break;
        case 3:
            hardwareTouchTest();
            break;
        case 4:
            std::cout << "亮度值(0-255): ";
            std::cin >> level;
            hardwareSetBrightness(level);
            break;
        default:
            log("无效选项", true);
    }
    std::cout << COLOR_BLUE << "按任意键返回主菜单..." << COLOR_RESET;
    std::cin.ignore();
    std::cin.get();
}

void backupMenu() {
    printSeparator("备份恢复子菜单");
    int opt;
    std::string pkg, path, dev, imgPath;
    std::cout << COLOR_YELLOW << "  1. 备份APP数据  2. 恢复APP数据  3. 备份分区镜像  4. 恢复分区镜像" << COLOR_RESET << std::endl;
    std::cout << "请选择功能: ";
    std::cin >> opt;
    switch (opt) {
        case 1:
            std::cout << "应用包名: ";
            std::cin >> pkg;
            std::cout << "备份路径: ";
            std::cin >> path;
            backupAppData(pkg, path);
            break;
        case 2:
            std::cout << "应用包名: ";
            std::cin >> pkg;
            std::cout << "备份路径: ";
            std::cin >> path;
            restoreAppData(pkg, path);
            break;
        case 3:
            std::cout << "设备路径: ";
            std::cin >> dev;
            std::cout << "镜像保存路径: ";
            std::cin >> imgPath;
            backupPartition(dev, imgPath);
            break;
        case 4:
            std::cout << "镜像路径: ";
            std::cin >> imgPath;
            std::cout << "目标设备路径: ";
            std::cin >> dev;
            restorePartition(imgPath, dev);
            break;
        default:
            log("无效选项", true);
    }
    std::cout << COLOR_BLUE << "按任意键返回主菜单..." << COLOR_RESET;
    std::cin.ignore();
    std::cin.get();
}

void propMenu() {
    printSeparator("系统属性子菜单");
    int opt;
    std::string prop, value;
    std::cout << COLOR_YELLOW << "  1. 获取属性  2. 设置属性  3. 查看IP地址" << COLOR_RESET << std::endl;
    std::cout << "请选择功能: ";
    std::cin >> opt;
    switch (opt) {
        case 1:
            std::cout << "属性名(如ro.build.version.release): ";
            std::cin >> prop;
            systemGetProp(prop);
            break;
        case 2:
            std::cout << "属性名: ";
            std::cin >> prop;
            std::cout << "属性值: ";
            std::cin >> value;
            systemSetProp(prop, value);
            break;
        case 3:
            networkGetIp();
            break;
        default:
            log("无效选项", true);
    }
    std::cout << COLOR_BLUE << "按任意键返回主菜单..." << COLOR_RESET;
    std::cin.ignore();
    std::cin.get();
}

void rebootMenu() {
    printSeparator("重启系统子菜单");
    int opt;
    std::cout << COLOR_YELLOW << "  1. 正常重启  2. 重启到Recovery  3. 重启到Bootloader" << COLOR_RESET << std::endl;
    std::cout << "请选择功能: ";
    std::cin >> opt;
    switch (opt) {
        case 1:
            systemReboot("");
            break;
        case 2:
            systemReboot("recovery");
            break;
        case 3:
            systemReboot("bootloader");
            break;
        default:
            log("无效选项", true);
    }
}

void newToolsMenu() {
    printSeparator("新增工具子菜单");
    int opt;
    std::string path, name, proc;
    std::cout << COLOR_YELLOW << "  1. 查看CPU信息  2. 查找文件  3. 查看IP地址  4. 杀死进程" << COLOR_RESET << std::endl;
    std::cout << "请选择功能: ";
    std::cin >> opt;
    switch (opt) {
        case 1:
            systemGetCpuInfo();
            break;
        case 2:
            std::cout << "查找路径: ";
            std::cin >> path;
            std::cout << "文件名: ";
            std::cin >> name;
            fileFind(path, name);
            break;
        case 3:
            networkGetIp();
            break;
        case 4:
            std::cout << "进程名: ";
            std::cin >> proc;
            systemKillProcess(proc);
            break;
        default:
            log("无效选项", true);
    }
    std::cout << COLOR_BLUE << "按任意键返回主菜单..." << COLOR_RESET;
    std::cin.ignore();
    std::cin.get();
}

void vabFlashMenu() {
    printSeparator("VAB识别+线刷子菜单");
    int opt;
    std::string romPath, targetDevice;
    std::cout << COLOR_YELLOW << "  1. 检测VAB机型  2. 查看VAB分区信息  3. 手机间线刷ROM" << COLOR_RESET << std::endl;
    std::cout << "请选择功能: ";
    std::cin >> opt;
    switch (opt) {
        case 1:
            isVabDevice();
            break;
        case 2:
            vabPartitionInfo();
            break;
        case 3:
            std::cout << "ROM 文件夹路径: ";
            std::cin >> romPath;
            std::cout << "目标设备标识(如usb-0): ";
            std::cin >> targetDevice;
            phoneToPhoneFlashRom(romPath, targetDevice);
            break;
        default:
            log("无效选项", true);
    }
    std::cout << COLOR_BLUE << "按任意键返回主菜单..." << COLOR_RESET;
    std::cin.ignore();
    std::cin.get();
}

// ========== 主函数 ==========
int main() {
    int mainOpt;
    while (true) {
        showMainMenu();
        std::cout << "请选择主功能: ";
        std::cin >> mainOpt;
        if (mainOpt == 0) {
            printSeparator("感谢使用");
            log("工具箱已退出");
            break;
        }
        switch (mainOpt) {
            case 1:
                fileManagerMenu();
                break;
            case 2:
                partitionMenu();
                break;
            case 3:
                magiskMenu();
                break;
            case 4:
                systemOptimizeMenu();
                break;
            case 5:
                hardwareMenu();
                break;
            case 6:
                backupMenu();
                break;
            case 7:
                propMenu();
                break;
            case 8:
                rebootMenu();
                break;
            case 9:
                newToolsMenu();
                break;
            case 10:
                vabFlashMenu();
                break;
            case 11:
                vpnMenu();
                break;
            default:
                log("无效主功能选项", true);
        }
    }
    return 0;
}
