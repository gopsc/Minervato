// 全局变量初始化
window.autoRefreshInterval = null;
window.isAutoRefreshing = false;
window.realtimeChart = null;
window.historyChart = null;
window.historyData = [];
window.dataPoints = 0;
window.currentCommand = '';
window.currentDataFields = [];
window.selectedDataField = '';
window.lastCommand = '';

// uPlot 主题配置
const uPlotTheme = {
    textColor: "#e6e6e6",
    axisColor: "rgba(255, 255, 255, 0.3)",
    gridColor: "rgba(255, 255, 255, 0.1)",
    tickColor: "#a1cae2",
    realtimeColor: "#4cc9f0",
    historyColor: "#f04c4c"
};

// 工具函数
function getAllDataFields() {
    const fields = new Set();
    window.historyData.forEach(entry => {
        Object.keys(entry.data).forEach(field => {
            if (field !== 'msg' && typeof entry.data[field] === 'number') {
                fields.add(field);
            }
        });
    });
    return Array.from(fields);
}

function saveData() {
    const dataToSave = {
        data: window.historyData,
        count: window.dataPoints,
        selectedField: window.selectedDataField,
        currentCommand: window.currentCommand
    };
    localStorage.setItem('sensorHistoryData', JSON.stringify(dataToSave));
}

function updateDataFieldOptions(fields) {
    const select = document.getElementById('dataFieldSelect');
    const currentSelected = select.value;
    
    select.innerHTML = '<option value="">-- 选择数据字段 --</option>';
    document.getElementById('availableFields').textContent = fields.length;
    window.currentDataFields = fields;
    
    fields.forEach(field => {
        const option = document.createElement('option');
        option.value = field;
        option.textContent = field;
        select.appendChild(option);
    });
    
    if (currentSelected && fields.includes(currentSelected)) {
        select.value = currentSelected;
        window.selectedDataField = currentSelected;
    } else if (fields.length > 0 && !window.selectedDataField) {
        window.selectedDataField = fields[0];
        select.value = window.selectedDataField;
    }
}

// 初始化函数
function initData() {
    const savedData = localStorage.getItem('sensorHistoryData');
    if (savedData) {
        try {
            const parsedData = JSON.parse(savedData);
            window.historyData = parsedData.data || [];
            window.dataPoints = parsedData.count || 0;
            window.selectedDataField = parsedData.selectedField || '';
            window.currentCommand = parsedData.currentCommand || '';
            window.lastCommand = window.currentCommand;
            
            // 限制历史数据最多400条
            if (window.historyData.length > 400) {
                window.historyData = window.historyData.slice(-400);
                window.dataPoints = 400;
            }
            
            document.getElementById('dataPoints').textContent = window.dataPoints;
            document.getElementById('currentCommand').textContent = window.currentCommand;
            
            if (window.historyData.length > 0) {
                const allFields = getAllDataFields();
                updateDataFieldOptions(allFields);
                if (window.selectedDataField && allFields.includes(window.selectedDataField)) {
                    document.getElementById('dataFieldSelect').value = window.selectedDataField;
                }
                updateCharts();
            }
            updateStats();
        } catch (e) {
            console.error('加载历史数据失败:', e);
            window.historyData = [];
        }
    }
}

