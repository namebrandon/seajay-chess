- Always push to remote once we've commited, and make sure our "bench <node-count>" string is in the commit message.

# PRIME DIRECTIVE: UCI Scoring Convention
**UCI Protocol mandates that ALL scoring is side-to-move scoring.**
- Internal engine evaluation must return scores from the side-to-move perspective
- UCI output must display scores from the side-to-move perspective  
- Never convert to "White's perspective" for display
- A positive score means the side to move is winning
- A negative score means the side to move is losing
- This applies to:
  - The evaluate() function return value
  - The UCI "eval" command output
  - Search scores and PV display
  - All UCI info output during search