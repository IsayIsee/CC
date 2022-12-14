/*
 * Copyright 2002-2014 the original author or authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      CC/LICENSE
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <switch.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <switch_core.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <cstring>
#include <fstream>
#include <map>
#include <vector>
#include <sstream>
#include <algorithm>

#define MAX_EMAIL_LEN 1024
#define MAX_PHONE_LEN 1024
#define MAX_LINE_LEN  1024

#define CONF_SECTION_IVRSERVER "IVRServers"
#define CONF_SECTION_INTERVAL  "CheckInterval"
#define CONF_SECTION_TIMEOUT   "TimeOut"
#define CONF_SECTION_RULE      "Rule"
#define CONF_SECTION_ALARM     "Alarm"
#define CONF_KEY_IVRBASE       "IVR"
#define CONF_KEY_IVRDEFAULT    "IVR_DEFAULT"
#define CONF_KEY_INTERVAL      "INTERVAL"
#define CONF_KEY_CON_TIMEOUT   "CON_TIMEOUT"
#define CONF_KEY_CPU_THRESHOLD "CPU_THR"
#define CONF_KEY_EMAIL         "EMAIL"
#define CONF_KEY_PHONE         "PHONE"

const char* g_config_path = "../conf/ivr_loadbalance/ivr_loadbalance.conf";

/******************************/
/*** IVR Servers info table ***/
/******************************/
typedef struct {
    int cpu;                 //CPU info
    int chan_num;            //Channel num in used
    bool is_available;       //Server is available? It is false while checking
} server_info_t;
//IVR Servers info map
std::map <std::string, server_info_t> g_ip_svrinfo;

/***********************************/
/*** Something about config file ***/
/***********************************/
typedef struct ip_info_t {
    std::string ip_total;    //IP address (IP+port)
    std::string ip;          //IP address (x.x.x.x)
    uint32_t port;           //Port num
} ip_info_t;

typedef struct conf_info_t {
    std::vector <ip_info_t> vec_ivr_ip;      //IP address of IVR Servers
    ip_info_t default_ivr_ip;                //IP address of default IVR Server
    int32_t ivr_num;                         //Number of IVR Servers
    int32_t interval_time;                   //Interval time of Checking IVR Server(ms)
    int32_t connect_timeout;                 //Timeout of connecting to IVR Server
    int32_t cpu_threshold;                   //Decision threshold
    char str_email[MAX_EMAIL_LEN + 1];       //Alarm email list
    char str_phone[MAX_PHONE_LEN + 1];       //Alarm phone list (sms)
} conf_info_t;
//Config file info
conf_info_t g_conf_info;

//Config file map for loading
typedef std::map <std::string, std::string> section_map;
typedef std::map <std::string, section_map> file_map;

/**
 * @brief ????????????
 *
 * @param [in] file_path : ????????????
 * @return  ????????/????
**/
bool load_file(const char* file_path);

/**
 * @brief ??????????????????????
 *
 * @param [in] filemap : ??????????
 * @param [in] section_name : ??????section????
 * @param [in] key_name : ??????key????
 * @param [in] default_value : ??????
 * @return ??????????
**/
int32_t get_int(file_map filemap, const char* section_name, const char* key_name,
                int32_t default_value);

/**
 * @brief ????????????????????????
 *
 * @param [in] filemap : ??????????
 * @param [in] section_name : ??????section????
 * @param [in] key_name : ??????key????
 * @param [in] default_value : ??????
 * @param [out] dest_buff : ??????????????????????
 * @param [in] buff_size : ??????????
 * @return ??????????????????-1??????
**/
int32_t get_string(file_map filemap, const char* section_name, const char* key_name,
                   const char* default_value, char* dest_buff, uint32_t buff_size);

/**
 * @brief ??????????????????????????????(g_conf_info)
 *
 * @param [in] filemap : ??????????
 * @return ????????/????
**/
bool load_to_struct(file_map filemap);


/**
 * @brief ??(IP:port)??????????IP??port????????????????????????????????????
 *
 * @param [in] ip_addr : IP????
 * @param [out] ip : ????????IP????
 * @param [out] port : ????????port
 * @return ????????/????
**/
bool splitIP(const char* ip_addr, char* ip, uint32_t& port);

/**
 * @brief ??????????????????
 *
 * @param [out] conf_path : ??????????????????
 * @param [in] conf_path_size : ??????????
 * @param [in] conf_file : ????????
 * @return ????????/????
**/
bool get_conf_path(char* conf_path, uint32_t conf_path_size, const char* conf_file);

/**********************************/
/*** Something about IVR Server ***/
/**********************************/
//Return value of checking IVR Server
enum IVR_CHECK_RET {
    IVR_SUCCESS         =  0,    //????
    IVR_CREATE_SOCK_ERR = -1,    //????socket????
    IVR_CONNECT_ERR     = -2,    //????????
    IVR_CONNECT_TIMEOUT = -3,    //????
    IVR_RECV_CONN_ERR1  = -4,    //????"connected\n\n"??????????????????????????
    IVR_RECV_CONN_ERR2  = -5,    //????"connected\n\n"??????????recv????
    IVR_SEND_ERR        = -6,    //????????????
    IVR_RECV_INFO_ERR1  = -7,    //????????????????????????????????????
    IVR_RECV_INFO_ERR2  = -8,    //????????????????????recv????
};
//string of IVR_CHECK_RET. For alarm mail.
const char* g_ivr_check_ret_str = " 0:????\n \
-1:????socket????\n \
-2:????????\n \
-3:????????\n \
-4:????\"connect\"??????????????????????????\n \
-5:????\"connect\"??????????recv????\n \
-6:????????????\n \
-7:????????????????????????????????????\n \
-8:????????????????????recv????\n";

