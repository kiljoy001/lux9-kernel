/*
 * ECMA-335 Generic Collections - Lux9 BCL
 * 
 * Generic collection implementations per CLI specification.
 */
namespace System.Collections.Generic
{
    using System;
    using System.Collections;

    // List<T> (ECMA-335)
    public class List<T> : IList<T>, IList, IReadOnlyList<T>
    {
        private T[] _items;
        private int _size;
        private int _version;

        private const int DefaultCapacity = 4;

        public List() { _items = Array.Empty<T>(); }
        public List(int capacity) { _items = capacity == 0 ? Array.Empty<T>() : new T[capacity]; }
        public List(IEnumerable<T> collection)
        {
            if (collection == null) throw new ArgumentNullException(nameof(collection));
            if (collection is ICollection<T> c)
            {
                _items = new T[c.Count];
                c.CopyTo(_items, 0);
                _size = c.Count;
            }
            else
            {
                _items = Array.Empty<T>();
                foreach (T item in collection) Add(item);
            }
        }

        public int Count => _size;
        public int Capacity
        {
            get => _items.Length;
            set
            {
                if (value < _size) throw new ArgumentOutOfRangeException(nameof(value));
                if (value != _items.Length)
                {
                    T[] newItems = new T[value];
                    if (_size > 0) Array.Copy(_items, newItems, _size);
                    _items = newItems;
                }
            }
        }

        public T this[int index]
        {
            get
            {
                if ((uint)index >= (uint)_size) throw new ArgumentOutOfRangeException();
                return _items[index];
            }
            set
            {
                if ((uint)index >= (uint)_size) throw new ArgumentOutOfRangeException();
                _items[index] = value;
                _version++;
            }
        }

        public void Add(T item)
        {
            if (_size == _items.Length) EnsureCapacity(_size + 1);
            _items[_size++] = item;
            _version++;
        }

        public void AddRange(IEnumerable<T> collection)
        {
            if (collection == null) throw new ArgumentNullException(nameof(collection));
            foreach (T item in collection) Add(item);
        }

        public void Insert(int index, T item)
        {
            if ((uint)index > (uint)_size) throw new ArgumentOutOfRangeException();
            if (_size == _items.Length) EnsureCapacity(_size + 1);
            if (index < _size) Array.Copy(_items, index, _items, index + 1, _size - index);
            _items[index] = item;
            _size++;
            _version++;
        }

        public bool Remove(T item)
        {
            int index = IndexOf(item);
            if (index >= 0) { RemoveAt(index); return true; }
            return false;
        }

        public void RemoveAt(int index)
        {
            if ((uint)index >= (uint)_size) throw new ArgumentOutOfRangeException();
            _size--;
            if (index < _size) Array.Copy(_items, index + 1, _items, index, _size - index);
            _items[_size] = default;
            _version++;
        }

        public void RemoveRange(int index, int count)
        {
            if (index < 0 || count < 0 || _size - index < count) throw new ArgumentOutOfRangeException();
            if (count > 0)
            {
                _size -= count;
                if (index < _size) Array.Copy(_items, index + count, _items, index, _size - index);
                Array.Clear(_items, _size, count);
                _version++;
            }
        }

        public void Clear()
        {
            if (_size > 0)
            {
                Array.Clear(_items, 0, _size);
                _size = 0;
            }
            _version++;
        }

        public bool Contains(T item) => IndexOf(item) >= 0;

        public int IndexOf(T item) => Array.IndexOf(_items, item, 0, _size);

        public int LastIndexOf(T item)
        {
            if (_size == 0) return -1;
            return Array.LastIndexOf(_items, item, _size - 1, _size);
        }

        public void CopyTo(T[] array, int arrayIndex) => Array.Copy(_items, 0, array, arrayIndex, _size);

        public T[] ToArray()
        {
            if (_size == 0) return Array.Empty<T>();
            T[] array = new T[_size];
            Array.Copy(_items, array, _size);
            return array;
        }

        public void Sort() => Sort(0, _size, null);
        public void Sort(IComparer<T> comparer) => Sort(0, _size, comparer);
        public void Sort(int index, int count, IComparer<T> comparer)
        {
            if (index < 0 || count < 0 || _size - index < count) throw new ArgumentOutOfRangeException();
            Array.Sort(_items, index, count, comparer);
            _version++;
        }
        public void Sort(Comparison<T> comparison)
        {
            if (comparison == null) throw new ArgumentNullException(nameof(comparison));
            Array.Sort(_items, comparison);
            _version++;
        }

        public void Reverse() => Reverse(0, _size);
        public void Reverse(int index, int count)
        {
            if (index < 0 || count < 0 || _size - index < count) throw new ArgumentOutOfRangeException();
            Array.Reverse(_items, index, count);
            _version++;
        }

