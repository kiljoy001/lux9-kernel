/*
 * Compiler Attributes
 * Lux9 CLR Base Class Library
 */

namespace System.Runtime.CompilerServices
{
    using System;

    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Assembly)]
    public sealed class ExtensionAttribute : Attribute { }
    
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Interface | AttributeTargets.Delegate | AttributeTargets.Method | AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class CompilerGeneratedAttribute : Attribute { }
    
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Method)]
    public sealed class AsyncStateMachineAttribute : Attribute 
    {
        public AsyncStateMachineAttribute(Type stateMachineType)
        {
            StateMachineType = stateMachineType;
        }
        public Type StateMachineType { get; }
    }
    
    public sealed class StateMachineAttribute : Attribute 
    {
        public StateMachineAttribute(Type stateMachineType)
        {
            StateMachineType = stateMachineType;
        }
        public Type StateMachineType { get; }
    }    
}

namespace System
{
    [AttributeUsage(AttributeTargets.Parameter)]
    public sealed class ParamArrayAttribute : Attribute { }

    [AttributeUsage(AttributeTargets.Field)]
    public sealed class NonSerializedAttribute : Attribute { }

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Delegate, Inherited = false)]
    public sealed class SerializableAttribute : Attribute { }
    
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor, Inherited = false)]
    public sealed class ObsoleteAttribute : Attribute
    {
        public ObsoleteAttribute() { }
        public ObsoleteAttribute(string message) { Message = message; }
        public ObsoleteAttribute(string message, bool error) { Message = message; IsError = error; }
        
        public string Message { get; }
        public bool IsError { get; }
    }
}