//No IVR Alarm Tag
bool g_no_ivr_alarm = false;

/**
 * @brief ????????????????????????
 *
 * @return
**/
void check_ivr_main();

/**
 * @brief ????IVR Server??????????interval_time????????????????????????????????????????
 *
 * @param [in] : ????????????IVR Server??IP????
 * @return
**/
void* check_ivr_pthread(void* ip_info_t);

/**
 * @brief ????IVR Server
 *
 * @param [in] ip_addr : ??????IP????????port????
 * @param [in] port : ??????port????
 * @param [out] cpu : IVR Server??CPU????
 * @param [out] chan_num : IVR Server????????????
 * @return 0-??????????-????
**/
int32_t check_ivr_server(const char* ip_addr, uint32_t port, int32_t* cpu, int32_t* chan_num);

/**
 * @brief ????????????????IVR Server??????????"????????"??Server??????????IVR_Host????
 *        ??????????????????????IVR??????
 *
 * @return
**/
void decision_ivr_server();

/*******************************/
/*** Something about threads ***/
/*******************************/

//If the thread continue running
bool g_mod_running = true;
//Thread ID
pthread_t* g_ptid;

/**
 * @brief ??????????????????????
 *
 * @return
**/
void exit_threads();

/*****************************/
/*** Something about alarm ***/
/*****************************/

/**
 * @brief ????????
 *
 * @param [in] title : ????
 * @param [in] content : ????????
 * @param [in] emaillist : ??????????
 * @return ????????/????
**/
bool alarm_by_email(std::string& title, std::string& content, const std::string& emaillist);

/**
 * @brief ????????
 *
 * @param [in] content : ????????
 * @param [in] smslist : ??????????
 * @return ????????/????
**/
bool alarm_by_sms(std::string& content, const ::std::string& smslist);

/* Prototypes */
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_ivr_loadbalance_shutdown);
SWITCH_MODULE_RUNTIME_FUNCTION(mod_ivr_loadbalance_runtime);
SWITCH_MODULE_LOAD_FUNCTION(mod_ivr_loadbalance_load);

/* SWITCH_MODULE_DEFINITION(name, load, shutdown, runtime)
 * Defines a switch_loadable_module_function_table_t and a static const char[] modname
 */
SWITCH_MODULE_DEFINITION(mod_ivr_loadbalance, mod_ivr_loadbalance_load,
                         mod_ivr_loadbalance_shutdown, mod_ivr_loadbalance_runtime);

//Macro expands to: switch_status_t mod_ivr_loadbalance_load(switch_loadable_module_interface_t **module_interface, switch_memory_pool_t *pool)
SWITCH_MODULE_LOAD_FUNCTION(mod_ivr_loadbalance_load) {
    // connect my internal structure to the blank pointer passed to me
    //??????????FS??????????????????????
    *module_interface = switch_loadable_module_create_module_interface(pool, modname);

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                      "[IVR_LB]==== LOAD IVR_LOADBALANCE START ====\n");
    char confFile[MAX_LINE_LEN + 1] = {0};

    if (!get_conf_path(confFile, MAX_LINE_LEN, g_config_path)) {
        return SWITCH_STATUS_FALSE;
    }

    if (!load_file(confFile)) {
        return SWITCH_STATUS_FALSE;
    }

    g_ptid = (pthread_t*)malloc(sizeof(pthread_t) * g_conf_info.ivr_num);

    if (NULL == g_ptid) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[IVR_LB]MALLOC FOR PID ERR:num=%d\n",
                          g_conf_info.ivr_num);
        return SWITCH_STATUS_FALSE;
    }

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                      "[IVR_LB]==== LOAD IVR_LOADBALANCE END ====\n");

    return SWITCH_STATUS_SUCCESS;
}
//Macro expands to: switch_status_t mod_ivr_loadbalance_runtime(switch_loadable_module_interface_t **module_interface, switch_memory_pool_t *pool)
SWITCH_MODULE_RUNTIME_FUNCTION(mod_ivr_loadbalance_runtime) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                      "[IVR_LB]==== RUNTIME IVR_LOADBALANCE START ====\n");
    g_mod_running = true;
    check_ivr_main();
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                      "[IVR_LB]==== RUNTIME IVR_LOADBALANCE CREATE THREADS END ====\n");
    return SWITCH_STATUS_TERM;
}
//Macro expands to: switch_status_t mod_ivr_loadbalance_shutdown(switch_loadable_module_interface_t **module_interface, switch_memory_pool_t *pool)
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_ivr_loadbalance_shutdown) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                      "[IVR_LB]==== SHUTDOWN IVR_LOADBALANCE START ====\n");
    g_mod_running = false;
    //Wait for threads exit
    exit_threads();

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                      "[IVR_LB]==== RUNTIME IVR_LOADBALANCE END ====\n");
    //free g_ptid
    free(g_ptid);

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                      "[IVR_LB]==== SHUTDOWN IVR_LOADBALANCE END ====\n");
    return SWITCH_STATUS_SUCCESS;
}


