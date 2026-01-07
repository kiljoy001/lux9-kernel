namespace System.Runtime.CompilerServices
{
    // Required for init properties (C# 9)
    // Allows properties to be set during object initialization but not afterwards
    public sealed class IsExternalInit { }
    
    // Required for required keyword (C# 11)
    // Marks properties/fields as mandatory in constructors
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Property | AttributeTargets.Field, Inherited = false)]
    public sealed class RequiredMemberAttribute : Attribute { }
    
    // Required for ref fields (C# 11)
    // Enables ref struct fields with lifetime safety guarantees
    [AttributeUsage(AttributeTargets.Module, AllowMultiple = false, Inherited = false)]
    public sealed class RefSafetyRulesAttribute : Attribute
    {
        public RefSafetyRulesAttribute(int version)
        {
            Version = version;
        }
        
        public int Version { get; }
    }
    
    // Required for inline arrays (C# 12)
    // Enables fixed-size arrays in structs with array-like indexing
    [AttributeUsage(AttributeTargets.Struct, AllowMultiple = false)]
    public sealed class InlineArrayAttribute : Attribute
    {
        public InlineArrayAttribute(int length)
        {
            Length = length;
        }
        
        public int Length { get; }
    }
    
    // Required for collection expressions (C# 12)
    [AttributeUsage(AttributeTargets.All, AllowMultiple = true, Inherited = false)]
    public sealed class CollectionBuilderAttribute : Attribute
    {
        public CollectionBuilderAttribute(Type builderType, string methodName)
        {
            BuilderType = builderType;
            MethodName = methodName;
        }
        
        public Type BuilderType { get; }
        public string MethodName { get; }
    }
}
