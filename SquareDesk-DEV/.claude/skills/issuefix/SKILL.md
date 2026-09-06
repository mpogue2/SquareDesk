---
name: issuefix
description: Root-cause, fix, commit and close out a SquareDesk GitHub issue
---
Given issue number $ARGUMENTS:
1. `gh issue view $ARGUMENTS --comments` to read the report.
2. Reproduce the bug (standalone Qt/Python probe if useful). Do NOT guess a root cause; gather evidence (ASan, instrumentation, benchmark numbers).
3. Post the root-cause analysis + plan to the issue. ALWAYS prefix comments with the Claude Code attribution header.
4. Wait for my approval, then implement.
5. After I confirm the build works: commit, push.
6. Comment the verification result on the issue (with attribution header).
7. `gh issue edit $ARGUMENTS --add-label "Ready for final check"`
