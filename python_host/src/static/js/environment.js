// 更新传感器类型多选标签
function updateSensorTypeDropdown(uniqueSensorTypes) {
    const sensorTypeTagsContainer = document.getElementById('sensor-type-tags');
    if (!sensorTypeTagsContainer) return;

    // 获取当前选中的传感器类型（用于页面刷新或切换时间范围时保持选择）
    const selectedTypes = Array.from(
        document.querySelectorAll('.sensor-checkbox:checked')
    ).map(cb => cb.value);

    // 清除现有标签
    sensorTypeTagsContainer.innerHTML = '';

    // 添加新的传感器类型标签 - 默认不选中任何标签
    uniqueSensorTypes.forEach(type => {
        // 只有在有实际选中的类型时才设置选中状态，否则默认不选中
        const isSelected = selectedTypes.length > 0 && selectedTypes.includes(type);
        
        const tag = document.createElement('label');
        tag.className = `sensor-tag inline-flex items-center px-3 py-1 rounded-full ${isSelected ? 'bg-primary text-white' : 'bg-gray-200 text-gray-700'} text-xs cursor-pointer`;
        tag.innerHTML = `
            <input type="checkbox" class="sensor-checkbox sr-only" value="${type}" ${isSelected ? 'checked' : ''}>
            <span>${type}</span>
        `;
        sensorTypeTagsContainer.appendChild(tag);
    });

    // 重新添加事件监听器
    addSensorTagListeners();
}

// 添加传感器标签的事件监听器
function addSensorTagListeners() {
    const allCheckboxes = document.querySelectorAll('.sensor-checkbox');

    // 单个传感器复选框的点击事件
    allCheckboxes.forEach(checkbox => {
        checkbox.addEventListener('change', function() {
            // 如果当前复选框被选中，则取消所有其他复选框的选中状态
            if (this.checked) {
                allCheckboxes.forEach(cb => {
                    if (cb !== this) {
                        cb.checked = false;
                        updateTagStyles(cb);
                    }
                });
            }
            // 更新当前标签样式
            updateTagStyles(this);
            // 当传感器选择发生变化时，重新加载数据以更新图表
            loadSensorData();
        });
        
        // 点击标签时也应该只选中单个传感器
        const tag = checkbox.closest('.sensor-tag');
        if (tag) {
            tag.addEventListener('click', function(e) {
                // 如果点击的是复选框本身，则让默认行为处理
                if (e.target.type === 'checkbox') return;
                
                // 阻止事件冒泡，避免触发多次
                e.stopPropagation();
                
                // 如果当前标签未选中，则选中它并取消其他标签
                if (!checkbox.checked) {
                    allCheckboxes.forEach(cb => {
                        cb.checked = (cb === checkbox);
                        updateTagStyles(cb);
                    });
                    loadSensorData();
                }
            });
        }
    });
}

// 更新标签样式
function updateTagStyles(checkbox) {
    const tag = checkbox.closest('.sensor-tag');
    if (checkbox.checked) {
        tag.classList.add('bg-primary', 'text-white');
        tag.classList.remove('bg-gray-200', 'text-gray-700');
    } else {
        tag.classList.remove('bg-primary', 'text-white');
        tag.classList.add('bg-gray-200', 'text-gray-700');
    }
}

// 获取选中的传感器类型
function getSelectedSensorTypes() {
    // 返回选中的传感器类型数组
    return Array.from(
        document.querySelectorAll('.sensor-checkbox:checked')
    ).map(cb => cb.value);
}

