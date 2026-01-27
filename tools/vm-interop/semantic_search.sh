#!/bin/bash

# Semantic Search Functions for Plan 9 Solr Index

# 1. More Like This (MLT) - Find similar documents
find_similar_docs() {
    local doc_id="$1"
    echo "Finding docs similar to: $doc_id"
    curl -s "http://localhost:8983/solr/plan9/mlt?q=id:$doc_id&mlt.fl=content,coq_word2vec&mlt.mindf=1&mlt.mintf=1&rows=5&fl=id,file_path,coq_similarity_score&wt=json" | \
    jq -r '.response.docs[] | "[\(.coq_similarity_score // 0)] \(.file_path)"'
}

# 2. Vector similarity search using word2vec embeddings
vector_search() {
    local query="$1"
    echo "Vector search for: $query"
    # First get a document with similar content, then use its vector
    curl -s "http://localhost:8983/solr/plan9/select?q=content:\"$query\"&fl=coq_word2vec&rows=1&wt=json" | \
    jq -r '.response.docs[0].coq_word2vec' | \
    xargs -I {} curl -s "http://localhost:8983/solr/plan9/select?q=coq_word2vec:\"{}\"~0.8&fl=file_path,coq_similarity_score&rows=5&wt=json" | \
    jq -r '.response.docs[] | "[\(.coq_similarity_score)] \(.file_path)"'
}

# 3. MinHash similarity for near-duplicate detection
minhash_similarity() {
    local doc_id="$1"
    echo "Finding near-duplicates of: $doc_id"
    # Get the minhash of the source document
    local minhash=$(curl -s "http://localhost:8983/solr/plan9/select?q=id:$doc_id&fl=simhash_fingerprint&wt=json" | jq -r '.response.docs[0].simhash_fingerprint')

    if [ -n "$minhash" ]; then
        curl -s "http://localhost:8983/solr/plan9/select?q=simhash_fingerprint:$minhash~2&fl=file_path,id&rows=10&wt=json" | \
        jq -r '.response.docs[] | "\(.id): \(.file_path)"'
    fi
}

# 4. Fuzzy semantic search with boosting
semantic_search() {
    local query="$1"
    echo "Semantic search for: $query"
    curl -s "http://localhost:8983/solr/plan9/select?q=(content:\"$query\"~2^3 OR content:$query*^2 OR content:*$query*^1)&qf=content^2 file_path^1.5&defType=edismax&mm=50%&fl=file_path,coq_similarity_score,score&rows=10&wt=json" | \
    jq -r '.response.docs[] | "[Score: \(.score)] [\(.coq_similarity_score // 0)] \(.file_path)"'
}

# 5. Find related code by pattern matching
pattern_search() {
    local pattern="$1"
    echo "Pattern search for: $pattern"
    curl -s "http://localhost:8983/solr/plan9/select?q=coq_logic_patterns:\"$pattern\"&fl=file_path,type,coq_similarity_score&rows=10&wt=json" | \
    jq -r '.response.docs[] | "[\(.type)] \(.file_path)"'
}

# 6. Conceptual search using symbol frequency
symbol_freq_search() {
    local doc_id="$1"
    echo "Finding docs with similar symbol frequency to: $doc_id"
    local sym_freq=$(curl -s "http://localhost:8983/solr/plan9/select?q=id:$doc_id&fl=coq_symbol_freq&wt=json" | jq -r '.response.docs[0].coq_symbol_freq')

    if [ -n "$sym_freq" ]; then
        curl -s "http://localhost:8983/solr/plan9/select?q=coq_symbol_freq:\"$sym_freq\"~10&fl=file_path,coq_similarity_score&rows=5&wt=json" | \
        jq -r '.response.docs[] | "[\(.coq_similarity_score)] \(.file_path)"'
    fi
}

# Main menu
echo "Plan 9 Semantic Search Tools"
echo "============================"
echo "Usage:"
echo "  find_similar_docs <doc_id>    - Find similar documents using MLT"
echo "  vector_search <query>          - Search using word2vec embeddings"
echo "  minhash_similarity <doc_id>    - Find near-duplicates"
echo "  semantic_search <query>        - Fuzzy semantic search with boosting"
echo "  pattern_search <pattern>       - Search by logic patterns"
echo "  symbol_freq_search <doc_id>    - Find docs with similar symbol usage"
echo ""
echo "Example: semantic_search 'imap message parsing'"