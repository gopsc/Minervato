// 树状笔记功能模块

// 树状笔记模块 - 全局变量定义
window.currentNode = { id: '0', title: '根目录', content: '', pid: '-1', is_public: false };
window.breadcrumbPath = [{ id: '0', title: '根目录' }];

// 初始化笔记功能
function initializeNotes() {
    // 隐藏编辑器
    document.getElementById('notes-editor').style.display = 'none';
    
    // 重置到根节点
    window.currentNode = { id: '0', title: '根目录', content: '', pid: '-1' };
    window.breadcrumbPath = [{ id: '0', title: '根目录' }];
    updateBreadcrumb();
    
    // 加载根节点的子节点
    loadNotes('0');
}

// 加载笔记列表
async function loadNotes(pid) {
    try {
        const response = await window.apiRequest('/get_notes', {
            method: 'POST',
            body: JSON.stringify({ pid: pid })
        });
        
        if (!response.ok) {
            throw new Error('获取笔记失败');
        }
        
        const notes = await response.json();
        renderNotesList(notes);
    } catch (error) {
        console.error('加载笔记失败:', error);
        alert('加载笔记失败: ' + error.message);
    }
}

// 渲染笔记列表
function renderNotesList(notes) {
    const notesList = document.getElementById('child-notes-list');
    notesList.innerHTML = '';
    
    if (notes.length === 0) {
        const noNotesEl = document.createElement('div');
        noNotesEl.id = 'no-child-notes';
        noNotesEl.className = 'text-gray-500 text-center py-4';
        noNotesEl.textContent = '暂无子节点，点击上方"添加子节点"按钮创建';
        notesList.appendChild(noNotesEl);
        return;
    }
    
    notes.forEach(note => {
        const noteItem = document.createElement('div');
        noteItem.className = 'flex justify-between items-center p-3 border border-gray-200 rounded-lg hover:bg-gray-50 cursor-pointer';
        noteItem.dataset.id = note.id;
        noteItem.dataset.title = note.title;
        noteItem.dataset.content = note.content;
        noteItem.dataset.pid = note.pid;
        noteItem.dataset.username = note.username || '未知用户';
        noteItem.dataset.is_public = note.is_public || false;
        
        const titleEl = document.createElement('div');
        titleEl.className = 'flex items-center';
        const iconEl = document.createElement('i');
        iconEl.className = 'fa fa-file-text-o text-primary mr-2';
        const textEl = document.createElement('span');
        textEl.textContent = note.title;
        titleEl.appendChild(iconEl);
        titleEl.appendChild(textEl);
        
        const metaContainer = document.createElement('div');
        metaContainer.className = 'flex items-center space-x-2';
        
        // 公开状态标签
        const publicStatusEl = document.createElement('span');
        if (note.is_public) {
            publicStatusEl.className = 'text-xs px-1.5 py-0.5 rounded-full bg-green-100 text-green-800';
            publicStatusEl.innerHTML = '<i class="fa fa-globe mr-0.5"></i>公开';
        } else {
            publicStatusEl.className = 'text-xs px-1.5 py-0.5 rounded-full bg-gray-100 text-gray-600';
            publicStatusEl.innerHTML = '<i class="fa fa-lock mr-0.5"></i>私有';
        }
        
        // 作者信息
        const authorEl = document.createElement('span');
        authorEl.className = 'text-sm text-gray-500';
        authorEl.textContent = `由 ${note.username || '未知用户'} 创建`;
        
        metaContainer.appendChild(publicStatusEl);
        metaContainer.appendChild(authorEl);
        
        noteItem.appendChild(titleEl);
        noteItem.appendChild(metaContainer);
        
        // 添加点击事件
        noteItem.addEventListener('click', () => navigateToNote(note));
        
        notesList.appendChild(noteItem);
    });
}

// 导航到指定笔记
function navigateToNote(note) {
    window.currentNode = {
        id: note.id,
        title: note.title,
        content: note.content,
        pid: note.pid,
        username: note.username,
        is_public: note.is_public || false
    };
    
    // 更新面包屑
    updateBreadcrumb();
    
    // 显示当前节点内容
    displayCurrentNote();
    
    // 加载子节点
    loadNotes(note.id);
}

