/*
 * System.Linq - LINQ extension methods
 * Lux9 CLR Base Class Library
 */
namespace System.Linq
{
    using System;
    using System.Collections;
    using System.Collections.Generic;

    /// <summary>
    /// Provides a set of static methods for querying IEnumerable<T>.
    /// </summary>
    public static class Enumerable
    {
        // Where
        public static IEnumerable<TSource> Where<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (predicate == null) throw new ArgumentNullException(nameof(predicate));
            
            foreach (TSource item in source)
            {
                if (predicate(item))
                    yield return item;
            }
        }
        
        public static IEnumerable<TSource> Where<TSource>(this IEnumerable<TSource> source, Func<TSource, int, bool> predicate)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (predicate == null) throw new ArgumentNullException(nameof(predicate));
            
            int index = 0;
            foreach (TSource item in source)
            {
                if (predicate(item, index))
                    yield return item;
                index++;
            }
        }
        
        // Select
        public static IEnumerable<TResult> Select<TSource, TResult>(this IEnumerable<TSource> source, Func<TSource, TResult> selector)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (selector == null) throw new ArgumentNullException(nameof(selector));
            
            foreach (TSource item in source)
            {
                yield return selector(item);
            }
        }
        
        public static IEnumerable<TResult> Select<TSource, TResult>(this IEnumerable<TSource> source, Func<TSource, int, TResult> selector)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (selector == null) throw new ArgumentNullException(nameof(selector));
            
            int index = 0;
            foreach (TSource item in source)
            {
                yield return selector(item, index);
                index++;
            }
        }
        
        // SelectMany
        public static IEnumerable<TResult> SelectMany<TSource, TResult>(this IEnumerable<TSource> source, Func<TSource, IEnumerable<TResult>> selector)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (selector == null) throw new ArgumentNullException(nameof(selector));
            
            foreach (TSource item in source)
            {
                foreach (TResult result in selector(item))
                {
                    yield return result;
                }
            }
        }
        
        // Take and Skip
        public static IEnumerable<TSource> Take<TSource>(this IEnumerable<TSource> source, int count)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            
            int taken = 0;
            foreach (TSource item in source)
            {
                if (taken >= count) yield break;
                yield return item;
                taken++;
            }
        }
        
        public static IEnumerable<TSource> Skip<TSource>(this IEnumerable<TSource> source, int count)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            
            int skipped = 0;
            foreach (TSource item in source)
            {
                if (skipped >= count)
                    yield return item;
                else
                    skipped++;
            }
        }
        
        public static IEnumerable<TSource> TakeWhile<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (predicate == null) throw new ArgumentNullException(nameof(predicate));
            
            foreach (TSource item in source)
            {
                if (!predicate(item)) yield break;
                yield return item;
            }
        }
        
        public static IEnumerable<TSource> SkipWhile<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (predicate == null) throw new ArgumentNullException(nameof(predicate));
            
            bool yielding = false;
            foreach (TSource item in source)
            {
                if (!yielding && !predicate(item))
                    yielding = true;
                if (yielding)
                    yield return item;
            }
        }
        
        // First, FirstOrDefault, Last, LastOrDefault
        public static TSource First<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            
            if (source is IList<TSource> list)
            {
                if (list.Count > 0) return list[0];
            }
            else
            {
                foreach (TSource item in source) return item;
            }
            throw new InvalidOperationException("Sequence contains no elements");
        }
        
        public static TSource First<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (predicate == null) throw new ArgumentNullException(nameof(predicate));
            
            foreach (TSource item in source)
            {
                if (predicate(item)) return item;
            }
            throw new InvalidOperationException("Sequence contains no matching element");
        }
        
        public static TSource FirstOrDefault<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            
            if (source is IList<TSource> list)
            {
                if (list.Count > 0) return list[0];
            }
            else
            {
                foreach (TSource item in source) return item;
            }
            return default(TSource);
        }
        
        public static TSource FirstOrDefault<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (predicate == null) throw new ArgumentNullException(nameof(predicate));
            
            foreach (TSource item in source)
            {
                if (predicate(item)) return item;
            }
            return default(TSource);
        }
        
        public static TSource Last<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            
            if (source is IList<TSource> list)
            {
                int count = list.Count;
                if (count > 0) return list[count - 1];
            }
            else
            {
                using (IEnumerator<TSource> e = source.GetEnumerator())
                {
                    if (e.MoveNext())
                    {
                        TSource result;
                        do { result = e.Current; } while (e.MoveNext());
                        return result;
                    }
                }
            }
            throw new InvalidOperationException("Sequence contains no elements");
        }
        
        public static TSource LastOrDefault<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            
            if (source is IList<TSource> list)
            {
                int count = list.Count;
                if (count > 0) return list[count - 1];
            }
            else
            {
                using (IEnumerator<TSource> e = source.GetEnumerator())
                {
                    if (e.MoveNext())
                    {
                        TSource result;
                        do { result = e.Current; } while (e.MoveNext());
                        return result;
                    }
                }
            }
            return default(TSource);
        }
        
        // Single
        public static TSource Single<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            
            using (IEnumerator<TSource> e = source.GetEnumerator())
            {
                if (!e.MoveNext()) throw new InvalidOperationException("Sequence contains no elements");
                TSource result = e.Current;
                if (e.MoveNext()) throw new InvalidOperationException("Sequence contains more than one element");
                return result;
            }
        }
        
        public static TSource SingleOrDefault<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            
            using (IEnumerator<TSource> e = source.GetEnumerator())
            {
                if (!e.MoveNext()) return default(TSource);
                TSource result = e.Current;
                if (e.MoveNext()) throw new InvalidOperationException("Sequence contains more than one element");
                return result;
            }
        }
        
        // Count
        public static int Count<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            
            if (source is ICollection<TSource> collection) return collection.Count;
            if (source is ICollection nonGenericCollection) return nonGenericCollection.Count;
            
            int count = 0;
            using (IEnumerator<TSource> e = source.GetEnumerator())
            {
                while (e.MoveNext()) count++;
            }
            return count;
        }
        
        public static int Count<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (predicate == null) throw new ArgumentNullException(nameof(predicate));
            
            int count = 0;
            foreach (TSource item in source)
            {
                if (predicate(item)) count++;
            }
            return count;
        }
        
        // Any and All
        public static bool Any<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            
            using (IEnumerator<TSource> e = source.GetEnumerator())
            {
                return e.MoveNext();
            }
        }
        
        public static bool Any<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (predicate == null) throw new ArgumentNullException(nameof(predicate));
            
            foreach (TSource item in source)
            {
                if (predicate(item)) return true;
            }
            return false;
        }
        
        public static bool All<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (predicate == null) throw new ArgumentNullException(nameof(predicate));
            
            foreach (TSource item in source)
            {
                if (!predicate(item)) return false;
            }
            return true;
        }
        
        // Contains
        public static bool Contains<TSource>(this IEnumerable<TSource> source, TSource value)
        {
            if (source is ICollection<TSource> collection) return collection.Contains(value);
            return Contains(source, value, null);
        }
        
        public static bool Contains<TSource>(this IEnumerable<TSource> source, TSource value, IEqualityComparer<TSource> comparer)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (comparer == null) comparer = EqualityComparer<TSource>.Default;
            
            foreach (TSource item in source)
            {
                if (comparer.Equals(item, value)) return true;
            }
            return false;
        }
        
        // ToList and ToArray
        public static List<TSource> ToList<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            return new List<TSource>(source);
        }
        
        public static TSource[] ToArray<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            
            if (source is ICollection<TSource> collection)
            {
                TSource[] array = new TSource[collection.Count];
                collection.CopyTo(array, 0);
                return array;
            }
            
            List<TSource> list = new List<TSource>();
            foreach (TSource item in source)
            {
                list.Add(item);
            }
            return list.ToArray();
        }
        
        // ToDictionary
        public static Dictionary<TKey, TSource> ToDictionary<TSource, TKey>(this IEnumerable<TSource> source, Func<TSource, TKey> keySelector)
        {
            return ToDictionary(source, keySelector, x => x, null);
        }
        
        public static Dictionary<TKey, TElement> ToDictionary<TSource, TKey, TElement>(
            this IEnumerable<TSource> source, 
            Func<TSource, TKey> keySelector, 
            Func<TSource, TElement> elementSelector,
            IEqualityComparer<TKey> comparer = null)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (keySelector == null) throw new ArgumentNullException(nameof(keySelector));
            if (elementSelector == null) throw new ArgumentNullException(nameof(elementSelector));
            
            Dictionary<TKey, TElement> d = new Dictionary<TKey, TElement>(comparer);
            foreach (TSource item in source)
            {
                d.Add(keySelector(item), elementSelector(item));
            }
            return d;
        }
        
        // Distinct
        public static IEnumerable<TSource> Distinct<TSource>(this IEnumerable<TSource> source)
        {
            return Distinct(source, null);
        }
        
        public static IEnumerable<TSource> Distinct<TSource>(this IEnumerable<TSource> source, IEqualityComparer<TSource> comparer)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            
            HashSet<TSource> set = new HashSet<TSource>(comparer);
            foreach (TSource item in source)
            {
                if (set.Add(item))
                    yield return item;
            }
        }
        
        // Concat
        public static IEnumerable<TSource> Concat<TSource>(this IEnumerable<TSource> first, IEnumerable<TSource> second)
        {
            if (first == null) throw new ArgumentNullException(nameof(first));
            if (second == null) throw new ArgumentNullException(nameof(second));
            
            foreach (TSource item in first) yield return item;
            foreach (TSource item in second) yield return item;
        }
        
        // OrderBy (simple in-memory sort)
        public static IOrderedEnumerable<TSource> OrderBy<TSource, TKey>(this IEnumerable<TSource> source, Func<TSource, TKey> keySelector)
        {
            return new OrderedEnumerable<TSource, TKey>(source, keySelector, null, false);
        }
        
        public static IOrderedEnumerable<TSource> OrderByDescending<TSource, TKey>(this IEnumerable<TSource> source, Func<TSource, TKey> keySelector)
        {
            return new OrderedEnumerable<TSource, TKey>(source, keySelector, null, true);
        }
        
        // Range and Repeat
        public static IEnumerable<int> Range(int start, int count)
        {
            if (count < 0) throw new ArgumentOutOfRangeException(nameof(count));
            
            for (int i = 0; i < count; i++)
            {
                yield return start + i;
            }
        }
        
        public static IEnumerable<TResult> Repeat<TResult>(TResult element, int count)
        {
            if (count < 0) throw new ArgumentOutOfRangeException(nameof(count));
            
            for (int i = 0; i < count; i++)
            {
                yield return element;
            }
        }
        
        // Empty
        public static IEnumerable<TResult> Empty<TResult>()
        {
            return EmptyEnumerable<TResult>.Instance;
        }
        
        // Aggregate
        public static TSource Aggregate<TSource>(this IEnumerable<TSource> source, Func<TSource, TSource, TSource> func)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (func == null) throw new ArgumentNullException(nameof(func));
            
            using (IEnumerator<TSource> e = source.GetEnumerator())
            {
                if (!e.MoveNext()) throw new InvalidOperationException("Sequence contains no elements");
                TSource result = e.Current;
                while (e.MoveNext()) result = func(result, e.Current);
                return result;
            }
        }
        
        public static TAccumulate Aggregate<TSource, TAccumulate>(this IEnumerable<TSource> source, TAccumulate seed, Func<TAccumulate, TSource, TAccumulate> func)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (func == null) throw new ArgumentNullException(nameof(func));
            
            TAccumulate result = seed;
            foreach (TSource item in source) result = func(result, item);
            return result;
        }
        
        // Sum (for int)
        public static int Sum(this IEnumerable<int> source)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            int sum = 0;
            foreach (int v in source) sum += v;
            return sum;
        }
        
        public static int Sum<TSource>(this IEnumerable<TSource> source, Func<TSource, int> selector)
        {
            return source.Select(selector).Sum();
        }
        
        // Min and Max
        public static int Min(this IEnumerable<int> source)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            int value = 0;
            bool hasValue = false;
            foreach (int x in source)
            {
                if (hasValue)
                {
                    if (x < value) value = x;
                }
                else
                {
                    value = x;
                    hasValue = true;
                }
            }
            if (!hasValue) throw new InvalidOperationException("Sequence contains no elements");
            return value;
        }
        
        public static int Max(this IEnumerable<int> source)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            int value = 0;
            bool hasValue = false;
            foreach (int x in source)
            {
                if (hasValue)
                {
                    if (x > value) value = x;
                }
                else
                {
                    value = x;
                    hasValue = true;
                }
            }
            if (!hasValue) throw new InvalidOperationException("Sequence contains no elements");
            return value;
        }
        
        // Reverse
        public static IEnumerable<TSource> Reverse<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            return new ReverseIterator<TSource>(source);
        }
        
        private class ReverseIterator<TSource> : IEnumerable<TSource>, IEnumerator<TSource>
        {
            private TSource[] _buffer;
            private int _index;
            
            public ReverseIterator(IEnumerable<TSource> source)
            {
                _buffer = source.ToArray();
                _index = _buffer.Length;
            }
            
            public TSource Current => _buffer[_index];
            object IEnumerator.Current => Current;
            
            public bool MoveNext()
            {
                if (_index > 0)
                {
                    _index--;
                    return true;
                }
                return false;
            }
            
            public void Reset() { _index = _buffer.Length; }
            public void Dispose() { }
            
            public IEnumerator<TSource> GetEnumerator() => this;
            IEnumerator IEnumerable.GetEnumerator() => this;
        }
    }
    
    // Helper classes
    internal class EmptyEnumerable<TElement>
    {
        public static readonly TElement[] Instance = new TElement[0];
    }
    
    public interface IOrderedEnumerable<TElement> : IEnumerable<TElement>
    {
    }
    
    internal class OrderedEnumerable<TElement, TKey> : IOrderedEnumerable<TElement>
    {
        private IEnumerable<TElement> _source;
        private Func<TElement, TKey> _keySelector;
        private IComparer<TKey> _comparer;
        private bool _descending;
        
        public OrderedEnumerable(IEnumerable<TElement> source, Func<TElement, TKey> keySelector, IComparer<TKey> comparer, bool descending)
        {
            _source = source;
            _keySelector = keySelector;
            _comparer = comparer ?? Comparer<TKey>.Default;
            _descending = descending;
        }
        
        public IEnumerator<TElement> GetEnumerator()
        {
            // Simple implementation: copy to list and sort
            List<TElement> list = new List<TElement>(_source);
            // Bubble sort (simple, not efficient)
            for (int i = 0; i < list.Count - 1; i++)
            {
                for (int j = 0; j < list.Count - i - 1; j++)
                {
                    TKey key1 = _keySelector(list[j]);
                    TKey key2 = _keySelector(list[j + 1]);
                    int cmp = _comparer.Compare(key1, key2);
                    if (_descending) cmp = -cmp;
                    if (cmp > 0)
                    {
                        TElement temp = list[j];
                        list[j] = list[j + 1];
                        list[j + 1] = temp;
                    }
                }
            }
            return list.GetEnumerator();
        }
        
        IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
    }
}

namespace System
{

    
    // Predicate delegate
    public delegate bool Predicate<in T>(T obj);
    
    // Comparison delegate
    public delegate int Comparison<in T>(T x, T y);
    
    // Converter delegate
    public delegate TOutput Converter<in TInput, out TOutput>(TInput input);
}
