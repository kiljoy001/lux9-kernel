/*
 * System.Collections.Generic.Dictionary<TKey, TValue>
 * Lux9 CLR Base Class Library
 */
namespace System.Collections.Generic
{
    using System;
    using System.Collections;

    using System.Reflection;

    public class Dictionary<TKey, TValue> : IDictionary<TKey, TValue>, IDictionary
    {
        private struct Entry
        {
            public int hashCode;    // Lower 31 bits of hash code, -1 if unused
            public int next;        // Index of next entry, -1 if last
            public TKey key;
            public TValue value;
        }

        private int[] _buckets;
        private Entry[] _entries;
        private int _count;
        private int _freeList;
        private int _freeCount;
        private int _version;
        private IEqualityComparer<TKey> _comparer;
        private KeyCollection _keys;
        private ValueCollection _values;

        public Dictionary() : this(0, null) { }
        
        public Dictionary(int capacity) : this(capacity, null) { }
        
        public Dictionary(IEqualityComparer<TKey> comparer) : this(0, comparer) { }
        
        public Dictionary(int capacity, IEqualityComparer<TKey> comparer)
        {
            if (capacity < 0) throw new ArgumentOutOfRangeException(nameof(capacity));
            if (capacity > 0) Initialize(capacity);
            _comparer = comparer ?? EqualityComparer<TKey>.Default;
        }
        
        public int Count => _count - _freeCount;
        
        public TValue this[TKey key]
        {
            get
            {
                int i = FindEntry(key);
                if (i >= 0) return _entries[i].value;
                throw new KeyNotFoundException();
            }
            set
            {
                Insert(key, value, false);
            }
        }
        
        public KeyCollection Keys
        {
            get
            {
                if (_keys == null) _keys = new KeyCollection(this);
                return _keys;
            }
        }
        
        public ValueCollection Values
        {
            get
            {
                if (_values == null) _values = new ValueCollection(this);
                return _values;
            }
        }
        
        public void Add(TKey key, TValue value)
        {
            Insert(key, value, true);
        }
        
        public void Clear()
        {
            if (_count > 0)
            {
                for (int i = 0; i < _buckets.Length; i++) _buckets[i] = -1;
                Array.Clear(_entries, 0, _count);
                _freeList = -1;
                _count = 0;
                _freeCount = 0;
                _version++;
            }
        }
        
        public bool ContainsKey(TKey key)
        {
            return FindEntry(key) >= 0;
        }
        
        public bool ContainsValue(TValue value)
        {
            if (value == null)
            {
                for (int i = 0; i < _count; i++)
                {
                    if (_entries[i].hashCode >= 0 && _entries[i].value == null) return true;
                }
            }
            else
            {
                EqualityComparer<TValue> c = EqualityComparer<TValue>.Default;
                for (int i = 0; i < _count; i++)
                {
                    if (_entries[i].hashCode >= 0 && c.Equals(_entries[i].value, value)) return true;
                }
            }
            return false;
        }
        
        public bool Remove(TKey key)
        {
            if (key == null) throw new ArgumentNullException(nameof(key));
            
            if (_buckets != null)
            {
                int hashCode = _comparer.GetHashCode(key) & 0x7FFFFFFF;
                int bucket = hashCode % _buckets.Length;
                int last = -1;
                for (int i = _buckets[bucket]; i >= 0; last = i, i = _entries[i].next)
                {
                    if (_entries[i].hashCode == hashCode && _comparer.Equals(_entries[i].key, key))
                    {
                        if (last < 0)
                            _buckets[bucket] = _entries[i].next;
                        else
                            _entries[last].next = _entries[i].next;
                        _entries[i].hashCode = -1;
                        _entries[i].next = _freeList;
                        _entries[i].key = default(TKey);
                        _entries[i].value = default(TValue);
                        _freeList = i;
                        _freeCount++;
                        _version++;
                        return true;
                    }
                }
            }
            return false;
        }
        
        public bool TryGetValue(TKey key, out TValue value)
        {
            int i = FindEntry(key);
            if (i >= 0)
            {
                value = _entries[i].value;
                return true;
            }
            value = default(TValue);
            return false;
        }
        
