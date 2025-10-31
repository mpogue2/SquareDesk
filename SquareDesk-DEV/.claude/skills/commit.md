# Commit Skill

This skill handles git commits following the project's specific workflow and standards.

## When to Use This Skill

Use this skill when the user asks to:
- "Commit these changes"
- "Make a commit"
- "Commit this"
- "Create a commit for issue #XXXX"

## Repository Detection

1. **Main Repository**: `/Users/mpogue/clean3/SquareDesk`
   - If the current working directory is anywhere under `/Users/mpogue/clean3/SquareDesk` (including subdirectories like `SquareDesk-DEV`), use `/Users/mpogue/clean3/SquareDesk` as the repository root

2. **Other Repositories**:
   - For any other location, use the current working directory as the repository root

## Workflow Steps

### 1. Determine Repository Root

Check if current directory is under `/Users/mpogue/clean3/SquareDesk`:
- If yes → Repository is `/Users/mpogue/clean3/SquareDesk`
- If no → Repository is current working directory

### 2. Get Issue Number

- If user provided an issue number (e.g., "#1151"), use it
- If no issue number provided, ask: "What is the GitHub issue number for this commit?"

### 3. Propose Commit Description

Analyze the changes made and propose a commit DESCRIPTION (summary):
- Propose: "I propose this commit description: [DESCRIPTION]"
- Wait for user response:
  - If approved → Use the proposed description
  - If user provides alternative → Use their description

### 4. Propose Commit Message

Analyze the changes made and propose a detailed commit MESSAGE:
- Propose: "I propose this commit message: [DETAILED_MESSAGE]"
- Wait for user response:
  - If approved → Use the proposed message
  - If user provides alternative → Use their message

### 5. Check for Modified Files

Run `git status` in the repository root to see all modified files.

Identify two categories:
1. **Session files**: Files that were modified during this session (files you edited using Write, Edit, or NotebookEdit tools)
2. **Other files**: Files modified outside this session

### 6. Handle Other Modified Files

If there are modified files that you did NOT edit in this session:
- List them to the user
- Ask: "There are other modified files that I didn't edit. Should I include them in this commit? (yes/no)"
  - If yes → Stage all modified files
  - If no → Stage only files you edited

### 7. Stage Files

Navigate to repository root and stage the appropriate files:
```bash
cd /path/to/repo
git add file1 file2 file3 ...
```

### 8. Create Commit

Format the commit with:
- Summary line: `#ISSUE_NUMBER: DESCRIPTION`
- Detailed message: `MESSAGE` (on subsequent lines)

**IMPORTANT**: Do NOT add Claude Code attribution footer. Do NOT add Co-Authored-By line.

Execute the commit:
```bash
git commit -m "#ISSUE_NUMBER: DESCRIPTION" -m "MESSAGE"
```

### 9. Get Commit Hash

After successful commit, get the full commit hash:
```bash
git rev-parse HEAD
```

### 10. Ask About Push

Ask the user: "Should I push this commit to the remote? (yes/no)"

If yes:
```bash
git push
```

### 11. Update GitHub Issue

After commit (whether pushed or not):

1. **Add comment to the issue**:
   - Format:
     ```
     Commit: FULL_COMMIT_HASH
     DESCRIPTION

     MESSAGE
     ```
   - Use `gh issue comment ISSUE_NUMBER --body "MESSAGE"`

2. **Add label**:
   - Check if issue already has "Ready for final check" label
   - If not, add it: `gh issue edit ISSUE_NUMBER --add-label "Ready for final check"`

3. **Do NOT close the issue**

## Example Interaction

```
User: Commit this for issue #1151 with description "add context menu items to treeWidget"
```
