#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#define HOTSPOT_SSID_PREFIX "TITA"
#define HOTSPOT_PASSWORD "12345678"
#define CMD_BUFFER_SIZE 512

// 获取设备序列号作为SSID后缀
const char* get_device_id() {
    static char id[32] = {0};
    FILE *fp = fopen("/proc/device-tree/serial-number", "r");
    if (fp) {
        size_t len = fread(id, 1, sizeof(id)-1, fp);
        fclose(fp);
        // 去除非打印字符
        for (size_t i = 0; i < len; i++) {
            if (id[i] < 32 || id[i] > 126) {
                id[i] = '\0';
                break;
            }
        }
        return (strlen(id) > 6) ? id + 6 : "";
    }
    return "";
}

// 安全执行系统命令并返回结果
int execute_command(const char *cmd) {
    return system(cmd);
}

// 禁用回显（用于密码输入）
void disable_echo() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

// 恢复回显
void enable_echo() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

// 开启WiFi并连接网络
void wifi_on() {
    printf("WiFi is turned on. Scanning for available networks (please wait 10 seconds)...\n");
    execute_command("nmcli radio wifi on");
    sleep(10);
    execute_command("nmcli device wifi rescan");
    execute_command("nmcli device wifi list");
    printf("\n");
    char ssid[128] = {0};
    char password[128] = {0};
    
    // 清除输入缓冲区
    while(getchar() != '\n');
    
    printf("please enter wifi id: ");
    fflush(stdout);
    if (fgets(ssid, sizeof(ssid), stdin)) {
        ssid[strcspn(ssid, "\n")] = '\0';
    }
    
    printf("please enter wifi password: ");
    fflush(stdout);
    // 直接显示密码输入（不移除回显）
    if (fgets(password, sizeof(password), stdin)) {
        password[strcspn(password, "\n")] = '\0';
    }
    printf("\n");
    
    char cmd[CMD_BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "nmcli device wifi connect \"%s\" password \"%s\"", 
             ssid, password);
    
    if (execute_command(cmd) != 0) {
        printf("Failed to connect to %s\n", ssid);
    } else {
        printf("Successfully connected to %s\n", ssid);
    }
}


// 关闭WiFi
void wifi_off() {
    execute_command("nmcli radio wifi off");
    printf("WiFi has been turned off.\n");
}

// 开启AP热点

void ap_on() {
    char ssid[128];
    const char *id = get_device_id();
    
    // 生成SSID名称
    if (id && *id) {
        snprintf(ssid, sizeof(ssid), "%s%s", HOTSPOT_SSID_PREFIX, id);
    } else {
        strncpy(ssid, HOTSPOT_SSID_PREFIX, sizeof(ssid));
    }

    // 强制清理旧配置
    execute_command("nmcli connection down Hotspot 2>/dev/null");
    execute_command("nmcli connection delete Hotspot 2>/dev/null");
    execute_command("nmcli device disconnect wlan0 2>/dev/null");

    // 创建新热点
    char cmd[CMD_BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd),
        "nmcli device wifi hotspot ifname wlan0 ssid \"%s\" password \"%s\"",
        ssid, HOTSPOT_PASSWORD);
    
    if (execute_command(cmd) == 0) {
        printf("Access Point mode has been turned on.\n");
        printf("SSID: %s, Password: %s\n", ssid, HOTSPOT_PASSWORD);
    } else {
        printf("Failed to create hotspot!\n");
    }
}

// 关闭AP热点
void ap_off() {
    execute_command("nmcli connection down Hotspot 2>/dev/null");
    printf("Access Point mode has been turned off.\n");
}

// 显示帮助信息
void show_help() {
    printf("Usage: wifi-app [option]\n");
    printf("Options:\n");
    printf("  -on       Turn on WiFi and connect to a network\n");
    printf("  -off      Turn off WiFi\n");
    printf("  -ap_on    Turn on Access Point mode\n");
    printf("  -ap_off   Turn off Access Point mode\n");
    printf("  -h        Show this help message\n");
}

int main(int argc, char *argv[]) {
    if (getuid() != 0) {
        printf("This program must be run as root. Try using sudo.\n");
        return EXIT_FAILURE;
    }
    
    if (system("which nmcli > /dev/null 2>&1") != 0) {
        printf("Error: nmcli is not installed. Please install NetworkManager.\n");
        return EXIT_FAILURE;
    }
    
    if (argc < 2) {
        show_help();
        return EXIT_FAILURE;
    }
    
    if (strcmp(argv[1], "-on") == 0) {
        wifi_on();
    } else if (strcmp(argv[1], "-off") == 0) {
        wifi_off();
    } else if (strcmp(argv[1], "-ap_on") == 0) {
        ap_on();
    } else if (strcmp(argv[1], "-ap_off") == 0) {
        ap_off();
    } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        show_help();
    } else {
        printf("Invalid option. Use -h for help.\n");
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}