        private void Initialize(int capacity)
        {
            int size = GetPrime(capacity);
            _buckets = new int[size];
            for (int i = 0; i < _buckets.Length; i++) _buckets[i] = -1;
            _entries = new Entry[size];
            _freeList = -1;
        }
        
        private int FindEntry(TKey key)
        {
            if (key == null) throw new ArgumentNullException(nameof(key));
            
            if (_buckets != null)
            {
                int hashCode = _comparer.GetHashCode(key) & 0x7FFFFFFF;
                for (int i = _buckets[hashCode % _buckets.Length]; i >= 0; i = _entries[i].next)
                {
                    if (_entries[i].hashCode == hashCode && _comparer.Equals(_entries[i].key, key))
                        return i;
                }
            }
            return -1;
        }
        
        private void Insert(TKey key, TValue value, bool add)
        {
            if (key == null) throw new ArgumentNullException(nameof(key));
            
            if (_buckets == null) Initialize(0);
            int hashCode = _comparer.GetHashCode(key) & 0x7FFFFFFF;
            int targetBucket = hashCode % _buckets.Length;
            
            for (int i = _buckets[targetBucket]; i >= 0; i = _entries[i].next)
            {
                if (_entries[i].hashCode == hashCode && _comparer.Equals(_entries[i].key, key))
                {
                    if (add) throw new ArgumentException("An item with the same key has already been added.");
                    _entries[i].value = value;
                    _version++;
                    return;
                }
            }
            
            int index;
            if (_freeCount > 0)
            {
                index = _freeList;
                _freeList = _entries[index].next;
                _freeCount--;
            }
            else
            {
                if (_count == _entries.Length)
                {
                    Resize();
                    targetBucket = hashCode % _buckets.Length;
                }
                index = _count;
                _count++;
            }
            
            _entries[index].hashCode = hashCode;
            _entries[index].next = _buckets[targetBucket];
            _entries[index].key = key;
            _entries[index].value = value;
            _buckets[targetBucket] = index;
            _version++;
        }
        
        private void Resize()
        {
            int newSize = GetPrime(_count * 2);
            int[] newBuckets = new int[newSize];
            for (int i = 0; i < newBuckets.Length; i++) newBuckets[i] = -1;
            Entry[] newEntries = new Entry[newSize];
            Array.Copy(_entries, 0, newEntries, 0, _count);
            for (int i = 0; i < _count; i++)
            {
                if (newEntries[i].hashCode >= 0)
                {
                    int bucket = newEntries[i].hashCode % newSize;
                    newEntries[i].next = newBuckets[bucket];
                    newBuckets[bucket] = i;
                }
            }
            _buckets = newBuckets;
            _entries = newEntries;
        }
        
        // Simple prime number calculation for hash table sizes
        private static readonly int[] primes = {
            3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89, 107, 131, 163, 197, 239, 293, 353, 431, 521, 631, 761, 919,
            1103, 1327, 1597, 1931, 2333, 2801, 3371, 4049, 4861, 5839, 7013, 8419, 10103, 12143, 14591, 17519,
            21023, 25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523, 108631, 130363, 156437, 187751, 225307
        };
        
        private static int GetPrime(int min)
        {
            for (int i = 0; i < primes.Length; i++)
            {
                int prime = primes[i];
                if (prime >= min) return prime;
            }
            // For really large sizes, return an odd number
            return min | 1;
        }
        
        // ICollection<KeyValuePair> implementation
        bool ICollection<KeyValuePair<TKey, TValue>>.IsReadOnly => false;
        
        void ICollection<KeyValuePair<TKey, TValue>>.Add(KeyValuePair<TKey, TValue> keyValuePair)
        {
            Add(keyValuePair.Key, keyValuePair.Value);
        }
        
        bool ICollection<KeyValuePair<TKey, TValue>>.Contains(KeyValuePair<TKey, TValue> keyValuePair)
        {
            int i = FindEntry(keyValuePair.Key);
            if (i >= 0 && EqualityComparer<TValue>.Default.Equals(_entries[i].value, keyValuePair.Value))
                return true;
            return false;
        }
        
        void ICollection<KeyValuePair<TKey, TValue>>.CopyTo(KeyValuePair<TKey, TValue>[] array, int index)
        {
            // Stub
        }
        
