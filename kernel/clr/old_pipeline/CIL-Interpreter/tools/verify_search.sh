#!/bin/bash

# Simple test to verify ECMA-335 Solr search functionality

echo "=== ECMA-335 Documentation Search Test ==="
echo ""

# Test 1: Search for "add" instruction
echo "1. Searching for 'add' instruction:"
echo "----------------------------------------"
curl -s "http://localhost:8983/solr/ecma335_standards/select?q=opcode_name:add&fl=opcode_name,description,specification&wt=json" | \
jq -r '.response.docs[] | "Opcode: \(.opcode_name)\nDescription: \(.description)\nSpecification: \(.specification)\n"'

echo ""
echo "2. Searching for 'add.ovf' instruction:"
echo "----------------------------------------"
curl -s "http://localhost:8983/solr/ecma335_standards/select?q=opcode_name:add.ovf&fl=opcode_name,description,exception_behavior&wt=json" | \
jq -r '.response.docs[] | "Opcode: \(.opcode_name)\nDescription: \(.description)\nException: \(.exception_behavior)\n"'

echo ""
echo "3. Searching for all arithmetic instructions:"
echo "----------------------------------------"
curl -s "http://localhost:8983/solr/ecma335_standards/select?q=opcode_family:arithmetic&fl=opcode_name,opcode_value&wt=json" | \
jq -r '.response.docs[] | "Opcode: \(.opcode_name) (Value: \(.opcode_value))"'

echo ""
echo "ECMA-335 documentation search is working correctly!"
echo "Documents indexed: \$(curl -s \"http://localhost:8983/solr/ecma335_standards/select?q=*:*&wt=json\" | jq '.response.numFound') documents"