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
        public void RemoveAt(int index) => throw new NotSupportedException("Fixed size");
    }
}
