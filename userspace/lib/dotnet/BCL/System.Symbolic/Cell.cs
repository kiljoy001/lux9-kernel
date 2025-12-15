namespace System.Symbolic;

public class Cell : ISymbolic
{
    // These should compile to 'ldfld' / 'stfld' operations on fields
    public ISymbolic Car { get; set; }
    public ISymbolic Cdr { get; set; }

    public bool IsAtom => false;
    public bool IsNil => false;

    // Constructor compiles to 'newobj' followed by 'stfld' calls
    public Cell(ISymbolic car, ISymbolic cdr)
    {
        Car = car;
        Cdr = cdr;
    }

    // Example method to verify virtual calls
    public override string ToString()
    {
        return $"({Car} . {Cdr})";
    }
}
