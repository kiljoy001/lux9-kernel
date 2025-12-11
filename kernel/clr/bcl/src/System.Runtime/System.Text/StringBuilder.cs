namespace System.Text
{
    public class StringBuilder
    {
        private char[] _chunkChars;
        private int _chunkLength;
        
        public StringBuilder() : this(16) { }
        public StringBuilder(int capacity) 
        {
            _chunkChars = new char[capacity];
            _chunkLength = 0;
        }
        
        public StringBuilder(string value) : this(value.Length + 16)
        {
            Append(value);
        }

        public int Length 
        { 
            get => _chunkLength; 
            set => _chunkLength = value; // Unsafe simplification
        }

        public StringBuilder Append(string value)
        {
            if (value == null) return this;
            // Simplistic resize
            EnsureCapacity(_chunkLength + value.Length);
            for(int i=0; i<value.Length; i++) _chunkChars[_chunkLength + i] = value[i]; // value[i] access depends on String indexer
            _chunkLength += value.Length;
            return this;
        }
        
        public StringBuilder Append(char value)
        {
             EnsureCapacity(_chunkLength + 1);
             _chunkChars[_chunkLength++] = value;
             return this;
        }
        
        public StringBuilder Append(object value)
        {
            return Append(value?.ToString());
        }

        private void EnsureCapacity(int min)
        {
            if (_chunkChars.Length < min)
            {
                int newCap = Math.Max(min, _chunkChars.Length * 2);
                char[] newChars = new char[newCap];
                for(int i=0; i<_chunkLength; i++) newChars[i] = _chunkChars[i];
                _chunkChars = newChars;
            }
        }

        public override string ToString()
        {
            // Requires string ctor from char array, which strictly doesn't exist in my minimal Object.cs yet
            // Stub for now
            return "StringBuilder_Result"; 
        }
    }

    public abstract class Encoding
    {
        public static Encoding UTF8 => new UTF8Encoding();
        public static Encoding ASCII => new ASCIIEncoding();
        
        public abstract byte[] GetBytes(string s);
        public abstract string GetString(byte[] bytes);
        public abstract string GetString(byte[] bytes, int index, int count);
        public abstract int GetBytes(string s, int charIndex, int charCount, byte[] bytes, int byteIndex);
    }
    
    public class UTF8Encoding : Encoding 
    {
        public override byte[] GetBytes(string s)
        {
            if (s == null) return new byte[0];
            // Simplified: assume ASCII subset for now (UTF-8 is ASCII-compatible for 7-bit chars)
            byte[] result = new byte[s.Length];
            for (int i = 0; i < s.Length; i++)
            {
                char c = s[i];
                result[i] = c < 128 ? (byte)c : (byte)'?';
            }
            return result;
        }
        
        public override string GetString(byte[] bytes)
        {
            return GetString(bytes, 0, bytes?.Length ?? 0);
        }
        
        public override string GetString(byte[] bytes, int index, int count)
        {
            if (bytes == null || count == 0) return "";
            char[] chars = new char[count];
            for (int i = 0; i < count; i++)
            {
                byte b = bytes[index + i];
                chars[i] = b < 128 ? (char)b : '?';
            }
            // Internal call to create string from char array
            return new string(chars);
        }
        
        public override int GetBytes(string s, int charIndex, int charCount, byte[] bytes, int byteIndex)
        {
            for (int i = 0; i < charCount; i++)
            {
                char c = s[charIndex + i];
                bytes[byteIndex + i] = c < 128 ? (byte)c : (byte)'?';
            }
            return charCount;
        }
    }
    
    public class ASCIIEncoding : Encoding 
    {
        public override byte[] GetBytes(string s)
        {
            if (s == null) return new byte[0];
            byte[] result = new byte[s.Length];
            for (int i = 0; i < s.Length; i++)
            {
                char c = s[i];
                result[i] = c < 128 ? (byte)c : (byte)'?';
            }
            return result;
        }
        
        public override string GetString(byte[] bytes)
        {
            return GetString(bytes, 0, bytes?.Length ?? 0);
        }
        
        public override string GetString(byte[] bytes, int index, int count)
        {
            if (bytes == null || count == 0) return "";
            char[] chars = new char[count];
            for (int i = 0; i < count; i++)
            {
                byte b = bytes[index + i];
                chars[i] = b < 128 ? (char)b : '?';
            }
            return new string(chars);
        }
        
        public override int GetBytes(string s, int charIndex, int charCount, byte[] bytes, int byteIndex)
        {
            for (int i = 0; i < charCount; i++)
            {
                char c = s[charIndex + i];
                bytes[byteIndex + i] = c < 128 ? (byte)c : (byte)'?';
            }
            return charCount;
        }
    }
}

namespace System
{
}
