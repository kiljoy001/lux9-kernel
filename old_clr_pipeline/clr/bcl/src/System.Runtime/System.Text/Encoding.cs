namespace System.Text
{
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
        // ... (existing content logic is fine, just adding classes at the end of namespace)
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

    public abstract class Decoder { }
    public abstract class Encoder { }
}
