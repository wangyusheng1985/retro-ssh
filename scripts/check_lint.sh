#!/usr/bin/env bash
set -euo pipefail

# 确保 clang-format 可用
if ! command -v clang-format &> /dev/null; then
    echo "ERROR: clang-format is not installed or not in PATH."
    exit 1
fi

echo "=== Running clang-format check on C++ sources (clang-format $(clang-format --version)) ==="

FILES=$(find src tests -type f \( -name "*.cpp" -o -name "*.h" \) 2>/dev/null || true)

if [ -z "$FILES" ]; then
    echo "No files found to check."
    exit 0
fi

FAILED=0
for f in $FILES; do
    DIFF=$(clang-format "$f" | diff -u "$f" - || true)
    if [ -n "$DIFF" ]; then
        echo "FAIL: $f does not conform to .clang-format:"
        echo "$DIFF"
        FAILED=1
    fi
done

if [ $FAILED -ne 0 ]; then
    echo "=== Lint check failed! Please run 'clang-format -i <file>' to fix. ==="
    exit 1
fi

echo "=== All files passed clang-format check! ==="
exit 0
