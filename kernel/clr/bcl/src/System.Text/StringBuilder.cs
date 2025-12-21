/*
 * System.Text.StringBuilder - Mutable string builder
 * Lux9 CLR Base Class Library
 */
namespace System.Text
{
    public sealed class StringBuilder
    {
        private char[] _buffer;
        private int _length;
        private const int DefaultCapacity = 16;

        public StringBuilder() : this(DefaultCapacity) { }

        public StringBuilder(int capacity)
        {
            _buffer = new char[capacity > 0 ? capacity : DefaultCapacity];
            _length = 0;
        }

        public StringBuilder(string value) : this(value?.Length ?? DefaultCapacity)
        {
            if (value != null)
            {
                Append(value);
            }
        }

        public int Length => _length;
        public int Capacity => _buffer.Length;

        public char this[int index]
        {
            get
            {
                if (index < 0 || index >= _length)
                    throw new IndexOutOfRangeException();
                return _buffer[index];
            }
            set
            {
                if (index < 0 || index >= _length)
                    throw new IndexOutOfRangeException();
                _buffer[index] = value;
            }
        }

        private void EnsureCapacity(int minCapacity)
        {
            if (_buffer.Length < minCapacity)
            {
                int newCapacity = _buffer.Length * 2;
                if (newCapacity < minCapacity)
                    newCapacity = minCapacity;
                
                char[] newBuffer = new char[newCapacity];
                for (int i = 0; i < _length; i++)
                {
                    newBuffer[i] = _buffer[i];
                }
                _buffer = newBuffer;
            }
        }

        public StringBuilder Append(string value)
        {
            if (value == null || value.Length == 0)
                return this;

            EnsureCapacity(_length + value.Length);
            for (int i = 0; i < value.Length; i++)
            {
                _buffer[_length++] = value[i];
            }
            return this;
        }

        public StringBuilder Append(char value)
        {
            EnsureCapacity(_length + 1);
            _buffer[_length++] = value;
            return this;
        }

        public StringBuilder Append(object value)
        {
            if (value == null)
                return this;
            return Append(value.ToString());
        }

        public StringBuilder Append(int value)
        {
            return Append(value.ToString());
        }

        public StringBuilder Append(long value)
        {
            return Append(value.ToString());
        }

        public StringBuilder Append(char value, int repeatCount)
        {
            if (repeatCount < 0)
                throw new ArgumentOutOfRangeException(nameof(repeatCount));
            
            EnsureCapacity(_length + repeatCount);
            for (int i = 0; i < repeatCount; i++)
            {
                _buffer[_length++] = value;
            }
            return this;
        }

        public StringBuilder AppendLine()
        {
            return Append('\n');
        }

        public StringBuilder AppendLine(string value)
        {
            Append(value);
            return Append('\n');
        }

        public StringBuilder Insert(int index, string value)
        {
            if (index < 0 || index > _length)
                throw new ArgumentOutOfRangeException(nameof(index));
            
            if (value == null || value.Length == 0)
                return this;

            EnsureCapacity(_length + value.Length);
            
            // Shift existing characters right
            for (int i = _length - 1; i >= index; i--)
            {
                _buffer[i + value.Length] = _buffer[i];
            }
            
            // Insert new value
            for (int i = 0; i < value.Length; i++)
            {
                _buffer[index + i] = value[i];
            }
            _length += value.Length;
            return this;
        }

        public StringBuilder Remove(int startIndex, int length)
        {
            if (startIndex < 0 || startIndex >= _length)
                throw new ArgumentOutOfRangeException(nameof(startIndex));
            if (length < 0 || startIndex + length > _length)
                throw new ArgumentOutOfRangeException(nameof(length));

            // Shift characters left
            int remaining = _length - (startIndex + length);
            for (int i = 0; i < remaining; i++)
            {
                _buffer[startIndex + i] = _buffer[startIndex + length + i];
            }
            _length -= length;
            return this;
        }

        public StringBuilder Replace(char oldChar, char newChar)
        {
            for (int i = 0; i < _length; i++)
            {
                if (_buffer[i] == oldChar)
                    _buffer[i] = newChar;
            }
            return this;
        }

        public StringBuilder Replace(string oldValue, string newValue)
        {
            if (string.IsNullOrEmpty(oldValue))
                throw new ArgumentException("Old value cannot be null or empty", nameof(oldValue));
            
            if (newValue == null)
                newValue = "";

            // Simple implementation: build new StringBuilder
            StringBuilder result = new StringBuilder();
            int i = 0;
            while (i < _length)
            {
                bool found = true;
                if (i + oldValue.Length <= _length)
                {
                    for (int j = 0; j < oldValue.Length; j++)
                    {
                        if (_buffer[i + j] != oldValue[j])
                        {
                            found = false;
                            break;
                        }
                    }
                }
                else
                {
                    found = false;
                }

                if (found)
                {
                    result.Append(newValue);
                    i += oldValue.Length;
                }
                else
                {
                    result.Append(_buffer[i]);
                    i++;
                }
            }

            // Copy result back
            _buffer = result._buffer;
            _length = result._length;
            return this;
        }

        public StringBuilder Clear()
        {
            _length = 0;
            return this;
        }

        public override string ToString()
        {
            if (_length == 0)
                return "";
            
            char[] result = new char[_length];
            for (int i = 0; i < _length; i++)
            {
                result[i] = _buffer[i];
            }
            return new string(result);
        }

        public string ToString(int startIndex, int length)
        {
            if (startIndex < 0 || startIndex >= _length)
                throw new ArgumentOutOfRangeException(nameof(startIndex));
            if (length < 0 || startIndex + length > _length)
                throw new ArgumentOutOfRangeException(nameof(length));

            char[] result = new char[length];
            for (int i = 0; i < length; i++)
            {
                result[i] = _buffer[startIndex + i];
            }
            return new string(result);
        }
    }
}
