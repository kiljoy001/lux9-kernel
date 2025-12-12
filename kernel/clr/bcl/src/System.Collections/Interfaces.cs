/*
 * System.Collections.Generic Interfaces and EqualityComparer
 * Lux9 CLR Base Class Library
 */
namespace System.Collections.Generic
{
    using System;
    using System.Collections;

    /// <summary>
    /// Provides a method for comparing objects of type T for equality.
    /// </summary>
    public interface IEqualityComparer<in T>
    {
        bool Equals(T x, T y);
        int GetHashCode(T obj);
    }
    
    /// <summary>
    /// Default equality comparer that uses object.Equals and object.GetHashCode.
    /// </summary>
    public abstract class EqualityComparer<T> : IEqualityComparer<T>, IEqualityComparer
    {
        private static volatile EqualityComparer<T> _default;
        
        public static EqualityComparer<T> Default
        {
            get
            {
                if (_default == null)
                {
                    _default = new ObjectEqualityComparer<T>();
                }
                return _default;
            }
        }
        
        public abstract bool Equals(T x, T y);
        public abstract int GetHashCode(T obj);
        
        bool IEqualityComparer.Equals(object x, object y)
        {
            if (x == y) return true;
            if (x == null || y == null) return false;
            if (x is T && y is T)
                return Equals((T)x, (T)y);
            throw new ArgumentException("Invalid type");
        }
        
        int IEqualityComparer.GetHashCode(object obj)
        {
            if (obj == null) return 0;
            if (obj is T) return GetHashCode((T)obj);
            throw new ArgumentException("Invalid type");
        }
    }
    
    internal class ObjectEqualityComparer<T> : EqualityComparer<T>
    {
        public override bool Equals(T x, T y)
        {
            if (x != null)
            {
                if (y != null) return x.Equals(y);
                return false;
            }
            if (y != null) return false;
            return true;
        }
        
        public override int GetHashCode(T obj)
        {
            if (obj == null) return 0;
            return obj.GetHashCode();
        }
    }
    
    /// <summary>
    /// Defines methods to compare objects.
    /// </summary>
    public interface IComparer<in T>
    {
        int Compare(T x, T y);
    }
    
    /// <summary>
    /// Default comparer that uses IComparable.
    /// </summary>
    public abstract class Comparer<T> : IComparer<T>, IComparer
    {
        private static volatile Comparer<T> _default;
        
        public static Comparer<T> Default
        {
            get
            {
                if (_default == null)
                {
                    _default = new DefaultComparer<T>();
                }
                return _default;
            }
        }
        
        public abstract int Compare(T x, T y);
        
        int IComparer.Compare(object x, object y)
        {
            if (x == null)
                return y == null ? 0 : -1;
            if (y == null)
                return 1;
            if (x is T && y is T)
                return Compare((T)x, (T)y);
            throw new ArgumentException("Invalid type");
        }
    }
    
    internal class DefaultComparer<T> : Comparer<T>
    {
        public override int Compare(T x, T y)
        {
            if (x == null)
                return y == null ? 0 : -1;
            if (y == null)
                return 1;
            if (x is IComparable<T> comparable)
                return comparable.CompareTo(y);
            if (x is IComparable c)
                return c.CompareTo(y);
            throw new ArgumentException("Type does not implement IComparable");
        }
    }
    
    /// <summary>
    /// Represents a read-only collection of elements.
    /// </summary>
    public interface IReadOnlyCollection<out T> : IEnumerable<T>
    {
        int Count { get; }
    }
    
    /// <summary>
    /// Represents a read-only list.
    /// </summary>
    public interface IReadOnlyList<out T> : IReadOnlyCollection<T>
    {
        T this[int index] { get; }
    }
    
    /// <summary>
    /// Represents a read-only dictionary.
    /// </summary>
    public interface IReadOnlyDictionary<TKey, TValue> : IReadOnlyCollection<KeyValuePair<TKey, TValue>>
    {
        bool ContainsKey(TKey key);
        bool TryGetValue(TKey key, out TValue value);
        
        TValue this[TKey key] { get; }
        IEnumerable<TKey> Keys { get; }
        IEnumerable<TValue> Values { get; }
    }

    // Missing Generic Interfaces
    
    public interface IEnumerable<out T> : IEnumerable
    {
        new IEnumerator<T> GetEnumerator();
    }
    
    public interface IEnumerator<out T> : IDisposable, IEnumerator
    {
        new T Current { get; }
    }
    
    public interface ICollection<T> : IEnumerable<T>
    {
        int Count { get; }
        bool IsReadOnly { get; }
        void Add(T item);
        void Clear();
        bool Contains(T item);
        void CopyTo(T[] array, int arrayIndex);
        bool Remove(T item);
    }
    
    public interface IList<T> : ICollection<T>
    {
        T this[int index] { get; set; }
        int IndexOf(T item);
        void Insert(int index, T item);
        void RemoveAt(int index);
    }
    
    public interface IDictionary<TKey, TValue> : ICollection<KeyValuePair<TKey, TValue>>
    {
        TValue this[TKey key] { get; set; }
        ICollection<TKey> Keys { get; }
        ICollection<TValue> Values { get; }
        bool ContainsKey(TKey key);
        void Add(TKey key, TValue value);
        bool Remove(TKey key);
        bool TryGetValue(TKey key, out TValue value);
    }
    

    
    public struct KeyValuePair<TKey, TValue>
    {
        private TKey key;
        private TValue value;
        
        public KeyValuePair(TKey key, TValue value)
        {
            this.key = key;
            this.value = value;
        }
        
        public TKey Key => key;
        public TValue Value => value;
        
        public override string ToString()
        {
            return String.Concat("[", (Key != null ? Key.ToString() : ""), ", ", (Value != null ? Value.ToString() : ""), "]");
        }
    }
    

}


