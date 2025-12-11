/*
 * System.Collections.Generic.LinkedList<T>
 * Lux9 CLR Base Class Library
 */
namespace System.Collections.Generic
{
    using System;
    using System.Collections;

    /// <summary>
    /// Represents a doubly linked list.
    /// </summary>
    public class LinkedList<T> : ICollection<T>, IEnumerable<T>, IEnumerable, ICollection
    {
        private LinkedListNode<T> _head;
        private int _count;
        private int _version;
        
        public LinkedList() { }
        
        public LinkedList(IEnumerable<T> collection)
        {
            if (collection == null) throw new ArgumentNullException(nameof(collection));
            foreach (T item in collection)
            {
                AddLast(item);
            }
        }
        
        public int Count => _count;
        
        public LinkedListNode<T> First => _head;
        
        public LinkedListNode<T> Last => _head?.Previous;
        
        bool ICollection<T>.IsReadOnly => false;
        bool ICollection.IsSynchronized => false;
        object ICollection.SyncRoot => this;
        
        public LinkedListNode<T> AddFirst(T value)
        {
            LinkedListNode<T> node = new LinkedListNode<T>(this, value);
            if (_head == null)
            {
                InsertNodeToEmptyList(node);
            }
            else
            {
                InsertNodeBefore(_head, node);
                _head = node;
            }
            return node;
        }
        
        public LinkedListNode<T> AddLast(T value)
        {
            LinkedListNode<T> node = new LinkedListNode<T>(this, value);
            if (_head == null)
            {
                InsertNodeToEmptyList(node);
            }
            else
            {
                InsertNodeBefore(_head, node);
            }
            return node;
        }
        
        public LinkedListNode<T> AddBefore(LinkedListNode<T> node, T value)
        {
            ValidateNode(node);
            LinkedListNode<T> newNode = new LinkedListNode<T>(this, value);
            InsertNodeBefore(node, newNode);
            if (node == _head) _head = newNode;
            return newNode;
        }
        
        public LinkedListNode<T> AddAfter(LinkedListNode<T> node, T value)
        {
            ValidateNode(node);
            LinkedListNode<T> newNode = new LinkedListNode<T>(this, value);
            InsertNodeBefore(node.Next, newNode);
            return newNode;
        }
        
        public void Clear()
        {
            LinkedListNode<T> current = _head;
            while (current != null)
            {
                LinkedListNode<T> temp = current;
                current = current.Next;
                temp.Invalidate();
            }
            _head = null;
            _count = 0;
            _version++;
        }
        
        public bool Contains(T value)
        {
            return Find(value) != null;
        }
        
        public LinkedListNode<T> Find(T value)
        {
            LinkedListNode<T> node = _head;
            EqualityComparer<T> c = EqualityComparer<T>.Default;
            if (node != null)
            {
                if (value != null)
                {
                    do
                    {
                        if (c.Equals(node.Value, value)) return node;
                        node = node.Next;
                    } while (node != _head);
                }
                else
                {
                    do
                    {
                        if (node.Value == null) return node;
                        node = node.Next;
                    } while (node != _head);
                }
            }
            return null;
        }
        
        public LinkedListNode<T> FindLast(T value)
        {
            if (_head == null) return null;
            
            LinkedListNode<T> last = _head.Previous;
            LinkedListNode<T> node = last;
            EqualityComparer<T> c = EqualityComparer<T>.Default;
            if (node != null)
            {
                if (value != null)
                {
                    do
                    {
                        if (c.Equals(node.Value, value)) return node;
                        node = node.Previous;
                    } while (node != last);
                }
                else
                {
                    do
                    {
                        if (node.Value == null) return node;
                        node = node.Previous;
                    } while (node != last);
                }
            }
            return null;
        }
        
        public bool Remove(T value)
        {
            LinkedListNode<T> node = Find(value);
            if (node != null)
            {
                Remove(node);
                return true;
            }
            return false;
        }
        