// 更新面包屑导航
function updateBreadcrumb() {
    // 检查当前节点是否已在面包屑路径中
    const currentIndex = window.breadcrumbPath.findIndex(item => item.id === window.currentNode.id);
    
    if (currentIndex !== -1) {
        // 如果节点已存在，截断路径到该节点
        window.breadcrumbPath = window.breadcrumbPath.slice(0, currentIndex + 1);
    } else {
        // 如果节点不存在，添加到路径末尾
        // 找到父节点在路径中的位置
        const parentIndex = window.breadcrumbPath.findIndex(item => item.id === window.currentNode.pid);
        if (parentIndex !== -1) {
            // 保留父节点及之前的路径
            window.breadcrumbPath = window.breadcrumbPath.slice(0, parentIndex + 1);
        }
        // 添加当前节点到路径
        window.breadcrumbPath.push({ id: window.currentNode.id, title: window.currentNode.title });
    }
    
    // 渲染面包屑
    const breadcrumbEl = document.getElementById('breadcrumb');
    breadcrumbEl.innerHTML = '';
    
    window.breadcrumbPath.forEach((item, index) => {
        const spanEl = document.createElement('span');
        spanEl.className = item.id === window.currentNode.id ? 'text-primary' : 'text-gray-600 hover:text-primary';
        spanEl.textContent = item.title;
        
        if (item.id !== window.currentNode.id) {
            spanEl.style.cursor = 'pointer';
            spanEl.addEventListener('click', () => {
                // 点击面包屑导航到对应节点
                navigateToBreadcrumbNode(item);
            });
        }
        
        breadcrumbEl.appendChild(spanEl);
        
        // 添加分隔符
        if (index < window.breadcrumbPath.length - 1) {
            const separator = document.createElement('span');
            separator.className = 'mx-2 text-gray-400';
            separator.textContent = '>';
            breadcrumbEl.appendChild(separator);
        }
    });
}

// 导航到面包屑中的节点
async function navigateToBreadcrumbNode(node) {
    if (node.id === '0') {
        // 根节点
        window.currentNode = { id: '0', title: '根目录', content: '', pid: '-1', is_public: false };
        displayCurrentNote();
        loadNotes('0');
        updateBreadcrumb();
    } else {
        try {
            // 获取节点详情
            const response = await window.apiRequest('/get_notes', {
                method: 'POST',
                body: JSON.stringify({ pid: node.pid })
            });
            
            if (!response.ok) {
                throw new Error('获取节点信息失败');
            }
            
            const notes = await response.json();
            const foundNote = notes.find(note => note.id === node.id);
            
            if (foundNote) {
                navigateToNote(foundNote);
            }
        } catch (error) {
            console.error('导航失败:', error);
        }
    }
}

// 显示当前节点内容
function displayCurrentNote() {
    document.getElementById('current-note-title').textContent = window.currentNode.title;
    
    const contentDisplay = document.getElementById('current-note-content-display');
    const authorDisplay = document.getElementById('current-note-author');
    const publicStatusDisplay = document.getElementById('current-note-is-public');
    const publicStatusIcon = document.getElementById('public-status-icon');
    const publicStatusText = document.getElementById('public-status-text');
    
    if (window.currentNode.id === '0') {
        contentDisplay.innerHTML = '<p class="text-gray-500">根节点没有内容</p>';
        authorDisplay.innerHTML = '';
        publicStatusDisplay.classList.add('hidden');
    } else {
        // 显示内容（简单的换行处理）
        const content = escapeHtml(window.currentNode.content).replace(/\n/g, '<br>');
        contentDisplay.innerHTML = `<p>${content}</p>`;
        
        // 显示作者信息
        authorDisplay.textContent = `由 ${window.currentNode.username || '未知用户'} 创建`;
        
        // 显示公开状态
        publicStatusDisplay.classList.remove('hidden');
        if (window.currentNode.is_public) {
            publicStatusIcon.innerHTML = '<i class="fa fa-globe"></i>';
            publicStatusText.textContent = '公开';
            publicStatusDisplay.classList.remove('bg-gray-100');
            publicStatusDisplay.classList.add('bg-green-100');
        } else {
            publicStatusIcon.innerHTML = '<i class="fa fa-lock"></i>';
            publicStatusText.textContent = '私有';
            publicStatusDisplay.classList.remove('bg-green-100');
            publicStatusDisplay.classList.add('bg-gray-100');
        }
    }
    
    // 启用/禁用删除按钮（根节点不能删除）
    document.getElementById('delete-current-node').disabled = window.currentNode.id === '0';
}

// 显示添加子节点表单
function showAddChildForm(pid) {
    const editor = document.getElementById('notes-editor');
    document.getElementById('editor-title').textContent = '添加子节点';
    document.getElementById('note-id').value = '';
    document.getElementById('note-pid').value = pid;
    document.getElementById('note-title-input').value = '';
    document.getElementById('note-content-input').value = '';
    // 默认设置为私有
    document.getElementById('note-is-public').checked = false;
    editor.style.display = 'block';
    document.getElementById('note-title-input').focus();
}

