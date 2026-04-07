from flask import jsonify, Response, current_app
import sys

# 初始化模块级文档缓存
_document_cache = {}


def download_document(doc_id):
    """
    根据文档ID下载已生成的文档
    """
    try:
        # 从缓存中获取文档
        result = None
        
        # 首先尝试从current_app.config获取
        try:
            if hasattr(current_app, 'config'):
                document_cache = current_app.config.get('document_cache', {})
                if doc_id in document_cache:
                    result = document_cache[doc_id]
        except RuntimeError:
            pass
        
        # 如果从current_app.config获取失败，尝试从模块内存存储获取
        if result is None:
            # 直接访问当前模块的缓存
            if doc_id in _document_cache:
                result = _document_cache[doc_id]
        
        # 如果文档不存在，返回错误
        if result is None:
            return jsonify({"error": "文档不存在或已过期"}), 404
        
        # 创建响应，将JSON内容作为txt文件提供下载
        response = Response(
            result['content'],
            mimetype='text/plain',
            headers={
                'Content-Disposition': f'attachment; filename="{result["filename"]}"'
            }
        )
        
        # 可选：下载后从缓存中删除，避免内存占用
        # del document_cache[doc_id]
        
        return response
    
    except Exception as e:
        error_message = f"下载文档失败: {str(e)}"
        print(f"[错误] {error_message}")
        return jsonify({"error": error_message}), 500