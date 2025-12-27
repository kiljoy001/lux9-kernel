/*
 * System.Collections.Generic.Queue<T> and Stack<T>
 * Lux9 CLR Base Class Library
 */
namespace System.Collections.Generic
{
    using System;
    using System.Collections;

    /// <summary>
    /// Represents a first-in, first-out collection of objects.
    /// </summary>
    public class Queue<T> : IEnumerable<T>, IEnumerable, ICollection
    {
        private T[] _array;
        private int _head;       // First valid element in the queue
        private int _tail;       // Last valid element in the queue
        private int _size;       // Number of elements
        private int _version;
        
        private const int DefaultCapacity = 4;
        private static readonly T[] _emptyArray = new T[0];
        
        public Queue()
        {
            _array = _emptyArray;
        }
        
        public Queue(int capacity)
        {
            if (capacity < 0) throw new ArgumentOutOfRangeException(nameof(capacity));
            _array = new T[capacity];
        }
        
        public int Count => _size;
        
        bool ICollection.IsSynchronized => false;
        object ICollection.SyncRoot => this;
        
        public void Clear()
        {
            if (_head < _tail)
            {
                Array.Clear(_array, _head, _size);
            }
            else
            {
                Array.Clear(_array, _head, _array.Length - _head);
                Array.Clear(_array, 0, _tail);
            }
            _head = 0;
            _tail = 0;
            _size = 0;
            _version++;
        }
        
        public void Enqueue(T item)
        {
            if (_size == _array.Length)
            {
                int newcapacity = _array.Length == 0 ? DefaultCapacity : _array.Length * 2;
                SetCapacity(newcapacity);
            }
            
            _array[_tail] = item;
            _tail = (_tail + 1) % _array.Length;
            _size++;
            _version++;
        }
        
        public T Dequeue()
        {
            if (_size == 0) throw new InvalidOperationException("Queue is empty");
            
            T removed = _array[_head];
            _array[_head] = default(T);
            _head = (_head + 1) % _array.Length;
            _size--;
            _version++;
            return removed;
        }
        
        public T Peek()
        {
            if (_size == 0) throw new InvalidOperationException("Queue is empty");
            return _array[_head];
        }
        
        public bool Contains(T item)
        {
            int index = _head;
            int count = _size;
            
            EqualityComparer<T> c = EqualityComparer<T>.Default;
            while (count-- > 0)
            {
                if (item == null)
                {
                    if (_array[index] == null) return true;
                }
                else if (_array[index] != null && c.Equals(_array[index], item))
                {
                    return true;
                }
                index = (index + 1) % _array.Length;
            }
            return false;
        }
        
        public T[] ToArray()
        {
            if (_size == 0) return _emptyArray;
            
            T[] arr = new T[_size];
            if (_head < _tail)
            {
                Array.Copy(_array, _head, arr, 0, _size);
            }
            else
            {
                Array.Copy(_array, _head, arr, 0, _array.Length - _head);
                Array.Copy(_array, 0, arr, _array.Length - _head, _tail);
            }
            return arr;
        }
        
        private void SetCapacity(int capacity)
        {
            T[] newarray = new T[capacity];
            if (_size > 0)
            {
                if (_head < _tail)
                {
                    Array.Copy(_array, _head, newarray, 0, _size);
                }
                else
                {
                    Array.Copy(_array, _head, newarray, 0, _array.Length - _head);
                    Array.Copy(_array, 0, newarray, _array.Length - _head, _tail);
                }
            }
            
            _array = newarray;
            _head = 0;
            _tail = (_size == capacity) ? 0 : _size;
        }
        
        void ICollection.CopyTo(Array array, int index)
        {
            T[] arr = ToArray();
            Array.Copy(arr, 0, array, index, arr.Length);
        }
        
        public Enumerator GetEnumerator() => new Enumerator(this);
        IEnumerator<T> IEnumerable<T>.GetEnumerator() => new Enumerator(this);
        IEnumerator IEnumerable.GetEnumerator() => new Enumerator(this);
        
        public struct Enumerator : IEnumerator<T>
        {
            private Queue<T> _q;
            private int _version;
            private int _index;
            private T _current;
            
            internal Enumerator(Queue<T> q)
            {
                _q = q;
                _version = q._version;
                _index = -1;
                _current = default;
            }
            
