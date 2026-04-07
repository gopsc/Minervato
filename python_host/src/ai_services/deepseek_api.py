from flask import request, jsonify, Response
from flask_jwt_extended import jwt_required
import json
import requests
import uuid
import os
import logging

from .note_search import retrieve_notes_content, note_directory_browser
from .document_generator import generate_document_internal
from .utils import get_document_templates

@jwt_required()
def deepseek_api():
    """调用DeepSeek大模型并返回流式响应，使用function calling让模型自行决定是否调用文档生成"""
    try:
        data = request.get_json()
        prompt = data.get('prompt', '')
        
        if not prompt:
            return jsonify({"error": "提示词不能为空"}), 400
        
        # DeepSeek API配置
        deepseek_api_url = "https://api.deepseek.com/v1/chat/completions"  # 假设的API地址
        # 从环境变量读取API密钥
        deepseek_api_key = os.environ.get("DEEPSEEK_API_KEY")
        if not deepseek_api_key:
            logging.error("DeepSeek API密钥未设置，请在环境变量中设置DEEPSEEK_API_KEY")
            return {"success": False, "error": "DeepSeek API密钥未配置"}
        
        # 获取员工角色提示词（避免依赖应用上下文）
        try:
            # 使用不依赖应用上下文的方式读取提示词文件
            basedir = os.path.abspath(os.path.dirname(__file__))
            prompt_path = os.path.join(basedir, 'prompts', 'employee_role.txt')
            with open(prompt_path, 'r', encoding='utf-8') as f:
                employee_prompt = f.read().strip()
        except Exception as e:
            print(f"读取提示词文件失败: {e}")
            employee_prompt = ""
        
        # 构建请求参数，结合角色提示词和用户输入
        messages = []
        if employee_prompt:
            messages.append({"role": "system", "content": employee_prompt})
        messages.append({"role": "user", "content": prompt})
        
        # 获取文档模板列表
        document_templates = get_document_templates()
        
        # 初始化全局function_tools，传入文档模板列表
        from . import init_function_tools
        tools = init_function_tools(document_templates)
        
        payload = {
            "model": "deepseek-chat",  # 假设的模型名称
            "messages": messages,
            "tools": tools,  # 使用包含所有工具（包括note_directory_browser）的全局function_tools
            "tool_choice": "auto",  # 让模型自动决定是否调用工具
            "stream": True,
            "temperature": 0.2  # 降低温度值，使输出更加确定和一致
        }
        
        # 发送请求到DeepSeek API
        headers = {
            "Content-Type": "application/json",
            "Authorization": f"Bearer {deepseek_api_key}"
        }
        
        # 流式处理响应
        def generate():
            print(f"[日志] 开始调用DeepSeek API，stream模式")
            with requests.post(deepseek_api_url, headers=headers, json=payload, stream=True) as response:
                print(f"[日志] API响应状态码: {response.status_code}")
                if response.status_code != 200:
                    error_msg = f"API请求失败: {response.status_code} - {response.text}"
                    print(f"[错误] {error_msg}")
                    yield json.dumps({"error": error_msg}) + '\n'
                    return
                
                # 用于累积流式返回的函数参数
                accumulated_function_args = ""
                function_name = None
                has_content = False
                all_chunks = []
                
                # 处理流式响应
                for chunk in response.iter_lines():
                    if chunk:
                        # 处理SSE格式的响应
                        chunk = chunk.decode('utf-8').strip()
                        #print(f"[日志] 收到chunk: {chunk[:100]}..." if len(chunk) > 100 else f"[日志] 收到chunk: {chunk}")
                        all_chunks.append(chunk)
                        
                        if chunk.startswith('data:'):
                            chunk_data = chunk[5:].strip()
                            if chunk_data != '[DONE]':
                                try:
                                    json_data = json.loads(chunk_data)
                                    #print(f"[日志] 解析的JSON数据: {json.dumps(json_data, ensure_ascii=False)[:200]}...")
                                        
                                    if 'choices' in json_data and json_data['choices']:
                                        choice = json_data['choices'][0]
                                            
                                        # 检查顶层tool_calls
                                        if 'tool_calls' in choice and isinstance(choice['tool_calls'], list):
                                            for tool_call in choice['tool_calls']:
                                                if ('function' in tool_call and 
                                                    isinstance(tool_call['function'], dict) and 
                                                    'arguments' in tool_call['function']):
                                                    # 初始化或更新函数名
                                                    if function_name is None:
                                                        function_name = tool_call['function'].get('name', 'generate_document')
                                                    # 累积函数参数
                                                    arg_part = tool_call['function']['arguments']
                                                    accumulated_function_args += arg_part
                                                    #print(f"[日志] 累积参数(顶层): '{arg_part}', 当前累积: '{accumulated_function_args[:50]}...'")
                                                # 根据不同的工具名称显示相应的消息
                                                if 'function' in tool_call and isinstance(tool_call['function'], dict):
                                                    tool_name = tool_call['function'].get('name', 'unknown')
                                                    if tool_name == 'generate_document':
                                                        yield json.dumps({"content": "我将为您生成所需文档..."}) + '\n'
                                                    elif tool_name == 'retrieve_notes_content':
                                                        yield json.dumps({"content": "正在检索笔记内容，请稍候..."}) + '\n'
                                                    else:
                                                        yield json.dumps({"content": f"正在调用工具: {tool_name}，请稍候..."}) + '\n'
                                        # 检查delta中是否有tool_calls
                                        if 'delta' in choice and isinstance(choice['delta'], dict):
                                            delta = choice['delta']
                                                
                                            # 处理delta中的tool_calls
                                            if 'tool_calls' in delta and isinstance(delta['tool_calls'], list):
                                                for tool_call in delta['tool_calls']:
                                                    if ('function' in tool_call and 
                                                        isinstance(tool_call['function'], dict)):
                                                        # 提取函数名（如果有）
                                                        if 'name' in tool_call['function']:
                                                            function_name = tool_call['function']['name']
                                                        elif function_name is None:
                                                            function_name = 'generate_document'
                                                        # 提取并累积参数（如果有）
                                                        if 'arguments' in tool_call['function']:
                                                            arg_part = tool_call['function']['arguments']
                                                            accumulated_function_args += arg_part
                                                            #print(f"[日志] 累积参数(delta): '{arg_part}', 当前累积: '{accumulated_function_args[:50]}...'")
                                                
                                            # 处理普通内容输出
                                            elif 'content' in delta:
                                                content = delta['content']
                                                if content:
                                                    has_content = True
                                                    #print(f"[日志] 输出内容: {content[:50]}...")
                                                    yield json.dumps({"content": content}) + '\n'
                                except json.JSONDecodeError as e:
                                    error_msg = f"解析响应失败: {str(e)}"
                                    print(f"[错误] {error_msg}")
                                    yield json.dumps({"error": error_msg}) + '\n'
                
                # 检查是否有累积的函数参数
                print(f"[日志] 所有chunk处理完毕，累积参数长度: {len(accumulated_function_args)}")
                print(f"[日志] 累积的函数参数: '{accumulated_function_args[:100]}...'" if accumulated_function_args else "[日志] 没有累积的函数参数")
                
                # 如果有累积的函数参数，处理文档生成
                if accumulated_function_args.strip():
                    try:
                        function_args = accumulated_function_args.strip()
                        print(f"[日志] 开始处理工具调用: {function_name or 'generate_document'}")
                        print(f"[日志] 工具调用参数: '{function_args}'")
                        
                        # 由于DeepSeek返回的参数可能不是有效的JSON，我们需要处理这种情况
                        # 首先尝试作为JSON解析
                        try:
                            args = json.loads(function_args)
                            print(f"[日志] 成功解析工具参数: {function_args[:100]}...")
                        except json.JSONDecodeError:
                            # 如果解析失败，使用空字典
                            print(f"[日志] 参数不是有效的JSON，使用空字典")
                            args = {}
                        
                        # 根据工具名称调用相应的函数
                        result = None
                        tool_name = function_name or 'generate_document'
                        print(f"[日志] 开始处理工具调用: {tool_name}")
                        
                        # 根据不同的工具名称调用不同的函数
                        if tool_name == 'generate_document':
                            # 处理文档生成工具
                            document_requirements = args.get('document_requirements', function_args)
                            document_template = args.get('document_template')
                            
                            print(f"[日志] 文档需求: {document_requirements[:100]}...")
                            if document_template:
                                print(f"[日志] 使用文档模板: {document_template}")
                            
                            # 显示正在生成文档的消息
                            yield json.dumps({"content": "正在生成文档，请稍候..."}) + '\n'
                            
                            # 直接调用文档生成函数，注意为模板名添加.json扩展名
                            print(f"[日志] 调用generate_document_internal生成文档")
                            # 如果指定了模板，添加.json扩展名
                            template_with_ext = None
                            if document_template:
                                template_with_ext = f"{document_template}.json"
                                print(f"[日志] 添加.json扩展名后的模板名: {template_with_ext}")
                            result = generate_document_internal(document_requirements, template_with_ext)
                            print(f"[日志] 文档生成结果: {result.get('success')}")
                            
                            # 检查文档生成是否成功
                            if result.get('success', False):
                                # 生成一个临时ID来标识这个文档请求
                                doc_id = str(uuid.uuid4())[:8]
                                
                                # 从flask导入current_app，确保可以访问应用实例
                                from flask import current_app
                                # 创建一个模块级别的缓存字典作为后备
                                import sys
                                # 导入document_handler模块
                                from . import document_handler
                                # 创建当前模块的缓存字典作为后备
                                module = sys.modules[__name__]
                                if not hasattr(module, '_document_cache'):
                                    module._document_cache = {}
                                
                                # 将生成的文档内容存储在会话或内存中
                                # 使用try-except确保即使没有应用上下文也能处理
                                try:
                                    # 确保有应用上下文
                                    if hasattr(current_app, 'config'):
                                        # 使用current_app.config直接访问
                                        current_app.config['document_cache'] = current_app.config.get('document_cache', {})
                                        current_app.config['document_cache'][doc_id] = result
                                    module._document_cache[doc_id] = result
                                    # 同时更新document_handler模块的缓存
                                    document_handler._document_cache[doc_id] = result
                                    print(f"[日志] 文档缓存成功保存到document_handler模块")
                                except RuntimeError:
                                    # 处理应用上下文错误
                                    module._document_cache[doc_id] = result
                                    # 同时更新document_handler模块的缓存
                                    document_handler._document_cache[doc_id] = result
                                    print(f"[警告] 在没有应用上下文的情况下使用模块内存存储文档缓存")
                                print(f"[日志] 文档缓存成功，ID: {doc_id}")
                                
                                # 通知前端文档已生成，提供下载URL
                                download_info = {
                                    "action_required": "download_document",
                                    "document_id": doc_id,
                                    "filename": result["filename"],
                                    "message": "文档已生成，请点击下载按钮获取"
                                }
                                print(f"[日志] 发送下载通知: {download_info}")
                                yield json.dumps(download_info) + '\n'
                            else:
                                # 文档生成失败
                                error_msg = f"文档生成失败: {result.get('error', '未知错误')}"
                                print(f"[错误] {error_msg}")
                                yield json.dumps({"content": error_msg}) + '\n'
                        
                        elif tool_name == 'retrieve_notes_content':
                            # 处理笔记内容检索工具
                            keyword = args.get('keyword', '')
                            limit = args.get('limit', 10)
                            include_private = args.get('include_private', False)
                            
                            print(f"[日志] 笔记检索参数 - 关键词: {keyword}, 限制: {limit}, 包含私有笔记: {include_private}")
                            
                            # 调用笔记检索函数
                            try:
                                result = retrieve_notes_content(keyword, limit, include_private)
                                print(f"[日志] 笔记检索结果 - 成功: {result.get('success', False)}")
                                
                                # 格式化结果并返回
                                if result.get('success'):
                                    # 检查是否有content字段且不为空
                                    if result.get('content'):
                                        # 构建返回消息
                                        content = result['content']
                                        if isinstance(content, dict):
                                            # 如果content是单个字典（节点内容）
                                            title = content.get('title', '无标题')
                                            content_text = content.get('content', '')
                                            content_preview = content_text[:100] + "..." if len(content_text) > 100 else content_text
                                            notes_summary = f"**{title}**\n{content_preview}\n\n"
                                        elif isinstance(content, list):
                                            # 如果content是列表（多个笔记）
                                            notes_summary = "找到以下笔记内容：\n\n"
                                            for idx, note in enumerate(content, 1):
                                                title = note.get('title', '无标题')
                                                content_preview = note.get('content', '')[:100] + "..." if len(note.get('content', '')) > 100 else note.get('content', '')
                                                notes_summary += f"{idx}. **{title}**\n{content_preview}\n\n"
                                        else:
                                            notes_summary = str(content)
                                            
                                        yield json.dumps({"content": notes_summary}) + '\n'
                                    else:
                                        yield json.dumps({"content": result.get('message', '未找到相关内容')}) + '\n'
                                else:
                                    error_msg = result.get('error', result.get('message', '检索失败'))
                                    yield json.dumps({"content": f"笔记检索失败: {error_msg}"}) + '\n'
                            except Exception as e:
                                error_msg = f"笔记检索失败: {str(e)}"
                                print(f"[错误] {error_msg}")
                                yield json.dumps({"content": error_msg}) + '\n'
                        
                        elif tool_name == 'note_directory_browser':
                            # 处理笔记目录浏览工具
                            node_id = args.get('node_id', 0)
                            action = args.get('action', 'list')
                            target_node_id = args.get('target_node_id')
                            include_private = args.get('include_private', False)
                            
                            print(f"[日志] 笔记目录浏览参数 - 节点ID: {node_id}, 操作: {action}, 目标节点ID: {target_node_id}, 包含私有笔记: {include_private}")
                            
                            # 调用笔记目录浏览函数
                            try:
                                result = note_directory_browser(node_id, action, target_node_id, include_private)
                                print(f"[日志] 笔记目录浏览结果 - 成功: {result.get('success')}")
                                
                                # 格式化结果并返回
                                if result.get('success'):
                                    # 根据操作类型返回不同的信息
                                    if action == 'list':
                                        nodes = result.get('nodes', [])
                                        if nodes:
                                            node_list = "目录内容：\n\n"
                                            for node in nodes:
                                                node_type = "文件夹" if node.get('is_folder', False) else "笔记"
                                                node_list += f"- [{node.get('id')}] {node.get('title', '无标题')} ({node_type})\n"
                                            node_list += "\n您可以使用 'enter' 操作进入子节点，或使用 'view' 操作查看笔记内容。"
                                            yield json.dumps({"content": node_list}) + '\n'
                                        else:
                                            yield json.dumps({"content": "当前目录为空。"}) + '\n'
                                    elif action == 'view':
                                        content = result.get('content', '')
                                        title = result.get('title', '无标题')
                                        yield json.dumps({"content": f"## {title}\n\n{content}"}) + '\n'
                                    elif action == 'enter':
                                        new_node_id = result.get('new_node_id')
                                        title = result.get('title', '无标题')
                                        yield json.dumps({"content": f"已进入 '{title}'，当前节点ID: {new_node_id}。您可以使用 'list' 操作查看其内容。"}) + '\n'
                                else:
                                    yield json.dumps({"content": f"操作失败: {result.get('message', '未知错误')}"}) + '\n'
                            except Exception as e:
                                error_msg = f"笔记目录浏览失败: {str(e)}"
                                print(f"[错误] {error_msg}")
                                yield json.dumps({"content": error_msg}) + '\n'
                        
                        else:
                            # 处理未知工具
                            yield json.dumps({"content": f"未知的工具名称: {tool_name}，无法调用。"}) + '\n'
                    except Exception as e:
                        error_msg = f"处理文档生成时出错: {str(e)}"
                        print(f"[错误] {error_msg}")
                        yield json.dumps({"content": f"处理文档生成时遇到问题: {str(e)}"}) + '\n'
                        # 即使出错，也继续让对话进行下去
                        yield json.dumps({"content": "处理文档生成时遇到问题，您可以继续与我对话或尝试再次请求生成文档。"}) + '\n'
                elif not has_content:
                    # 如果没有输出内容且没有工具调用，提供一个默认响应
                    print(f"[日志] 没有内容输出且没有工具调用，发送默认响应")
                    yield json.dumps({"content": "我已收到您的请求，请继续详细说明您的需求，我可以为您提供帮助或生成相关文档。"}) + '\n'
                else:
                    print(f"[日志] 正常内容输出完成")
        
        # 返回流式响应
        return Response(generate(), content_type='text/event-stream')
        
    except Exception as e:
        return jsonify({"error": f"服务器内部错误: {str(e)}"}), 500
    
