// 事务预约相关功能代码

// 显示事务预约内容
function showDashboardContent() {
    // 隐藏其他内容区域
    document.getElementById('initial-content').style.display = 'none';
    document.getElementById('environment-content').style.display = 'none';
    document.getElementById('dashboard-content').style.display = 'flex';
    document.getElementById('notes-content').style.display = 'none';
    
    // 聚焦到输入框
    setTimeout(() => {
        const promptInput = document.getElementById('prompt-input');
        if (promptInput) promptInput.focus();
    }, 100);
}

// 发送消息函数
function sendMessage() {
    const promptInput = document.getElementById('prompt-input');
    const prompt = promptInput.value.trim();
    if (!prompt) return;
    
    const chatContainer = document.getElementById('chat-container');
    
    // 添加用户消息到聊天容器
    const userMessageHtml = `
        <div class="flex items-start justify-end">
            <div class="bg-primary text-white rounded-lg p-3 max-w-[80%]">
                <p>${escapeHtml(prompt)}</p>
            </div>
            <div class="w-8 h-8 bg-gray-200 rounded-full flex items-center justify-center ml-3 flex-shrink-0">
                <i class="fa fa-user"></i>
            </div>
        </div>
    `;
    chatContainer.insertAdjacentHTML('beforeend', userMessageHtml);
    
    // 清空输入框并滚动到底部
    promptInput.value = '';
    scrollToBottom();
    
    // 显示加载指示器
    const loadingIndicator = document.getElementById('loading-indicator');
    const errorMessage = document.getElementById('error-message');
    loadingIndicator.classList.remove('hidden');
    errorMessage.classList.add('hidden');
    
    // 创建AI响应消息容器
    const aiMessageId = 'ai-response-' + Date.now();
    const aiMessageHtml = `
        <div class="flex items-start">
            <div class="w-8 h-8 bg-primary text-white rounded-full flex items-center justify-center mr-3 flex-shrink-0">
                <i class="fa fa-robot"></i>
            </div>
            <div class="bg-gray-100 rounded-lg p-3 max-w-[80%]">
                <div id="${aiMessageId}" class="markdown-content text-gray-700"></div>
            </div>
        </div>
    `;
    
    // 隐藏加载指示器并添加AI消息容器
    loadingIndicator.classList.add('hidden');
    chatContainer.insertAdjacentHTML('beforeend', aiMessageHtml);
    scrollToBottom();
    
    // 调用DeepSeek API（使用apiRequest处理令牌刷新）
    const aiResponseElement = document.getElementById(aiMessageId);
    
    window.apiRequest('/api/deepseek', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ prompt: prompt })
    })
    .then(response => {
        if (!response.ok) {
            throw new Error(`API请求失败: ${response.status}`);
        }
        
        const reader = response.body.getReader();
        const decoder = new TextDecoder();
        
        // 用于累积完整的响应内容
        let fullResponseContent = '';
        
        function readStream() {
            return reader.read().then(({ done, value }) => {
                try {
                    // 处理接收到的数据流
                    const chunk = decoder.decode(value, { stream: true });
                    // 将接收到的数据按行分割
                    const lines = chunk.split('\n');
                    
                    lines.forEach(line => {
                        if (line.trim()) {
                            try {
                                const data = JSON.parse(line);
                                if (data.content) {
                                    // 累积完整内容
                                    fullResponseContent += data.content;
                                    // 流式更新文本内容，但不进行markdown解析
                                    aiResponseElement.textContent = fullResponseContent;
                                    scrollToBottom();
                                } else if (data.error) {
                                    errorMessage.textContent = data.error;
                                    errorMessage.classList.remove('hidden');
                                } else if (data.action_required && data.action_required === 'download_document') {
                                    // 处理下载通知
                                    const downloadButton = document.createElement('button');
                                    downloadButton.textContent = '下载文档';
                                    downloadButton.className = 'mt-2 bg-primary text-white px-4 py-2 rounded-lg hover:bg-primary/90';
                                    downloadButton.addEventListener('click', function() {
                                        // 构造下载URL并使用apiRequest触发下载
                                    const downloadUrl = `/api/download-document/${data.document_id}`;
                                    window.apiRequest(downloadUrl, {
                                        method: 'GET'
                                    }).then(response => {
                                        // 创建一个Blob URL并触发下载
                                        response.blob().then(blob => {
                                            const url = window.URL.createObjectURL(blob);
                                            const a = document.createElement('a');
                                            a.href = url;
                                            a.download = `document_${data.document_id}.md`;
                                            document.body.appendChild(a);
                                            a.click();
                                            document.body.removeChild(a);
                                            window.URL.revokeObjectURL(url);
                                        });
                                    }).catch(error => {
                                        console.error('文档下载失败:', error);
                                        errorMessage.textContent = '文档下载失败，请稍后再试';
                                        errorMessage.classList.remove('hidden');
                                    });
                                    });
                                    
                                    // 创建消息元素
                                    const messageElement = document.createElement('p');
                                    messageElement.textContent = data.message || '文档已生成，点击下载按钮获取';
                                    messageElement.className = 'mt-2';
                                    
                                    // 添加到AI响应容器
                                    aiResponseElement.parentNode.appendChild(messageElement);
                                    aiResponseElement.parentNode.appendChild(downloadButton);
                                    scrollToBottom();
                                }
                            } catch (jsonError) {
                                console.error('解析JSON失败:', jsonError);
                            }
                        }
                    });
                } catch (error) {
                    console.error('处理流数据失败:', error);
                    errorMessage.textContent = '处理响应数据失败';
                    errorMessage.classList.remove('hidden');
                }
                
                // 当流结束时，对完整内容进行markdown解析
                if (done) {
                    aiResponseElement.innerHTML = parseMarkdown(fullResponseContent);
                    scrollToBottom();
                    return;
                }
                
                return readStream();
            });
        }
        
        return readStream();
    })
    .catch(error => {
        console.error('请求错误:', error);
        errorMessage.textContent = '网络错误，请稍后再试';
        errorMessage.classList.remove('hidden');
    });
}

