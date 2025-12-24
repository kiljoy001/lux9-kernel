/*
 * ECMA-335 Array Type - Lux9 BCL
 * 
 * The Array type is the base class for all array types.
 */
namespace System
{
    using System.Collections;
    using System.Collections.Generic;

    // Array (ECMA-335 IV.5.66)
    public abstract class Array : ICloneable, IList, ICollection, IEnumerable
    {
        // Internal - provided by runtime
        internal Array() { }

        public int Length => GetLength();
        public long LongLength => Length;
        public int Rank => GetRank();

        // Runtime-implemented
        internal extern int GetLength();
        internal extern int GetRank();
        public extern int GetLength(int dimension);
        public extern int GetLowerBound(int dimension);
        public extern int GetUpperBound(int dimension);
        public extern Object GetValue(int index);
        public extern Object GetValue(int index1, int index2);
        public extern Object GetValue(params int[] indices);
        public extern void SetValue(Object value, int index);
        public extern void SetValue(Object value, int index1, int index2);
        public extern void SetValue(Object value, params int[] indices);

        public Object Clone() => MemberwiseClone();

        public static void Clear(Array array, int index, int length)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            if (index < 0 || length < 0 || index + length > array.Length)
                throw new IndexOutOfRangeException();
            for (int i = index; i < index + length; i++)
                array.SetValue(null, i);
        }

        public static void Copy(Array sourceArray, Array destinationArray, int length) =>
            Copy(sourceArray, 0, destinationArray, 0, length);

        public static void Copy(Array sourceArray, int sourceIndex, Array destinationArray, int destinationIndex, int length)
        {
            if (sourceArray == null) throw new ArgumentNullException(nameof(sourceArray));
            if (destinationArray == null) throw new ArgumentNullException(nameof(destinationArray));
            if (length < 0) throw new ArgumentOutOfRangeException(nameof(length));
            if (sourceIndex < 0 || sourceIndex + length > sourceArray.Length)
                throw new ArgumentOutOfRangeException(nameof(sourceIndex));
            if (destinationIndex < 0 || destinationIndex + length > destinationArray.Length)
                throw new ArgumentOutOfRangeException(nameof(destinationIndex));

            for (int i = 0; i < length; i++)
                destinationArray.SetValue(sourceArray.GetValue(sourceIndex + i), destinationIndex + i);
        }

        public void CopyTo(Array array, int index) => Copy(this, 0, array, index, Length);

        public static int IndexOf(Array array, Object value) => IndexOf(array, value, 0, array?.Length ?? 0);

        public static int IndexOf(Array array, Object value, int startIndex) =>
            IndexOf(array, value, startIndex, (array?.Length ?? 0) - startIndex);

        public static int IndexOf(Array array, Object value, int startIndex, int count)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            if (startIndex < 0 || startIndex > array.Length) throw new ArgumentOutOfRangeException(nameof(startIndex));
            if (count < 0 || startIndex > array.Length - count) throw new ArgumentOutOfRangeException(nameof(count));

