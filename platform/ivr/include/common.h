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

#ifndef _IVR_COMMON_H_
#define _IVR_COMMON_H_

#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <stdint.h>


#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <memory>

#include <bgcc.h>

#include "esl.h"
#include "esl_threadmutex.h"

#include "ims/ims_struct.h"

//global macro
#define  SUCCESS  (0)
#define  FAILURE  (1)
#define  MAX_THREAD_NUM  (950)
#define  LOG_PATH  "./conf/"
#define  FLOW_PATH  "./flow/"

#define IMS_SESSION_TAG "IMSDATA"
#define CREATE_SESSION_EMPTY_CALLID "create empty session by ivr"

#define LEN_8  8
#define LEN_16 16
#define LEN_32 32
#define LEN_64 64
#define LEN_128 128
#define LEN_256 256
#define LEN_512 512

#define IVR_IMS_REINIT_TIME                 3

#define IVR_FAIL_GENERAL                   -1
#define IVR_SUCCESS                         0
#define IVR_FAIL_LOCK                       1
#define IVR_FAIL_EXISTS                     2
#define IVR_FAIL_NOTEXISTS                  3
#define IVR_FAIL_MEM                        4
#define IVR_FAIL_INVALID_FSNO               5
#define IVR_FAIL_ALL_BUSY                   6
#define IVR_FAIL_CONNECT                    7
#define IVR_FAIL_TIMEOUT                    8
#define IVR_FAIL_NOIMS                      9
#define IVR_FAIL_IMS_REGIST                10
#define IVR_FAIL_IMS_NOTEXIST              11
#define IVR_FAIL_IMS_ROUTEREQUEST          12
#define IVR_FAIL_IMS_CANCELROUTEREQUEST    13
#define IVR_FAIL_IMS_TRANSAGENT            14
#define IVR_FAIL_IMS_SETASSOCIATEDATA      15
#define IVR_FAIL_IMS_GETASSOCIATEDATA      16
#define IVR_FAIL_IMS_NOMAIN                17
#define IVR_FAIL_IMS_CREATESESSION         18
#define IVR_FAIL_IMS_SESSIONINVALIDATE     19
#define IVR_FAIL_IMS_INFOINVALIDATE        20
#define IVR_FAIL_IMS_AGENTNOTASSIGNED      21
#define IVR_FAIL_IMS_ROUTEREQUEST_REPEATED 22
#define IVR_FAIL_IMS_REQIDINVALIDATE       23
#define IVR_FAIL_IMS_PROXY_FAILED          24
#define IVR_FAIL_IMS_REINIT                25
#define IVR_FAIL_IMS_MAKECALL              26
#define IVR_TRACE(fmt, arg...) do { BGCC_TRACE("ivr", fmt, ##arg); }  while(0)
#define IVR_NOTICE(fmt, arg...) do { BGCC_NOTICE("ivr", fmt, ##arg); }  while(0)
#define IVR_DEBUG(fmt, arg...) do { BGCC_DEBUG("ivr", fmt, ##arg); }  while(0)
#define IVR_WARN(fmt, arg...) do { BGCC_WARN("ivr", fmt, ##arg); }  while(0)
#define IVR_FATAL(fmt, arg...) do { BGCC_FATAL("ivr", fmt, ##arg); }  while(0)

using namespace std;

typedef map<string, string> key_map_t;
typedef map<string, key_map_t> section_map_t;
typedef std::map<std::string, std::string> str2str_map;

typedef uint32_t fw_id_t;     //Freeswitch????????
typedef uint64_t ivr_session_id_t;    //ivr??sessionid

typedef ivr::ReloadFlagT conf_reload_flag_t;
typedef ims::RouteEventT ims_route_eventdata_t;

typedef bgcc::Guard<bgcc::Mutex> locker_t;

/**
 * @brief ????????????
 *   detail description
 *
 */
class DBPool;

/**
 * @brief ????????????
 *   detail description
 *
 */
class NodeBase;

class ThreadPool;

class IvrTimer;

/**
 * @brief ????????
 *   detail description
 *
 */
enum var_type_t {
    INT32 = 0,      ///<  ????
    STRING,         ///<  STRING
    RESULTSET,      ///<  ??????
    MAP             ///<  ????
};

/**
 * @brief ????????????
 *   detail description
 *
 */
struct variable_t {
    string name;     ///< ????
    var_type_t type; ///< ????
    void* pvalue;    ///< ?????????????? ?????????????? ????????
    string initial;  ///< ????????
};

/**
 * @brief????????????
 *   detail description
 *
 */
struct flow_t {
    uint32_t begin_id;                      ///< ????????????????
    map<uint32_t, NodeBase*>  id_node_map;  ///< ????????
    map<string, variable_t> name_var_map;   ///< ????????????????
    uint32_t ref_count;                     ///< ???????????? ????????????????
    time_t last_modify_time;                ///< ????????????????
    bgcc::Mutex refcmutex;                  ///< mutex for ref_count
};