// 环境监测功能 - 数据加载函数
function loadSensorData() {
    // 获取筛选参数
    const deviceSelectElement = document.getElementById('device-select');
    const timeRangeSelectElement = document.getElementById('time-range-select');
    
    // 检查元素是否存在
    if (!timeRangeSelectElement) {
        console.error('缺少必要的选择器元素');
        return;
    }
    
    const deviceId = deviceSelectElement ? deviceSelectElement.value : '';
    const selectedSensorTypes = getSelectedSensorTypes();
    const timeRange = timeRangeSelectElement.value;
    
    // 显示加载状态
    const loadingIndicator = document.getElementById('loading-indicator-data');
    if (loadingIndicator) {
        loadingIndicator.classList.remove('hidden');
    }
    
    // 构建API请求参数
    const params = new URLSearchParams();
    if (deviceId) params.append('device_id', deviceId);
    
    // 根据时间范围计算开始时间
    const now = new Date();
    let startTime;
    
    switch (timeRange) {
        case '1h':
            startTime = new Date(now.getTime() - 1 * 60 * 60 * 1000);
            break;
        case '24h':
            startTime = new Date(now.getTime() - 24 * 60 * 60 * 1000);
            break;
        case '7d':
            startTime = new Date(now.getTime() - 7 * 24 * 60 * 60 * 1000);
            break;
        case '30d':
            startTime = new Date(now.getTime() - 30 * 24 * 60 * 60 * 1000);
            break;
        default:
            startTime = new Date(now.getTime() - 24 * 60 * 60 * 1000);
    }
    
    params.append('start_time', startTime.toISOString());
    params.append('end_time', now.toISOString());
    params.append('limit', 1000); // 设置合理的限制值
    
    // 添加聚合参数，启用数据聚合功能
    params.append('aggregate', 'true');
    
    // 发送API请求，使用全局的apiRequest函数（支持自动刷新令牌）
    window.apiRequest(`/api/sensor-data?${params.toString()}`, {
        method: 'GET'
    })
    .then(response => {
        if (!response.ok) {
            throw new Error('API请求失败');
        }
        return response.json();
    })
    .then(data => {
        // 隐藏加载状态
        if (loadingIndicator) {
            loadingIndicator.classList.add('hidden');
        }
        
        // 提取所有唯一的传感器类型
        const uniqueSensorTypes = [...new Set(data.map(item => item.sensor_type).filter(type => type))];
        
        // 更新传感器类型下拉列表前，保存当前选中的传感器类型
        const previouslySelectedTypes = [...selectedSensorTypes];
        
        // 更新传感器类型下拉列表
        updateSensorTypeDropdown(uniqueSensorTypes);
        
        // 更新后，重新选中之前选择的传感器类型
        if (previouslySelectedTypes.length > 0) {
            const newCheckboxes = document.querySelectorAll('.sensor-checkbox');
            newCheckboxes.forEach(cb => {
                if (previouslySelectedTypes.includes(cb.value)) {
                    cb.checked = true;
                    updateTagStyles(cb);
                }
            });
        }
        
        // 重新获取选中的传感器类型（因为DOM元素已更新）
        const updatedSelectedTypes = getSelectedSensorTypes();
        
        // 如果用户选择了特定的传感器类型，则过滤数据
        let filteredData = [];
        if (updatedSelectedTypes.length > 0) {
            filteredData = data.filter(item => updatedSelectedTypes.includes(item.sensor_type));
        } else {
            // 当没有选择任何传感器类型时，不显示图表数据
            filteredData = [];
        }
        
        // 优化性能：限制图表数据点数量
        // 聚合数据已经是优化后的数据点，不再需要前端降采样处理
        // 仅保留少量数据点的显示限制，确保图表性能良好
        if (filteredData.length > 300) {
            // 对于24小时的数据，使用更激进的采样，减少数据点数量
            const maxPoints = timeRange === '24h' ? 100 : 300;
            const sampleRate = Math.ceil(filteredData.length / maxPoints);
            filteredData = filteredData.filter((_, index) => index % sampleRate === 0);
        }
        
        // 更新实时数据显示
        updateRealTimeValues(filteredData);
        
        // 更新图表数据
        updateCharts(filteredData);
        
        // 隐藏错误消息
        const errorMessageElement = document.getElementById('data-error-message');
        if (errorMessageElement) {
            errorMessageElement.classList.add('hidden');
        }
    })
    .catch(error => {
        console.error('获取传感器数据失败:', error);
        
        // 隐藏加载状态
        if (loadingIndicator) {
            loadingIndicator.classList.add('hidden');
        }
        
        // 显示错误消息
        const errorMessageElement = document.getElementById('data-error-message');
        if (errorMessageElement) {
            const spanElement = errorMessageElement.querySelector('span');
            if (spanElement) {
                spanElement.textContent = '获取传感器数据失败，请稍后重试';
            } else {
                errorMessageElement.textContent = '获取传感器数据失败，请稍后重试';
            }
            errorMessageElement.classList.remove('hidden');
            
            // 3秒后隐藏错误消息
            setTimeout(() => {
                errorMessageElement.classList.add('hidden');
            }, 3000);
        }
    });
}