            public bool MoveNext()
            {
                if (_version != _q._version) throw new InvalidOperationException("Collection modified");
                
                if (_index == -2) return false;
                
                _index++;
                if (_index == _q._size)
                {
                    _index = -2;
                    _current = default;
                    return false;
                }
                
                int arrayIndex = (_q._head + _index) % _q._array.Length;
                _current = _q._array[arrayIndex];
                return true;
            }
            
            public T Current => _current;
            object IEnumerator.Current => _current;
            public void Dispose() { }
            void IEnumerator.Reset() { _index = -1; _current = default; }
        }
    }
    
    /// <summary>
    /// Represents a last-in, first-out collection of objects.
    /// </summary>
    public class Stack<T> : IEnumerable<T>, IEnumerable, ICollection
    {
        private T[] _array;
        private int _size;
        private int _version;
        
        private const int DefaultCapacity = 4;
        private static readonly T[] _emptyArray = new T[0];
        
        public Stack()
        {
            _array = _emptyArray;
        }
        
        public Stack(int capacity)
        {
            if (capacity < 0) throw new ArgumentOutOfRangeException(nameof(capacity));
            _array = new T[capacity];
        }
        
        public int Count => _size;
        
        bool ICollection.IsSynchronized => false;
        object ICollection.SyncRoot => this;
        
        public void Clear()
        {
            Array.Clear(_array, 0, _size);
            _size = 0;
            _version++;
        }
        
        public void Push(T item)
        {
            if (_size == _array.Length)
            {
                int newcapacity = _array.Length == 0 ? DefaultCapacity : _array.Length * 2;
                T[] newArray = new T[newcapacity];
                Array.Copy(_array, 0, newArray, 0, _size);
                _array = newArray;
            }
            _array[_size++] = item;
            _version++;
        }
        
        public T Pop()
        {
            if (_size == 0) throw new InvalidOperationException("Stack is empty");
            _version++;
            T item = _array[--_size];
            _array[_size] = default(T);
            return item;
        }
        
        public T Peek()
        {
            if (_size == 0) throw new InvalidOperationException("Stack is empty");
            return _array[_size - 1];
        }
        
        public bool Contains(T item)
        {
            int count = _size;
            EqualityComparer<T> c = EqualityComparer<T>.Default;
            while (count-- > 0)
            {
                if (item == null)
                {
                    if (_array[count] == null) return true;
                }
                else if (_array[count] != null && c.Equals(_array[count], item))
                {
                    return true;
                }
            }
            return false;
        }
        
        public T[] ToArray()
        {
            if (_size == 0) return _emptyArray;
            T[] arr = new T[_size];
            for (int i = 0; i < _size; i++)
            {
                arr[i] = _array[_size - i - 1];
            }
            return arr;
        }
        
        void ICollection.CopyTo(Array array, int arrayIndex)
        {
            T[] arr = ToArray();
            Array.Copy(arr, 0, array, arrayIndex, arr.Length);
        }
        
        public Enumerator GetEnumerator() => new Enumerator(this);
        IEnumerator<T> IEnumerable<T>.GetEnumerator() => new Enumerator(this);
        IEnumerator IEnumerable.GetEnumerator() => new Enumerator(this);
        
        public struct Enumerator : IEnumerator<T>
        {
            private Stack<T> _stack;
            private int _version;
            private int _index;
            private T _current;
            
            internal Enumerator(Stack<T> stack)
            {
                _stack = stack;
                _version = stack._version;
                _index = -2;
                _current = default;
            }
            
            public bool MoveNext()
            {
                if (_version != _stack._version) throw new InvalidOperationException("Collection modified");
                
                if (_index == -2)
                {
                    _index = _stack._size - 1;
                    if (_index >= 0)
                    {
                        _current = _stack._array[_index];
                        return true;
                    }
                    return false;
                }
                
                if (_index == -1) return false;
                
                if (--_index >= 0)
                {
                    _current = _stack._array[_index];
                    return true;
                }
                
                _current = default;
                return false;
            }
            
            public T Current => _current;
            object IEnumerator.Current => _current;
            public void Dispose() { }
            void IEnumerator.Reset() { _index = -2; _current = default; }
        }
    }
}
