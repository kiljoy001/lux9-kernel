/*
 * System.Reflection - Reflection types
 * Lux9 CLR Base Class Library
 */
namespace System.Reflection
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// Provides information about members of a type.
    /// </summary>
    public abstract class MemberInfo
    {
        public abstract string Name { get; }
        public abstract Type DeclaringType { get; }

        public abstract Type ReflectedType { get; }
        public abstract MemberTypes MemberType { get; }
        
        public virtual object[] GetCustomAttributes(bool inherit) => new object[0];
        public virtual object[] GetCustomAttributes(Type attributeType, bool inherit) => new object[0];
        public virtual bool IsDefined(Type attributeType, bool inherit) => false;
        
        public virtual int MetadataToken => 0;
        public virtual Module Module => null;
    }
}

namespace System
{
    using System.Reflection;

    /// <summary>
    /// Represents type declarations.
    /// </summary>
    public abstract class Type : MemberInfo
    {
        public override MemberTypes MemberType => MemberTypes.TypeInfo;
        
        public abstract string Namespace { get; }
        public abstract string FullName { get; }
        public abstract Type BaseType { get; }
        public abstract Assembly Assembly { get; }
        public abstract Module Module { get; }
        
        // Type characteristics
        public abstract bool IsClass { get; }
        public abstract bool IsValueType { get; }
        public abstract bool IsInterface { get; }
        public abstract bool IsArray { get; }
        public abstract bool IsEnum { get; }
        public abstract bool IsPrimitive { get; }
        public abstract bool IsGenericType { get; }
        public abstract bool IsGenericTypeDefinition { get; }
        public abstract bool IsAbstract { get; }
        public abstract bool IsSealed { get; }
        public abstract bool IsPublic { get; }
        public abstract bool IsNested { get; }
        
        // Members
        public abstract FieldInfo[] GetFields();
        public abstract FieldInfo[] GetFields(BindingFlags bindingAttr);
        public abstract FieldInfo GetField(string name);
        public abstract FieldInfo GetField(string name, BindingFlags bindingAttr);
        
        public abstract PropertyInfo[] GetProperties();
        public abstract PropertyInfo[] GetProperties(BindingFlags bindingAttr);
        public abstract PropertyInfo GetProperty(string name);
        public abstract PropertyInfo GetProperty(string name, BindingFlags bindingAttr);
        
        public abstract MethodInfo[] GetMethods();
        public abstract MethodInfo[] GetMethods(BindingFlags bindingAttr);
        public abstract MethodInfo GetMethod(string name);
        public abstract MethodInfo GetMethod(string name, BindingFlags bindingAttr);
        public abstract MethodInfo GetMethod(string name, Type[] types);
        
        public abstract ConstructorInfo[] GetConstructors();
        public abstract ConstructorInfo[] GetConstructors(BindingFlags bindingAttr);
        public abstract ConstructorInfo GetConstructor(Type[] types);
        
        public abstract EventInfo[] GetEvents();
        public abstract EventInfo GetEvent(string name);
        
        public abstract Type[] GetInterfaces();
        public abstract Type[] GetNestedTypes();
        public abstract Type GetNestedType(string name);
        
        public abstract MemberInfo[] GetMembers();
        public abstract MemberInfo[] GetMembers(BindingFlags bindingAttr);
        
        // Generic type support
        public abstract Type[] GetGenericArguments();
        public abstract Type GetGenericTypeDefinition();
        public abstract Type MakeGenericType(params Type[] typeArguments);
        
        // Array support
        public abstract Type GetElementType();
        public abstract int GetArrayRank();
        public abstract Type MakeArrayType();
        public abstract Type MakeArrayType(int rank);
        
        // Instance creation
        public object GetDefaultValue() => IsValueType ? Activator.CreateInstance(this) : null;
        
        // Type checking
        public virtual bool IsAssignableFrom(Type c) => false;
        public virtual bool IsSubclassOf(Type c) => false;
        public virtual bool IsInstanceOfType(object o) => o != null && IsAssignableFrom(o.GetType());
        
        // Static helpers
        public static Type GetType(string typeName) => GetType(typeName, false);
        public static Type GetType(string typeName, bool throwOnError) => null; // Stub
        
        public static Type GetTypeFromHandle(RuntimeTypeHandle handle) => null; // InternalCall
        
        public override string ToString() => FullName ?? Name;
        
        public static bool operator ==(Type left, Type right)
        {
            if ((object)left == null) return (object)right == null;
            if ((object)right == null) return false;
            return left.Equals(right);
        }
        
        public static bool operator !=(Type left, Type right) => !(left == right);
    }
}

namespace System.Reflection
{
    using System;

    /// <summary>
    /// Represents a method.
    /// </summary>
    public abstract class MethodInfo : MethodBase
    {
        public override MemberTypes MemberType => MemberTypes.Method;
        
        public abstract Type ReturnType { get; }
        public abstract ParameterInfo ReturnParameter { get; }
        
        public abstract MethodInfo GetBaseDefinition();
        public abstract MethodInfo MakeGenericMethod(params Type[] typeArguments);
        
