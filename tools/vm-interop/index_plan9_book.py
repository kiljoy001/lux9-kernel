#!/usr/bin/env python3
"""
Index Plan 9 Introduction PDF into Solr
Creates a new core called 'plan9' with proper chapter/section structure
"""

import json
import requests
import PyPDF2
import sys
import re

def create_solr_core():
    """Create the plan9 core if it doesn't exist"""
    try:
        # Check if core exists
        r = requests.get('http://localhost:8983/solr/admin/cores?action=STATUS&core=plan9')
        if 'plan9' in r.json().get('status', {}):
            print("Core 'plan9' already exists")
            return
        
        # Create core
        r = requests.get('http://localhost:8983/solr/admin/cores?action=CREATE&name=plan9&configSet=_default')
        print(f"Created core 'plan9': {r.status_code}")
    except Exception as e:
        print(f"Error creating core: {e}")

def extract_chapters(pdf_path):
    """Extract text organized by chapters and sections"""
    documents = []
    
    try:
        with open(pdf_path, 'rb') as file:
            pdf = PyPDF2.PdfReader(file)
            
            current_chapter = ""
            current_section = ""
            current_text = []
            doc_id = 1
            
            for page_num in range(len(pdf.pages)):
                page = pdf.pages[page_num]
                text = page.extract_text()
                
                # Look for chapter markers (e.g., "8. Using the Shell")
                chapter_match = re.search(r'^(\d+)\.\s+(.+?)$', text, re.MULTILINE)
                if chapter_match:
                    # Save previous section if exists
                    if current_text:
                        documents.append({
                            'id': f'plan9_{doc_id}',
                            'chapter': current_chapter,
                            'section': current_section,
                            'page': page_num,
                            'content': '\n'.join(current_text),
                            'type': 'section'
                        })
                        doc_id += 1
                        current_text = []
                    
                    current_chapter = f"{chapter_match.group(1)}. {chapter_match.group(2)}"
                    current_section = ""
                
                # Look for section markers (e.g., "8.1. Programs are tools")
                section_match = re.search(r'^(\d+\.\d+)\.\s+(.+?)$', text, re.MULTILINE)
                if section_match:
                    # Save previous section if exists
                    if current_text:
                        documents.append({
                            'id': f'plan9_{doc_id}',
                            'chapter': current_chapter,
                            'section': current_section,
                            'page': page_num,
                            'content': '\n'.join(current_text),
                            'type': 'section'
                        })
                        doc_id += 1
                        current_text = []
                    
                    current_section = f"{section_match.group(1)}. {section_match.group(2)}"
                
                # Accumulate text
                current_text.append(text)
                
                # Also index each page separately for full-text search
                documents.append({
                    'id': f'plan9_page_{page_num}',
                    'chapter': current_chapter,
                    'section': current_section,
                    'page': page_num,
                    'content': text,
                    'type': 'page'
                })
            
            # Save final section
            if current_text:
                documents.append({
                    'id': f'plan9_{doc_id}',
                    'chapter': current_chapter,
                    'section': current_section,
                    'page': page_num,
                    'content': '\n'.join(current_text),
                    'type': 'section'
                })
                
    except Exception as e:
        print(f"Error extracting PDF: {e}")
        return []
    
    return documents

def index_to_solr(documents):
    """Index documents to Solr plan9 core"""
    url = 'http://localhost:8983/solr/plan9/update/json/docs'
    headers = {'Content-Type': 'application/json'}
    
    # Index in batches
    batch_size = 50
    for i in range(0, len(documents), batch_size):
        batch = documents[i:i+batch_size]
        try:
            r = requests.post(url, json=batch, headers=headers)
            if r.status_code == 200:
                print(f"Indexed batch {i//batch_size + 1}: {len(batch)} docs")
            else:
                print(f"Error indexing batch: {r.status_code} - {r.text}")
        except Exception as e:
            print(f"Error indexing: {e}")
    
    # Commit changes
    requests.get('http://localhost:8983/solr/plan9/update?commit=true')
    print("Committed changes to Solr")

def search_rc_syntax():
    """Search for rc shell syntax examples"""
    queries = [
        'while loop rc',
        'for loop rc', 
        'rc shell syntax',
        'infinite loop',
        'control flow rc'
    ]
    
    for query in queries:
        url = f'http://localhost:8983/solr/plan9/select?q=content:{query}&fl=chapter,section,content&rows=3'
        try:
            r = requests.get(url)
            results = r.json()
            print(f"\n=== Search: {query} ===")
            for doc in results['response']['docs']:
                print(f"Chapter: {doc.get('chapter', 'N/A')}")
                print(f"Section: {doc.get('section', 'N/A')}")
                print(f"Snippet: {doc.get('content', '')[:200]}...")
                print("-" * 40)
        except Exception as e:
            print(f"Search error: {e}")

if __name__ == "__main__":
    pdf_path = sys.argv[1] if len(sys.argv) > 1 else "/home/scott/plan9_intro.pdf"
    
    print("Creating Solr core 'plan9'...")
    create_solr_core()
    
    print(f"Extracting chapters from {pdf_path}...")
    documents = extract_chapters(pdf_path)
    print(f"Extracted {len(documents)} documents")
    
    if documents:
        print("Indexing to Solr...")
        index_to_solr(documents)
        
        print("\nSearching for rc syntax...")
        search_rc_syntax()
        
        print("\nDone! You can now search with:")
        print("curl 'http://localhost:8983/solr/plan9/select?q=content:while+loop+rc&fl=*'")