        public int BinarySearch(T item) => BinarySearch(0, _size, item, null);
        public int BinarySearch(T item, IComparer<T> comparer) => BinarySearch(0, _size, item, comparer);
        public int BinarySearch(int index, int count, T item, IComparer<T> comparer)
        {
            if (index < 0 || count < 0 || _size - index < count) throw new ArgumentOutOfRangeException();
            // Manual binary search to avoid Array.BinarySearch IComparer issue
            int lo = index;
            int hi = index + count - 1;
            IComparer<T> cmp = comparer ?? Comparer<T>.Default;
            while (lo <= hi)
            {
                int mid = lo + ((hi - lo) >> 1);
                int result = cmp.Compare(_items[mid], item);
                if (result == 0) return mid;
                if (result < 0) lo = mid + 1;
                else hi = mid - 1;
            }
            return ~lo;
        }

        public bool Exists(Predicate<T> match) => FindIndex(match) >= 0;
        public T Find(Predicate<T> match) => Array.Find(_items, match);
        public List<T> FindAll(Predicate<T> match)
        {
            List<T> list = new List<T>();
            for (int i = 0; i < _size; i++)
                if (match(_items[i])) list.Add(_items[i]);
            return list;
        }
        public int FindIndex(Predicate<T> match) => FindIndex(0, _size, match);
        public int FindIndex(int startIndex, Predicate<T> match) => FindIndex(startIndex, _size - startIndex, match);
        public int FindIndex(int startIndex, int count, Predicate<T> match)
        {
            if ((uint)startIndex > (uint)_size) throw new ArgumentOutOfRangeException();
            if (count < 0 || startIndex > _size - count) throw new ArgumentOutOfRangeException();
            return Array.FindIndex(_items, startIndex, count, match);
        }

        public void ForEach(Action<T> action)
        {
            if (action == null) throw new ArgumentNullException(nameof(action));
            for (int i = 0; i < _size; i++) action(_items[i]);
        }

        public List<TOutput> ConvertAll<TOutput>(Converter<T, TOutput> converter)
        {
            if (converter == null) throw new ArgumentNullException(nameof(converter));
            List<TOutput> list = new List<TOutput>(_size);
            for (int i = 0; i < _size; i++) list.Add(converter(_items[i]));
            return list;
        }

        public bool TrueForAll(Predicate<T> match) => Array.TrueForAll(_items, match);

        private void EnsureCapacity(int min)
        {
            if (_items.Length < min)
            {
                int newCapacity = _items.Length == 0 ? DefaultCapacity : _items.Length * 2;
                if (newCapacity < min) newCapacity = min;
                Capacity = newCapacity;
            }
        }

        public Enumerator GetEnumerator() => new Enumerator(this);
        IEnumerator<T> IEnumerable<T>.GetEnumerator() => GetEnumerator();
        IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();

        public struct Enumerator : IEnumerator<T>
        {
            private List<T> _list;
            private int _index;
            private int _version;
            private T _current;

            internal Enumerator(List<T> list)
            {
                _list = list;
                _index = 0;
                _version = list._version;
                _current = default;
            }

            public T Current => _current;
            Object IEnumerator.Current => Current;

            public bool MoveNext()
            {
                if (_version != _list._version) throw new InvalidOperationException();
                if (_index < _list._size)
                {
                    _current = _list._items[_index];
                    _index++;
                    return true;
                }
                _index = _list._size + 1;
                _current = default;
                return false;
            }

            public void Reset()
            {
                if (_version != _list._version) throw new InvalidOperationException();
                _index = 0;
                _current = default;
            }

            public void Dispose() { }
        }

        // IList implementation
        bool ICollection<T>.IsReadOnly => false;
        bool IList.IsFixedSize => false;
        bool IList.IsReadOnly => false;
        bool ICollection.IsSynchronized => false;
        Object ICollection.SyncRoot => this;
        Object IList.this[int index] { get => this[index]; set => this[index] = (T)value; }
        int IList.Add(Object value) { Add((T)value); return _size - 1; }
        bool IList.Contains(Object value) => value is T t && Contains(t);
        int IList.IndexOf(Object value) => value is T t ? IndexOf(t) : -1;
        void IList.Insert(int index, Object value) => Insert(index, (T)value);
        void IList.Remove(Object value) { if (value is T t) Remove(t); }
        void ICollection.CopyTo(Array array, int index) => Array.Copy(_items, 0, array, index, _size);
    }

    // IReadOnlyList<T>
    public interface IReadOnlyList<out T> : IReadOnlyCollection<T>
    {
        T this[int index] { get; }
    }