/**
 * @brief ????????????
 *
 * @param [in] file_path : ????????????
 * @return  ????????/????
**/
bool load_file(const char* file_path) {
    file_map fileMap;

    //Find all options in config file
    std::ifstream fp(file_path);

    if (fp.fail()) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "[IVR_LB]LOAD CONF FILE ERR:path=%s\n",
                          file_path);
        return false;
    }

    char szbuf[MAX_LINE_LEN + 1];
    std::string strLine;
    bool hasRead = false;

    while (!fp.eof() && !fp.fail()) {
        if (!hasRead) {
            fp.getline(szbuf, MAX_LINE_LEN);

            if ('#' == szbuf[0] || ';' == szbuf[0] || '\0' == szbuf[0]) {
                continue;
            }

            strLine = szbuf;
        } else {
            hasRead = false;
        }

        uint32_t firstPos = strLine.find_first_not_of(' ');
        uint32_t lastPos = strLine.find_last_not_of(' ');

        if (strLine[firstPos] != '[' || strLine[lastPos] != ']' || lastPos - firstPos <= 1) {
            continue;
        }

        std::string strSection = strLine.substr(firstPos + 1, lastPos - firstPos - 1);
        std::transform(strSection.begin(), strSection.end(), strSection.begin(), ::tolower);

        section_map sectionMap;
        fileMap[strSection] = sectionMap;
        section_map& pSectionMap = fileMap[strSection];

        while (!fp.eof() && !fp.fail()) {
            fp.getline(szbuf, MAX_LINE_LEN);

            if ('#' == szbuf[0] || ';' == szbuf[0] || '\0' == szbuf[0])
                if (szbuf[0] == '#' || szbuf[0] == ';' || szbuf[0] == '\0') {
                    continue;
                }

            strLine = szbuf;

            firstPos = strLine.find_first_not_of(' ');
            lastPos = strLine.find_last_not_of(' ');
            int32_t pos0 = strLine.find_first_of('=');

            if (strLine[firstPos] == '=' || lastPos - firstPos <= 1) {
                continue;
            }

            if (strLine[firstPos] == '[') {
                hasRead = true;
                break;
            }

            if (-1 == pos0) {
                continue;
            }

            int32_t pos = strLine.find_last_not_of(' ', pos0 - 1);
            std::string strKey = strLine.substr(firstPos, pos - firstPos + 1);
            std::transform(strKey.begin(), strKey.end(), strKey.begin(), ::tolower);
            pos = strLine.find_first_not_of(' ', pos0 + 1);
            std::string strVal("");

            if (-1 == pos) {
                strVal = "";
            } else {
                strVal = strLine.substr(pos);
            }

            pSectionMap[strKey] = strVal;
        }
    }//while(!fp.eof() && !fp.fail())

    fp.close();

    //Get values into struct
    //g_conf_info.interval_time = get_int(fileMap, "CheckInterval", "INTERVAL", 500);
    if (!load_to_struct(fileMap)) {
        return false;
    }

    switch_core_set_variable("IVR_HOST", g_conf_info.default_ivr_ip.ip_total.c_str());

    return true;
}