// 更新实时数据卡片显示
function updateRealTimeValues(data) {
    const container = document.getElementById('real-time-data');
    if (!container) return;
    
    if (!data || data.length === 0) {
        // 如果没有数据，显示默认消息
        container.innerHTML = `
            <div class="p-8 flex-shrink-0">
                <p class="text-gray-500">暂无传感器数据</p>
            </div>
        `;
        return;
    }
    
    // 预处理数据，确保时间戳格式正确
    const processedData = data.map(item => {
        const processedItem = {...item};
        // 确保时间戳字段为字符串格式
        if (processedItem.aggregated_time && typeof processedItem.aggregated_time !== 'string') {
            processedItem.aggregated_time = String(processedItem.aggregated_time);
        }
        if (processedItem.timestamp && typeof processedItem.timestamp !== 'string') {
            processedItem.timestamp = String(processedItem.timestamp);
        }
        return processedItem;
    });
    
    // 按传感器类型分组数据
    const sensorDataByType = {};
    processedData.forEach(item => {
        if (!sensorDataByType[item.sensor_type]) {
            sensorDataByType[item.sensor_type] = [];
        }
        sensorDataByType[item.sensor_type].push(item);
    });
    
    // 清空容器
    container.innerHTML = '';
    
    // 为每种传感器类型创建卡片
    Object.keys(sensorDataByType).forEach(sensorType => {
        try {
            const sensorData = sensorDataByType[sensorType];
            
            // 确定时间字段和值字段（适配聚合数据格式）
            const timestampField = sensorData[0] && sensorData[0].aggregated_time ? 'aggregated_time' : 'timestamp';
            const valueField = sensorData[0] && sensorData[0].avg_value ? 'avg_value' : 'value';
            
            // 按时间戳排序，获取最新数据
            sensorData.sort((a, b) => {
                try {
                    const timeA = new Date(b[timestampField]);
                    const timeB = new Date(a[timestampField]);
                    return isNaN(timeA.getTime()) || isNaN(timeB.getTime()) ? 0 : timeA - timeB;
                } catch (e) {
                    return 0;
                }
            });
            
            const latestItem = sensorData[0];
            if (!latestItem) return;
            
            // 安全地解析数值
            const latestValue = parseFloat(latestItem[valueField]);
            if (isNaN(latestValue)) return;
            
            // 获取单位
            const unit = getSensorUnit(sensorType, latestValue);
            
            // 安全地格式化时间
            let formattedTime = '未知时间';
            try {
                const time = new Date(latestItem[timestampField]);
                if (!isNaN(time.getTime())) {
                    formattedTime = time.toLocaleTimeString();
                }
            } catch (e) {
                console.error('时间格式化错误:', e);
            }
            
            // 创建卡片元素 - 使用flex-shrink-0确保卡片不会被压缩
            const card = document.createElement('div');
            card.className = 'flex-shrink-0 w-36 p-3 bg-white rounded shadow-sm hover:shadow transition-shadow';
            card.innerHTML = `
                <p class="text-xs text-gray-500 mb-1 truncate">${sensorType}</p>
                <p class="text-xl font-semibold text-gray-800 mb-1">${unit}</p>
                <p class="text-xs text-gray-400">${formattedTime}</p>
            `;
            
            container.appendChild(card);
        } catch (e) {
            console.error('处理传感器数据时出错:', e);
        }
    });
}

