#!/bin/bash
# Multi-core search helpers for Claude
# Use ANY Solr core, not just plan9!

API_URL="http://localhost:5000/api"

# List all available cores with document counts
list_cores() {
    curl -s "$API_URL/cores" | \
        jq -r '.cores[] | "\(.name): \(.documents) docs"'
}

# Search specific core
search_core() {
    local core="$1"
    local query="$2"
    curl -s -X POST "$API_URL/search" \
        -H "Content-Type: application/json" \
        -d "{\"core\": \"$core\", \"query\": \"$query\", \"limit\": 10}" | \
        jq -r '.results[] | "\(.title // .id): \(.snippet[:100])..."'
}

# Search ALL cores at once!
search_all() {
    local query="$1"
    curl -s -X POST "$API_URL/search" \
        -H "Content-Type: application/json" \
        -d "{\"query\": \"$query\", \"limit\": 15}" | \
        jq -r '.results_by_core | to_entries[] | "\n[\(.key)]", (.value[] | "  - \(.title // .id)")'
}

# Search proofs specifically
search_proofs() {
    local query="$1"
    search_core "solved_proofs" "$query"
}

# Search blockchain docs
search_blockchain() {
    local query="$1"
    search_core "blockchain" "$query"
}

# Search Coq stdlib
search_coq() {
    local query="$1"
    search_core "coq-stdlib" "$query"
}

# Search CPU architecture docs
search_cpu() {
    local query="$1"
    # Search across all CPU-related cores
    curl -s -X POST "$API_URL/search" \
        -H "Content-Type: application/json" \
        -d "{\"cores\": [\"cpu_universal\", \"intel64\", \"arm64-manual\", \"powerpc_arch\"], \"query\": \"$query\"}" | \
        jq -r '.results_by_core | to_entries[] | "\n[\(.key)]", (.value[] | "  - \(.title // .id)")'
}

# Find similar docs in any core
find_similar_in_core() {
    local core="$1"
    local doc_id="$2"
    curl -s "$API_URL/similar/$core/$doc_id?limit=5" | \
        jq -r '.similar_docs[] | "\(.similarity_score): \(.title // .id)"'
}

# Get document from specific core
get_doc_from_core() {
    local core="$1"
    local doc_id="$2"
    curl -s "$API_URL/document/$core/$doc_id" | jq '.document'
}

# Show what fields a core has
show_core_schema() {
    local core="$1"
    curl -s "$API_URL/cores/$core/schema" | \
        jq -r '.schema | to_entries[] | "\(.key): \(.value | length) fields"'
}

echo "Multi-Core Search Commands Loaded!"
echo "=================================="
echo "Commands:"
echo "  list_cores                  - Show all available cores"
echo "  search_core <core> <query>  - Search specific core"
echo "  search_all <query>          - Search ALL cores"
echo "  search_proofs <query>       - Search solved_proofs core"
echo "  search_blockchain <query>   - Search blockchain core"
echo "  search_coq <query>          - Search Coq stdlib"
echo "  search_cpu <query>          - Search CPU architecture docs"
echo "  show_core_schema <core>     - Show fields in a core"
echo ""
echo "Examples:"
echo "  search_core plan9 'memory allocation'"
echo "  search_all 'hash table'"
echo "  search_proofs 'induction'"
echo "  search_cpu 'cache coherence'"