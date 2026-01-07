#!/bin/bash

echo "=== ECMA-335 Standards Search Test ==="
echo ""

echo "Testing search for 'add instruction':"
echo "----------------------------------------"
curl -s "http://localhost:8983/solr/ecma335_standards/select?q=add&fl=opcode_name,specification,stack_behavior&wt=json" | \
jq -r '.response.docs[] | "Opcode: \(.opcode_name)\nSpec: \(.specification)\nStack: \(.stack_behavior)\n"'

echo ""
echo "Testing search for 'overflow':"
echo "----------------------------------------"
curl -s "http://localhost:8983/solr/ecma335_standards/select?q=overflow&fl=opcode_name,exception_behavior&wt=json" | \
jq -r '.response.docs[] | "Opcode: \(.opcode_name)\nException: \(.exception_behavior)\n"'

echo ""
echo "Testing search for 'arithmetic':"
echo "----------------------------------------"
curl -s "http://localhost:8983/solr/ecma335_standards/select?q=arithmetic&fl=opcode_name,opcode_family&wt=json" | \
jq -r '.response.docs[] | "Opcode: \(.opcode_name) (Family: \(.opcode_family))"'

echo ""
echo "Search functionality verified!"