// 辅助函数：获取传感器数据的单位和格式化的值
function getSensorUnit(sensorType, value) {
    // 根据传感器类型返回适当的单位和格式化的值
    switch (sensorType.toLowerCase()) {
        case 'temperature':
        case 'temp':
        case '温度':
            return `${value.toFixed(1)}°C`;
        case 'humidity':
        case 'hum':
        case '湿度':
            return `${value.toFixed(1)}%`;
        case 'pressure':
        case '气压':
            return `${value.toFixed(0)} hPa`;
        case 'light':
        case '光照':
            return `${value.toFixed(0)} lux`;
        case 'co2':
        case '二氧化碳':
            return `${value.toFixed(0)} ppm`;
        case 'pm2.5':
        case 'pm10':
            return `${value.toFixed(0)} μg/m³`;
        default:
            // 对于未知传感器类型，根据值的范围选择合适的小数位数
            if (value < 1) return value.toFixed(3);
            if (value < 10) return value.toFixed(2);
            if (value < 100) return value.toFixed(1);
            return value.toFixed(0);
    }
}

// 图表对象缓存
let combinedChart;

// 初始化图表
function initializeCharts() {
    // 初始化多参数对比图表
    const combinedCtx = document.getElementById('combined-chart').getContext('2d');
    combinedChart = new Chart(combinedCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [] // 初始为空，将根据选中的传感器动态添加
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            interaction: {
                mode: 'index',
                intersect: false,
            },
            plugins: {
                title: {
                    display: false
                },
                legend: {
                    position: 'top',
                    labels: {
                        boxWidth: 12,
                        usePointStyle: true,
                        pointStyle: 'circle',
                        padding: 15,
                        // 当有多个数据集时，使用更紧凑的图例布局
                        boxWidth: function(context) {
                            const datasetCount = context.chart.data.datasets.length;
                            return datasetCount > 3 ? 8 : 12;
                        }
                    }
                },
                tooltip: {
                    callbacks: {
                        label: function(context) {
                            let label = context.dataset.label || '';
                            if (label) {
                                label += ': ';
                            }
                            if (context.parsed.y !== null) {
                                // 根据传感器类型确定显示的小数位数
                                const sensorType = context.dataset.label || '';
                                let decimalPlaces = 2;
                                
                                if (sensorType.includes('气压') || sensorType.includes('pressure') || 
                                    sensorType.includes('ppm') || sensorType.includes('μg/m³') || 
                                    sensorType.includes('lux')) {
                                    decimalPlaces = 0; // 整数值传感器
                                } else if (sensorType.includes('温度') || sensorType.includes('温度') || 
                                           sensorType.includes('湿度') || sensorType.includes('humidity')) {
                                    decimalPlaces = 1; // 一位小数传感器
                                }
                                
                                label += context.parsed.y.toFixed(decimalPlaces);
                            }
                            return label;
                        }
                    }
                }
            },
            scales: {
                x: {
                    display: true,
                    grid: {
                        display: false
                    },
                    ticks: {
                        maxRotation: 45,
                        minRotation: 45,
                        autoSkip: true,
                        maxTicksLimit: 6
                    }
                },
                y: {
                    type: 'linear',
                    display: true,
                    position: 'left',
                    grid: {
                        color: 'rgba(0, 0, 0, 0.05)'
                    },
                    title: {
                        display: true,
                        text: '值'
                    }
                },
                // 移除右侧Y轴，简化图表
                y1: {
                    display: false
                }
            }
        }
    });
    
    // 添加刷新按钮事件监听
    document.getElementById('refresh-data').addEventListener('click', loadSensorData);
}

