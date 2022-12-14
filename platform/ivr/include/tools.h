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

#ifndef  __TOOLS_H_
#define  __TOOLS_H_

#include <string>
#include <vector>

#include "common.h"

class ivr_tools_t {
public:

    /**
     * @brief 获取timespec结构体
     *
     * @param [in/out] ts   : struct timespec*
     * @param [in/out] milliSeconds   : int32_t
     * @return  int32_t
     * @retval
     * @see
     * @note
     * @author zhangyue
     * @date 2012/04/27 16:39:17
    **/
    static int32_t get_abs_timespec(struct timespec* ts, int32_t milliSeconds);
    /**
     * @brief 获取当前时间戳，毫秒
     *
     * @return  uint64_t
     * @retval
     * @see
     * @note
     * @author zhangyue
     * @date 2012/04/24 14:23:36
    **/
    static uint64_t get_timestamp_ms();
    /**
     * @brief 判断 ip地址 并转换
     *
     * @param [in] src   : string&
     * @param [out] uint32_t   : string&
     * @return  bool
     * @retval
     * @see
     * @note
     * @author zhangyue
     */
    static bool check_ip(const char* src, uint32_t& result);
    /**
     * @brief 判断 port 并转换
     *
     * @param [in/out] str   : string&
     * @return  bool
     * @retval
     * @see
     * @note
     * @author zhangyue
     */
    static bool check_port(const char* src, uint32_t& result);
    /**
     * @brief 判断 正整数字符串(不以零开头的)  范围[1-2147483647]
     *
     * @param [in/out] str   : string&
     * @return  bool
     * @retval
     * @see
     * @note
     * @author chenyuzhen
     * @date 2011/08/25 15:43:57
     **/
    static bool is_positive_integer(std::string& str);


    /**
     * @brief  判断 非负数字符串  范围[0-2147483647]
     *
     * @param [in/out] str   : string&
     * @return  bool
     * @retval
     * @see
     * @note
     * @author chenyuzhen
     * @date 2011/08/25 15:44:01
     **/
    static bool is_nonnegative_integer(std::string& str);

    /**
     * @brief 判断 整数  范围[-2147483647 ~ 2147483647]
     *
     * @param [in/out] str   : string&
     * @return  bool
     * @retval
     * @see
     * @note
     * @author zhangyue
     * @date 2012/02/20 15:43:57
     **/
    static bool is_integer(std::string& str);

    /**
     * @brief 判断 字符串中仅包含数字
     *
     * @param [in/out] str   : string&
     * @return  bool
     * @retval
     * @see
     * @note
     * @author zhangyue
     * @date 2012/02/19 15:43:57
     **/
    static bool is_all_digit(std::string& str);

    /**
     * @brief 拆分(指定分隔符)字符串
     *
     * @param [in/out] src   : const string&
     * @param [in/out] split_str   : const string&
     * @param [in/out] dest   : vector<string>&
     * @return  int32_t
     * @retval
     * @see
     * @note
     * @author chenyuzhen
     * @date 2011/08/25 15:44:05
     **/
    static bool split_string(const std::string& src,
                             const std::string& separator, std::vector<std::string>& dest);

    /**
     * @brief  返回1970到现在的秒数
     *
     * @return  string
     * @retval
     * @see
     * @note
     * @author chenyuzhen
     * @date 2011/08/25 15:44:10
     **/
    static std::string get_current_second();

    /**
     * @brief 将时间格式化为? "%Y-%m-%d %H:%M:%S"
     *
     * @param [in/out] second   : string&
     * @return  string
     * @retval
     * @see
     * @note
     * @author chenyuzhen
     * @date 2011/08/25 15:44:32
     **/
    static std::string format_datatime_str(std::string& second);

    /**
     * @brief 字符串转换为double
     *
     * @param [in/out] str   : const std::string&
     * @param [in/out] value   : double&
     * @return  bool
     * @retval
     * @see
     * @note
     * @author chenyuzhen
     * @date 2011/08/24 17:33:15
     **/
    static bool str2double(const std::string& str, double& value);

    /**
     * @brief 去掉字符串前后的空格
     *
     * @param [in/out] str   : std::string&
     * @return  void
     * @retval
     * @see
     * @note
     * @author chenyuzhen
     * @date 2011/08/25 15:48:08
    **/
    static void trim(std::string& str);

