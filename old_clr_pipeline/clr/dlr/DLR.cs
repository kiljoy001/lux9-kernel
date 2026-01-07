namespace System.Linq.Expressions
{
    using System;
    using System.Runtime.CompilerServices;

    public abstract class Expression
    {
        public abstract ExpressionType NodeType { get; }
        public abstract Type Type { get; }

        public static ConstantExpression Constant(object value) 
        {
            return new ConstantExpression(value);
        }

        // ... myriad other factory methods ...
    }

    public enum ExpressionType
    {
        Add, Subtract, Multiply, Divide, Equal, Constant, Parameter, Call, Lambda // etc
    }

    public class ConstantExpression : Expression
    {
        internal ConstantExpression(object value) { Value = value; }
        public object Value { get; }
        public override ExpressionType NodeType => ExpressionType.Constant;
        public override Type Type => Value?.GetType() ?? typeof(object);
    }

    public class ParameterExpression : Expression
    {
        internal ParameterExpression(Type type, string name) { Type = type; Name = name; }
        public string Name { get; }
        public override Type Type { get; }
        public override ExpressionType NodeType => ExpressionType.Parameter;
    }

    public class LabelTarget
    {
        internal LabelTarget(Type type, string name) { Type = type; Name = name; }
        public string Name { get; }
        public Type Type { get; }
    }

    public delegate TDelegate CallSiteShim<TDelegate>(CallSite site, object arg);
}

namespace System.Runtime.CompilerServices
{
    using System.Linq.Expressions;

    public class CallSite
    {
        public CallSiteBinder Binder { get; }
        // Contains the cache and the delegate to the target
    }

    public class CallSite<T> : CallSite where T : class
    {
        public T Target; // The fast-path delegate
        
        public static CallSite<T> Create(CallSiteBinder binder)
        {
            return new CallSite<T>(binder);
        }

        private CallSite(CallSiteBinder binder)
        {
             // Binder handles cache miss
        }
    }

    public abstract class CallSiteBinder
    {
        public abstract Expression Bind(object[] args, System.Collections.ObjectModel.ReadOnlyCollection<ParameterExpression> parameters, LabelTarget returnLabel);
    }
}

// Minimal Collections stub for DLR
namespace System.Collections.ObjectModel
{
    public class ReadOnlyCollection<T> : System.Collections.Generic.IEnumerable<T> 
    {
        // Stub
    }
}

namespace System.Collections.Generic
{
    public interface IEnumerable<T>
    {
        // IEnumerator<T> GetEnumerator();
    }
}