typedef std::map<std::string, flow_t*> flow_name_map;

struct channel_info_t {
    std::string channel_name;
    std::string channel_id;
    std::string callsource;
};

/**
 * @brief IVR????????
 *   detail description
 *
 */
struct ivr_base_event_t {
    // virtual destruct, delete drived class's member objects
    virtual ~ivr_base_event_t() {}

    enum ivr_event_type {
        CALL_EVENT = 0,
        MEDIA_EVENT,
        IMS_EVENT,
        OTHER_EVENT
    };
    ivr_event_type evt_type;
    ivr_session_id_t sessionId;
    uint64_t timestamp;
};

/**
 * @brief IVR????????
 *   detail description
 *
 */
struct ivr_call_event_t : public ivr_base_event_t {
    enum call_event_name {
        IVR_EVT_ANSWER = 0,     //????
        IVR_EVT_ALERT,          //????
        IVR_EVT_HANGUP,         //????
        IVR_EVT_BRIDGE,         //????????
        IVR_EVT_UNBRIDGE,       //????????????
        IVR_EVT_CHANLE_DESTORY, //Freeswitch????
        IVR_EVT_CALLFAIL        //????????
    };

    enum call_event_reason {
        CALL_NORMAL,        //????????/
        CALL_DEST_BUSY,     //??????
        CALL_DEST_NOANSWER, //??????????
        CALL_REJECTED,      //????????
        CALL_USER_NOT_REGISTERED,
        CALL_SUBSCRIBER_ABSENT,
        CALL_UNKNOW         //????????
    };

    call_event_name evt_name;
    call_event_reason evt_reason;

    string callid;              //??????????callid
    string ani;                 //????????
    string dnis;                //????????

    //????????????????????????????
    string other_callid;        //????????????callid
    string other_ani;           //????????????????????
    string other_dnis;          //????????????????????
};

/**
 * @brief IVR????????
 *   detail description
 *
 */
struct ivr_media_event_t : public ivr_base_event_t {
    enum media_event_name {
        IVR_EVT_PLAY_END = 100, //????????????
        IVR_EVT_FILENOTEXIST,   //??????????????
        IVR_EVT_GETDIGIT_SUCC,  //????????
        IVR_EVT_GETDIGIT_FAIL,  //????????
        IVR_EVT_DTMF,           //????????
        IVR_EVT_RECORD_START,   //????????
        IVR_EVT_RECORD_END      //????????
    };
    enum media_event_reason {
        MEDIA_END,              //????????
        MEDIA_GETDIGIT,         //????
        MEDIA_INVALID_FILE,     //????????
        MEDIA_DISCONNECT,       //????????
        MEDIA_UNKNOW            //????????
    };

    media_event_name evt_name;
    media_event_reason evt_reason;

    string callid;              //??????????callid
    string media_data;          //????????????
    string digits;              //????????
};

/**
 * @brief IVR????????
 *   detail description
 *
 */
struct ivr_other_event_t : public ivr_base_event_t {
    enum other_event_name {
        IVR_EVT_TIMEOUT = 200,  //????????
        IVR_EVT_UNKNOW          //????????
    };
    other_event_name evt_name;
    string data;                //????????(??????????timerId)
};

struct ivr_ims_event_t : public ivr_base_event_t {
    enum ims_event_name {
        IVR_EVT_IMS_ROUTE_RESPONSE = 300,
        IVR_EVT_IMS_NOT_RESPONSE,
        IVR_EVT_IMS_UNKNOW
    };

    ims_event_name evt_name;
    ims_route_eventdata_t data;
};

typedef queue< ivr_base_event_t* > ivr_event_queue_t;

struct ivr_thread_param_t {
    ThreadPool* pool;               //??????????_worker_queue
    ivr_event_queue_t* event_queue;   //??????????????IVR????????
    //    sem_t * evt_sem;            //??????????
    IvrTimer* timer;                //????????????????????
};
/*
struct ivr_task_param_t{
    ivr_event_queue_t * event_queue;  //??????????????IVR????????
    sem_t * evt_sem;              //????????????????
    IvrTimer * timer;               //????????????????????
};
*/
struct session_dynamic_resource_t {
    session_dynamic_resource_t() : event_queue(NULL), timer(NULL) { /*, sem(NULL)*/  }
    ivr_event_queue_t* event_queue;   ///<IVR??????????session??????????
    IvrTimer* timer;                ///<IVR????????session??????????
    //    sem_t *sem;
};

// timer type: usr timer or system defined timer
enum timer_type_t {
    user_define, // user defined timer
    ims_transagent, // ims trans agent timer
    unknown // other timer
};

/**
 * @brief ????????
 *   detail description
 *
 */
struct base_script_t {
    // virtual destruct
    virtual ~base_script_t() {}
    enum script_type_t {
        AUTOBOUND = 0,       ///<????????
        INBOUND = 1,         ///<????????
    };
    string name;          ///<????????
    string desc;          ///<????????
    string flow_name;     ///<????????????
    string vox_base;      ///<??????????????
    flow_t* flow;         ///<????????
    uint32_t type;        ///<????????
    map<string, variable_t> name_var_map;   ///<?????????????? ????

