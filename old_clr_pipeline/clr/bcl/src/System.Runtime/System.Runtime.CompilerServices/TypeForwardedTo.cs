// Basic TypeForwardedToAttribute for our BCL
namespace System.Runtime.CompilerServices
{
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple = true, Inherited = false)]
    public sealed class TypeForwardedToAttribute : Attribute
    {
        public TypeForwardedToAttribute(Type destination)
        {
            Destination = destination;
        }

        public Type Destination { get; }
    }
}
