/*
 * ECMA-335 Runtime Helpers and Compiler Services - Lux9 BCL
 * 
 * Internal types used by the runtime and compiler.
 */
namespace System
{
    using System.Reflection;
    // Type (ECMA-335 IV.5.77) - base for reflection
    public abstract class Type : MemberInfo
    {
        protected Type() { }

        public abstract override String Name { get; }
        public abstract String FullName { get; }
        public abstract String Namespace { get; }
        public abstract Type BaseType { get; }
        public abstract Assembly Assembly { get; }
        public abstract Module Module { get; }
        public virtual String AssemblyQualifiedName => $"{FullName}, {Assembly?.FullName}";
        public abstract Guid GUID { get; }

        public virtual bool IsAbstract => (GetAttributeFlagsImpl() & TypeAttributes.Abstract) != 0;
        public virtual bool IsSealed => (GetAttributeFlagsImpl() & TypeAttributes.Sealed) != 0;
        public virtual bool IsClass => (GetAttributeFlagsImpl() & TypeAttributes.ClassSemanticsMask) == TypeAttributes.Class && !IsValueType;
        public virtual bool IsInterface => (GetAttributeFlagsImpl() & TypeAttributes.ClassSemanticsMask) == TypeAttributes.Interface;
        public bool IsValueType => IsValueTypeImpl();
        public bool IsEnum => GetType().IsSubclassOf(typeof(Enum));
        public bool IsArray => IsArrayImpl();
        public bool IsByRef => IsByRefImpl();
        public bool IsPointer => IsPointerImpl();
        public bool IsPrimitive => IsPrimitiveImpl();

        protected abstract bool IsValueTypeImpl();
        protected abstract TypeAttributes GetAttributeFlagsImpl();
        protected abstract bool IsArrayImpl();
        protected abstract bool IsByRefImpl();
        protected abstract bool IsPointerImpl();
        protected abstract bool IsPrimitiveImpl();
        public abstract Type GetElementType();

        public virtual bool IsGenericType => false;
        public virtual bool IsGenericTypeDefinition => false;
        public virtual bool ContainsGenericParameters => false;
        public virtual Type[] GetGenericArguments() => Array.Empty<Type>();
        public virtual Type GetGenericTypeDefinition() => throw new InvalidOperationException();
        public virtual Type MakeGenericType(params Type[] typeArguments) => throw new InvalidOperationException();

        public virtual bool IsSubclassOf(Type c)
        {
            Type p = this;
            if (p == c) return false;
            while (p != null)
            {
                if (p == c) return true;
                p = p.BaseType;
            }
            return false;
        }

        public virtual bool IsAssignableFrom(Type c)
        {
            if (c == null) return false;
            if (this == c) return true;
            if (c.IsSubclassOf(this)) return true;
            if (IsInterface)
            {
                Type[] interfaces = c.GetInterfaces();
                for (int i = 0; i < interfaces.Length; i++)
                    if (interfaces[i] == this) return true;
            }
            return false;
        }

