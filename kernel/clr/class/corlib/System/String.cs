/*
 * ECMA-335 String Type - Lux9 BCL
 * 
 * String is a fundamental type in the CLI.
 */
namespace System
{
    using System.Collections;
    using System.Collections.Generic;
    using System.Runtime.CompilerServices;

    // String (ECMA-335 IV.5.45)
    public sealed class String : IComparable, IComparable<String>, IEquatable<String>, IEnumerable<Char>, ICloneable
    {
        // Internal storage - the runtime manages this
        internal readonly int m_stringLength;
        internal readonly char m_firstChar;

        public static readonly String Empty = "";

        // Runtime-provided constructors
        public String(char c, int count)
        {
            if (count < 0) throw new ArgumentOutOfRangeException(nameof(count));
            // Implemented by runtime
        }

        public unsafe String(char* value)
        {
            // Implemented by runtime
        }

        public unsafe String(char* value, int startIndex, int length)
        {
            // Implemented by runtime
        }

        public String(char[] value)
        {
            // Implemented by runtime
        }

        public String(char[] value, int startIndex, int length)
        {
            // Implemented by runtime
        }

        public int Length => m_stringLength;

        public char this[int index]
        {
            get
            {
                if ((uint)index >= (uint)m_stringLength)
                    throw new IndexOutOfRangeException();
                // Runtime-implemented: access internal char storage
                return GetCharInternal(index);
            }
        }

        // Runtime intrinsic for char access
        private extern char GetCharInternal(int index);

        public int CompareTo(Object value)
        {
            if (value == null) return 1;
            if (!(value is String s)) throw new ArgumentException();
            return CompareTo(s);
        }

        public int CompareTo(String strB)
        {
            if (strB == null) return 1;
            if (ReferenceEquals(this, strB)) return 0;
            return CompareOrdinal(this, strB);
        }

        public override bool Equals(Object obj) => obj is String s && Equals(s);

        public bool Equals(String value)
        {
            if (value == null) return false;
            if (ReferenceEquals(this, value)) return true;
            if (m_stringLength != value.m_stringLength) return false;
            return EqualsHelper(this, value);
        }

        public bool Equals(String value, StringComparison comparisonType)
        {
            if (comparisonType == StringComparison.OrdinalIgnoreCase)
                return EqualsIgnoreCase(value);
            return Equals(value);
        }

        private bool EqualsIgnoreCase(String value)
        {
            if (value == null) return false;
            if (m_stringLength != value.m_stringLength) return false;
            for (int i = 0; i < m_stringLength; i++)
            {
                if (Char.ToUpper(this[i]) != Char.ToUpper(value[i]))
                    return false;
            }
            return true;
        }

        public static bool Equals(String a, String b) => a?.Equals(b) ?? b == null;

        public static bool Equals(String a, String b, StringComparison comparisonType) =>
            a?.Equals(b, comparisonType) ?? b == null;

        private static bool EqualsHelper(String a, String b)
        {
            for (int i = 0; i < a.m_stringLength; i++)
            {
                if (a[i] != b[i]) return false;
            }
            return true;
        }

        public override int GetHashCode()
        {
            int hash = 5381;
            for (int i = 0; i < m_stringLength; i++)
            {
                hash = ((hash << 5) + hash) ^ this[i];
            }
            return hash;
        }

        public static int Compare(String strA, String strB) => strA?.CompareTo(strB) ?? (strB == null ? 0 : -1);

        public static int CompareOrdinal(String strA, String strB)
        {
            if (ReferenceEquals(strA, strB)) return 0;
            if (strA == null) return -1;
            if (strB == null) return 1;
            int len = Math.Min(strA.m_stringLength, strB.m_stringLength);
            for (int i = 0; i < len; i++)
            {
                int diff = strA[i] - strB[i];
                if (diff != 0) return diff;
            }
            return strA.m_stringLength - strB.m_stringLength;
        }

        public static String Concat(String str0, String str1)
        {
            if (IsNullOrEmpty(str0)) return str1 ?? Empty;
            if (IsNullOrEmpty(str1)) return str0;
            int len = str0.m_stringLength + str1.m_stringLength;
            char[] chars = new char[len];
            for (int i = 0; i < str0.m_stringLength; i++) chars[i] = str0[i];
            for (int i = 0; i < str1.m_stringLength; i++) chars[str0.m_stringLength + i] = str1[i];
            return new String(chars);
        }