    // IReadOnlyCollection<T>
    public interface IReadOnlyCollection<out T> : IEnumerable<T>
    {
        int Count { get; }
    }

    // IReadOnlyDictionary<TKey, TValue>
    public interface IReadOnlyDictionary<TKey, TValue> : IReadOnlyCollection<KeyValuePair<TKey, TValue>>
    {
        TValue this[TKey key] { get; }
        IEnumerable<TKey> Keys { get; }
        IEnumerable<TValue> Values { get; }
        bool ContainsKey(TKey key);
        bool TryGetValue(TKey key, out TValue value);
    }

    // Comparer<T> (ECMA-335)
    public abstract class Comparer<T> : IComparer<T>, IComparer
    {
        private static Comparer<T> _default;
        public static Comparer<T> Default => _default ??= new DefaultComparer();

        public abstract int Compare(T x, T y);
        int IComparer.Compare(Object x, Object y) => Compare((T)x, (T)y);

        public static Comparer<T> Create(Comparison<T> comparison) => new ComparisonComparer(comparison);

        private sealed class DefaultComparer : Comparer<T>
        {
            public override int Compare(T x, T y)
            {
                if (x is IComparable<T> c) return c.CompareTo(y);
                if (x is IComparable c2) return c2.CompareTo(y);
                return 0;
            }
        }

        private sealed class ComparisonComparer : Comparer<T>
        {
            private Comparison<T> _comparison;
            public ComparisonComparer(Comparison<T> comparison) { _comparison = comparison; }
            public override int Compare(T x, T y) => _comparison(x, y);
        }
    }

    // EqualityComparer<T> (ECMA-335)
    public abstract class EqualityComparer<T> : IEqualityComparer<T>, IEqualityComparer
    {
        private static EqualityComparer<T> _default;
        public static EqualityComparer<T> Default => _default ??= new DefaultEqualityComparer();

        public abstract bool Equals(T x, T y);
        public abstract int GetHashCode(T obj);
        bool IEqualityComparer.Equals(Object x, Object y) => Equals((T)x, (T)y);
        int IEqualityComparer.GetHashCode(Object obj) => GetHashCode((T)obj);

        private sealed class DefaultEqualityComparer : EqualityComparer<T>
        {
            public override bool Equals(T x, T y)
            {
                if (x is IEquatable<T> e) return e.Equals(y);
                return Object.Equals(x, y);
            }
            public override int GetHashCode(T obj) => obj?.GetHashCode() ?? 0;
        }
    }
}

namespace System.Collections.ObjectModel
{
    using System.Collections.Generic;

    // ReadOnlyCollection<T> (ECMA-335)
    public class ReadOnlyCollection<T> : IList<T>, IList, IReadOnlyList<T>
    {
        private IList<T> _list;

        public ReadOnlyCollection(IList<T> list)
        {
            _list = list ?? throw new ArgumentNullException(nameof(list));
        }

        public int Count => _list.Count;
        public T this[int index] => _list[index];
        T IList<T>.this[int index] { get => _list[index]; set => throw new NotSupportedException(); }
        public bool Contains(T item) => _list.Contains(item);
        public int IndexOf(T item) => _list.IndexOf(item);
        public void CopyTo(T[] array, int index) => _list.CopyTo(array, index);
        public IEnumerator<T> GetEnumerator() => _list.GetEnumerator();
        IEnumerator IEnumerable.GetEnumerator() => _list.GetEnumerator();

        bool ICollection<T>.IsReadOnly => true;
        void ICollection<T>.Add(T item) => throw new NotSupportedException();
        void ICollection<T>.Clear() => throw new NotSupportedException();
        bool ICollection<T>.Remove(T item) => throw new NotSupportedException();
        void IList<T>.Insert(int index, T item) => throw new NotSupportedException();
        void IList<T>.RemoveAt(int index) => throw new NotSupportedException();

        bool IList.IsFixedSize => true;
        bool IList.IsReadOnly => true;
        bool ICollection.IsSynchronized => false;
        Object ICollection.SyncRoot => _list is ICollection c ? c.SyncRoot : this;
        Object IList.this[int index] { get => _list[index]; set => throw new NotSupportedException(); }
        int IList.Add(Object value) => throw new NotSupportedException();
        void IList.Clear() => throw new NotSupportedException();
        bool IList.Contains(Object value) => value is T t && Contains(t);
        int IList.IndexOf(Object value) => value is T t ? IndexOf(t) : -1;
        void IList.Insert(int index, Object value) => throw new NotSupportedException();
        void IList.Remove(Object value) => throw new NotSupportedException();
        void IList.RemoveAt(int index) => throw new NotSupportedException();
        void ICollection.CopyTo(Array array, int index) => ((ICollection)_list).CopyTo(array, index);
    }
}
