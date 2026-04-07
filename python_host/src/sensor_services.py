#!/usr/bin/env python3

from flask import Blueprint, request, jsonify, current_app, g
from flask_jwt_extended import jwt_required
import json
import datetime
import pytz
from sqlalchemy import func, and_, desc, Integer

# 创建传感器服务蓝图
sensor_bp = Blueprint('sensor', __name__)

# 传感器数据接收端点
@sensor_bp.route('/api/sensor-data', methods=['POST'])
def receive_sensor_data():
    """
    接收来自微控制器的传感器数据并保存到数据库
    格式示例：
    {
        "device_id": "esp32_1",
        "sensor_type": "temperature",
        "value": 25.5
    }
    或批量格式：
    {
        "device_id": "esp32_1",
        "data": [
            {"sensor_type": "temperature", "value": 25.5},
            {"sensor_type": "humidity", "value": 60.2}
        ]
    }
    """
    try:
        data = request.get_json()
        
        if not data:
            return jsonify({"error": "请求体不能为空"}), 400
        
        # 获取数据库实例和模型
        db = get_db()
        SensorData = get_model('SensorData')
        
        # 验证必要字段
        device_id = data.get('device_id')
        if not device_id:
            return jsonify({"error": "缺少device_id字段"}), 400
        
        saved_count = 0
        
        # 处理批量数据格式
        if 'data' in data and isinstance(data['data'], list):
            # 确保有应用上下文
            with current_app.app_context():
                for item in data['data']:
                    sensor_type = item.get('sensor_type')
                    value = item.get('value')
                    
                    if sensor_type and value is not None:
                        try:
                            # 转换value为浮点数
                            float_value = float(value)
                            
                            # 创建传感器数据记录
                            sensor_data = SensorData(
                                device_id=device_id,
                                sensor_type=sensor_type,
                                value=float_value
                            )
                            
                            # 如果数据中包含timestamp字段，使用它（已经是北京时间）
                            if 'timestamp' in item:
                                try:
                                    # 解析时间戳字符串为datetime对象
                                    sensor_data.timestamp = datetime.datetime.strptime(
                                        item['timestamp'], '%Y-%m-%d %H:%M:%S'
                                    ).replace(tzinfo=pytz.timezone('Asia/Shanghai'))
                                except (ValueError, TypeError):
                                    print(f"[警告] 无法解析时间戳 {item['timestamp']}")
                            db.session.add(sensor_data)
                            saved_count += 1
                        except (ValueError, TypeError):
                            print(f"[警告] 无法转换值 {value} 为浮点数")
                            continue
                
                # 提交到数据库
                if saved_count > 0:
                    db.session.commit()
                    print(f"[日志] 成功保存 {saved_count} 条传感器数据")
                    return jsonify({"message": f"成功保存 {saved_count} 条传感器数据", "saved": saved_count}), 200
                else:
                    return jsonify({"error": "没有有效的传感器数据需要保存"}), 400
        
        # 处理单条数据格式
        else:
            sensor_type = data.get('sensor_type')
            value = data.get('value')
            
            if not sensor_type or value is None:
                return jsonify({"error": "缺少sensor_type或value字段"}), 400
            
            try:
                # 转换value为浮点数
                float_value = float(value)
                
                # 创建传感器数据记录
                with current_app.app_context():
                    sensor_data = SensorData(
                        device_id=device_id,
                        sensor_type=sensor_type,
                        value=float_value
                    )
                    
                    # 如果数据中包含timestamp字段，使用它（已经是北京时间）
                    if 'timestamp' in data:
                        try:
                            # 解析时间戳字符串为datetime对象
                            sensor_data.timestamp = datetime.datetime.strptime(
                                data['timestamp'], '%Y-%m-%d %H:%M:%S'
                            ).replace(tzinfo=pytz.timezone('Asia/Shanghai'))
                        except (ValueError, TypeError):
                            print(f"[警告] 无法解析时间戳 {data['timestamp']}")
                    db.session.add(sensor_data)
                    db.session.commit()
                
                print(f"[日志] 成功保存传感器数据: device={device_id}, type={sensor_type}, value={float_value}")
                return jsonify({"message": "传感器数据保存成功"}), 200
                
            except (ValueError, TypeError):
                return jsonify({"error": "value必须是有效的数字"}), 400
                
    except Exception as e:
        error_message = f"保存传感器数据失败: {str(e)}"
        print(f"[错误] {error_message}")
        return jsonify({"error": error_message}), 500

