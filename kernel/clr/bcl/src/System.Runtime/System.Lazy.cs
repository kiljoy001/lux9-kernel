namespace System
{
    public class Lazy<T>
    {
        private bool _isValueCreated;
        private T _value;
        private Func<T> _valueFactory;

        public Lazy(Func<T> valueFactory)
        {
            _valueFactory = valueFactory ?? throw new ArgumentNullException();
            _isValueCreated = false;
        }

        public Lazy(T value)
        {
            _value = value;
            _isValueCreated = true;
        }

        public bool IsValueCreated => _isValueCreated;

        public T Value
        {
            get
            {
                if (!_isValueCreated)
                {
                    _value = _valueFactory();
                    _isValueCreated = true;
                }
                return _value;
            }
        }
    }
}