            int end = startIndex + count;
            for (int i = startIndex; i < end; i++)
            {
                Object elem = array.GetValue(i);
                if (value == null ? elem == null : value.Equals(elem))
                    return i;
            }
            return -1;
        }

        public static int IndexOf<T>(T[] array, T value) => IndexOf(array, value, 0, array?.Length ?? 0);

        public static int IndexOf<T>(T[] array, T value, int startIndex, int count)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            if (startIndex < 0 || startIndex > array.Length) throw new ArgumentOutOfRangeException(nameof(startIndex));
            if (count < 0 || startIndex > array.Length - count) throw new ArgumentOutOfRangeException(nameof(count));

            int end = startIndex + count;
            EqualityComparer<T> comparer = EqualityComparer<T>.Default;
            for (int i = startIndex; i < end; i++)
            {
                if (comparer.Equals(array[i], value))
                    return i;
            }
            return -1;
        }

        public static int LastIndexOf(Array array, Object value) =>
            LastIndexOf(array, value, (array?.Length ?? 0) - 1, array?.Length ?? 0);

        public static int LastIndexOf(Array array, Object value, int startIndex, int count)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            if (array.Length == 0) return -1;
            if (startIndex < 0 || startIndex >= array.Length) throw new ArgumentOutOfRangeException(nameof(startIndex));
            if (count < 0 || startIndex - count + 1 < 0) throw new ArgumentOutOfRangeException(nameof(count));

            int end = startIndex - count + 1;
            for (int i = startIndex; i >= end; i--)
            {
                Object elem = array.GetValue(i);
                if (value == null ? elem == null : value.Equals(elem))
                    return i;
            }
            return -1;
        }

        public static void Reverse(Array array) => Reverse(array, 0, array?.Length ?? 0);

        public static void Reverse(Array array, int index, int length)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            if (index < 0 || length < 0 || index + length > array.Length)
                throw new ArgumentOutOfRangeException();

            int i = index, j = index + length - 1;
            while (i < j)
            {
                Object temp = array.GetValue(i);
                array.SetValue(array.GetValue(j), i);
                array.SetValue(temp, j);
                i++; j--;
            }
        }

        public static void Sort(Array array) => Sort(array, null);

        public static void Sort(Array array, IComparer comparer)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            Sort(array, 0, array.Length, comparer);
        }

        public static void Sort(Array array, int index, int length, IComparer comparer)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            if (index < 0 || length < 0 || index + length > array.Length)
                throw new ArgumentOutOfRangeException();
            // Simple insertion sort
            for (int i = index + 1; i < index + length; i++)
            {
                Object key = array.GetValue(i);
                int j = i - 1;
                while (j >= index)
                {
                    Object elem = array.GetValue(j);
                    int cmp = comparer?.Compare(elem, key) ??
                              ((IComparable)elem)?.CompareTo(key) ?? 0;
                    if (cmp <= 0) break;
                    array.SetValue(elem, j + 1);
                    j--;
                }
                array.SetValue(key, j + 1);
            }
        }

        public static void Sort<T>(T[] array) => Sort(array, 0, array?.Length ?? 0, null);

        public static void Sort<T>(T[] array, Comparison<T> comparison)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            if (comparison == null) throw new ArgumentNullException(nameof(comparison));
            // Simple insertion sort
            for (int i = 1; i < array.Length; i++)
            {
                T key = array[i];
                int j = i - 1;
                while (j >= 0 && comparison(array[j], key) > 0)
                {
                    array[j + 1] = array[j];
                    j--;
                }
                array[j + 1] = key;
            }
        }

        public static void Sort<T>(T[] array, int index, int length, IComparer<T> comparer)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            if (index < 0 || length < 0 || index + length > array.Length)
                throw new ArgumentOutOfRangeException();
            comparer ??= Comparer<T>.Default;
            for (int i = index + 1; i < index + length; i++)
            {
                T key = array[i];
                int j = i - 1;
                while (j >= index && comparer.Compare(array[j], key) > 0)
                {
                    array[j + 1] = array[j];
                    j--;
                }
                array[j + 1] = key;
            }
        }

        public static int BinarySearch(Array array, Object value) =>
            BinarySearch(array, 0, array?.Length ?? 0, value, null);

        public static int BinarySearch(Array array, int index, int length, Object value, IComparer comparer)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            int lo = index, hi = index + length - 1;
            while (lo <= hi)
            {
                int mid = lo + (hi - lo) / 2;
                Object elem = array.GetValue(mid);
                int cmp = comparer?.Compare(elem, value) ??
                          ((IComparable)elem)?.CompareTo(value) ?? 0;
                if (cmp == 0) return mid;
                if (cmp < 0) lo = mid + 1;
                else hi = mid - 1;
            }
            return ~lo;
        }

        public static T[] Empty<T>() => EmptyArray<T>.Value;

        public static void Resize<T>(ref T[] array, int newSize)
        {
            if (newSize < 0) throw new ArgumentOutOfRangeException(nameof(newSize));
            T[] oldArray = array;
            if (oldArray == null)
            {
                array = new T[newSize];
                return;
            }
            if (oldArray.Length != newSize)
            {
                T[] newArray = new T[newSize];
                Copy(oldArray, 0, newArray, 0, Math.Min(oldArray.Length, newSize));
                array = newArray;
            }
        }

        public static bool Exists<T>(T[] array, Predicate<T> match)
        {
            return FindIndex(array, match) >= 0;
        }

        public static T Find<T>(T[] array, Predicate<T> match)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            if (match == null) throw new ArgumentNullException(nameof(match));
            for (int i = 0; i < array.Length; i++)
            {
                if (match(array[i])) return array[i];
            }
            return default;
        }

        public static T[] FindAll<T>(T[] array, Predicate<T> match)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            if (match == null) throw new ArgumentNullException(nameof(match));
            var list = new List<T>();
            for (int i = 0; i < array.Length; i++)
            {
                if (match(array[i])) list.Add(array[i]);
            }
            return list.ToArray();
        }

        public static int FindIndex<T>(T[] array, Predicate<T> match) =>
            FindIndex(array, 0, array?.Length ?? 0, match);

        public static int FindIndex<T>(T[] array, int startIndex, int count, Predicate<T> match)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            if (match == null) throw new ArgumentNullException(nameof(match));
            if (startIndex < 0 || startIndex > array.Length) throw new ArgumentOutOfRangeException(nameof(startIndex));
            if (count < 0 || startIndex > array.Length - count) throw new ArgumentOutOfRangeException(nameof(count));

            int end = startIndex + count;
            for (int i = startIndex; i < end; i++)
            {
                if (match(array[i])) return i;
            }
            return -1;
        }

        public static void ForEach<T>(T[] array, Action<T> action)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            if (action == null) throw new ArgumentNullException(nameof(action));
            for (int i = 0; i < array.Length; i++)
                action(array[i]);
        }

        public static bool TrueForAll<T>(T[] array, Predicate<T> match)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            if (match == null) throw new ArgumentNullException(nameof(match));
            for (int i = 0; i < array.Length; i++)
            {
                if (!match(array[i])) return false;
            }
            return true;
        }

        public static TOutput[] ConvertAll<TInput, TOutput>(TInput[] array, Converter<TInput, TOutput> converter)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            if (converter == null) throw new ArgumentNullException(nameof(converter));
            TOutput[] result = new TOutput[array.Length];
            for (int i = 0; i < array.Length; i++)
                result[i] = converter(array[i]);
            return result;
        }

        // IList implementation
        Object IList.this[int index]
        {
            get => GetValue(index);
            set => SetValue(value, index);
        }
        bool IList.IsFixedSize => true;
        bool IList.IsReadOnly => false;
        int IList.Add(Object value) => throw new NotSupportedException();
        void IList.Clear() => Clear(this, 0, Length);
        bool IList.Contains(Object value) => IndexOf(this, value) >= 0;
        int IList.IndexOf(Object value) => IndexOf(this, value);
        void IList.Insert(int index, Object value) => throw new NotSupportedException();
        void IList.Remove(Object value) => throw new NotSupportedException();
        void IList.RemoveAt(int index) => throw new NotSupportedException();

        // ICollection implementation
        int ICollection.Count => Length;
        bool ICollection.IsSynchronized => false;
        Object ICollection.SyncRoot => this;

        // IEnumerable implementation
        public IEnumerator GetEnumerator() => new ArrayEnumerator(this);

        private class ArrayEnumerator : IEnumerator
        {
            private Array _array;
            private int _index;
            internal ArrayEnumerator(Array array) { _array = array; _index = -1; }
            public Object Current => _array.GetValue(_index);
            public bool MoveNext() => ++_index < _array.Length;
            public void Reset() => _index = -1;
        }
    }

    // Helper for Empty<T>()
    internal static class EmptyArray<T>
    {
        public static readonly T[] Value = new T[0];
    }

    // Delegate types for array operations
    public delegate void Action();
    public delegate void Action<in T>(T obj);
    public delegate void Action<in T1, in T2>(T1 arg1, T2 arg2);
    public delegate void Action<in T1, in T2, in T3>(T1 arg1, T2 arg2, T3 arg3);
    public delegate void Action<in T1, in T2, in T3, in T4>(T1 arg1, T2 arg2, T3 arg3, T4 arg4);

    public delegate TResult Func<out TResult>();
    public delegate TResult Func<in T, out TResult>(T arg);
    public delegate TResult Func<in T1, in T2, out TResult>(T1 arg1, T2 arg2);
    public delegate TResult Func<in T1, in T2, in T3, out TResult>(T1 arg1, T2 arg2, T3 arg3);
    public delegate TResult Func<in T1, in T2, in T3, in T4, out TResult>(T1 arg1, T2 arg2, T3 arg3, T4 arg4);

    public delegate bool Predicate<in T>(T obj);
    public delegate int Comparison<in T>(T x, T y);
    public delegate TOutput Converter<in TInput, out TOutput>(TInput input);
}