// 辅助函数：HTML转义
function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// 解析markdown内容
function parseMarkdown(text) {
    try {
        // 使用marked.js解析markdown
        const html = marked.parse(text);
        return html;
    } catch (error) {
        console.error('Markdown解析错误:', error);
        // 如果解析失败，返回转义后的纯文本
        return escapeHtml(text);
    }
}

// 滚动到底部
function scrollToBottom() {
    const chatWindow = document.getElementById('chat-container');
    chatWindow.scrollTop = chatWindow.scrollHeight;
}

// 初始化事务预约功能
function initAppointmentFunctions() {
    // 获取元素
    const appointmentLink = document.querySelector('a[href="#dashboard"]');
    const sendPrompt = document.getElementById('send-prompt');
    const promptInput = document.getElementById('prompt-input');
    
    // 点击事务预约链接事件
    if (appointmentLink) {
        appointmentLink.addEventListener('click', function(e) {
            e.preventDefault();
            // 更新侧边栏活动状态
            document.querySelectorAll('aside a').forEach(link => {
                link.classList.remove('sidebar-active');
                link.classList.add('text-gray-600', 'hover:bg-gray-50');
            });
            this.classList.add('sidebar-active');
            this.classList.remove('text-gray-600', 'hover:bg-gray-50');
            
            // 显示事务预约内容
            showDashboardContent();
        });
    }
    
    // 发送按钮点击事件
    if (sendPrompt) {
        sendPrompt.addEventListener('click', function() {
            sendMessage();
        });
    }
    
    // 输入框按键事件
    if (promptInput) {
        promptInput.addEventListener('keydown', function(e) {
            if (e.key === 'Enter' && !e.shiftKey) {
                e.preventDefault();
                sendMessage();
            }
        });
    }
}

// 在DOM加载完成后初始化
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initAppointmentFunctions);
} else {
    initAppointmentFunctions();
}