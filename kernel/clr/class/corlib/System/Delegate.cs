/*
 * ECMA-335 Delegate Types - Lux9 BCL
 * 
 * Base delegate types required by the CLI.
 */
namespace System
{
    using System.Reflection;
    // Delegate (ECMA-335 IV.5.67)
    public abstract class Delegate : ICloneable
    {
        internal Object _target;
        internal IntPtr _methodPtr;
        internal IntPtr _methodPtrAux;

        protected Delegate(Object target, String method)
        {
            _target = target;
        }

        protected Delegate(Type target, String method)
        {
        }

        public Object Target => _target;
        public MethodInfo Method => null; // Runtime-implemented

        public virtual Object Clone() => MemberwiseClone();

        public static Delegate Combine(Delegate a, Delegate b)
        {
            if (a == null) return b;
            if (b == null) return a;
            return a.CombineImpl(b);
        }

        public static Delegate Combine(params Delegate[] delegates)
        {
            if (delegates == null || delegates.Length == 0) return null;
            Delegate result = delegates[0];
            for (int i = 1; i < delegates.Length; i++)
                result = Combine(result, delegates[i]);
            return result;
        }

        protected virtual Delegate CombineImpl(Delegate d) =>
            throw new MulticastNotSupportedException();

        public static Delegate Remove(Delegate source, Delegate value)
        {
            if (source == null) return null;
            if (value == null) return source;
            return source.RemoveImpl(value);
        }

        public static Delegate RemoveAll(Delegate source, Delegate value)
        {
            Delegate result = source;
            while (true)
            {
                Delegate newResult = Remove(result, value);
                if (ReferenceEquals(newResult, result)) break;
                result = newResult;
            }
            return result;
        }

        protected virtual Delegate RemoveImpl(Delegate d) =>
            Equals(d) ? null : this;

        public virtual Delegate[] GetInvocationList() => new Delegate[] { this };

        public override bool Equals(Object obj)
        {
            if (obj is Delegate d)
                return _target == d._target && _methodPtr == d._methodPtr;
            return false;
        }

        public override int GetHashCode() => _methodPtr.GetHashCode();

        public static bool operator ==(Delegate d1, Delegate d2) =>
            ReferenceEquals(d1, d2) || (d1?.Equals(d2) ?? false);

        public static bool operator !=(Delegate d1, Delegate d2) => !(d1 == d2);

        // Dynamic invocation
        public Object DynamicInvoke(params Object[] args) => DynamicInvokeImpl(args);
        protected virtual Object DynamicInvokeImpl(Object[] args) => null;
    }

    // MulticastDelegate (ECMA-335 IV.5.68)
    public abstract class MulticastDelegate : Delegate
    {
        private Object _invocationList;  // Array of delegates for multicast
        private IntPtr _invocationCount;

        protected MulticastDelegate(Object target, String method) : base(target, method) { }
        protected MulticastDelegate(Type target, String method) : base(target, method) { }

        protected override Delegate CombineImpl(Delegate follow)
        {
            if (follow == null) return this;
            if (!(follow is MulticastDelegate mcd))
                throw new ArgumentException("Incompatible delegate type");

            MulticastDelegate result = (MulticastDelegate)Clone();
            if (_invocationList == null)
            {
                result._invocationList = new Delegate[] { this, mcd };
                result._invocationCount = (IntPtr)2;
            }
            else
            {
                Delegate[] oldList = (Delegate[])_invocationList;
                Delegate[] newList = new Delegate[oldList.Length + 1];
                Array.Copy(oldList, newList, oldList.Length);
                newList[oldList.Length] = mcd;
                result._invocationList = newList;
                result._invocationCount = (IntPtr)newList.Length;
            }
            return result;
        }

        protected override Delegate RemoveImpl(Delegate value)
        {
            if (_invocationList == null)
                return Equals(value) ? null : this;

            Delegate[] list = (Delegate[])_invocationList;
            int idx = -1;
            for (int i = list.Length - 1; i >= 0; i--)
            {
                if (list[i].Equals(value)) { idx = i; break; }
            }
            if (idx < 0) return this;
            if (list.Length == 2) return list[1 - idx];

            Delegate[] newList = new Delegate[list.Length - 1];
            Array.Copy(list, 0, newList, 0, idx);
            Array.Copy(list, idx + 1, newList, idx, list.Length - idx - 1);

            MulticastDelegate result = (MulticastDelegate)Clone();
            result._invocationList = newList;
            result._invocationCount = (IntPtr)newList.Length;
            return result;
        }

        public override Delegate[] GetInvocationList()
        {
            if (_invocationList == null)
                return new Delegate[] { this };
            return (Delegate[])((Delegate[])_invocationList).Clone();
        }

        public override bool Equals(Object obj)
        {
            if (!(obj is MulticastDelegate mcd)) return false;
            if (_invocationList == null && mcd._invocationList == null)
                return base.Equals(obj);
            if (_invocationList == null || mcd._invocationList == null)
                return false;

            Delegate[] list1 = (Delegate[])_invocationList;
            Delegate[] list2 = (Delegate[])mcd._invocationList;
            if (list1.Length != list2.Length) return false;
            for (int i = 0; i < list1.Length; i++)
            {
                if (!list1[i].Equals(list2[i])) return false;
            }
            return true;
        }

        public override int GetHashCode()
        {
            if (_invocationList == null) return base.GetHashCode();
            Delegate[] list = (Delegate[])_invocationList;
            int hash = 0;
            for (int i = 0; i < list.Length; i++)
                hash ^= list[i].GetHashCode();
            return hash;
        }
    }

    // MulticastNotSupportedException
    public sealed class MulticastNotSupportedException : SystemException
    {
        public MulticastNotSupportedException() : base("Attempted to add multiple callbacks to a non-multicast delegate.") { }
        public MulticastNotSupportedException(String message) : base(message) { }
        public MulticastNotSupportedException(String message, Exception innerException) : base(message, innerException) { }
    }

    // AsyncCallback (ECMA-335)
    public delegate void AsyncCallback(IAsyncResult ar);

    // EventHandler (ECMA-335)
    public delegate void EventHandler(Object sender, EventArgs e);
    public delegate void EventHandler<TEventArgs>(Object sender, TEventArgs e);

    // EventArgs (ECMA-335)
    public class EventArgs
    {
        public static readonly EventArgs Empty = new EventArgs();
        public EventArgs() { }
    }
}
