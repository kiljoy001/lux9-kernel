#!/bin/bash

echo "=== ECMA-335 Enhanced Search Verification ==="
echo ""

# Test 1: Check total document count
echo "1. Total document count:"
echo "------------------------"
TOTAL_DOCS=$(curl -s "http://localhost:8983/solr/ecma335_standards/select?q=*:*&wt=json" | jq '.response.numFound')
echo "Total documents: $TOTAL_DOCS"
echo ""

# Test 2: Search for specific opcode
echo "2. Searching for 'add' opcode:"
echo "------------------------------"
curl -s "http://localhost:8983/solr/ecma335_standards/select?q=opcode_name:add&fl=opcode_name,description,opcode_family&wt=json" | \
jq -r '.response.docs[] | "Opcode: \(.opcode_name)\nFamily: \(.opcode_family)\nDescription: \(.description)\n"'
echo ""

# Test 3: Search for documentation chunks
echo "3. Searching for documentation chunks:"
echo "--------------------------------------"
curl -s "http://localhost:8983/solr/ecma335_standards/select?q=opcode_family:documentation&fl=id,description&wt=json" | \
jq -r '.response.docs[] | "Document: \(.id) - \(.description)"' | head -5
echo ""

# Test 4: Search for text within documentation
echo "4. Searching for 'partition' in documentation:"
echo "----------------------------------------------"
curl -s "http://localhost:8983/solr/ecma335_standards/select?q=partition&fl=id,description&wt=json" | \
jq -r '.response.docs[] | "Found in: \(.id) - \(.description)"' | head -3
echo ""

# Test 5: Mixed search (both opcodes and documentation)
echo "5. Searching for 'arithmetic' (should find both opcodes and docs):"
echo "------------------------------------------------------------------"
curl -s "http://localhost:8983/solr/ecma335_standards/select?q=arithmetic&wt=json" | \
jq -r '.response.docs[] | if .opcode_family == "documentation" then "Doc: \(.description)" else "Opcode: \(.opcode_name) (\(.opcode_family))" end' | head -5
echo ""

echo "Enhanced search system verification completed!"
echo "You can now search for both CIL opcodes and ECMA-335 documentation."