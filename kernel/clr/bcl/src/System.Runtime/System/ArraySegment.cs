namespace System
{
    using System.Collections.Generic;

    public struct ArraySegment<T> : IList<T>, IReadOnlyList<T>
    {
        private T[] _array;
        private int _offset;
        private int _count;

        public ArraySegment(T[] array)
        {
            if (array == null) throw new ArgumentNullException("array");
            _array = array;
            _offset = 0;
            _count = array.Length;
        }

        public ArraySegment(T[] array, int offset, int count)
        {
             if (array == null) throw new ArgumentNullException("array");
             _array = array;
             _offset = offset;
             _count = count;
        }

        public T[] Array => _array;
        public int Offset => _offset;
        public int Count => _count;

        public T this[int index]
        {
            get => _array[_offset + index];
            set => _array[_offset + index] = value;
        }

        public Enumerator GetEnumerator() => new Enumerator(this);
        IEnumerator<T> IEnumerable<T>.GetEnumerator() => new Enumerator(this);
        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator() => new Enumerator(this);

        public struct Enumerator : IEnumerator<T>
        {
            private T[] _array;
            private int _start;
            private int _end;
            private int _current;

            internal Enumerator(ArraySegment<T> segment)
            {
                _array = segment._array;
                _start = segment._offset;
                _end = segment._offset + segment._count;
                _current = _start - 1;
            }

            public bool MoveNext()
            {
                if (_current < _end - 1)
                {
                    _current++;
                    return true;
                }
                _current = _end;
                return false;
            }

            public T Current
            {
                get
                {
                    if (_current < _start) throw new InvalidOperationException();
                    if (_current >= _end) throw new InvalidOperationException();
                    return _array[_current];
                }
            }

            object System.Collections.IEnumerator.Current => Current;

            public void Reset()
            {
                _current = _start - 1;
            }

            public void Dispose()
            {
            }
        }

        // Stub other interfaces
        public int IndexOf(T item) => -1;
        public void Insert(int index, T item) { }
        public void RemoveAt(int index) { }
        public void Add(T item) { }
        public void Clear() { }
        public bool Contains(T item) => false;
        public void CopyTo(T[] array, int arrayIndex) { }
        public bool Remove(T item) => false;
        public bool IsReadOnly => true;
    }
}
