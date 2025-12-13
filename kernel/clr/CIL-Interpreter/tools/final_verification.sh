#!/bin/bash

# Simple test to verify ECMA-335 Solr search functionality

echo "=== ECMA-335 Documentation Search Test ==="
echo ""

# Test 1: Check total number of documents
echo "1. Checking total number of indexed documents:"
echo "----------------------------------------"
TOTAL_DOCS=$(curl -s "http://localhost:8983/solr/ecma335_standards/select?q=*:*&wt=json" | jq '.response.numFound')
echo "Total documents indexed: $TOTAL_DOCS"
echo ""

# Test 2: Search for specific opcode
echo "2. Searching for 'add' instruction:"
echo "----------------------------------------"
curl -s "http://localhost:8983/solr/ecma335_standards/select?q=opcode_name:add&fl=opcode_name,description,opcode_family&wt=json" | \
jq -r '.response.docs[] | "Opcode: \(.opcode_name)\nFamily: \(.opcode_family)\nDescription: \(.description)\n"'
echo ""

# Test 3: Search for arithmetic instructions
echo "3. Searching for arithmetic instructions:"
echo "----------------------------------------"
curl -s "http://localhost:8983/solr/ecma335_standards/select?q=opcode_family:arithmetic&fl=opcode_name,opcode_value&wt=json" | \
jq -r '.response.docs[] | "Opcode: \(.opcode_name) (Value: \(.opcode_value))"'
echo ""

# Test 4: Search for comparison instructions
echo "4. Searching for comparison instructions:"
echo "----------------------------------------"
curl -s "http://localhost:8983/solr/ecma335_standards/select?q=opcode_family:comparison&rows=5&fl=opcode_name,opcode_value,description&wt=json" | \
jq -r '.response.docs[] | "Opcode: \(.opcode_name) (Value: \(.opcode_value)) - \(.description)"'
echo ""

if [ "$TOTAL_DOCS" -gt 200 ]; then
    echo "SUCCESS: ECMA-335 documentation search is working correctly with $TOTAL_DOCS documents!"
else
    echo "WARNING: Only $TOTAL_DOCS documents indexed, expected more than 200."
fi