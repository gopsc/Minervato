#!/usr/bin/env python3

from flask import Blueprint, request, jsonify, current_app
from flask_jwt_extended import jwt_required, get_jwt_identity

# 创建笔记服务蓝图
notes_bp = Blueprint('notes', __name__)

@notes_bp.route('/add_note', methods=['POST'])
@jwt_required()
def add_note():
    current_username = get_jwt_identity()
    
    # 使用current_app获取模型和数据库引用
    User = current_app.models['User']
    Note = current_app.models['Note']
    db = current_app.db
    
    with current_app.app_context():
        # 获取当前用户对象
        current_user = User.query.filter_by(username=current_username).first()
        if not current_user:
            return jsonify({"error": "用户不存在"}), 404
    
    data = request.get_json()
    pid = data.get('pid')
    title = data.get('title')
    content = data.get('content')
    is_public = data.get('is_public', False)  # 默认设置为私有
    
    if pid is None or not title or not content:
        return jsonify({"error": "Pid, title and content are required"}), 400
    
    # 创建新笔记并关联用户
    new_note = Note(
        pid=int(pid),
        title=title,
        content=content,
        user_id=current_user.id,  # 关联到当前用户
        is_public=is_public  # 设置公开性
    )

    with current_app.app_context():
        db.session.add(new_note)
        db.session.commit()

    return jsonify({"message": "Note added successfully"}), 200


@notes_bp.route('/get_notes', methods=["POST"])
@jwt_required()
def get_notes():
    current_username = get_jwt_identity()
    
    # 使用current_app获取模型引用
    User = current_app.models['User']
    Note = current_app.models['Note']
    
    with current_app.app_context():
        # 获取当前用户对象
        current_user = User.query.filter_by(username=current_username).first()
        if not current_user:
            return jsonify({"error": "用户不存在"}), 404
    
    data = request.get_json()
    pid = data.get('pid')
    
    if not pid:
        pid = 0
    
    with current_app.app_context():
        # 查询条件：只能查看自己的笔记或者公开的笔记
        notes = Note.query.filter(
            (Note.pid == int(pid)) & 
            ((Note.user_id == current_user.id) | (Note.is_public == True))
        )
        result = [{
            'id': note.id, 
            'pid': note.pid, 
            'title': note.title, 
            'content': note.content,
            'user_id': note.user_id,
            'username': note.user.username if note.user else '未知用户',
            'is_public': note.is_public  # 返回公开状态
        } for note in notes]

    return jsonify(result), 200


@notes_bp.route('/delete_note', methods=['POST'])
@jwt_required()
def delete_note():
    current_username = get_jwt_identity()
    
    # 使用current_app获取模型和数据库引用
    User = current_app.models['User']
    Note = current_app.models['Note']
    db = current_app.db
    
    with current_app.app_context():
        # 获取当前用户对象
        current_user = User.query.filter_by(username=current_username).first()
        if not current_user:
            return jsonify({"error": "用户不存在"}), 404
    
    data = request.get_json()
    note_id = data.get('id')
    
    if not note_id:
        return jsonify({"error": "Note ID is required"}), 400
    
    with current_app.app_context():
        note = Note.query.get(note_id)
        
        if note:
            # 验证权限：只有笔记作者才能删除
            if note.user_id != current_user.id:
                return jsonify({"error": "无权删除此笔记"}), 403
            
            db.session.delete(note)
            db.session.commit()
            return jsonify({'message': "Note deleted successfully"}), 200
        else:
            return jsonify({"error": "Note not found"}), 404


@notes_bp.route('/update_note', methods=['POST'])
@jwt_required()
def update_note():
    current_username = get_jwt_identity()
    
    # 使用current_app获取模型和数据库引用
    User = current_app.models['User']
    Note = current_app.models['Note']
    db = current_app.db
    
    with current_app.app_context():
        # 获取当前用户对象
        current_user = User.query.filter_by(username=current_username).first()
        if not current_user:
            return jsonify({"error": "用户不存在"}), 404
    
    data = request.get_json()
    id = data.get('id')
    pid = data.get('pid')
    title = data.get('title')
    content = data.get('content')
    is_public = data.get('is_public')  # 获取公开性设置
    
    if id is None or pid is None or not title or not content:
        return jsonify({"error": "Invalid param"}), 400
    
    with current_app.app_context():
        note = Note.query.get(id)
        
        if note:
            # 验证权限：只有笔记作者才能更新
            if note.user_id != current_user.id:
                return jsonify({"error": "无权更新此笔记"}), 403
            
            note.pid = pid
            note.title = title
            note.content = content
            # 只有在请求中提供了is_public字段时才更新
            if is_public is not None:
                note.is_public = is_public
            db.session.commit()
            return jsonify({'message': "Your change is up-to-date."}), 200
        else:
            return jsonify({'error': 'Note not found'}), 400