        bool ICollection<KeyValuePair<TKey, TValue>>.Remove(KeyValuePair<TKey, TValue> keyValuePair)
        {
            int i = FindEntry(keyValuePair.Key);
            if (i >= 0 && EqualityComparer<TValue>.Default.Equals(_entries[i].value, keyValuePair.Value))
            {
                Remove(keyValuePair.Key);
                return true;
            }
            return false;
        }
        
        public Enumerator GetEnumerator() => new Enumerator(this);
        
        IEnumerator<KeyValuePair<TKey, TValue>> IEnumerable<KeyValuePair<TKey, TValue>>.GetEnumerator() => 
            new Enumerator(this);
            
        IEnumerator IEnumerable.GetEnumerator() => new Enumerator(this);
        
        // IDictionary implementation
        ICollection<TKey> IDictionary<TKey, TValue>.Keys => Keys;
        ICollection<TValue> IDictionary<TKey, TValue>.Values => Values;
        
        // IDictionary (non-generic) implementation
        bool IDictionary.IsFixedSize => false;
        bool IDictionary.IsReadOnly => false;
        ICollection IDictionary.Keys => Keys;
        ICollection IDictionary.Values => Values;
        
        object IDictionary.this[object key]
        {
            get => this[(TKey)key];
            set => this[(TKey)key] = (TValue)value;
        }
        
        void IDictionary.Add(object key, object value) => Add((TKey)key, (TValue)value);
        bool IDictionary.Contains(object key) => ContainsKey((TKey)key);
        IDictionaryEnumerator IDictionary.GetEnumerator() => new Enumerator(this);
        void IDictionary.Remove(object key) => Remove((TKey)key);
        
        bool ICollection.IsSynchronized => false;
        object ICollection.SyncRoot => this;
        void ICollection.CopyTo(Array array, int index) { }
        
        public struct Enumerator : IEnumerator<KeyValuePair<TKey, TValue>>, IDictionaryEnumerator
        {
            private Dictionary<TKey, TValue> _dictionary;
            private int _version;
            private int _index;
            private KeyValuePair<TKey, TValue> _current;
            
            internal Enumerator(Dictionary<TKey, TValue> dictionary)
            {
                _dictionary = dictionary;
                _version = dictionary._version;
                _index = 0;
                _current = default;
            }
            
            public bool MoveNext()
            {
                if (_version != _dictionary._version) throw new InvalidOperationException("Collection modified");
                
                while ((uint)_index < (uint)_dictionary._count)
                {
                    if (_dictionary._entries[_index].hashCode >= 0)
                    {
                        _current = new KeyValuePair<TKey, TValue>(_dictionary._entries[_index].key, _dictionary._entries[_index].value);
                        _index++;
                        return true;
                    }
                    _index++;
                }
                
                _index = _dictionary._count + 1;
                _current = default;
                return false;
            }
            
            public KeyValuePair<TKey, TValue> Current => _current;
            object IEnumerator.Current => _current;
            
            public void Dispose() { }
            
            void IEnumerator.Reset()
            {
                if (_version != _dictionary._version) throw new InvalidOperationException("Collection modified");
                _index = 0;
                _current = default;
            }
            
            DictionaryEntry IDictionaryEnumerator.Entry => new DictionaryEntry(_current.Key, _current.Value);
            object IDictionaryEnumerator.Key => _current.Key;
            object IDictionaryEnumerator.Value => _current.Value;
        }
        
        public sealed class KeyCollection : ICollection<TKey>, ICollection
        {
            private Dictionary<TKey, TValue> _dictionary;
            
            public KeyCollection(Dictionary<TKey, TValue> dictionary)
            {
                _dictionary = dictionary ?? throw new ArgumentNullException(nameof(dictionary));
            }
            
            public int Count => _dictionary.Count;
            bool ICollection<TKey>.IsReadOnly => true;
            bool ICollection.IsSynchronized => false;
            object ICollection.SyncRoot => ((ICollection)_dictionary).SyncRoot;
            
            public void CopyTo(TKey[] array, int index)
            {
                int count = _dictionary._count;
                Entry[] entries = _dictionary._entries;
                for (int i = 0; i < count; i++)
                {
                    if (entries[i].hashCode >= 0) array[index++] = entries[i].key;
                }
            }
            