function initRealtimeChart() {
    const container = document.getElementById('realtimeChart');
    if (!container) {
        console.error('找不到实时图表容器');
        return;
    }

    const containerWidth = container.parentElement.offsetWidth;
    
    const opts = {
        width: containerWidth,
        height: 400,
        scales: {
            x: { 
                time: false,
                grid: {
                    stroke: uPlotTheme.gridColor,
                    width: 1
                }
            },
            y: {
                grid: {
                    stroke: uPlotTheme.gridColor,
                    width: 1
                }
            }
        },
        series: [
            {},
            {
                label: "实时数据",
                stroke: uPlotTheme.realtimeColor,
                fill: "rgba(76, 201, 240, 0.1)",
                width: 3,
                points: {
                    show: true,
                    stroke: uPlotTheme.realtimeColor,
                    fill: "#fff",
                    width: 2
                }
            }
        ],
        axes: [
            {
                label: "数据点序号",
                stroke: uPlotTheme.axisColor,
                grid: { 
                    stroke: uPlotTheme.gridColor,
                    width: 1
                },
                ticks: { 
                    stroke: uPlotTheme.tickColor,
                    width: 1
                },
                labelSize: 14,
                labelColor: uPlotTheme.realtimeColor,
                font: "12px 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif",
                gap: 8,
                values: (u, vals) => vals.map(v => Math.round(v))
            },
            {
                label: "数值",
                stroke: uPlotTheme.axisColor,
                grid: { 
                    stroke: uPlotTheme.gridColor,
                    width: 1
                },
                ticks: { 
                    stroke: uPlotTheme.tickColor,
                    width: 1
                },
                labelSize: 14,
                labelColor: uPlotTheme.realtimeColor,
                font: "12px 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif",
                side: 1,
                gap: 8,
                values: (u, vals) => vals.map(v => v == null ? "" : v.toFixed(2))
            }
        ],
        cursor: {
            drag: { x: false, y: false },
            points: {
                size: 8,
                stroke: uPlotTheme.realtimeColor,
                fill: "#fff",
                width: 2
            }
        },
        legend: {
            show: true,
            live: false
        },
        select: {
            show: false
        }
    };
    
    try {
        window.realtimeChart = new uPlot(opts, [[], []], container);
        console.log('实时图表初始化成功');
    } catch (error) {
        console.error('初始化实时图表失败:', error);
    }
}

function initHistoryChart() {
    const container = document.getElementById('historyChart');
    if (!container) {
        console.error('找不到历史图表容器');
        return;
    }

    const containerWidth = container.parentElement.offsetWidth;
    
    const opts = {
        width: containerWidth,
        height: 400,
        scales: {
            x: { 
                time: false, // 不使用时间
                grid: {
                    stroke: uPlotTheme.gridColor,
                    width: 1
                }
            },
            y: {
                grid: {
                    stroke: uPlotTheme.gridColor,
                    width: 1
                }
            }
        },
        series: [
            {},
            {
                label: "历史数据",
                stroke: uPlotTheme.historyColor,
                fill: "rgba(240, 76, 76, 0.1)",
                width: 3,
                points: {
                    show: true,
                    stroke: uPlotTheme.historyColor,
                    fill: "#fff",
                    width: 2
                }
            }
        ],
        axes: [
            {
                label: "数据点序号",
                stroke: uPlotTheme.axisColor,
                grid: { 
                    stroke: uPlotTheme.gridColor,
                    width: 1
                },
                ticks: { 
                    stroke: uPlotTheme.tickColor,
                    width: 1
                },
                labelSize: 14,
                labelColor: uPlotTheme.historyColor,
                font: "12px 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif",
                gap: 8,
                // 只显示整数刻度，从1到20
                values: (u, vals, space) => {
                    const ticks = [];
                    for (let i = 1; i <= 20; i++) {
                        ticks.push(i);
                    }
                    return ticks;
                }
            },
            {
                label: "数值",
                stroke: uPlotTheme.axisColor,
                grid: { 
                    stroke: uPlotTheme.gridColor,
                    width: 1
                },
                ticks: { 
                    stroke: uPlotTheme.tickColor,
                    width: 1
                },
                labelSize: 14,
                labelColor: uPlotTheme.historyColor,
                font: "12px 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif",
                side: 1,
                gap: 8,
                values: (u, vals) => vals.map(v => v == null ? "" : v.toFixed(2))
            }
        ],
        cursor: {
            drag: { x: false, y: false },
            points: {
                size: 8,
                stroke: uPlotTheme.historyColor,
                fill: "#fff",
                width: 2
            }
        },
        legend: {
            show: true,
            live: false
        },
        select: {
            show: false
        }
    };
    
    try {
        window.historyChart = new uPlot(opts, [[], []], container);
        console.log('历史图表初始化成功');
    } catch (error) {
        console.error('初始化历史图表失败:', error);
    }
}