// 获取图表通用配置选项
function getChartOptions(title) {
    return {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
            title: {
                display: true,
                text: title,
                font: {
                    size: 16
                }
            },
            legend: {
                position: 'top',
            },
            tooltip: {
                callbacks: {
                    label: function(context) {
                        let label = context.dataset.label || '';
                        if (label) {
                            label += ': ';
                        }
                        if (context.parsed.y !== null) {
                            label += context.parsed.y.toFixed(1);
                        }
                        return label;
                    }
                }
            }
        },
        scales: {
            x: {
                display: true,
                title: {
                    display: true,
                    text: '时间'
                },
                ticks: {
                    maxRotation: 45,
                    minRotation: 45
                }
            },
            y: {
                display: true,
                title: {
                    display: true,
                    text: ''
                }
            }
        }
    };
}

// 更新图表数据
function updateCharts(data) {
    if (!data || data.length === 0) {
        // 如果没有数据，清空图表
        combinedChart.data.labels = [];
        combinedChart.data.datasets = [];
        combinedChart.update();
        return;
    }
    
    // 按时间戳排序数据
    // 先确保所有时间戳都是字符串格式，避免类型转换错误
    data.forEach(item => {
        if (item.aggregated_time && typeof item.aggregated_time !== 'string') {
            item.aggregated_time = String(item.aggregated_time);
        }
        if (item.timestamp && typeof item.timestamp !== 'string') {
            item.timestamp = String(item.timestamp);
        }
    });
    
    // 按时间戳排序数据
    data.sort((a, b) => {
        const timeA = new Date(a.aggregated_time || a.timestamp);
        const timeB = new Date(b.aggregated_time || b.timestamp);
        return timeA - timeB;
    });
    
    // 按传感器类型分组数据，适配聚合数据格式
    const sensorDataByType = {};
    
    // 对于聚合数据，使用aggregated_time作为时间戳
    const timestampField = data[0] && data[0].aggregated_time ? 'aggregated_time' : 'timestamp';
    
    const allTimestamps = [...new Set(data.map(item => item[timestampField]))];
    allTimestamps.sort((a, b) => new Date(a) - new Date(b));
    
    data.forEach(item => {
        if (!sensorDataByType[item.sensor_type]) {
            sensorDataByType[item.sensor_type] = {};
        }
        // 存储整个数据项，以便后续可以访问avg_value或其他聚合字段
        sensorDataByType[item.sensor_type][item[timestampField]] = item;
    });
    
    // 准备标签（时间）
    const labels = allTimestamps.map(timestamp => {
        try {
            // 确保时间戳是有效的
            const date = new Date(timestamp);
            if (isNaN(date.getTime())) {
                return '无效时间';
            }
            
            // 根据数据量决定显示格式
            const timeRange = allTimestamps.length;
            if (timeRange > 100) {
                // 数据量大时，只显示小时和分钟
                return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
            } else if (timeRange > 24) {
                // 中等数据量，显示日期和时间
                return date.toLocaleDateString() + ' ' + date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
            } else {
                // 数据量小时，显示完整时间
                return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
            }
        } catch (e) {
            console.error('时间格式化错误:', e);
            return timestamp;
        }
    });
    
    // 准备数据集
    const datasets = [];
    let colorIndex = 0;
    const colors = [
        { border: 'rgb(255, 99, 132)', bg: 'rgba(255, 99, 132, 0.1)' },
        { border: 'rgb(54, 162, 235)', bg: 'rgba(54, 162, 235, 0.1)' },
        { border: 'rgb(255, 205, 86)', bg: 'rgba(255, 205, 86, 0.1)' },
        { border: 'rgb(75, 192, 192)', bg: 'rgba(75, 192, 192, 0.1)' },
        { border: 'rgb(153, 102, 255)', bg: 'rgba(153, 102, 255, 0.1)' },
        { border: 'rgb(255, 159, 64)', bg: 'rgba(255, 159, 64, 0.1)' },
        { border: 'rgb(0, 0, 255)', bg: 'rgba(0, 0, 255, 0.1)' },
        { border: 'rgb(0, 128, 0)', bg: 'rgba(0, 128, 0, 0.1)' },
        { border: 'rgb(128, 0, 0)', bg: 'rgba(128, 0, 0, 0.1)' },
        { border: 'rgb(128, 128, 0)', bg: 'rgba(128, 128, 0, 0.1)' }
    ];
    
    // 获取选中的传感器类型
    const selectedTypes = getSelectedSensorTypes();
    
    // 根据选中的传感器类型或所有传感器类型创建数据集
    let sensorTypesToDisplay = selectedTypes.length > 0 ? 
        selectedTypes.filter(type => sensorDataByType[type]) : 
        Object.keys(sensorDataByType);
    
    // 为每种传感器类型创建数据集
    sensorTypesToDisplay.forEach((sensorType, index) => {
        const color = colors[colorIndex % colors.length];
        colorIndex++;
        
        // 准备数据点，处理聚合数据
        const dataPoints = allTimestamps.map(timestamp => {
            try {
                // 查找匹配的时间点数据
                const dataPoint = sensorDataByType[sensorType][timestamp];
                if (!dataPoint) return null;
                
                // 确保数据值是数字类型
                let value;
                if (dataPoint.avg_value !== undefined) {
                    value = parseFloat(dataPoint.avg_value);
                } else if (dataPoint.value !== undefined) {
                    value = parseFloat(dataPoint.value);
                } else {
                    return null;
                }
                
                // 检查是否为有效数字
                return isNaN(value) ? null : value;
            } catch (e) {
                console.error('数据点处理错误:', e);
                return null;
            }
        });
        
        // 创建数据集
        datasets.push({
            label: sensorType,
            data: dataPoints,
            borderColor: color.border,
            backgroundColor: color.bg,
            tension: 0.4,
            // 对于多个传感器，使用左侧Y轴，避免右侧Y轴过于拥挤
            yAxisID: 'y'
        });
    });
    
    // 更新图表
    combinedChart.data.labels = labels;
    combinedChart.data.datasets = datasets;
    
    // 更新图表配置以适应多个数据集
    combinedChart.options.plugins.legend.display = sensorTypesToDisplay.length > 1;
    
    // 对于多个传感器，调整X轴标签的最大数量，避免重叠
    if (sensorTypesToDisplay.length > 3) {
        combinedChart.options.scales.x.ticks.maxTicksLimit = 5;
    } else {
        combinedChart.options.scales.x.ticks.maxTicksLimit = 8;
    }
    
    combinedChart.update();
}

// 显示环境监测内容的函数
function showEnvironmentContent() {
    document.getElementById('initial-content').style.display = 'none';
    document.getElementById('dashboard-content').style.display = 'none';
    document.getElementById('environment-content').style.display = 'block';
    document.getElementById('notes-content').style.display = 'none';
    
    // 初始化图表
    if (!combinedChart) {
        initializeCharts();
    }
    
    // 加载传感器数据
    loadSensorData();
}

// 设置页面初始加载时显示初始内容
window.addEventListener('DOMContentLoaded', function() {
    // 检查URL hash以确定初始显示内容
    if (window.location.hash === '#environment') {
        // 更新侧边栏活动状态
        const environmentLink = document.querySelector('a[href="#environment"]');
        document.querySelectorAll('aside a').forEach(link => {
            link.classList.remove('sidebar-active');
            link.classList.add('text-gray-600', 'hover:bg-gray-50');
        });
        environmentLink.classList.add('sidebar-active');
        environmentLink.classList.remove('text-gray-600', 'hover:bg-gray-50');
        
        // 调用函数显示环境监测内容
        showEnvironmentContent();
    }
});