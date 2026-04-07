#!/usr/bin/env python3

from flask import Blueprint, request, jsonify, current_app
from flask_jwt_extended import create_access_token, jwt_required, get_jwt_identity, decode_token
from werkzeug.security import generate_password_hash, check_password_hash

# 创建认证蓝图
auth_bp = Blueprint('auth', __name__)

# 用户注册
@auth_bp.route('/register', methods=['POST'])
def register():
    # 使用current_app访问数据库
    db = current_app.extensions['sqlalchemy']
    # 动态获取User模型
    User = current_app.models['User']
    
    data = request.get_json()
    username = data.get('username')
    password = data.get('password')
    
    if not username or not password:
        return jsonify({"error": "用户名和密码不能为空"}), 400
    
    # 检查用户是否已存在
    if User.query.filter_by(username=username).first():
        return jsonify({"error": "用户名已存在"}), 400
    
    # 创建新用户
    new_user = User(username=username)
    new_user.set_password(password)
    
    db.session.add(new_user)
    db.session.commit()
    
    return jsonify({"message": "注册成功"}), 201

# 用户登录
@auth_bp.route('/login', methods=['POST'])
def login():
    # 使用current_app访问数据库
    db = current_app.extensions['sqlalchemy']
    # 动态获取User模型
    User = current_app.models['User']
    
    data = request.get_json()
    username = data.get('username')
    password = data.get('password')
    
    if not username or not password:
        return jsonify({"error": "用户名和密码不能为空"}), 400
    
    user = User.query.filter_by(username=username).first()
    
    if not user or not user.check_password(password):
        return jsonify({"error": "用户名或密码错误"}), 401
    
    # 创建访问令牌和刷新令牌
    access_token = create_access_token(identity=username)
    refresh_token = create_access_token(identity=username, fresh=False)
    return jsonify({"access_token": access_token, "refresh_token": refresh_token}), 200

# 获取当前用户信息
@auth_bp.route('/profile', methods=['GET'])
@jwt_required()
def profile():
    current_user = get_jwt_identity()
    return jsonify({"username": current_user}), 200

# 刷新访问令牌
@auth_bp.route('/refresh', methods=['POST'])
def refresh():
    refresh_token = request.json.get('refresh_token')
    if not refresh_token:
        return jsonify({"error": "刷新令牌不能为空"}), 400
    
    try:
        # 直接解码刷新令牌获取用户身份
        decoded = decode_token(refresh_token)
        identity = decoded.get('sub')
        
        if not identity:
            return jsonify({"error": "无效的刷新令牌"}), 401
        
        # 创建新的访问令牌
        new_access_token = create_access_token(identity=identity)
        return jsonify({"access_token": new_access_token}), 200
    except Exception as e:
        current_app.logger.error(f"刷新令牌验证失败: {str(e)}")
        return jsonify({"error": "刷新令牌验证失败"}), 401