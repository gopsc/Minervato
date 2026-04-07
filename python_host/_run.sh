#!/bin/bash
cd "$(dirname "$0")" || exit

# 读取DeepSeek API密钥并设置为环境变量
if [ -f "../../cert/deepseek.key" ]; then
    export DEEPSEEK_API_KEY=$(cat "../../cert/deepseek.key")
    echo "DeepSeek API key loaded from ../../cert/deepseek.key"
else
    echo "Warning: ../../cert/deepseek.key file not found"
fi

#bash ./_setup.sh
source ./.env/bin/activate
exec python3 src/main.py