// 核心功能函数
async function fetchSensorData() {
    console.log('fetchSensorData 被调用');
    const command = document.getElementById('sensorCommand').value.trim();
    const statusDiv = document.getElementById('status');
    
    if (!command) {
        statusDiv.innerHTML = '<span class="pulse"></span>错误: 请输入传感器命令';
        statusDiv.className = 'status error';
        return;
    }

    try {
        statusDiv.innerHTML = `<span class="pulse"></span>正在获取 ${command} 数据...`;
        statusDiv.className = 'status';
        
        // API 调用代码
        const response = await fetch('/api/sensor', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ cmd: command })
        });

        if (!response.ok) throw new Error(`HTTP错误! 状态: ${response.status}`);
        const data = await response.json();
        if (data.error) throw new Error(data.error);
        if (data.msg !== "OK") throw new Error('传感器返回错误');

        // 命令变更时清理旧数据
        if (command !== window.lastCommand && window.lastCommand !== '') {
            window.historyData = window.historyData.filter(entry => entry.command === command);
            window.dataPoints = window.historyData.length;
            document.getElementById('dataPoints').textContent = window.dataPoints;
            
            const allFields = getAllDataFields();
            updateDataFieldOptions(allFields);
            if (allFields.length > 0 && !document.getElementById('dataFieldSelect').value) {
                window.selectedDataField = allFields[0];
                document.getElementById('dataFieldSelect').value = window.selectedDataField;
            }
            updateCharts();
            updateStats();
        }

        // 更新UI和历史数据
        updateSensorData(data, command);
        const now = new Date();
        window.historyData.push({ 
            time: now.toISOString(), 
            command: command, 
            data: data 
        });
        
        // 限制历史数据最多400条
        if (window.historyData.length > 400) {
            window.historyData = window.historyData.slice(-400);
        }
        
        // 刷新状态
        window.dataPoints = window.historyData.length;
        document.getElementById('dataPoints').textContent = window.dataPoints;
        document.getElementById('lastUpdate').textContent = now.toLocaleTimeString();
        
        updateDataFieldOptions(getAllDataFields());
        updateCharts();
        updateStats();
        saveData();
        window.lastCommand = command;
        window.currentCommand = command;

        statusDiv.innerHTML = `<span class="pulse"></span>${command} 数据更新成功: ${now.toLocaleTimeString()}`;
        statusDiv.className = 'status connected';
        
        console.log('数据获取成功', data);
    } catch (error) {
        console.error('获取传感器数据失败:', error);
        statusDiv.innerHTML = `<span class="pulse"></span>错误: ${error.message}`;
        statusDiv.className = 'status error';
    }
}

function updateSensorData(data, command) {
    const grid = document.getElementById('sensorDataGrid');
    grid.innerHTML = '';

    const cmdItem = document.createElement('div');
    cmdItem.className = 'data-item';
    cmdItem.innerHTML = `<span class="label">传感器命令</span><span class="value" id="currentCommand">${command}</span>`;
    grid.appendChild(cmdItem);

    Object.keys(data).forEach(key => {
        if (key !== 'msg') {
            const item = document.createElement('div');
            item.className = 'data-item';
            const value = typeof data[key] === 'number' ? data[key].toFixed(2) : data[key];
            item.innerHTML = `<span class="label">${key}</span><span class="value">${value}</span>`;
            grid.appendChild(item);
        }
    });
}

