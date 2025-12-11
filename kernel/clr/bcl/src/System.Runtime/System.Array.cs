namespace System
{
    using System.Runtime.InteropServices;
    using System.Runtime.CompilerServices;
    using System.Collections;

    [Serializable]
    public abstract class Array : ICollection, IEnumerable, IList
    {
        public extern int Length {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
        }

        public long LongLength => Length;
        public int Rank => 1; // Simplify for now

        public abstract IEnumerator GetEnumerator();

        // ICollection
        public int Count => Length;
        public object SyncRoot => this;
        public bool IsSynchronized => false;
        public void CopyTo(Array array, int index) 
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            // Stub: naive copy
            for(int i=0; i<Length; i++) {
                array.SetValue(GetValue(i), index + i);
            }
        }

        // Mapped to internal calls ideally
        public static void Copy(Array sourceArray, Array destinationArray, int length) 
        {
             for(int i=0; i<length; i++) {
                destinationArray.SetValue(sourceArray.GetValue(i), i);
            }
        }
        
        // Internal accessors
        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern object GetValue(int index);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern void SetValue(object value, int index);
        
        // IList
        public object this[int index] { 
            get => GetValue(index); 
            set => SetValue(value, index); 
        }
        
        public bool IsFixedSize => true;
        public bool IsReadOnly => false;
        public int Add(object value) => throw new NotSupportedException("Fixed size");
        public void Clear() 
        {
            for(int i=0; i<Length; i++) SetValue(null, i);
        }
        public bool Contains(object value) 
        {
            for(int i=0; i<Length; i++) {
                object v = GetValue(i);
                if (Object.Equals(v, value)) return true;
            }
            return false;
        }
        public int IndexOf(object value) 
        {
            for(int i=0; i<Length; i++) {
                object v = GetValue(i);
                if (Object.Equals(v, value)) return i;
            }
            return -1;
        }
        public void Insert(int index, object value) => throw new NotSupportedException("Fixed size");
        public void Remove(object value) => throw new NotSupportedException("Fixed size");
        public static void Clear(Array array, int index, int length)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            for(int i=0; i<length; i++) array.SetValue(null, index + i);
        }

        public static void Copy(Array sourceArray, int sourceIndex, Array destinationArray, int destinationIndex, int length)
        {
            if (sourceArray == null) throw new ArgumentNullException(nameof(sourceArray));
            if (destinationArray == null) throw new ArgumentNullException(nameof(destinationArray));
            
            // Handle overlap? Naive copy for now.
            if (sourceArray == destinationArray && sourceIndex < destinationIndex)
            {
                for(int i=length-1; i>=0; i--)
                {
                   destinationArray.SetValue(sourceArray.GetValue(sourceIndex+i), destinationIndex+i);
                }
            }
            else
            {
                for(int i=0; i<length; i++)
                {
                   destinationArray.SetValue(sourceArray.GetValue(sourceIndex+i), destinationIndex+i);
                }
            }
        }

        public static int IndexOf(Array array, object value) => IndexOf(array, value, 0, array.Length);
        
        public static int IndexOf(Array array, object value, int startIndex) => IndexOf(array, value, startIndex, array.Length - startIndex);
        
        public static int IndexOf(Array array, object value, int startIndex, int count)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            // Bounds check omitted for brevity/speed in minimal BCL
            for(int i=0; i<count; i++)
            {
                object v = array.GetValue(startIndex + i);
                if (Object.Equals(v, value)) return startIndex + i;
            }
            return -1;
        }

        public static int IndexOf<T>(T[] array, T value) => IndexOf((Array)array, value, 0, array.Length);
        public static int IndexOf<T>(T[] array, T value, int startIndex) => IndexOf((Array)array, value, startIndex, array.Length - startIndex);
        public static int IndexOf<T>(T[] array, T value, int startIndex, int count)
        {
             return IndexOf((Array)array, value, startIndex, count); 
             // Using non-generic for simplicity now as EqualityComparer might be missing
        }

        public static void Resize<T>(ref T[] array, int newSize)
        {
            if (newSize < 0) throw new ArgumentOutOfRangeException(nameof(newSize));
            T[] newArray = new T[newSize];
            int toCopy = 0;
            if (array != null)
            {
                toCopy = Math.Min(array.Length, newSize);
                Copy(array, 0, newArray, 0, toCopy);
            }
            array = newArray;
        }
        
        public static T[] Empty<T>() => EmptyArray<T>.Value;
        
        private static class EmptyArray<T>
        {
            public static readonly T[] Value = new T[0];
        }


        public void RemoveAt(int index) => throw new NotSupportedException("Fixed size");
    }
}
