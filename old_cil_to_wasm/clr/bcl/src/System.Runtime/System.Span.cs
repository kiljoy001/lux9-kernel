namespace System
{
    /// <summary>
    /// Span - stack-only type representing a contiguous region of memory.
    /// This implementation stores array reference with offset/length for slicing.
    /// </summary>
    public readonly ref struct Span<T>
    {
        private readonly T[] _array;
        private readonly int _start;
        private readonly int _length;
        
        public Span(T[] array)
        {
            _array = array;
            _start = 0;
            _length = array != null ? array.Length : 0;
        }
        
        public Span(T[] array, int start, int length)
        {
            if (array == null)
            {
                if (start != 0 || length != 0)
                    throw new ArgumentOutOfRangeException();
                _array = null;
                _start = 0;
                _length = 0;
            }
            else
            {
                if ((uint)start > (uint)array.Length || (uint)length > (uint)(array.Length - start))
                    throw new ArgumentOutOfRangeException();
                _array = array;
                _start = start;
                _length = length;
            }
        }
        
        // Internal constructor for slicing
        private Span(T[] array, int start, int length, bool noCheck)
        {
            _array = array;
            _start = start;
            _length = length;
        }
        
        public int Length => _length;
        public bool IsEmpty => _length == 0;
        
        public ref T this[int index]
        {
            get
            {
                if ((uint)index >= (uint)_length)
                    throw new IndexOutOfRangeException();
                return ref _array[_start + index];
            }
        }
        
        public Span<T> Slice(int start)
        {
            if ((uint)start > (uint)_length)
                throw new ArgumentOutOfRangeException();
            return new Span<T>(_array, _start + start, _length - start, true);
        }
        
        public Span<T> Slice(int start, int length)
        {
            if ((uint)start > (uint)_length || (uint)length > (uint)(_length - start))
                throw new ArgumentOutOfRangeException();
            return new Span<T>(_array, _start + start, length, true);
        }
        
        public void Clear()
        {
            if (_array != null && _length > 0)
            {
                for (int i = 0; i < _length; i++)
                    _array[_start + i] = default(T);
            }
        }
        
        public void Fill(T value)
        {
            if (_array != null)
            {
                for (int i = 0; i < _length; i++)
                    _array[_start + i] = value;
            }
        }
        
        public void CopyTo(Span<T> destination)
        {
            if (_length > destination._length)
                throw new ArgumentException("Destination too short");
            for (int i = 0; i < _length; i++)
                destination[i] = this[i];
        }
        
        public T[] ToArray()
        {
            if (_length == 0) return new T[0];
            T[] result = new T[_length];
            for (int i = 0; i < _length; i++)
                result[i] = _array[_start + i];
            return result;
        }
        
        public static implicit operator Span<T>(T[] array) => new Span<T>(array);
        public static implicit operator ReadOnlySpan<T>(Span<T> span) => 
            new ReadOnlySpan<T>(span._array, span._start, span._length);
    }
    
    /// <summary>
    /// ReadOnlySpan - immutable version of Span.
    /// </summary>
    public readonly ref struct ReadOnlySpan<T>
    {
        private readonly T[] _array;
        private readonly int _start;
        private readonly int _length;
        
        public ReadOnlySpan(T[] array)
        {
            _array = array;
            _start = 0;
            _length = array != null ? array.Length : 0;
        }
        
        public ReadOnlySpan(T[] array, int start, int length)
        {
            if (array == null)
            {
                if (start != 0 || length != 0)
                    throw new ArgumentOutOfRangeException();
                _array = null;
                _start = 0;
                _length = 0;
            }
            else
            {
                if ((uint)start > (uint)array.Length || (uint)length > (uint)(array.Length - start))
                    throw new ArgumentOutOfRangeException();
                _array = array;
                _start = start;
                _length = length;
            }
        }
        
        internal ReadOnlySpan(T[] array, int start, int length, bool noCheck)
        {
            _array = array;
            _start = start;
            _length = length;
        }
        
        public int Length => _length;
        public bool IsEmpty => _length == 0;
        
        public T this[int index]
        {
            get
            {
                if ((uint)index >= (uint)_length)
                    throw new IndexOutOfRangeException();
                return _array[_start + index];
            }
        }
        
        public ReadOnlySpan<T> Slice(int start)
        {
            if ((uint)start > (uint)_length)
                throw new ArgumentOutOfRangeException();
            return new ReadOnlySpan<T>(_array, _start + start, _length - start, true);
        }
        
        public ReadOnlySpan<T> Slice(int start, int length)
        {
            if ((uint)start > (uint)_length || (uint)length > (uint)(_length - start))
                throw new ArgumentOutOfRangeException();
            return new ReadOnlySpan<T>(_array, _start + start, length, true);
        }
        
        public void CopyTo(Span<T> destination)
        {
            if (_length > destination.Length)
                throw new ArgumentException("Destination too short");
            for (int i = 0; i < _length; i++)
                destination[i] = _array[_start + i];
        }
        
        public T[] ToArray()
        {
            if (_length == 0) return new T[0];
            T[] result = new T[_length];
            for (int i = 0; i < _length; i++)
                result[i] = _array[_start + i];
            return result;
        }
        
        public static implicit operator ReadOnlySpan<T>(T[] array) => new ReadOnlySpan<T>(array);
    }
    
    /// <summary>
    /// Memory - heap-allocated equivalent of Span.
    /// </summary>
    public readonly struct Memory<T>
    {
        private readonly T[] _array;
        private readonly int _start;
        private readonly int _length;
        
        public Memory(T[] array)
        {
            _array = array;
            _start = 0;
            _length = array != null ? array.Length : 0;
        }
        
        public Memory(T[] array, int start, int length)
        {
            if (array == null)
            {
                if (start != 0 || length != 0)
                    throw new ArgumentOutOfRangeException();
                _array = null;
                _start = 0;
                _length = 0;
            }
            else
            {
                if ((uint)start > (uint)array.Length || (uint)length > (uint)(array.Length - start))
                    throw new ArgumentOutOfRangeException();
                _array = array;
                _start = start;
                _length = length;
            }
        }
        
        public int Length => _length;
        public bool IsEmpty => _length == 0;
        
        public Span<T> Span => new Span<T>(_array, _start, _length);
        
        public Memory<T> Slice(int start)
        {
            if ((uint)start > (uint)_length)
                throw new ArgumentOutOfRangeException();
            return new Memory<T>(_array, _start + start, _length - start);
        }
        
        public Memory<T> Slice(int start, int length)
        {
            if ((uint)start > (uint)_length || (uint)length > (uint)(_length - start))
                throw new ArgumentOutOfRangeException();
            return new Memory<T>(_array, _start + start, length);
        }
        
        public T[] ToArray()
        {
            Span<T> span = Span;
            return span.ToArray();
        }
        
        public static implicit operator Memory<T>(T[] array) => new Memory<T>(array);
        public static implicit operator ReadOnlyMemory<T>(Memory<T> memory) => 
            new ReadOnlyMemory<T>(memory._array, memory._start, memory._length);
    }
    
    /// <summary>
    /// ReadOnlyMemory - immutable version of Memory.
    /// </summary>
    public readonly struct ReadOnlyMemory<T>
    {
        private readonly T[] _array;
        private readonly int _start;
        private readonly int _length;
        
        public ReadOnlyMemory(T[] array)
        {
            _array = array;
            _start = 0;
            _length = array != null ? array.Length : 0;
        }
        
        public ReadOnlyMemory(T[] array, int start, int length)
        {
            if (array == null)
            {
                if (start != 0 || length != 0)
                    throw new ArgumentOutOfRangeException();
                _array = null;
                _start = 0;
                _length = 0;
            }
            else
            {
                if ((uint)start > (uint)array.Length || (uint)length > (uint)(array.Length - start))
                    throw new ArgumentOutOfRangeException();
                _array = array;
                _start = start;
                _length = length;
            }
        }
        
        public int Length => _length;
        public bool IsEmpty => _length == 0;
        
        public ReadOnlySpan<T> Span => new ReadOnlySpan<T>(_array, _start, _length, true);
        
        public ReadOnlyMemory<T> Slice(int start)
        {
            if ((uint)start > (uint)_length)
                throw new ArgumentOutOfRangeException();
            return new ReadOnlyMemory<T>(_array, _start + start, _length - start);
        }
        
        public ReadOnlyMemory<T> Slice(int start, int length)
        {
            if ((uint)start > (uint)_length || (uint)length > (uint)(_length - start))
                throw new ArgumentOutOfRangeException();
            return new ReadOnlyMemory<T>(_array, _start + start, length);
        }
        
        public T[] ToArray()
        {
            ReadOnlySpan<T> span = Span;
            return span.ToArray();
        }
        
        public static implicit operator ReadOnlyMemory<T>(T[] array) => new ReadOnlyMemory<T>(array);
    }
}
