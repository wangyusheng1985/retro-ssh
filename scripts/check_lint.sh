#!/usr/bin/env bash
set -euo pipefail

# 确保 clang-format 可用
if ! command -v clang-format &> /dev/null; then
    echo "ERROR: clang-format is not installed or not in PATH."
    exit 1
fi

echo "=== Running clang-format check on C++ sources ==="

# 收集所有源码和头文件
FILES=$(find src tests -type f \( -name "*.cpp" -o -name "*.h" \) 2>/dev/null || true)

if [ -z "$FILES" ]; then
    echo "No files found to check."
    exit 0
fi

# 运行干跑格式化检查
FAILED=0
for f in $FILES; do
    if ! clang-format --dry-run --Werror "$f" 2>&1; then
        echo "FAIL: $f does not conform to .clang-format"
        FAILED=1
    fi
done

if [ $FAILED -ne 0 ]; then
    echo "=== Lint check failed! Please run 'clang-format -i <file>' to fix. ==="
    exit 1
fi

echo "=== All files passed clang-format check! ==="
exit 0