/**
 * @brief ??????????????????????????????(g_conf_info)
 *
 * @param [in] filemap : ??????????
 * @return ????????/????
**/
bool load_to_struct(file_map filemap) {
    char tmpIP[MAX_LINE_LEN] = {0};
    char tmpIP2[20] = {0};
    char keyIVRServer[10] = {0};
    uint32_t port = 0;
    bool ret = true;

    //Get Default IP Address
    get_string(filemap, CONF_SECTION_IVRSERVER, CONF_KEY_IVRDEFAULT, "", tmpIP, MAX_EMAIL_LEN);

    if (splitIP(tmpIP, tmpIP2, port)) {
        g_conf_info.default_ivr_ip.ip_total = tmpIP;
        g_conf_info.default_ivr_ip.ip = tmpIP2;
        g_conf_info.default_ivr_ip.port = port;
    } else {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "[IVR_LB]CONF DEFAULT IP WRONG:ip=%s\n",
                          tmpIP);
        ret = false;
    }

    //Get IVR Servers IP Address
    g_conf_info.vec_ivr_ip.clear();
    int32_t ivrNum = 1;

    do {
        tmpIP[0] = '\0';
        sprintf(keyIVRServer, "%s%d", CONF_KEY_IVRBASE, ivrNum);

        if (-1 == get_string(filemap, CONF_SECTION_IVRSERVER, keyIVRServer, "", tmpIP, MAX_EMAIL_LEN)) {
            break;
        }

        if (splitIP(tmpIP, tmpIP2, port)) {
            //Put IP info into conf struct
            ip_info_t ipInfo;
            ipInfo.ip_total = tmpIP;
            ipInfo.ip = tmpIP2;
            ipInfo.port = port;
            g_conf_info.vec_ivr_ip.push_back(ipInfo);

            //Create Server info
            server_info_t serverInfo;
            serverInfo.is_available = false;
            g_ip_svrinfo[tmpIP] = serverInfo;
        } else {
            //Log
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "[IVR_LB]CONF IP WRONG:ip%d=%s\n", ivrNum,
                              tmpIP);
            ret = false;
            break;
        }

        ivrNum++;
    } while (true);

    g_conf_info.ivr_num = ivrNum - 1;

    if (0 == g_conf_info.ivr_num) {
        //??????IVR????????????????
        //Log & Alarm
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "[IVR_LB]NO IVR IN CONF\n");
        std::string title = "[ivr_loadbalance????]????????????????IVR??????";
        std::string content = "[ivr_loadbalance????]????????????????IVR??????";
        alarm_by_email(title, content, g_conf_info.str_email);
        alarm_by_sms(content, g_conf_info.str_phone);
        ret = false;
    }

    //Get Other Options
    int32_t tmpNum = get_int(filemap, CONF_SECTION_INTERVAL, CONF_KEY_INTERVAL, 1000);
    g_conf_info.interval_time = (tmpNum > 0) ? tmpNum : 1000;
    tmpNum = get_int(filemap, CONF_SECTION_TIMEOUT, CONF_KEY_CON_TIMEOUT, 2000);
    g_conf_info.connect_timeout = (tmpNum > 0) ? tmpNum : 2000;
    tmpNum = get_int(filemap, CONF_SECTION_RULE, CONF_KEY_CPU_THRESHOLD, 70);
    g_conf_info.cpu_threshold = (tmpNum > 0) ? tmpNum : 70;
    get_string(filemap, CONF_SECTION_ALARM, CONF_KEY_EMAIL, "", g_conf_info.str_email, MAX_EMAIL_LEN);
    get_string(filemap, CONF_SECTION_ALARM, CONF_KEY_PHONE, "", g_conf_info.str_phone, MAX_PHONE_LEN);

    //Config Log Output
    std::vector <ip_info_t>::iterator iter = g_conf_info.vec_ivr_ip.begin();

    for (; iter != g_conf_info.vec_ivr_ip.end(); iter++) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[IVR_LB]CONF:ip=%s,port=%d\n",
                          (*iter).ip.c_str(), (*iter).port);
        server_info_t serverInfo;

    }

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[IVR_LB]CONF:ivr_num=%d\n",
                      g_conf_info.ivr_num);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[IVR_LB]CONF:Default_ip=%s,port=%d\n",
                      g_conf_info.default_ivr_ip.ip.c_str(), g_conf_info.default_ivr_ip.port);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[IVR_LB]CONF:interval=%d\n",
                      g_conf_info.interval_time);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[IVR_LB]CONF:timeout=%d\n",
                      g_conf_info.connect_timeout);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[IVR_LB]CONF:cputhr=%d\n",
                      g_conf_info.cpu_threshold);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[IVR_LB]CONF:email=%s\n",
                      g_conf_info.str_email);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[IVR_LB]CONF:phone=%s\n",
                      g_conf_info.str_phone);
    return ret;
}

/**
 * @brief ??(IP:port)??????????IP??port????????????????????????????????????
 *
 * @param [in] ip_addr : IP????
 * @param [out] ip : ????????IP????
 * @param [out] port : ????????port
 * @return ????????/????
**/
bool splitIP(const char* ip_addr, char* ip, uint32_t& port) {
    std::string strIPPort = ip_addr;
    int32_t pos0 = strIPPort.find_first_of(':');

    if (-1 == pos0 || 0 == pos0) {
        return false;
    }

    std::string strIP = strIPPort.substr(0, pos0);
    strncpy(ip, strIP.c_str(), 20);
    int32_t pos = strIPPort.find_first_not_of(' ', pos0 + 1);

    if (-1 == pos) {
        return false;
    }

    port = atoi(strIPPort.substr(pos).c_str());

    if (0 == port) {
        return false;
    }

    return true;
}

/**
 * @brief ??????????????????????
 *
 * @param [in] filemap : ??????????
 * @param [in] section_name : ??????section????
 * @param [in] key_name : ??????key????
 * @param [in] default_value : ??????
 * @return ??????????
**/
int32_t get_int(file_map filemap, const char* section_name, const char* key_name,
                int32_t default_value) {
    int32_t ret = default_value;
    std:: string strTmp;

    strTmp = section_name;
    std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::tolower);

    if (filemap.find(strTmp) == filemap.end()) {
        return ret;
    }

    section_map& pSectionMap = filemap[strTmp];
    strTmp = key_name;
    std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::tolower);

    if (pSectionMap.find(strTmp) == pSectionMap.end()) {
        return ret;
    }

    ret = atoi(pSectionMap[strTmp].c_str());
    return ret;
}

/**
 * @brief ????????????????????????
 *
 * @param [in] filemap : ??????????
 * @param [in] section_name : ??????section????
 * @param [in] key_name : ??????key????
 * @param [in] default_value : ??????
 * @param [out] dest_buff : ??????????????????????
 * @param [in] buff_size : ??????????
 * @return ??????????????????-1??????
**/
int32_t get_string(file_map filemap, const char* section_name, const char* key_name,
                   const char* default_value, char* dest_buff, uint32_t buff_size) {
    std:: string strTmp;

    strTmp = section_name;
    std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::tolower);

    if (filemap.find(strTmp) == filemap.end()) {
        strncpy(dest_buff, default_value, buff_size - 1);
        return -1;
    }

    section_map& pSectionMap = filemap[strTmp];
    strTmp = key_name;
    std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::tolower);

    if (pSectionMap.find(strTmp) == pSectionMap.end()) {
        strncpy(dest_buff, default_value, buff_size - 1);
        return -1;
    }

    std::string& strVal = pSectionMap[strTmp];
    uint32_t ret = strVal.length();

    if (buff_size <= ret) {
        strncpy(dest_buff, strVal.c_str(), buff_size - 1);
        ret = buff_size - 1;
    } else {
        strncpy(dest_buff, strVal.c_str(), buff_size);
        ret = strlen(dest_buff);
    }

    return ret;
}

