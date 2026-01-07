#!/usr/bin/env python3
"""
Multi-Core Solr Semantic Search Backend
Supports dynamic core selection for searching across different indexes
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
from flask_caching import Cache
import requests
from typing import Dict, List, Any, Optional
from dataclasses import dataclass
from datetime import datetime
import json

app = Flask(__name__)
CORS(app)

# Configure caching
cache = Cache(app, config={
    'CACHE_TYPE': 'simple',
    'CACHE_DEFAULT_TIMEOUT': 300
})

SOLR_HOST = "http://localhost:8983/solr"

@dataclass
class SearchResult:
    id: str
    file_path: str = None
    title: str = None
    content: str = None
    snippet: str = None
    similarity_score: float = 0.0
    relevance: float = 0.0
    metadata: Dict = None

class MultiCoreSearchEngine:
    def __init__(self):
        self.solr_host = SOLR_HOST
        self.core_configs = {
            'plan9': {
                'fields': ['id', 'file_path', 'file_type', 'content', 'coq_similarity_score'],
                'search_fields': 'content^3 file_path^2',
                'display_field': 'file_path'
            },
            'solved_proofs': {
                'fields': ['id', 'title', 'theorem', 'proof', 'proof_type', 'content'],
                'search_fields': 'content^3 theorem^2 title^2',
                'display_field': 'title'
            },
            'blockchain': {
                'fields': ['id', 'title', 'content', 'block_hash', 'transaction_id'],
                'search_fields': 'content^3 title^2',
                'display_field': 'title'
            },
            'coq-stdlib': {
                'fields': ['id', 'name', 'content', 'module', 'type'],
                'search_fields': 'content^3 name^2 module',
                'display_field': 'name'
            }
        }

    def get_available_cores(self) -> List[Dict[str, Any]]:
        """Discover all available Solr cores"""
        try:
            response = requests.get(f"{self.solr_host}/admin/cores?action=STATUS&wt=json")
            data = response.json()

            cores = []
            for core_name, core_info in data.get('status', {}).items():
                # Get document count
                count_response = requests.get(
                    f"{self.solr_host}/{core_name}/select?q=*:*&rows=0&wt=json"
                )
                count_data = count_response.json()
                doc_count = count_data['response']['numFound']

                cores.append({
                    'name': core_name,
                    'documents': doc_count,
                    'size': core_info.get('index', {}).get('sizeInBytes', 0),
                    'last_modified': core_info.get('index', {}).get('lastModified', ''),
                    'configured': core_name in self.core_configs
                })

            return sorted(cores, key=lambda x: x['documents'], reverse=True)
        except Exception as e:
            print(f"Error getting cores: {e}")
            return []

    def get_core_schema(self, core: str) -> Dict[str, List[str]]:
        """Get schema fields for a core"""
        try:
            response = requests.get(
                f"{self.solr_host}/{core}/schema/fields?wt=json"
            )
            data = response.json()

            fields = {}
            for field in data.get('fields', []):
                field_type = field.get('type', 'unknown')
                if field_type not in fields:
                    fields[field_type] = []
                fields[field_type].append(field['name'])

            return fields
        except:
            return {}

    def semantic_search(self, core: str, query: str, filters: Dict = None, limit: int = 20) -> List[SearchResult]:
        """Perform semantic search on specified core"""

        # Get core configuration or use defaults
        config = self.core_configs.get(core, {
            'fields': ['*'],
            'search_fields': 'content^2 title',
            'display_field': 'id'
        })

        # Build query
        solr_query = f'content:"{query}"~2 OR content:{query}* OR title:"{query}"~2 OR title:{query}*'

        # Add filters if provided
        if filters:
            for field, value in filters.items():
                solr_query += f' AND {field}:{value}'

        params = {
            'q': solr_query,
            'defType': 'edismax',
            'qf': config['search_fields'],
            'mm': '30%',
            'fl': ','.join(config['fields']) + ',score',
            'rows': limit,
            'wt': 'json',
            'hl': 'true',
            'hl.fl': 'content,title,theorem',
            'hl.fragsize': 200
        }

        try:
            response = requests.get(f"{self.solr_host}/{core}/select", params=params)
            data = response.json()

            results = []
            highlights = data.get('highlighting', {})

            for doc in data['response']['docs']:
                doc_id = doc['id']

                # Get highlighted snippet
                snippet = ""
                if doc_id in highlights:
                    for field in ['content', 'title', 'theorem']:
                        if field in highlights[doc_id]:
                            snippet = highlights[doc_id][field][0]
                            break

                if not snippet:
                    # Fallback to first 200 chars of content
                    snippet = str(doc.get('content', ''))[:200] + "..."

                # Build result based on core type
                display_value = doc.get(config['display_field'], doc_id)

                result = SearchResult(
                    id=doc_id,
                    file_path=doc.get('file_path'),
                    title=doc.get('title') or doc.get('name') or display_value,
                    content=doc.get('content'),
                    snippet=snippet,
                    similarity_score=doc.get('coq_similarity_score', 0),
                    relevance=doc.get('score', 0),
                    metadata={k: v for k, v in doc.items()
                             if k not in ['id', 'content', '_version_']}
                )
                results.append(result)

            return results
        except Exception as e:
            print(f"Search error: {e}")
            return []

    def find_similar(self, core: str, doc_id: str, limit: int = 10) -> List[SearchResult]:
        """Find similar documents in specified core"""
        config = self.core_configs.get(core, {'fields': ['*'], 'display_field': 'id'})

        params = {
            'q': f'id:{doc_id}',
            'mlt.fl': 'content,title,theorem',
            'mlt.mindf': 1,
            'mlt.mintf': 1,
            'fl': ','.join(config['fields']),
            'rows': limit,
            'wt': 'json'
        }

        try:
            response = requests.get(f"{self.solr_host}/{core}/mlt", params=params)
            data = response.json()

            results = []
            for doc in data['response']['docs']:
                display_value = doc.get(config['display_field'], doc['id'])

                result = SearchResult(
                    id=doc['id'],
                    file_path=doc.get('file_path'),
                    title=doc.get('title') or doc.get('name') or display_value,
                    snippet=str(doc.get('content', ''))[:200] + "...",
                    similarity_score=doc.get('coq_similarity_score', 0),
                    metadata={k: v for k, v in doc.items()
                             if k not in ['id', 'content', '_version_']}
                )
                results.append(result)

            return results
        except Exception as e:
            print(f"Similar search error: {e}")
            return []

    def cross_core_search(self, query: str, cores: List[str] = None, limit_per_core: int = 5) -> Dict[str, List[SearchResult]]:
        """Search across multiple cores simultaneously"""
        if not cores:
            # Search all available cores
            available = self.get_available_cores()
            cores = [c['name'] for c in available if c['documents'] > 0]

        results = {}
        for core in cores:
            try:
                core_results = self.semantic_search(core, query, limit=limit_per_core)
                if core_results:
                    results[core] = core_results
            except:
                continue

        return results

# Initialize search engine
search_engine = MultiCoreSearchEngine()

# API Routes

@app.route('/api/cores', methods=['GET'])
def list_cores():
    """List all available Solr cores"""
    cores = search_engine.get_available_cores()
    return jsonify({'cores': cores})

@app.route('/api/cores/<core>/schema', methods=['GET'])
def get_schema(core):
    """Get schema information for a core"""
    schema = search_engine.get_core_schema(core)
    return jsonify({'core': core, 'schema': schema})

@app.route('/api/search', methods=['POST'])
def search():
    """Search within a specific core or across multiple cores"""
    data = request.json
    query = data.get('query', '')
    core = data.get('core')  # Optional - if not provided, searches all cores
    cores = data.get('cores')  # Optional - list of cores to search
    filters = data.get('filters', {})
    limit = min(data.get('limit', 20), 100)

    if not query:
        return jsonify({'error': 'Query is required'}), 400

    try:
        if core:
            # Single core search
            results = search_engine.semantic_search(core, query, filters, limit)
            return jsonify({
                'query': query,
                'core': core,
                'count': len(results),
                'results': [
                    {
                        'id': r.id,
                        'title': r.title,
                        'snippet': r.snippet,
                        'similarity_score': r.similarity_score,
                        'relevance': r.relevance,
                        'metadata': r.metadata
                    }
                    for r in results
                ]
            })
        else:
            # Multi-core search
            limit_per_core = limit // 3 if not cores else limit // len(cores)
            results = search_engine.cross_core_search(query, cores, limit_per_core)

            response = {
                'query': query,
                'cores_searched': list(results.keys()),
                'total_results': sum(len(r) for r in results.values()),
                'results_by_core': {}
            }

            for core_name, core_results in results.items():
                response['results_by_core'][core_name] = [
                    {
                        'id': r.id,
                        'title': r.title,
                        'snippet': r.snippet,
                        'similarity_score': r.similarity_score,
                        'relevance': r.relevance
                    }
                    for r in core_results
                ]

            return jsonify(response)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/similar/<core>/<doc_id>', methods=['GET'])
def find_similar(core, doc_id):
    """Find similar documents within a specific core"""
    limit = min(request.args.get('limit', 10, type=int), 50)

    try:
        results = search_engine.find_similar(core, doc_id, limit)
        return jsonify({
            'source_id': doc_id,
            'core': core,
            'count': len(results),
            'similar_docs': [
                {
                    'id': r.id,
                    'title': r.title,
                    'snippet': r.snippet,
                    'similarity_score': r.similarity_score,
                    'metadata': r.metadata
                }
                for r in results
            ]
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/document/<core>/<doc_id>', methods=['GET'])
def get_document(core, doc_id):
    """Get full document details from specific core"""
    params = {
        'q': f'id:{doc_id}',
        'fl': '*',
        'rows': 1,
        'wt': 'json'
    }

    try:
        response = requests.get(f"{SOLR_HOST}/{core}/select", params=params)
        data = response.json()

        if data['response']['docs']:
            doc = data['response']['docs'][0]
            # Remove large fields
            for field in ['coq_minhash', 'minhash_signature', 'coq_word2vec', '_version_']:
                doc.pop(field, None)
            return jsonify({'core': core, 'document': doc})
        else:
            return jsonify({'error': 'Document not found'}), 404
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/stats', methods=['GET'])
@cache.cached(timeout=3600)
def stats():
    """Get statistics across all cores"""
    cores = search_engine.get_available_cores()

    total_docs = sum(c['documents'] for c in cores)
    total_size = sum(c['size'] for c in cores)

    return jsonify({
        'total_documents': total_docs,
        'total_size_bytes': total_size,
        'total_size_mb': round(total_size / (1024 * 1024), 2),
        'core_count': len(cores),
        'cores': cores
    })

@app.route('/api/health', methods=['GET'])
def health():
    """Health check endpoint"""
    try:
        response = requests.get(f"{SOLR_HOST}/admin/cores?action=STATUS&wt=json")
        if response.status_code == 200:
            cores = search_engine.get_available_cores()
            return jsonify({
                'status': 'healthy',
                'solr': 'connected',
                'cores_available': len(cores),
                'timestamp': datetime.now().isoformat()
            })
    except:
        pass

    return jsonify({
        'status': 'unhealthy',
        'solr': 'disconnected'
    }), 503

if __name__ == '__main__':
    print("Multi-Core Semantic Search Backend")
    print("===================================")
    print("Available cores:")
    for core in search_engine.get_available_cores():
        print(f"  - {core['name']}: {core['documents']:,} documents")
    print("\nStarting server on http://localhost:5000")
    app.run(host='0.0.0.0', port=5000, debug=True)