        public abstract object Invoke(object obj, object[] parameters);
        
        public virtual Delegate CreateDelegate(Type delegateType) => null;
        public virtual Delegate CreateDelegate(Type delegateType, object target) => null;
    }

    /// <summary>
    /// Represents a constructor.
    /// </summary>
    public abstract class ConstructorInfo : MethodBase
    {
        public override MemberTypes MemberType => MemberTypes.Constructor;
        
        public abstract object Invoke(object[] parameters);
        
        public static readonly string ConstructorName = ".ctor";
        public static readonly string TypeConstructorName = ".cctor";
    }

    /// <summary>
    /// Base class for methods and constructors.
    /// </summary>
    public abstract class MethodBase : MemberInfo
    {
        public abstract MethodAttributes Attributes { get; }
        public abstract CallingConventions CallingConvention { get; }
        public abstract ParameterInfo[] GetParameters();
        
        public bool IsPublic => (Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.Public;
        public bool IsPrivate => (Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.Private;
        public bool IsStatic => (Attributes & MethodAttributes.Static) != 0;
        public bool IsAbstract => (Attributes & MethodAttributes.Abstract) != 0;
        public bool IsVirtual => (Attributes & MethodAttributes.Virtual) != 0;
        public bool IsFinal => (Attributes & MethodAttributes.Final) != 0;
        public bool IsSpecialName => (Attributes & MethodAttributes.SpecialName) != 0;
        
        public abstract bool IsGenericMethod { get; }
        public abstract bool IsGenericMethodDefinition { get; }
        public abstract Type[] GetGenericArguments();
    }

    /// <summary>
    /// Represents a field.
    /// </summary>
    public abstract class FieldInfo : MemberInfo
    {
        public override MemberTypes MemberType => MemberTypes.Field;
        
        public abstract Type FieldType { get; }
        public abstract FieldAttributes Attributes { get; }
        
        public bool IsPublic => (Attributes & FieldAttributes.FieldAccessMask) == FieldAttributes.Public;
        public bool IsPrivate => (Attributes & FieldAttributes.FieldAccessMask) == FieldAttributes.Private;
        public bool IsStatic => (Attributes & FieldAttributes.Static) != 0;
        public bool IsInitOnly => (Attributes & FieldAttributes.InitOnly) != 0;
        public bool IsLiteral => (Attributes & FieldAttributes.Literal) != 0;
        
        public abstract object GetValue(object obj);
        public abstract void SetValue(object obj, object value);
    }

    /// <summary>
    /// Represents a property.
    /// </summary>
    public abstract class PropertyInfo : MemberInfo
    {
        public override MemberTypes MemberType => MemberTypes.Property;
        
        public abstract Type PropertyType { get; }
        public abstract PropertyAttributes Attributes { get; }
        
        public abstract bool CanRead { get; }
        public abstract bool CanWrite { get; }
        
        public abstract MethodInfo GetGetMethod();
        public abstract MethodInfo GetGetMethod(bool nonPublic);
        public abstract MethodInfo GetSetMethod();
        public abstract MethodInfo GetSetMethod(bool nonPublic);
        public abstract ParameterInfo[] GetIndexParameters();
        
        public object GetValue(object obj) => GetValue(obj, null);
        public abstract object GetValue(object obj, object[] index);
        
        public void SetValue(object obj, object value) => SetValue(obj, value, null);
        public abstract void SetValue(object obj, object value, object[] index);
    }

    /// <summary>
    /// Represents an event.
    /// </summary>
    public abstract class EventInfo : MemberInfo
    {
        public override MemberTypes MemberType => MemberTypes.Event;
        
        public abstract Type EventHandlerType { get; }
        public abstract EventAttributes Attributes { get; }
        
        public abstract MethodInfo GetAddMethod();
        public abstract MethodInfo GetAddMethod(bool nonPublic);
        public abstract MethodInfo GetRemoveMethod();
        public abstract MethodInfo GetRemoveMethod(bool nonPublic);
        public abstract MethodInfo GetRaiseMethod();
        public abstract MethodInfo GetRaiseMethod(bool nonPublic);
        
        public abstract void AddEventHandler(object target, Delegate handler);
        public abstract void RemoveEventHandler(object target, Delegate handler);
    }

    /// <summary>
    /// Represents a method or constructor parameter.
    /// </summary>
    public class ParameterInfo
    {
        public virtual string Name { get; }
        public virtual Type ParameterType { get; }
        public virtual int Position { get; }
        public virtual ParameterAttributes Attributes { get; }
        public virtual bool HasDefaultValue { get; }
        public virtual object DefaultValue { get; }
        
        public bool IsIn => (Attributes & ParameterAttributes.In) != 0;
        public bool IsOut => (Attributes & ParameterAttributes.Out) != 0;
        public bool IsOptional => (Attributes & ParameterAttributes.Optional) != 0;
    }

    /// <summary>
    /// Represents an assembly.
    /// </summary>
    public abstract class Assembly
    {
        public abstract string FullName { get; }
        public abstract string Location { get; }
        
        public abstract Type[] GetTypes();
        public abstract Type[] GetExportedTypes();
        public abstract Type GetType(string name);
        public abstract Type GetType(string name, bool throwOnError);
        
        public abstract AssemblyName GetName();
        public abstract AssemblyName[] GetReferencedAssemblies();
        
        public abstract Module[] GetModules();
        public abstract Module GetModule(string name);
        
        public static Assembly GetExecutingAssembly() => null;
        public static Assembly GetCallingAssembly() => null;
        public static Assembly GetEntryAssembly() => null;
        
        public static Assembly Load(string assemblyString) => null;
        public static Assembly Load(AssemblyName assemblyRef) => null;
        public static Assembly LoadFrom(string assemblyFile) => null;
    }

    /// <summary>
    /// Represents the name of an assembly.
    /// </summary>
    public sealed class AssemblyName
    {
        public string Name { get; set; }
        public string Version { get; set; }
        public string CultureName { get; set; }
        public string CodeBase { get; set; }
        
        public AssemblyName() { }
        public AssemblyName(string assemblyName) { Name = assemblyName; }
        
        public string FullName => Name;
        
        public override string ToString() => FullName;
    }

    /// <summary>
    /// Represents a module in the common language runtime.
    /// </summary>
    public abstract class Module
    {
        public abstract string Name { get; }
        public abstract Assembly Assembly { get; }
        public abstract string FullyQualifiedName { get; }
        
        public abstract Type[] GetTypes();
        public abstract Type GetType(string className);
        
        public abstract FieldInfo[] GetFields();
        public abstract MethodInfo[] GetMethods();
    }

}

namespace System
{
    using System.Reflection;

