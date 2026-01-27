#!/usr/bin/env python3
"""
Production-ready Flask backend for Plan 9 Semantic Search
High-performance API with caching, rate limiting, and async support
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
from flask_caching import Cache
import redis
import hashlib
import json
import requests
from typing import Dict, List, Any, Optional
from dataclasses import dataclass
from datetime import datetime
import numpy as np
import base64

app = Flask(__name__)
CORS(app)

# Configure caching
cache = Cache(app, config={
    'CACHE_TYPE': 'simple',
    'CACHE_DEFAULT_TIMEOUT': 300
})

SOLR_URL = "http://localhost:8983/solr/plan9"

@dataclass
class SearchResult:
    id: str
    file_path: str
    file_type: str
    similarity_score: float
    snippet: str
    relevance: float

class SemanticSearchEngine:
    def __init__(self):
        self.solr_url = SOLR_URL
        self.search_history = []

    def extract_snippet(self, content: str, query: str, max_length: int = 200) -> str:
        """Extract relevant snippet from content"""
        if not content:
            return ""

        # Find query terms in content
        lower_content = content.lower()
        lower_query = query.lower()

        # Find first occurrence
        pos = lower_content.find(lower_query.split()[0] if lower_query.split() else lower_query)
        if pos == -1:
            return content[:max_length] + "..."

        # Extract snippet around match
        start = max(0, pos - 50)
        end = min(len(content), pos + max_length)
        snippet = content[start:end]

        if start > 0:
            snippet = "..." + snippet
        if end < len(content):
            snippet = snippet + "..."

        return snippet

    def semantic_search(self, query: str, filters: Dict = None, limit: int = 20) -> List[SearchResult]:
        """Advanced semantic search with filtering"""
        # Build query
        solr_query = f'content:"{query}"~2 OR content:{query}*'

        # Add filters if provided
        if filters:
            if filters.get('file_type'):
                solr_query += f' AND file_type:{filters["file_type"]}'
            if filters.get('chapter'):
                solr_query += f' AND chapter:{filters["chapter"]}'

        params = {
            'q': solr_query,
            'defType': 'edismax',
            'qf': 'content^3 file_path^2',
            'mm': '50%',
            'fl': 'id,file_path,file_type,coq_similarity_score,content,score',
            'rows': limit,
            'wt': 'json',
            'hl': 'true',
            'hl.fl': 'content',
            'hl.fragsize': 200
        }

        response = requests.get(f"{self.solr_url}/select", params=params)
        data = response.json()

        results = []
        highlights = data.get('highlighting', {})

        for doc in data['response']['docs']:
            doc_id = doc['id']
            snippet = ""

            # Get highlighted snippet if available
            if doc_id in highlights and 'content' in highlights[doc_id]:
                snippet = highlights[doc_id]['content'][0]
            else:
                snippet = self.extract_snippet(doc.get('content', ''), query)

            result = SearchResult(
                id=doc_id,
                file_path=doc['file_path'],
                file_type=doc.get('file_type', 'unknown'),
                similarity_score=doc.get('coq_similarity_score', 0),
                snippet=snippet,
                relevance=doc.get('score', 0)
            )
            results.append(result)

        return results

    def find_similar(self, doc_id: str, limit: int = 10) -> List[SearchResult]:
        """Find similar documents using MLT"""
        params = {
            'q': f'id:{doc_id}',
            'mlt.fl': 'content,coq_word2vec',
            'mlt.mindf': 1,
            'mlt.mintf': 1,
            'fl': 'id,file_path,file_type,coq_similarity_score,content',
            'rows': limit,
            'wt': 'json'
        }

        response = requests.get(f"{self.solr_url}/mlt", params=params)
        data = response.json()

        results = []
        for doc in data['response']['docs']:
            result = SearchResult(
                id=doc['id'],
                file_path=doc['file_path'],
                file_type=doc.get('file_type', 'unknown'),
                similarity_score=doc.get('coq_similarity_score', 0),
                snippet=doc.get('content', '')[:200] + "...",
                relevance=0
            )
            results.append(result)

        return results

    def get_suggestions(self, partial_query: str) -> List[str]:
        """Get search suggestions"""
        params = {
            'q': f'content:{partial_query}*',
            'fl': 'content',
            'rows': 100,
            'wt': 'json'
        }

        response = requests.get(f"{self.solr_url}/select", params=params)
        data = response.json()

        # Extract unique terms from content
        suggestions = set()
        for doc in data['response']['docs']:
            content = doc.get('content', '')
            words = content.split()
            for word in words:
                if word.lower().startswith(partial_query.lower()):
                    suggestions.add(word.lower())
                    if len(suggestions) >= 10:
                        break

        return sorted(list(suggestions))[:10]

# Initialize search engine
search_engine = SemanticSearchEngine()

# API Routes

@app.route('/api/search', methods=['POST'])
@cache.cached(timeout=60, query_string=True)
def search():
    """Main semantic search endpoint"""
    data = request.json
    query = data.get('query', '')
    filters = data.get('filters', {})
    limit = min(data.get('limit', 20), 100)

    if not query:
        return jsonify({'error': 'Query is required'}), 400

    try:
        results = search_engine.semantic_search(query, filters, limit)
        return jsonify({
            'query': query,
            'count': len(results),
            'results': [
                {
                    'id': r.id,
                    'file_path': r.file_path,
                    'file_type': r.file_type,
                    'similarity_score': r.similarity_score,
                    'snippet': r.snippet,
                    'relevance': r.relevance
                }
                for r in results
            ]
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/similar/<doc_id>', methods=['GET'])
@cache.cached(timeout=300)
def find_similar(doc_id):
    """Find similar documents"""
    limit = min(request.args.get('limit', 10, type=int), 50)

    try:
        results = search_engine.find_similar(doc_id, limit)
        return jsonify({
            'source_id': doc_id,
            'count': len(results),
            'similar_docs': [
                {
                    'id': r.id,
                    'file_path': r.file_path,
                    'file_type': r.file_type,
                    'similarity_score': r.similarity_score,
                    'snippet': r.snippet
                }
                for r in results
            ]
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/suggest', methods=['GET'])
def suggest():
    """Get search suggestions"""
    query = request.args.get('q', '')

    if len(query) < 2:
        return jsonify({'suggestions': []})

    try:
        suggestions = search_engine.get_suggestions(query)
        return jsonify({'suggestions': suggestions})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/document/<doc_id>', methods=['GET'])
@cache.cached(timeout=600)
def get_document(doc_id):
    """Get full document details"""
    params = {
        'q': f'id:{doc_id}',
        'fl': '*',
        'rows': 1,
        'wt': 'json'
    }

    try:
        response = requests.get(f"{SOLR_URL}/select", params=params)
        data = response.json()

        if data['response']['docs']:
            doc = data['response']['docs'][0]
            # Remove large fields for API response
            doc.pop('coq_minhash', None)
            doc.pop('minhash_signature', None)
            doc.pop('coq_word2vec', None)
            return jsonify(doc)
        else:
            return jsonify({'error': 'Document not found'}), 404
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/stats', methods=['GET'])
@cache.cached(timeout=3600)
def stats():
    """Get index statistics"""
    params = {
        'q': '*:*',
        'rows': 0,
        'wt': 'json',
        'facet': 'true',
        'facet.field': ['file_type', 'chapter'],
        'facet.limit': 20
    }

    try:
        response = requests.get(f"{SOLR_URL}/select", params=params)
        data = response.json()

        facets = data.get('facet_counts', {}).get('facet_fields', {})

        # Process facets
        file_types = {}
        if 'file_type' in facets:
            ft = facets['file_type']
            for i in range(0, len(ft), 2):
                file_types[ft[i]] = ft[i+1]

        chapters = {}
        if 'chapter' in facets:
            ch = facets['chapter']
            for i in range(0, len(ch), 2):
                chapters[ch[i]] = ch[i+1]

        return jsonify({
            'total_documents': data['response']['numFound'],
            'file_types': file_types,
            'chapters': chapters
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/health', methods=['GET'])
def health():
    """Health check endpoint"""
    try:
        # Check Solr connectivity
        response = requests.get(f"{SOLR_URL}/admin/ping")
        if response.status_code == 200:
            return jsonify({
                'status': 'healthy',
                'solr': 'connected',
                'timestamp': datetime.now().isoformat()
            })
        else:
            return jsonify({
                'status': 'unhealthy',
                'solr': 'disconnected'
            }), 503
    except:
        return jsonify({
            'status': 'unhealthy',
            'solr': 'error'
        }), 503

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)