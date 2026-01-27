#!/usr/bin/env python3
"""
Index Plan 9 man pages into Solr
"""

import json
import requests
import os
import sys
import re
from pathlib import Path

SOLR_URL = 'http://localhost:8983/solr/plan9'

def index_man_pages(man_dir):
    """Index all man pages from Plan 9"""
    documents = []
    
    for section in range(1, 10):
        section_dir = os.path.join(man_dir, str(section))
        if not os.path.exists(section_dir):
            continue
            
        print(f"Indexing man section {section}...")
        
        for man_file in os.listdir(section_dir):
            filepath = os.path.join(section_dir, man_file)
            
            try:
                with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                
                # Extract synopsis and description
                synopsis = ""
                if ".SH SYNOPSIS" in content:
                    idx = content.find(".SH SYNOPSIS")
                    end = content.find(".SH", idx + 10)
                    if end > idx:
                        synopsis = content[idx:end]
                
                doc = {
                    'id': f'man_{section}_{man_file}',
                    'chapter': f'man({section})',
                    'section': man_file,
                    'page': section,
                    'content': content,
                    'type': 'manual',
                    'file_path': f'man/{section}/{man_file}'
                }
                
                documents.append(doc)
                
            except Exception as e:
                print(f"  Error reading {filepath}: {e}")
                continue
    
    return documents

def index_to_solr(documents):
    """Index documents to Solr"""
    url = f'{SOLR_URL}/update/json/docs'
    headers = {'Content-Type': 'application/json'}
    
    print(f"\nIndexing {len(documents)} man pages to Solr...")
    
    batch_size = 50
    for i in range(0, len(documents), batch_size):
        batch = documents[i:i+batch_size]
        try:
            r = requests.post(url, json=batch, headers=headers)
            if r.status_code == 200:
                print(f"  Indexed batch {i//batch_size + 1}: {len(batch)} docs")
            else:
                print(f"  Error: {r.status_code}")
        except Exception as e:
            print(f"  Error indexing: {e}")
    
    # Commit changes
    requests.get(f'{SOLR_URL}/update?commit=true')
    print("Committed changes to Solr")

def search_man_pages():
    """Test searching man pages"""
    queries = [
        ('acme', 'man(1) acme'),
        ('rc shell', 'man rc'),
        ('9p protocol', 'man 9p')
    ]
    
    for query, desc in queries:
        url = f'{SOLR_URL}/select'
        params = {
            'q': f'content:{query} AND type:manual',
            'fl': 'chapter,section',
            'rows': 3,
            'wt': 'json'
        }
        try:
            r = requests.get(url, params=params)
            results = r.json()
            print(f"\n{desc}: {results['response']['numFound']} results")
            for doc in results['response']['docs'][:3]:
                print(f"  - {doc.get('chapter', '')} {doc.get('section', '')}")
        except Exception as e:
            print(f"Search error: {e}")

if __name__ == "__main__":
    man_dir = "/home/scott/Repo/9front/sys/man"
    
    if not os.path.exists(man_dir):
        print(f"Error: {man_dir} does not exist")
        sys.exit(1)
    
    documents = index_man_pages(man_dir)
    print(f"Found {len(documents)} man pages")
    
    if documents:
        index_to_solr(documents)
        
        # Add TurboCID
        print("\nAdding TurboCID metadata...")
        os.system("python3 /home/scott/Repo/solr-plugins/add_turbocid_metadata.py plan9")
        
        search_man_pages()
        
        print("\nMan pages indexed! Search with:")
        print("curl 'http://localhost:8983/solr/plan9/select?q=type:manual+AND+content:acme&fl=chapter,section,content&rows=1'")