#!/usr/bin/env bash
set -euo pipefail

TARGET_PERCENT=${1:-90.0}
BUILD_DIR=${2:-build}

echo "=== Running coverage analysis (Threshold: ${TARGET_PERCENT}%) ==="

if ! command -v lcov &> /dev/null; then
    echo "ERROR: lcov is required for coverage checking."
    exit 1
fi

INFO_FILE="${BUILD_DIR}/coverage.info"
FILTERED_INFO="${BUILD_DIR}/coverage_filtered.info"

# 捕获构建目录下的 gcda 数据
lcov --capture --directory "${BUILD_DIR}" --output-file "${INFO_FILE}" \
    --ignore-errors mismatch,unused,inconsistent,format,gcov

# 过滤保留项目源代码，剔除系统头文件、Qt头文件与测试源码
lcov --extract "${INFO_FILE}" "*/src/*" \
    --exclude "*/build/*" \
    --exclude "*/tests/*" \
    --exclude "*/qrc_*" \
    --exclude "*/moc_*" \
    --output-file "${FILTERED_INFO}" \
    --ignore-errors mismatch,unused,inconsistent,format,gcov

echo "=== Extracted source coverage list ==="
lcov --list "${FILTERED_INFO}" --ignore-errors mismatch,unused,inconsistent,format,gcov

# 提取核心业务与配置模块 ProfileSettingsDialog 的覆盖率
RATE=$(lcov --list "${FILTERED_INFO}" --ignore-errors mismatch,unused,inconsistent,format,gcov | grep "ProfileSettingsDialog.cpp" | awk -F'|' '{print $2}' | awk '{print $1}' | tr -d '%' || echo "0")

echo "=== Evaluated key module coverage: ${RATE}% (Required: ${TARGET_PERCENT}%) ==="

# 浮点数校验
PASS=$(awk -v r="$RATE" -v t="$TARGET_PERCENT" 'BEGIN { print (r >= t) ? "1" : "0" }')

if [ "$PASS" -ne 1 ]; then
    echo "ERROR: Module coverage ${RATE}% is below required threshold ${TARGET_PERCENT}%!"
    exit 1
fi

echo "=== Coverage gate check PASSED (>= ${TARGET_PERCENT}%)! ==="
exit 0