function updateCharts() {
    if (!window.selectedDataField || !window.realtimeChart || !window.historyChart) {
        console.log('图表更新条件不满足');
        return;
    }
    
    // 更新实时图表（最近20个数据点）
    const recentData = [];
    for (let i = window.historyData.length - 1; i >= 0 && recentData.length < 20; i--) {
        const entry = window.historyData[i];
        if (entry.data[window.selectedDataField] !== undefined) {
            recentData.unshift(entry);
        }
    }

    // 实时图表：横坐标固定为从1到20
    const realtimeIndexes = Array.from({length: 20}, (_, i) => i + 1);
    const realtimeValues = Array.from({length: 20}, (_, i) => {
        if (i < recentData.length) {
            return parseFloat(recentData[i].data[window.selectedDataField]);
        }
        return null; // 对于没有数据的位置设为null
    });
    
    // 更新实时图表
    window.realtimeChart.series[1].label = window.selectedDataField;
    window.realtimeChart.setData([realtimeIndexes, realtimeValues]);

    // 更新历史图表 - 计算平均值
    const historyIndexes = [];
    const historyValues = [];
    
    // 获取所有有效数据点
    const validEntries = window.historyData
        .filter(entry => entry.data[window.selectedDataField] !== undefined)
        .map(entry => parseFloat(entry.data[window.selectedDataField]));
    
    // 获取最新的400个数据点
    const recent400 = validEntries.slice(-400);
    
    // 计算20个平均值点
    for (let i = 0; i < 20; i++) {
        const startIndex = i * 20;
        const endIndex = (i + 1) * 20;
        const batch = recent400.slice(startIndex, endIndex);
        
        if (batch.length > 0) {
            const sum = batch.reduce((a, b) => a + b, 0);
            const avg = sum / batch.length;
            
            // 使用从1开始的连续序号作为横坐标
            historyIndexes.push(i + 1);
            historyValues.push(avg);
        }
    }

    // 更新历史图表
    window.historyChart.series[1].label = window.selectedDataField;
    window.historyChart.setData([historyIndexes, historyValues]);
    
    console.log('图表更新完成');
}

function updateStats() {
    const minSpan = document.getElementById('minValue');
    const maxSpan = document.getElementById('maxValue');
    const avgSpan = document.getElementById('avgValue');
    const minLabel = document.getElementById('minLabel');
    const maxLabel = document.getElementById('maxLabel');
    const avgLabel = document.getElementById('avgLabel');

    if (window.historyData.length === 0 || !window.selectedDataField) {
        minSpan.textContent = '--';
        maxSpan.textContent = '--';
        avgSpan.textContent = '--';
        minLabel.textContent = '最小值';
        maxLabel.textContent = '最大值';
        avgLabel.textContent = '平均值';
        return;
    }

    const values = window.historyData
        .filter(entry => entry.data[window.selectedDataField] !== undefined)
        .map(entry => parseFloat(entry.data[window.selectedDataField]));

    if (values.length === 0) {
        minSpan.textContent = '--';
        maxSpan.textContent = '--';
        avgSpan.textContent = '--';
        return;
    }

    const minVal = Math.min(...values).toFixed(2);
    const maxVal = Math.max(...values).toFixed(2);
    const avgVal = (values.reduce((a, b) => a + b, 0) / values.length).toFixed(2);

    minSpan.textContent = minVal;
    maxSpan.textContent = maxVal;
    avgSpan.textContent = avgVal;
    minLabel.textContent = `最小值 (${window.selectedDataField})`;
    maxLabel.textContent = `最大值 (${window.selectedDataField})`;
    avgLabel.textContent = `平均值 (${window.selectedDataField})`;
}

function clearData() {
    if (!confirm('确定要清除所有历史数据吗？此操作不可撤销。')) return;

    window.historyData = [];
    window.dataPoints = 0;
    window.currentDataFields = [];
    window.selectedDataField = '';
    window.currentCommand = '';
    window.lastCommand = '';

    document.getElementById('dataPoints').textContent = '0';
    document.getElementById('availableFields').textContent = '0';
    document.getElementById('currentCommand').textContent = '';
    document.getElementById('dataFieldSelect').innerHTML = '<option value="">-- 选择数据字段 --</option>';
    document.getElementById('status').innerHTML = '<span class="pulse"></span>历史数据已清除';

    if (window.realtimeChart) window.realtimeChart.setData([[], []]);
    if (window.historyChart) window.historyChart.setData([[], []]);

    localStorage.removeItem('sensorHistoryData');
    updateStats();
    console.log('数据已清除');
}

