/*
 * ECMA-335 Globalization Types - Lux9 BCL
 */
namespace System.Globalization
{
    // CultureInfo (ECMA-335)
    public class CultureInfo : IFormatProvider, ICloneable
    {
        private static CultureInfo _invariantCulture;
        private static CultureInfo _currentCulture;

        public String Name { get; }
        public String DisplayName { get; }
        public String EnglishName { get; }
        public String NativeName { get; }
        public int LCID { get; }
        public bool IsNeutralCulture { get; }
        public virtual CultureInfo Parent => InvariantCulture;
        public virtual NumberFormatInfo NumberFormat { get; set; }
        public virtual DateTimeFormatInfo DateTimeFormat { get; set; }
        public virtual TextInfo TextInfo => null;
        public virtual CompareInfo CompareInfo => null;

        public CultureInfo(String name)
        {
            Name = name ?? "";
            DisplayName = name ?? "Invariant";
            EnglishName = name ?? "Invariant Language";
            NativeName = name ?? "Invariant";
        }

        public CultureInfo(int culture)
        {
            LCID = culture;
            Name = "";
        }

        public static CultureInfo InvariantCulture =>
            _invariantCulture ??= new CultureInfo("");

        public static CultureInfo CurrentCulture
        {
            get => _currentCulture ?? InvariantCulture;
            set => _currentCulture = value;
        }

        public static CultureInfo CurrentUICulture
        {
            get => CurrentCulture;
            set => CurrentCulture = value;
        }

        public virtual Object GetFormat(Type formatType)
        {
            if (formatType == typeof(NumberFormatInfo)) return NumberFormat;
            if (formatType == typeof(DateTimeFormatInfo)) return DateTimeFormat;
            return null;
        }

        public virtual Object Clone()
        {
            return new CultureInfo(Name);
        }

        public override String ToString() => Name;
    }

    // NumberFormatInfo (ECMA-335)
    public sealed class NumberFormatInfo : IFormatProvider, ICloneable
    {
        public String CurrencyDecimalSeparator { get; set; } = ".";
        public String CurrencyGroupSeparator { get; set; } = ",";
        public int CurrencyDecimalDigits { get; set; } = 2;
        public String CurrencySymbol { get; set; } = "$";
        public String NumberDecimalSeparator { get; set; } = ".";
        public String NumberGroupSeparator { get; set; } = ",";
        public int NumberDecimalDigits { get; set; } = 2;
        public String NegativeSign { get; set; } = "-";
        public String PositiveSign { get; set; } = "+";
        public String PercentSymbol { get; set; } = "%";
        public String PerMilleSymbol { get; set; } = "‰";
        public String NaNSymbol { get; set; } = "NaN";
        public String PositiveInfinitySymbol { get; set; } = "Infinity";
        public String NegativeInfinitySymbol { get; set; } = "-Infinity";

        public static NumberFormatInfo InvariantInfo { get; } = new NumberFormatInfo();
        public static NumberFormatInfo CurrentInfo => InvariantInfo;
        public Object GetFormat(Type formatType) => formatType == typeof(NumberFormatInfo) ? this : null;
        public Object Clone() => MemberwiseClone();
    }

    // DateTimeFormatInfo (ECMA-335)
    public sealed class DateTimeFormatInfo : IFormatProvider, ICloneable
    {
        public String DateSeparator { get; set; } = "/";
        public String TimeSeparator { get; set; } = ":";
        public String ShortDatePattern { get; set; } = "M/d/yyyy";
        public String LongDatePattern { get; set; } = "dddd, MMMM d, yyyy";
        public String ShortTimePattern { get; set; } = "h:mm tt";
        public String LongTimePattern { get; set; } = "h:mm:ss tt";
        public String FullDateTimePattern { get; set; } = "dddd, MMMM d, yyyy h:mm:ss tt";
        public String[] DayNames { get; set; } = new[] { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
        public String[] MonthNames { get; set; } = new[] { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December", "" };

        public static DateTimeFormatInfo InvariantInfo { get; } = new DateTimeFormatInfo();
        public static DateTimeFormatInfo CurrentInfo => InvariantInfo;
        public Object GetFormat(Type formatType) => formatType == typeof(DateTimeFormatInfo) ? this : null;
        public Object Clone() => MemberwiseClone();
    }

    // TextInfo (ECMA-335)
    public class TextInfo
    {
        public virtual String ToLower(String str) => str?.ToLower();
        public virtual String ToUpper(String str) => str?.ToUpper();
        public virtual char ToLower(char c) => Char.ToLower(c);
        public virtual char ToUpper(char c) => Char.ToUpper(c);
    }

    // CompareInfo (ECMA-335)
    public class CompareInfo
    {
        public virtual int Compare(String string1, String string2) =>
            String.CompareOrdinal(string1, string2);
        public virtual int Compare(String string1, String string2, CompareOptions options) =>
            Compare(string1, string2);
        public virtual bool IsPrefix(String source, String prefix) =>
            source?.StartsWith(prefix) ?? prefix == null;
        public virtual bool IsSuffix(String source, String suffix) =>
            source?.EndsWith(suffix) ?? suffix == null;
        public virtual int IndexOf(String source, char value) =>
            source?.IndexOf(value) ?? -1;
    }

    [Flags]
    public enum CompareOptions
    {
        None = 0,
        IgnoreCase = 1,
        IgnoreNonSpace = 2,
        IgnoreSymbols = 4,
        IgnoreKanaType = 8,
        IgnoreWidth = 16,
        OrdinalIgnoreCase = 0x10000000,
        StringSort = 0x20000000,
        Ordinal = 0x40000000
    }

    public enum NumberStyles
    {
        None = 0,
        AllowLeadingWhite = 1,
        AllowTrailingWhite = 2,
        AllowLeadingSign = 4,
        AllowTrailingSign = 8,
        AllowParentheses = 16,
        AllowDecimalPoint = 32,
        AllowThousands = 64,
        AllowExponent = 128,
        AllowCurrencySymbol = 256,
        AllowHexSpecifier = 512,
        Integer = 7,
        HexNumber = 515,
        Number = 111,
        Float = 167,
        Currency = 383,
        Any = 511
    }

    public enum DateTimeStyles
    {
        None = 0,
        AllowLeadingWhite = 1,
        AllowTrailingWhite = 2,
        AllowInnerWhite = 4,
        AllowWhiteSpaces = 7,
        NoCurrentDateDefault = 8,
        AdjustToUniversal = 16,
        AssumeLocal = 32,
        AssumeUniversal = 64,
        RoundtripKind = 128
    }
}