// 显示编辑表单
function showEditForm() {
    if (window.currentNode.id === '0') return; // 根节点不能编辑
    
    const editor = document.getElementById('notes-editor');
    document.getElementById('editor-title').textContent = '编辑笔记';
    document.getElementById('note-id').value = window.currentNode.id;
    document.getElementById('note-pid').value = window.currentNode.pid;
    document.getElementById('note-title-input').value = window.currentNode.title;
    document.getElementById('note-content-input').value = window.currentNode.content;
    // 设置公开性复选框状态
    document.getElementById('note-is-public').checked = window.currentNode.is_public;
    editor.style.display = 'block';
    document.getElementById('note-title-input').focus();
}

// 保存笔记
async function saveNote() {
    const id = document.getElementById('note-id').value;
    const pid = document.getElementById('note-pid').value;
    const title = document.getElementById('note-title-input').value.trim();
    const content = document.getElementById('note-content-input').value;
    const is_public = document.getElementById('note-is-public').checked;
    
    if (!title) {
        alert('标题不能为空');
        return;
    }
    
    try {
        let response;
        
        if (id) {
            // 更新笔记
            response = await window.apiRequest('/update_note', {
                method: 'POST',
                body: JSON.stringify({ id, pid, title, content, is_public })
            });
        } else {
            // 添加新笔记
            response = await window.apiRequest('/add_note', {
                method: 'POST',
                body: JSON.stringify({ pid, title, content, is_public })
            });
        }
        
        if (!response.ok) {
            throw new Error('保存失败');
        }
        
        // 隐藏编辑器
        document.getElementById('notes-editor').style.display = 'none';
        
        // 刷新当前节点的子节点
        loadNotes(window.currentNode.id);
        
        // 如果是编辑当前节点，更新当前节点信息
        if (id && id === window.currentNode.id) {
            window.currentNode.title = title;
            window.currentNode.content = content;
            window.currentNode.is_public = is_public;
            displayCurrentNote();
            updateBreadcrumb();
        }
        
    } catch (error) {
        console.error('保存笔记失败:', error);
        alert('保存失败: ' + error.message);
    }
}

// 删除笔记
async function deleteNote() {
    if (window.currentNode.id === '0') return; // 根节点不能删除
    
    if (!confirm('确定要删除这个笔记吗？')) {
        return;
    }
    
    try {
        const response = await window.apiRequest('/delete_note', {
            method: 'POST',
            body: JSON.stringify({ id: window.currentNode.id })
        });
        
        if (!response.ok) {
            throw new Error('删除失败');
        }
        
        // 导航到父节点
        if (window.currentNode.pid === '0') {
            // 如果父节点是根节点
            navigateToBreadcrumbNode({ id: '0', title: '根目录' });
        } else {
            // 找到父节点在面包屑中的位置
            const parentNode = window.breadcrumbPath.find(item => item.id === window.currentNode.pid);
            if (parentNode) {
                navigateToBreadcrumbNode(parentNode);
            } else {
                // 如果找不到父节点，返回根节点
                navigateToBreadcrumbNode({ id: '0', title: '根目录' });
            }
        }
        
    } catch (error) {
        console.error('删除笔记失败:', error);
        alert('删除失败: ' + error.message);
    }
}

// HTML 转义函数，防止 XSS 攻击
function escapeHtml(text) {
    const map = {
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        '"': '&quot;',
        "'": '&#039;'
    };
    return text.replace(/[&<>"']/g, m => map[m]);
}

// 显示树状笔记内容 - 暴露给全局作用域
window.showNotesContent = function() {
    document.getElementById('initial-content').style.display = 'none';
    document.getElementById('dashboard-content').style.display = 'none';
    document.getElementById('environment-content').style.display = 'none';
    document.getElementById('notes-content').style.display = 'block';
    
    // 初始化笔记功能
    initializeNotes();
    
    // 绑定按钮事件
    document.getElementById('add-child-node').addEventListener('click', () => {
        showAddChildForm(window.currentNode.id);
    });
    
    document.getElementById('edit-current-node').addEventListener('click', showEditForm);
    document.getElementById('delete-current-node').addEventListener('click', deleteNote);
    document.getElementById('save-note').addEventListener('click', saveNote);
    document.getElementById('cancel-edit').addEventListener('click', () => {
        document.getElementById('notes-editor').style.display = 'none';
    });
};

// 已经通过window.showNotesContent暴露给全局作用域