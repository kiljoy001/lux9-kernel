/*
 * ECMA-335 Nullable Type - Lux9 BCL
 */
namespace System
{
    using System.Collections.Generic;

    // Nullable<T> (ECMA-335)
    public struct Nullable<T> where T : struct
    {
        private bool _hasValue;
        private T _value;

        public Nullable(T value)
        {
            _value = value;
            _hasValue = true;
        }

        public bool HasValue => _hasValue;

        public T Value
        {
            get
            {
                if (!_hasValue) throw new InvalidOperationException("Nullable object must have a value.");
                return _value;
            }
        }

        public T GetValueOrDefault() => _value;
        public T GetValueOrDefault(T defaultValue) => _hasValue ? _value : defaultValue;

        public override bool Equals(Object other)
        {
            if (!_hasValue) return other == null;
            if (other == null) return false;
            return _value.Equals(other);
        }

        public override int GetHashCode() => _hasValue ? _value.GetHashCode() : 0;

        public override String ToString() => _hasValue ? _value.ToString() : "";

        public static implicit operator Nullable<T>(T value) => new Nullable<T>(value);
        public static explicit operator T(Nullable<T> value) => value.Value;
    }

    // Nullable helper class
    public static class Nullable
    {
        public static int Compare<T>(Nullable<T> n1, Nullable<T> n2) where T : struct
        {
            if (n1.HasValue)
            {
                if (n2.HasValue) return Comparer<T>.Default.Compare(n1.Value, n2.Value);
                return 1;
            }
            return n2.HasValue ? -1 : 0;
        }

        public static bool Equals<T>(Nullable<T> n1, Nullable<T> n2) where T : struct
        {
            if (n1.HasValue)
            {
                if (n2.HasValue) return EqualityComparer<T>.Default.Equals(n1.Value, n2.Value);
                return false;
            }
            return !n2.HasValue;
        }

        public static Type GetUnderlyingType(Type nullableType)
        {
            if (nullableType == null) throw new ArgumentNullException(nameof(nullableType));
            if (nullableType.IsGenericType && nullableType.GetGenericTypeDefinition() == typeof(Nullable<>))
            {
                return nullableType.GetGenericArguments()[0];
            }
            return null;
        }
    }
}
