/*
 * ECMA-335 Text Types - Lux9 BCL
 */
namespace System.Text
{
    // Encoding (ECMA-335)
    public abstract class Encoding
    {
        protected Encoding() { }
        public abstract String EncodingName { get; }
        public abstract int GetByteCount(char[] chars, int index, int count);
        public abstract int GetBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex);
        public abstract int GetCharCount(byte[] bytes, int index, int count);
        public abstract int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex);
        public abstract int GetMaxByteCount(int charCount);
        public abstract int GetMaxCharCount(int byteCount);

        public virtual int GetByteCount(String s) => GetByteCount(s.ToCharArray(), 0, s.Length);
        public virtual byte[] GetBytes(String s) => GetBytes(s.ToCharArray());
        public virtual byte[] GetBytes(char[] chars)
        {
            byte[] bytes = new byte[GetByteCount(chars, 0, chars.Length)];
            GetBytes(chars, 0, chars.Length, bytes, 0);
            return bytes;
        }
        public virtual char[] GetChars(byte[] bytes)
        {
            char[] chars = new char[GetCharCount(bytes, 0, bytes.Length)];
            GetChars(bytes, 0, bytes.Length, chars, 0);
            return chars;
        }
        public virtual String GetString(byte[] bytes) => new String(GetChars(bytes));
        public virtual String GetString(byte[] bytes, int index, int count)
        {
            char[] chars = new char[GetCharCount(bytes, index, count)];
            GetChars(bytes, index, count, chars, 0);
            return new String(chars);
        }

        public static Encoding UTF8 => UTF8Encoding.Instance;
        public static Encoding ASCII => ASCIIEncoding.Instance;
        public static Encoding Unicode => UnicodeEncoding.Instance;
        public static Encoding Default => UTF8;
    }

    public sealed class UTF8Encoding : Encoding
    {
        public static readonly UTF8Encoding Instance = new UTF8Encoding();
        public override String EncodingName => "utf-8";
        public override int GetByteCount(char[] chars, int index, int count)
        {
            int n = 0;
            for (int i = index; i < index + count; i++)
            {
                char c = chars[i];
                if (c < 0x80) n++;
                else if (c < 0x800) n += 2;
                else n += 3;
            }
            return n;
        }
        public override int GetBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex)
        {
            int n = byteIndex;
            for (int i = charIndex; i < charIndex + charCount; i++)
            {
                char c = chars[i];
                if (c < 0x80) bytes[n++] = (byte)c;
                else if (c < 0x800)
                {
                    bytes[n++] = (byte)(0xC0 | (c >> 6));
                    bytes[n++] = (byte)(0x80 | (c & 0x3F));
                }
                else
                {
                    bytes[n++] = (byte)(0xE0 | (c >> 12));
                    bytes[n++] = (byte)(0x80 | ((c >> 6) & 0x3F));
                    bytes[n++] = (byte)(0x80 | (c & 0x3F));
                }
            }
            return n - byteIndex;
        }
        public override int GetCharCount(byte[] bytes, int index, int count)
        {
            int n = 0;
            for (int i = index; i < index + count; )
            {
                byte b = bytes[i];
                if ((b & 0x80) == 0) { i++; n++; }
                else if ((b & 0xE0) == 0xC0) { i += 2; n++; }
                else if ((b & 0xF0) == 0xE0) { i += 3; n++; }
                else { i += 4; n++; }
            }
            return n;
        }
        public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex)
        {
            int n = charIndex;
            for (int i = byteIndex; i < byteIndex + byteCount; )
            {
                byte b = bytes[i];
                if ((b & 0x80) == 0) { chars[n++] = (char)b; i++; }
                else if ((b & 0xE0) == 0xC0)
                {
                    chars[n++] = (char)(((b & 0x1F) << 6) | (bytes[i + 1] & 0x3F));
                    i += 2;
                }
                else
                {
                    chars[n++] = (char)(((b & 0x0F) << 12) | ((bytes[i + 1] & 0x3F) << 6) | (bytes[i + 2] & 0x3F));
                    i += 3;
                }
            }
            return n - charIndex;
        }
        public override int GetMaxByteCount(int charCount) => charCount * 3;
        public override int GetMaxCharCount(int byteCount) => byteCount;
    }

    public sealed class ASCIIEncoding : Encoding
    {
        public static readonly ASCIIEncoding Instance = new ASCIIEncoding();
        public override String EncodingName => "us-ascii";
        public override int GetByteCount(char[] chars, int index, int count) => count;
        public override int GetBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex)
        {
            for (int i = 0; i < charCount; i++)
                bytes[byteIndex + i] = (byte)(chars[charIndex + i] & 0x7F);
            return charCount;
        }
        public override int GetCharCount(byte[] bytes, int index, int count) => count;
        public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex)
        {
            for (int i = 0; i < byteCount; i++)
                chars[charIndex + i] = (char)bytes[byteIndex + i];
            return byteCount;
        }
        public override int GetMaxByteCount(int charCount) => charCount;
        public override int GetMaxCharCount(int byteCount) => byteCount;
    }

    public sealed class UnicodeEncoding : Encoding
    {
        public static readonly UnicodeEncoding Instance = new UnicodeEncoding();
        public override String EncodingName => "utf-16";
        public override int GetByteCount(char[] chars, int index, int count) => count * 2;
        public override int GetBytes(char[] chars, int charIndex, int charCount, byte[] bytes, int byteIndex)
        {
            for (int i = 0; i < charCount; i++)
            {
                bytes[byteIndex + i * 2] = (byte)chars[charIndex + i];
                bytes[byteIndex + i * 2 + 1] = (byte)(chars[charIndex + i] >> 8);
            }
            return charCount * 2;
        }
        public override int GetCharCount(byte[] bytes, int index, int count) => count / 2;
        public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex)
        {
            int n = byteCount / 2;
            for (int i = 0; i < n; i++)
                chars[charIndex + i] = (char)(bytes[byteIndex + i * 2] | (bytes[byteIndex + i * 2 + 1] << 8));
            return n;
        }
        public override int GetMaxByteCount(int charCount) => charCount * 2;
        public override int GetMaxCharCount(int byteCount) => byteCount / 2;
    }

    // StringBuilder (ECMA-335)
    public sealed class StringBuilder
    {
        private char[] _chars;
        private int _length;
        private const int DefaultCapacity = 16;

        public StringBuilder() : this(DefaultCapacity) { }
        public StringBuilder(int capacity) { _chars = new char[capacity]; }
        public StringBuilder(String value) : this(value?.Length ?? 0 + DefaultCapacity)
        {
            if (value != null) Append(value);
        }

        public int Length { get => _length; set { EnsureCapacity(value); _length = value; } }
        public int Capacity { get => _chars.Length; set { if (value > _chars.Length) EnsureCapacity(value); } }

        public char this[int index]
        {
            get => _chars[index];
            set => _chars[index] = value;
        }

        public StringBuilder Append(char value)
        {
            EnsureCapacity(_length + 1);
            _chars[_length++] = value;
            return this;
        }

        public StringBuilder Append(char value, int repeatCount)
        {
            EnsureCapacity(_length + repeatCount);
            for (int i = 0; i < repeatCount; i++) _chars[_length++] = value;
            return this;
        }

        public StringBuilder Append(String value)
        {
            if (value == null || value.Length == 0) return this;
            EnsureCapacity(_length + value.Length);
            for (int i = 0; i < value.Length; i++) _chars[_length++] = value[i];
            return this;
        }

        public StringBuilder Append(Object value) => Append(value?.ToString());
        public StringBuilder Append(int value) => Append(value.ToString());
        public StringBuilder Append(long value) => Append(value.ToString());
        public StringBuilder Append(char[] value) => Append(value, 0, value?.Length ?? 0);
        public StringBuilder Append(char[] value, int startIndex, int charCount)
        {
            if (value == null || charCount == 0) return this;
            EnsureCapacity(_length + charCount);
            for (int i = 0; i < charCount; i++) _chars[_length++] = value[startIndex + i];
            return this;
        }

        public StringBuilder AppendLine() => Append(Environment.NewLine);
        public StringBuilder AppendLine(String value) { Append(value); return AppendLine(); }

        public StringBuilder Insert(int index, char value)
        {
            EnsureCapacity(_length + 1);
            Array.Copy(_chars, index, _chars, index + 1, _length - index);
            _chars[index] = value;
            _length++;
            return this;
        }

        public StringBuilder Insert(int index, String value)
        {
            if (value == null || value.Length == 0) return this;
            EnsureCapacity(_length + value.Length);
            Array.Copy(_chars, index, _chars, index + value.Length, _length - index);
            for (int i = 0; i < value.Length; i++) _chars[index + i] = value[i];
            _length += value.Length;
            return this;
        }

        public StringBuilder Remove(int startIndex, int length)
        {
            if (startIndex + length < _length)
                Array.Copy(_chars, startIndex + length, _chars, startIndex, _length - startIndex - length);
            _length -= length;
            return this;
        }

        public StringBuilder Replace(char oldChar, char newChar)
        {
            for (int i = 0; i < _length; i++)
                if (_chars[i] == oldChar) _chars[i] = newChar;
            return this;
        }

        public StringBuilder Replace(String oldValue, String newValue)
        {
            return Replace(oldValue, newValue, 0, _length);
        }

        public StringBuilder Replace(String oldValue, String newValue, int startIndex, int count)
        {
            String s = ToString(startIndex, count);
            String replaced = s.Replace(oldValue[0], newValue != null && newValue.Length > 0 ? newValue[0] : ' ');
            Remove(startIndex, count);
            Insert(startIndex, replaced);
            return this;
        }

        public StringBuilder Clear() { _length = 0; return this; }

        public override String ToString() => new String(_chars, 0, _length);
        public String ToString(int startIndex, int length) => new String(_chars, startIndex, length);

        private void EnsureCapacity(int capacity)
        {
            if (capacity > _chars.Length)
            {
                int newCap = _chars.Length * 2;
                if (newCap < capacity) newCap = capacity;
                char[] newChars = new char[newCap];
                Array.Copy(_chars, newChars, _length);
                _chars = newChars;
            }
        }
    }
}
