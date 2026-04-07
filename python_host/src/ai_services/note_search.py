import json
import os
import logging
import requests
from flask import current_app

# 工具函数定义
def retrieve_notes_content(keyword, limit=10, include_private=False):
    """根据关键词检索笔记内容
    
    Args:
        keyword: 搜索关键词
        limit: 返回结果数量限制
        include_private: 是否包含私有笔记
        
    Returns:
        笔记内容列表
    """
    # 确保在应用上下文中运行
    try:
        # 直接尝试获取或创建应用上下文
        from flask import current_app as flask_current_app
        
        # 尝试获取应用实例
        if hasattr(flask_current_app, '_get_current_object'):
            app = flask_current_app._get_current_object()
            # 在应用上下文中调用内部实现函数
            with app.app_context():
                return _retrieve_notes_content_impl(keyword, limit, include_private)
        else:
            # 如果无法获取应用实例，尝试直接使用应用上下文
            with flask_current_app.app_context():
                return _retrieve_notes_content_impl(keyword, limit, include_private)
    except RuntimeError as e:
        # 捕获应用上下文未初始化的错误
        if "Working outside of application context" in str(e):
            # 尝试从main模块导入app实例
            try:
                from main import app
                with app.app_context():
                    return _retrieve_notes_content_impl(keyword, limit, include_private)
            except ImportError:
                print(f"[错误] 无法导入应用实例: {str(e)}")
                return {"success": False, "error": "应用上下文未初始化，无法导入应用实例"}
            except Exception as import_e:
                print(f"[错误] 使用导入的应用实例时出错: {str(import_e)}")
                return {"success": False, "error": f"应用上下文初始化失败: {str(import_e)}"}
        else:
            print(f"[错误] retrieve_notes_content 运行时错误: {str(e)}")
            return {"success": False, "error": f"运行时错误: {str(e)}"}
    except Exception as e:
        print(f"[错误] retrieve_notes_content 执行失败: {str(e)}")
        return {"success": False, "error": f"执行失败: {str(e)}"}

