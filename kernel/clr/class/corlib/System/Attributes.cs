/*
 * ECMA-335 Attribute Types - Lux9 BCL
 * 
 * Attribute base class and common attributes.
 */
namespace System
{
    using System.Reflection;
    // Attribute (ECMA-335 IV.5.69)
    [AttributeUsage(AttributeTargets.All, Inherited = true, AllowMultiple = false)]
    public abstract class Attribute
    {
        protected Attribute() { }

        public override bool Equals(Object obj)
        {
            if (obj == null || GetType() != obj.GetType()) return false;
            return true; // Simplified - real impl compares fields
        }

        public override int GetHashCode() => GetType().GetHashCode();

        public virtual Object TypeId => GetType();

        public virtual bool IsDefaultAttribute() => false;

        public static Attribute GetCustomAttribute(MemberInfo element, Type attributeType) => null;
        public static Attribute GetCustomAttribute(MemberInfo element, Type attributeType, bool inherit) => null;
        public static Attribute[] GetCustomAttributes(MemberInfo element, bool inherit) => Array.Empty<Attribute>();
        public static Attribute[] GetCustomAttributes(MemberInfo element, Type type, bool inherit) => Array.Empty<Attribute>();
        public static bool IsDefined(MemberInfo element, Type attributeType, bool inherit) => false;
    }

    // AttributeTargets (ECMA-335 IV.5.70)
    [Flags]
    public enum AttributeTargets
    {
        Assembly = 0x0001,
        Module = 0x0002,
        Class = 0x0004,
        Struct = 0x0008,
        Enum = 0x0010,
        Constructor = 0x0020,
        Method = 0x0040,
        Property = 0x0080,
        Field = 0x0100,
        Event = 0x0200,
        Interface = 0x0400,
        Parameter = 0x0800,
        Delegate = 0x1000,
        ReturnValue = 0x2000,
        GenericParameter = 0x4000,
        All = 0x7FFF
    }

    // AttributeUsageAttribute (ECMA-335 IV.5.71)
    [AttributeUsage(AttributeTargets.Class, Inherited = true)]
    public sealed class AttributeUsageAttribute : Attribute
    {
        public AttributeUsageAttribute(AttributeTargets validOn)
        {
            ValidOn = validOn;
        }

        public AttributeTargets ValidOn { get; }
        public bool AllowMultiple { get; set; }
        public bool Inherited { get; set; } = true;
    }

    // CLSCompliantAttribute (ECMA-335 IV.5.72)
    [AttributeUsage(AttributeTargets.All, Inherited = true, AllowMultiple = false)]
    public sealed class CLSCompliantAttribute : Attribute
    {
        public CLSCompliantAttribute(bool isCompliant)
        {
            IsCompliant = isCompliant;
        }

        public bool IsCompliant { get; }
    }

    // FlagsAttribute (ECMA-335 IV.5.73)
    [AttributeUsage(AttributeTargets.Enum, Inherited = false)]
    public sealed class FlagsAttribute : Attribute
    {
        public FlagsAttribute() { }
    }

    // ObsoleteAttribute (ECMA-335 IV.5.74)
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum |
                    AttributeTargets.Constructor | AttributeTargets.Method | AttributeTargets.Property |
                    AttributeTargets.Field | AttributeTargets.Event | AttributeTargets.Interface |
                    AttributeTargets.Delegate, Inherited = false)]
    public sealed class ObsoleteAttribute : Attribute
    {
        public ObsoleteAttribute() { }
        public ObsoleteAttribute(String message) { Message = message; }
        public ObsoleteAttribute(String message, bool error) { Message = message; IsError = error; }
        public String Message { get; }
        public bool IsError { get; }
    }

    // ParamArrayAttribute (ECMA-335 IV.5.75)
    [AttributeUsage(AttributeTargets.Parameter, Inherited = true, AllowMultiple = false)]
    public sealed class ParamArrayAttribute : Attribute
    {
        public ParamArrayAttribute() { }
    }

    // ThreadStaticAttribute (ECMA-335)
    [AttributeUsage(AttributeTargets.Field, Inherited = false)]
    public sealed class ThreadStaticAttribute : Attribute
    {
        public ThreadStaticAttribute() { }
    }

    // SerializableAttribute (for compatibility)
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Delegate, Inherited = false)]
    public sealed class SerializableAttribute : Attribute
    {
        public SerializableAttribute() { }
    }

    // NonSerializedAttribute
    [AttributeUsage(AttributeTargets.Field, Inherited = false)]
    public sealed class NonSerializedAttribute : Attribute
    {
        public NonSerializedAttribute() { }
    }
}

namespace System.Diagnostics
{
    // ConditionalAttribute (ECMA-335 IV.5.76)
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Class, AllowMultiple = true)]
    public sealed class ConditionalAttribute : Attribute
    {
        public ConditionalAttribute(String conditionString)
        {
            ConditionString = conditionString;
        }

        public String ConditionString { get; }
    }
}
