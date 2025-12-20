namespace System.Symbolic;

public abstract class Atom : ISymbolic
{
    public bool IsAtom => true;
    public virtual bool IsNil => false;
}

public class Symbol : Atom
{
    public string Name { get; }

    public Symbol(string name)
    {
        Name = name;
    }

    public override string ToString() => Name;
}

public class Nil : Atom
{
    // Singleton instance
    public static readonly Nil Instance = new Nil();

    private Nil() { }

    public override bool IsNil => true;

    public override string ToString() => "nil";
}
