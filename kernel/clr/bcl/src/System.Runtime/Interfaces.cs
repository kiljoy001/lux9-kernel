namespace System
{
    public interface IComparable
    {
        int CompareTo(object obj);
    }
    
    public interface IComparable<in T>
    {
        int CompareTo(T other);
    }

    public interface IEquatable<T>
    {
        bool Equals(T other);
    }

    /// <summary>
    /// Defines a method to support the cloning of objects.
    /// </summary>
    public interface ICloneable
    {
        object Clone();
    }
    
    /// <summary>
    /// Provides functionality to format the value of an object.
    /// </summary>
    public interface IFormattable
    {
        string ToString(string format, IFormatProvider formatProvider);
    }
    
    /// <summary>
    /// Provides a mechanism for retrieving an object to control formatting.
    /// </summary>
    public interface IFormatProvider
    {
        object GetFormat(Type formatType);
    }
    
    /// <summary>
    /// Nullable value type wrapper.
    /// </summary>
    public struct Nullable<T> where T : struct
    {
        private readonly bool _hasValue;
        private readonly T _value;
        
        public Nullable(T value)
        {
            _value = value;
            _hasValue = true;
        }
        
        public bool HasValue => _hasValue;
        
        public T Value
        {
            get
            {
                if (!_hasValue) throw new InvalidOperationException("Nullable object must have a value");
                return _value;
            }
        }
        
        public T GetValueOrDefault() => _value;
        public T GetValueOrDefault(T defaultValue) => _hasValue ? _value : defaultValue;
        
        public override bool Equals(object other)
        {
            if (!_hasValue) return other == null;
            if (other == null) return false;
            return _value.Equals(other);
        }
        
        public override int GetHashCode()
        {
            if (!_hasValue) return 0;
            return _value.GetHashCode();
        }
        
        public override string ToString()
        {
            if (!_hasValue) return "";
            return _value.ToString();
        }
        
        public static implicit operator Nullable<T>(T value) => new Nullable<T>(value);
        
        public static explicit operator T(Nullable<T> value) => value.Value;
    }
}

namespace System.Collections
{
    public interface IEnumerable
    {
        IEnumerator GetEnumerator();
    }

    public interface IEnumerator
    {
        bool MoveNext();
        object Current { get; }
        void Reset();
    }

    public interface ICollection : IEnumerable
    {
        int Count { get; }
        object SyncRoot { get; }
        bool IsSynchronized { get; }
        void CopyTo(Array array, int index);
    }

    public interface IList : ICollection
    {
        object this[int index] { get; set; }
        bool IsFixedSize { get; }
        bool IsReadOnly { get; }
        int Add(object value);
        void Clear();
        bool Contains(object value);
        int IndexOf(object value);
        void Insert(int index, object value);
        void Remove(object value);
        void RemoveAt(int index);
    }

    public interface IDictionary : ICollection
    {
        object this[object key] { get; set; }
        bool IsFixedSize { get; }
        bool IsReadOnly { get; }
        ICollection Keys { get; }
        ICollection Values { get; }
        void Add(object key, object value);
        void Clear();
        bool Contains(object key);
        new IDictionaryEnumerator GetEnumerator();
        void Remove(object key);
    }

    public interface IDictionaryEnumerator : IEnumerator
    {
        object Key { get; }
        object Value { get; }
        DictionaryEntry Entry { get; }
    }
    
    // DictionaryEntry definition needs to be somewhere. 
    // It was in Dictionary.cs (generic) and Interfaces.cs (generic file before).
    // It is non-generic, so belongs in System.Collections.
    public struct DictionaryEntry
    {
        public object Key { get; set; }
        public object Value { get; set; }
        
        public DictionaryEntry(object key, object value)
        {
            Key = key;
            Value = value;
        }
    }
}
