#!/usr/bin/env python3

"""
AI Services 笔记处理模块

提供笔记内容处理、分析和转换功能。
"""

import re
import json
import requests
import markdown
from bs4 import BeautifulSoup
from datetime import datetime

def analyze_notes_content(content, analysis_type="summary"):
    """
    分析笔记内容，根据指定的分析类型进行处理
    
    Args:
        content: 要分析的笔记内容
        analysis_type: 分析类型，可以是summary(摘要)、keywords(关键词)、sentiment(情感分析)等
    
    Returns:
        dict: 包含分析结果的字典
    """
    try:
        # DeepSeek API配置
        deepseek_api_url = "https://api.deepseek.com/v1/chat/completions"
        # 从环境变量读取API密钥
        deepseek_api_key = os.environ.get("DEEPSEEK_API_KEY")
        if not deepseek_api_key:
            print("[错误] DeepSeek API密钥未设置，请在环境变量中设置DEEPSEEK_API_KEY")
            return {"success": False, "error": "DeepSeek API密钥未配置"}
        
        # 根据分析类型构建提示
        if analysis_type == "summary":
            prompt = f"""
            请对以下笔记内容生成一个简明扼要的摘要（不超过200字）：
            
            {content}
            """
        elif analysis_type == "keywords":
            prompt = f"""
            请从以下笔记内容中提取5-10个最关键的关键词：
            
            {content}
            
            请以JSON格式返回，例如：{{"keywords": ["关键词1", "关键词2", ...]}}
            """
        elif analysis_type == "sentiment":
            prompt = f"""
            请分析以下笔记内容的情感倾向，判断是积极、中性还是消极，并给出简短的理由：
            
            {content}
            
            请以JSON格式返回，例如：{{"sentiment": "积极", "reason": "理由内容"}}
            """
        else:
            return {
                "success": False,
                "message": f"不支持的分析类型: {analysis_type}"
            }
        
        # 调用大模型进行分析
        headers = {
            "Content-Type": "application/json",
            "Authorization": f"Bearer {deepseek_api_key}"
        }
        
        payload = {
            "model": "deepseek-chat",
            "messages": [
                {"role": "system", "content": "你是一个专业的内容分析助手，根据用户的要求进行文本分析。"},
                {"role": "user", "content": prompt}
            ],
            "temperature": 0.3
        }
        
        response = requests.post(deepseek_api_url, headers=headers, json=payload, timeout=15)
        response_data = response.json()
        
        if response.status_code == 200 and response_data.get("choices"):
            analysis_result = response_data["choices"][0]["message"]["content"]
            
            # 处理不同类型的结果
            if analysis_type == "summary":
                return {
                    "success": True,
                    "summary": analysis_result.strip()
                }
            elif analysis_type in ["keywords", "sentiment"]:
                try:
                    # 尝试解析JSON格式的结果
                    result_json = json.loads(analysis_result)
                    return {
                        "success": True,
                        "analysis_type": analysis_type,
                        **result_json
                    }
                except json.JSONDecodeError:
                    # 如果解析失败，直接返回文本结果
                    return {
                        "success": True,
                        "analysis_type": analysis_type,
                        "result": analysis_result.strip()
                    }
        else:
            error_msg = response_data.get("error", {}).get("message", "分析失败")
            return {
                "success": False,
                "message": f"大模型分析失败: {error_msg}"
            }
            
    except Exception as e:
        return {
            "success": False,
            "message": f"分析过程中发生错误: {str(e)}"
        }

def process_notes_content(content, process_type="format", **kwargs):
    """
    处理笔记内容，根据指定的处理类型进行转换
    
    Args:
        content: 要处理的笔记内容
        process_type: 处理类型，可以是format(格式化)、clean(清洗)、convert(转换)等
        **kwargs: 额外的处理参数
    
    Returns:
        dict: 包含处理结果的字典
    """
    try:
        if process_type == "format":
            # 格式化笔记内容
            formatted_content = _format_notes_content(content)
            return {
                "success": True,
                "formatted_content": formatted_content
            }
        
        elif process_type == "clean":
            # 清洗笔记内容
            clean_options = kwargs.get("options", {})
            cleaned_content = _clean_notes_content(content, **clean_options)
            return {
                "success": True,
                "cleaned_content": cleaned_content
            }
        
        elif process_type == "convert":
            # 转换笔记格式
            target_format = kwargs.get("target_format", "plaintext")
            converted_content = _convert_notes_content(content, target_format)
            return {
                "success": True,
                "converted_content": converted_content,
                "format": target_format
            }
        
        else:
            return {
                "success": False,
                "message": f"不支持的处理类型: {process_type}"
            }
            
    except Exception as e:
        return {
            "success": False,
            "message": f"处理过程中发生错误: {str(e)}"
        }

