/*
 * ECMA-335 Core Interfaces - Lux9 BCL
 * 
 * Fundamental interfaces required by the CLI specification.
 */
namespace System
{
    // IComparable (ECMA-335 IV.5.21)
    public interface IComparable
    {
        int CompareTo(Object obj);
    }

    // IComparable<T> (ECMA-335 IV.5.22)
    public interface IComparable<in T>
    {
        int CompareTo(T other);
    }

    // IEquatable<T> (ECMA-335 IV.5.23)
    public interface IEquatable<T>
    {
        bool Equals(T other);
    }

    // ICloneable (ECMA-335 IV.5.24)
    public interface ICloneable
    {
        Object Clone();
    }

    // IDisposable (ECMA-335 IV.5.25)
    public interface IDisposable
    {
        void Dispose();
    }

    // IFormattable (ECMA-335 IV.5.26)
    public interface IFormattable
    {
        string ToString(string format, IFormatProvider formatProvider);
    }

    // IFormatProvider (ECMA-335 IV.5.27)
    public interface IFormatProvider
    {
        Object GetFormat(Type formatType);
    }

    // StringComparison enum
    public enum StringComparison
    {
        CurrentCulture = 0,
        CurrentCultureIgnoreCase = 1,
        InvariantCulture = 2,
        InvariantCultureIgnoreCase = 3,
        Ordinal = 4,
        OrdinalIgnoreCase = 5
    }
}

namespace System.Collections
{
    // IEnumerable (ECMA-335 IV.5.28)
    public interface IEnumerable
    {
        IEnumerator GetEnumerator();
    }

    // IEnumerator (ECMA-335 IV.5.29)
    public interface IEnumerator
    {
        Object Current { get; }
        bool MoveNext();
        void Reset();
    }

    // ICollection (ECMA-335 IV.5.30)
    public interface ICollection : IEnumerable
    {
        int Count { get; }
        bool IsSynchronized { get; }
        Object SyncRoot { get; }
        void CopyTo(Array array, int index);
    }

    // IList (ECMA-335 IV.5.31)
    public interface IList : ICollection, IEnumerable
    {
        Object this[int index] { get; set; }
        bool IsFixedSize { get; }
        bool IsReadOnly { get; }
        int Add(Object value);
        void Clear();
        bool Contains(Object value);
        int IndexOf(Object value);
        void Insert(int index, Object value);
        void Remove(Object value);
        void RemoveAt(int index);
    }

    // IDictionary (ECMA-335 IV.5.32)
    public interface IDictionary : ICollection, IEnumerable
    {
        Object this[Object key] { get; set; }
        ICollection Keys { get; }
        ICollection Values { get; }
        bool IsFixedSize { get; }
        bool IsReadOnly { get; }
        void Add(Object key, Object value);
        void Clear();
        bool Contains(Object key);
        new IDictionaryEnumerator GetEnumerator();
        void Remove(Object key);
    }

    // IDictionaryEnumerator (ECMA-335 IV.5.33)
    public interface IDictionaryEnumerator : IEnumerator
    {
        DictionaryEntry Entry { get; }
        Object Key { get; }
        Object Value { get; }
    }

    // DictionaryEntry (ECMA-335 IV.5.34)
    public struct DictionaryEntry
    {
        private Object _key;
        private Object _value;
        public DictionaryEntry(Object key, Object value) { _key = key; _value = value; }
        public Object Key { get => _key; set => _key = value; }
        public Object Value { get => _value; set => _value = value; }
    }

    // IComparer (ECMA-335 IV.5.35)
    public interface IComparer
    {
        int Compare(Object x, Object y);
    }

    // IHashCodeProvider - deprecated but in standard
    public interface IHashCodeProvider
    {
        int GetHashCode(Object obj);
    }

    // IEqualityComparer
    public interface IEqualityComparer
    {
        bool Equals(Object x, Object y);
        int GetHashCode(Object obj);
    }
}

namespace System.Collections.Generic
{
    // IEnumerable<T> (ECMA-335 IV.5.36)
    public interface IEnumerable<out T> : System.Collections.IEnumerable
    {
        new IEnumerator<T> GetEnumerator();
    }

    // IEnumerator<T> (ECMA-335 IV.5.37)
    public interface IEnumerator<out T> : System.Collections.IEnumerator, IDisposable
    {
        new T Current { get; }
    }

    // ICollection<T> (ECMA-335 IV.5.38)
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

    // IList<T> (ECMA-335 IV.5.39)
    public interface IList<T> : ICollection<T>
    {
        T this[int index] { get; set; }
        int IndexOf(T item);
        void Insert(int index, T item);
        void RemoveAt(int index);
    }

    // IDictionary<TKey, TValue> (ECMA-335 IV.5.40)
    public interface IDictionary<TKey, TValue> : ICollection<KeyValuePair<TKey, TValue>>
    {
        TValue this[TKey key] { get; set; }
        ICollection<TKey> Keys { get; }
        ICollection<TValue> Values { get; }
        void Add(TKey key, TValue value);
        bool ContainsKey(TKey key);
        bool Remove(TKey key);
        bool TryGetValue(TKey key, out TValue value);
    }

    // IComparer<T> (ECMA-335 IV.5.41)
    public interface IComparer<in T>
    {
        int Compare(T x, T y);
    }

    // IEqualityComparer<T> (ECMA-335 IV.5.42)
    public interface IEqualityComparer<in T>
    {
        bool Equals(T x, T y);
        int GetHashCode(T obj);
    }

    // ISet<T> (ECMA-335 IV.5.43)
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

    // KeyValuePair<TKey, TValue> (ECMA-335 IV.5.44)
    public struct KeyValuePair<TKey, TValue>
    {
        private TKey _key;
        private TValue _value;
        public KeyValuePair(TKey key, TValue value) { _key = key; _value = value; }
        public TKey Key => _key;
        public TValue Value => _value;
        public override string ToString() => String.Concat("[", _key?.ToString() ?? "", ", ", _value?.ToString() ?? "", "]");
    }
}
