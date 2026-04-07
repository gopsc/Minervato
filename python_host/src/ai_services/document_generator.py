import json
import os
import logging
import requests
import datetime
from jinja2 import Template
from flask import request, jsonify, Response
from flask_jwt_extended import jwt_required

def generate_document_internal(prompt, template=None):
    """内部文档生成函数，接收提示词和可选的文档模板并生成文档内容
    
    Args:
        prompt: 文档生成提示词
        template: 可选的文档模板文件名
    
    Returns:
        包含文档内容和文件名的字典，或包含错误信息的字典
    """
    try:
        # DeepSeek API配置
        deepseek_api_url = "https://api.deepseek.com/v1/chat/completions"  # 假设的API地址
        # 从环境变量读取API密钥
        deepseek_api_key = os.environ.get("DEEPSEEK_API_KEY")
        if not deepseek_api_key:
            logging.error("DeepSeek API密钥未设置，请在环境变量中设置DEEPSEEK_API_KEY")
            raise Exception("DeepSeek API密钥未配置")
        
        # 构建提示词，包含可选的模板信息
        template_info = ""
        template_content = ""
        
        # 如果指定了模板，尝试读取模板内容
        if template:
            try:
                basedir = os.path.abspath(os.path.dirname(__file__))
                template_path = os.path.join(basedir, 'documents_templates', template)
                
                if os.path.exists(template_path):
                    with open(template_path, 'r', encoding='utf-8') as f:
                        template_content = f.read()
                        print(f"[日志] 成功读取模板内容: {template}")
                    template_info = f"请根据以下模板格式生成文档:\n{template_content}\n\n"
                else:
                    print(f"[警告] 指定的模板文件不存在: {template_path}")
            except Exception as e:
                print(f"[警告] 读取模板文件失败: {str(e)}")
        
        # 构建强制生成JSON格式的提示词
        json_format_prompt = f"""
        请根据以下需求生成一个结构化的JSON文档：
        {prompt}
        
        {template_info}
        请严格按照JSON格式输出，不要包含任何JSON之外的内容。JSON应该包含足够详细的信息来描述请求的文档内容。
        """
        
        # 获取文档生成提示词（避免依赖应用上下文）
        try:
            # 使用不依赖应用上下文的方式读取提示词文件
            basedir = os.path.abspath(os.path.dirname(__file__))
            prompt_path = os.path.join(basedir, 'prompts', 'document_generator.txt')
            with open(prompt_path, 'r', encoding='utf-8') as f:
                document_generator_prompt = f.read().strip()
        except Exception as e:
            print(f"读取提示词文件失败: {e}")
            document_generator_prompt = ""
        
        # 构建请求参数
        messages = []
        if document_generator_prompt:
            messages.append({"role": "system", "content": document_generator_prompt})
        else:
            # 备用系统提示词，以防提示词文件读取失败
            messages.append({"role": "system", "content": "你是一个专业的文档生成助手，严格按照要求生成JSON格式的文档内容。"})
        messages.append({"role": "user", "content": json_format_prompt})
        
        payload = {
            "model": "deepseek-chat",  # 假设的模型名称
            "messages": messages,
            "stream": False,  # 非流式响应
            "response_format": {"type": "json_object"},  # 强制返回JSON格式
            "temperature": 0.0  # 将温度降到最低，使输出完全确定和一致
        }
        
        # 发送请求到DeepSeek API
        headers = {
            "Content-Type": "application/json",
            "Authorization": f"Bearer {deepseek_api_key}"
        }
        
        response = requests.post(deepseek_api_url, headers=headers, json=payload)
        
        # 检查响应状态码
        if response.status_code != 200:
            raise Exception(f"API请求失败: {response.status_code} - {response.text}")
        
        # 解析API响应
        response_data = response.json()
        
        # 检查响应格式
        if 'choices' not in response_data or not response_data['choices']:
            raise Exception("API返回格式异常")
        
        # 获取生成的文档内容
        document_content = response_data['choices'][0].get('message', {}).get('content', '')
        
        # 验证返回内容是否为有效的JSON
        try:
            document_data = json.loads(document_content)
        except json.JSONDecodeError:
            raise Exception("生成的内容不是有效的JSON格式")
        
        # 尝试读取对应的文本模板文件（使用相同的模板名但.md扩展名）
        rendered_content = None
        if template:
            # 获取对应的txt模板文件
            template_name = os.path.splitext(template)[0]  # 移除.json扩展名
            txt_template_path = os.path.join(basedir, 'documents_templates', f'{template_name}.md')
            
            if os.path.exists(txt_template_path):
                try:
                    with open(txt_template_path, 'r', encoding='utf-8') as f:
                        txt_template_content = f.read()
                    
                    # 使用Jinja2渲染模板
                    template_obj = Template(txt_template_content)
                    rendered_content = template_obj.render(**document_data)
                    print(f"[日志] 成功使用文本模板渲染文档: {txt_template_path}")
                except Exception as e:
                    print(f"[警告] 渲染文本模板失败: {str(e)}")
                    # 渲染失败时，使用原始JSON内容
                    rendered_content = document_content
            else:
                print(f"[警告] 对应的文本模板文件不存在: {txt_template_path}")
                rendered_content = document_content
        else:
            rendered_content = document_content
        
        # 生成带时间戳的文件名
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"document_{timestamp}.txt"
        
        return {
            "filename": filename,
            "content": rendered_content,
            "success": True
        }
        
    except Exception as e:
        # 详细记录异常
        error_message = f"文档生成过程中出现错误: {str(e)}"
        print(f"[错误] {error_message}")
        # 返回错误信息，但允许前端继续处理
        return {
            "success": False,
            "error": error_message,
            "filename": f"error_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.txt",
            "content": f"文档生成失败: {str(e)}"
        }

@jwt_required()
def generate_document():
    """调用DeepSeek大模型生成文档并提供下载（需要JWT认证）"""
    try:
        data = request.get_json()
        prompt = data.get('prompt', '')
        document_template = data.get('document_template')
        
        if not prompt:
            return jsonify({"error": "提示词不能为空"}), 400
        
        try:
            # 直接调用内部文档生成函数，注意为模板名添加.json扩展名
            # 如果指定了模板，添加.json扩展名
            template_with_ext = None
            if document_template:
                template_with_ext = f"{document_template}.json"
                print(f"[日志] 添加.json扩展名后的模板名: {template_with_ext}")
            result = generate_document_internal(prompt, template_with_ext)
            
            # 检查是否成功
            if not result.get('success', False):
                # 返回JSON错误响应，前端可以据此显示通知
                return jsonify({"error": result.get("error", "文档生成失败"), "notification": True}), 500
            
            # 创建响应，将JSON内容作为txt文件提供下载
            return Response(
                result['content'],
                mimetype='text/plain',
                headers={
                    'Content-Disposition': f'attachment; filename="{result["filename"]}"'
                }
            )
        except RuntimeError as e:
            # 处理运行时错误
            error_message = f"文档生成运行时错误: {str(e)}"
            print(f"[错误] {error_message}")
            return jsonify({"error": error_message, "notification": True}), 500
        except Exception as e:
            # 处理其他所有异常
            error_message = f"文档生成过程中出现错误: {str(e)}"
            print(f"[错误] {error_message}")
            return jsonify({"error": error_message, "notification": True}), 500
    except Exception as e:
        # 捕获外层异常
        error_message = f"处理文档生成请求时出错: {str(e)}"
        print(f"[错误] {error_message}")
        return jsonify({"error": error_message}), 500