/**
 * @brief ??????????????????
 *
 * @param [out] conf_path : ??????????????????
 * @param [in] conf_path_size : ??????????
 * @param [in] conf_file : ????????
 * @return ????????/????
**/
bool get_conf_path(char* conf_path, uint32_t conf_path_size, const char* conf_file) {
    char proc[MAX_LINE_LEN + 1] = {0};
    char basePath[MAX_LINE_LEN + 1] = {0};
    int32_t count;

    snprintf(proc, sizeof(proc), "/proc/%u/exe", (uint32_t)getpid());
    count = readlink(proc, basePath, MAX_LINE_LEN);

    if (count < 0 || count >= MAX_LINE_LEN) {
        memset(conf_path, conf_path_size, 0);
        return false;
    } else {
        basePath[count] = '\0';
    }

    dirname(basePath);
    snprintf(conf_path, conf_path_size, "%s/%s", basePath, conf_file);
    return true;
}
/**
 * @brief ????????????????????????
 *
 * @return
**/
void check_ivr_main() {
    //Check all IVR servers, update the info
    std::vector <ip_info_t>::iterator iter = g_conf_info.vec_ivr_ip.begin();

    for (; iter != g_conf_info.vec_ivr_ip.end(); iter++) {
        ip_info_t tmpIPInfo = *iter;
        int32_t cpu = 0;
        int32_t chanNum = 0;
        int32_t ivrret = check_ivr_server(tmpIPInfo.ip.c_str(), tmpIPInfo.port, &cpu, &chanNum);

        if (0 == ivrret) {
            g_ip_svrinfo[tmpIPInfo.ip_total.c_str()].cpu = cpu;
            g_ip_svrinfo[tmpIPInfo.ip_total.c_str()].chan_num = chanNum;
            g_ip_svrinfo[tmpIPInfo.ip_total.c_str()].is_available = true;
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[IVR_LB]IVR OK:ip=%s,cpu=%d,chan=%d\n",
                              (*iter).ip_total.c_str(), cpu, chanNum);
        } else {
            g_ip_svrinfo[tmpIPInfo.ip_total.c_str()].is_available = false;
            //Log & Alarm
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "[IVR_LB]IVR ERR:ip=%s,errno=%d\n",
                              (*iter).ip_total.c_str(), ivrret);
            std::string title = "[ivr_loadbalance????]IVR??????????????????????IP=";
            title += (*iter).ip_total;
            std::string content;
            std::stringstream ss;
            ss << "[ivr_loadbalance????]IVR??????????????????????IP=" << (*iter).ip_total << "????????=" <<
               ivrret;
            ss >> content;
            content += "\n??????????\n";
            content += g_ivr_check_ret_str;
            alarm_by_email(title, content, g_conf_info.str_email);
            alarm_by_sms(content, g_conf_info.str_phone);
        }
    }

    decision_ivr_server();

    int32_t threadTag = g_conf_info.interval_time / g_conf_info.ivr_num;
    //Create threads for every IVR Server
    /*
        iter = g_conf_info.vec_ivr_ip.begin();
        for(int32_t i = 0; iter != g_conf_info.vec_ivr_ip.end(); i++,iter++)
        {
            ip_info_t tmpIPInfo = *iter;
            int32_t threadret;

            threadret = pthread_create(tid+i, NULL, check_ivr_pthread, &tmpIPInfo);
            if(0 != threadret)
            {
                //Log & Alarm
            }
        }
    */

    for (int32_t i = 0; i < g_conf_info.ivr_num; i++) {
        int32_t threadret;

        threadret = pthread_create(g_ptid + i, NULL, check_ivr_pthread, &(g_conf_info.vec_ivr_ip[i]));

        if (0 != threadret) {
            //Log & Alarm
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                              "[IVR_LB]CREATE THRD ERR:ip=%s,errno=%d\n",
                              g_conf_info.vec_ivr_ip[i].ip_total.c_str(), threadret);
            std::string title = "[ivr_loadbalance????]????????????";
            std::string content = "[ivr_loadbalance????]??????????????IVR IP=";
            content += g_conf_info.vec_ivr_ip[i].ip_total.c_str();
            alarm_by_email(title, content, g_conf_info.str_email);
            alarm_by_sms(content, g_conf_info.str_phone);
        }

        usleep(threadTag * 1000);
    }
}

