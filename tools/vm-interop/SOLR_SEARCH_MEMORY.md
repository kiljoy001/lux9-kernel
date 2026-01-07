## MANDATORY: How to Search Solr for Documentation

**NEVER USE PYTHON FOR SOLR SEARCHES - IT ALWAYS FAILS**

### Available Solr Cores:
- `plan9` - Plan 9 documentation and man pages
- `solved_proofs` - Coq proofs 
- `blockchain`, `coq-stdlib`, `cpu_universal`, etc.

### CORRECT Solr Search Method:
```bash
# For Plan 9 documentation:
curl -s "http://localhost:8983/solr/plan9/select?q=SEARCH_TERM&fl=title,content&rows=5&wt=json" | jq -r '.response.docs[] | "\n=== \(.title) ===\n\(.content[:1000])"'

# For proofs:
curl -s "http://localhost:8983/solr/solved_proofs/select?q=content:SEARCH_TERM&fl=title,content&rows=5&wt=json" | jq -r '.response.docs[] | .title'
```

### WRONG Methods (DO NOT USE):
```bash
# NEVER do this - Python JSON parsing always fails:
curl ... | python3 -c "import json, sys; ..."  # BROKEN!
```

### Before ANY Plan 9 Command:
1. Search Solr for the command's man page
2. Read and understand the ACTUAL flags and syntax
3. Only then use the command

### Example Searches:
```bash
# Search for mk documentation:
curl -s "http://localhost:8983/solr/plan9/select?q=mk&fl=title,content&rows=3&wt=json" | jq -r '.response.docs[].content[:500]'

# Search for rc shell syntax:
curl -s "http://localhost:8983/solr/plan9/select?q=rc+redirect&fl=content&rows=2&wt=json" | jq -r '.response.docs[].content[:1000]'
```

**REMEMBER**: Always use `jq`, never Python. Always search before using a command.