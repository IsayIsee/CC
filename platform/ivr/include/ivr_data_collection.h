/*
 * Copyright 2002-2015 the original author or authors.
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

// ivr data collection define

#ifndef _IVR_DATA_COLLECTION_H
#define _IVR_DATA_COLLECTION_H

#include "ivr_callinfo.h"
#include "fs_struct.h"

namespace ivr {
// 每个技能组的信息
struct SkillCallData{
public:
    SkillCallData() : giveuptimes(0){}
public:
    uint32_t giveuptimes;  // 该技能组的排队放弃量
};

// 当前的呼叫数据呼叫数据
struct IvrCallData{
public:
    IvrCallData()
    : cur_inbound_num(0)
    , cur_accept_num(0)
    , trans_num(0)
    , total_inbound_num(0)
    , total_accept_num(0)
    , giveuptimes(0){}
public:
    uint32_t cur_inbound_num;  // 当前呼入总量
    uint32_t cur_accept_num;  // 当前活跃的呼入量
    uint32_t trans_num;  //转坐席量
    uint32_t total_inbound_num;  // 总呼入总量
    uint32_t total_accept_num;  // 总活跃的呼入量
    uint32_t giveuptimes; // 排队放弃量
    std::map<std::string, SkillCallData*> skill_call_data; //技能呼叫数据
};

class IvrInboundCall {
public:
    enum {
        BEGIN,
        ACCEPT,
        INFLOW,
        CUSTOMERANSWER,
        AGENTALERTING,
        AGENTANSWER,
        CONNECTED,
        ROUTEBYSKILL,
        ROUTEBYAGENTID,
        ROUTEBYAGENTLIST,
        TRANSAGENT,
        EXITFLOW,
        CALLWITHOUTFOLW,
        THISPARTYHANGUP,
        OTHERPARTYHANGUP,
        UNKNOW
    }; // state

    IvrInboundCall() : _m_state(UNKNOW), _has_hangup(false), _exit_flow(false){}
    ~IvrInboundCall(){}
    int32_t update_route_skill_endtime(const std::string& skill_name);
    int32_t set_appdata(const std::string& appdata);
    int32_t set_state(int32_t state);//设置当前状态
    int32_t get_state();//设置当前状态
    void init_new_call(const ivr_session_id_t& sessionId, const string &callid, const string &caller, const string &callee); 
    void set_skill(const std::string& skill);
    std::string get_skill();
    std::string get_called();
    bool get_exit_flow();
    void set_agent_num(const string &agentnum);
    time_t get_begintime();
private:
    int32_t _m_state;
    bool _has_hangup;
    bool _exit_flow;
    IvrCallInfo _m_ivr_callinfo;
} ;

class IvrCallDataCollection
{
public:
    enum {
        GETBYPLAT,
        GETBYIVRNUM,
    };

    IvrCallDataCollection(){}
private:
    ~IvrCallDataCollection(){}
public:
    static IvrCallDataCollection& instance();

    // @breif 初始化函数
    int32_t initialize(const char* path);
    
    // @breif 反 初始化函数
    int32_t uninitialize();
    
    // 将数据写入文件且判断日期函数
    void calldata_to_file_block_func();
    
    // @brief 呼入量(进入流程) +1
    // @param caller 呼入号码
    // @param called ivr转接码
    // @parma sessionid ,标记每通呼叫?
    // @return 0:success other:failed
    int32_t new_accept_call(const std::string& caller, const std::string& called
                    , const ivr_session_id_t& sessionId, const std::string& uuid);

    // @brief 呼入量(所有的量) +1
    // @param caller 呼入号码
    // @param called ivr转接码
    // @parma uuit uuid,标记每通呼叫?
    // @return 0:success other:failed
    int32_t new_inbound_call(const std::string& caller, const std::string& called
                    , const std::string& uuid);
    
    // @brief 设置状态
    // @parma callid callid,标记每通呼叫
    // @parma state 状态 
    // @parma skill 技能，默认为空
    // @return 0:success other:failed   
    int32_t set_appdata(const ivr_session_id_t& sessionId, const std::string& appdata);
    int32_t set_state(const ivr_session_id_t& sessionId, const int32_t state, const std::string& skill);
    
    // @brief 处理事件
    // @parma event,事件
    // @return 0:success other:failed
    int32_t process_event(struct fs_event& event);

    // @brief 处理路由事件和超时事件
    // @parma event,事件
    // @return 0:success other:failed
    int32_t process_event(ivr_base_event_t* event);
    
    // @breif 获取系统当前呼叫数据
    // @param calldata calldata地址
    // @param result 返回的技能组数据
    // @return 0:success other:failed  
    int32_t get_call_data(struct IvrCallData* p, std::string& result);
    
    // @breif 获取系统当前呼叫数据
    // @param result 返回的技能组数据
    // @return 0:success other:failed  
    int32_t get_inbound_by_plat(std::string& result);

    // @breif 获取系统当前呼叫数据
    // @param result 返回的技能组数据
    // @return 0:success other:failed  
    int32_t get_inbound_call_data(const int32_t type, const std::string& input, std::string& result);
    
    // @breif 获取系统当前呼叫数据
    // @param ivrnum ivr接入码
    // @param result 根据ivr接入码返回数据
    // @return 0:success other:failed  
    int32_t get_inbound_by_ivrnum(const std::string& ivrnum, std::string& result);

    // @brief 从文件中获取缓存数据
    int32_t get_data_from_file();

    // @brief 将缓存数据写入文件中
    int32_t put_data_to_file();

    // @判断是否排队放弃，如果是更新相关数据
    int32_t is_giveup_call(IvrInboundCall *call
                    , const uint64_t sessionid, const std::string& called);

    // @brief 复位
    void clear_invaid_call();
    
    // @brief 复位
    void reset();

private:
    bgcc::Mutex _m_locker;
    std::string _cached_file; // 缓存文件路径
    std::string _cached_date; // 缓存日期
    struct IvrCallData _plat_call_data;//当前的呼叫数据
    std::map<std::string, struct IvrCallData*> _ivrnum_call_data; // 按ivr接入码存储的当天呼叫数据
    std::map<ivr_session_id_t, IvrInboundCall*> _realtime_call;
    std::map<ivr_session_id_t, IvrInboundCall*> _exitflow_call;
    std::map<std::string, ivr_session_id_t> _first_uuid_map;
    std::map<std::string, ivr_session_id_t> _second_uuid_map;
    std::set<std::string> _refuse_call;
};

typedef std::map<std::string, struct SkillCallData*>::iterator IterSkill;
typedef std::map<std::string, struct IvrCallData*>::iterator IterIvrNum;
typedef std::map<ivr_session_id_t, IvrInboundCall*>::iterator IterCall;
typedef std::map<std::string, ivr_session_id_t>::iterator IterSession;
typedef std::set<std::string>::iterator IterStr;
}

#endif

