/*
 * System.Collections.Generic.HashSet<T>
 * Lux9 CLR Base Class Library
 */
namespace System.Collections.Generic
{
    using System;
    using System.Collections;

    /// <summary>
    /// Represents a set of values with O(1) lookup, add, and remove.
    /// </summary>
    public class HashSet<T> : ISet<T>, ICollection<T>, IEnumerable<T>, IEnumerable
    {
        private struct Slot
        {
            internal int hashCode;
            internal int next;
            internal T value;
        }
        
        private int[] _buckets;
        private Slot[] _slots;
        private int _count;
        private int _lastIndex;
        private int _freeList;
        private IEqualityComparer<T> _comparer;
        private int _version;
        
        public HashSet() : this((IEqualityComparer<T>)null) { }
        
        public HashSet(IEqualityComparer<T> comparer)
        {
            _comparer = comparer ?? EqualityComparer<T>.Default;
            _freeList = -1;
        }
        
        public HashSet(IEnumerable<T> collection) : this(collection, null) { }
        
        public HashSet(IEnumerable<T> collection, IEqualityComparer<T> comparer) : this(comparer)
        {
            if (collection == null) throw new ArgumentNullException(nameof(collection));
            foreach (T item in collection)
            {
                Add(item);
            }
        }
        
        public int Count => _count;
        
        bool ICollection<T>.IsReadOnly => false;
        
        public new bool Add(T item)
        {
            return AddIfNotPresent(item);
        }
        
        void ICollection<T>.Add(T item)
        {
            AddIfNotPresent(item);
        }
        
        public void Clear()
        {
            if (_lastIndex > 0)
            {
                Array.Clear(_slots, 0, _lastIndex);
                Array.Clear(_buckets, 0, _buckets.Length);
                _lastIndex = 0;
                _count = 0;
                _freeList = -1;
            }
            _version++;
        }
        
        public bool Contains(T item)
        {
            if (_buckets != null)
            {
                int hashCode = InternalGetHashCode(item);
                for (int i = _buckets[hashCode % _buckets.Length] - 1; i >= 0; i = _slots[i].next)
                {
                    if (_slots[i].hashCode == hashCode && _comparer.Equals(_slots[i].value, item))
                    {
                        return true;
                    }
                }
            }
            return false;
        }
        
        public bool Remove(T item)
        {
            if (_buckets != null)
            {
                int hashCode = InternalGetHashCode(item);
                int bucket = hashCode % _buckets.Length;
                int last = -1;
                for (int i = _buckets[bucket] - 1; i >= 0; last = i, i = _slots[i].next)
                {
                    if (_slots[i].hashCode == hashCode && _comparer.Equals(_slots[i].value, item))
                    {
                        if (last < 0)
                        {
                            _buckets[bucket] = _slots[i].next + 1;
                        }
                        else
                        {
                            _slots[last].next = _slots[i].next;
                        }
                        _slots[i].hashCode = -1;
                        _slots[i].value = default(T);
                        _slots[i].next = _freeList;
                        
                        _count--;
                        _version++;
                        if (_count == 0)
                        {
                            _lastIndex = 0;
                            _freeList = -1;
                        }
                        else
                        {
                            _freeList = i;
                        }
                        return true;
                    }
                }
            }
            return false;
        }
        
        public void CopyTo(T[] array, int arrayIndex)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            if (arrayIndex < 0) throw new ArgumentOutOfRangeException(nameof(arrayIndex));
            
            for (int i = 0; i < _lastIndex && arrayIndex < array.Length; i++)
            {
                if (_slots[i].hashCode >= 0)
                {
                    array[arrayIndex++] = _slots[i].value;
                }
            }
        }
        
        // Set operations
        public void UnionWith(IEnumerable<T> other)
        {
            if (other == null) throw new ArgumentNullException(nameof(other));
            foreach (T item in other)
            {
                AddIfNotPresent(item);
            }
        }
        
        public void IntersectWith(IEnumerable<T> other)
        {
            if (other == null) throw new ArgumentNullException(nameof(other));
            if (_count == 0) return;
            
            HashSet<T> otherSet = other as HashSet<T> ?? new HashSet<T>(other, _comparer);
            for (int i = 0; i < _lastIndex; i++)
            {
                if (_slots[i].hashCode >= 0)
                {
                    T item = _slots[i].value;
                    if (!otherSet.Contains(item))
                    {
                        Remove(item);
                    }
                }
            }
        }
        
        public void ExceptWith(IEnumerable<T> other)
        {
            if (other == null) throw new ArgumentNullException(nameof(other));
            if (_count == 0) return;
            foreach (T item in other)
            {
                Remove(item);
            }
        }
        
        public void SymmetricExceptWith(IEnumerable<T> other)
        {
            if (other == null) throw new ArgumentNullException(nameof(other));
            foreach (T item in other)
            {
                if (!Remove(item))
                {
                    AddIfNotPresent(item);
                }
            }
        }
        
        public bool IsSubsetOf(IEnumerable<T> other)
        {
            if (other == null) throw new ArgumentNullException(nameof(other));
            if (_count == 0) return true;
            
            HashSet<T> otherSet = other as HashSet<T> ?? new HashSet<T>(other, _comparer);
            if (_count > otherSet.Count) return false;
            
            foreach (T item in this)
            {
                if (!otherSet.Contains(item)) return false;
            }
            return true;
        }
        
        public bool IsSupersetOf(IEnumerable<T> other)
        {
            if (other == null) throw new ArgumentNullException(nameof(other));
            foreach (T item in other)
            {
                if (!Contains(item)) return false;
            }
            return true;
        }
        
