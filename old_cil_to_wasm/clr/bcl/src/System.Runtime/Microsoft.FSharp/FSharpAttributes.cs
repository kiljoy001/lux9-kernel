namespace Microsoft.FSharp.Core
{
    using System;

    // F# Compiler Attributes
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct, AllowMultiple = false)]
    public class StructAttribute : Attribute
    {
    }

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Module, AllowMultiple = false)]
    public class AutoOpenAttribute : Attribute
    {
        public AutoOpenAttribute() { }
        public AutoOpenAttribute(string path) { }
    }

    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false)]
    public class AbstractClassAttribute : Attribute
    {
    }

    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false)]
    public class SealedAttribute : Attribute
    {
    }

    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Property, AllowMultiple = false)]
    public sealed class InlineAttribute : Attribute { }

    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
    public sealed class EntryPointAttribute : Attribute { }
}