        public static String Concat(String str0, String str1, String str2) =>
            Concat(Concat(str0, str1), str2);

        public static String Concat(params String[] values)
        {
            if (values == null || values.Length == 0) return Empty;
            String result = values[0];
            for (int i = 1; i < values.Length; i++)
                result = Concat(result, values[i]);
            return result;
        }

        public static String Concat(Object arg0) => arg0?.ToString() ?? Empty;
        public static String Concat(Object arg0, Object arg1) => Concat(arg0?.ToString(), arg1?.ToString());
        public static String Concat(Object arg0, Object arg1, Object arg2) => Concat(arg0?.ToString(), arg1?.ToString(), arg2?.ToString());
        public static String Concat(params Object[] args)
        {
            if (args == null || args.Length == 0) return Empty;
            String[] strings = new String[args.Length];
            for (int i = 0; i < args.Length; i++) strings[i] = args[i]?.ToString() ?? Empty;
            return Concat(strings);
        }

        public static String Concat(String str0, String str1, String str2, String str3) =>
            Concat(Concat(str0, str1), Concat(str2, str3));

        // String.Format - simplified implementation for string interpolation
        public static String Format(String format, params Object[] args)
        {
            if (format == null) throw new ArgumentNullException(nameof(format));
            if (args == null) return format;
            
            var result = new System.Text.StringBuilder(format.Length + args.Length * 8);
            int pos = 0;
            while (pos < format.Length)
            {
                char c = format[pos];
                if (c == '{')
                {
                    if (pos + 1 < format.Length && format[pos + 1] == '{')
                    {
                        result.Append('{');
                        pos += 2;
                    }
                    else
                    {
                        int end = format.IndexOf('}', pos);
                        if (end < 0) { result.Append(c); pos++; continue; }
                        String indexStr = format.Substring(pos + 1, end - pos - 1);
                        int colonPos = indexStr.IndexOf(':');
                        if (colonPos >= 0) indexStr = indexStr.Substring(0, colonPos);
                        int index = 0;
                        for (int i = 0; i < indexStr.Length; i++)
                            index = index * 10 + (indexStr[i] - '0');
                        if (index >= 0 && index < args.Length)
                            result.Append(args[index]?.ToString() ?? Empty);
                        pos = end + 1;
                    }
                }
                else if (c == '}')
                {
                    if (pos + 1 < format.Length && format[pos + 1] == '}')
                    {
                        result.Append('}');
                        pos += 2;
                    }
                    else
                    {
                        result.Append(c);
                        pos++;
                    }
                }
                else
                {
                    result.Append(c);
                    pos++;
                }
            }
            return result.ToString();
        }

        public static String Format(IFormatProvider provider, String format, params Object[] args) =>
            Format(format, args);

        public bool Contains(String value) => IndexOf(value) >= 0;

        public bool StartsWith(String value)
        {
            if (value == null) throw new ArgumentNullException(nameof(value));
            if (value.m_stringLength > m_stringLength) return false;
            for (int i = 0; i < value.m_stringLength; i++)
            {
                if (this[i] != value[i]) return false;
            }
            return true;
        }

        public bool EndsWith(String value)
        {
            if (value == null) throw new ArgumentNullException(nameof(value));
            if (value.m_stringLength > m_stringLength) return false;
            int offset = m_stringLength - value.m_stringLength;
            for (int i = 0; i < value.m_stringLength; i++)
            {
                if (this[offset + i] != value[i]) return false;
            }
            return true;
        }

        public int IndexOf(char value) => IndexOf(value, 0, m_stringLength);

        public int IndexOf(char value, int startIndex) => IndexOf(value, startIndex, m_stringLength - startIndex);

        public int IndexOf(char value, int startIndex, int count)
        {
            if (startIndex < 0 || startIndex > m_stringLength) throw new ArgumentOutOfRangeException(nameof(startIndex));
            if (count < 0 || startIndex > m_stringLength - count) throw new ArgumentOutOfRangeException(nameof(count));
            int end = startIndex + count;
            for (int i = startIndex; i < end; i++)
            {
                if (this[i] == value) return i;
            }
            return -1;
        }