/**
 * @brief ????IVR Server??????????interval_time????????????????????????????????????????
 *
 * @param [in] : ????????????IVR Server??IP????
 * @return
**/
void* check_ivr_pthread(void* pip_info_t) {
    ip_info_t* pipInfo = (ip_info_t*)pip_info_t;
    int32_t cpu;
    int32_t chanNum;
    int32_t intervalTime = g_conf_info.interval_time;
    bool hasAlarm = false;
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                      "[IVR_LB][pid:%d]CREATE THRD SUCCESS:ip=%s\n", pthread_self(), pipInfo->ip_total.c_str());

    while (g_mod_running) {
        g_ip_svrinfo[pipInfo->ip_total.c_str()].is_available = false;
        int32_t ivrret = check_ivr_server(pipInfo->ip.c_str(), pipInfo->port, &cpu, &chanNum);

        if (0 == ivrret) {
            //????????????????????
            //????IVR??????????????????????is_available????????
            g_ip_svrinfo[pipInfo->ip_total.c_str()].cpu = cpu;
            g_ip_svrinfo[pipInfo->ip_total.c_str()].chan_num = chanNum;
            g_ip_svrinfo[pipInfo->ip_total.c_str()].is_available = true;
            //Unlock ?
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                              "[IVR_LB][pid:%d]IVR OK:ip=%s,cpu=%d,chan=%d\n",
                              pthread_self(), pipInfo->ip_total.c_str(), cpu, chanNum);

            if (hasAlarm) {
                hasAlarm = false;
                std::string title = "[ivr_loadbalance????????]IVR????????????????IP=";
                title += pipInfo->ip_total;
                std::string content = "[ivr_loadbalance????????]IVR????????????????IP=";
                content += pipInfo->ip_total;
                alarm_by_email(title, content, g_conf_info.str_email);
            }
        } else {
            g_ip_svrinfo[pipInfo->ip_total.c_str()].is_available = false;
            //Warning Log
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                              "[IVR_LB][pid:%d]IVR ERR:ip=%s,errno=%d\n",
                              pthread_self(), pipInfo->ip_total.c_str(), ivrret);

            if (!hasAlarm) {
                hasAlarm = true;
                std::string title = "[ivr_loadbalance????]IVR????????????????IP=";
                title += pipInfo->ip_total;
                std::string content;
                std::stringstream ss;
                ss << "[ivr_loadbalance????]IVR????????????????IP=" << pipInfo->ip_total << "????????=" << ivrret;
                ss >> content;
                content += "\n??????:\n";
                content += g_ivr_check_ret_str;
                alarm_by_email(title, content, g_conf_info.str_email);
            }
        }

        //Choose a server
        decision_ivr_server();

        //Sleep for a while
        usleep(intervalTime * 1000);
    }
}

/**
 * @brief ????IVR Server
 *
 * @param [in] ip_addr : ??????IP????????port????
 * @param [in] port : ??????port????
 * @param [out] cpu : IVR Server??CPU????
 * @param [out] chan_num : IVR Server????????????
 * @return 0-??????????-????
**/
int32_t check_ivr_server(const char* ip_addr, uint32_t port, int32_t* cpu, int32_t* chan_num) {
    struct timeval startTime, endTime, timeoutTime;
    double timeUsed;
    int32_t sockfd;
    char buffer[MAX_LINE_LEN + 1];
    struct sockaddr_in serverAddr;
    struct hostent host, * ptmpHost;
    char strPtmpHost[MAX_LINE_LEN + 1] = {0};
    int32_t hostRes = 0;
    int32_t hostErr = 0;
    int32_t nbytes = 0;
    int32_t getbytes = 0;
    int32_t ret = IVR_SUCCESS;
    int32_t tmpRet = 0;
    char* tmpbuf = NULL;

    //host = gethostbyname(ip_addr);
    hostRes = gethostbyname_r(ip_addr, &host, strPtmpHost, MAX_LINE_LEN, &ptmpHost, &hostErr);

    if (hostRes != 0 || NULL == ptmpHost) {
        //Log & Alarm
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "[IVR_LB][pid:%d]GET HOST ERR:ip=%s:%d\n",
                          pthread_self(), ip_addr, port);
        ret = IVR_CREATE_SOCK_ERR;
        goto CHECK_END;
    }

    if (-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0))) {
        //Log & Alarm
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                          "[IVR_LB][pid:%d]CREATE SOCK ERR:ip=%s:%d\n", pthread_self(), ip_addr, port);
        ret = IVR_CREATE_SOCK_ERR;
        goto CHECK_END;
    }

    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr = *((struct in_addr*)host.h_addr);
    //????recv??send??????????
    timeoutTime.tv_sec = g_conf_info.connect_timeout / 1000;
    timeoutTime.tv_usec = g_conf_info.connect_timeout % 1000;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeoutTime, sizeof(timeoutTime));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeoutTime, sizeof(timeoutTime));

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[IVR_LB][pid:%d]CONNECT IVR:ip=%s:%d\n",
                      pthread_self(), ip_addr, port);
    gettimeofday(&startTime, 0);

    if (-1 == (tmpRet = connect(sockfd, (struct sockaddr*)(&serverAddr), sizeof(serverAddr)))) {
        //Log & Alarm
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "[IVR_LB][pid:%d]CONNECT ERR:ip=%s:%d\n",
                          pthread_self(), ip_addr, port);
        ret = IVR_CONNECT_ERR;
        goto CHECK_CLOSE;
    }

    //????"Connect"????
    bzero(&buffer, sizeof(buffer));
    getbytes = 9;
    tmpbuf = buffer;

    do {
        nbytes = recv(sockfd, tmpbuf, getbytes, 0);

        if (nbytes > 0) {
            tmpbuf += nbytes;
            getbytes -= nbytes;
        } else if (EINTR == errno) {
            //????????????
            continue;
        } else {
            //????????
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                              "[IVR_LB][pid:%d]CONNECT MSG ERR:ip=%s:%d,errno=%d\n",
                              pthread_self(), ip_addr, port, errno);
            ret = IVR_RECV_CONN_ERR2;
            goto CHECK_CLOSE;
        }
    } while (0 != getbytes);

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                      "[IVR_LB][pid:%d]RECV FROM IVR:ip=%s:%d,msg=%s\n",
                      pthread_self(), ip_addr, port, buffer);

    if (0 != getbytes || 0 != strncmp(buffer, "connect\n\n", strlen("connect\n\n"))) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                          "[IVR_LB][pid:%d]CONNECT MSG ERR:ip=%s:%d,msg=%s\n",
                          pthread_self(), ip_addr, port, buffer);
        ret = IVR_RECV_CONN_ERR1;
        goto CHECK_CLOSE;
    }

    //????????????
    //    sprintf(buffer, "content-type:command/reply\nreply-text:+ok\n\n\n");
    //    sprintf(buffer, "content-type:command/reply\nreply-text:+ok accepted\n\n\n");
    sprintf(buffer, "caller-unique-id:xyz\ncontent-type:command/reply\nreply-text:+ok\n\n");
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                      "[IVR_LB][pid:%d]SEND TO IVR:ip=%s:%d,msg=%s\n",
                      pthread_self(), ip_addr, port, buffer);

    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                          "[IVR_LB][pid:%d]SEND MSG ERR:ip=%s,port=%d,msg=%d\n",
                          pthread_self(), ip_addr, port, buffer);
        ret = IVR_SEND_ERR;
        goto CHECK_CLOSE;
    }

    //????????
    bzero(&buffer, sizeof(buffer));
    getbytes = 17;
    tmpbuf = buffer;

    do {
        nbytes = recv(sockfd, tmpbuf, getbytes, 0);

        if (nbytes > 0) {
            tmpbuf += nbytes;
            getbytes -= nbytes;
        } else if (EINTR == errno) {
            //????????????
            continue;
        } else {
            //????????
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                              "[IVR_LB][pid:%d]RECV MSG ERR:ip=%s,port=%d,errno=%d\n",
                              pthread_self(), ip_addr, port, errno);
            ret = IVR_RECV_INFO_ERR2;
            goto CHECK_CLOSE;
        }
    } while (0 != getbytes);

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                      "[IVR_LB][pid:%d]RECV FROM IVR:ip=%s:%d,msg=%s\n",
                      pthread_self(), ip_addr, port, buffer);

    if (0 != getbytes || sscanf(buffer, "cpu=%d,chan=%d", cpu, chan_num) != 2) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                          "[IVR_LB][pid:%d]RECV MSG ERR:ip=%s,port=%d,msg=%s\n",
                          pthread_self(), ip_addr, port, buffer);
        ret = IVR_RECV_INFO_ERR1;
        goto CHECK_CLOSE;
    }

    gettimeofday(&endTime , 0);
    timeUsed = (endTime.tv_usec - startTime.tv_usec) / 1000 + (endTime.tv_sec - startTime.tv_sec) * 1000;

    if (timeUsed >= g_conf_info.connect_timeout) {
        //????????????????
        //Log & Alarm
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                          "[IVR_LB][pid:%d]CONNECT TIMEOUT:ip=%s:%d,timeUsed=%lf\n",
                          pthread_self(), ip_addr, port, timeUsed);
        ret = IVR_CONNECT_TIMEOUT;
        goto CHECK_CLOSE;
    }

