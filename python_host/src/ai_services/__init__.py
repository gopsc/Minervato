#!/usr/bin/env python3
from flask import Blueprint


# 全局工具函数列表
function_tools = []  # 将在运行时初始化

# 初始化function_tools的函数
def init_function_tools(document_templates=None):
    """
    初始化function_tools列表
    
    Args:
        document_templates: 文档模板列表，用于generate_document工具的enum参数
    
    Returns:
        初始化后的function_tools列表
    """
    global function_tools
    function_tools = [
        {
            "type": "function",
            "function": {
                "name": "generate_document",
                "description": "生成结构化JSON文档并提供下载。当用户请求需要生成正式文档、报告、计划书等结构化内容时使用。",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "document_requirements": {
                            "type": "string",
                            "description": "文档内容要求，包含需要生成的文档详细需求"
                        },
                        "document_template": {
                            "type": "string",
                            "description": "选择使用的文档模板文件名",
                            "enum": document_templates if document_templates else None
                        }
                    },
                    "required": ["document_requirements"]
                }
            }
        },
        {
            "type": "function",
            "function": {
                "name": "retrieve_notes_content",
                "description": "检索笔记模块中的特定内容",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "keyword": {
                            "type": "string",
                            "description": "要搜索的关键词"
                        },
                        "limit": {
                            "type": "integer",
                            "description": "返回结果的最大数量，默认为10",
                            "default": 10
                        },
                        "include_private": {
                            "type": "boolean",
                            "description": "是否包含私有笔记，默认为false",
                            "default": False
                        }
                    },
                    "required": ["keyword"]
                }
            }
        }
    ]
    return function_tools






# 创建AI服务蓝图
ai_bp = Blueprint('ai', __name__)

# 从document_generator模块导入文档生成函数
# 20251106(qing): 生成的文档存储在上下文中，是否应该及时清理
from .document_generator import generate_document as document_generator_generate_document
# 从document_handler模块导入文档下载函数
from .document_handler import download_document as document_handler_download_document
ai_bp.add_url_rule('/api/generate-document', 'generate_document', document_generator_generate_document, methods=['POST'])
ai_bp.add_url_rule('/api/download-document/<doc_id>', 'download_document', document_handler_download_document, methods=['GET'])

# 从deepseek_api模块导入deepseek_api函数并添加到蓝图
from .deepseek_api import deepseek_api as deepseek_api_deepseek_api
ai_bp.add_url_rule('/api/deepseek', 'deepseek_api', deepseek_api_deepseek_api, methods=['POST'])

# 导入document_handler模块并初始化缓存共享
from . import document_handler

# 如果需要共享缓存，可以在这里设置
def get_document_cache():
    """获取共享的文档缓存"""
    return document_handler._document_cache
