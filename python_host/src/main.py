#!/usr/bin/env python3

from flask import Flask, request, jsonify, render_template, Response, stream_with_context, current_app
from flask_cors import CORS
from flask_sqlalchemy import SQLAlchemy
from sqlalchemy import Integer
from flask_jwt_extended import JWTManager, create_access_token, jwt_required, get_jwt_identity
from waitress import serve
import os
import datetime
import json
import requests
import pytz
from werkzeug.security import generate_password_hash, check_password_hash

# 获取当前北京时间
def get_beijing_time():
    return datetime.datetime.now(pytz.timezone('Asia/Shanghai'))

# 读取标题文件
def get_site_title():
    title_path = os.path.join(basedir, 'title.txt')
    try:
        with open(title_path, 'r', encoding='utf-8') as f:
            return f.read().strip()
    except Exception as e:
        print(f"读取标题文件失败: {e}")
        return "公司内部面板"  # 默认标题

# 读取提示词文件
def get_prompt(prompt_name='employee_role'):
    prompt_path = os.path.join(basedir, 'prompts', f'{prompt_name}.txt')
    try:
        with open(prompt_path, 'r', encoding='utf-8') as f:
            return f.read().strip()
    except Exception as e:
        print(f"读取提示词文件失败: {e}")
        return ""  # 返回空字符串作为默认值

basedir = os.path.abspath(os.path.dirname(__file__))

app = Flask(__name__, static_folder='static')
CORS(app)

app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///' + os.path.join(basedir, 'notes.db')
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
# JWT配置
app.config['JWT_SECRET_KEY'] = 'dendrachy-secret-key-change-in-production'
app.config['JWT_ACCESS_TOKEN_EXPIRES'] = datetime.timedelta(days=30)
app.config['JWT_REFRESH_TOKEN_EXPIRES'] = datetime.timedelta(days=90)

db = SQLAlchemy(app)
jwt = JWTManager(app)

# 存储db实例到app对象中，供蓝图使用
app.db = db

# 用户模型
class User(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), unique=True, nullable=False)
    password_hash = db.Column(db.String(128), nullable=False)

    def set_password(self, password):
        from werkzeug.security import generate_password_hash
        self.password_hash = generate_password_hash(password)
    
    def check_password(self, password):
        from werkzeug.security import check_password_hash
        return check_password_hash(self.password_hash, password)
    
    def __repr__(self):
        return f"<User {self.username}>"

class Note(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    pid = db.Column(db.Integer, nullable=False)
    title = db.Column(db.String(128), nullable=False)
    content = db.Column(db.Text, nullable=False)
    user_id = db.Column(db.Integer, db.ForeignKey('user.id'), nullable=False)
    is_public = db.Column(db.Boolean, default=False, nullable=False)  # 公开性标志，False表示私有，True表示公开
    
    # 添加关系，方便查询
    user = db.relationship('User', backref=db.backref('notes', lazy=True))
    
    def __repr__(self):
        return f"<Node {self.title}>"

class SensorData(db.Model):
    """传感器数据模型"""
    id = db.Column(db.Integer, primary_key=True)
    device_id = db.Column(db.String(128), nullable=False)  # 设备标识符
    sensor_type = db.Column(db.String(128), nullable=False)  # 传感器类型
    value = db.Column(db.Float, nullable=False)  # 传感器值
    timestamp = db.Column(db.DateTime, default=get_beijing_time)  # 北京时间戳
    
    def __repr__(self):
        return f"<SensorData device_id={self.device_id}, sensor_type={self.sensor_type}, value={self.value}>"

# 在app实例中存储模型引用，供蓝图使用
app.models = {
    'User': User,
    'Note': Note,
    'SensorData': SensorData
}

# 从auth.py导入认证蓝图并注册
from auth import auth_bp
app.register_blueprint(auth_bp)

# 从ai_services.py导入AI服务蓝图并注册
from ai_services import ai_bp
app.register_blueprint(ai_bp)

# 从notes_service.py导入笔记服务蓝图并注册
from notes_service import notes_bp
app.register_blueprint(notes_bp)

with app.app_context():
    db.create_all()

@app.route('/register_page')
def register_page():
    """返回注册页面"""
    site_title = get_site_title()
    return render_template('register.html', site_title=site_title)

@app.route('/login')
def login_page():
    """返回登录页面"""
    site_title = get_site_title()
    return render_template('login.html', site_title=site_title)

@app.route('/')
def view_page():
    """返回应用主页 - 简单的验证，实际应该在前端处理token验证"""
    # 注意：这只是一个简单实现，实际项目中应该在前端通过Axios等拦截器处理token验证
    # 如果token不存在，前端应该跳转到登录页面
    site_title = get_site_title()
    return render_template('index.html', site_title=site_title)

@app.route('/api-docs')
def api_docs_page():
    """返回API文档页面"""
    site_title = get_site_title()
    return render_template('api_docs.html', site_title=site_title)


# 导入传感器服务
from sensor_services import sensor_bp

# 注册传感器服务蓝图
app.register_blueprint(sensor_bp)

ADDR='0.0.0.0'
PORT=9201
if __name__ == "__main__":
    print(f'Listen at http://{ADDR}:{PORT}')
    #app.run(host="127.0.0.1", port=8006)
    serve(app, host=ADDR, port=PORT)