# 获取传感器数据的端点
@sensor_bp.route('/api/sensor-data', methods=['GET'])
@jwt_required()
def get_sensor_data():
    """
    获取传感器数据（需要认证）
    可以通过查询参数过滤：device_id, sensor_type, start_time, end_time
    支持数据聚合：设置aggregate=true可根据时间跨度自动计算合适的时间间隔进行数据聚合
    """
    try:
        # 获取数据库实例和模型
        db = get_db()
        SensorData = get_model('SensorData')
        
        # 获取查询参数
        device_id = request.args.get('device_id')
        sensor_type = request.args.get('sensor_type')
        start_time_str = request.args.get('start_time')
        end_time_str = request.args.get('end_time')
        # 为非聚合查询设置默认限制，为聚合查询准备
        limit = request.args.get('limit', 100, type=int)
        aggregate = request.args.get('aggregate', 'false').lower() == 'true'
        
        # 构建查询
        with current_app.app_context():
            query = SensorData.query
            
            # 应用过滤条件
            if device_id:
                query = query.filter(SensorData.device_id == device_id)
            
            if sensor_type:
                query = query.filter(SensorData.sensor_type == sensor_type)
            
            # 处理时间范围
            start_time = None
            end_time = None
            
            if start_time_str:
                try:
                    # 解析前端发送的ISO格式时间（UTC时间）
                    start_time = datetime.datetime.fromisoformat(start_time_str.replace('Z', '+00:00'))
                    # 转换为北京时间进行比较
                    if start_time.tzinfo is None:
                        start_time = pytz.UTC.localize(start_time)
                    start_time = start_time.astimezone(pytz.timezone('Asia/Shanghai'))
                    query = query.filter(SensorData.timestamp >= start_time)
                except ValueError:
                    return jsonify({"error": "start_time格式无效，应为ISO格式"}), 400
            
            if end_time_str:
                try:
                    # 解析前端发送的ISO格式时间（UTC时间）
                    end_time = datetime.datetime.fromisoformat(end_time_str.replace('Z', '+00:00'))
                    # 转换为北京时间进行比较
                    if end_time.tzinfo is None:
                        end_time = pytz.UTC.localize(end_time)
                    end_time = end_time.astimezone(pytz.timezone('Asia/Shanghai'))
                    query = query.filter(SensorData.timestamp <= end_time)
                except ValueError:
                    return jsonify({"error": "end_time格式无效，应为ISO格式"}), 400
            
            # 如果启用了聚合且有时间范围
            if aggregate and start_time and end_time:
                # 计算时间跨度
                time_diff = end_time - start_time
                total_seconds = time_diff.total_seconds()
                
                # 根据时间跨度确定聚合间隔（秒）
                if total_seconds <= 10800:  # 3小时内
                    interval_seconds = 60  # 1分钟
                elif total_seconds <= 259200:  # 3天内
                    interval_seconds = 300  # 5分钟
                elif total_seconds <= 2592000:  # 30天内
                    interval_seconds = 3600  # 1小时
                else:  # 超过30天
                    interval_seconds = 86400  # 1天
                
                # 使用SQL进行聚合查询
                
                # 为SQLite设计的时间聚合方案
                # 使用简单的时间格式化和分组方法，避免复杂的SQL表达式
                if interval_seconds == 60:  # 1分钟
                    # 1分钟间隔：将秒数设为00
                    time_format = func.strftime('%Y-%m-%d %H:%M:00', SensorData.timestamp).label('aggregated_time')
                elif interval_seconds == 300:  # 5分钟
                    # 5分钟间隔：将时间戳转换为秒数，然后按5分钟(300秒)为单位取整
                    seconds_since_epoch = func.cast(func.strftime('%s', SensorData.timestamp), Integer)
                    five_minute_block = (seconds_since_epoch / 300) * 300
                    time_format = func.datetime(five_minute_block, 'unixepoch').label('aggregated_time')
                elif interval_seconds == 3600:  # 1小时
                    # 1小时间隔：将分钟和秒都设为00
                    time_format = func.strftime('%Y-%m-%d %H:00:00', SensorData.timestamp).label('aggregated_time')
                elif interval_seconds == 86400:  # 1天
                    # 1天间隔：将小时、分钟和秒都设为00:00:00
                    time_format = func.strftime('%Y-%m-%d 00:00:00', SensorData.timestamp).label('aggregated_time')
                
                # 构建查询
                aggregate_query = db.session.query(
                    SensorData.device_id,
                    SensorData.sensor_type,
                    func.avg(SensorData.value).label('avg_value'),
                    time_format
                ).filter(
                    and_(
                        SensorData.timestamp >= start_time,
                        SensorData.timestamp <= end_time
                    )
                )
                
                if device_id:
                    aggregate_query = aggregate_query.filter(SensorData.device_id == device_id)
                if sensor_type:
                    aggregate_query = aggregate_query.filter(SensorData.sensor_type == sensor_type)
                
                # 按设备ID、传感器类型和聚合时间分组，按聚合时间降序排序
                # 聚合查询不应用limit限制，以确保用户能看到完整时间段的数据
                results = aggregate_query.group_by(
                    SensorData.device_id,
                    SensorData.sensor_type,
                    time_format
                ).order_by(
                    desc(time_format)
                ).all()
            else:
                # 按时间戳降序排序并限制数量
                results = query.order_by(SensorData.timestamp.desc()).limit(limit).all()
            
            # 转换为JSON格式
            data = []
            for result in results:
                if hasattr(result, 'avg_value'):  # 聚合数据
                    # 对于聚合数据，aggregated_time已经是格式化的字符串
                    data_item = {
                        "device_id": result.device_id,
                        "sensor_type": result.sensor_type,
                        "avg_value": float(result.avg_value),  # 确保是浮点数
                        "aggregated_time": result.aggregated_time  # 已经是字符串格式
                    }
                else:  # 原始数据
                    data_item = {
                        "id": result.id,
                        "device_id": result.device_id,
                        "sensor_type": result.sensor_type,
                        "value": result.value,
                        "timestamp": result.timestamp.isoformat()
                    }
                data.append(data_item)
            
            return jsonify(data), 200
            
    except Exception as e:
        error_message = f"获取传感器数据失败: {str(e)}"
        print(f"[错误] {error_message}")
        return jsonify({"error": error_message}), 500

# 辅助函数：获取数据库实例
def get_db():
    """从应用上下文中获取数据库实例"""
    if not hasattr(current_app, 'extensions'):
        raise RuntimeError("应用实例未初始化")
    
    # 尝试从Flask-SQLAlchemy扩展获取
    if 'sqlalchemy' in current_app.extensions:
        # 返回sqlalchemy实例，而不是直接返回db
        return current_app.extensions['sqlalchemy']
    
    raise RuntimeError("数据库实例不可用")

# 辅助函数：获取模型
def get_model(model_name):
    """从应用上下文中获取模型类"""
    if hasattr(current_app, 'models') and model_name in current_app.models:
        return current_app.models[model_name]
    
    raise RuntimeError(f"模型 {model_name} 不可用")