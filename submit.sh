#!/bin/bash

HW="homework9"

# Get platform argument, default to linux
PLATFORM="${1:-linux}"

# Handle shorthand arguments
case "$PLATFORM" in
    l) PLATFORM="linux" ;;
    o) PLATFORM="osx" ;;
    w) PLATFORM="windows" ;;
esac

# Validate platform
if [[ "$PLATFORM" != "linux" && "$PLATFORM" != "osx" && "$PLATFORM" != "windows" ]]; then
    echo "❌ Invalid platform: $PLATFORM"
    echo "Usage: ./submit.sh [linux|osx|windows|l|o|w]"
    echo "Defaulting to linux..."
    PLATFORM="linux"
fi

echo "🚀 Submitting $HW for platform: $PLATFORM"

# Bump submission counter in README to guarantee a diff
COUNTER_FILE="README.md"
if grep -q "^Submission #" "$COUNTER_FILE"; then
    CURRENT=$(grep -o '^Submission #[0-9]*' "$COUNTER_FILE" | grep -o '[0-9]*')
    NEXT=$((CURRENT + 1))
    sed -i '' "s/^Submission #${CURRENT}/Submission #${NEXT}/" "$COUNTER_FILE"
else
    echo "" >> "$COUNTER_FILE"
    echo "Submission #1" >> "$COUNTER_FILE"
fi
echo "📝 Marked Submission #${NEXT:-1} in README.md"

echo "📦 Adding files to git..."
git add .

echo "💾 Committing changes..."
git commit -m "please grade $HW $PLATFORM"

echo "📤 Pushing to remote..."
git push

echo "✅ Submission complete!"