    /// <summary>
    /// Contains methods to create types at runtime.
    /// </summary>
    public static class Activator
    {
        public static object CreateInstance(Type type)
        {
            if (type == null) throw new ArgumentNullException(nameof(type));
            return CreateInstance(type, new object[0]);
        }
        
        public static object CreateInstance(Type type, params object[] args)
        {
            if (type == null) throw new ArgumentNullException(nameof(type));
            // Would call constructor via reflection
            return null;
        }
        
        public static T CreateInstance<T>()
        {
            return (T)CreateInstance(typeof(T));
        }
    }
}

namespace System.Reflection
{

    // Enums and flags
    [Flags]
    public enum BindingFlags
    {
        Default = 0,
        IgnoreCase = 1,
        DeclaredOnly = 2,
        Instance = 4,
        Static = 8,
        Public = 16,
        NonPublic = 32,
        FlattenHierarchy = 64,
        InvokeMethod = 256,
        CreateInstance = 512,
        GetField = 1024,
        SetField = 2048,
        GetProperty = 4096,
        SetProperty = 8192
    }
    
    [Flags]
    public enum MemberTypes
    {
        Constructor = 1,
        Event = 2,
        Field = 4,
        Method = 8,
        Property = 16,
        TypeInfo = 32,
        Custom = 64,
        NestedType = 128,
        All = 191
    }
    
    [Flags]
    public enum MethodAttributes
    {
        MemberAccessMask = 7,
        PrivateScope = 0,
        Private = 1,
        FamANDAssem = 2,
        Assembly = 3,
        Family = 4,
        FamORAssem = 5,
        Public = 6,
        Static = 16,
        Final = 32,
        Virtual = 64,
        HideBySig = 128,
        VtableLayoutMask = 256,
        ReuseSlot = 0,
        NewSlot = 256,
        Abstract = 1024,
        SpecialName = 2048,
        PinvokeImpl = 8192,
        UnmanagedExport = 8
    }
    
    [Flags]
    public enum FieldAttributes
    {
        FieldAccessMask = 7,
        PrivateScope = 0,
        Private = 1,
        FamANDAssem = 2,
        Assembly = 3,
        Family = 4,
        FamORAssem = 5,
        Public = 6,
        Static = 16,
        InitOnly = 32,
        Literal = 64,
        NotSerialized = 128,
        SpecialName = 512
    }
    
    [Flags]
    public enum PropertyAttributes
    {
        None = 0,
        SpecialName = 512,
        RTSpecialName = 1024,
        HasDefault = 4096
    }
    
    [Flags]
    public enum EventAttributes
    {
        None = 0,
        SpecialName = 512,
        RTSpecialName = 1024
    }
    
    [Flags]
    public enum ParameterAttributes
    {
        None = 0,
        In = 1,
        Out = 2,
        Lcid = 4,
        Retval = 8,
        Optional = 16,
        HasDefault = 4096,
        HasFieldMarshal = 8192
    }
    
    public enum CallingConventions
    {
        Standard = 1,
        VarArgs = 2,
        Any = 3,
        HasThis = 32,
        ExplicitThis = 64
    }

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Interface)]
    public sealed class DefaultMemberAttribute : Attribute
    {
        public DefaultMemberAttribute(string memberName)
        {
            MemberName = memberName;
        }

        public string MemberName { get; }
    }
}