CHECK_CLOSE:
    close(sockfd);
CHECK_END:
    return ret;
}

/**
 * @brief ????????????????IVR Server??????????"????????"??Server??????????IVR_Host????
 *        ??????????????,??????????IVR??????
 *
 * @return
**/
void decision_ivr_server() {
    server_info_t minInfo;
    std::string minIP;
    uint32_t cpuThr = g_conf_info.cpu_threshold;

    int32_t i = 0;

    //??????????????????????
    for (; i < g_conf_info.ivr_num; i++) {
        std::string tmpIP = g_conf_info.vec_ivr_ip[i].ip_total;
        server_info_t& tmpInfo = g_ip_svrinfo[tmpIP];

        if (tmpInfo.is_available) {
            minInfo.cpu = tmpInfo.cpu;
            minInfo.chan_num = tmpInfo.chan_num;
            minIP = tmpIP;
            break;
        }
    }

    //????????????????????????????????????????????????????????
    if (i == g_conf_info.ivr_num) {
        minIP = g_conf_info.default_ivr_ip.ip_total;
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "[IVR_LB][pid:%d]NO IVR SERVER\n",
                          pthread_self());

        if (!g_no_ivr_alarm) {
            g_no_ivr_alarm = true;
            //No available server. Log & Alarm
            std::string title = "[ivr_loadbalance????]??????????????????????????????????";
            title += minIP;
            std::string content = "[ivr_loadbalance????]????????????????????????????????";
            content += minIP;
            alarm_by_email(title, content, g_conf_info.str_email);
            alarm_by_sms(content, g_conf_info.str_phone);
        }
    }

    //??????????????(??????????)????????????????????
    //??????
    //    1??cpu??????????????????cpu??????;
    //    2) cpu1>??????cpu2<????????????cpu??????;
    //    3) cpu??????????????????????????????
    for (; i < g_conf_info.ivr_num; i++) {
        std::string tmpIP = g_conf_info.vec_ivr_ip[i].ip_total;
        server_info_t& tmpInfo = g_ip_svrinfo[tmpIP];

        if (tmpInfo.is_available) {
            if (minInfo.cpu >= cpuThr) {
                if (tmpInfo.cpu < minInfo.cpu) {
                    minInfo.cpu = tmpInfo.cpu;
                    minInfo.chan_num = tmpInfo.chan_num;
                    minIP = tmpIP;
                }
            } else if (tmpInfo.cpu < cpuThr) {
                if (tmpInfo.chan_num < minInfo.chan_num) {
                    minInfo.cpu = tmpInfo.cpu;
                    minInfo.chan_num = tmpInfo.chan_num;
                    minIP = tmpIP;
                }
            }
        }
    }

    std::string oldIP = switch_core_get_variable("IVR_HOST");
    //    std::string oldIP;
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                      "[IVR_LB][pid:%d]IVR HOST:oldip=%s,newip=%s\n",
                      pthread_self(), oldIP.c_str(), minIP.c_str());

    if (oldIP != minIP) {
        if (g_no_ivr_alarm && minIP != g_conf_info.default_ivr_ip.ip_total) {
            g_no_ivr_alarm = false;
            std::string title = "[ivr_loadbalance????????]??????????????????IP=";
            title += minIP;
            std::string content = "[ivr_loadbalance????????]??????????????????IP=";
            content += minIP;
            alarm_by_email(title, content, g_conf_info.str_email);
            alarm_by_sms(content, g_conf_info.str_phone);
        }

        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                          "[IVR_LB][pid:%d]IVR HOST CHANGE:oldip=%s,newip=%s\n",
                          pthread_self(), oldIP.c_str(), minIP.c_str());
        switch_core_set_variable("IVR_HOST", minIP.c_str());
    }
}