        public void Remove(LinkedListNode<T> node)
        {
            ValidateNode(node);
            if (node.Next == node)
            {
                _head = null;
            }
            else
            {
                node.Next.Previous = node.Previous;
                node.Previous.Next = node.Next;
                if (_head == node) _head = node.Next;
            }
            node.Invalidate();
            _count--;
            _version++;
        }
        
        public void RemoveFirst()
        {
            if (_head == null) throw new InvalidOperationException("The LinkedList is empty");
            Remove(_head);
        }
        
        public void RemoveLast()
        {
            if (_head == null) throw new InvalidOperationException("The LinkedList is empty");
            Remove(_head.Previous);
        }
        
        public void CopyTo(T[] array, int index)
        {
            if (array == null) throw new ArgumentNullException(nameof(array));
            if (index < 0) throw new ArgumentOutOfRangeException(nameof(index));
            if (array.Length - index < Count) throw new ArgumentException("Not enough space");
            
            LinkedListNode<T> node = _head;
            if (node != null)
            {
                do
                {
                    array[index++] = node.Value;
                    node = node.Next;
                } while (node != _head);
            }
        }
        
        void ICollection<T>.Add(T value) => AddLast(value);
        
        void ICollection.CopyTo(Array array, int index)
        {
            T[] arr = array as T[];
            if (arr != null) CopyTo(arr, index);
        }
        
        private void InsertNodeBefore(LinkedListNode<T> node, LinkedListNode<T> newNode)
        {
            newNode.Next = node;
            newNode.Previous = node.Previous;
            node.Previous.Next = newNode;
            node.Previous = newNode;
            _count++;
            _version++;
        }
        
        private void InsertNodeToEmptyList(LinkedListNode<T> newNode)
        {
            newNode.Next = newNode;
            newNode.Previous = newNode;
            _head = newNode;
            _count++;
            _version++;
        }
        
        private void ValidateNode(LinkedListNode<T> node)
        {
            if (node == null) throw new ArgumentNullException(nameof(node));
            if (node.List != this) throw new InvalidOperationException("Node is not in this list");
        }
        
        public Enumerator GetEnumerator() => new Enumerator(this);
        IEnumerator<T> IEnumerable<T>.GetEnumerator() => new Enumerator(this);
        IEnumerator IEnumerable.GetEnumerator() => new Enumerator(this);
        
        public struct Enumerator : IEnumerator<T>
        {
            private LinkedList<T> _list;
            private LinkedListNode<T> _node;
            private int _version;
            private T _current;
            private int _index;
            
            internal Enumerator(LinkedList<T> list)
            {
                _list = list;
                _version = list._version;
                _node = list._head;
                _current = default;
                _index = 0;
            }
            
            public bool MoveNext()
            {
                if (_version != _list._version) throw new InvalidOperationException("Collection modified");
                
                if (_node == null) return false;
                
                _current = _node.Value;
                _node = _node.Next;
                _index++;
                
                if (_node == _list._head)
                {
                    _node = null;
                }
                
                return true;
            }
            
            public T Current => _current;
            object IEnumerator.Current => _current;
            public void Dispose() { }
            
            void IEnumerator.Reset()
            {
                _node = _list._head;
                _current = default;
                _index = 0;
            }
        }
    }
    
    /// <summary>
    /// Represents a node in a LinkedList.
    /// </summary>
    public sealed class LinkedListNode<T>
    {
        internal LinkedList<T> List;
        internal LinkedListNode<T> Next;
        internal LinkedListNode<T> Previous;
        
        public T Value { get; set; }
        
        public LinkedListNode(T value)
        {
            Value = value;
        }
        
        internal LinkedListNode(LinkedList<T> list, T value)
        {
            List = list;
            Value = value;
        }
        
        internal void Invalidate()
        {
            List = null;
            Next = null;
            Previous = null;
        }
    }
}