            void ICollection.CopyTo(Array array, int index) { }
            
            void ICollection<TKey>.Add(TKey item) => throw new NotSupportedException();
            void ICollection<TKey>.Clear() => throw new NotSupportedException();
            bool ICollection<TKey>.Contains(TKey item) => _dictionary.ContainsKey(item);
            bool ICollection<TKey>.Remove(TKey item) => throw new NotSupportedException();
            
            public IEnumerator<TKey> GetEnumerator() => new Enumerator(_dictionary);
            IEnumerator IEnumerable.GetEnumerator() => new Enumerator(_dictionary);
            
            public struct Enumerator : IEnumerator<TKey>
            {
                private Dictionary<TKey, TValue> _dictionary;
                private int _index;
                private int _version;
                private TKey _current;
                
                internal Enumerator(Dictionary<TKey, TValue> dictionary)
                {
                    _dictionary = dictionary;
                    _version = dictionary._version;
                    _index = 0;
                    _current = default;
                }
                
                public bool MoveNext()
                {
                    if (_version != _dictionary._version) throw new InvalidOperationException();
                    while ((uint)_index < (uint)_dictionary._count)
                    {
                        if (_dictionary._entries[_index].hashCode >= 0)
                        {
                            _current = _dictionary._entries[_index].key;
                            _index++;
                            return true;
                        }
                        _index++;
                    }
                    _current = default;
                    return false;
                }
                
                public TKey Current => _current;
                object IEnumerator.Current => _current;
                public void Dispose() { }
                void IEnumerator.Reset() { _index = 0; _current = default; }
            }
        }
        
        public sealed class ValueCollection : ICollection<TValue>, ICollection
        {
            private Dictionary<TKey, TValue> _dictionary;
            
            public ValueCollection(Dictionary<TKey, TValue> dictionary)
            {
                _dictionary = dictionary ?? throw new ArgumentNullException(nameof(dictionary));
            }
            
            public int Count => _dictionary.Count;
            bool ICollection<TValue>.IsReadOnly => true;
            bool ICollection.IsSynchronized => false;
            object ICollection.SyncRoot => ((ICollection)_dictionary).SyncRoot;
            
            public void CopyTo(TValue[] array, int index)
            {
                int count = _dictionary._count;
                Entry[] entries = _dictionary._entries;
                for (int i = 0; i < count; i++)
                {
                    if (entries[i].hashCode >= 0) array[index++] = entries[i].value;
                }
            }
            
            void ICollection.CopyTo(Array array, int index) { }
            
            void ICollection<TValue>.Add(TValue item) => throw new NotSupportedException();
            void ICollection<TValue>.Clear() => throw new NotSupportedException();
            bool ICollection<TValue>.Contains(TValue item) => _dictionary.ContainsValue(item);
            bool ICollection<TValue>.Remove(TValue item) => throw new NotSupportedException();
            
            public IEnumerator<TValue> GetEnumerator() => new Enumerator(_dictionary);
            IEnumerator IEnumerable.GetEnumerator() => new Enumerator(_dictionary);
            
            public struct Enumerator : IEnumerator<TValue>
            {
                private Dictionary<TKey, TValue> _dictionary;
                private int _index;
                private int _version;
                private TValue _current;
                
                internal Enumerator(Dictionary<TKey, TValue> dictionary)
                {
                    _dictionary = dictionary;
                    _version = dictionary._version;
                    _index = 0;
                    _current = default;
                }
                
                public bool MoveNext()
                {
                    if (_version != _dictionary._version) throw new InvalidOperationException();
                    while ((uint)_index < (uint)_dictionary._count)
                    {
                        if (_dictionary._entries[_index].hashCode >= 0)
                        {
                            _current = _dictionary._entries[_index].value;
                            _index++;
                            return true;
                        }
                        _index++;
                    }
                    _current = default;
                    return false;
                }
                
                public TValue Current => _current;
                object IEnumerator.Current => _current;
                public void Dispose() { }
                void IEnumerator.Reset() { _index = 0; _current = default; }
            }
        }
    }
    

}
