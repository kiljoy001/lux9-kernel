#!/bin/bash
# Simplified search helpers for Claude using the API
# Much more reliable than direct Solr queries!

API_URL="http://localhost:5000/api"

# Simple semantic search - MUCH easier than Solr!
search_code() {
    local query="$1"
    curl -s -X POST "$API_URL/search" \
        -H "Content-Type: application/json" \
        -d "{\"query\": \"$query\", \"limit\": 10}" | \
        jq -r '.results[] | "\(.file_path): \(.snippet[:100])..."'
}

# Find similar files - one line instead of complex Solr MLT
find_similar() {
    local doc_id="$1"
    curl -s "$API_URL/similar/$doc_id?limit=5" | \
        jq -r '.similar_docs[] | "\(.similarity_score): \(.file_path)"'
}

# Quick file type search
search_c_files() {
    local query="$1"
    curl -s -X POST "$API_URL/search" \
        -H "Content-Type: application/json" \
        -d "{\"query\": \"$query\", \"filters\": {\"file_type\": \"c_source\"}}" | \
        jq -r '.results[] | .file_path'
}

# Get document details
get_doc() {
    local doc_id="$1"
    curl -s "$API_URL/document/$doc_id" | jq '.'
}

# Check what's indexed
check_stats() {
    curl -s "$API_URL/stats" | \
        jq -r '"Total docs: \(.total_documents)\nFile types: \(.file_types | keys | join(", "))"'
}

echo "Claude-friendly search commands loaded!"
echo "Commands: search_code, find_similar, search_c_files, get_doc, check_stats"