function exportData() {
    if (window.historyData.length === 0) {
        alert('没有数据可导出');
        return;
    }

    const allFields = new Set();
    window.historyData.forEach(entry => {
        Object.keys(entry.data).forEach(field => {
            if (field !== 'msg') allFields.add(field);
        });
    });

    let csv = "时间,命令";
    allFields.forEach(field => csv += `,${field}`);
    csv += "\n";

    window.historyData.forEach(entry => {
        const date = new Date(entry.time).toLocaleString();
        csv += `${date},${entry.command}`;
        
        allFields.forEach(field => {
            const value = entry.data[field];
            if (value === 0 || value === '0') csv += ',0';
            else if (value !== undefined && value !== null) {
                const numValue = parseFloat(value);
                csv += `,${isNaN(numValue) ? value : numValue.toFixed(2)}`;
            } else csv += ',';
        });
        
        csv += "\n";
    });

    const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = `sensor_${new Date().toISOString().slice(0, 10)}.csv`;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    console.log('数据已导出');
}

function toggleAutoRefresh() {
    const btn = document.getElementById('autoRefresh');
    const statusSpan = document.getElementById('autoRefreshStatus');
    const statusDiv = document.getElementById('status');

    if (window.isAutoRefreshing) {
        clearInterval(window.autoRefreshInterval);
        btn.textContent = '自动刷新: 关闭';
        statusSpan.textContent = '关闭';
        statusDiv.innerHTML = '<span class="pulse"></span>已停止自动刷新';
        window.isAutoRefreshing = false;
        console.log('自动刷新已停止');
    } else {
        btn.textContent = '自动刷新: 开启 (30秒)';
        statusSpan.textContent = '开启 (30秒)';
        statusDiv.innerHTML = '<span class="pulse"></span>已开启自动刷新，每30秒获取一次数据';
        window.autoRefreshInterval = setInterval(fetchSensorData, 30000);
        window.isAutoRefreshing = true;
        console.log('自动刷新已开启');
    }
}

async function setCommandOptions() {
    // API 调用代码
    const response = await fetch('/api/show', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' }
    });
    if (!response.ok) throw new Error(`HTTP错误! 状态: ${response.status}`);
    const data = await response.json();
    if(!Array.isArray(data)) {
        alert("获取的类型有误！")
        return;
    }

    
    let str = ""
    // const data = ["命令一", "命令二"]
    data.forEach(ele=>{
        str += `
            <option value="${ele}" style="color: #333;">${ele}</option>
        `
    })
    let dom = document.querySelector("#sensorCommand")
    if(dom) {
        dom.innerHTML = str;
    }
}

// 页面加载完成后初始化
document.addEventListener('DOMContentLoaded', function() {
    console.log('DOM 加载完成，开始初始化...');
    
    // 检查 uPlot 库是否加载
    if (typeof uPlot === 'undefined') {
        console.error('uPlot 库未正确加载');
        document.getElementById('status').innerHTML = '<span class="pulse"></span>错误: 图表库加载失败';
        return;
    }

    // 绑定事件监听
    document.getElementById('fetchData').addEventListener('click', fetchSensorData);
    document.getElementById('autoRefresh').addEventListener('click', toggleAutoRefresh);
    document.getElementById('exportData').addEventListener('click', exportData);
    document.getElementById('clearData').addEventListener('click', clearData);
    document.getElementById('dataFieldSelect').addEventListener('change', function() {
        window.selectedDataField = this.value;
        if (window.selectedDataField) {
            updateCharts();
            updateStats();
            saveData();
        }
    });

    console.log('事件监听器已绑定');

    // 初始化流程
    setCommandOptions();
    initData();
    initRealtimeChart();
    initHistoryChart();
    updateStats();
    
    console.log('初始化完成');
    
    // 窗口大小变化时重新设置图表大小
    window.addEventListener('resize', function() {
        setTimeout(() => {
            if (window.realtimeChart) {
                try {
                    const container = document.getElementById('realtimeChart');
                    window.realtimeChart.setSize({
                        width: container.parentElement.offsetWidth,
                        height: 400
                    });
                } catch (error) {
                    console.error('实时图表重设大小失败:', error);
                }
            }
            if (window.historyChart) {
                try {
                    const container = document.getElementById('historyChart');
                    window.historyChart.setSize({
                        width: container.parentElement.offsetWidth,
                        height: 400
                    });
                } catch (error) {
                    console.error('历史图表重设大小失败:', error);
                }
            }
        }, 100);
    });
});