/**
 * @brief ??????????????????????
 *
 * @return
**/
void exit_threads() {
    g_mod_running = false;

    for (int32_t i = 0; i < g_conf_info.ivr_num; i++) {
        int32_t threadret;

        threadret = pthread_join(g_ptid[i], NULL);

        if (0 != threadret) {
            //Log & Alarm
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "[IVR_LB]EXIT THRD ERR:ip=%s,errno=%d\n",
                              g_conf_info.vec_ivr_ip[i].ip_total.c_str(), threadret);
        }
    }
}

/**
 * @brief ????????
 *
 * @param [in] title : ????
 * @param [in] content : ????????
 * @param [in] emaillist : ??????
 * @return ????????/????
**/

bool alarm_by_email(std::string& title, std::string& content, const std::string& emaillist) {
    bool ret = false;

    if (emaillist.empty()) {
        return false;
    }

    //?????????? ????????
    title.erase(
        remove_if(title.begin(), title.end(), std::bind2nd(std::equal_to<char>(), '\'')), title.end());
    content.erase(
        remove_if(content.begin(), content.end(), std::bind2nd(std::equal_to<char>(), '\'')),
        content.end());

    //??????????
    title.erase(
        remove_if(title.begin(), title.end(), std::bind2nd(std::equal_to<char>(), '%')), title.end());
    content.erase(
        remove_if(content.begin(), content.end(), std::bind2nd(std::equal_to<char>(), '%')), content.end());

    std::ostringstream ostm;
    ostm << "/bin/echo " << "'" << content << "'";
    ostm << " | mail -v -s " << "'" << title << "'";
    ostm << " " << emaillist;

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[IVR_LB]SEND MAIL:cmd=%s\n",
                      ostm.str().c_str());

    int32_t result = system(ostm.str().c_str());
    //    int32_t result = true;
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[IVR_LB]SEND MAIL RET:ret=%d\n", result);

    if (0 == result) {
        ret = true;
    } else {
        ret = false;
    }

    return ret;
}

/**
 * @brief ????????
 *
 * @param [in] content : ????????
 * @param [in] smslist : ??????
 * @return ????????/????
**/
bool alarm_by_sms(std::string& content, const ::std::string& smslist) {
    bool ret = false;

    //?????????? ????????
    content.erase(
        remove_if(content.begin(), content.end(), std::bind2nd(std::equal_to<char>(), '\'')),
        content.end());

    //??????????
    content.erase(
        remove_if(content.begin(), content.end(), std::bind2nd(std::equal_to<char>(), '%')), content.end());

    std::vector<std::string> number_vec;

    if (smslist.empty()) {
        return false;
    }

    std::string::size_type sizePos = 0;
    std::string::size_type sizePrevPos = 0;

    while ((sizePos = smslist.find_first_of("," , sizePos)) != std::string::npos) {
        std::string strtemp = smslist.substr(sizePrevPos , sizePos - sizePrevPos);
        number_vec.push_back(strtemp);
        sizePrevPos = ++ sizePos;
    }

    number_vec.push_back(&smslist[sizePrevPos]);

    for (uint32_t i = 0; i < number_vec.size(); i++) {
        std::ostringstream ostm;
        ostm << "/bin/gsmsend -s 0.0.0.0:0 -s 0.0.0.0:0 ";//use to send SMS
        ostm << number_vec[i] << "@'" << content << "'";

        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[IVR_LB]SEND SMS:cmd=%s\n",
                          ostm.str().c_str());

        int32_t result = system(ostm.str().c_str());
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[IVR_LB]SEND SMS RET:ret=%d\n", result);

        if (0 == result) {
            ret = true;
        } else {
            ret = false;
        }
    }

    return ret;
}
