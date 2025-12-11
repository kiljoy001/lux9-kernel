namespace System
{
    /// <summary>
    /// Span - stack-only type representing a contiguous region of memory.
    /// NOTE: This is a simplified stub implementation. Full Span requires runtime intrinsics support.
    /// </summary>
    public readonly ref struct Span<T>
    {
        // Note: Real Span uses ref fields which require special runtime support
        // For now we stub with a minimal implementation
        
        private readonly int _length;
        
        public Span(T[] array)
        {
            _length = array != null ? array.Length : 0;
        }
        
        public int Length => _length;
        
        // Simplified indexer - normally returns ref T but that requires runtime support
        // public ref T this[int index] => throw new NotImplementedException();
        
        public Span<T> Slice(int start) => throw new NotImplementedException();
        public Span<T> Slice(int start, int length) => throw new NotImplementedException();
        public void Clear() => throw new NotImplementedException();
    }
    
    /// <summary>
    /// ReadOnlySpan - immutable version of Span.
    /// </summary>
    public readonly ref struct ReadOnlySpan<T>
    {
        private readonly int _length;
        
        public ReadOnlySpan(T[] array)
        {
            _length = array != null ? array.Length : 0;
        }
        
        public int Length => _length;
        
        // Simplified indexer
        // public ref readonly T this[int index] => throw new NotImplementedException();
        
        public ReadOnlySpan<T> Slice(int start) => throw new NotImplementedException();
        public ReadOnlySpan<T> Slice(int start, int length) => throw new NotImplementedException();
    }
    
    /// <summary>
    /// Memory - heap-allocated equivalent of Span.
    /// </summary>
    public readonly struct Memory<T>
    {
        private readonly T[] _array;
        
        public Memory(T[] array)
        {
            _array = array;
        }
        
        public int Length => _array != null ? _array.Length : 0;
        public Span<T> Span => new Span<T>(_array);
        
        public Memory<T> Slice(int start) => throw new NotImplementedException();
        public Memory<T> Slice(int start, int length) => throw new NotImplementedException();
    }
    
    /// <summary>
    /// ReadOnlyMemory - immutable version of Memory.
    /// </summary>
    public readonly struct ReadOnlyMemory<T>
    {
        private readonly T[] _array;
        
        public ReadOnlyMemory(T[] array)
        {
            _array = array;
        }
        
        public int Length => _array != null ? _array.Length : 0;
        public ReadOnlySpan<T> Span => new ReadOnlySpan<T>(_array);
        
        public ReadOnlyMemory<T> Slice(int start) => throw new NotImplementedException();
        public ReadOnlyMemory<T> Slice(int start, int length) => throw new NotImplementedException();
    }
}
