namespace System
{
    using System.Runtime.CompilerServices;

    public sealed class String : Object
    {
        // Internal length field - layout matches kernel expectation
        [NonSerialized]
        private int _stringLength;

        // First char - this is where the string data starts in memory
        [NonSerialized]
        private char _firstChar;

        // Empty string constant
        public static readonly string Empty = "";
        
        // Public Length property mapped to internal call or intrinsic
        public int Length
        {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
        }

        // Indexer mapped to internal call
        public extern char this[int index]
        {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
        }

        // Constructor from char array
        public String(char[] value)
        {
            // InternalCall to allocate and copy chars
            // Stub for now - actual implementation in runtime
        }
        
        public String(char[] value, int startIndex, int length)
        {
            // InternalCall to allocate and copy chars subset
        }
        
        public String(char c, int count)
        {
            // Fill string with repeated char
        }

        public static string Concat(string str0, string str1)
        {
            return Internal_Concat2(str0, str1);
        }

        public static string Concat(string str0, string str1, string str2)
        {
            return Internal_Concat3(str0, str1, str2);
        }

        public static string Concat(params string[] values)
        {
            if (values == null || values.Length == 0) return Empty;
            string result = values[0] ?? Empty;
            for (int i = 1; i < values.Length; i++)
            {
                result = Concat(result, values[i] ?? Empty);
            }
            return result;
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern string Internal_Concat2(string str0, string str1);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern string Internal_Concat3(string str0, string str1, string str2);

        public override string ToString() => this;

        public static bool operator ==(String a, String b)
        {
            return Equals(a, b);
        }

        public static bool operator !=(String a, String b) => !(a == b);

        public override bool Equals(object obj)
        {
             if (obj is string str)
                 return Equals(this, str);
             return false;
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool Equals(string a, string b);
        
        // String comparison and search methods
        public static bool IsNullOrEmpty(string value)
        {
            return value == null || value.Length == 0;
        }
        
        public static bool IsNullOrWhiteSpace(string value)
        {
            if (value == null) return true;
            for (int i = 0; i < value.Length; i++)
            {
                if (!char.IsWhiteSpace(value[i]))
                    return false;
            }
            return true;
        }
        
        public bool Contains(string value)
        {
            return IndexOf(value) >= 0;
        }
        
        public bool Contains(char value)
        {
            return IndexOf(value) >= 0;
        }
        
        public bool StartsWith(string value)
        {
            if (value == null) return false;
            if (value.Length > Length) return false;
            for (int i = 0; i < value.Length; i++)
            {
                if (this[i] != value[i]) return false;
            }
            return true;
        }
        
        public bool EndsWith(string value)
        {
            if (value == null) return false;
            if (value.Length > Length) return false;
            int offset = Length - value.Length;
            for (int i = 0; i < value.Length; i++)
            {
                if (this[offset + i] != value[i]) return false;
            }
            return true;
        }
        
        public int IndexOf(char value)
        {
            return IndexOf(value, 0);
        }
        
        public int IndexOf(char value, int startIndex)
        {
            for (int i = startIndex; i < Length; i++)
            {
                if (this[i] == value) return i;
            }
            return -1;
        }
        
        public int IndexOf(string value)
        {
            return IndexOf(value, 0);
        }
        
        public int IndexOf(string value, int startIndex)
        {
            if (value == null || value.Length == 0) return startIndex;
            if (value.Length > Length - startIndex) return -1;
            
            for (int i = startIndex; i <= Length - value.Length; i++)
            {
                bool found = true;
                for (int j = 0; j < value.Length; j++)
                {
                    if (this[i + j] != value[j])
                    {
                        found = false;
                        break;
                    }
                }
                if (found) return i;
            }
            return -1;
        }
        
        public int LastIndexOf(char value)
        {
            for (int i = Length - 1; i >= 0; i--)
            {
                if (this[i] == value) return i;
            }
            return -1;
        }
        
        public string Substring(int startIndex)
        {
            return Substring(startIndex, Length - startIndex);
        }
        
        public string Substring(int startIndex, int length)
        {
            if (startIndex < 0 || startIndex > Length) throw new ArgumentOutOfRangeException("startIndex");
            if (length < 0 || startIndex + length > Length) throw new ArgumentOutOfRangeException("length");
            if (length == 0) return Empty;
            if (startIndex == 0 && length == Length) return this;
            
            char[] chars = new char[length];
            for (int i = 0; i < length; i++)
            {
                chars[i] = this[startIndex + i];
            }
            return new string(chars);
        }
        
        public string[] Split(char separator)
        {
            return Split(new char[] { separator });
        }
        
        public string[] Split(char[] separator)
        {
            // Count occurrences first
            int count = 1;
            for (int i = 0; i < Length; i++)
            {
                for (int j = 0; j < separator.Length; j++)
                {
                    if (this[i] == separator[j])
                    {
                        count++;
                        break;
                    }
                }
            }
            
            string[] result = new string[count];
            int start = 0;
            int idx = 0;
            
            for (int i = 0; i < Length; i++)
            {
                bool isSeparator = false;
                for (int j = 0; j < separator.Length; j++)
                {
                    if (this[i] == separator[j])
                    {
                        isSeparator = true;
                        break;
                    }
                }
                if (isSeparator)
                {
                    result[idx++] = Substring(start, i - start);
                    start = i + 1;
                }
            }
            result[idx] = Substring(start, Length - start);
            return result;
        }
        
        public static string Join(string separator, string[] values)
        {
            if (values == null || values.Length == 0) return Empty;
            
            string result = values[0] ?? Empty;
            for (int i = 1; i < values.Length; i++)
            {
                result = Concat(result, separator ?? Empty, values[i] ?? Empty);
            }
            return result;
        }
        
        public string Replace(char oldChar, char newChar)
        {
            char[] chars = new char[Length];
            for (int i = 0; i < Length; i++)
            {
                chars[i] = this[i] == oldChar ? newChar : this[i];
            }
            return new string(chars);
        }
        
        public string Replace(string oldValue, string newValue)
        {
            if (IsNullOrEmpty(oldValue)) return this;
            if (newValue == null) newValue = Empty;
            
            System.Text.StringBuilder sb = new System.Text.StringBuilder();
            int start = 0;
            int idx;
            
            while ((idx = IndexOf(oldValue, start)) >= 0)
            {
                sb.Append(Substring(start, idx - start));
                sb.Append(newValue);
                start = idx + oldValue.Length;
            }
            sb.Append(Substring(start));
            return sb.ToString();
        }
        
        public string ToLower()
        {
            char[] chars = new char[Length];
            for (int i = 0; i < Length; i++)
            {
                char c = this[i];
                if (c >= 'A' && c <= 'Z')
                    chars[i] = (char)(c + 32);
                else
                    chars[i] = c;
            }
            return new string(chars);
        }
        
        public string ToUpper()
        {
            char[] chars = new char[Length];
            for (int i = 0; i < Length; i++)
            {
                char c = this[i];
                if (c >= 'a' && c <= 'z')
                    chars[i] = (char)(c - 32);
                else
                    chars[i] = c;
            }
            return new string(chars);
        }
        
        public string Trim()
        {
            int start = 0;
            int end = Length - 1;
            while (start <= end && char.IsWhiteSpace(this[start])) start++;
            while (end >= start && char.IsWhiteSpace(this[end])) end--;
            return Substring(start, end - start + 1);
        }
        
        public string TrimStart()
        {
            int start = 0;
            while (start < Length && char.IsWhiteSpace(this[start])) start++;
            return Substring(start);
        }
        
        public string TrimEnd()
        {
            int end = Length - 1;
            while (end >= 0 && char.IsWhiteSpace(this[end])) end--;
            return Substring(0, end + 1);
        }
        
        public char[] ToCharArray()
        {
            char[] chars = new char[Length];
            for (int i = 0; i < Length; i++)
            {
                chars[i] = this[i];
            }
            return chars;
        }
        
        public override int GetHashCode()
        {
            // FNV-1a hash
            int hash = unchecked((int)2166136261);
            for (int i = 0; i < Length; i++)
            {
                hash ^= this[i];
                hash *= 16777619;
            }
            return hash;
        }
        
        // IComparable support
        public int CompareTo(string other)
        {
            if (other == null) return 1;
            int len = Length < other.Length ? Length : other.Length;
            for (int i = 0; i < len; i++)
            {
                if (this[i] != other[i])
                    return this[i] - other[i];
            }
            return Length - other.Length;
        }
    }
    
    // Char helper methods
    public partial struct Char
    {
        public static bool IsWhiteSpace(char c)
        {
            return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
        }
        
        public static bool IsDigit(char c)
        {
            return c >= '0' && c <= '9';
        }
        
        public static bool IsLetter(char c)
        {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        }
        
        public static bool IsLetterOrDigit(char c)
        {
            return IsLetter(c) || IsDigit(c);
        }
        
        public static bool IsUpper(char c)
        {
            return c >= 'A' && c <= 'Z';
        }
        
        public static bool IsLower(char c)
        {
            return c >= 'a' && c <= 'z';
        }
        
        public static char ToLower(char c)
        {
            if (c >= 'A' && c <= 'Z')
                return (char)(c + 32);
            return c;
        }
        
        public static char ToUpper(char c)
        {
            if (c >= 'a' && c <= 'z')
                return (char)(c - 32);
            return c;
        }
    }
}