        public bool IsProperSubsetOf(IEnumerable<T> other)
        {
            if (other == null) throw new ArgumentNullException(nameof(other));
            HashSet<T> otherSet = other as HashSet<T> ?? new HashSet<T>(other, _comparer);
            return _count < otherSet.Count && IsSubsetOf(otherSet);
        }
        
        public bool IsProperSupersetOf(IEnumerable<T> other)
        {
            if (other == null) throw new ArgumentNullException(nameof(other));
            HashSet<T> otherSet = other as HashSet<T> ?? new HashSet<T>(other, _comparer);
            return _count > otherSet.Count && IsSupersetOf(otherSet);
        }
        
        public bool Overlaps(IEnumerable<T> other)
        {
            if (other == null) throw new ArgumentNullException(nameof(other));
            if (_count == 0) return false;
            foreach (T item in other)
            {
                if (Contains(item)) return true;
            }
            return false;
        }
        
        public bool SetEquals(IEnumerable<T> other)
        {
            if (other == null) throw new ArgumentNullException(nameof(other));
            HashSet<T> otherSet = other as HashSet<T> ?? new HashSet<T>(other, _comparer);
            if (_count != otherSet.Count) return false;
            return IsSubsetOf(otherSet);
        }
        
        private bool AddIfNotPresent(T value)
        {
            if (_buckets == null)
            {
                Initialize(0);
            }
            
            int hashCode = InternalGetHashCode(value);
            int bucket = hashCode % _buckets.Length;
            
            for (int i = _buckets[bucket] - 1; i >= 0; i = _slots[i].next)
            {
                if (_slots[i].hashCode == hashCode && _comparer.Equals(_slots[i].value, value))
                {
                    return false;
                }
            }
            
            int index;
            if (_freeList >= 0)
            {
                index = _freeList;
                _freeList = _slots[index].next;
            }
            else
            {
                if (_lastIndex == _slots.Length)
                {
                    IncreaseCapacity();
                    bucket = hashCode % _buckets.Length;
                }
                index = _lastIndex;
                _lastIndex++;
            }
            
            _slots[index].hashCode = hashCode;
            _slots[index].value = value;
            _slots[index].next = _buckets[bucket] - 1;
            _buckets[bucket] = index + 1;
            _count++;
            _version++;
            return true;
        }
        
        private void Initialize(int capacity)
        {
            int size = GetPrime(capacity);
            _buckets = new int[size];
            _slots = new Slot[size];
        }
        
        private void IncreaseCapacity()
        {
            int newSize = GetPrime(_count * 2);
            Slot[] newSlots = new Slot[newSize];
            if (_slots != null)
            {
                Array.Copy(_slots, 0, newSlots, 0, _lastIndex);
            }
            
            int[] newBuckets = new int[newSize];
            for (int i = 0; i < _lastIndex; i++)
            {
                int bucket = newSlots[i].hashCode % newSize;
                newSlots[i].next = newBuckets[bucket] - 1;
                newBuckets[bucket] = i + 1;
            }
            _slots = newSlots;
            _buckets = newBuckets;
        }
        
        private int InternalGetHashCode(T item)
        {
            if (item == null) return 0;
            return _comparer.GetHashCode(item) & 0x7FFFFFFF;
        }
        
        private static readonly int[] primes = {
            3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89, 107, 131, 163, 197, 239, 293, 353, 431, 521
        };
        
        private static int GetPrime(int min)
        {
            for (int i = 0; i < primes.Length; i++)
            {
                if (primes[i] >= min) return primes[i];
            }
            return min | 1;
        }
        
        public Enumerator GetEnumerator() => new Enumerator(this);
        IEnumerator<T> IEnumerable<T>.GetEnumerator() => new Enumerator(this);
        IEnumerator IEnumerable.GetEnumerator() => new Enumerator(this);
        
        public struct Enumerator : IEnumerator<T>
        {
            private HashSet<T> _set;
            private int _index;
            private int _version;
            private T _current;
            
            internal Enumerator(HashSet<T> set)
            {
                _set = set;
                _index = 0;
                _version = set._version;
                _current = default;
            }
            
            public bool MoveNext()
            {
                if (_version != _set._version) throw new InvalidOperationException("Collection modified");
                
                while (_index < _set._lastIndex)
                {
                    if (_set._slots[_index].hashCode >= 0)
                    {
                        _current = _set._slots[_index].value;
                        _index++;
                        return true;
                    }
                    _index++;
                }
                _current = default;
                return false;
            }
            
            public T Current => _current;
            object IEnumerator.Current => _current;
            public void Dispose() { }
            void IEnumerator.Reset() { _index = 0; _current = default; }
        }
    }
    
    /// <summary>
    /// ISet interface for set operations
    /// </summary>
    public interface ISet<T> : ICollection<T>
    {
        bool Add(T item);
        void ExceptWith(IEnumerable<T> other);
        void IntersectWith(IEnumerable<T> other);
        bool IsProperSubsetOf(IEnumerable<T> other);
        bool IsProperSupersetOf(IEnumerable<T> other);
        bool IsSubsetOf(IEnumerable<T> other);
        bool IsSupersetOf(IEnumerable<T> other);
        bool Overlaps(IEnumerable<T> other);
        bool SetEquals(IEnumerable<T> other);
        void SymmetricExceptWith(IEnumerable<T> other);
        void UnionWith(IEnumerable<T> other);
    }
}