def extract_notes_entities(content, entity_types=None):
    """
    从笔记内容中提取实体信息
    
    Args:
        content: 要提取实体的笔记内容
        entity_types: 要提取的实体类型列表，如["person", "location", "time"]等，默认为None(全部提取)
    
    Returns:
        dict: 包含提取实体的字典
    """
    try:
        # DeepSeek API配置
        deepseek_api_url = "https://api.deepseek.com/v1/chat/completions"
        deepseek_api_key = "sk-029ae14d24e24af3b5d36f1e2b961c66"
        
        # 构建实体提取提示
        entity_types_str = ", ".join(entity_types) if entity_types else "人名、地点、时间、组织、事件等关键实体"
        prompt = f"""
        请从以下文本中提取{entity_types_str}。
        
        文本内容:
        {content}
        
        请以JSON格式返回提取结果，例如：
        {{
            "entities": [
                {{
                    "text": "实体文本",
                    "type": "实体类型",
                    "start_pos": 起始位置,
                    "end_pos": 结束位置
                }}
            ]
        }}
        """
        
        # 调用大模型提取实体
        headers = {
            "Content-Type": "application/json",
            "Authorization": f"Bearer {deepseek_api_key}"
        }
        
        payload = {
            "model": "deepseek-chat",
            "messages": [
                {"role": "system", "content": "你是一个实体提取助手，根据用户要求从文本中提取指定类型的实体。"},
                {"role": "user", "content": prompt}
            ],
            "temperature": 0.1
        }
        
        response = requests.post(deepseek_api_url, headers=headers, json=payload, timeout=15)
        response_data = response.json()
        
        if response.status_code == 200 and response_data.get("choices"):
            result_text = response_data["choices"][0]["message"]["content"]
            
            try:
                # 尝试解析JSON结果
                result = json.loads(result_text)
                return {
                    "success": True,
                    "entities": result.get("entities", []),
                    "entity_count": len(result.get("entities", []))
                }
            except json.JSONDecodeError:
                # 如果解析失败，返回默认格式
                return {
                    "success": True,
                    "entities": [],
                    "message": "实体提取成功但结果格式不正确",
                    "raw_result": result_text
                }
        else:
            error_msg = response_data.get("error", {}).get("message", "实体提取失败")
            return {
                "success": False,
                "message": f"实体提取失败: {error_msg}"
            }
            
    except Exception as e:
        return {
            "success": False,
            "message": f"实体提取过程中发生错误: {str(e)}"
        }

def _format_notes_content(content):
    """
    格式化笔记内容
    """
    # 去除多余的空白行
    lines = content.splitlines()
    formatted_lines = []
    
    for line in lines:
        stripped_line = line.rstrip()
        if stripped_line or (formatted_lines and formatted_lines[-1]):
            formatted_lines.append(stripped_line)
    
    # 处理标题格式（确保#后有空格）
    for i, line in enumerate(formatted_lines):
        if line.startswith('#'):
            # 修复标题格式
            match = re.match(r'(#+)', line)
            if match and match.end() < len(line) and line[match.end()] != ' ':
                formatted_lines[i] = line[:match.end()] + ' ' + line[match.end():]
    
    return '\n'.join(formatted_lines)

def _clean_notes_content(content, remove_extra_spaces=True, remove_empty_lines=True, 
                         remove_markdown=False, normalize_linebreaks=True):
    """
    清洗笔记内容
    """
    cleaned_content = content
    
    # 移除多余空格
    if remove_extra_spaces:
        cleaned_content = re.sub(r'\s+', ' ', cleaned_content)
    
    # 标准化换行符
    if normalize_linebreaks:
        cleaned_content = cleaned_content.replace('\r\n', '\n')
    
    # 移除空行
    if remove_empty_lines:
        lines = cleaned_content.split('\n')
        cleaned_content = '\n'.join([line.strip() for line in lines if line.strip()])
    
    # 移除Markdown格式
    if remove_markdown:
        # 转换为HTML，然后提取纯文本
        html_content = markdown.markdown(cleaned_content)
        soup = BeautifulSoup(html_content, 'html.parser')
        cleaned_content = soup.get_text(separator='\n')
    
    return cleaned_content

def _convert_notes_content(content, target_format):
    """
    转换笔记内容格式
    """
    if target_format == "html":
        # Markdown转HTML
        return markdown.markdown(content)
    elif target_format == "plaintext":
        # 转换为纯文本
        html_content = markdown.markdown(content)
        soup = BeautifulSoup(html_content, 'html.parser')
        return soup.get_text(separator='\n')
    elif target_format == "markdown":
        # 如果已经是Markdown，简单返回
        return content
    else:
        raise ValueError(f"不支持的目标格式: {target_format}")

def summarize_notes_content(content, max_length=200):
    """
    总结笔记内容，生成简短摘要
    
    Args:
        content: 要总结的笔记内容
        max_length: 摘要最大长度，默认为200字
    
    Returns:
        dict: 包含摘要的字典
    """
    return analyze_notes_content(content, analysis_type="summary")

def extract_notes_keywords(content, keyword_count=8):
    """
    提取笔记内容的关键词
    
    Args:
        content: 要提取关键词的笔记内容
        keyword_count: 要提取的关键词数量，默认为8个
    
    Returns:
        dict: 包含关键词的字典
    """
    result = analyze_notes_content(content, analysis_type="keywords")
    if result.get("success") and "keywords" in result:
        # 确保返回指定数量的关键词
        result["keywords"] = result["keywords"][:keyword_count]
    return result

# 导出的公共接口
__all__ = [
    'analyze_notes_content',
    'process_notes_content',
    'extract_notes_entities',
    'summarize_notes_content',
    'extract_notes_keywords'
]