    /**
     * @brief 读取ini文件中的数组集合用到
     * 读取同一prefix引导的section，将索引信息保存到vec中
     *
     * @param [in/out] prefix   : const std::string
     * @param [in/out] sec   : const section_map_t&
     * @param [in/out] vec   : std::vector<uint32_t>
     * @return  void
     * @retval
     * @see
     * @note
     * @author chenyuzhen
     * @date 2011/08/26 17:28:47
    **/
    static void idx2vec(const char* prefix,
                        const section_map_t& sec, std::vector<uint32_t>& vec);

    /**
     * @brief 读取ini文件中的数组集合用到
     *
     * @param [in/out] prefix   : const char*
     * @param [in/out] sec   : const key_map_t&
     * @param [in/out] vec   : std::vector<uint32_t>&
     * @return  void
     * @retval
     * @see
     * @note
     * @author chenyuzhen
     * @date 2011/08/26 20:02:26
    **/
    static void idx2vec(const char* prefix,
                        const key_map_t& sec, std::vector<uint32_t>& vec);

    /**
     * @brief 判断std::string和const char*是否相同
     *
     * @param [in/out] str   : const std::string&
     * @param [in/out] sz   : const char*
     * @return  bool
     * @retval
     * @see
     * @note 不区分大小写
     * @author chenyuzhen
     * @date 2011/08/28 14:40:11
    **/
    static bool str_eq(const std::string& str, const char* sz);

    /**
     * @brief 判断目录是否存在，如果不存在则创建
     *
     * @param [in/out] dirname   : const char*
     * @return  void
     * @retval
     * @see
     * @note
     * @author chenyuzhen
     * @date 2011/08/29 20:09:13
    **/
    static bool check_and_create_dir(const char* dirname);

    /**
     * @brief 删除字符串中的所有空格
     *
     * @param [in/out] exp   : const std::string&
     * @return  std::string
     * @retval
     * @see
     * @note
     * @author chenyuzhen
     * @date 2011/09/02 10:56:41
    **/
    static std::string delete_blank(const std::string& exp);


    /**
     * @brief 将map数值转换为字符串
     *
     * @param [in/out] std::map<std::string   : const
     * @param [in/out] val   : std::string>
     * @return  std::string
     * @retval
     * @see
     * @note
     * @author chenyuzhen
     * @date 2011/09/02 17:39:24
    **/
    static std::string map2str(const std::map<std::string, std::string>& val);

    static void build_vars(const std::map<std::string, variable_t>& temp,
                           std::map<std::string, variable_t>& out);

    static void destroy_vars(std::map<std::string, variable_t>& vars);

    static std::string chlname2no(const std::string& src);
    static bool chlname2no(const std::string& src, std::string& dest);
    static const char* inet_ntop(uint32_t src, char* dest, uint32_t dest_len);

    /**
     * @brief 支持中断的sleep
     *
     * @param [in/out] s   : uint32_t
     * @return  void
     * @retval
     * @see
     * @note
     * @author chenyuzhen
     * @date 2011/09/16 12:36:39
    **/
    static void safe_sleeps(uint32_t s);

    /**
     * @brief sleep ms
     *
     * @param [in/out] ms   : uint32_t
     * @return  void
     * @retval
     * @see
     * @note
     * @author chenyuzhen
     * @date 2011/10/20 12:09:59
    **/
    static void safe_sleepms(uint32_t ms);


    // return "-h ip -p port"
    static std::string get_address(uint32_t ip);
    static std::string get_callsource(uint32_t ip, uint32_t port);
    static std::string get_callsource(const std::string& ip, uint32_t port);
private:
    /**
     * @brief 从sting中判断prefix
     *
     * @param [in/out] prefix   : const char*
     * @param [in/out] sec   : const std::vector<std::string>
     * @param [in/out] vec   : std::vector<uint32_t>&
     * @return  void
     * @retval
     * @see
     * @note
     * @author chenyuzhen
     * @date 2011/08/26 20:03:58
    **/
    static void idx2vec_imp(const char* prefix,
                            const std::vector<std::string>& sec, std::vector<uint32_t>& vec);

};

class string_build {
public:
    string_build() {
    }
private:
    string_build(const string_build&);
    string_build& operator=(const string_build&);
    string_build& operator+(const string_build&);
    string_build& operator+=(const string_build&);
public:
    string str() const {
        return _oss.str();
    }
public:
    template<typename _T> string_build& operator +(const _T& v) {
        _oss << v;
        return *this;
    }

    template<typename _T> string_build& operator +=(const _T& v) {
        _oss << v;
        return *this;
    }
private:
    std::ostringstream _oss;
};

#endif  //__TOOLS_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
