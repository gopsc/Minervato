import json
import os
import logging
import requests
import datetime
from jinja2 import Template
from flask import current_app

def get_prompt(prompt_name='employee_role'):
    """读取提示词文件"""
    basedir = current_app.root_path
    prompt_path = os.path.join(basedir, 'prompts', f'{prompt_name}.txt')
    try:
        with open(prompt_path, 'r', encoding='utf-8') as f:
            return f.read().strip()
    except Exception as e:
        print(f"读取提示词文件失败: {e}")
        return ""  # 返回空字符串作为默认值

# 从note_search模块导入笔记搜索相关功能
from .note_search import retrieve_notes_content, note_directory_browser
    

# 从document_generator模块导入文档生成功能
from .document_generator import generate_document_internal

def get_document_templates():
    """获取文档模板列表"""
    try:
        # 获取文档模板目录路径
        basedir = os.path.abspath(os.path.dirname(__file__))
        templates_dir = os.path.join(basedir, 'documents_templates')
        
        # 检查目录是否存在
        if not os.path.exists(templates_dir):
            print(f"[警告] 文档模板目录不存在: {templates_dir}")
            return []
        
        # 获取目录下的所有JSON文件
        templates = []
        for filename in os.listdir(templates_dir):
            if filename.endswith('.json'):
                # 移除.json扩展名
                templates.append(filename[:-5])
                
        return templates
    except Exception as e:
        print(f"[错误] 获取文档模板失败: {str(e)}")
        return []