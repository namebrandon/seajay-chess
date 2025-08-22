#!/bin/bash

# SeaJay Git Aliases Setup Script
# This script sets up all the Git aliases for organized branch management
# as specified in project_docs/Git_Strategy_for_SeaJay.txt

echo "Setting up SeaJay Git aliases..."
echo "================================"

# Branch creation aliases
echo "Installing branch creation aliases..."

git config --global alias.feature '!f() { git checkout -b feature/$(date +%Y%m%d)-$1 && git push -u origin feature/$(date +%Y%m%d)-$1; }; f'
git config --global alias.bugfix '!f() { git checkout -b bugfix/$(date +%Y%m%d)-$1 && git push -u origin bugfix/$(date +%Y%m%d)-$1; }; f'
git config --global alias.test '!f() { git checkout -b test/$(date +%Y%m%d)-$1 && git push -u origin test/$(date +%Y%m%d)-$1; }; f'
git config --global alias.tune '!f() { git checkout -b tune/$(date +%Y%m%d)-$1 && git push -u origin tune/$(date +%Y%m%d)-$1; }; f'
git config --global alias.ob '!f() { git checkout -b ob/$(date +%Y%m%d)-$1 && git push -u origin ob/$(date +%Y%m%d)-$1; }; f'

echo "✅ Branch creation aliases installed"

# Branch listing aliases
echo "Installing branch listing aliases..."

git config --global alias.list-features '!git branch -r | grep "origin/feature/"'
git config --global alias.list-bugfix '!git branch -r | grep "origin/bugfix/"'
git config --global alias.list-tests '!git branch -r | grep "origin/test/"'
git config --global alias.list-tune '!git branch -r | grep "origin/tune/"'
git config --global alias.list-ob '!git branch -r | grep "origin/ob/"'
git config --global alias.list-all-branches '!git branch -r | grep -E "origin/(feature|bugfix|test|tune|ob)/" | sort'

echo "✅ Branch listing aliases installed"

# Branch cleanup aliases (WARNING: These delete remote branches!)
echo "Installing branch cleanup aliases (use with caution!)..."

git config --global alias.clean-tests '!git branch -r | grep "origin/test/" | sed "s/origin\///" | xargs -r -n 1 git push origin --delete'
git config --global alias.clean-bugfix '!git branch -r | grep "origin/bugfix/" | sed "s/origin\///" | xargs -r -n 1 git push origin --delete'
git config --global alias.clean-features '!git branch -r | grep "origin/feature/" | sed "s/origin\///" | xargs -r -n 1 git push origin --delete'
git config --global alias.clean-tune '!git branch -r | grep "origin/tune/" | sed "s/origin\///" | xargs -r -n 1 git push origin --delete'

echo "✅ Branch cleanup aliases installed"

# Utility aliases
echo "Installing utility aliases..."

git config --global alias.show-branch-age '!git for-each-ref --format="%(refname:short) %(committerdate:relative)" refs/remotes/origin/ | grep -E "(feature|bugfix|test|tune|ob)/" | column -t'
git config --global alias.clean-old-tests '!git for-each-ref --format="%(refname:short) %(committerdate:unix)" refs/remotes/origin/test/ | while read branch date; do if [ $(($(date +%s) - date)) -gt 604800 ]; then echo "Deleting $branch"; git push origin --delete ${branch#origin/}; fi; done'

echo "✅ Utility aliases installed"

echo ""
echo "================================"
echo "All Git aliases have been installed successfully!"
echo ""
echo "Verification:"
echo "-------------"
echo "Checking installed aliases..."
echo ""

# Show the installed aliases
git config --global --list | grep alias | head -10
echo "... (showing first 10 aliases)"

echo ""
echo "Total aliases installed: $(git config --global --list | grep -c alias)"

echo ""
echo "Quick Reference:"
echo "================"
echo ""
echo "BRANCH CREATION:"
echo "  git feature <name>  - Create feature/YYYYMMDD-name branch"
echo "  git bugfix <name>   - Create bugfix/YYYYMMDD-name branch"
echo "  git test <name>     - Create test/YYYYMMDD-name branch"
echo "  git tune <name>     - Create tune/YYYYMMDD-name branch"
echo "  git ob <name>       - Create ob/YYYYMMDD-name branch"
echo ""
echo "BRANCH LISTING:"
echo "  git list-all-branches - Show all organized branches"
echo "  git list-features     - Show feature branches"
echo "  git list-bugfix       - Show bugfix branches"
echo "  git list-tests        - Show test branches"
echo "  git list-tune         - Show tuning branches"
echo "  git list-ob           - Show historical references"
echo "  git show-branch-age   - Show branch ages"
echo ""
echo "BRANCH CLEANUP (USE WITH CAUTION!):"
echo "  git clean-tests       - Delete ALL test branches"
echo "  git clean-old-tests   - Delete test branches older than 7 days"
echo "  git clean-bugfix      - Delete ALL bugfix branches"
echo "  git clean-features    - Delete ALL feature branches"
echo "  git clean-tune        - Delete ALL tune branches"
echo ""
echo "Example usage:"
echo "  git feature null-move-pruning"
echo "  Creates: feature/$(date +%Y%m%d)-null-move-pruning"
echo ""
echo "For more information, see: /workspace/project_docs/Git_Strategy_for_SeaJay.txt"