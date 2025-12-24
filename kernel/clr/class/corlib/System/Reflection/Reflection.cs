/*
 * ECMA-335 Reflection Types - Lux9 BCL
 * 
 * Reflection type hierarchy per CLI specification.
 */
namespace System.Reflection
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.CompilerServices;

    // MemberInfo (ECMA-335)
    public abstract class MemberInfo
    {
        protected MemberInfo() { }
        public abstract String Name { get; }
        public abstract Type DeclaringType { get; }
        public abstract Type ReflectedType { get; }
        public abstract MemberTypes MemberType { get; }
        public virtual int MetadataToken => 0;
        public virtual Module Module => DeclaringType?.Module;
        public abstract Object[] GetCustomAttributes(bool inherit);
        public abstract Object[] GetCustomAttributes(Type attributeType, bool inherit);
        public abstract bool IsDefined(Type attributeType, bool inherit);
        public virtual IList<CustomAttributeData> GetCustomAttributesData() => Array.Empty<CustomAttributeData>();
    }

    // MemberTypes (ECMA-335)
    [Flags]
    public enum MemberTypes
    {
        Constructor = 0x01,
        Event = 0x02,
        Field = 0x04,
        Method = 0x08,
        Property = 0x10,
        TypeInfo = 0x20,
        Custom = 0x40,
        NestedType = 0x80,
        All = 0xBF
    }

    // MethodBase (ECMA-335)
    public abstract class MethodBase : MemberInfo
    {
        protected MethodBase() { }
        public abstract MethodAttributes Attributes { get; }
        public abstract MethodImplAttributes GetMethodImplementationFlags();
        public abstract ParameterInfo[] GetParameters();
        public abstract Object Invoke(Object obj, BindingFlags invokeAttr, Binder binder, Object[] parameters, System.Globalization.CultureInfo culture);
        public Object Invoke(Object obj, Object[] parameters) => Invoke(obj, BindingFlags.Default, null, parameters, null);
        public virtual RuntimeMethodHandle MethodHandle => default;
        public virtual CallingConventions CallingConvention => CallingConventions.Standard;
        public bool IsPublic => (Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.Public;
        public bool IsPrivate => (Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.Private;
        public bool IsStatic => (Attributes & MethodAttributes.Static) != 0;
        public bool IsVirtual => (Attributes & MethodAttributes.Virtual) != 0;
        public bool IsAbstract => (Attributes & MethodAttributes.Abstract) != 0;
        public bool IsFinal => (Attributes & MethodAttributes.Final) != 0;
        public bool IsConstructor => this is ConstructorInfo;
        public bool IsGenericMethod => false;
        public bool IsGenericMethodDefinition => false;
        public virtual Type[] GetGenericArguments() => Array.Empty<Type>();
    }

    // MethodInfo (ECMA-335)
    public abstract class MethodInfo : MethodBase
    {
        protected MethodInfo() { }
        public override MemberTypes MemberType => MemberTypes.Method;
        public abstract Type ReturnType { get; }
        public abstract ICustomAttributeProvider ReturnTypeCustomAttributes { get; }
        public abstract MethodInfo GetBaseDefinition();
        public virtual Delegate CreateDelegate(Type delegateType) => null;
        public virtual Delegate CreateDelegate(Type delegateType, Object target) => null;
    }

    // ConstructorInfo (ECMA-335)
    public abstract class ConstructorInfo : MethodBase
    {
        protected ConstructorInfo() { }
        public override MemberTypes MemberType => MemberTypes.Constructor;
        public static readonly String ConstructorName = ".ctor";
        public static readonly String TypeConstructorName = ".cctor";
        public Object Invoke(Object[] parameters) => Invoke(BindingFlags.Default, null, parameters, null);
        public abstract Object Invoke(BindingFlags invokeAttr, Binder binder, Object[] parameters, System.Globalization.CultureInfo culture);
    }

    // FieldInfo (ECMA-335)
    public abstract class FieldInfo : MemberInfo
    {
        protected FieldInfo() { }
        public override MemberTypes MemberType => MemberTypes.Field;
        public abstract Type FieldType { get; }
        public abstract FieldAttributes Attributes { get; }
        public abstract RuntimeFieldHandle FieldHandle { get; }
        public abstract Object GetValue(Object obj);
        public abstract void SetValue(Object obj, Object value, BindingFlags invokeAttr, Binder binder, System.Globalization.CultureInfo culture);
        public void SetValue(Object obj, Object value) => SetValue(obj, value, BindingFlags.Default, null, null);
        public bool IsPublic => (Attributes & FieldAttributes.FieldAccessMask) == FieldAttributes.Public;
        public bool IsPrivate => (Attributes & FieldAttributes.FieldAccessMask) == FieldAttributes.Private;
        public bool IsStatic => (Attributes & FieldAttributes.Static) != 0;
        public bool IsInitOnly => (Attributes & FieldAttributes.InitOnly) != 0;
        public bool IsLiteral => (Attributes & FieldAttributes.Literal) != 0;
    }

    // PropertyInfo (ECMA-335)
    public abstract class PropertyInfo : MemberInfo
    {
        protected PropertyInfo() { }
        public override MemberTypes MemberType => MemberTypes.Property;
        public abstract Type PropertyType { get; }
        public abstract PropertyAttributes Attributes { get; }
        public abstract bool CanRead { get; }
        public abstract bool CanWrite { get; }
        public abstract MethodInfo GetGetMethod(bool nonPublic);
        public abstract MethodInfo GetSetMethod(bool nonPublic);
        public MethodInfo GetGetMethod() => GetGetMethod(false);
        public MethodInfo GetSetMethod() => GetSetMethod(false);
        public abstract MethodInfo[] GetAccessors(bool nonPublic);
        public MethodInfo[] GetAccessors() => GetAccessors(false);
        public abstract ParameterInfo[] GetIndexParameters();
        public Object GetValue(Object obj) => GetValue(obj, null);
        public abstract Object GetValue(Object obj, Object[] index);
        public void SetValue(Object obj, Object value) => SetValue(obj, value, null);
        public abstract void SetValue(Object obj, Object value, Object[] index);
    }

    // EventInfo (ECMA-335)
    public abstract class EventInfo : MemberInfo
    {
        protected EventInfo() { }
        public override MemberTypes MemberType => MemberTypes.Event;
        public abstract EventAttributes Attributes { get; }
        public abstract Type EventHandlerType { get; }
        public abstract MethodInfo GetAddMethod(bool nonPublic);
        public abstract MethodInfo GetRemoveMethod(bool nonPublic);
        public abstract MethodInfo GetRaiseMethod(bool nonPublic);
        public MethodInfo GetAddMethod() => GetAddMethod(false);
        public MethodInfo GetRemoveMethod() => GetRemoveMethod(false);
        public MethodInfo GetRaiseMethod() => GetRaiseMethod(false);
        public void AddEventHandler(Object target, Delegate handler) { }
        public void RemoveEventHandler(Object target, Delegate handler) { }
        public bool IsMulticast => true;
    }

    // ParameterInfo (ECMA-335)
    public class ParameterInfo
    {
        protected ParameterInfo() { }
        public virtual String Name { get; protected set; }
        public virtual Type ParameterType { get; protected set; }
        public virtual int Position { get; protected set; }
        public virtual ParameterAttributes Attributes { get; protected set; }
        public virtual Object DefaultValue { get; protected set; }
        public virtual MemberInfo Member { get; protected set; }
        public bool IsIn => (Attributes & ParameterAttributes.In) != 0;
        public bool IsOut => (Attributes & ParameterAttributes.Out) != 0;
        public bool IsOptional => (Attributes & ParameterAttributes.Optional) != 0;
        public virtual Object[] GetCustomAttributes(bool inherit) => Array.Empty<Object>();
        public virtual Object[] GetCustomAttributes(Type attributeType, bool inherit) => Array.Empty<Object>();
        public virtual bool IsDefined(Type attributeType, bool inherit) => false;
    }

    // Assembly (ECMA-335)
    public abstract class Assembly
    {
        protected Assembly() { }
        public virtual String FullName => null;
        public virtual String Location => null;
        public virtual AssemblyName GetName() => null;
        public virtual Type[] GetTypes() => Array.Empty<Type>();
        public virtual Type[] GetExportedTypes() => Array.Empty<Type>();
        public virtual Type GetType(String name) => null;
        public virtual Type GetType(String name, bool throwOnError) => GetType(name, throwOnError, false);
        public virtual Type GetType(String name, bool throwOnError, bool ignoreCase) => null;
        public static Assembly GetExecutingAssembly() => null;
        public static Assembly GetCallingAssembly() => null;
        public static Assembly GetEntryAssembly() => null;
        public static Assembly Load(String assemblyString) => null;
        public static Assembly Load(AssemblyName assemblyRef) => null;
        public static Assembly LoadFrom(String assemblyFile) => null;
        public virtual Object[] GetCustomAttributes(bool inherit) => Array.Empty<Object>();
        public virtual Object[] GetCustomAttributes(Type attributeType, bool inherit) => Array.Empty<Object>();
        public virtual bool IsDefined(Type attributeType, bool inherit) => false;
    }

    // AssemblyName (ECMA-335)
    public sealed class AssemblyName
    {
        public AssemblyName() { }
        public AssemblyName(String assemblyName) { Name = assemblyName; }
        public String Name { get; set; }
        public Version Version { get; set; }
        public String CultureName { get; set; }
        public String FullName => Name;
        public byte[] GetPublicKey() => null;
        public byte[] GetPublicKeyToken() => null;
        public void SetPublicKey(byte[] publicKey) { }
        public void SetPublicKeyToken(byte[] publicKeyToken) { }
        public override String ToString() => FullName;
    }

    // Module (ECMA-335)
    public abstract class Module
    {
        protected Module() { }
        public virtual String Name => null;
        public virtual String FullyQualifiedName => null;
        public virtual Assembly Assembly => null;
        public virtual Type[] GetTypes() => Array.Empty<Type>();
        public virtual Type GetType(String className) => null;
        public virtual Type GetType(String className, bool ignoreCase) => null;
    }

    // Attribute flags
    [Flags] public enum MethodAttributes { MemberAccessMask = 0x0007, Private = 0x0001, FamANDAssem = 0x0002, Assembly = 0x0003, Family = 0x0004, FamORAssem = 0x0005, Public = 0x0006, Static = 0x0010, Final = 0x0020, Virtual = 0x0040, HideBySig = 0x0080, VtableLayoutMask = 0x0100, ReuseSlot = 0x0000, NewSlot = 0x0100, CheckAccessOnOverride = 0x0200, Abstract = 0x0400, SpecialName = 0x0800, PinvokeImpl = 0x2000, RTSpecialName = 0x1000, HasSecurity = 0x4000, RequireSecObject = 0x8000 }
    [Flags] public enum MethodImplAttributes { CodeTypeMask = 0x0003, IL = 0x0000, Native = 0x0001, OPTIL = 0x0002, Runtime = 0x0003, ManagedMask = 0x0004, Unmanaged = 0x0004, Managed = 0x0000, ForwardRef = 0x0010, PreserveSig = 0x0080, InternalCall = 0x1000, Synchronized = 0x0020, NoInlining = 0x0008, AggressiveInlining = 0x0100, NoOptimization = 0x0040 }
    [Flags] public enum FieldAttributes { FieldAccessMask = 0x0007, Private = 0x0001, FamANDAssem = 0x0002, Assembly = 0x0003, Family = 0x0004, FamORAssem = 0x0005, Public = 0x0006, Static = 0x0010, InitOnly = 0x0020, Literal = 0x0040, NotSerialized = 0x0080, SpecialName = 0x0200, PinvokeImpl = 0x2000, RTSpecialName = 0x0400, HasFieldMarshal = 0x1000, HasDefault = 0x8000, HasFieldRVA = 0x0100 }
    [Flags] public enum PropertyAttributes { None = 0, SpecialName = 0x0200, RTSpecialName = 0x0400, HasDefault = 0x1000 }
    [Flags] public enum EventAttributes { None = 0, SpecialName = 0x0200, RTSpecialName = 0x0400 }
    [Flags] public enum ParameterAttributes { None = 0, In = 0x0001, Out = 0x0002, Lcid = 0x0004, Retval = 0x0008, Optional = 0x0010, HasDefault = 0x1000, HasFieldMarshal = 0x2000 }
    [Flags] public enum BindingFlags { Default = 0, IgnoreCase = 1, DeclaredOnly = 2, Instance = 4, Static = 8, Public = 16, NonPublic = 32, FlattenHierarchy = 64, InvokeMethod = 256, CreateInstance = 512, GetField = 1024, SetField = 2048, GetProperty = 4096, SetProperty = 8192, PutDispProperty = 16384, PutRefDispProperty = 32768, ExactBinding = 65536, SuppressChangeType = 131072 }
    [Flags] public enum CallingConventions { Standard = 1, VarArgs = 2, Any = 3, HasThis = 32, ExplicitThis = 64 }

    // ICustomAttributeProvider (ECMA-335)
    public interface ICustomAttributeProvider
    {
        Object[] GetCustomAttributes(bool inherit);
        Object[] GetCustomAttributes(Type attributeType, bool inherit);
        bool IsDefined(Type attributeType, bool inherit);
    }

    // CustomAttributeData
    public class CustomAttributeData
    {
        protected CustomAttributeData() { }
        public virtual Type AttributeType => null;
        public virtual ConstructorInfo Constructor => null;
    }

    // Binder
    public abstract class Binder
    {
        protected Binder() { }
        public abstract MethodBase BindToMethod(BindingFlags bindingAttr, MethodBase[] match, ref Object[] args, ParameterModifier[] modifiers, System.Globalization.CultureInfo culture, String[] names, out Object state);
        public abstract FieldInfo BindToField(BindingFlags bindingAttr, FieldInfo[] match, Object value, System.Globalization.CultureInfo culture);
        public abstract MethodBase SelectMethod(BindingFlags bindingAttr, MethodBase[] match, Type[] types, ParameterModifier[] modifiers);
        public abstract PropertyInfo SelectProperty(BindingFlags bindingAttr, PropertyInfo[] match, Type returnType, Type[] indexes, ParameterModifier[] modifiers);
        public abstract Object ChangeType(Object value, Type type, System.Globalization.CultureInfo culture);
        public abstract void ReorderArgumentArray(ref Object[] args, Object state);
    }

    // ParameterModifier
    public struct ParameterModifier
    {
        private bool[] _byRef;
        public ParameterModifier(int parameterCount) { _byRef = new bool[parameterCount]; }
        public bool this[int index] { get => _byRef[index]; set => _byRef[index] = value; }
    }
}