    //esl_handle_t*  handle;                  ///<?????? ??????socket handle
    /** ???????????????????? **/
    fw_id_t fid;          ///<Freeswitch????
    string ani;           ///<??????
    string dnis;          ///<??????
    string callid;        ///<????????callid
    ivr_session_id_t session_id;    ///<session id??session??????????

    string channel_name;
    string channel_id;
    string callsource; // Freeswitch IP

    string trunck;//data when transfer from other ivr

    IvrTimer* timer;                ///<IVR????????session??????????

    // ims infomation
    uint32_t imsno; // ims index
    ims_session_id_t imssid; // ims session id
    ims_routerequest_id_t requestId;


    std::string bill_info();
};

/**
 * @brief inbound????
 *   detail description
 *
 */
struct inbound_script_t: public base_script_t {
    esl_socket_t socket;        ///< ??freeswitch??????
    struct sockaddr_in* addr;   ///<  sockaddr

    std::string dialdata;

    inbound_script_t(): addr(NULL) {}
    ~inbound_script_t() {
        if (addr) {
            delete addr;
            addr = NULL;
        }
    }
};


/**
 * @brief ????????????
 *   detail description
 *
 */
struct autorun_script_t: public base_script_t {
    //    vector<freeswitch_info_t> freeswitch_info_vec;      ///<??????fs????
    uint32_t autorun_chan_num;                          ///<????????????????
    uint32_t autorun_interval;                          ///<??????????
    time_t last_startup_time;                           ///<????????????????
    bool is_write_bill;                                 ///<??????????
};

/**
 * @brief ??????????????
 *   detail description
 *
 */
struct inbound_conf_t {
    map<string, inbound_script_t> dnis_script_map;  ///<????????????
    inbound_script_t default_script;                ///<????????
    uint32_t max_inbound_chan_num;                  ///<??????????????
    uint32_t max_autodial_chan_num;                 ///<max autodial chan number
    time_t last_modify_time;                        ///<????????????????
    bool has_default;                               ///<??????????????????
};

/**
 * @brief ??????????????????
 *   detail description
 *
 */
struct autorun_conf_t {
    vector<autorun_script_t> autorun_script_vec;///<??????????????
    uint32_t max_autorun_chan_num;              ///<??????????????????
    time_t last_modify_time;                    ///????????????
};

struct call_data_conf_t {
    string file_name;
    uint32_t generate_interval;
    uint32_t generate_file_size;
    string generate_path;
    string sendto_path;
};

typedef struct _sys_var_t {
    enum {//??????SYS_VAR??????????
        BEGIN = 0,
        ANI = 0,
        DNIS = 1,
        CALLID = 2,
        RECORD_FILE = 3,
        HANGUP_CAUSE = 4,
        CALL_BEGIN_TIME = 5, ///< ??????????????????
        CALL_ANSWER_TIME = 6, ///< ????????????????
        TRUNCK = 7,
        AGENTID= 8, //????????????
        END = 9
    };
} sys_var_t;

struct errorcode_string {
    // get description of error code
    static string tostring(int32_t errorcode);
    //    static int32_t tocode(const string& errorstring);

private:
    static const std::map<int32_t, string>& errormap();
};


extern int32_t g_stopped;
extern sem_t g_stop_sem;

//logger config
extern uint32_t g_log_count;
extern uint32_t g_log_filecount;
extern uint32_t g_log_level;
extern uint32_t g_log_timeout;

extern string g_server_ip;
extern uint32_t g_server_port;
extern uint32_t g_reload_port;
extern call_data_conf_t g_calldata_conf;

extern inbound_conf_t  g_inbound_conf;
extern autorun_conf_t  g_autorun_conf;

extern pthread_rwlock_t g_inbound_conf_rwlock; // used for lock g_inbound_conf
extern pthread_rwlock_t g_autorun_conf_rwlock; // used for lock g_autorun_conf

extern DBPool* g_dbpool;///< ????????????

extern uint32_t g_inbound_chan_num;
extern uint32_t g_autorun_chan_num;
extern uint32_t g_autodial_chan_num;

extern pthread_rwlock_t g_inbound_chan_rwlock;
extern pthread_rwlock_t g_autorun_chan_rwlock;
extern pthread_rwlock_t g_autodial_chan_rwlock;

extern const char* SYS_VAR[];

extern bool
g_output_esllog; // enable/disable esl log, true: output log information, false: not print
extern time_t g_autorun_script_stamp;   // mark last autorun script loaded time
// every autorun chan thread save the time(st) when it started
//  if st < g_autorun_script_stamp, the thread need stop,
//      because autorun script ust reloaded

const char DATA_CACHE_FILE[] = "log/ivr_datacached.log";

#endif
