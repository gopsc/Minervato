#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
传感器数据采集守护进程

该程序周期性地从指定API获取传感器列表和对应的传感器数据，并提交到主机服务器。
"""

import json
import time
import os
import requests
from datetime import datetime
import pytz
from typing import List, Dict, Any, Optional

# 获取脚本所在目录
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# 轮询间隔（秒）
POLLING_INTERVAL = 60  # 默认每分钟轮询一次

# IP文件路径 - 使用相对路径
IP_FILE_PATH = os.path.join(SCRIPT_DIR, "ip.txt")

# 字段映射字典文件路径
DICT_FILE_PATH = os.path.join(SCRIPT_DIR, "dict.json")

# 主机服务器URL
HOST_SERVER_URL = "http://127.0.0.1:80/api/sensor-data"

# 设备ID
DEVICE_ID = "python_daemon"

def read_ip_from_file(file_path: str) -> List[str]:
    """
    从文件中读取所有IP地址，支持#注释
    
    Args:
        file_path: IP文件路径
        
    Returns:
        List[str]: 读取到的所有有效IP地址列表，如果没有找到有效IP则返回默认IP列表
    """
    default_ips = ["192.168.254.6"]
    try:
        ips = []
        with open(file_path, 'r', encoding='utf-8') as f:
            for line in f:
                # 去除行首尾空格
                line = line.strip()
                # 跳过空行和注释行
                if not line or line.startswith('#'):
                    continue
                # 添加非注释、非空行作为IP
                ips.append(line)
        # 如果文件为空或没有有效行，返回默认IP列表
        return ips if ips else default_ips
    except Exception as e:
        print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 读取IP文件失败: {str(e)}")
        # 如果文件读取失败，返回默认IP列表
        return default_ips

def get_beijing_time() -> datetime:
    """
    获取当前北京时间
    
    Returns:
        datetime: 当前北京时间
    """
    return datetime.now(pytz.timezone('Asia/Shanghai'))

def load_field_mapping(file_path: str) -> Dict[str, str]:
    """
    从文件加载字段映射关系
    
    Args:
        file_path: 映射字典文件路径
        
    Returns:
        Dict[str, str]: 字段名映射字典 {英文键名: 中文键名}
    """
    default_mapping = {}
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            mapping_array = json.load(f)
            
        # 将数组转换为字典
        mapping_dict = {}
        for item in mapping_array:
            if isinstance(item, dict) and len(item) == 1:
                en_key, zh_key = next(iter(item.items()))
                mapping_dict[en_key] = zh_key
        
        print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 成功加载 {len(mapping_dict)} 个字段映射关系")
        return mapping_dict
    except Exception as e:
        print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 加载字段映射文件失败: {str(e)}")
        return default_mapping

class SensorDataCollector:
    """
    传感器数据采集器
    """
    
    def __init__(self):
        self.session = requests.Session()
        self.session.timeout = 3  # 减少超时时间，避免长时间等待无响应的IP
        self.field_mapping = load_field_mapping(DICT_FILE_PATH)
    
    def get_sensor_list(self) -> Optional[List[str]]:
        """
        尝试从所有IP地址获取传感器列表
        
        Returns:
            Optional[List[str]]: 传感器名称列表，如果所有IP都获取失败则返回None
        """
        # 实时读取所有IP
        ips = read_ip_from_file(IP_FILE_PATH)
        print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 正在尝试从以下IP获取传感器列表: {ips}")
        
        for ip in ips:
            try:
                base_url = f"http://{ip}:80"
                sensor_list_endpoint = f"{base_url}/api/show"
                
                print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 正在从 {ip} 获取传感器列表...")
                response = self.session.post(sensor_list_endpoint, json={})
                response.raise_for_status()
                
                # 解析响应为JSON数组
                sensor_list = response.json()
                
                # 验证响应格式
                if isinstance(sensor_list, list):
                    print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 从 {ip} 成功获取 {len(sensor_list)} 个传感器")
                    return sensor_list
                else:
                    print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 从 {ip} 获取的传感器列表格式错误")
                    # 继续尝试下一个IP
                    continue
                    
            except Exception as e:
                print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 从 {ip} 获取传感器列表失败: {str(e)}")
                # 继续尝试下一个IP
                continue
        
        # 所有IP都尝试失败
        print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 所有IP地址获取传感器列表均失败")
        return None
    
    def get_sensor_data(self, sensor_name: str) -> Optional[Dict[str, Any]]:
        """
        尝试从所有IP地址获取指定传感器的数据
        
        Args:
            sensor_name: 传感器名称
            
        Returns:
            Optional[Dict[str, Any]]: 传感器数据，如果所有IP都获取失败则返回None
        """
        # 实时读取所有IP
        ips = read_ip_from_file(IP_FILE_PATH)
        
        for ip in ips:
            try:
                base_url = f"http://{ip}:80"
                sensor_data_endpoint = f"{base_url}/api/sensor"
                
                payload = {"cmd": sensor_name}
                response = self.session.post(sensor_data_endpoint, json=payload)
                response.raise_for_status()
                
                # 解析响应为JSON对象
                sensor_data = response.json()
                
                # 验证响应格式（至少包含msg字段）
                if isinstance(sensor_data, dict) and "msg" in sensor_data:
                    print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 从 {ip} 成功获取传感器 {sensor_name} 数据")
                    return sensor_data
                else:
                    print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 从 {ip} 获取的传感器 {sensor_name} 数据格式错误")
                    # 继续尝试下一个IP
                    continue
                    
            except Exception as e:
                print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 从 {ip} 获取传感器 {sensor_name} 数据失败: {str(e)}")
                # 继续尝试下一个IP
                continue
        
        # 所有IP都尝试失败
        print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 所有IP地址获取传感器 {sensor_name} 数据均失败")
        return None
    
    def process_all_sensors(self):
        """
        处理所有传感器：获取列表并获取每个传感器的数据
        """
        # 获取传感器列表
        sensor_list = self.get_sensor_list()
        
        if not sensor_list:
            return
        
        # 准备批量数据
        batch_data = []
        
        # 处理每个传感器
        for sensor_name in sensor_list:
            # 验证传感器名称格式
            if not isinstance(sensor_name, str) or not sensor_name:
                continue
            
            # 获取传感器数据
            sensor_data = self.get_sensor_data(sensor_name)
            
            if sensor_data:
                # 处理并收集传感器数据
                processed_data = self.handle_sensor_data(sensor_name, sensor_data)
                if processed_data:
                    batch_data.extend(processed_data)
            
            # 在请求之间添加短暂延迟，避免过度请求
            time.sleep(0.5)
        
        # 如果有收集到的数据，批量提交到服务器
        if batch_data:
            self.submit_sensor_data_to_server(batch_data)
        else:
            print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 没有有效的传感器数据需要提交")
    
    def handle_sensor_data(self, sensor_name: str, sensor_data: Dict[str, Any]) -> List[Dict[str, Any]]:
        """
        处理传感器数据并准备提交格式，根据字段映射将英文字段名替换为中文，并添加模块名称前缀
        
        Args:
            sensor_name: 传感器名称
            sensor_data: 传感器数据
            
        Returns:
            List[Dict[str, Any]]: 格式化后的传感器数据列表
        """
        # 提取传感器数据（排除msg字段）
        data_fields = {k: v for k, v in sensor_data.items() if k != "msg"}
        
        # 准备要提交的数据列表
        formatted_data = []
        
        # 转换每个数据字段为传感器数据格式
        for field_name, value in data_fields.items():
            try:
                # 检查是否有对应的中文映射
                if field_name in self.field_mapping:
                    # 使用中文名称替换英文名称
                    chinese_field_name = self.field_mapping[field_name]
                    
                    # 添加传感器模块名称前缀
                    # 使用"模块名:中文名称"格式
                    prefixed_chinese_name = f"{sensor_name}:{chinese_field_name}"
                    
                    # 尝试转换为浮点数
                    float_value = float(value)
                    # 使用带模块前缀的中文名称作为sensor_type
                    sensor_type = prefixed_chinese_name
                    
                    formatted_data.append({
                        "sensor_type": sensor_type,
                        "value": float_value
                    })
                    
                    print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 处理传感器数据: {field_name} -> {prefixed_chinese_name} = {float_value}")
                else:
                    # 如果没有找到映射关系，不进行推送
                    print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 未找到字段 {field_name} 的映射关系，跳过推送")
                    
            except (ValueError, TypeError):
                print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 无法转换 {field_name} 的值 {value} 为浮点数")
                continue
        
        return formatted_data
    
    def submit_sensor_data_to_server(self, sensor_data_list: List[Dict[str, Any]]):
        """
        提交传感器数据到服务器
        
        Args:
            sensor_data_list: 格式化的传感器数据列表
        """
        try:
            # 构建批量提交数据
            payload = {
                "device_id": DEVICE_ID,
                "data": sensor_data_list
            }
            
            # 添加北京时间戳到每个传感器数据
            for data in sensor_data_list:
                data['timestamp'] = get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')
            
            print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 正在提交 {len(sensor_data_list)} 条传感器数据到服务器...")
            
            # 发送POST请求到服务器
            response = self.session.post(HOST_SERVER_URL, json=payload, timeout=15)
            response.raise_for_status()
            
            # 解析响应
            result = response.json()
            print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 数据提交成功: {result.get('message', '未知响应')}")
            
        except requests.exceptions.RequestException as e:
            print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 数据提交失败: 网络错误 - {str(e)}")
        except (ValueError, TypeError) as e:
            print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 数据提交失败: 响应解析错误 - {str(e)}")
        except Exception as e:
            print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 数据提交失败: 未知错误 - {str(e)}")


def main():
    """
    主函数
    """
    collector = SensorDataCollector()
    
    print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 传感器数据采集守护进程已启动")
    print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 轮询间隔: {POLLING_INTERVAL} 秒")
    print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 按 Ctrl+C 停止")
    
    try:
        while True:
            try:
                # 处理所有传感器
                collector.process_all_sensors()
                
            except Exception as e:
                # 捕获所有异常，确保程序不会退出
                print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 处理传感器时出错: {str(e)}")
            
            # 等待下一次轮询
            print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 等待 {POLLING_INTERVAL} 秒后进行下一次轮询...")
            time.sleep(POLLING_INTERVAL)
            
    except KeyboardInterrupt:
        # 捕获Ctrl+C，优雅退出
        print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 守护进程已停止")
    except Exception as e:
        # 捕获其他异常
        print(f"[{get_beijing_time().strftime('%Y-%m-%d %H:%M:%S')}] 守护进程异常退出: {str(e)}")


if __name__ == "__main__":
    main()