        public int IndexOf(String value) => IndexOf(value, 0);

        public int IndexOf(String value, int startIndex)
        {
            if (value == null) throw new ArgumentNullException(nameof(value));
            if (value.m_stringLength == 0) return startIndex;
            for (int i = startIndex; i <= m_stringLength - value.m_stringLength; i++)
            {
                bool found = true;
                for (int j = 0; j < value.m_stringLength; j++)
                {
                    if (this[i + j] != value[j]) { found = false; break; }
                }
                if (found) return i;
            }
            return -1;
        }

        public int LastIndexOf(char value)
        {
            for (int i = m_stringLength - 1; i >= 0; i--)
            {
                if (this[i] == value) return i;
            }
            return -1;
        }

        public static bool IsNullOrEmpty(String value) => value == null || value.m_stringLength == 0;

        public static bool IsNullOrWhiteSpace(String value)
        {
            if (value == null) return true;
            for (int i = 0; i < value.m_stringLength; i++)
            {
                if (!Char.IsWhiteSpace(value[i])) return false;
            }
            return true;
        }

        public String Substring(int startIndex) => Substring(startIndex, m_stringLength - startIndex);

        public String Substring(int startIndex, int length)
        {
            if (startIndex < 0) throw new ArgumentOutOfRangeException(nameof(startIndex));
            if (length < 0 || startIndex > m_stringLength - length) throw new ArgumentOutOfRangeException(nameof(length));
            if (length == 0) return Empty;
            if (startIndex == 0 && length == m_stringLength) return this;
            char[] chars = new char[length];
            for (int i = 0; i < length; i++) chars[i] = this[startIndex + i];
            return new String(chars);
        }

        public String ToLower()
        {
            char[] chars = new char[m_stringLength];
            for (int i = 0; i < m_stringLength; i++) chars[i] = Char.ToLower(this[i]);
            return new String(chars);
        }

        public String ToUpper()
        {
            char[] chars = new char[m_stringLength];
            for (int i = 0; i < m_stringLength; i++) chars[i] = Char.ToUpper(this[i]);
            return new String(chars);
        }

        public String Trim() => Trim(' ', '\t', '\n', '\r');

        public String Trim(params char[] trimChars)
        {
            int start = 0, end = m_stringLength - 1;
            while (start <= end && Array.IndexOf(trimChars, this[start]) >= 0) start++;
            while (end >= start && Array.IndexOf(trimChars, this[end]) >= 0) end--;
            return Substring(start, end - start + 1);
        }

        public String Replace(char oldChar, char newChar)
        {
            char[] chars = new char[m_stringLength];
            for (int i = 0; i < m_stringLength; i++)
                chars[i] = this[i] == oldChar ? newChar : this[i];
            return new String(chars);
        }

        public String[] Split(params char[] separator) => Split(separator, Int32.MaxValue);

        public String[] Split(char[] separator, int count)
        {
            var list = new System.Collections.Generic.List<String>();
            int start = 0;
            for (int i = 0; i < m_stringLength && list.Count < count - 1; i++)
            {
                if (Array.IndexOf(separator, this[i]) >= 0)
                {
                    list.Add(Substring(start, i - start));
                    start = i + 1;
                }
            }
            list.Add(Substring(start));
            return list.ToArray();
        }

        public char[] ToCharArray()
        {
            char[] chars = new char[m_stringLength];
            for (int i = 0; i < m_stringLength; i++) chars[i] = this[i];
            return chars;
        }

        public Object Clone() => this;

        public override String ToString() => this;

        IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
        public IEnumerator<Char> GetEnumerator() => new CharEnumerator(this);

        public static bool operator ==(String a, String b) => Equals(a, b);
        public static bool operator !=(String a, String b) => !Equals(a, b);
    }

    // CharEnumerator (ECMA-335)
    public sealed class CharEnumerator : IEnumerator<Char>
    {
        private String _str;
        private int _index;
        internal CharEnumerator(String str) { _str = str; _index = -1; }
        public Char Current => _str[_index];
        Object IEnumerator.Current => Current;
        public bool MoveNext() => ++_index < _str.Length;
        public void Reset() => _index = -1;
        public void Dispose() { }
    }
}
