#!/bin/bash

# YOLO - Quick build and test script
# "You Only Live Once" - for when you want to quickly test changes

echo "🚀 YOLO Mode Activated!"
echo "========================"

# Quick clean and rebuild
echo "🔨 Building SeaJay..."
cd /workspace/build
make clean > /dev/null 2>&1
cmake .. > /dev/null 2>&1
make -j$(nproc) 2>&1 | grep -E "(error|warning|\[.*%\]|Built target)"

if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    
    # Quick smoke test
    echo ""
    echo "🧪 Running quick test..."
    echo -e "position startpos\ngo depth 5\nquit" | ./seajay | grep -E "(bestmove|info depth 5)"
    
    if [ $? -eq 0 ]; then
        echo "✅ Engine responding correctly!"
    else
        echo "❌ Engine test failed!"
        exit 1
    fi
else
    echo "❌ Build failed!"
    exit 1
fi

echo ""
echo "🎉 YOLO complete! Ship it!"