def _retrieve_notes_content_impl(keyword, limit=10, include_private=False):
    """
    retrieve_notes_content的实际实现，需要在应用上下文中运行
    """
    try:
        # 设置最大循环次数，避免无限循环
        max_iterations = 10
        current_node_id = 0  # 从根节点开始
        iteration = 0
        search_history = []  # 记录搜索历史
        
        # DeepSeek API配置
        deepseek_api_url = "https://api.deepseek.com/v1/chat/completions"
        # 从环境变量读取API密钥
        deepseek_api_key = os.environ.get("DEEPSEEK_API_KEY")
        if not deepseek_api_key:
            print("[错误] DeepSeek API密钥未设置，请在环境变量中设置DEEPSEEK_API_KEY")
            return {"success": False, "error": "DeepSeek API密钥未配置"}
        
        while iteration < max_iterations:
            iteration += 1
            print(f"[日志] 检索迭代 {iteration}: 当前节点ID = {current_node_id}")
            
            # 1. 调用note_directory_browser获取当前节点的子节点列表
            current_dir_info = note_directory_browser(node_id=current_node_id, action="list", include_private=include_private)
            
            if not current_dir_info.get("success", False):
                return {
                    "success": False,
                    "message": f"获取目录信息失败: {current_dir_info.get('message', '未知错误')}"
                }
            
            # 构建当前目录信息的文本描述
            current_node_info = current_dir_info.get("current_node_info", {})
            children = current_dir_info.get("children", [])
            
            dir_description = f"当前节点信息: 节点ID={current_node_id}"
            if current_node_info:
                dir_description += f", 标题={current_node_info.get('title', '未命名')}"
            
            dir_description += f"\n当前目录下的子节点 (共{len(children)}个):\n"
            for child in children:
                dir_description += f"- ID: {child['id']}, 标题: {child['title']}\n"
            
            # 记录当前目录信息到搜索历史
            search_history.append({
                "node_id": current_node_id,
                "node_title": current_node_info.get('title', '未命名') if current_node_info else '根节点',
                "children_count": len(children)
            })
            
            # 构建搜索历史描述
            history_description = "搜索历史:\n"
            for i, history in enumerate(search_history):
                history_description += f"{i+1}. 节点ID={history['node_id']}, 标题={history['node_title']}\n"
            
            # 2. 调用大模型，让它决定下一步操作
            decision_prompt = f"""
你是一个智能助手，帮助用户在笔记系统中查找与关键词相关的内容。

用户的搜索关键词是: {keyword}

{history_description}

{dir_description}

请根据关键词和当前目录结构，决定下一步操作：
1. 如果当前目录有多个子节点，请选择一个最可能包含与关键词相关内容的子节点ID
2. 如果当前目录没有子节点或你认为已经找到了目标节点，请选择查看当前节点内容
3. 如果所有尝试都失败，请返回'放弃'表示找不到相关内容

请以JSON格式返回你的决策：
- 要进入子节点: {{"action": "enter", "target_node_id": 子节点ID}}
- 要查看当前节点内容: {{"action": "view"}}
- 要放弃搜索: {{"action": "give_up"}}

请确保你的决策是基于关键词和目录内容的合理推断，不要猜测不存在的节点ID。
"""
            
            # 调用大模型获取决策
            headers = {
                "Content-Type": "application/json",
                "Authorization": f"Bearer {deepseek_api_key}"
            }
            
            payload = {
                "model": "deepseek-chat",
                "messages": [
                    {"role": "system", "content": "你是一个智能助手，帮助用户在笔记系统中查找内容。请以JSON格式返回你的决策。"},
                    {"role": "user", "content": decision_prompt}
                ],
                "temperature": 0.2
            }
            
            try:
                response = requests.post(deepseek_api_url, headers=headers, json=payload, timeout=10)
                response_data = response.json()
                
                # 提取模型的决策
                if response_data.get("choices") and response_data["choices"][0].get("message"):
                    model_response = response_data["choices"][0]["message"]["content"]
                    print(f"[日志] 大模型决策响应: {model_response}")
                    
                    # 解析JSON响应
                    try:
                        decision = json.loads(model_response)
                        action = decision.get("action", "")
                        
                        # 处理不同的决策类型
                        if action == "enter":
                            target_node_id = decision.get("target_node_id")
                            # 验证目标节点ID是否存在于当前子节点中
                            valid_node_ids = [child["id"] for child in children]
                            if target_node_id in valid_node_ids:
                                print(f"[日志] 进入子节点: {target_node_id}")
                                current_node_id = target_node_id
                            else:
                                print(f"[警告] 无效的目标节点ID: {target_node_id}")
                                # 继续查看当前节点内容
                                action = "view"
                        
                        if action == "view":
                            # 查看当前节点内容
                            content_info = note_directory_browser(node_id=current_node_id, action="view", include_private=include_private)
                            if content_info.get("success", False) and content_info.get("content"):
                                print(f"[日志] 找到节点内容: {current_node_id}")
                                return {
                                    "success": True,
                                    "message": "成功找到相关笔记内容",
                                    "content": content_info["content"],
                                    "search_history": search_history
                                }
                            else:
                                print(f"[警告] 当前节点无内容: {current_node_id}")
                                # 如果当前节点没有内容，且没有子节点，则搜索失败
                                if not children:
                                    return {
                                        "success": False,
                                        "message": "当前节点无内容且无子节点，搜索失败",
                                        "search_history": search_history
                                    }
                        
                        if action == "give_up":
                            return {
                                "success": False,
                                "message": "未能找到与关键词相关的内容",
                                "search_history": search_history
                            }
                            
                    except json.JSONDecodeError as e:
                        print(f"[错误] 解析大模型响应失败: {e}")
                        # 如果解析失败，尝试查看当前节点内容
                        content_info = note_directory_browser(node_id=current_node_id, action="view", include_private=include_private)
                        if content_info.get("success", False) and content_info.get("content"):
                            return {
                                "success": True,
                                "message": "成功找到相关笔记内容",
                                "content": content_info["content"],
                                "search_history": search_history
                            }
                            
                else:
                    print(f"[错误] 大模型响应格式不正确")
                    
            except Exception as e:
                print(f"[错误] 调用大模型失败: {e}")
                # 错误情况下，直接查看当前节点内容
                content_info = note_directory_browser(node_id=current_node_id, action="view", include_private=include_private)
                if content_info.get("success", False) and content_info.get("content"):
                    return {
                        "success": True,
                        "message": "成功找到相关笔记内容",
                        "content": content_info["content"],
                        "search_history": search_history
                    }
            
            # 如果没有子节点了，搜索失败
            if not children:
                return {
                    "success": False,
                    "message": "当前节点无子节点，搜索失败",
                    "search_history": search_history
                }
        
        # 达到最大迭代次数
        return {
            "success": False,
            "message": f"达到最大搜索次数({max_iterations})，未能找到相关内容",
            "search_history": search_history
        }
        
    except Exception as e:
        print(f"[错误] retrieve_notes_content 执行失败: {str(e)}")
        return {
            "success": False,
            "message": f"检索过程中发生错误: {str(e)}"
        }

