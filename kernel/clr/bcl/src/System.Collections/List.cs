namespace System.Collections.Generic
{
    using System;
    using System.Collections;

    public class List<T> : IList<T>, IList, IReadOnlyList<T>
    {
        private T[] _items;
        private int _size;
        private int _version;
        private static readonly T[] _emptyArray = new T[0];

        public List()
        {
            _items = _emptyArray;
        }

        public List(int capacity)
        {
            if (capacity < 0) throw new ArgumentOutOfRangeException(nameof(capacity));
            if (capacity == 0)
                _items = _emptyArray;
            else
                _items = new T[capacity];
        }

        public int Count => _size;

        public bool IsReadOnly => false;

        bool IList.IsFixedSize => false;
        bool IList.IsReadOnly => false;
        object IList.this[int index]
        {
            get => this[index];
            set => this[index] = (T)value;
        }

        public T this[int index]
        {
            get
            {
                if ((uint)index >= (uint)_size) throw new ArgumentOutOfRangeException(nameof(index));
                return _items[index];
            }
            set
            {
                if ((uint)index >= (uint)_size) throw new ArgumentOutOfRangeException(nameof(index));
                _items[index] = value;
                _version++;
            }
        }

        public void Add(T item)
        {
            if (_size == _items.Length)
                EnsureCapacity(_size + 1);
            _items[_size++] = item;
            _version++;
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

        public bool Contains(T item)
        {
            return IndexOf(item) != -1;
        }

        public void CopyTo(T[] array, int arrayIndex)
        {
            Array.Copy(_items, 0, array, arrayIndex, _size);
        }

        bool ICollection<T>.Remove(T item)
        {
            int index = IndexOf(item);
            if (index >= 0)
            {
                RemoveAt(index);
                return true;
            }
            return false;
        }

        public int IndexOf(T item)
        {
            return Array.IndexOf(_items, item, 0, _size);
        }

        public void Insert(int index, T item)
        {
            if ((uint)index > (uint)_size) throw new ArgumentOutOfRangeException(nameof(index));
            if (_size == _items.Length) EnsureCapacity(_size + 1);
            if (index < _size)
            {
                Array.Copy(_items, index, _items, index + 1, _size - index);
            }
            _items[index] = item;
            _size++;
            _version++;
        }

        public void RemoveAt(int index)
        {
             if ((uint)index >= (uint)_size) throw new ArgumentOutOfRangeException(nameof(index));
             _size--;
             if (index < _size)
             {
                 Array.Copy(_items, index + 1, _items, index, _size - index);
             }
             _items[_size] = default(T);
             _version++;
        }

        private void EnsureCapacity(int min)
        {
            if (_items.Length < min)
            {
                int newCapacity = _items.Length == 0 ? 4 : _items.Length * 2;
                if (newCapacity < min) newCapacity = min;
                Capacity = newCapacity;
            }
        }

        public int Capacity
        {
            get => _items.Length;
            set
            {
                if (value < _size) throw new ArgumentOutOfRangeException(nameof(value));
                if (value != _items.Length)
                {
                    if (value > 0)
                    {
                        T[] newItems = new T[value];
                        if (_size > 0)
                        {
                            Array.Copy(_items, 0, newItems, 0, _size);
                        }
                        _items = newItems;
                    }
                    else
                    {
                        _items = _emptyArray;
                    }
                }
            }
        }

        public Enumerator GetEnumerator() => new Enumerator(this);

        IEnumerator<T> IEnumerable<T>.GetEnumerator() => new Enumerator(this);

        IEnumerator IEnumerable.GetEnumerator() => new Enumerator(this);

        // Enumerator Struct
        public struct Enumerator : IEnumerator<T>, IEnumerator
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
                _current = default(T);
            }

            public void Dispose() {}

            public bool MoveNext()
            {
                List<T> localList = _list;
                if (_version == localList._version && ((uint)_index < (uint)localList._size))
                {
                    _current = localList._items[_index];
                    _index++;
                    return true;
                }
                return MoveNextRare();
            }

            private bool MoveNextRare()
            {
                if (_version != _list._version) throw new InvalidOperationException("Collection modified");
                _index = _list._size + 1;
                _current = default(T);
                return false;
            }

            public T Current => _current;

            object IEnumerator.Current
            {
                get
                {
                    if (_index == 0 || _index == _list._size + 1)
                        throw new InvalidOperationException("Enumeration unstarted or finished");
                    return Current;
                }
            }

            void IEnumerator.Reset()
            {
                 if (_version != _list._version) throw new InvalidOperationException("Collection modified");
                 _index = 0;
                 _current = default(T);
            }
        }
        
        // IList/ICollection Non-generic stubs omitted for brevity but required?
        // Implementing implicitly via interface methods above
        int IList.Add(object value) { Add((T)value); return Count - 1; }
        bool IList.Contains(object value) { return IsCompatibleObject(value) && Contains((T)value); }
        void IList.Clear() { Clear(); }
        int IList.IndexOf(object value) { return IsCompatibleObject(value) ? IndexOf((T)value) : -1; }
        void IList.Insert(int index, object value) { Insert(index, (T)value); }
        void IList.Remove(object value) { if (IsCompatibleObject(value)) Remove((T)value); }
        void IList.RemoveAt(int index) { RemoveAt(index); }
        void ICollection.CopyTo(Array array, int index) { 
             if ((array != null) && (array.Rank != 1)) throw new ArgumentException("Multi-dim");
             // naive untyped copy
             try { Array.Copy(_items, 0, array, index, _size); }
             catch(ArrayTypeMismatchException) { throw new ArgumentException("Type mismatch"); }
        }
        object ICollection.SyncRoot => this;
        bool ICollection.IsSynchronized => false;

        private static bool IsCompatibleObject(object value) 
        {
            return (value is T) || (value == null && default(T) == null);
        }
    }
}
