namespace System.Runtime.CompilerServices
{
    public static class RuntimeHelpers
    {
        public static bool ReferenceEquals(object o1, object o2)
        {
            return o1 == o2; // In C#, == on objects is ref equality unless overloaded
        }
        
        public static void InitializeArray(Array array, RuntimeFieldHandle fldHandle) { }
        public static int GetHashCode(object o) { return 0; }
    }
}
