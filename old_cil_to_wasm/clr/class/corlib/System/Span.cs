/*
 * ECMA-335 Span Types - Lux9 BCL
 * Simplified implementation without ref fields for compatibility
 */
namespace System
{
    // Span<T> - simplified array-based implementation
    public readonly struct Span<T>
    {
        private readonly T[] _array;
        private readonly int _start;
        private readonly int _length;

        public Span(T[] array)
        {
            _array = array;
            _start = 0;
            _length = array?.Length ?? 0;
        }
        public Span(T[] array, int start, int length)
        {
            _array = array;
            _start = start;
            _length = length;
        }

        public int Length => _length;
        public bool IsEmpty => _length == 0;
        public static Span<T> Empty => default;

        public ref T this[int index]
        {
            get
            {
                if ((uint)index >= (uint)_length) throw new IndexOutOfRangeException();
                return ref _array[_start + index];
            }
        }

        public Span<T> Slice(int start) => Slice(start, _length - start);
        public Span<T> Slice(int start, int length)
        {
            if ((uint)start > (uint)_length || (uint)length > (uint)(_length - start))
                throw new ArgumentOutOfRangeException();
            return new Span<T>(_array, _start + start, length);
        }

        public T[] ToArray()
        {
            if (_length == 0) return Array.Empty<T>();
            T[] array = new T[_length];
            for (int i = 0; i < _length; i++) array[i] = _array[_start + i];
            return array;
        }

        public void CopyTo(Span<T> destination)
        {
            for (int i = 0; i < _length; i++) destination[i] = this[i];
        }

        public void Clear()
        {
            for (int i = 0; i < _length; i++) _array[_start + i] = default;
        }

        public void Fill(T value)
        {
            for (int i = 0; i < _length; i++) _array[_start + i] = value;
        }

        public static implicit operator Span<T>(T[] array) => new Span<T>(array);
        public static implicit operator ReadOnlySpan<T>(Span<T> span) => new ReadOnlySpan<T>(span._array, span._start, span._length);
    }

    // ReadOnlySpan<T> - simplified array-based implementation
    public readonly struct ReadOnlySpan<T>
    {
        private readonly T[] _array;
        private readonly int _start;
        private readonly int _length;

        public ReadOnlySpan(T[] array)
        {
            _array = array;
            _start = 0;
            _length = array?.Length ?? 0;
        }
        public ReadOnlySpan(T[] array, int start, int length)
        {
            _array = array;
            _start = start;
            _length = length;
        }

        public int Length => _length;
        public bool IsEmpty => _length == 0;
        public static ReadOnlySpan<T> Empty => default;

        public T this[int index]
        {
            get
            {
                if ((uint)index >= (uint)_length) throw new IndexOutOfRangeException();
                return _array[_start + index];
            }
        }

        public ReadOnlySpan<T> Slice(int start) => Slice(start, _length - start);
        public ReadOnlySpan<T> Slice(int start, int length)
        {
            if ((uint)start > (uint)_length || (uint)length > (uint)(_length - start))
                throw new ArgumentOutOfRangeException();
            return new ReadOnlySpan<T>(_array, _start + start, length);
        }

        public T[] ToArray()
        {
            if (_length == 0) return Array.Empty<T>();
            T[] array = new T[_length];
            for (int i = 0; i < _length; i++) array[i] = _array[_start + i];
            return array;
        }

        public void CopyTo(Span<T> destination)
        {
            for (int i = 0; i < _length; i++) destination[i] = this[i];
        }

        public static implicit operator ReadOnlySpan<T>(T[] array) => new ReadOnlySpan<T>(array);
    }

    // ArraySegment<T>
    public readonly struct ArraySegment<T> : Collections.Generic.IList<T>
    {
        private readonly T[] _array;
        private readonly int _offset;
        private readonly int _count;

        public ArraySegment(T[] array) : this(array, 0, array?.Length ?? 0) { }
        public ArraySegment(T[] array, int offset, int count)
        {
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

        public static ArraySegment<T> Empty => new ArraySegment<T>(System.Array.Empty<T>());

        // IList<T>
        int Collections.Generic.ICollection<T>.Count => _count;
        bool Collections.Generic.ICollection<T>.IsReadOnly => true;
        void Collections.Generic.ICollection<T>.Add(T item) => throw new NotSupportedException();
        void Collections.Generic.ICollection<T>.Clear() => throw new NotSupportedException();
        bool Collections.Generic.ICollection<T>.Contains(T item) => System.Array.IndexOf(_array, item, _offset, _count) >= 0;
        void Collections.Generic.ICollection<T>.CopyTo(T[] array, int arrayIndex) => System.Array.Copy(_array, _offset, array, arrayIndex, _count);
        bool Collections.Generic.ICollection<T>.Remove(T item) => throw new NotSupportedException();
        int Collections.Generic.IList<T>.IndexOf(T item)
        {
            int idx = System.Array.IndexOf(_array, item, _offset, _count);
            return idx >= 0 ? idx - _offset : -1;
        }
        void Collections.Generic.IList<T>.Insert(int index, T item) => throw new NotSupportedException();
        void Collections.Generic.IList<T>.RemoveAt(int index) => throw new NotSupportedException();
        public Collections.Generic.IEnumerator<T> GetEnumerator()
        {
            for (int i = _offset; i < _offset + _count; i++) yield return _array[i];
        }
        Collections.IEnumerator Collections.IEnumerable.GetEnumerator() => GetEnumerator();
    }
}