        public abstract ConstructorInfo[] GetConstructors(BindingFlags bindingAttr);
        public ConstructorInfo[] GetConstructors() => GetConstructors(BindingFlags.Public | BindingFlags.Instance);
        public abstract MethodInfo[] GetMethods(BindingFlags bindingAttr);
        public MethodInfo[] GetMethods() => GetMethods(BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
        public abstract PropertyInfo[] GetProperties(BindingFlags bindingAttr);
        public PropertyInfo[] GetProperties() => GetProperties(BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
        public abstract FieldInfo[] GetFields(BindingFlags bindingAttr);
        public FieldInfo[] GetFields() => GetFields(BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
        public abstract Type[] GetInterfaces();
        public abstract Type GetInterface(String name, bool ignoreCase);
        public Type GetInterface(String name) => GetInterface(name, false);
        public abstract EventInfo[] GetEvents(BindingFlags bindingAttr);
        public abstract MemberInfo[] GetMembers(BindingFlags bindingAttr);
        public abstract Type[] GetNestedTypes(BindingFlags bindingAttr);

        public MethodInfo GetMethod(String name) => GetMethod(name, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
        public MethodInfo GetMethod(String name, BindingFlags bindingAttr) => GetMethodImpl(name, bindingAttr, null, CallingConventions.Any, null, null);
        public MethodInfo GetMethod(String name, Type[] types) => GetMethodImpl(name, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static, null, CallingConventions.Any, types, null);
        protected abstract MethodInfo GetMethodImpl(String name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers);

        public ConstructorInfo GetConstructor(Type[] types) => GetConstructorImpl(BindingFlags.Public | BindingFlags.Instance, null, CallingConventions.Any, types, null);
        protected abstract ConstructorInfo GetConstructorImpl(BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers);

        public PropertyInfo GetProperty(String name) => GetPropertyImpl(name, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static, null, null, null, null);
        public PropertyInfo GetProperty(String name, Type returnType) => GetPropertyImpl(name, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static, null, returnType, null, null);
        protected abstract PropertyInfo GetPropertyImpl(String name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers);

        public FieldInfo GetField(String name) => GetField(name, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
        public abstract FieldInfo GetField(String name, BindingFlags bindingAttr);

        public abstract EventInfo GetEvent(String name, BindingFlags bindingAttr);
        public EventInfo GetEvent(String name) => GetEvent(name, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);

        public abstract Type GetNestedType(String name, BindingFlags bindingAttr);

        public abstract Object InvokeMember(String name, BindingFlags invokeAttr, Binder binder, Object target, Object[] args,
            ParameterModifier[] modifiers, System.Globalization.CultureInfo culture, String[] namedParameters);

        public Object InvokeMember(String name, BindingFlags invokeAttr, Binder binder, Object target, Object[] args) =>
            InvokeMember(name, invokeAttr, binder, target, args, null, null, null);

        public virtual Type UnderlyingSystemType => this;

        public static Type GetType(String typeName) => null; // Runtime-implemented
        public static Type GetType(String typeName, bool throwOnError) => null; // Runtime-implemented
        public static Type GetTypeFromHandle(RuntimeTypeHandle handle) => null; // Runtime-implemented

        public override String ToString() => FullName;

        public static bool operator ==(Type left, Type right) => ReferenceEquals(left, right) || (left?.Equals(right) ?? false);
        public static bool operator !=(Type left, Type right) => !(left == right);
        public override bool Equals(Object o) => o is Type t && GUID == t.GUID;
        public override int GetHashCode() => GUID.GetHashCode();
    }

    // RuntimeTypeHandle (ECMA-335)
    public struct RuntimeTypeHandle
    {
        internal IntPtr m_type;
        public IntPtr Value => m_type;
        public override bool Equals(Object obj) => obj is RuntimeTypeHandle h && m_type == h.m_type;
        public bool Equals(RuntimeTypeHandle handle) => m_type == handle.m_type;
        public override int GetHashCode() => m_type.GetHashCode();
    }

    // TypeAttributes (ECMA-335)
    [Flags]
    public enum TypeAttributes
    {
        VisibilityMask = 0x00000007,
        NotPublic = 0x00000000,
        Public = 0x00000001,
        NestedPublic = 0x00000002,
        NestedPrivate = 0x00000003,
        NestedFamily = 0x00000004,
        NestedAssembly = 0x00000005,
        NestedFamANDAssem = 0x00000006,
        NestedFamORAssem = 0x00000007,
        LayoutMask = 0x00000018,
        AutoLayout = 0x00000000,
        SequentialLayout = 0x00000008,
        ExplicitLayout = 0x00000010,
        ClassSemanticsMask = 0x00000020,
        Class = 0x00000000,
        Interface = 0x00000020,
        Abstract = 0x00000080,
        Sealed = 0x00000100,
        SpecialName = 0x00000400,
        Import = 0x00001000,
        Serializable = 0x00002000,
        StringFormatMask = 0x00030000,
        AnsiClass = 0x00000000,
        UnicodeClass = 0x00010000,
        AutoClass = 0x00020000,
        BeforeFieldInit = 0x00100000,
        RTSpecialName = 0x00000800,
        HasSecurity = 0x00040000,
    }

    // Enum (ECMA-335 IV.5.78)
    public abstract class Enum : ValueType, IComparable, IFormattable
    {
        protected Enum() { }

        public int CompareTo(Object target)
        {
            if (target == null) return 1;
            if (GetType() != target.GetType())
                throw new ArgumentException("Object must be the same type as the enum");
            return GetValue().CompareTo(((Enum)target).GetValue());
        }

        private int GetValue() => System.Runtime.CompilerServices.RuntimeHelpers.EnumToInt(this);

        public override bool Equals(Object obj)
        {
            if (obj == null || GetType() != obj.GetType()) return false;
            return GetValue() == ((Enum)obj).GetValue();
        }

        public override int GetHashCode() => GetValue();

        public static String GetName(Type enumType, Object value) => null; // Runtime-implemented
        public static String[] GetNames(Type enumType) => Array.Empty<String>(); // Runtime-implemented
        public static Array GetValues(Type enumType) => null; // Runtime-implemented
        public static bool IsDefined(Type enumType, Object value) => false; // Runtime-implemented
        public static Object Parse(Type enumType, String value) => Parse(enumType, value, false);
        public static Object Parse(Type enumType, String value, bool ignoreCase) => null; // Runtime-implemented
        public static bool TryParse<TEnum>(String value, out TEnum result) where TEnum : struct
        {
            result = default;
            return false; // Simplified
        }

        public bool HasFlag(Enum flag)
        {
            int thisValue = GetValue();
            int flagValue = flag.GetValue();
            return (thisValue & flagValue) == flagValue;
        }

        public override String ToString() => GetValue().ToString();
        public String ToString(String format, IFormatProvider formatProvider) => ToString();
        public String ToString(String format) => ToString();
    }
}

namespace System.Runtime.CompilerServices
{
    // RuntimeHelpers (ECMA-335)
    public static class RuntimeHelpers
    {
        public static int OffsetToStringData => 0; // Runtime-specific
        public static bool IsReferenceOrContainsReferences<T>() => !typeof(T).IsValueType;
        public static int GetHashCode(Object o) => o?.GetType().GetHashCode() ?? 0; // Runtime intrinsic
        public static Type GetType(Object o) => null; // Runtime intrinsic
        public static Object MemberwiseClone(Object o) => null; // Runtime intrinsic
        public static bool ReferenceEquals(Object objA, Object objB) => (Object)(objA) == (Object)(objB); // Runtime intrinsic
        internal static bool ValueTypeEquals(Object a, Object b) => true; // Runtime intrinsic
        internal static int EnumToInt(Enum e) => 0; // Runtime intrinsic
        public static void InitializeArray(Array array, RuntimeFieldHandle fldHandle) { } // Runtime intrinsic
        public static void RunClassConstructor(RuntimeTypeHandle type) { } // Runtime intrinsic
        public static void EnsureSufficientExecutionStack() { } // Runtime intrinsic
        public static bool TryEnsureSufficientExecutionStack() => true;
    }

    // MethodImplAttribute (ECMA-335)
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor, Inherited = false)]
    public sealed class MethodImplAttribute : Attribute
    {
        public MethodImplAttribute() { }
        public MethodImplAttribute(MethodImplOptions methodImplOptions) { Value = methodImplOptions; }
        public MethodImplAttribute(short value) { Value = (MethodImplOptions)value; }
        public MethodImplOptions Value { get; }
        public MethodCodeType MethodCodeType;
    }

    [Flags]
    public enum MethodImplOptions
    {
        Unmanaged = 0x0004,
        NoInlining = 0x0008,
        ForwardRef = 0x0010,
        Synchronized = 0x0020,
        NoOptimization = 0x0040,
        PreserveSig = 0x0080,
        AggressiveInlining = 0x0100,
        AggressiveOptimization = 0x0200,
        InternalCall = 0x1000
    }

    public enum MethodCodeType
    {
        IL = 0,
        Native = 1,
        OPTIL = 2,
        Runtime = 3
    }

    // CompilerGeneratedAttribute
    [AttributeUsage(AttributeTargets.All, Inherited = true)]
    public sealed class CompilerGeneratedAttribute : Attribute
    {
        public CompilerGeneratedAttribute() { }
    }

    // IndexerNameAttribute
    [AttributeUsage(AttributeTargets.Property, Inherited = true)]
    public sealed class IndexerNameAttribute : Attribute
    {
        public IndexerNameAttribute(String indexerName) { }
    }

    // Unsafe class
    public static unsafe class Unsafe
    {
        public static ref T Add<T>(ref T source, int elementOffset) => ref source;
        public static ref T AsRef<T>(in T source) => ref System.Runtime.CompilerServices.Unsafe.AsRef(in source);
        public static T As<T>(Object o) where T : class => (T)o;
        public static ref TTo As<TFrom, TTo>(ref TFrom source) => ref Unsafe.As<TFrom, TTo>(ref source);
        public static int SizeOf<T>() => sizeof(T);
        public static void* AsPointer<T>(ref T value) => null;
        public static ref T AsRef<T>(void* source) => ref Unsafe.AsRef<T>(source);
        public static bool IsNullRef<T>(ref T source) => false;
    }

    // InternalsVisibleToAttribute
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple = true, Inherited = false)]
    public sealed class InternalsVisibleToAttribute : Attribute
    {
        public InternalsVisibleToAttribute(String assemblyName) { AssemblyName = assemblyName; }
        public String AssemblyName { get; }
        public bool AllVisibleTo { get; set; }
    }

    // RuntimeFieldHandle
    public struct RuntimeFieldHandle
    {
        internal IntPtr m_ptr;
        public IntPtr Value => m_ptr;
    }

    // RuntimeMethodHandle
    public struct RuntimeMethodHandle
    {
        internal IntPtr m_value;
        public IntPtr Value => m_value;
    }
}