def note_directory_browser(node_id=0, action="list", target_node_id=None, include_private=False):
    """
    循环访问笔记目录，返回目录列表给大模型选择
    
    Args:
        node_id: 当前节点ID，默认为0（根节点）
        action: 操作类型：list（列出目录）、enter（进入子节点）、view（查看当前节点内容）
        target_node_id: 当action为enter时，要进入的子节点ID
        include_private: 是否包含私有笔记，默认为false
    
    Returns:
        包含操作结果的字典，格式为：
        {
            "success": True/False,
            "message": "操作结果描述",
            "current_node_id": 当前节点ID,
            "current_node_info": 当前节点信息（如果有）,
            "children": 子节点列表（当action为list时）,
            "content": 当前节点内容（当action为view时）
        }
    """
    try:
        # 尝试获取或创建应用上下文
        from flask import current_app as flask_current_app
        
        # 尝试获取应用实例
        try:
            if hasattr(flask_current_app, '_get_current_object'):
                app = flask_current_app._get_current_object()
                # 在应用上下文中调用内部实现函数
                with app.app_context():
                    return _note_directory_browser_impl(node_id, action, target_node_id, include_private)
            else:
                # 如果无法获取应用实例，尝试直接使用应用上下文
                with flask_current_app.app_context():
                    return _note_directory_browser_impl(node_id, action, target_node_id, include_private)
        except RuntimeError as e:
            # 捕获应用上下文未初始化的错误
            if "Working outside of application context" in str(e):
                # 尝试从main模块导入app实例
                try:
                    from main import app
                    with app.app_context():
                        return _note_directory_browser_impl(node_id, action, target_node_id, include_private)
                except ImportError:
                    print(f"[错误] 无法导入应用实例: {str(e)}")
                    return {"success": False, "error": "应用上下文未初始化，无法导入应用实例", "current_node_id": node_id}
                except Exception as import_e:
                    print(f"[错误] 使用导入的应用实例时出错: {str(import_e)}")
                    return {"success": False, "error": f"应用上下文初始化失败: {str(import_e)}", "current_node_id": node_id}
            else:
                raise
    except Exception as e:
        # 处理异常
        return {
            "success": False,
            "message": f"操作失败: {str(e)}",
            "current_node_id": node_id
        }

def _note_directory_browser_impl(node_id=0, action="list", target_node_id=None, include_private=False):
    """
    note_directory_browser的实际实现，在应用上下文中运行
    """
    from flask import current_app
    
    # 使用current_app获取模型引用
    Note = current_app.models.get('Note')
    User = current_app.models.get('User')
    
    if not Note or not User:
        return {
            "success": False,
            "message": "无法获取笔记模型",
            "current_node_id": node_id
        }
    
    result = {
        "success": True,
        "message": "操作成功",
        "current_node_id": node_id
    }
    if action == "list":
        # 列出当前节点的子节点
        query = Note.query.filter(Note.pid == node_id)
        
        # 根据include_private决定是否只显示公开笔记
        if not include_private:
            query = query.filter(Note.is_public == True)
        
        children = query.all()
        
        result["children"] = [{
            "id": child.id,
            "title": child.title,
            "is_public": child.is_public,
            "username": child.user.username if child.user else "未知用户"
        } for child in children]
        
        result["message"] = f"已列出节点 {node_id} 的子节点，共 {len(children)} 个"
        
    elif action == "enter":
        # 进入子节点
        if target_node_id is None:
            return {
                "success": False,
                "message": "进入子节点时必须指定target_node_id",
                "current_node_id": node_id
            }
        
        # 检查目标节点是否存在且是当前节点的子节点
        target_node = Note.query.filter(
            Note.id == target_node_id,
            Note.pid == node_id
        ).first()
        
        if not target_node:
            return {
                "success": False,
                "message": f"节点 {target_node_id} 不存在或不是节点 {node_id} 的子节点",
                "current_node_id": node_id
            }
        
        # 检查是否有权限访问（公开或私有且include_private为True）
        if not target_node.is_public and not include_private:
            return {
                "success": False,
                "message": f"无权访问私有节点 {target_node_id}",
                "current_node_id": node_id
            }
        
        result["current_node_id"] = target_node_id
        result["current_node_info"] = {
            "id": target_node.id,
            "title": target_node.title,
            "pid": target_node.pid,
            "is_public": target_node.is_public,
            "username": target_node.user.username if target_node.user else "未知用户"
        }
        result["message"] = f"已进入节点 {target_node_id}: {target_node.title}"
        
        # 列出新节点的子节点
        children_query = Note.query.filter(Note.pid == target_node_id)
        if not include_private:
            children_query = children_query.filter(Note.is_public == True)
        
        children = children_query.all()
        result["children"] = [{
            "id": child.id,
            "title": child.title,
            "is_public": child.is_public,
            "username": child.user.username if child.user else "未知用户"
        } for child in children]
        
    elif action == "view":
        # 查看当前节点内容
        current_node = Note.query.get(node_id)
        
        if not current_node:
            # 根节点没有内容
            if node_id == 0:
                result["message"] = "当前在根节点，无内容可查看"
                result["content"] = None
            else:
                return {
                    "success": False,
                    "message": f"节点 {node_id} 不存在",
                    "current_node_id": node_id
                }
        else:
            # 检查访问权限
            if not current_node.is_public and not include_private:
                return {
                    "success": False,
                    "message": f"无权访问私有节点 {node_id}",
                    "current_node_id": node_id
                }
            
            result["content"] = {
                "id": current_node.id,
                "title": current_node.title,
                "content": current_node.content,
                "is_public": current_node.is_public,
                "username": current_node.user.username if current_node.user else "未知用户"
            }
            result["message"] = f"已获取节点 {node_id} 的内容"
    
    return result